# Chapter 10: Power Management Examples Specifications

## Hardware Requirements
- LPC-P1343 development board
- Button (onboard or from SunFounder kit)
- LED (onboard)
- Optional: Multimeter with µA range for current measurement

---

## Example 1: Sleep-Wakeup
**Status:** CREATED

### Purpose
Demonstrate entering Sleep mode and waking up via button interrupt.

### Hardware Connections
```
Button: P0.1 (onboard BUT1, active low with internal pull-up)
LED:    P0.7 (onboard, active low)
```

### Key Concepts
- Sleep mode (WFI instruction)
- GPIO interrupt for wake-up
- Interrupt-driven button handling
- Power consumption reduction

### Behavior
1. LED blinks 3 times to indicate start
2. MCU enters sleep mode
3. Button press triggers interrupt
4. MCU wakes, LED blinks once
5. Returns to sleep

### Expected Power Reduction
- Active mode: ~10-15 mA
- Sleep mode: ~3-5 mA

---

## Example 2: Deep-Sleep-Wakeup
**Status:** PENDING

### Purpose
Demonstrate Deep Sleep mode with GPIO start logic wake-up.

### Hardware Connections
```
Button: P0.1 (onboard BUT1)
LED:    P0.7 (onboard)
```

### Key Concepts
- Deep Sleep mode configuration
- Start Logic registers for wake-up
- PDRUNCFG register
- Re-initialization after wake

### Deep Sleep Configuration
```c
/* Enable deep sleep in SCR */
SCB_SCR |= (1 << 2);  /* SLEEPDEEP bit */

/* Configure PDRUNCFG for minimum power */
PDRUNCFG = 0x0000EDFC;  /* Power down unused blocks */

/* Configure GPIO wake-up source */
STARTAPRP0 &= ~(1 << 1);   /* Falling edge on P0.1 */
STARTERP0 |= (1 << 1);     /* Enable P0.1 as start source */
STARTRSRP0CLR = (1 << 1);  /* Clear pending */
```

### Expected Power Reduction
- Active mode: ~10-15 mA
- Deep Sleep: ~100-200 µA

---

## Example 3: Low-Power-Blink
**Status:** CREATED

### Purpose
Demonstrate power-efficient LED blinking using sleep between toggles.

### Key Concepts
- SysTick wake-up
- Minimal active time
- Sleep during wait periods

### Behavior
- Blink LED with 1 second period
- Sleep between toggles
- Much lower average current than polling delay

---

## Common Power Management Registers

```c
/* System Control Block */
#define SCB_SCR         (*((volatile uint32_t *)0xE000ED10))

/* Power Configuration */
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))
#define PDSLEEPCFG      (*((volatile uint32_t *)0x40048230))
#define PDAWAKECFG      (*((volatile uint32_t *)0x40048234))

/* Start Logic (Wake-up) */
#define STARTAPRP0      (*((volatile uint32_t *)0x40048200))
#define STARTERP0       (*((volatile uint32_t *)0x40048204))
#define STARTRSRP0CLR   (*((volatile uint32_t *)0x40048208))
#define STARTSRP0       (*((volatile uint32_t *)0x4004820C))

/* Clock Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))
```

---

## Power Modes Summary

| Mode | Entry | Wake Sources | Power | Notes |
|------|-------|--------------|-------|-------|
| Active | Default | N/A | ~10-15 mA | Full operation |
| Sleep | WFI | Any interrupt | ~3-5 mA | Core stopped, peripherals run |
| Deep Sleep | WFI + SLEEPDEEP | Start Logic, WDT | ~100-200 µA | Most peripherals off |
| Deep Power-Down | Special | External reset | ~1 µA | RAM lost |

---

## WFI Macro Definition

```c
/* Wait For Interrupt - puts CPU to sleep */
#define __WFI() __asm volatile ("wfi")
```

---

## Build Notes
- Each example uses the same Makefile template
- startup_lpc1343_gcc.s and lpc1343_flash.ld copied to each folder
- Build with: `make`
- Flash with: `make flash`

---

## Testing Checklist

- [ ] Example 1: Sleep/wake via button works
- [ ] Example 2: Deep sleep/wake via button works
- [ ] Example 3: Power-efficient blink confirmed

---

## Current Measurement Tips

To measure actual power consumption:
1. Insert ammeter in series with power supply
2. Use multimeter with µA range for sleep modes
3. Measure for several seconds and average
4. Consider inrush current at wake-up

---

*Chapter 10 of the LPC1343 Examples Series*
*Power Management*
