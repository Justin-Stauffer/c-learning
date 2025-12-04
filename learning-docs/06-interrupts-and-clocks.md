# Chapter 6: Interrupts and Clocks

A comprehensive guide to interrupt-driven programming and clock/PLL configuration for efficient embedded systems.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 1 | Clock sources and system clock | ⭐⭐ Intermediate |
| Part 2 | PLL configuration | ⭐⭐ Intermediate |
| Part 3 | Interrupt basics | ⭐⭐ Intermediate |
| Part 4 | NVIC deep dive | ⭐⭐⭐ Advanced |
| Part 5 | SysTick timer | ⭐⭐ Intermediate |

**Prerequisites:** [Chapter 4: Timers and PWM](04-timers-and-pwm.md) recommended

---

## Quick Start: Your First SysTick Interrupt

Before diving deep, let's see a working interrupt-driven LED blink using the SysTick timer:

```c
#include <stdint.h>

/* System Control Registers */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/* SysTick Registers (ARM Cortex-M3 core peripheral) */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))  /* Control and Status */
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))  /* Reload Value */
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))  /* Current Value */

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR        (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA       (*((volatile uint32_t *)0x50033FFC))

/* Clock enable */
#define GPIO_CLK        (1 << 6)

/* System clock (assuming 72 MHz after PLL setup) */
#define SYSTEM_CLOCK    72000000UL

volatile uint32_t ms_ticks = 0;

/* SysTick interrupt handler - called every 1ms */
void SysTick_Handler(void) {
    ms_ticks++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

int main(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= GPIO_CLK;

    /* Configure LED (P3.0) as output */
    GPIO3DIR |= (1 << 0);
    GPIO3DATA |= (1 << 0);  /* LED off initially */

    /* Configure SysTick for 1ms interrupts at 72MHz
     * Reload = (72,000,000 / 1000) - 1 = 71,999
     */
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;
    SYST_CVR = 0;           /* Clear current value */
    SYST_CSR = 0x07;        /* Enable, use CPU clock, enable interrupt */

    while (1) {
        GPIO3DATA &= ~(1 << 0);  /* LED ON (active-low) */
        delay_ms(500);
        GPIO3DATA |= (1 << 0);   /* LED OFF */
        delay_ms(500);
    }
}
```

**What makes this different from polling?**
- The CPU isn't stuck in a loop counting
- `SysTick_Handler` is called automatically every 1ms by hardware
- Your main code can do other work between delays
- More accurate timing (hardware-driven)

---

## Part 1: Understanding Clocks

### What is a Clock?

A clock is a repeating electrical signal that pulses HIGH/LOW at a regular frequency. Think of it like a metronome - it keeps time for the CPU.

```
Clock Signal:
     ___     ___     ___     ___
    |   |   |   |   |   |   |   |
___|   |___|   |___|   |___|   |___
    ^   ^   ^   ^   ^   ^   ^   ^
    Each pulse = 1 "clock cycle"
```

The CPU performs one operation per clock cycle. **Faster clock = more operations per second**.

### Clock Frequency

Measured in Hz (Hertz) = cycles per second:
- 1 MHz = 1,000,000 cycles/second
- 12 MHz = 12,000,000 cycles/second
- 72 MHz = 72,000,000 cycles/second

### LPC1343 Clock Sources

The LPC1343 has multiple clock sources:

#### 1. Internal RC Oscillator (IRC)
- Built into the chip
- **12 MHz**
- ±1% accuracy (varies with temperature)
- Available immediately on power-up
- No external components needed
- Default clock source on reset

#### 2. System Oscillator (External Crystal)
- Uses external quartz crystal on XTALIN/XTALOUT pins
- Typically **12 MHz** on LPC-P1343 board
- Very accurate (±0.005%)
- Better for timing-critical applications (UART, USB)
- Requires external crystal and capacitors

#### 3. Watchdog Oscillator
- Low-power internal oscillator
- 7.8 kHz typical (varies 5-14 kHz)
- Used for watchdog timer and low-power modes

#### 4. PLL Output
- Takes IRC or System Oscillator as input
- Multiplies frequency up to **72 MHz**
- Most common clock source for full-speed operation

### Clock-Related Registers

```c
/* System Control Block Registers */
#define SYSPLLCTRL      (*((volatile uint32_t *)0x40048008))  /* PLL Control */
#define SYSPLLSTAT      (*((volatile uint32_t *)0x4004800C))  /* PLL Status */
#define SYSOSCCTRL      (*((volatile uint32_t *)0x40048020))  /* Sys Osc Control */
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))  /* Main Clock Select */
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))  /* Main Clock Update */
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))  /* AHB Clock Divider */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))  /* AHB Clock Control */
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))  /* Power-Down Config */
```

### Main Clock Selection (MAINCLKSEL)

| Value | Clock Source |
|-------|--------------|
| 0x00 | IRC Oscillator (12 MHz) |
| 0x01 | PLL Input (IRC or System Osc) |
| 0x02 | Watchdog Oscillator |
| 0x03 | PLL Output |

### Default Clock Configuration

On reset, the LPC1343 runs at:
- **12 MHz** from the Internal RC Oscillator
- PLL is powered down
- This is safe but slow

---

## Part 2: PLL Configuration

### What is a PLL?

A **PLL (Phase-Locked Loop)** is a circuit that multiplies the input clock frequency to create a faster output clock.

```
Input: 12 MHz  →  [  PLL  ]  →  Output: 72 MHz
                  [× 6    ]
```

### Why Use the PLL?

- The oscillators only provide 12 MHz
- The LPC1343 can run at up to **72 MHz**
- Faster CPU = more instructions per second
- Many peripherals benefit from higher clock speeds

### PLL Parameters

The LPC1343 PLL has two parameters in SYSPLLCTRL:

#### MSEL (bits 4:0) - Feedback Divider
- Determines multiplication factor
- Output frequency = Input × (MSEL + 1)
- Valid range: 0-31

#### PSEL (bits 6:5) - Post Divider
- Keeps internal CCO frequency in valid range (156-320 MHz)
- CCO = Output × 2 × P, where P = 2^PSEL

| PSEL | P value | Division |
|------|---------|----------|
| 0 | 1 | ÷2 |
| 1 | 2 | ÷4 |
| 2 | 4 | ÷8 |
| 3 | 8 | ÷16 |

### PLL Configuration Example: 12 MHz → 72 MHz

```c
/* PLL Control Register */
#define SYSPLLCTRL      (*((volatile uint32_t *)0x40048008))
#define SYSPLLSTAT      (*((volatile uint32_t *)0x4004800C))
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))

void pll_init(void) {
    /* Goal: 12 MHz IRC → 72 MHz system clock
     *
     * MSEL = (72 / 12) - 1 = 5
     * For CCO in range 156-320 MHz:
     *   CCO = 72 × 2 × P
     *   With PSEL=1 (P=2): CCO = 72 × 2 × 2 = 288 MHz ✓
     */

    /* Step 1: Power up the system PLL */
    PDRUNCFG &= ~(1 << 7);      /* Clear bit 7 to power up PLL */

    /* Step 2: Configure PLL
     * MSEL = 5 (bits 4:0) → multiply by 6
     * PSEL = 1 (bits 6:5) → P = 2
     * SYSPLLCTRL = (1 << 5) | 5 = 0x25
     */
    SYSPLLCTRL = (1 << 5) | 5;

    /* Step 3: Wait for PLL to lock */
    while (!(SYSPLLSTAT & 0x01));  /* Wait for LOCK bit */

    /* Step 4: Set system AHB clock divider to 1 (no division) */
    SYSAHBCLKDIV = 1;

    /* Step 5: Select PLL output as main clock */
    MAINCLKSEL = 0x03;          /* Select PLL output */

    /* Step 6: Update main clock selection */
    MAINCLKUEN = 0;             /* Toggle update enable */
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01)); /* Wait for update */

    /* Now running at 72 MHz! */
}
```

### Using External Crystal with PLL

To use the more accurate external crystal:

```c
#define SYSOSCCTRL      (*((volatile uint32_t *)0x40048020))
#define SYSPLLCLKSEL    (*((volatile uint32_t *)0x40048040))
#define SYSPLLCLKUEN    (*((volatile uint32_t *)0x40048044))

void pll_init_external(void) {
    /* Step 1: Power up system oscillator */
    PDRUNCFG &= ~(1 << 5);      /* Clear bit 5 to power up sys osc */

    /* Step 2: Configure system oscillator for 1-20 MHz range */
    SYSOSCCTRL = 0x00;          /* No bypass, 1-20 MHz range */

    /* Wait for oscillator to stabilize (few hundred microseconds) */
    for (volatile int i = 0; i < 1000; i++);

    /* Step 3: Select system oscillator as PLL input */
    SYSPLLCLKSEL = 0x01;        /* 0=IRC, 1=System oscillator */
    SYSPLLCLKUEN = 0;
    SYSPLLCLKUEN = 1;
    while (!(SYSPLLCLKUEN & 0x01));

    /* Step 4: Configure and enable PLL (same as before) */
    PDRUNCFG &= ~(1 << 7);
    SYSPLLCTRL = (1 << 5) | 5;  /* 12 MHz × 6 = 72 MHz */
    while (!(SYSPLLSTAT & 0x01));

    /* Step 5: Switch main clock to PLL output */
    SYSAHBCLKDIV = 1;
    MAINCLKSEL = 0x03;
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01));
}
```

---

## Part 3: Understanding Interrupts

### What is an Interrupt?

An interrupt is like a doorbell. You're doing something, and when someone rings:
1. **Stop** what you're doing
2. **Save** your place (so you can return)
3. **Answer** the door (handle the interrupt)
4. **Return** to what you were doing

In a microcontroller, an interrupt makes the CPU temporarily stop and execute a special function called an **Interrupt Service Routine (ISR)** or **Interrupt Handler**.

### Polling vs Interrupts

#### Polling (inefficient)

```c
while (1) {
    if (button_pressed()) {    /* Check constantly! */
        handle_button();
    }
    if (timer_expired()) {     /* Check constantly! */
        handle_timer();
    }
    if (uart_has_data()) {     /* Check constantly! */
        handle_uart();
    }
    /* CPU is ALWAYS busy checking */
}
```

**Problems:**
- CPU wastes time checking "is it time yet?"
- Can miss events if checking too slowly
- Burns power unnecessarily
- Hard to handle multiple time-critical events

#### Interrupt-Driven (efficient)

```c
int main(void) {
    setup_interrupts();
    while (1) {
        /* CPU can sleep or do other work */
        /* Hardware monitors for events */
    }
}

/* Called automatically by hardware when button pressed */
void GPIO_IRQHandler(void) {
    handle_button();
}

/* Called automatically when timer expires */
void CT32B0_IRQHandler(void) {
    handle_timer();
}

/* Called automatically when UART receives data */
void UART0_IRQHandler(void) {
    handle_uart();
}
```

**Benefits:**
- CPU can sleep (saves power)
- Immediate response to events
- Hardware does the monitoring
- Clean separation of concerns

### Interrupt Flow

```
Normal Program Execution
        ↓
[Interrupt Signal] ← Hardware event occurs
        ↓
CPU saves current state (registers, PC)
        ↓
CPU looks up handler address in Vector Table
        ↓
Jump to Interrupt Handler (ISR)
        ↓
Execute ISR code
        ↓
Clear interrupt flag
        ↓
CPU restores saved state
        ↓
Return to normal execution (exactly where it left off)
```

### LPC1343 Interrupt Sources

| IRQ # | Source | Description |
|-------|--------|-------------|
| -1 | Reset | System reset |
| -5 | SysTick | System tick timer (ARM core) |
| 0-12 | PIO0_0 to PIO0_11 | GPIO Port 0 pin interrupts |
| 13 | PIO0 | GPIO Port 0 combined |
| 14 | PIO1 | GPIO Port 1 combined |
| 15 | PIO2 | GPIO Port 2 combined |
| 16 | CT16B0 | 16-bit Timer 0 |
| 17 | CT16B1 | 16-bit Timer 1 |
| 18 | CT32B0 | 32-bit Timer 0 |
| 19 | CT32B1 | 32-bit Timer 1 |
| 20 | SSP | SPI |
| 21 | UART | UART |
| 22 | USB_IRQ | USB |
| 23 | USB_FIQ | USB Fast |
| 24 | ADC | ADC conversion complete |
| 25 | WDT | Watchdog timer |
| 26 | BOD | Brown-out detect |
| 27 | PIO3 | GPIO Port 3 combined |

---

## Part 4: NVIC (Nested Vectored Interrupt Controller)

The NVIC is the ARM Cortex-M3 hardware that manages all interrupts.

### NVIC Registers

```c
/* NVIC Registers (ARM Cortex-M3 core) */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))  /* Interrupt Set Enable */
#define NVIC_ICER       (*((volatile uint32_t *)0xE000E180))  /* Interrupt Clear Enable */
#define NVIC_ISPR       (*((volatile uint32_t *)0xE000E200))  /* Interrupt Set Pending */
#define NVIC_ICPR       (*((volatile uint32_t *)0xE000E280))  /* Interrupt Clear Pending */
#define NVIC_IPR0       (*((volatile uint32_t *)0xE000E400))  /* Interrupt Priority 0-3 */
#define NVIC_IPR1       (*((volatile uint32_t *)0xE000E404))  /* Interrupt Priority 4-7 */
/* ... IPR registers continue for all interrupts */
```

### Enabling an Interrupt

To enable an interrupt, you need to:
1. Configure the peripheral to generate the interrupt
2. Enable the interrupt in the NVIC

```c
/* Enable Timer CT32B0 interrupt (IRQ 18) */
NVIC_ISER = (1 << 18);

/* Enable UART interrupt (IRQ 21) */
NVIC_ISER = (1 << 21);

/* Enable GPIO Port 0 interrupt (IRQ 13) */
NVIC_ISER = (1 << 13);
```

### Disabling an Interrupt

```c
/* Disable Timer CT32B0 interrupt */
NVIC_ICER = (1 << 18);
```

### Interrupt Priorities

The NVIC supports interrupt priorities (0-3 on LPC1343, where 0 is highest priority).

```c
/* Set CT32B0 (IRQ 18) to priority 1
 * IPR4 handles IRQs 16-19
 * Each IRQ gets 8 bits, but only top 2 bits used on LPC1343
 * Priority value goes in bits [7:6] of each byte
 */
#define NVIC_IPR4       (*((volatile uint32_t *)0xE000E410))

/* IRQ 18 is byte 2 of IPR4 (bits 23:16) */
NVIC_IPR4 = (NVIC_IPR4 & ~(0xFF << 16)) | (1 << 22);  /* Priority 1 */
```

### Nested Interrupts

"Nested" means a higher-priority interrupt can interrupt a lower-priority ISR:

```
Main code running
    ↓
[Low priority interrupt] → ISR_low starts
    ↓
[High priority interrupt] → ISR_high starts (interrupts ISR_low!)
    ↓
ISR_high completes
    ↓
ISR_low continues
    ↓
Return to main code
```

### Vector Table

The vector table is a list of addresses at the start of flash memory. Each entry points to an interrupt handler function:

```c
/* Vector table (in startup_lpc1343_gcc.s) */
Address 0x00000000: Initial Stack Pointer
Address 0x00000004: Reset_Handler
Address 0x00000008: NMI_Handler
Address 0x0000000C: HardFault_Handler
...
Address 0x000000E4: CT16B0_IRQHandler  /* IRQ 16 */
Address 0x000000E8: CT16B1_IRQHandler  /* IRQ 17 */
Address 0x000000EC: CT32B0_IRQHandler  /* IRQ 18 */
Address 0x000000F0: CT32B1_IRQHandler  /* IRQ 19 */
...
Address 0x000000F8: UART0_IRQHandler   /* IRQ 21 */
```

When you define a function with the exact handler name (like `CT32B0_IRQHandler`), the linker places its address in the vector table.

---

## Part 5: SysTick Timer

The SysTick is a simple 24-bit countdown timer built into the ARM Cortex-M3 core. It's ideal for creating a system "heartbeat" or millisecond tick.

### SysTick Registers

```c
/* SysTick Registers */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))  /* Control and Status */
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))  /* Reload Value */
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))  /* Current Value */
#define SYST_CALIB      (*((volatile uint32_t *)0xE000E01C))  /* Calibration */
```

### SYST_CSR (Control and Status Register)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | ENABLE | 1 = Counter enabled |
| 1 | TICKINT | 1 = Generate interrupt when counter reaches 0 |
| 2 | CLKSOURCE | 0 = External clock, 1 = CPU clock |
| 16 | COUNTFLAG | 1 = Counter has counted to 0 since last read |

### SysTick Configuration

```c
#define SYSTEM_CLOCK    72000000UL

void systick_init(uint32_t ticks_per_second) {
    /* Calculate reload value
     * For 1000 Hz (1ms): reload = 72,000,000 / 1000 - 1 = 71,999
     */
    uint32_t reload = (SYSTEM_CLOCK / ticks_per_second) - 1;

    /* Ensure reload fits in 24 bits */
    if (reload > 0xFFFFFF) {
        reload = 0xFFFFFF;
    }

    SYST_RVR = reload;          /* Set reload value */
    SYST_CVR = 0;               /* Clear current value */
    SYST_CSR = (1 << 2) |       /* Use CPU clock */
               (1 << 1) |       /* Enable interrupt */
               (1 << 0);        /* Enable counter */
}

volatile uint32_t system_ticks = 0;

void SysTick_Handler(void) {
    system_ticks++;
}

uint32_t get_ticks(void) {
    return system_ticks;
}

void delay_ms(uint32_t ms) {
    uint32_t start = system_ticks;
    while ((system_ticks - start) < ms);
}
```

---

## Part 6: Timer Interrupts

While SysTick is great for system timing, the 32-bit timers offer more flexibility.

### CT32B0 Interrupt Example

```c
/* Timer CT32B0 Registers */
#define TMR32B0IR       (*((volatile uint32_t *)0x40014000))  /* Interrupt */
#define TMR32B0TCR      (*((volatile uint32_t *)0x40014004))  /* Control */
#define TMR32B0TC       (*((volatile uint32_t *)0x40014008))  /* Counter */
#define TMR32B0PR       (*((volatile uint32_t *)0x4001400C))  /* Prescaler */
#define TMR32B0MCR      (*((volatile uint32_t *)0x40014014))  /* Match Control */
#define TMR32B0MR0      (*((volatile uint32_t *)0x40014018))  /* Match 0 */

/* Clock and NVIC */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))

#define CT32B0_CLK      (1 << 9)
#define CT32B0_IRQn     18

volatile uint32_t timer_ticks = 0;

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & 0x01) {     /* MR0 interrupt */
        TMR32B0IR = 0x01;       /* Clear interrupt flag */
        timer_ticks++;
    }
}

void timer_init(uint32_t interval_ms) {
    /* Enable timer clock */
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Reset timer */
    TMR32B0TCR = 0x02;          /* Reset */
    TMR32B0TCR = 0x00;          /* Release reset */

    /* Configure for 1 MHz tick (1 µs per count) */
    TMR32B0PR = 71;             /* 72 MHz / 72 = 1 MHz */

    /* Set match value for desired interval */
    TMR32B0MR0 = (interval_ms * 1000) - 1;  /* µs to counts */

    /* Configure match control: interrupt and reset on MR0 */
    TMR32B0MCR = (1 << 0) |     /* Interrupt on MR0 */
                 (1 << 1);      /* Reset on MR0 */

    /* Clear pending interrupts */
    TMR32B0IR = 0x1F;

    /* Enable interrupt in NVIC */
    NVIC_ISER = (1 << CT32B0_IRQn);

    /* Start timer */
    TMR32B0TCR = 0x01;
}
```

---

## Best Practices for ISRs

### 1. Keep ISRs Short

```c
/* BAD - too much work in ISR */
void UART0_IRQHandler(void) {
    char buffer[100];
    read_all_data(buffer);
    process_data(buffer);      /* This takes too long! */
    send_response(buffer);
}

/* GOOD - set flag, process in main loop */
volatile uint8_t uart_data_ready = 0;

void UART0_IRQHandler(void) {
    /* Just read the byte and set a flag */
    rx_buffer[rx_head++] = U0RBR;
    uart_data_ready = 1;
}

int main(void) {
    while (1) {
        if (uart_data_ready) {
            process_data();    /* Heavy processing in main loop */
            uart_data_ready = 0;
        }
    }
}
```

### 2. Always Clear Interrupt Flags

```c
void CT32B0_IRQHandler(void) {
    /* MUST clear flag or interrupt fires forever! */
    TMR32B0IR = 0x01;

    /* Do your work */
    toggle_led();
}
```

### 3. Use `volatile` for Shared Variables

```c
volatile uint32_t tick_count = 0;  /* Shared between ISR and main */

void SysTick_Handler(void) {
    tick_count++;  /* Modified in ISR */
}

int main(void) {
    while (tick_count < 1000);  /* Read in main - needs volatile! */
}
```

### 4. Be Careful with Shared Data

```c
/* DANGEROUS - data corruption possible */
volatile uint32_t sensor_value;

void ADC_IRQHandler(void) {
    sensor_value = read_adc();  /* 32-bit write */
}

int main(void) {
    uint32_t value = sensor_value;  /* May read partial update! */
}

/* SAFER - disable interrupts briefly */
int main(void) {
    __disable_irq();
    uint32_t value = sensor_value;
    __enable_irq();
}
```

---

## Quick Reference

### Clock Registers

```c
#define SYSPLLCTRL      (*((volatile uint32_t *)0x40048008))
#define SYSPLLSTAT      (*((volatile uint32_t *)0x4004800C))
#define SYSOSCCTRL      (*((volatile uint32_t *)0x40048020))
#define SYSPLLCLKSEL    (*((volatile uint32_t *)0x40048040))
#define SYSPLLCLKUEN    (*((volatile uint32_t *)0x40048044))
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))
```

### NVIC Registers

```c
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))
#define NVIC_ICER       (*((volatile uint32_t *)0xE000E180))
#define NVIC_ISPR       (*((volatile uint32_t *)0xE000E200))
#define NVIC_ICPR       (*((volatile uint32_t *)0xE000E280))
```

### SysTick Registers

```c
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))
```

### Common IRQ Numbers

```c
#define CT16B0_IRQn     16
#define CT16B1_IRQn     17
#define CT32B0_IRQn     18
#define CT32B1_IRQn     19
#define SSP_IRQn        20
#define UART_IRQn       21
#define ADC_IRQn        24
#define WDT_IRQn        25
#define PIO3_IRQn       27
#define PIO2_IRQn       28
#define PIO1_IRQn       29
#define PIO0_IRQn       30
```

---

## What's Next?

Congratulations! You've completed the core curriculum of the Embedded C Learning Series!

**You now understand:**
- Bitwise operations for register manipulation
- The firmware build process
- GPIO for input/output control
- Timers and PWM for time-based operations
- UART for serial communication
- Interrupts and clock configuration

**Continue your journey:**
- [Return to Index](00-index.md) - Review the complete learning path
- [Chapter 0: Getting Started](00-getting-started.md) - Quick reference
- Build your own projects combining these concepts!

**Suggested projects:**
1. **Reaction timer** - Measure button press reaction time with UART output
2. **PWM LED with serial control** - Adjust brightness via terminal commands
3. **Multi-channel data logger** - Timer-triggered ADC sampling with UART output
4. **State machine** - Interrupt-driven mode switching with button input

---

*Chapter 6 of the Embedded C Learning Series*
*Part of the LPC1343 Learning Library*
