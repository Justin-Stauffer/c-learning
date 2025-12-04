# Chapter 6 Example Specifications

Reference file for creating Interrupts and Clocks example projects. Read this before creating each example.

## Common Files Needed Per Project

Each example folder needs:
1. `main.c` - The example code
2. `Makefile` - Build configuration (copy template, change PROJECT name)
3. `startup_lpc1343_gcc.s` - Copy from 00-Getting-Started
4. `lpc1343_flash.ld` - Copy from 00-Getting-Started
5. `README.md` - Brief description of the example

## Hardware Configuration

```
System Clock: 12 MHz default (IRC), 72 MHz after PLL setup

LEDs (Active-Low):
  P3.0 - LED0
  P3.1 - LED1
  P3.2 - LED2
  P3.3 - LED3
  LED_MASK = 0x0F

Button:
  P0.1 - Main button (active-low)
```

## Register Addresses

```c
/* System Control - Clock Configuration */
#define SYSPLLCTRL      (*((volatile uint32_t *)0x40048008))  /* PLL Control */
#define SYSPLLSTAT      (*((volatile uint32_t *)0x4004800C))  /* PLL Status */
#define SYSOSCCTRL      (*((volatile uint32_t *)0x40048020))  /* Sys Osc Control */
#define SYSPLLCLKSEL    (*((volatile uint32_t *)0x40048040))  /* PLL Clock Select */
#define SYSPLLCLKUEN    (*((volatile uint32_t *)0x40048044))  /* PLL Clock Update */
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))  /* Main Clock Select */
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))  /* Main Clock Update */
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))  /* AHB Clock Divider */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))  /* AHB Clock Control */
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))  /* Power-Down Config */

/* SysTick Registers (ARM Cortex-M3 core) */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))  /* Control and Status */
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))  /* Reload Value */
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))  /* Current Value */

/* NVIC Registers */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))  /* Interrupt Set Enable */
#define NVIC_ICER       (*((volatile uint32_t *)0xE000E180))  /* Interrupt Clear Enable */
#define NVIC_ISPR       (*((volatile uint32_t *)0xE000E200))  /* Interrupt Set Pending */
#define NVIC_ICPR       (*((volatile uint32_t *)0xE000E280))  /* Interrupt Clear Pending */

/* Timer CT32B0 Registers */
#define TMR32B0IR       (*((volatile uint32_t *)0x40014000))  /* Interrupt */
#define TMR32B0TCR      (*((volatile uint32_t *)0x40014004))  /* Control */
#define TMR32B0TC       (*((volatile uint32_t *)0x40014008))  /* Counter */
#define TMR32B0PR       (*((volatile uint32_t *)0x4001400C))  /* Prescaler */
#define TMR32B0MCR      (*((volatile uint32_t *)0x40014014))  /* Match Control */
#define TMR32B0MR0      (*((volatile uint32_t *)0x40014018))  /* Match 0 */

/* Timer CT32B1 Registers */
#define TMR32B1IR       (*((volatile uint32_t *)0x40018000))
#define TMR32B1TCR      (*((volatile uint32_t *)0x40018004))
#define TMR32B1TC       (*((volatile uint32_t *)0x40018008))
#define TMR32B1PR       (*((volatile uint32_t *)0x4001800C))
#define TMR32B1MCR      (*((volatile uint32_t *)0x40018014))
#define TMR32B1MR0      (*((volatile uint32_t *)0x40018018))

/* GPIO Port 0 (Button) */
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))
#define GPIO0IS         (*((volatile uint32_t *)0x50008004))  /* Interrupt Sense */
#define GPIO0IBE        (*((volatile uint32_t *)0x50008008))  /* Interrupt Both Edges */
#define GPIO0IEV        (*((volatile uint32_t *)0x5000800C))  /* Interrupt Event */
#define GPIO0IE         (*((volatile uint32_t *)0x50008010))  /* Interrupt Mask */
#define GPIO0RIS        (*((volatile uint32_t *)0x50008014))  /* Raw Interrupt Status */
#define GPIO0MIS        (*((volatile uint32_t *)0x50008018))  /* Masked Interrupt Status */
#define GPIO0IC         (*((volatile uint32_t *)0x5000801C))  /* Interrupt Clear */

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR        (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA       (*((volatile uint32_t *)0x50033FFC))

/* IOCON for Button */
#define IOCON_PIO0_1    (*((volatile uint32_t *)0x40044010))
```

## Clock Enable Bits (SYSAHBCLKCTRL)

```c
#define GPIO_CLK        (1 << 6)
#define CT16B0_CLK      (1 << 7)
#define CT16B1_CLK      (1 << 8)
#define CT32B0_CLK      (1 << 9)
#define CT32B1_CLK      (1 << 10)
#define IOCON_CLK       (1 << 16)
```

## NVIC IRQ Numbers

```c
#define CT16B0_IRQn     16
#define CT16B1_IRQn     17
#define CT32B0_IRQn     18
#define CT32B1_IRQn     19
#define UART_IRQn       21
#define PIO3_IRQn       27
#define PIO2_IRQn       28
#define PIO1_IRQn       29
#define PIO0_IRQn       31
```

## PLL Configuration

```c
/* 12 MHz IRC → 72 MHz via PLL
 * MSEL = 5 (multiply by 6)
 * PSEL = 1 (P=2, CCO = 72×2×2 = 288 MHz)
 */
#define PLL_MSEL        5
#define PLL_PSEL        1
#define SYSPLLCTRL_VAL  ((PLL_PSEL << 5) | PLL_MSEL)  /* 0x25 */
```

## Project Naming Convention

- SysTick-Blink: `PROJECT = lpc1343_systick_blink`
- PLL-Setup: `PROJECT = lpc1343_pll_setup`
- Multi-Interrupt: `PROJECT = lpc1343_multi_interrupt`
- Button-Interrupt: `PROJECT = lpc1343_button_interrupt`

---

## Example 1: SysTick-Blink

**Status: CREATED** (1052 bytes)

**Concepts:** SysTick timer, interrupt handler, millisecond delay

**Behavior:**
- Configure SysTick for 1ms interrupts
- Provide `delay_ms()` function
- Blink LED using SysTick-based delay
- Show tick count periodically (optional UART output)

**Key code:**
```c
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))

#define SYSTEM_CLOCK    72000000UL

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

void systick_init(void) {
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;  /* 1ms reload */
    SYST_CVR = 0;
    SYST_CSR = 0x07;  /* Enable, CPU clock, interrupt */
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}
```

---

## Example 2: PLL-Setup

**Status: CREATED** (1064 bytes)

**Concepts:** Clock sources, PLL configuration, system clock switching

**Behavior:**
- Start at 12 MHz IRC (default)
- Configure PLL for 72 MHz
- Switch main clock to PLL output
- Blink LED at different speeds to show clock change
- Optional: Use UART to report clock status

**Key code:**
```c
void pll_init_72mhz(void) {
    /* Power up PLL */
    PDRUNCFG &= ~(1 << 7);

    /* Configure: MSEL=5, PSEL=1 */
    SYSPLLCTRL = (1 << 5) | 5;

    /* Wait for lock */
    while (!(SYSPLLSTAT & 0x01));

    /* Set divider to 1 */
    SYSAHBCLKDIV = 1;

    /* Select PLL output */
    MAINCLKSEL = 0x03;
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01));
}
```

---

## Example 3: Multi-Interrupt

**Status: CREATED** (1048 bytes)

**Concepts:** Multiple interrupt sources, interrupt priorities, NVIC configuration

**Behavior:**
- SysTick: System tick counter (1ms)
- CT32B0: Toggle LED0 every 250ms
- CT32B1: Toggle LED1 every 500ms
- Show how interrupts interleave

**Key code:**
```c
void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & 0x01) {
        TMR32B0IR = 0x01;  /* Clear */
        GPIO3DATA ^= (1 << 0);  /* Toggle LED0 */
    }
}

void CT32B1_IRQHandler(void) {
    if (TMR32B1IR & 0x01) {
        TMR32B1IR = 0x01;  /* Clear */
        GPIO3DATA ^= (1 << 1);  /* Toggle LED1 */
    }
}

void timers_init(void) {
    SYSAHBCLKCTRL |= CT32B0_CLK | CT32B1_CLK;

    /* CT32B0: 250ms interval */
    TMR32B0PR = 71;
    TMR32B0MR0 = 250000 - 1;
    TMR32B0MCR = 0x03;
    TMR32B0IR = 0x1F;
    NVIC_ISER = (1 << CT32B0_IRQn);
    TMR32B0TCR = 0x01;

    /* CT32B1: 500ms interval */
    TMR32B1PR = 71;
    TMR32B1MR0 = 500000 - 1;
    TMR32B1MCR = 0x03;
    TMR32B1IR = 0x1F;
    NVIC_ISER = (1 << CT32B1_IRQn);
    TMR32B1TCR = 0x01;
}
```

---

## Example 4: Button-Interrupt

**Status: CREATED** (1028 bytes)

**Concepts:** GPIO interrupt, edge detection, debouncing

**Behavior:**
- Configure button (P0.1) for falling edge interrupt
- Each press toggles LED pattern
- Simple software debounce in ISR
- Show interrupt vs polling approach

**Key code:**
```c
volatile uint8_t button_pressed = 0;
volatile uint32_t last_press_time = 0;

void PIO0_IRQHandler(void) {
    if (GPIO0MIS & (1 << 1)) {  /* Button on P0.1 */
        GPIO0IC = (1 << 1);     /* Clear interrupt */

        /* Simple debounce: ignore if <50ms since last */
        if ((ms_ticks - last_press_time) > 50) {
            button_pressed = 1;
            last_press_time = ms_ticks;
        }
    }
}

void button_interrupt_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK | IOCON_CLK;

    /* Configure P0.1 as input with pull-up */
    IOCON_PIO0_1 = 0x10;  /* Pull-up enabled */
    GPIO0DIR &= ~(1 << 1);

    /* Configure interrupt: falling edge */
    GPIO0IS &= ~(1 << 1);   /* Edge sensitive */
    GPIO0IBE &= ~(1 << 1);  /* Single edge */
    GPIO0IEV &= ~(1 << 1);  /* Falling edge */
    GPIO0IE |= (1 << 1);    /* Enable interrupt */

    /* Clear and enable in NVIC */
    GPIO0IC = (1 << 1);
    NVIC_ISER = (1 << PIO0_IRQn);
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

Chapter 6: Interrupts and Clocks - <Example Name>

## What This Example Demonstrates

- Bullet points

## Hardware

- P3.0-P3.3: LEDs
- P0.1: Button (if used)

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
