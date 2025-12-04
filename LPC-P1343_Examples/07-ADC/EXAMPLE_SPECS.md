# Chapter 7: ADC Examples Specifications

## Hardware Requirements
- LPC-P1343 development board
- Potentiometer (10K) - from SunFounder kit
- Photoresistor (LDR) with 10K resistor - from SunFounder kit
- Joystick module - from SunFounder kit (optional for Example 3)

---

## Example 1: Potentiometer-Read
**Status:** CREATED

### Purpose
Read analog voltage from a potentiometer and indicate value with LED brightness or blink rate.

### Hardware Connections
```
Potentiometer:
  - VCC → 3.3V
  - GND → GND
  - Wiper (middle) → P0.11 (AD0)

LED (onboard):
  - P0.7 (active low)
```

### Key Concepts
- ADC initialization
- 10-bit resolution (0-1023)
- Software start conversion
- Polling for conversion complete
- Mapping ADC value to output

### Register Configuration
```c
SYSAHBCLKCTRL |= (1 << 13);  // Enable ADC clock
AD0CR = (1 << 0)              // SEL: AD0 channel
      | (11 << 8)             // CLKDIV: 72MHz/12 = 6MHz
      | (1 << 21);            // PDN: ADC operational
```

### Expected Behavior
- Turn pot clockwise → LED blinks faster
- Turn pot counter-clockwise → LED blinks slower
- ADC reading printed via UART (optional)

---

## Example 2: Light-Sensor
**Status:** CREATED

### Purpose
Read light level from photoresistor (LDR) and control LED based on ambient light.

### Hardware Connections
```
Light Sensor (Voltage Divider):
  3.3V ────┬────
           │
         ┌─┴─┐
         │LDR│
         └─┬─┘
           │
           ├──► P0.11 (AD0)
           │
         ┌─┴─┐
         │10K│
         └─┬─┘
           │
  GND ─────┴────

LED (onboard):
  - P0.7 (active low)
```

### Key Concepts
- Voltage divider circuit
- Light-dependent resistance
- Threshold detection
- Hysteresis for stable switching

### Expected Behavior
- Dark room → LED ON (nightlight mode)
- Bright room → LED OFF
- Hysteresis prevents flickering near threshold

---

## Example 3: ADC-Multi-Channel
**Status:** PENDING

### Purpose
Demonstrate reading multiple ADC channels (potentiometer + light sensor or joystick X/Y).

### Hardware Connections
```
Potentiometer → P0.11 (AD0)
Light Sensor  → P1.0 (AD1) - Note: Verify pin availability
  OR
Joystick:
  VRx → P0.11 (AD0)
  VRy → P1.0 (AD1)
  SW  → GPIO input (optional)
```

### Key Concepts
- Channel multiplexing
- Sequential channel reading
- Burst mode option
- Data organization

### Expected Behavior
- Read two channels alternately
- Display values via UART
- LED pattern based on combined inputs

---

## Example 4: ADC-Interrupt
**Status:** CREATED

### Purpose
Use ADC interrupt for asynchronous conversion instead of polling.

### Hardware Connections
```
Potentiometer → P0.11 (AD0)
LED → P0.7 (onboard)
```

### Key Concepts
- ADC interrupt enable
- NVIC configuration (ADC IRQ = 24)
- ISR implementation
- Double buffering for readings

### Register Configuration
```c
AD0CR |= (1 << 16);           // BURST mode (continuous)
AD0INTEN = (1 << 0);          // Enable interrupt for AD0
ISER0 |= (1 << 24);           // Enable ADC interrupt in NVIC
```

### Expected Behavior
- ADC runs continuously in background
- ISR updates global variable
- Main loop uses latest value without blocking

---

## Common Register Definitions

```c
/* System Clock Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/* ADC Registers */
#define AD0CR           (*((volatile uint32_t *)0x4001C000))
#define AD0GDR          (*((volatile uint32_t *)0x4001C004))
#define AD0INTEN        (*((volatile uint32_t *)0x4001C00C))
#define AD0DR0          (*((volatile uint32_t *)0x4001C010))
#define AD0DR1          (*((volatile uint32_t *)0x4001C014))
#define AD0STAT         (*((volatile uint32_t *)0x4001C030))

/* IOCON for ADC pins */
#define IOCON_R_PIO0_11 (*((volatile uint32_t *)0x4004407C))
#define IOCON_R_PIO1_0  (*((volatile uint32_t *)0x40044080))

/* ADC configuration bits */
#define ADC_PDN         (1 << 21)
#define ADC_START_NOW   (1 << 24)
#define ADC_DONE        (1 << 31)
```

---

## ADC Pin Function Selection

| Pin | Default | ADC Function | IOCON Value |
|-----|---------|--------------|-------------|
| P0.11 | R/GPIO | AD0 | FUNC=0x02, ADMODE=0 |
| P1.0 | R/GPIO | AD1 | FUNC=0x02, ADMODE=0 |
| P1.1 | R/GPIO | AD2 | FUNC=0x02, ADMODE=0 |
| P1.2 | R/GPIO | AD3 | FUNC=0x02, ADMODE=0 |

---

## Build Notes
- Each example uses the same Makefile template
- startup_lpc1343_gcc.s and lpc1343_flash.ld copied to each folder
- Build with: `make`
- Flash with: `make flash`

---

## Testing Checklist

- [ ] Example 1: Potentiometer controls LED blink rate
- [ ] Example 2: Light sensor controls LED on/off with hysteresis
- [ ] Example 3: Multiple channels read and reported
- [ ] Example 4: Interrupt-driven ADC working

---

*Chapter 7 of the LPC1343 Examples Series*
*ADC - Analog to Digital Conversion*
