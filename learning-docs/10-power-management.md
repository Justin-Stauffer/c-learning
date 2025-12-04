# Chapter 10: Power Management

## Sleep Modes and Low-Power Techniques for Battery-Powered Applications

Learn how to minimize power consumption for longer battery life and efficient operation.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 1 | Why power matters | Beginner |
| Part 2 | LPC1343 power architecture | Beginner |
| Part 3 | Sleep modes | Intermediate |
| Part 4 | Peripheral power control | Intermediate |
| Part 5 | Wake-up sources | Intermediate |
| Part 6 | Practical low-power design | Advanced |
| Part 7 | Measuring power consumption | All levels |

**Prerequisites:** Chapter 0-3 (GPIO), Chapter 6 (Interrupts)

---

## Part 1: Why Power Matters

### The Battery Reality

For battery-powered devices, power consumption directly affects:
- **Battery life**: Lower power = longer runtime
- **Battery size**: Lower power = smaller battery
- **Heat generation**: Lower power = cooler device
- **Cost**: Smaller batteries = lower cost

```
Power Consumption vs Battery Life:

LPC1343 at 72 MHz:     ~18 mA active
                        CR2032 (225 mAh) = 12.5 hours

LPC1343 in Sleep:      ~3 mA
                        CR2032 = 75 hours

LPC1343 in Deep Sleep: ~10 µA
                        CR2032 = 2.5 YEARS!

Power optimization can extend battery life by 1000x or more!
```

### Power Consumption Basics

```
Power (W) = Voltage (V) × Current (A)

For digital circuits:
- Higher clock speed = more power
- More active peripherals = more power
- Higher voltage = more power

LPC1343 Power Domains:
┌────────────────────────────────────────┐
│                VDD (3.3V)               │
├────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐            │
│  │   CPU    │  │  Flash   │            │
│  │ Cortex-M3│  │  32 KB   │            │
│  └──────────┘  └──────────┘            │
│                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────┐  │
│  │   RAM    │  │Peripherals│  │ GPIO │  │
│  │   8 KB   │  │(can power│  │      │  │
│  │          │  │  down)   │  │      │  │
│  └──────────┘  └──────────┘  └──────┘  │
│                                         │
│  ┌──────────────────────────────────┐  │
│  │     Always-On Domain (RTC, WDT)  │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘
```

---

## Part 2: LPC1343 Power Architecture

### Power Control Registers

```c
/* Power Control Registers */
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))  /* Power-Down Config */
#define PDSLEEPCFG      (*((volatile uint32_t *)0x40048230))  /* Sleep Power-Down Config */
#define PDAWAKECFG      (*((volatile uint32_t *)0x40048234))  /* Wake Power-Down Config */

/* System Control Registers */
#define SCR             (*((volatile uint32_t *)0xE000ED10))  /* System Control Register */
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))  /* Main Clock Select */
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))  /* Main Clock Update */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))  /* AHB Clock Control */
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))  /* AHB Clock Divider */

/* Wake-up Registers */
#define STARTERP0       (*((volatile uint32_t *)0x40048200))  /* Start Logic 0 */
#define STARTERP1       (*((volatile uint32_t *)0x40048204))  /* Start Logic 1 */
#define STARTAPRP0      (*((volatile uint32_t *)0x40048208))  /* Start Logic Edge 0 */
#define STARTRSRP0CLR   (*((volatile uint32_t *)0x40048210))  /* Start Logic Reset 0 */
```

### PDRUNCFG Register (Power-Down Configuration)

```
PDRUNCFG (0x40048238) - Controls which blocks are powered

Bit │ Function      │ Power Down (1) / Run (0)
────┼───────────────┼───────────────────────────
 0  │ IRCOUT        │ IRC oscillator output
 1  │ IRC           │ IRC oscillator
 2  │ FLASH         │ Flash memory
 3  │ BOD           │ Brown-out detector
 4  │ ADC           │ A/D converter
 5  │ SYSOSC        │ System oscillator
 6  │ WDTOSC        │ Watchdog oscillator
 7  │ SYSPLL        │ System PLL
 8  │ USBPLL        │ USB PLL
 10 │ USBPAD        │ USB PHY

Default: 0x0000EDF0 (most blocks powered down)
```

### Sleep Modes Summary

| Mode | CPU | Flash | RAM | Peripherals | Wake-up Time | Current |
|------|-----|-------|-----|-------------|--------------|---------|
| Active | On | On | On | On | N/A | ~18 mA |
| Sleep | Off | On | On | On | Immediate | ~8 mA |
| Deep Sleep | Off | Off | On | Configurable | ~10 µs | ~3 mA |
| Deep Power-Down | Off | Off | Off | Off | Full reset | ~10 µA |

---

## Part 3: Sleep Modes

### Sleep Mode (WFI)

The simplest power-saving: CPU stops, peripherals continue.

```c
/**
 * Enter Sleep mode
 * CPU halts until interrupt occurs
 */
void enter_sleep(void) {
    /* Clear SLEEPDEEP bit in System Control Register */
    SCR &= ~(1 << 2);

    /* Wait For Interrupt */
    __WFI();

    /* CPU resumes here after interrupt */
}

/* ARM intrinsic for Wait For Interrupt */
#define __WFI() __asm volatile ("wfi")
```

**Usage Example:**

```c
int main(void) {
    led_init();
    systick_init();  /* 1ms tick for wake-up */

    while (1) {
        /* Do work */
        process_data();

        /* Sleep until next tick */
        enter_sleep();
    }
}
```

### Deep Sleep Mode

CPU and flash off, RAM retained, configurable peripherals.

```c
/**
 * Enter Deep Sleep mode
 * Wakes on configured interrupt sources
 */
void enter_deep_sleep(void) {
    /* Configure what stays powered during deep sleep */
    PDSLEEPCFG = (1 << 0) |    /* IRC output off */
                 (1 << 1) |    /* IRC oscillator off */
                 (1 << 2) |    /* Flash off */
                 (1 << 3) |    /* BOD off */
                 (1 << 5) |    /* System osc off */
                 (1 << 6);     /* WDT osc off (unless using for wake) */

    /* Configure power state after wake-up */
    PDAWAKECFG = PDRUNCFG;     /* Same as current state */

    /* Set SLEEPDEEP bit */
    SCR |= (1 << 2);

    /* Wait For Interrupt */
    __WFI();

    /* Clear SLEEPDEEP after wake */
    SCR &= ~(1 << 2);
}
```

### Deep Power-Down Mode

Lowest power: almost everything off, only wake-up logic powered.

```c
/* Power Management Unit Registers */
#define PCON            (*((volatile uint32_t *)0x40038000))  /* Power Control */
#define GPREG0          (*((volatile uint32_t *)0x40038004))  /* General Purpose 0 */
#define GPREG1          (*((volatile uint32_t *)0x40038008))  /* General Purpose 1 */
#define GPREG2          (*((volatile uint32_t *)0x4003800C))  /* General Purpose 2 */
#define GPREG3          (*((volatile uint32_t *)0x40038010))  /* General Purpose 3 */
#define GPREG4          (*((volatile uint32_t *)0x40038014))  /* General Purpose 4 */

/**
 * Enter Deep Power-Down mode
 * Only WAKEUP pin can wake the device
 * @param gpreg_data Data to preserve in GPREG4 (survives deep power-down)
 */
void enter_deep_power_down(uint32_t gpreg_data) {
    /* Store data that survives deep power-down */
    GPREG4 = gpreg_data;

    /* Configure deep power-down */
    PCON = (1 << 1);  /* DPDEN = 1 */

    /* Set SLEEPDEEP */
    SCR |= (1 << 2);

    /* Enter deep power-down */
    __WFI();

    /* Device will reset on wake-up, won't return here */
}

/**
 * Check if we woke from deep power-down
 * @return 1 if woke from deep power-down, 0 otherwise
 */
uint8_t woke_from_deep_power_down(void) {
    return (PCON & (1 << 11)) ? 1 : 0;  /* DPDFLAG */
}

/**
 * Clear deep power-down flag
 */
void clear_deep_power_down_flag(void) {
    PCON |= (1 << 11);  /* Write 1 to clear DPDFLAG */
}

/**
 * Get preserved data from GPREG4
 */
uint32_t get_preserved_data(void) {
    return GPREG4;
}
```

---

## Part 4: Peripheral Power Control

### Disabling Unused Peripheral Clocks

Every enabled peripheral consumes power. Disable what you don't need:

```c
/**
 * Configure peripheral clocks for low power
 * Only enable what's needed
 */
void configure_peripheral_clocks(void) {
    /* Start with minimum: ROM, RAM, Flash, GPIO */
    SYSAHBCLKCTRL = (1 << 0)  |  /* SYS (system) */
                    (1 << 1)  |  /* ROM */
                    (1 << 2)  |  /* RAM */
                    (1 << 4)  |  /* Flash registers */
                    (1 << 5)  |  /* Flash array */
                    (1 << 6);    /* GPIO */

    /* Add specific peripherals as needed:
     * (1 << 7)  - CT16B0
     * (1 << 8)  - CT16B1
     * (1 << 9)  - CT32B0
     * (1 << 10) - CT32B1
     * (1 << 11) - SSP0
     * (1 << 12) - UART
     * (1 << 13) - ADC
     * (1 << 14) - USB
     * (1 << 15) - WDT
     * (1 << 16) - IOCON
     */
}

/**
 * Enable a peripheral clock
 */
void enable_peripheral(uint8_t bit) {
    SYSAHBCLKCTRL |= (1 << bit);
}

/**
 * Disable a peripheral clock
 */
void disable_peripheral(uint8_t bit) {
    SYSAHBCLKCTRL &= ~(1 << bit);
}
```

### Power Down Individual Blocks

```c
/**
 * Power down unused analog blocks
 */
void power_down_analog(void) {
    /* Power down ADC if not needed */
    PDRUNCFG |= (1 << 4);

    /* Power down BOD if not monitoring brownout */
    PDRUNCFG |= (1 << 3);

    /* Power down USB if not using */
    PDRUNCFG |= (1 << 8) | (1 << 10);  /* USB PLL and PHY */
}

/**
 * Power up ADC when needed
 */
void power_up_adc(void) {
    PDRUNCFG &= ~(1 << 4);

    /* Wait for ADC to stabilize */
    for (volatile int i = 0; i < 1000; i++);
}
```

### Reduce Clock Speed

Lower clock = lower power, but slower execution.

```c
/**
 * Switch to IRC (12 MHz) for lower power
 */
void switch_to_irc(void) {
    /* Select IRC as main clock */
    MAINCLKSEL = 0x00;  /* IRC oscillator */

    /* Toggle update enable */
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01));

    /* Power down PLL (no longer needed) */
    PDRUNCFG |= (1 << 7);
}

/**
 * Switch back to PLL (72 MHz)
 */
void switch_to_pll(void) {
    /* Power up PLL */
    PDRUNCFG &= ~(1 << 7);

    /* Wait for PLL lock */
    while (!(SYSPLLSTAT & 0x01));

    /* Select PLL as main clock */
    MAINCLKSEL = 0x03;
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01));
}

/**
 * Reduce AHB clock with divider
 * @param div Divider (1-255, 0 = disable)
 */
void set_clock_divider(uint8_t div) {
    SYSAHBCLKDIV = div;
}
```

---

## Part 5: Wake-up Sources

### GPIO Wake-up (Start Logic)

The LPC1343 has 40 GPIO pins that can wake from deep sleep:

```c
/* Start Logic Registers */
#define STARTERP0       (*((volatile uint32_t *)0x40048200))
#define STARTERP1       (*((volatile uint32_t *)0x40048204))
#define STARTAPRP0      (*((volatile uint32_t *)0x40048208))
#define STARTRSRP0CLR   (*((volatile uint32_t *)0x40048210))
#define STARTSRP0       (*((volatile uint32_t *)0x40048218))

/**
 * Configure GPIO pin as wake-up source
 * @param port GPIO port (0-3)
 * @param pin Pin number within port (0-11)
 * @param edge 0 = falling edge, 1 = rising edge
 */
void configure_wakeup_pin(uint8_t port, uint8_t pin, uint8_t edge) {
    uint32_t bit;

    /* Calculate bit position in start logic registers */
    /* P0.0-P0.11 = bits 0-11 */
    /* P1.0-P1.11 = bits 12-23 */
    /* P2.0-P2.11 = bits 24-35 (split across registers) */
    /* P3.0-P3.3 = bits 36-39 */

    if (port == 0) {
        bit = pin;
    } else if (port == 1) {
        bit = 12 + pin;
    } else if (port == 2) {
        bit = 24 + pin;
    } else {
        bit = 36 + pin;
    }

    if (bit < 32) {
        /* Use STARTERP0 */
        STARTERP0 |= (1 << bit);

        /* Configure edge */
        if (edge) {
            STARTAPRP0 |= (1 << bit);   /* Rising edge */
        } else {
            STARTAPRP0 &= ~(1 << bit);  /* Falling edge */
        }

        /* Clear any pending status */
        STARTRSRP0CLR = (1 << bit);
    } else {
        /* Use STARTERP1 for bits 32+ */
        bit -= 32;
        STARTERP1 |= (1 << bit);
        /* Note: STARTAPRP1 doesn't exist, all falling edge */
    }
}

/**
 * Clear wake-up source
 */
void clear_wakeup_source(uint8_t port, uint8_t pin) {
    uint32_t bit = (port * 12) + pin;

    if (bit < 32) {
        STARTRSRP0CLR = (1 << bit);
    }
}
```

### Button Wake-up Example

```c
/* Button on P0.1 */
#define BUTTON_PORT     0
#define BUTTON_PIN      1

/**
 * Initialize button as wake-up source
 */
void button_wakeup_init(void) {
    /* Configure P0.1 as input with pull-up */
    GPIO0DIR &= ~(1 << BUTTON_PIN);
    IOCON_PIO0_1 = (1 << 4);  /* Pull-up enabled */

    /* Configure as wake-up source (falling edge) */
    configure_wakeup_pin(BUTTON_PORT, BUTTON_PIN, 0);

    /* Enable GPIO interrupt for wake-up */
    NVIC_ISER = (1 << 31);  /* PIO0 interrupt */
}

/**
 * Go to deep sleep, wake on button press
 */
void sleep_until_button(void) {
    /* Clear any pending wake-up */
    clear_wakeup_source(BUTTON_PORT, BUTTON_PIN);

    /* Enter deep sleep */
    enter_deep_sleep();

    /* Woke up! Clear the wake source */
    clear_wakeup_source(BUTTON_PORT, BUTTON_PIN);
}
```

### Timer Wake-up

Use watchdog oscillator for periodic wake-up:

```c
#define WDTOSCCTRL      (*((volatile uint32_t *)0x40048024))

/**
 * Configure watchdog timer for periodic wake-up
 * @param interval_ms Approximate interval in milliseconds
 */
void configure_wdt_wakeup(uint32_t interval_ms) {
    /* Enable watchdog oscillator */
    PDRUNCFG &= ~(1 << 6);

    /* Configure WDT oscillator:
     * DIVSEL = interval based
     * FREQSEL = 1 (0.5 MHz)
     */
    uint32_t divsel = (interval_ms * 500) / 64;  /* Rough calculation */
    if (divsel > 31) divsel = 31;

    WDTOSCCTRL = (divsel << 0) | (1 << 5);

    /* Enable WDT clock */
    SYSAHBCLKCTRL |= (1 << 15);

    /* Configure WDT for interrupt (not reset) */
    /* ... WDT configuration code ... */

    /* Enable WDT interrupt as wake source */
    NVIC_ISER = (1 << 0);  /* WDT interrupt */
}
```

### RTC Wake-up (External RTC)

For precise timing, use an external RTC with I2C and GPIO interrupt:

```c
/**
 * Set alarm on external RTC (e.g., DS3231)
 * RTC INT pin connected to P0.3
 */
void rtc_set_alarm(uint8_t hours, uint8_t minutes) {
    /* Write alarm registers via I2C */
    /* ... I2C write to RTC ... */

    /* Configure P0.3 as wake-up source (falling edge) */
    configure_wakeup_pin(0, 3, 0);
}
```

---

## Part 6: Practical Low-Power Design

### Complete Low-Power Application

```c
/**
 * Battery-powered sensor node example
 * - Wake every 5 seconds
 * - Read sensor
 * - Transmit data
 * - Return to deep sleep
 */

#include <stdint.h>

/* Register definitions... */

volatile uint8_t wakeup_flag = 0;

/**
 * WDT interrupt - wake-up trigger
 */
void WDT_IRQHandler(void) {
    wakeup_flag = 1;
    /* Clear WDT interrupt */
}

/**
 * Initialize for low power
 */
void low_power_init(void) {
    /* Minimal peripheral clocks */
    configure_peripheral_clocks();

    /* Power down unused blocks */
    power_down_analog();

    /* Switch to IRC (12 MHz) */
    switch_to_irc();

    /* Reduce clock to 1 MHz */
    set_clock_divider(12);

    /* Configure WDT wake-up */
    configure_wdt_wakeup(5000);  /* 5 seconds */
}

/**
 * Read sensor (powers up required peripherals)
 */
uint16_t read_sensor(void) {
    uint16_t value;

    /* Power up ADC */
    power_up_adc();
    enable_peripheral(13);  /* ADC clock */

    /* Read sensor */
    value = adc_read();

    /* Power down ADC */
    disable_peripheral(13);
    PDRUNCFG |= (1 << 4);

    return value;
}

/**
 * Transmit data (e.g., via UART or radio)
 */
void transmit_data(uint16_t data) {
    /* Enable UART */
    enable_peripheral(12);

    /* Send data */
    uart_printf("Sensor: %d\r\n", data);

    /* Wait for transmission complete */
    while (!uart_tx_complete());

    /* Disable UART */
    disable_peripheral(12);
}

/**
 * Main application
 */
int main(void) {
    uint16_t sensor_value;

    /* Check if we woke from deep power-down */
    if (woke_from_deep_power_down()) {
        clear_deep_power_down_flag();
        /* Restore state from GPREG if needed */
    }

    /* Initialize low-power mode */
    low_power_init();

    while (1) {
        /* Go to deep sleep */
        wakeup_flag = 0;
        enter_deep_sleep();

        /* Woke up! */
        if (wakeup_flag) {
            /* Read sensor */
            sensor_value = read_sensor();

            /* Transmit if value changed significantly */
            if (sensor_value > last_value + 10 ||
                sensor_value < last_value - 10) {
                transmit_data(sensor_value);
                last_value = sensor_value;
            }
        }
    }
}
```

### Power Mode State Machine

```c
typedef enum {
    POWER_ACTIVE,
    POWER_IDLE,
    POWER_SLEEP,
    POWER_DEEP_SLEEP,
    POWER_DEEP_POWER_DOWN
} PowerMode;

PowerMode current_mode = POWER_ACTIVE;

/**
 * Transition to appropriate power mode based on activity
 */
void power_mode_manager(void) {
    static uint32_t idle_ticks = 0;

    if (has_work_to_do()) {
        idle_ticks = 0;
        current_mode = POWER_ACTIVE;
    } else {
        idle_ticks++;

        if (idle_ticks < 10) {
            /* Brief idle - just WFI */
            current_mode = POWER_IDLE;
            enter_sleep();
        } else if (idle_ticks < 1000) {
            /* Longer idle - deep sleep */
            current_mode = POWER_SLEEP;
            enter_deep_sleep();
        } else {
            /* Very long idle - deep power-down */
            current_mode = POWER_DEEP_POWER_DOWN;
            enter_deep_power_down(idle_ticks);
        }
    }
}
```

### Power-Efficient Peripherals

```c
/**
 * Burst mode - power up, do work fast, power down
 */
void adc_burst_sample(uint16_t *samples, uint32_t count) {
    /* Power up and configure at maximum speed */
    power_up_adc();
    SYSAHBCLKCTRL |= (1 << 13);

    /* Take samples as fast as possible */
    for (uint32_t i = 0; i < count; i++) {
        samples[i] = adc_read_fast();
    }

    /* Power down immediately */
    SYSAHBCLKCTRL &= ~(1 << 13);
    PDRUNCFG |= (1 << 4);
}
```

---

## Part 7: Measuring Power Consumption

### Hardware Setup

```
Power Measurement Circuit:

     USB/Battery
         │
         ▼
    ┌────────────┐
    │ Ammeter    │ (µA range)
    │ or         │
    │ Shunt (1Ω) │◄── Measure voltage here
    └────────────┘
         │
         ▼
    ┌────────────┐
    │  LPC1343   │
    │  VCC       │
    └────────────┘
         │
         ▼
        GND

For accurate measurements:
- Remove USB connection (powers through debugger)
- Use separate power supply
- Measure at different operating points
```

### Software Power Profiler

```c
/**
 * Toggle GPIO to mark power states (for oscilloscope)
 */
#define POWER_MARKER_PIN    (1 << 3)  /* P3.3 */

void power_marker_init(void) {
    GPIO3DIR |= POWER_MARKER_PIN;
    GPIO3DATA &= ~POWER_MARKER_PIN;
}

void power_marker_active(void) {
    GPIO3DATA |= POWER_MARKER_PIN;  /* High = active */
}

void power_marker_sleep(void) {
    GPIO3DATA &= ~POWER_MARKER_PIN;  /* Low = sleep */
}

/* Usage with scope/logic analyzer */
void timed_operation(void) {
    power_marker_active();

    /* Do work */
    perform_measurement();

    power_marker_sleep();
    enter_sleep();
}
```

### Expected Current Measurements

| Mode | Clock | Peripherals | Expected Current |
|------|-------|-------------|------------------|
| Active | 72 MHz | All on | 15-20 mA |
| Active | 12 MHz | All on | 5-8 mA |
| Active | 12 MHz | Minimal | 3-5 mA |
| Sleep | - | All on | 2-3 mA |
| Deep Sleep | - | WDT only | 1-2 mA |
| Deep Sleep | - | None | 200-500 µA |
| Deep Power-Down | - | - | 10-20 µA |

### Power Debugging Tips

```
Common issues causing high power consumption:

1. Floating inputs
   - Unused GPIO pins should be outputs or have pull resistors
   - Floating inputs oscillate, consuming power

2. Unused peripherals enabled
   - Check SYSAHBCLKCTRL
   - Check PDRUNCFG

3. PLL running when not needed
   - Switch to IRC for low-power periods

4. LED or external loads
   - LEDs draw 5-20 mA each!
   - External pull-ups/downs add load

5. Debug interface connected
   - SWD/JTAG debugger powers through pins
   - Disconnect for accurate measurement
```

---

## Quick Reference

### Power Registers

| Register | Address | Description |
|----------|---------|-------------|
| PDRUNCFG | 0x40048238 | Power-down configuration |
| PDSLEEPCFG | 0x40048230 | Sleep power-down config |
| PDAWAKECFG | 0x40048234 | Wake power config |
| PCON | 0x40038000 | Power control |
| SCR | 0xE000ED10 | System control (SLEEPDEEP) |

### Sleep Mode Entry

```c
/* Simple sleep (WFI) */
SCR &= ~(1 << 2);   /* Clear SLEEPDEEP */
__WFI();

/* Deep sleep */
SCR |= (1 << 2);    /* Set SLEEPDEEP */
__WFI();
SCR &= ~(1 << 2);   /* Clear after wake */

/* Deep power-down */
PCON = (1 << 1);    /* DPDEN */
SCR |= (1 << 2);
__WFI();
/* Won't return - device resets */
```

### PDRUNCFG Bits

| Bit | Block | Power Down (1) |
|-----|-------|----------------|
| 0 | IRCOUT | IRC output off |
| 1 | IRC | IRC oscillator off |
| 2 | FLASH | Flash off |
| 3 | BOD | Brown-out detector off |
| 4 | ADC | ADC off |
| 5 | SYSOSC | System oscillator off |
| 6 | WDTOSC | Watchdog oscillator off |
| 7 | SYSPLL | System PLL off |
| 8 | USBPLL | USB PLL off |
| 10 | USBPAD | USB PHY off |

### Power Optimization Checklist

```
□ Disable unused peripheral clocks (SYSAHBCLKCTRL)
□ Power down unused analog blocks (PDRUNCFG)
□ Use lowest clock speed needed
□ Configure unused pins as outputs (low)
□ Use sleep modes when waiting
□ Burst operation: power up, work fast, power down
□ Configure appropriate wake-up sources
□ Remove/disable debug connections for production
```

---

## What's Next?

With power management mastered, you can build battery-powered projects that last months or years. Consider exploring:
- Solar-powered sensors
- Energy harvesting
- Wireless protocols (BLE, LoRa)
- Ultra-low-power displays (e-paper)

**Continue Learning:**
- [Chapter 11: DMA](future) - Offload data transfers for even lower power
- [Chapter 12: USB](future) - USB connectivity
- [Back to Index](00-index.md)

---

**Navigation:**
- Previous: [Chapter 9: SPI Communication](09-spi-communication.md)
- [Back to Index](00-index.md)

---

*Chapter 10 of the LPC1343 Embedded C Learning Series*
*Power Management: Making Every Microamp Count*
