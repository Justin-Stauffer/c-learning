# Chapter 4 Example Specifications

Reference file for creating Timer and PWM example projects. Read this before creating each example.

## Common Files Needed Per Project

Each example folder needs:
1. `main.c` - The example code
2. `Makefile` - Build configuration (copy template, change PROJECT name)
3. `startup_lpc1343_gcc.s` - Copy from 00-Getting-Started
4. `lpc1343_flash.ld` - Copy from 00-Getting-Started
5. `README.md` - Brief description of the example

## Hardware Configuration

```
System Clock: 72 MHz (assumed)

LEDs (Active-Low):
  P3.0 - LED0
  P3.1 - LED1
  P3.2 - LED2
  P3.3 - LED3
  LED_MASK = 0x0F

Button:
  P0.1 - Main button (active-low)

Timer PWM Pins:
  CT32B0_MAT0 - P1.6 (IOCON = 0x02)
  CT32B0_MAT1 - P1.7 (IOCON = 0x02)
  CT32B0_MAT2 - P0.1 (IOCON = 0x02) - conflicts with button!
  CT32B0_MAT3 - P0.11 (IOCON = 0x03)

  CT32B1_MAT0 - P1.1 (IOCON = 0x03)
  CT32B1_MAT1 - P1.2 (IOCON = 0x03)
  CT32B1_MAT2 - P1.3 (IOCON = 0x03)
  CT32B1_MAT3 - P1.4 (IOCON = 0x02)
```

## Register Addresses

```c
/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* IOCON for PWM pins */
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))
#define IOCON_PIO1_7   (*((volatile uint32_t *)0x400440A8))

/* CT32B0 Registers (base 0x40014000) */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))  /* Interrupt */
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))  /* Control */
#define TMR32B0TC      (*((volatile uint32_t *)0x40014008))  /* Counter */
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))  /* Prescaler */
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))  /* Match Control */
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))  /* Match 0 */
#define TMR32B0MR1     (*((volatile uint32_t *)0x4001401C))  /* Match 1 */
#define TMR32B0MR2     (*((volatile uint32_t *)0x40014020))  /* Match 2 */
#define TMR32B0MR3     (*((volatile uint32_t *)0x40014024))  /* Match 3 */
#define TMR32B0EMR     (*((volatile uint32_t *)0x4001403C))  /* External Match */
#define TMR32B0PWMC    (*((volatile uint32_t *)0x40014074))  /* PWM Control */

/* CT32B1 Registers (base 0x40018000) */
#define TMR32B1IR      (*((volatile uint32_t *)0x40018000))
#define TMR32B1TCR     (*((volatile uint32_t *)0x40018004))
#define TMR32B1TC      (*((volatile uint32_t *)0x40018008))
#define TMR32B1PR      (*((volatile uint32_t *)0x4001800C))
#define TMR32B1MCR     (*((volatile uint32_t *)0x40018014))
#define TMR32B1MR0     (*((volatile uint32_t *)0x40018018))
#define TMR32B1MR1     (*((volatile uint32_t *)0x4001801C))
#define TMR32B1MR2     (*((volatile uint32_t *)0x40018020))
#define TMR32B1MR3     (*((volatile uint32_t *)0x40018024))
#define TMR32B1PWMC    (*((volatile uint32_t *)0x40018074))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))
```

## Clock Enable Bits (SYSAHBCLKCTRL)

```c
#define CT16B0_CLK     (1 << 7)
#define CT16B1_CLK     (1 << 8)
#define CT32B0_CLK     (1 << 9)
#define CT32B1_CLK     (1 << 10)
#define GPIO_CLK       (1 << 6)
```

## NVIC IRQ Numbers

```c
#define CT16B0_IRQn    16
#define CT16B1_IRQn    17
#define CT32B0_IRQn    18
#define CT32B1_IRQn    19
```

## Project Naming Convention

- Timer-Delay: `PROJECT = lpc1343_timer_delay`
- LED-Dimmer: `PROJECT = lpc1343_led_dimmer`
- Breathing-LED: `PROJECT = lpc1343_breathing_led`
- Servo-Control: `PROJECT = lpc1343_servo_control`
- Tone-Generator: `PROJECT = lpc1343_tone_generator`

---

## Example 1: Timer-Delay

**Status: CREATED**

**Concepts:** Timer interrupt, system tick, blocking/non-blocking delay

**Behavior:**
- Initialize CT32B0 for 1ms tick interrupt
- Provide `delay_ms()` function
- Blink LED using timer-based delay (not software loop)

**Key code:**
```c
volatile uint32_t ms_ticks = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & 1) {
        TMR32B0IR = 1;  // Clear flag
        ms_ticks++;
    }
}

void timer_init(void) {
    SYSAHBCLKCTRL |= (1 << 9);   // Enable CT32B0 clock
    TMR32B0TCR = 0x02;           // Reset
    TMR32B0TCR = 0x00;
    TMR32B0PR = 71;              // 72MHz/72 = 1MHz
    TMR32B0MR0 = 999;            // 1000 ticks = 1ms
    TMR32B0MCR = 0x03;           // Interrupt + Reset on MR0
    TMR32B0IR = 0x1F;            // Clear all flags
    NVIC_ISER = (1 << 18);       // Enable CT32B0 IRQ
    TMR32B0TCR = 0x01;           // Start
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}
```

---

## Example 2: LED-Dimmer

**Status: CREATED**

**Concepts:** PWM output, duty cycle control, button input

**Behavior:**
- PWM on P1.6 (CT32B0_MAT0) at 1kHz
- Button press cycles brightness: 0%, 25%, 50%, 75%, 100%
- Connect external LED to P1.6 or use oscilloscope

**Key code:**
```c
void pwm_init(void) {
    SYSAHBCLKCTRL |= (1 << 9);
    IOCON_PIO1_6 = 0x02;         // CT32B0_MAT0 function
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;
    TMR32B0PR = 0;
    TMR32B0MR3 = 71999;          // Period: 72MHz/72000 = 1kHz
    TMR32B0MR0 = 36000;          // 50% duty
    TMR32B0MCR = (1 << 10);      // Reset on MR3
    TMR32B0PWMC = (1 << 0);      // Enable PWM ch0
    TMR32B0TCR = 0x01;
}

void pwm_set_duty(uint8_t percent) {
    TMR32B0MR0 = (72000UL * percent) / 100;
}
```

---

## Example 3: Breathing-LED

**Status: CREATED**

**Concepts:** PWM, smooth transitions, gamma correction

**Behavior:**
- LED smoothly fades in and out (breathing effect)
- Uses PWM on P1.6
- Optional gamma correction for perceived linear brightness

**Key code:**
```c
void breathing_effect(void) {
    while (1) {
        // Fade in
        for (int duty = 0; duty <= 100; duty++) {
            pwm_set_duty(duty);
            delay_ms(15);
        }
        // Fade out
        for (int duty = 100; duty >= 0; duty--) {
            pwm_set_duty(duty);
            delay_ms(15);
        }
        delay_ms(200);
    }
}
```

---

## Example 4: Servo-Control

**Status: CREATED**

**Concepts:** 50Hz PWM, pulse width 1-2ms, angle mapping

**Behavior:**
- PWM at 50Hz (20ms period) on P1.6
- Sweep servo from 0° to 180° and back
- 1ms pulse = 0°, 1.5ms = 90°, 2ms = 180°

**Key code:**
```c
void servo_init(void) {
    SYSAHBCLKCTRL |= (1 << 9);
    IOCON_PIO1_6 = 0x02;
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;
    TMR32B0PR = 71;              // 1MHz timer clock (1µs/tick)
    TMR32B0MR3 = 19999;          // 20ms period (50Hz)
    TMR32B0MR0 = 1500;           // 1.5ms = center
    TMR32B0MCR = (1 << 10);
    TMR32B0PWMC = (1 << 0);
    TMR32B0TCR = 0x01;
}

void servo_set_angle(uint16_t angle) {
    if (angle > 180) angle = 180;
    // Map 0-180 to 1000-2000 µs
    uint32_t pulse = 1000 + (angle * 1000) / 180;
    TMR32B0MR0 = pulse;
}
```

---

## Example 5: Tone-Generator

**Status: CREATED**

**Concepts:** PWM frequency control, musical notes, melodies

**Behavior:**
- Generate tones at different frequencies on P1.6
- Play a simple melody (e.g., scale or "Mary Had a Little Lamb")
- 50% duty cycle for square wave

**Key code:**
```c
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_REST 0

void tone(uint16_t freq) {
    if (freq == 0) {
        TMR32B0PWMC = 0;  // Silence
        return;
    }
    uint32_t period = 72000000UL / freq;
    TMR32B0MR3 = period - 1;
    TMR32B0MR0 = period / 2;     // 50% duty
    TMR32B0PWMC = (1 << 0);
}

void play_note(uint16_t freq, uint16_t duration_ms) {
    tone(freq);
    delay_ms(duration_ms);
    tone(0);
    delay_ms(50);  // Gap between notes
}
```

---

## Makefile Template

Copy from previous examples, change:
- Comment line to match example name
- `PROJECT = lpc1343_<example_name>`

---

## README Template

```markdown
# <Example Name>

Chapter 4: Timers and PWM - <Example Name>

## What This Example Demonstrates

- Bullet points

## Hardware

- P1.6: PWM output (connect LED with resistor, or servo, or speaker)
- P3.0-P3.3: Status LEDs (optional)

## Building and Flashing

\`\`\`bash
make clean
make
make flash
\`\`\`

## Expected Behavior

Description...

## Code Highlights

Key code snippets...
```
