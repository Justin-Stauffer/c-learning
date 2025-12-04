# Chapter 4: Timers and PWM

A comprehensive beginner's guide to timer peripherals and Pulse Width Modulation on the LPC1343 microcontroller.

---

## Chapter Overview

| | |
|---|---|
| **Prerequisites** | Chapters 0-3 (especially GPIO and Bitwise Operations) |
| **Time to Complete** | 3-4 hours |
| **Hands-On Projects** | Precise delays, LED dimming, servo control, tone generation |
| **You Will Learn** | Hardware timing, PWM output, input capture |

---

## Quick Start: 1ms Timer Interrupt

Here's the minimal code to generate a 1ms periodic interrupt:

```c
#define SYSAHBCLKCTRL (*((volatile unsigned int *)0x40048080))
#define TMR32B0TCR    (*((volatile unsigned int *)0x40014004))
#define TMR32B0PR     (*((volatile unsigned int *)0x4001400C))
#define TMR32B0MR0    (*((volatile unsigned int *)0x40014018))
#define TMR32B0MCR    (*((volatile unsigned int *)0x40014014))
#define TMR32B0IR     (*((volatile unsigned int *)0x40014000))

volatile uint32_t ms_ticks = 0;

void CT32B0_IRQHandler(void) {
    TMR32B0IR = 1;       // Clear interrupt flag
    ms_ticks++;          // Increment counter
}

void timer_init_1ms(void) {
    SYSAHBCLKCTRL |= (1 << 9);   // Enable timer clock

    TMR32B0TCR = 0x02;           // Reset timer
    TMR32B0TCR = 0x00;

    TMR32B0PR = 71;              // Prescaler: 72MHz/72 = 1MHz
    TMR32B0MR0 = 999;            // Match at 1000 counts = 1ms
    TMR32B0MCR = 0x03;           // Interrupt + reset on match

    NVIC_EnableIRQ(18);          // Enable CT32B0 interrupt
    TMR32B0TCR = 0x01;           // Start timer
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}
```

**Quick PWM setup (50% duty cycle on P1.6):**

```c
IOCON_PIO1_6 = 0x02;             // Pin function = CT32B0_MAT0
TMR32B0MR3 = 71999;              // Period (1kHz at 72MHz)
TMR32B0MR0 = 35999;              // 50% duty cycle
TMR32B0MCR = (1 << 10);          // Reset on MR3
TMR32B0PWMC = (1 << 0);          // Enable PWM on channel 0
TMR32B0TCR = 0x01;               // Start
```

The rest of this chapter explains the theory, math, and advanced uses.

---

## Table of Contents

1. [What is a Timer?](#what-is-a-timer)
2. [Timer Fundamentals](#timer-fundamentals)
3. [LPC1343 Timer Architecture](#lpc1343-timer-architecture)
4. [Timer Registers](#timer-registers)
5. [Basic Timer Configuration](#basic-timer-configuration)
6. [Generating Delays](#generating-delays)
7. [Periodic Interrupts](#periodic-interrupts)
8. [Understanding PWM](#understanding-pwm)
9. [PWM Configuration](#pwm-configuration)
10. [Input Capture](#input-capture)
11. [Practical Examples](#practical-examples)
12. [Timer Math and Calculations](#timer-math-and-calculations)
13. [Common Patterns](#common-patterns)
14. [Troubleshooting](#troubleshooting)
15. [Quick Reference](#quick-reference)

---

## What is a Timer?

A **timer** is a hardware peripheral that counts clock pulses. Think of it as a very fast digital stopwatch built into the microcontroller.

### Why Use Timers?

```
WITHOUT TIMERS (Software Delay):

void delay(int count) {
    while(count--) {
        // CPU just waits, doing nothing useful
        // Timing varies with compiler optimization
        // Interrupts mess up timing
    }
}

Problem: CPU is 100% busy doing nothing!


WITH TIMERS (Hardware Counting):

┌─────────────────────────────────────────┐
│        Timer Hardware                    │
│  ┌───────────┐    ┌───────────┐         │
│  │  Counter  │───→│  Compare  │──→ IRQ  │
│  │  Register │    │  Register │         │
│  └─────┬─────┘    └───────────┘         │
│        │                                 │
│   Clock pulses                           │
│   counted here                           │
└─────────────────────────────────────────┘
         │
    CPU is FREE to do other work!
    Timer counts in background.
```

**Timer Benefits:**
- **Precise timing** - Hardware is more accurate than software loops
- **CPU freedom** - Timer counts while CPU does other tasks
- **Consistent** - Not affected by compiler optimization
- **Low power** - CPU can sleep while timer runs
- **Multiple uses** - Generate interrupts, PWM, measure time

---

## Timer Fundamentals

### How a Timer Counts

```
System Clock: 72 MHz (72,000,000 pulses/second)
                │
                ↓
┌─────────────────────────────────────────┐
│           Prescaler                      │
│   Divides clock by (PR + 1)             │
│   72 MHz ÷ 72 = 1 MHz                   │
└─────────────────┬───────────────────────┘
                  │
                  ↓ 1 MHz (1,000,000 pulses/second)
┌─────────────────────────────────────────┐
│        Timer Counter (TC)                │
│   Counts from 0 upward                   │
│   0 → 1 → 2 → 3 → ... → 999999 → ...    │
│                                          │
│   At 1 MHz, counts 1,000,000 per second │
│   Each count = 1 microsecond            │
└─────────────────┬───────────────────────┘
                  │
                  ↓ Compare with Match Register
┌─────────────────────────────────────────┐
│        Match Register (MR)               │
│   When TC == MR:                         │
│   - Generate interrupt                   │
│   - Reset counter                        │
│   - Toggle output pin                    │
│   - Stop timer                           │
└─────────────────────────────────────────┘
```

### Key Concepts

**Timer Counter (TC):**
- 16-bit or 32-bit counter
- Increments on each prescaled clock pulse
- Can be read/written by software

**Prescaler (PR):**
- Divides the system clock
- Timer clock = System clock ÷ (PR + 1)
- Allows slower counting for longer delays

**Match Register (MR):**
- Value to compare against TC
- When TC equals MR, actions can occur
- Each timer has multiple match registers (MR0, MR1, MR2, MR3)

**Match Actions:**
- Generate interrupt
- Reset TC to 0
- Stop the timer
- Toggle an output pin

### Timer Math

```
Basic formulas:

Timer Clock Frequency:
    F_timer = F_system / (PR + 1)

Time per Timer Tick:
    T_tick = 1 / F_timer = (PR + 1) / F_system

Time to Match:
    T_match = (MR + 1) × T_tick = (MR + 1) × (PR + 1) / F_system

Match Value for Desired Time:
    MR = (T_desired × F_system) / (PR + 1) - 1


Example: 1 second delay at 72 MHz system clock

Option 1: PR = 71, MR = 999999
    F_timer = 72 MHz / 72 = 1 MHz
    T_match = 1,000,000 × 1 µs = 1 second

Option 2: PR = 7199, MR = 9999
    F_timer = 72 MHz / 7200 = 10 kHz
    T_match = 10,000 × 0.1 ms = 1 second

Option 3: PR = 71999, MR = 999
    F_timer = 72 MHz / 72000 = 1 kHz
    T_match = 1,000 × 1 ms = 1 second
```

---

## LPC1343 Timer Architecture

### Available Timers

```
LPC1343 has 4 timers:
┌────────────────────────────────────────────────────────┐
│                                                         │
│  16-bit Timers:                                        │
│  ┌──────────────────┐  ┌──────────────────┐            │
│  │  CT16B0          │  │  CT16B1          │            │
│  │  Counter: 16-bit │  │  Counter: 16-bit │            │
│  │  Max: 65,535     │  │  Max: 65,535     │            │
│  │  Match: MR0-MR3  │  │  Match: MR0-MR3  │            │
│  └──────────────────┘  └──────────────────┘            │
│                                                         │
│  32-bit Timers:                                        │
│  ┌──────────────────┐  ┌──────────────────┐            │
│  │  CT32B0          │  │  CT32B1          │            │
│  │  Counter: 32-bit │  │  Counter: 32-bit │            │
│  │  Max: 4,294,967,295 │  Max: 4,294,967,295          │
│  │  Match: MR0-MR3  │  │  Match: MR0-MR3  │            │
│  └──────────────────┘  └──────────────────┘            │
│                                                         │
└────────────────────────────────────────────────────────┘

16-bit vs 32-bit:
- 16-bit: Counts 0 to 65,535 (good for short delays, PWM)
- 32-bit: Counts 0 to 4,294,967,295 (good for long delays)
```

### Timer Block Diagram

```
                    ┌──────────────────────────────────────┐
                    │          Timer Block (CT32B0)        │
                    │                                       │
PCLK (system) ─────→│  ┌──────────┐      ┌──────────────┐  │
                    │  │ Prescaler│─────→│Timer Counter │  │
                    │  │   (PR)   │      │    (TC)      │  │
                    │  └──────────┘      └──────┬───────┘  │
                    │                           │          │
                    │         ┌─────────────────┼──────────┼──┐
                    │         │                 ↓          │  │
                    │  MR0 ───┤ Compare     ┌───────┐      │  │
                    │  MR1 ───┤   Logic     │Actions│──────┼──┼→ IRQ
                    │  MR2 ───┤             │ Reset │      │  │
                    │  MR3 ───┤             │ Stop  │      │  │
                    │         │             └───┬───┘      │  │
                    │         └─────────────────┼──────────┼──┘
                    │                           │          │
                    │  ┌────────────────────────┼──────────┤
                    │  │  PWM Output Logic      │          │
                    │  │  (for MAT0-MAT3)       ↓          │
                    │  └────────────────────────┼──────────┤
                    │                           │          │
                    └───────────────────────────┼──────────┘
                                                │
                                                ↓
                              External Pins (MAT0, MAT1, MAT2, MAT3)
                              or Capture inputs (CAP0)
```

### Timer Pins

```
Timer    Function      GPIO Pin    IOCON Register
─────    ────────      ────────    ──────────────
CT16B0   MAT0          P0.8        IOCON_PIO0_8
CT16B0   MAT1          P0.9        IOCON_PIO0_9
CT16B0   MAT2          P0.10       IOCON_PIO0_10
CT16B0   CAP0          P0.2        IOCON_PIO0_2

CT16B1   MAT0          P1.9        IOCON_PIO1_9
CT16B1   MAT1          P1.10       IOCON_PIO1_10
CT16B1   CAP0          P1.8        IOCON_PIO1_8

CT32B0   MAT0          P1.6        IOCON_PIO1_6
CT32B0   MAT1          P1.7        IOCON_PIO1_7
CT32B0   MAT2          P0.1        IOCON_PIO0_1
CT32B0   MAT3          P0.11       IOCON_PIO0_11
CT32B0   CAP0          P1.5        IOCON_PIO1_5

CT32B1   MAT0          P1.1        IOCON_PIO1_1
CT32B1   MAT1          P1.2        IOCON_PIO1_2
CT32B1   MAT2          P1.3        IOCON_PIO1_3
CT32B1   MAT3          P1.4        IOCON_PIO1_4
CT32B1   CAP0          P1.0        IOCON_PIO1_0
```

---

## Timer Registers

### Register Overview (CT32B0 example)

```
Register    Offset    Description
────────    ──────    ────────────────────────────────────────
TMR32B0IR   0x000     Interrupt Register (flags)
TMR32B0TCR  0x004     Timer Control Register (enable/reset)
TMR32B0TC   0x008     Timer Counter (current count)
TMR32B0PR   0x00C     Prescale Register (clock divider)
TMR32B0PC   0x010     Prescale Counter (current prescale count)
TMR32B0MCR  0x014     Match Control Register (match actions)
TMR32B0MR0  0x018     Match Register 0
TMR32B0MR1  0x01C     Match Register 1
TMR32B0MR2  0x020     Match Register 2
TMR32B0MR3  0x024     Match Register 3
TMR32B0CCR  0x028     Capture Control Register
TMR32B0CR0  0x02C     Capture Register 0
TMR32B0EMR  0x03C     External Match Register (pin control)
TMR32B0CTCR 0x070     Count Control Register (timer/counter mode)
TMR32B0PWMC 0x074     PWM Control Register
```

### TMR32B0TCR - Timer Control Register

```
Bit   Name    Description
───   ────    ─────────────────────────────────────
0     CEN     Counter Enable (1 = counting, 0 = stopped)
1     CRST    Counter Reset (1 = reset TC and PC to 0)
2-31  -       Reserved

Usage:
TMR32B0TCR = 0x01;  // Enable timer
TMR32B0TCR = 0x02;  // Reset timer (pulse this)
TMR32B0TCR = 0x00;  // Disable timer
```

### TMR32B0MCR - Match Control Register

Controls what happens when TC matches MRx:

```
Bit    Name     Description
───    ────     ──────────────────────────────
0      MR0I     Interrupt on MR0 match
1      MR0R     Reset TC on MR0 match
2      MR0S     Stop timer on MR0 match
3      MR1I     Interrupt on MR1 match
4      MR1R     Reset TC on MR1 match
5      MR1S     Stop timer on MR1 match
6      MR2I     Interrupt on MR2 match
7      MR2R     Reset TC on MR2 match
8      MR2S     Stop timer on MR2 match
9      MR3I     Interrupt on MR3 match
10     MR3R     Reset TC on MR3 match
11     MR3S     Stop timer on MR3 match
12-31  -        Reserved

Common configurations:
┌──────────────────────────────────────────────────────┐
│ Periodic interrupt (MR0):                            │
│   MCR = (1 << 0) | (1 << 1)  // Interrupt + Reset   │
│                                                      │
│ One-shot interrupt (MR0):                           │
│   MCR = (1 << 0) | (1 << 2)  // Interrupt + Stop    │
│                                                      │
│ Continuous counting (no actions):                   │
│   MCR = 0x000                                        │
└──────────────────────────────────────────────────────┘
```

### TMR32B0IR - Interrupt Register

```
Bit   Name    Description
───   ────    ─────────────────────────────────────
0     MR0     Match Register 0 interrupt flag
1     MR1     Match Register 1 interrupt flag
2     MR2     Match Register 2 interrupt flag
3     MR3     Match Register 3 interrupt flag
4     CR0     Capture Register 0 interrupt flag
5-31  -       Reserved

Write 1 to clear the flag:
TMR32B0IR = (1 << 0);  // Clear MR0 interrupt flag

Read to check flag:
if (TMR32B0IR & (1 << 0)) {
    // MR0 matched
}
```

### TMR32B0EMR - External Match Register

Controls the MAT output pins:

```
Bit     Name    Description
─────   ────    ─────────────────────────────────────
0       EM0     External Match 0 state
1       EM1     External Match 1 state
2       EM2     External Match 2 state
3       EM3     External Match 3 state
5:4     EMC0    External Match Control 0
7:6     EMC1    External Match Control 1
9:8     EMC2    External Match Control 2
11:10   EMC3    External Match Control 3

EMCn values:
00 = Do nothing
01 = Clear (set to 0) on match
10 = Set (set to 1) on match
11 = Toggle on match
```

### TMR32B0PWMC - PWM Control Register

```
Bit   Name     Description
───   ────     ─────────────────────────────────────
0     PWMEN0   Enable PWM on match channel 0
1     PWMEN1   Enable PWM on match channel 1
2     PWMEN2   Enable PWM on match channel 2
3     PWMEN3   Enable PWM on match channel 3
4-31  -        Reserved

Note: For PWM, one match register sets the period (MR3 typically)
and others set the duty cycle (MR0, MR1, MR2).
```

---

## Basic Timer Configuration

### Step-by-Step Timer Setup

```c
// ============================================
// Basic Timer Initialization (CT32B0)
// ============================================

void timer_init(void) {
    // Step 1: Enable timer clock
    // Each timer has a clock enable bit in SYSAHBCLKCTRL
    SYSAHBCLKCTRL |= (1 << 9);  // Enable CT32B0 clock

    // Step 2: Reset the timer
    TMR32B0TCR = 0x02;  // Assert reset
    TMR32B0TCR = 0x00;  // Release reset

    // Step 3: Set prescaler
    // Timer clock = 72 MHz / (PR + 1)
    TMR32B0PR = 71;  // Divide by 72 → 1 MHz timer clock

    // Step 4: Set match value
    // At 1 MHz, count of 1000 = 1 ms
    TMR32B0MR0 = 999;  // Match at 1000 counts (0-999)

    // Step 5: Configure match actions
    // Interrupt on match, reset counter on match
    TMR32B0MCR = (1 << 0) | (1 << 1);  // MR0I + MR0R

    // Step 6: Clear any pending interrupts
    TMR32B0IR = 0x1F;  // Clear all flags

    // Step 7: Enable timer interrupt in NVIC
    NVIC_EnableIRQ(CT32B0_IRQn);

    // Step 8: Start the timer
    TMR32B0TCR = 0x01;  // Enable counting
}
```

### Clock Enable Bits

```c
// Timer clock enable bits in SYSAHBCLKCTRL:
// Bit 7:  CT16B0
// Bit 8:  CT16B1
// Bit 9:  CT32B0
// Bit 10: CT32B1

// Enable all timers:
SYSAHBCLKCTRL |= (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10);
```

---

## Generating Delays

### Blocking Delay with Timer

```c
// ============================================
// Millisecond Delay using CT32B0
// ============================================

// System clock assumed to be 72 MHz
#define SYSTEM_CLOCK 72000000UL

volatile uint32_t delay_counter = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {  // MR0 interrupt
        TMR32B0IR = (1 << 0);    // Clear flag
        if (delay_counter > 0) {
            delay_counter--;
        }
    }
}

void delay_init(void) {
    // Enable timer clock
    SYSAHBCLKCTRL |= (1 << 9);

    // Reset timer
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    // Prescaler for 1 MHz timer clock (1 µs per tick)
    TMR32B0PR = (SYSTEM_CLOCK / 1000000) - 1;  // = 71

    // Match value for 1 ms
    TMR32B0MR0 = 999;  // 1000 ticks = 1 ms

    // Interrupt and reset on MR0
    TMR32B0MCR = (1 << 0) | (1 << 1);

    // Clear pending and enable interrupt
    TMR32B0IR = 0x1F;
    NVIC_EnableIRQ(CT32B0_IRQn);

    // Start timer
    TMR32B0TCR = 0x01;
}

void delay_ms(uint32_t ms) {
    delay_counter = ms;
    while (delay_counter > 0) {
        // Wait - could use __WFI() to save power
    }
}

// Usage:
void main(void) {
    delay_init();
    __enable_irq();

    while (1) {
        LED0_TOGGLE();
        delay_ms(500);  // 500 ms delay
    }
}
```

### Non-Blocking Delay (Timeout)

```c
// ============================================
// Non-blocking delay / timeout
// ============================================

volatile uint32_t system_ticks = 0;  // Increments every 1 ms

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {
        TMR32B0IR = (1 << 0);
        system_ticks++;
    }
}

uint32_t get_ticks(void) {
    return system_ticks;
}

// Check if timeout has elapsed
uint8_t timeout_elapsed(uint32_t start_time, uint32_t timeout_ms) {
    return (system_ticks - start_time) >= timeout_ms;
}

// Usage example: Non-blocking LED blink
void main(void) {
    uint32_t last_toggle = 0;

    delay_init();
    __enable_irq();

    while (1) {
        // Toggle LED every 500 ms without blocking
        if (timeout_elapsed(last_toggle, 500)) {
            LED0_TOGGLE();
            last_toggle = get_ticks();
        }

        // Can do other work here!
        process_serial_data();
        check_buttons();
    }
}
```

### Microsecond Delay

```c
// ============================================
// Microsecond Delay (uses polling, not interrupt)
// ============================================

void delay_us_init(void) {
    // Use CT16B0 for microsecond delays
    SYSAHBCLKCTRL |= (1 << 7);

    TMR16B0TCR = 0x02;  // Reset
    TMR16B0TCR = 0x00;

    // No prescaler - timer runs at system clock
    TMR16B0PR = 0;

    // Configure for one-shot (stop on match)
    TMR16B0MCR = (1 << 2);  // Stop on MR0
}

void delay_us(uint16_t us) {
    // At 72 MHz, each tick is ~13.9 ns
    // For 1 µs, need 72 ticks
    uint32_t ticks = (uint32_t)us * (SYSTEM_CLOCK / 1000000);

    // 16-bit timer max is 65535, limit to ~910 µs at 72 MHz
    if (ticks > 65535) ticks = 65535;

    TMR16B0MR0 = ticks;
    TMR16B0TCR = 0x02;  // Reset counter
    TMR16B0TCR = 0x01;  // Start

    // Wait for timer to stop
    while (TMR16B0TCR & 0x01) {
        // Polling - blocks CPU
    }
}
```

---

## Periodic Interrupts

### Fixed-Rate Interrupt

```c
// ============================================
// 1 kHz periodic interrupt (every 1 ms)
// ============================================

volatile uint32_t tick_count = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {
        TMR32B0IR = (1 << 0);  // Clear interrupt flag

        tick_count++;

        // Can do periodic tasks here
        // Keep ISR short!
    }
}

void periodic_timer_init(uint32_t frequency_hz) {
    SYSAHBCLKCTRL |= (1 << 9);

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    // Calculate prescaler and match for desired frequency
    // For simplicity, use prescaler = 0 (no division)
    TMR32B0PR = 0;

    // Match value = system_clock / desired_frequency - 1
    TMR32B0MR0 = (SYSTEM_CLOCK / frequency_hz) - 1;

    TMR32B0MCR = (1 << 0) | (1 << 1);  // Interrupt + Reset
    TMR32B0IR = 0x1F;

    NVIC_EnableIRQ(CT32B0_IRQn);
    TMR32B0TCR = 0x01;
}

void main(void) {
    periodic_timer_init(1000);  // 1 kHz = 1 ms period
    __enable_irq();

    while (1) {
        __WFI();  // Sleep until interrupt
    }
}
```

### Multiple Periodic Tasks

```c
// ============================================
// Software timers based on hardware tick
// ============================================

#define MAX_SW_TIMERS 4

typedef struct {
    uint32_t period;      // Period in ticks
    uint32_t counter;     // Current count
    void (*callback)(void);  // Function to call
    uint8_t active;
} SoftwareTimer;

SoftwareTimer sw_timers[MAX_SW_TIMERS];

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {
        TMR32B0IR = (1 << 0);

        // Update all software timers
        for (int i = 0; i < MAX_SW_TIMERS; i++) {
            if (sw_timers[i].active) {
                sw_timers[i].counter++;
                if (sw_timers[i].counter >= sw_timers[i].period) {
                    sw_timers[i].counter = 0;
                    if (sw_timers[i].callback) {
                        sw_timers[i].callback();
                    }
                }
            }
        }
    }
}

void sw_timer_start(uint8_t id, uint32_t period_ms, void (*callback)(void)) {
    if (id < MAX_SW_TIMERS) {
        sw_timers[id].period = period_ms;
        sw_timers[id].counter = 0;
        sw_timers[id].callback = callback;
        sw_timers[id].active = 1;
    }
}

void sw_timer_stop(uint8_t id) {
    if (id < MAX_SW_TIMERS) {
        sw_timers[id].active = 0;
    }
}

// Usage:
void led0_toggle(void) { LED0_TOGGLE(); }
void led1_toggle(void) { LED1_TOGGLE(); }

void main(void) {
    periodic_timer_init(1000);  // 1 ms tick

    sw_timer_start(0, 500, led0_toggle);   // LED0 every 500 ms
    sw_timer_start(1, 1000, led1_toggle);  // LED1 every 1000 ms

    __enable_irq();

    while (1) {
        __WFI();
    }
}
```

---

## Understanding PWM

### What is PWM?

**PWM** (Pulse Width Modulation) is a technique to create a variable "average" voltage by rapidly switching between ON and OFF.

```
PWM Signal:
                 Period (T)
            |←────────────────→|

100% Duty Cycle (Always ON):
─────────────────────────────────────────

75% Duty Cycle:
████████████████────████████████████────
|← ON (75%) →|      |← ON (75%) →|
              |OFF|              |OFF|

50% Duty Cycle:
████████────────████████────────████████
|← ON →| OFF   |← ON →| OFF   |← ON →|

25% Duty Cycle:
████────────────████────────────████────

0% Duty Cycle (Always OFF):
─────────────────────────────────────────
```

### PWM Applications

```
LED Brightness Control:
┌─────────────────────────────────────────────────┐
│  PWM Duty Cycle    →    Perceived Brightness   │
│       0%                    OFF                 │
│      25%                    Dim                 │
│      50%                    Medium              │
│      75%                    Bright              │
│     100%                    Full ON             │
└─────────────────────────────────────────────────┘

Motor Speed Control:
┌─────────────────────────────────────────────────┐
│  PWM Duty Cycle    →    Motor Speed            │
│       0%                    Stopped             │
│      50%                    Half Speed          │
│     100%                    Full Speed          │
└─────────────────────────────────────────────────┘

Servo Position Control:
┌─────────────────────────────────────────────────┐
│  Pulse Width       →    Servo Angle            │
│     1.0 ms                  0°                  │
│     1.5 ms                  90°                 │
│     2.0 ms                  180°                │
│  (at 50 Hz / 20 ms period)                     │
└─────────────────────────────────────────────────┘
```

### PWM Terminology

```
        ┌────────┐            ┌────────┐
        │        │            │        │
────────┘        └────────────┘        └──────

        |←──────→|
        Pulse Width (On Time)

|←─────────────────────────→|
            Period (T)

Frequency = 1 / Period

Duty Cycle = Pulse Width / Period × 100%

Example:
- Period = 1 ms (1000 µs)
- Frequency = 1 kHz
- Pulse Width = 250 µs
- Duty Cycle = 250/1000 = 25%
```

---

## PWM Configuration

### LPC1343 PWM Overview

The LPC1343 generates PWM using match registers:

```
PWM Generation:
                              Timer
                               TC
                                │
         Period ───────────── MR3 ────→ Reset TC when TC=MR3
                                │
         PWM Channel 0 ─────── MR0 ────→ Toggle/Set MAT0
         PWM Channel 1 ─────── MR1 ────→ Toggle/Set MAT1
         PWM Channel 2 ─────── MR2 ────→ Toggle/Set MAT2

How it works:
1. TC counts from 0 up
2. When TC = MRx (x=0,1,2), output goes LOW
3. When TC = MR3, output goes HIGH, TC resets
4. Repeat

    PWM Period (set by MR3)
    |←─────────────────→|
    ┌────────┐          ┌────────┐
    │        │          │        │
────┘        └──────────┘        └──────
    ↑        ↑          ↑
    TC=0     TC=MR0     TC=MR3
    (HIGH)   (LOW)      (Reset, HIGH)
```

### Basic PWM Setup

```c
// ============================================
// PWM Configuration on CT32B0 MAT0 (P1.6)
// ============================================

#define PWM_FREQUENCY 1000  // 1 kHz PWM
#define SYSTEM_CLOCK 72000000UL

void pwm_init(uint32_t frequency, uint8_t duty_cycle) {
    // Enable timer clock
    SYSAHBCLKCTRL |= (1 << 9);

    // Configure P1.6 as CT32B0_MAT0
    IOCON_PIO1_6 = (0x02 << 0);  // FUNC = CT32B0_MAT0

    // Reset timer
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    // No prescaler
    TMR32B0PR = 0;

    // Calculate period (number of ticks for one PWM cycle)
    uint32_t period = SYSTEM_CLOCK / frequency;

    // MR3 sets the period (PWM resets here)
    TMR32B0MR3 = period - 1;

    // MR0 sets duty cycle
    // duty_cycle is 0-100
    TMR32B0MR0 = (period * duty_cycle) / 100;

    // Reset on MR3 match
    TMR32B0MCR = (1 << 10);  // MR3R - Reset on MR3

    // Enable PWM mode on channel 0
    TMR32B0PWMC = (1 << 0);  // PWMEN0

    // Start timer
    TMR32B0TCR = 0x01;
}

void pwm_set_duty(uint8_t duty_cycle) {
    uint32_t period = TMR32B0MR3 + 1;
    TMR32B0MR0 = (period * duty_cycle) / 100;
}

// Usage:
void main(void) {
    pwm_init(1000, 50);  // 1 kHz, 50% duty cycle

    while (1) {
        // Vary brightness
        for (int duty = 0; duty <= 100; duty += 5) {
            pwm_set_duty(duty);
            delay_ms(50);
        }
        for (int duty = 100; duty >= 0; duty -= 5) {
            pwm_set_duty(duty);
            delay_ms(50);
        }
    }
}
```

### Multiple PWM Channels

```c
// ============================================
// 3-channel PWM (e.g., RGB LED control)
// ============================================

void pwm_rgb_init(uint32_t frequency) {
    SYSAHBCLKCTRL |= (1 << 9);

    // Configure pins for PWM output
    IOCON_PIO1_6 = 0x02;  // MAT0 - Red
    IOCON_PIO1_7 = 0x02;  // MAT1 - Green
    IOCON_PIO0_1 = 0x02;  // MAT2 - Blue

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;
    TMR32B0PR = 0;

    uint32_t period = SYSTEM_CLOCK / frequency;
    TMR32B0MR3 = period - 1;  // Period

    // Initial duty cycles (off)
    TMR32B0MR0 = 0;  // Red
    TMR32B0MR1 = 0;  // Green
    TMR32B0MR2 = 0;  // Blue

    // Reset on MR3
    TMR32B0MCR = (1 << 10);

    // Enable PWM on channels 0, 1, 2
    TMR32B0PWMC = (1 << 0) | (1 << 1) | (1 << 2);

    TMR32B0TCR = 0x01;
}

void pwm_set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t period = TMR32B0MR3 + 1;

    // Assuming 0-255 input (8-bit color)
    TMR32B0MR0 = (period * red) / 255;
    TMR32B0MR1 = (period * green) / 255;
    TMR32B0MR2 = (period * blue) / 255;
}

// Usage:
void main(void) {
    pwm_rgb_init(1000);

    while (1) {
        pwm_set_rgb(255, 0, 0);    // Red
        delay_ms(1000);
        pwm_set_rgb(0, 255, 0);    // Green
        delay_ms(1000);
        pwm_set_rgb(0, 0, 255);    // Blue
        delay_ms(1000);
        pwm_set_rgb(255, 255, 0);  // Yellow
        delay_ms(1000);
        pwm_set_rgb(0, 255, 255);  // Cyan
        delay_ms(1000);
        pwm_set_rgb(255, 0, 255);  // Magenta
        delay_ms(1000);
        pwm_set_rgb(255, 255, 255);// White
        delay_ms(1000);
    }
}
```

### Servo Control with PWM

```c
// ============================================
// Servo Motor Control
// Standard servo: 50 Hz, 1-2 ms pulse
// ============================================

#define SERVO_FREQUENCY 50  // 50 Hz = 20 ms period
#define SERVO_MIN_PULSE_US 1000   // 1 ms = 0 degrees
#define SERVO_MAX_PULSE_US 2000   // 2 ms = 180 degrees

void servo_init(void) {
    SYSAHBCLKCTRL |= (1 << 9);
    IOCON_PIO1_6 = 0x02;  // CT32B0_MAT0

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    // Prescaler for microsecond resolution
    TMR32B0PR = (SYSTEM_CLOCK / 1000000) - 1;  // 1 µs per tick

    // Period for 50 Hz (20,000 µs)
    TMR32B0MR3 = 20000 - 1;

    // Initial position (center = 1500 µs)
    TMR32B0MR0 = 1500;

    TMR32B0MCR = (1 << 10);  // Reset on MR3
    TMR32B0PWMC = (1 << 0);   // PWM on channel 0

    TMR32B0TCR = 0x01;
}

void servo_set_angle(uint16_t angle) {
    // Map 0-180 degrees to 1000-2000 µs
    if (angle > 180) angle = 180;

    uint32_t pulse_us = SERVO_MIN_PULSE_US +
        ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;

    TMR32B0MR0 = pulse_us;
}

void servo_set_pulse_us(uint16_t pulse_us) {
    if (pulse_us < SERVO_MIN_PULSE_US) pulse_us = SERVO_MIN_PULSE_US;
    if (pulse_us > SERVO_MAX_PULSE_US) pulse_us = SERVO_MAX_PULSE_US;

    TMR32B0MR0 = pulse_us;
}

// Usage:
void main(void) {
    servo_init();

    while (1) {
        // Sweep servo back and forth
        for (int angle = 0; angle <= 180; angle += 5) {
            servo_set_angle(angle);
            delay_ms(50);
        }
        for (int angle = 180; angle >= 0; angle -= 5) {
            servo_set_angle(angle);
            delay_ms(50);
        }
    }
}
```

---

## Input Capture

### What is Input Capture?

Input capture records the timer value when an external event occurs. Useful for measuring:
- Pulse width
- Frequency
- Period
- Time between events

```
Input Signal:
    ┌─────┐         ┌─────┐         ┌─────┐
────┘     └─────────┘     └─────────┘     └────

Timer:
    ↑           ↑   ↑           ↑   ↑
    │           │   │           │   │
Capture 1   Cap 2   Capture 3   Cap 4   ...
(TC=1234)   (5678)  (TC=9012)   ...

Pulse Width = Capture 2 - Capture 1
Period = Capture 3 - Capture 1
Frequency = Timer_Clock / Period
```

### Basic Input Capture

```c
// ============================================
// Input Capture on CT32B0_CAP0 (P1.5)
// Measure pulse width
// ============================================

volatile uint32_t capture_rising = 0;
volatile uint32_t capture_falling = 0;
volatile uint32_t pulse_width = 0;
volatile uint8_t capture_complete = 0;

void CT32B0_IRQHandler(void) {
    static uint8_t capture_state = 0;

    if (TMR32B0IR & (1 << 4)) {  // CR0 interrupt (capture)
        TMR32B0IR = (1 << 4);     // Clear flag

        if (capture_state == 0) {
            // Rising edge captured
            capture_rising = TMR32B0CR0;
            capture_state = 1;

            // Switch to falling edge
            TMR32B0CCR = (1 << 1) | (1 << 2);  // Falling + interrupt
        } else {
            // Falling edge captured
            capture_falling = TMR32B0CR0;
            capture_state = 0;

            // Calculate pulse width
            if (capture_falling > capture_rising) {
                pulse_width = capture_falling - capture_rising;
            } else {
                // Timer wrapped around
                pulse_width = (0xFFFFFFFF - capture_rising) + capture_falling + 1;
            }
            capture_complete = 1;

            // Switch back to rising edge
            TMR32B0CCR = (1 << 0) | (1 << 2);  // Rising + interrupt
        }
    }
}

void capture_init(void) {
    SYSAHBCLKCTRL |= (1 << 9);

    // Configure P1.5 as CT32B0_CAP0
    IOCON_PIO1_5 = 0x02;

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    // Prescaler for 1 µs resolution
    TMR32B0PR = (SYSTEM_CLOCK / 1000000) - 1;

    // Free-running timer (no match actions)
    TMR32B0MCR = 0;

    // Capture on rising edge, interrupt on capture
    TMR32B0CCR = (1 << 0) | (1 << 2);  // CAP0RE + CAP0I

    TMR32B0IR = 0x1F;
    NVIC_EnableIRQ(CT32B0_IRQn);

    TMR32B0TCR = 0x01;
}

uint32_t get_pulse_width_us(void) {
    if (capture_complete) {
        capture_complete = 0;
        return pulse_width;  // Already in microseconds
    }
    return 0;
}
```

### Frequency Measurement

```c
// ============================================
// Measure frequency of input signal
// ============================================

volatile uint32_t last_capture = 0;
volatile uint32_t period_ticks = 0;
volatile uint8_t new_measurement = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 4)) {
        TMR32B0IR = (1 << 4);

        uint32_t current = TMR32B0CR0;

        if (last_capture != 0) {
            if (current > last_capture) {
                period_ticks = current - last_capture;
            } else {
                period_ticks = (0xFFFFFFFF - last_capture) + current + 1;
            }
            new_measurement = 1;
        }
        last_capture = current;
    }
}

uint32_t get_frequency_hz(void) {
    if (new_measurement && period_ticks > 0) {
        new_measurement = 0;
        // Timer runs at 1 MHz (1 µs per tick)
        // Frequency = 1,000,000 / period_ticks
        return 1000000UL / period_ticks;
    }
    return 0;
}
```

---

## Practical Examples

### Example 1: LED Brightness Control with Button

```c
// ============================================
// Button adjusts LED brightness (PWM)
// ============================================

volatile uint8_t brightness = 50;  // 0-100%

void PIOINT0_IRQHandler(void) {
    if (GPIO0MIS & (1 << 1)) {  // Button on P0.1
        GPIO0IC = (1 << 1);

        brightness += 20;
        if (brightness > 100) brightness = 0;

        pwm_set_duty(brightness);
    }
}

void main(void) {
    init_button_interrupt();  // From GPIO guide
    pwm_init(1000, brightness);

    __enable_irq();

    while (1) {
        __WFI();
    }
}
```

### Example 2: Breathing LED Effect

```c
// ============================================
// LED "breathing" effect using PWM
// ============================================

void breathing_led(void) {
    pwm_init(1000, 0);

    // Use sine-like curve for smooth breathing
    // Approximation: duty = 50 + 50*sin(t)
    // Simplified: ramp up, ramp down

    while (1) {
        // Breathe in (brightness up)
        for (int duty = 0; duty <= 100; duty++) {
            pwm_set_duty(duty);
            delay_ms(10);
        }

        // Breathe out (brightness down)
        for (int duty = 100; duty >= 0; duty--) {
            pwm_set_duty(duty);
            delay_ms(10);
        }

        delay_ms(200);  // Pause at bottom
    }
}

// Smoother version with gamma correction
const uint8_t gamma_table[101] = {
    0, 0, 0, 0, 0, 1, 1, 1, 1, 2,
    2, 2, 3, 3, 4, 4, 5, 5, 6, 7,
    7, 8, 9, 10, 11, 12, 13, 14, 15, 17,
    18, 19, 21, 22, 24, 26, 27, 29, 31, 33,
    35, 37, 39, 42, 44, 47, 49, 52, 55, 58,
    61, 64, 67, 70, 74, 77, 81, 85, 89, 93,
    97, 101, 106, 110, 115, 120, 125, 130, 135, 141,
    146, 152, 158, 164, 170, 177, 183, 190, 197, 204,
    211, 218, 226, 234, 241, 250, 258, 266, 275, 284,
    293, 302, 312, 322, 332, 342, 352, 363, 374, 385,
    396
};

void breathing_led_smooth(void) {
    pwm_init(1000, 0);

    while (1) {
        for (int i = 0; i <= 100; i++) {
            pwm_set_duty(gamma_table[i] * 100 / 396);
            delay_ms(15);
        }
        for (int i = 100; i >= 0; i--) {
            pwm_set_duty(gamma_table[i] * 100 / 396);
            delay_ms(15);
        }
    }
}
```

### Example 3: Software PWM (Multiple LEDs)

```c
// ============================================
// Software PWM for GPIO pins without hardware PWM
// Uses timer interrupt
// ============================================

#define SW_PWM_RESOLUTION 100  // 0-100 duty cycle
#define NUM_SW_PWM_CHANNELS 4

typedef struct {
    volatile uint32_t *port;
    uint32_t pin;
    uint8_t duty;
} SwPwmChannel;

SwPwmChannel sw_pwm[NUM_SW_PWM_CHANNELS] = {
    { &GPIO3DATA, (1 << 0), 25 },   // LED0, 25%
    { &GPIO3DATA, (1 << 1), 50 },   // LED1, 50%
    { &GPIO3DATA, (1 << 2), 75 },   // LED2, 75%
    { &GPIO3DATA, (1 << 3), 100 },  // LED3, 100%
};

volatile uint8_t pwm_counter = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {
        TMR32B0IR = (1 << 0);

        pwm_counter++;
        if (pwm_counter >= SW_PWM_RESOLUTION) {
            pwm_counter = 0;
        }

        // Update each channel
        for (int i = 0; i < NUM_SW_PWM_CHANNELS; i++) {
            if (pwm_counter < sw_pwm[i].duty) {
                // ON (active-low: clear bit)
                *sw_pwm[i].port &= ~sw_pwm[i].pin;
            } else {
                // OFF (active-low: set bit)
                *sw_pwm[i].port |= sw_pwm[i].pin;
            }
        }
    }
}

void sw_pwm_init(void) {
    // Configure GPIO as outputs
    GPIO3DIR |= 0x0F;
    GPIO3DATA |= 0x0F;  // Start all off

    // Timer interrupt at PWM_FREQUENCY * RESOLUTION
    // For 100 Hz PWM with 100 steps: 10 kHz interrupt
    periodic_timer_init(10000);
}

void sw_pwm_set_duty(uint8_t channel, uint8_t duty) {
    if (channel < NUM_SW_PWM_CHANNELS) {
        sw_pwm[channel].duty = duty;
    }
}
```

### Example 4: Ultrasonic Distance Sensor

```c
// ============================================
// HC-SR04 Ultrasonic Sensor
// Trigger: Send 10µs pulse
// Echo: Measure pulse width
// Distance = pulse_width_us / 58 (in cm)
// ============================================

#define TRIGGER_PIN (1 << 0)  // P2.0
#define ECHO_PIN (1 << 1)     // Use capture input

volatile uint32_t echo_start = 0;
volatile uint32_t echo_end = 0;
volatile uint8_t echo_received = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 4)) {
        TMR32B0IR = (1 << 4);

        static uint8_t edge = 0;
        if (edge == 0) {
            // Rising edge - echo started
            echo_start = TMR32B0CR0;
            // Switch to falling edge
            TMR32B0CCR = (1 << 1) | (1 << 2);
            edge = 1;
        } else {
            // Falling edge - echo ended
            echo_end = TMR32B0CR0;
            echo_received = 1;
            // Switch back to rising edge
            TMR32B0CCR = (1 << 0) | (1 << 2);
            edge = 0;
        }
    }
}

void ultrasonic_init(void) {
    // Trigger pin as output
    GPIO2DIR |= TRIGGER_PIN;
    GPIO2DATA &= ~TRIGGER_PIN;

    // Capture init (similar to earlier)
    // ...
}

void send_trigger(void) {
    GPIO2DATA |= TRIGGER_PIN;
    delay_us(10);
    GPIO2DATA &= ~TRIGGER_PIN;
}

uint32_t get_distance_cm(void) {
    echo_received = 0;
    send_trigger();

    // Wait for echo (with timeout)
    uint32_t timeout = 30000;  // 30 ms max
    while (!echo_received && timeout--) {
        delay_us(1);
    }

    if (echo_received) {
        uint32_t pulse_us;
        if (echo_end > echo_start) {
            pulse_us = echo_end - echo_start;
        } else {
            pulse_us = (0xFFFFFFFF - echo_start) + echo_end + 1;
        }
        return pulse_us / 58;  // cm
    }
    return 0;  // Timeout
}

void main(void) {
    ultrasonic_init();
    __enable_irq();

    while (1) {
        uint32_t distance = get_distance_cm();
        // Use distance value...
        delay_ms(100);
    }
}
```

### Example 5: Music/Tone Generation

```c
// ============================================
// Generate musical tones using PWM
// ============================================

// Note frequencies (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_REST 0

typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} Note;

// Simple melody
const Note melody[] = {
    { NOTE_C4, 250 },
    { NOTE_D4, 250 },
    { NOTE_E4, 250 },
    { NOTE_F4, 250 },
    { NOTE_G4, 500 },
    { NOTE_G4, 500 },
    { NOTE_A4, 250 },
    { NOTE_A4, 250 },
    { NOTE_A4, 250 },
    { NOTE_A4, 250 },
    { NOTE_G4, 1000 },
    { NOTE_REST, 250 },
    { 0, 0 }  // End marker
};

void tone(uint16_t frequency) {
    if (frequency == 0) {
        // Silence - disable PWM output
        TMR32B0PWMC = 0;
        return;
    }

    uint32_t period = SYSTEM_CLOCK / frequency;
    TMR32B0MR3 = period - 1;          // Period
    TMR32B0MR0 = period / 2;          // 50% duty cycle (square wave)
    TMR32B0PWMC = (1 << 0);           // Enable PWM
}

void no_tone(void) {
    TMR32B0PWMC = 0;
}

void play_melody(const Note *notes) {
    while (notes->duration_ms != 0) {
        tone(notes->frequency);
        delay_ms(notes->duration_ms);
        no_tone();
        delay_ms(50);  // Small gap between notes
        notes++;
    }
}

void main(void) {
    pwm_init(440, 50);  // Initialize PWM

    while (1) {
        play_melody(melody);
        delay_ms(2000);
    }
}
```

---

## Timer Math and Calculations

### Common Calculations

```
═══════════════════════════════════════════════════════════════
TIMER CLOCK CALCULATIONS
═══════════════════════════════════════════════════════════════

Given: System Clock = 72 MHz

Timer Clock = System Clock / (Prescaler + 1)

Example prescalers:
  PR = 0    → Timer Clock = 72 MHz (13.9 ns/tick)
  PR = 71   → Timer Clock = 1 MHz (1 µs/tick)
  PR = 7199 → Timer Clock = 10 kHz (100 µs/tick)
  PR = 71999 → Timer Clock = 1 kHz (1 ms/tick)

═══════════════════════════════════════════════════════════════
DELAY CALCULATIONS
═══════════════════════════════════════════════════════════════

Time Delay = (Match Value + 1) × (Prescaler + 1) / System Clock

Rearranged to find Match Value:
Match Value = (Time Delay × System Clock) / (Prescaler + 1) - 1

Example: 100 ms delay at 72 MHz, PR = 71
  Match = (0.1 × 72,000,000) / 72 - 1
  Match = 100,000 - 1 = 99,999

═══════════════════════════════════════════════════════════════
INTERRUPT FREQUENCY CALCULATIONS
═══════════════════════════════════════════════════════════════

Interrupt Frequency = System Clock / [(Prescaler + 1) × (Match + 1)]

Example: 1 kHz interrupt (1 ms period)
  1000 = 72,000,000 / [(PR + 1) × (MR + 1)]
  (PR + 1) × (MR + 1) = 72,000

  Option 1: PR = 71, MR = 999     (72 × 1000 = 72,000) ✓
  Option 2: PR = 0, MR = 71999    (1 × 72000 = 72,000) ✓
  Option 3: PR = 7199, MR = 9     (7200 × 10 = 72,000) ✓

═══════════════════════════════════════════════════════════════
PWM CALCULATIONS
═══════════════════════════════════════════════════════════════

PWM Frequency = System Clock / [(Prescaler + 1) × (Period Register + 1)]

PWM Duty Cycle = (Duty Register + 1) / (Period Register + 1) × 100%

Example: 20 kHz PWM at 72 MHz, no prescaler
  Period = 72,000,000 / 20,000 - 1 = 3599

  For 25% duty: Duty Register = (3600 × 0.25) - 1 = 899
  For 50% duty: Duty Register = (3600 × 0.50) - 1 = 1799
  For 75% duty: Duty Register = (3600 × 0.75) - 1 = 2699

═══════════════════════════════════════════════════════════════
PWM RESOLUTION CALCULATIONS
═══════════════════════════════════════════════════════════════

Resolution (bits) = log2(Period + 1)

Example: Period = 3599
  Resolution = log2(3600) ≈ 11.8 bits

Higher frequency = lower resolution (fewer steps)
Lower frequency = higher resolution (more steps)

At 72 MHz:
  1 kHz PWM → Period = 71999 → ~16 bits resolution
  10 kHz PWM → Period = 7199 → ~12.8 bits resolution
  100 kHz PWM → Period = 719 → ~9.5 bits resolution
```

### Resolution vs Frequency Trade-off

```
System Clock: 72 MHz

PWM Freq    Period    Resolution    Typical Use
────────    ──────    ──────────    ────────────────────────
100 Hz      719999    ~19.4 bits    Servos (need low freq)
1 kHz       71999     ~16.1 bits    LED dimming
10 kHz      7199      ~12.8 bits    Motor control
20 kHz      3599      ~11.8 bits    Audio (above hearing)
50 kHz      1439      ~10.5 bits    Power supplies
100 kHz     719       ~9.5 bits     High-speed switching
```

---

## Common Patterns

### Pattern 1: Timer Initialization Template

```c
void timer_init_template(void) {
    // 1. Enable clock to timer peripheral
    SYSAHBCLKCTRL |= (1 << TIMER_CLOCK_BIT);

    // 2. Reset timer
    TIMERx_TCR = 0x02;  // Assert reset
    TIMERx_TCR = 0x00;  // Release reset

    // 3. Set prescaler
    TIMERx_PR = PRESCALER_VALUE;

    // 4. Set match value(s)
    TIMERx_MR0 = MATCH_VALUE;

    // 5. Configure match actions
    TIMERx_MCR = MATCH_CONTROL_VALUE;

    // 6. Clear pending interrupts
    TIMERx_IR = 0x1F;

    // 7. Enable interrupt in NVIC (if using interrupts)
    NVIC_EnableIRQ(TIMERx_IRQn);

    // 8. Start timer
    TIMERx_TCR = 0x01;
}
```

### Pattern 2: Generic Delay Function

```c
// Reusable delay configuration
typedef struct {
    volatile uint32_t *tcr;
    volatile uint32_t *pr;
    volatile uint32_t *mr0;
    volatile uint32_t *mcr;
    volatile uint32_t *ir;
    uint32_t system_clock;
} DelayTimer;

void delay_generic(DelayTimer *timer, uint32_t us) {
    // Calculate ticks for desired delay
    uint32_t ticks = (us * (timer->system_clock / 1000000));

    // Configure for one-shot
    *timer->pr = 0;
    *timer->mr0 = ticks;
    *timer->mcr = (1 << 2);  // Stop on match

    // Clear and start
    *timer->ir = 0x1F;
    *timer->tcr = 0x02;  // Reset
    *timer->tcr = 0x01;  // Start

    // Wait for stop
    while (*timer->tcr & 0x01);
}
```

### Pattern 3: PWM Abstraction

```c
typedef struct {
    volatile uint32_t *period_reg;
    volatile uint32_t *duty_reg;
    volatile uint32_t *pwm_ctrl;
    uint32_t channel_bit;
    uint32_t max_period;
} PwmChannel;

void pwm_set_duty_percent(PwmChannel *ch, uint8_t percent) {
    if (percent > 100) percent = 100;
    *ch->duty_reg = (ch->max_period * percent) / 100;
}

void pwm_enable(PwmChannel *ch) {
    *ch->pwm_ctrl |= ch->channel_bit;
}

void pwm_disable(PwmChannel *ch) {
    *ch->pwm_ctrl &= ~ch->channel_bit;
}
```

### Pattern 4: Timeout with Timer

```c
// Non-blocking timeout check
uint8_t wait_with_timeout(uint8_t (*condition)(void), uint32_t timeout_ms) {
    uint32_t start = get_ticks();

    while (!condition()) {
        if ((get_ticks() - start) >= timeout_ms) {
            return 0;  // Timeout
        }
    }
    return 1;  // Success
}

// Usage:
uint8_t check_button(void) {
    return BUTTON_PRESSED();
}

void main(void) {
    if (wait_with_timeout(check_button, 5000)) {
        // Button pressed within 5 seconds
    } else {
        // Timeout
    }
}
```

---

## Troubleshooting

### Problem: Timer doesn't count

**Possible causes:**
1. Timer clock not enabled
   ```c
   // Fix: Enable timer clock
   SYSAHBCLKCTRL |= (1 << 9);  // CT32B0
   ```

2. Timer not started
   ```c
   // Fix: Set counter enable bit
   TMR32B0TCR = 0x01;
   ```

3. Timer stuck in reset
   ```c
   // Fix: Clear reset bit
   TMR32B0TCR &= ~0x02;
   ```

### Problem: Timer interrupt doesn't fire

**Possible causes:**
1. Interrupt not enabled in MCR
   ```c
   // Fix: Enable MR0 interrupt
   TMR32B0MCR |= (1 << 0);
   ```

2. Interrupt not enabled in NVIC
   ```c
   // Fix: Enable in NVIC
   NVIC_EnableIRQ(CT32B0_IRQn);
   ```

3. Global interrupts disabled
   ```c
   // Fix: Enable global interrupts
   __enable_irq();
   ```

4. Interrupt flag not cleared in ISR
   ```c
   // Fix: Clear flag in ISR
   TMR32B0IR = (1 << 0);
   ```

### Problem: PWM not working

**Possible causes:**
1. Pin not configured for timer function
   ```c
   // Fix: Set IOCON for timer function
   IOCON_PIO1_6 = 0x02;  // CT32B0_MAT0
   ```

2. PWM not enabled in PWMC register
   ```c
   // Fix: Enable PWM for channel
   TMR32B0PWMC |= (1 << 0);
   ```

3. Period register (MR3) not set
   ```c
   // Fix: Set period
   TMR32B0MR3 = period - 1;
   TMR32B0MCR |= (1 << 10);  // Reset on MR3
   ```

4. Duty > Period (always high or always low)
   ```c
   // Fix: Ensure duty < period
   if (duty > period) duty = period;
   ```

### Problem: Timing is wrong

**Check:**
1. System clock frequency assumption
2. Prescaler calculation
3. Match value calculation
4. Off-by-one errors (use N-1 for N counts)

```c
// Common mistake:
TMR32B0MR0 = 1000;  // Matches at count 1000, not after 1000 counts!

// Correct:
TMR32B0MR0 = 999;   // Matches after 1000 counts (0-999)
```

---

## Quick Reference

### Timer Clock Enable Bits (SYSAHBCLKCTRL)

| Timer | Bit | Value |
|-------|-----|-------|
| CT16B0 | 7 | 0x0080 |
| CT16B1 | 8 | 0x0100 |
| CT32B0 | 9 | 0x0200 |
| CT32B1 | 10 | 0x0400 |

### NVIC Interrupt Numbers

| Timer | IRQ Number | Handler Name |
|-------|------------|--------------|
| CT16B0 | 16 | CT16B0_IRQHandler |
| CT16B1 | 17 | CT16B1_IRQHandler |
| CT32B0 | 18 | CT32B0_IRQHandler |
| CT32B1 | 19 | CT32B1_IRQHandler |

### Match Control Register (MCR) Bits

| Bits | Match | Interrupt | Reset | Stop |
|------|-------|-----------|-------|------|
| 0-2 | MR0 | Bit 0 | Bit 1 | Bit 2 |
| 3-5 | MR1 | Bit 3 | Bit 4 | Bit 5 |
| 6-8 | MR2 | Bit 6 | Bit 7 | Bit 8 |
| 9-11 | MR3 | Bit 9 | Bit 10 | Bit 11 |

### Common Prescaler Values (72 MHz system)

| Prescaler (PR) | Timer Clock | Time per Tick |
|----------------|-------------|---------------|
| 0 | 72 MHz | 13.9 ns |
| 71 | 1 MHz | 1 µs |
| 7199 | 10 kHz | 100 µs |
| 71999 | 1 kHz | 1 ms |

### Quick Formulas

```
Timer Clock = System_Clock / (PR + 1)

Match Time = (MR + 1) / Timer_Clock

Interrupt Frequency = Timer_Clock / (MR + 1)

PWM Duty % = (Duty_MR + 1) / (Period_MR + 1) × 100
```

### Code Snippets

```c
// Enable timer clock
SYSAHBCLKCTRL |= (1 << 9);  // CT32B0

// Reset timer
TMR32B0TCR = 0x02; TMR32B0TCR = 0x00;

// Start timer
TMR32B0TCR = 0x01;

// Stop timer
TMR32B0TCR = 0x00;

// Clear interrupt flag
TMR32B0IR = (1 << 0);  // MR0

// Periodic interrupt (reset on match)
TMR32B0MCR = (1 << 0) | (1 << 1);

// One-shot (stop on match)
TMR32B0MCR = (1 << 0) | (1 << 2);

// PWM mode
TMR32B0MR3 = period - 1;       // Period
TMR32B0MR0 = duty;             // Duty cycle
TMR32B0MCR = (1 << 10);        // Reset on MR3
TMR32B0PWMC = (1 << 0);        // Enable PWM ch 0
```

---

## Further Practice

1. **Metronome**: Create an audible/visual metronome with adjustable BPM
2. **Reaction Game**: Measure reaction time when LED lights up
3. **Fan Controller**: PWM control based on temperature (simulated or ADC)
4. **MIDI Player**: Parse MIDI-like data and play notes
5. **Oscilloscope Trigger**: Use input capture to implement trigger levels
6. **Frequency Counter**: Display measured frequency on LEDs or serial
7. **Multi-channel PWM**: Control RGB LED strips with fading effects

---

## What's Next?

You can now generate precise timing and control PWM outputs. But how do you communicate with a computer for debugging or talk to other devices? Time to learn serial communication!

**Next Chapter:** [Chapter 5: UART Communication](05-uart-serial-communication.md) - Send and receive serial data, implement printf debugging.

---

*Chapter 4 of the LPC1343 Embedded C Programming Guide*
