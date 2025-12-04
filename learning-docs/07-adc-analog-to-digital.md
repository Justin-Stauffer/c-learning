# Chapter 7: Analog-to-Digital Converter (ADC)

## Reading Analog Signals with the LPC1343

Learn how to measure real-world analog voltages using the on-chip ADC to interface with sensors, potentiometers, and other analog devices.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 1 | What is an ADC? | Beginner |
| Part 2 | LPC1343 ADC hardware | Beginner |
| Part 3 | Basic ADC reading | Beginner |
| Part 4 | ADC configuration details | Intermediate |
| Part 5 | Multiple channels | Intermediate |
| Part 6 | Interrupt-driven ADC | Advanced |
| Part 7 | Practical sensor examples | Intermediate |

**Prerequisites:** Chapter 0-3 (GPIO basics), Chapter 6 (Interrupts helpful but not required)

---

## Part 1: What is an ADC?

### The Analog vs Digital World

Microcontrollers are digital devices - they work with discrete values (0 or 1, HIGH or LOW). But the real world is analog - temperature, light, sound, and position all vary continuously.

```
Analog Signal (continuous):        Digital Signal (discrete):

Voltage                            Voltage
  ^                                  ^
  |    ∿∿∿∿∿∿∿∿                      |  ┌──┐  ┌──┐  ┌──┐
  |   ∿        ∿                     |  │  │  │  │  │  │
  |  ∿          ∿                    |  │  │  │  │  │  │
  | ∿            ∿                   |──┘  └──┘  └──┘  └──
  +──────────────→ Time              +──────────────────→ Time
```

### ADC = Analog-to-Digital Converter

An ADC measures an analog voltage and converts it to a digital number:

```
                    ADC
Analog Input  ─────────────────►  Digital Output
  (0V - 3.3V)                      (0 - 1023)

Examples:
  0.0V  →  0
  1.65V →  512  (middle of range)
  3.3V  →  1023
```

### Resolution: How Precise?

The LPC1343 has a **10-bit ADC**, meaning it can distinguish 2^10 = 1024 different voltage levels:

```
10-bit ADC Resolution:

Voltage Range: 0V to 3.3V
Levels: 1024 (0 to 1023)
Step Size: 3.3V / 1024 ≈ 3.22 mV per step

       3.3V ┤ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ = 1023
            │
            │
       1.65V┤ ▓▓▓▓▓▓▓▓▓▓           = 512
            │
            │
         0V ┤                       = 0
            └────────────────────────
```

### Common ADC Applications

| Application | Sensor Type | Output |
|-------------|-------------|--------|
| Temperature | Thermistor, LM35 | Voltage proportional to temp |
| Light | LDR, Phototransistor | Resistance/voltage varies with light |
| Position | Potentiometer | Voltage from 0V to VCC |
| Battery Level | Voltage divider | Scaled battery voltage |
| Sound | Microphone | AC voltage signal |
| Pressure | Pressure sensor | Voltage proportional to pressure |

---

## Part 2: LPC1343 ADC Hardware

### ADC Specifications

| Parameter | Value |
|-----------|-------|
| Resolution | 10 bits (0-1023) |
| Channels | 8 (AD0-AD7) |
| Reference Voltage | VDD (3.3V) |
| Conversion Time | ~2.44 µs at 4.5 MHz |
| Sample Rate | Up to 400 ksps |
| Input Range | 0V to VDD |

### ADC Pin Mapping

```
LPC1343 ADC Channels:

Channel │ Pin    │ IOCON Register    │ Function Select
────────┼────────┼───────────────────┼─────────────────
AD0     │ P0.11  │ IOCON_R_PIO0_11   │ FUNC = 0x02
AD1     │ P1.0   │ IOCON_R_PIO1_0    │ FUNC = 0x02
AD2     │ P1.1   │ IOCON_R_PIO1_1    │ FUNC = 0x02
AD3     │ P1.2   │ IOCON_R_PIO1_2    │ FUNC = 0x02
AD4     │ P1.3   │ IOCON_SWDIO_PIO1_3│ FUNC = 0x02
AD5     │ P1.4   │ IOCON_PIO1_4      │ FUNC = 0x01
AD6     │ P1.10  │ IOCON_PIO1_10     │ FUNC = 0x01
AD7     │ P1.11  │ IOCON_PIO1_11     │ FUNC = 0x01
```

**Note:** Some pins are shared with other functions (like SWD debug). AD4 shares with SWDIO!

### ADC Register Addresses

```c
/* ADC Registers */
#define AD0CR           (*((volatile uint32_t *)0x4001C000))  /* Control */
#define AD0GDR          (*((volatile uint32_t *)0x4001C004))  /* Global Data */
#define AD0INTEN        (*((volatile uint32_t *)0x4001C00C))  /* Interrupt Enable */
#define AD0DR0          (*((volatile uint32_t *)0x4001C010))  /* Channel 0 Data */
#define AD0DR1          (*((volatile uint32_t *)0x4001C014))  /* Channel 1 Data */
#define AD0DR2          (*((volatile uint32_t *)0x4001C018))  /* Channel 2 Data */
#define AD0DR3          (*((volatile uint32_t *)0x4001C01C))  /* Channel 3 Data */
#define AD0DR4          (*((volatile uint32_t *)0x4001C020))  /* Channel 4 Data */
#define AD0DR5          (*((volatile uint32_t *)0x4001C024))  /* Channel 5 Data */
#define AD0DR6          (*((volatile uint32_t *)0x4001C028))  /* Channel 6 Data */
#define AD0DR7          (*((volatile uint32_t *)0x4001C02C))  /* Channel 7 Data */
#define AD0STAT         (*((volatile uint32_t *)0x4001C030))  /* Status */
```

### ADC Control Register (AD0CR)

```
AD0CR - ADC Control Register (0x4001C000)

Bits 31-28: Reserved
Bit  27:    EDGE     - Start on rising (0) or falling (1) edge
Bits 26-24: START    - Start condition:
                       000 = No start (software controlled)
                       001 = Start now
                       010-111 = Start on external trigger
Bit  21:    PDN      - Power: 0=power down, 1=operational
Bits 20-17: Reserved
Bits 16-8:  CLKDIV   - Clock divider (ADC_CLK = PCLK/(CLKDIV+1))
Bits 7-0:   SEL      - Channel select (bit per channel)

Example: Read channel 0, start now, ADC powered on
AD0CR = (1 << 0)    |  /* SEL = channel 0 */
        (71 << 8)   |  /* CLKDIV = 71 (72MHz/72 = 1MHz ADC clock) */
        (1 << 21)   |  /* PDN = 1 (power on) */
        (1 << 24);     /* START = 001 (start now) */
```

### ADC Clock Requirements

The ADC needs a clock ≤ 4.5 MHz for accurate conversions:

```
ADC Clock = System Clock / (CLKDIV + 1)

For 72 MHz system clock:
  CLKDIV = 15 → 72MHz/16 = 4.5 MHz (maximum)
  CLKDIV = 71 → 72MHz/72 = 1.0 MHz (commonly used)

For 12 MHz system clock (IRC, no PLL):
  CLKDIV = 2  → 12MHz/3 = 4.0 MHz
  CLKDIV = 11 → 12MHz/12 = 1.0 MHz
```

---

## Part 3: Basic ADC Reading

### Quick Start: Read a Potentiometer

Connect a potentiometer to AD0 (P0.11):

```
Hardware Setup:

    3.3V ─────┬─────────────────
              │
              ▼
         ┌────┴────┐
         │   POT   │  ← 10K potentiometer
         │   ◐     │
         └────┬────┘
              │
              ├──────► P0.11 (AD0)
              │
    GND ──────┴─────────────────

As you turn the pot:
  - Full CCW: ~0V → ADC reads ~0
  - Middle:   ~1.65V → ADC reads ~512
  - Full CW:  ~3.3V → ADC reads ~1023
```

### Complete Example: Single Channel ADC

```c
/**
 * Basic ADC Example - Read potentiometer on AD0 (P0.11)
 */

#include <stdint.h>

/* System Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))

/* IOCON */
#define IOCON_R_PIO0_11 (*((volatile uint32_t *)0x40044074))

/* ADC Registers */
#define AD0CR           (*((volatile uint32_t *)0x4001C000))
#define AD0GDR          (*((volatile uint32_t *)0x4001C004))
#define AD0DR0          (*((volatile uint32_t *)0x4001C010))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR        (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA       (*((volatile uint32_t *)0x50033FFC))

/* Clock bits */
#define ADC_CLK         (1 << 13)
#define GPIO_CLK        (1 << 6)
#define IOCON_CLK       (1 << 16)

/* LED mask */
#define LED_MASK        0x0F

/**
 * Initialize ADC for channel 0
 */
void adc_init(void) {
    /* Enable ADC clock */
    SYSAHBCLKCTRL |= ADC_CLK | IOCON_CLK;

    /* Power up ADC (clear bit 4 in PDRUNCFG) */
    PDRUNCFG &= ~(1 << 4);

    /* Configure P0.11 as AD0 (function 2) */
    IOCON_R_PIO0_11 = 0x02;  /* AD0 function, no pull-up/down */

    /* Configure ADC:
     * - Channel 0 selected (SEL = bit 0)
     * - Clock divider = 71 (72MHz / 72 = 1MHz)
     * - Power on (PDN = 1)
     * - No start yet (START = 0)
     */
    AD0CR = (1 << 0) |      /* SEL = channel 0 */
            (71 << 8) |     /* CLKDIV = 71 */
            (1 << 21);      /* PDN = 1 (power on) */
}

/**
 * Read ADC channel 0
 * Returns: 10-bit value (0-1023)
 */
uint16_t adc_read(void) {
    uint32_t result;

    /* Start conversion (START = 001) */
    AD0CR |= (1 << 24);

    /* Wait for conversion to complete (DONE bit = bit 31) */
    do {
        result = AD0GDR;
    } while ((result & (1 << 31)) == 0);

    /* Stop conversion */
    AD0CR &= ~(7 << 24);

    /* Extract 10-bit result (bits 6-15) */
    return (result >> 6) & 0x3FF;
}

/**
 * Initialize LEDs on P3.0-P3.3
 */
void led_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;
    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All off */
}

/**
 * Display value on LEDs (4-bit bar graph)
 */
void led_bargraph(uint16_t value) {
    uint8_t leds = 0;

    /* Map 0-1023 to 0-4 LEDs */
    if (value > 200) leds |= (1 << 0);  /* LED0 on */
    if (value > 450) leds |= (1 << 1);  /* LED1 on */
    if (value > 700) leds |= (1 << 2);  /* LED2 on */
    if (value > 900) leds |= (1 << 3);  /* LED3 on */

    /* Active-low LEDs */
    GPIO3DATA = (GPIO3DATA | LED_MASK) & ~leds;
}

/**
 * Simple delay
 */
void delay(volatile uint32_t count) {
    while (count--);
}

/**
 * Main program
 */
int main(void) {
    uint16_t adc_value;

    led_init();
    adc_init();

    while (1) {
        /* Read potentiometer */
        adc_value = adc_read();

        /* Display as LED bar graph */
        led_bargraph(adc_value);

        /* Small delay */
        delay(10000);
    }

    return 0;
}
```

### Understanding the ADC Data Register

```
AD0GDR - Global Data Register (0x4001C004)

 31   30   29-27  26-24  23-16  15-6    5-3    2-0
┌────┬────┬──────┬──────┬──────┬───────┬──────┬─────┐
│DONE│OVRN│ Res  │ CHN  │ Res  │V_VREF │ Res  │ Res │
└────┴────┴──────┴──────┴──────┴───────┴──────┴─────┘

DONE (bit 31): 1 = conversion complete
OVERRUN (bit 30): 1 = previous result overwritten
CHN (bits 26-24): Channel that was converted
V_VREF (bits 15-6): 10-bit conversion result

To extract the 10-bit value:
  result = (AD0GDR >> 6) & 0x3FF;
```

---

## Part 4: ADC Configuration Details

### Analog Input Requirements

For accurate ADC readings, the input source should:

```
Input Requirements:

1. Source Impedance < 10kΩ
   - Higher impedance causes inaccurate readings
   - Use op-amp buffer for high-impedance sources

2. Voltage Range: 0V to VDDA (3.3V)
   - Never exceed VDDA!
   - Use voltage divider for higher voltages

3. Signal Bandwidth
   - ADC samples at specific moments
   - Fast-changing signals need proper sampling rate

Voltage Divider for Higher Voltages:

    Vin (0-12V) ─────┬─────────────────
                     │
                    ┌┴┐
                    │ │ R1 = 27kΩ
                    └┬┘
                     ├──────► AD0 (0-3.3V)
                    ┌┴┐
                    │ │ R2 = 10kΩ
                    └┬┘
                     │
    GND ─────────────┴─────────────────

    Vout = Vin × R2/(R1+R2)
    For Vin = 12V: Vout = 12V × 10k/37k = 3.24V ✓
```

### ADC Clock and Conversion Time

```
Conversion Timing:

ADC Clock = System Clock / (CLKDIV + 1)
Conversion Time = 11 ADC clock cycles

At 72 MHz system clock, CLKDIV = 71:
  ADC Clock = 72MHz / 72 = 1 MHz
  Conversion Time = 11 / 1MHz = 11 µs

At 72 MHz system clock, CLKDIV = 15:
  ADC Clock = 72MHz / 16 = 4.5 MHz
  Conversion Time = 11 / 4.5MHz ≈ 2.44 µs

Maximum sample rate ≈ 1 / 2.44µs ≈ 400 ksps
```

### Configuring for Different Channels

```c
/**
 * Initialize ADC for a specific channel
 */
void adc_init_channel(uint8_t channel) {
    /* Enable clocks */
    SYSAHBCLKCTRL |= ADC_CLK | IOCON_CLK;
    PDRUNCFG &= ~(1 << 4);

    /* Configure pin for analog function */
    switch (channel) {
        case 0:
            IOCON_R_PIO0_11 = 0x02;  /* AD0 */
            break;
        case 1:
            /* IOCON for P1.0 */
            (*((volatile uint32_t *)0x40044078)) = 0x02;  /* AD1 */
            break;
        case 2:
            /* IOCON for P1.1 */
            (*((volatile uint32_t *)0x4004407C)) = 0x02;  /* AD2 */
            break;
        /* Add more channels as needed */
    }

    /* Configure ADC for selected channel */
    AD0CR = (1 << channel) |  /* Select channel */
            (71 << 8) |       /* Clock divider */
            (1 << 21);        /* Power on */
}
```

### Averaging for Noise Reduction

ADC readings can be noisy. Averaging multiple samples improves accuracy:

```c
/**
 * Read ADC with averaging
 * @param samples Number of samples to average (power of 2 recommended)
 * @return Averaged 10-bit value
 */
uint16_t adc_read_averaged(uint8_t samples) {
    uint32_t sum = 0;

    for (uint8_t i = 0; i < samples; i++) {
        sum += adc_read();
    }

    return sum / samples;
}

/* Usage */
uint16_t stable_reading = adc_read_averaged(16);  /* Average 16 samples */
```

### Converting to Voltage

```c
/**
 * Convert ADC value to millivolts
 * @param adc_value 10-bit ADC reading (0-1023)
 * @return Voltage in millivolts (0-3300)
 */
uint16_t adc_to_millivolts(uint16_t adc_value) {
    /* 3300mV / 1024 steps = 3.22mV per step */
    /* Using integer math: (value * 3300) / 1024 */
    return (uint32_t)adc_value * 3300 / 1024;
}

/* Usage */
uint16_t voltage_mv = adc_to_millivolts(adc_read());
/* If adc_read() returns 512, voltage_mv = 1617 mV */
```

---

## Part 5: Multiple Channels

### Reading Multiple ADC Channels

You can read from multiple analog inputs by changing the channel selection:

```c
/* IOCON registers for AD channels */
#define IOCON_R_PIO0_11 (*((volatile uint32_t *)0x40044074))  /* AD0 */
#define IOCON_R_PIO1_0  (*((volatile uint32_t *)0x40044078))  /* AD1 */
#define IOCON_R_PIO1_1  (*((volatile uint32_t *)0x4004407C))  /* AD2 */
#define IOCON_R_PIO1_2  (*((volatile uint32_t *)0x40044080))  /* AD3 */

/**
 * Initialize multiple ADC channels
 */
void adc_init_multi(void) {
    SYSAHBCLKCTRL |= ADC_CLK | IOCON_CLK;
    PDRUNCFG &= ~(1 << 4);

    /* Configure all pins for analog function */
    IOCON_R_PIO0_11 = 0x02;  /* AD0 */
    IOCON_R_PIO1_0 = 0x02;   /* AD1 */
    IOCON_R_PIO1_1 = 0x02;   /* AD2 */
    IOCON_R_PIO1_2 = 0x02;   /* AD3 */

    /* Configure ADC (no channel selected yet) */
    AD0CR = (71 << 8) |      /* Clock divider */
            (1 << 21);       /* Power on */
}

/**
 * Read a specific ADC channel
 * @param channel ADC channel (0-7)
 * @return 10-bit ADC value
 */
uint16_t adc_read_channel(uint8_t channel) {
    uint32_t result;

    /* Clear previous channel, select new channel */
    AD0CR = (AD0CR & ~0xFF) | (1 << channel);

    /* Start conversion */
    AD0CR |= (1 << 24);

    /* Wait for completion */
    do {
        result = AD0GDR;
    } while ((result & (1 << 31)) == 0);

    /* Stop conversion */
    AD0CR &= ~(7 << 24);

    /* Return 10-bit result */
    return (result >> 6) & 0x3FF;
}

/* Usage */
int main(void) {
    uint16_t ch0, ch1, ch2, ch3;

    adc_init_multi();

    while (1) {
        ch0 = adc_read_channel(0);  /* Read potentiometer */
        ch1 = adc_read_channel(1);  /* Read temperature sensor */
        ch2 = adc_read_channel(2);  /* Read light sensor */
        ch3 = adc_read_channel(3);  /* Read battery voltage */

        /* Process readings... */
    }
}
```

### Burst Mode (All Channels at Once)

The ADC can sample multiple channels in sequence automatically:

```c
/**
 * Read all selected channels in burst mode
 * @param channels Bitmask of channels to read (bit 0 = AD0, etc.)
 * @param results Array to store results (must be size 8)
 */
void adc_burst_read(uint8_t channels, uint16_t *results) {
    /* Configure for burst mode */
    AD0CR = (channels << 0) |    /* Select channels */
            (71 << 8) |          /* Clock divider */
            (1 << 16) |          /* BURST = 1 */
            (1 << 21);           /* Power on */

    /* Wait for all channels to complete */
    uint32_t done_mask = channels;
    while ((AD0STAT & done_mask) != done_mask);

    /* Read results from individual data registers */
    if (channels & (1 << 0)) results[0] = (AD0DR0 >> 6) & 0x3FF;
    if (channels & (1 << 1)) results[1] = (AD0DR1 >> 6) & 0x3FF;
    if (channels & (1 << 2)) results[2] = (AD0DR2 >> 6) & 0x3FF;
    if (channels & (1 << 3)) results[3] = (AD0DR3 >> 6) & 0x3FF;
    if (channels & (1 << 4)) results[4] = (AD0DR4 >> 6) & 0x3FF;
    if (channels & (1 << 5)) results[5] = (AD0DR5 >> 6) & 0x3FF;
    if (channels & (1 << 6)) results[6] = (AD0DR6 >> 6) & 0x3FF;
    if (channels & (1 << 7)) results[7] = (AD0DR7 >> 6) & 0x3FF;

    /* Stop burst mode */
    AD0CR &= ~(1 << 16);
}
```

---

## Part 6: Interrupt-Driven ADC

### Why Use ADC Interrupts?

Polling wastes CPU cycles waiting for conversions. Interrupts let you:
- Start conversion and do other work
- Get notified when result is ready
- Achieve precise sample timing with timer triggers

### ADC Interrupt Example

```c
/* NVIC */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))
#define ADC_IRQn        24

/* ADC Interrupt Enable Register */
#define AD0INTEN        (*((volatile uint32_t *)0x4001C00C))

volatile uint16_t adc_result = 0;
volatile uint8_t adc_ready = 0;

/**
 * ADC Interrupt Handler
 */
void ADC_IRQHandler(void) {
    uint32_t data = AD0GDR;

    /* Check if conversion is done */
    if (data & (1 << 31)) {
        adc_result = (data >> 6) & 0x3FF;
        adc_ready = 1;
    }

    /* Reading AD0GDR clears the interrupt */
}

/**
 * Initialize ADC with interrupts
 */
void adc_init_interrupt(void) {
    /* Standard ADC init */
    SYSAHBCLKCTRL |= ADC_CLK | IOCON_CLK;
    PDRUNCFG &= ~(1 << 4);
    IOCON_R_PIO0_11 = 0x02;

    /* Configure ADC */
    AD0CR = (1 << 0) |      /* Channel 0 */
            (71 << 8) |     /* Clock divider */
            (1 << 21);      /* Power on */

    /* Enable interrupt on channel 0 completion */
    AD0INTEN = (1 << 0);    /* ADINTEN0 = 1 */

    /* Enable ADC interrupt in NVIC */
    NVIC_ISER = (1 << ADC_IRQn);
}

/**
 * Start ADC conversion (non-blocking)
 */
void adc_start(void) {
    adc_ready = 0;
    AD0CR |= (1 << 24);  /* Start conversion */
}

/**
 * Check if ADC result is ready
 */
uint8_t adc_is_ready(void) {
    return adc_ready;
}

/**
 * Get ADC result
 */
uint16_t adc_get_result(void) {
    return adc_result;
}

/* Usage */
int main(void) {
    adc_init_interrupt();

    while (1) {
        adc_start();

        /* Do other work while waiting... */

        /* Check for result */
        if (adc_is_ready()) {
            uint16_t value = adc_get_result();
            /* Process value */
        }
    }
}
```

### Timer-Triggered ADC

For precise sample timing, trigger ADC from a timer:

```c
/* Timer registers */
#define TMR32B0IR       (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR      (*((volatile uint32_t *)0x40014004))
#define TMR32B0PR       (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR      (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0      (*((volatile uint32_t *)0x40014018))
#define TMR32B0EMR      (*((volatile uint32_t *)0x4001403C))

/**
 * Configure timer to trigger ADC at fixed rate
 * @param sample_rate Samples per second
 */
void adc_timer_init(uint32_t sample_rate) {
    /* Enable timer clock */
    SYSAHBCLKCTRL |= (1 << 9);

    /* Reset timer */
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    /* Configure for sample rate */
    TMR32B0PR = 71;  /* 1µs per tick at 72MHz */
    TMR32B0MR0 = (1000000 / sample_rate) - 1;

    /* Reset on match, enable MAT0 output */
    TMR32B0MCR = (1 << 1);  /* Reset on MR0 */
    TMR32B0EMR = (3 << 4);  /* Toggle MAT0 on match */

    /* Configure ADC for hardware trigger */
    AD0CR = (1 << 0) |      /* Channel 0 */
            (71 << 8) |     /* Clock divider */
            (1 << 21) |     /* Power on */
            (2 << 24);      /* START = 010 (CT32B0_MAT0 trigger) */

    /* Enable ADC interrupt */
    AD0INTEN = (1 << 0);
    NVIC_ISER = (1 << ADC_IRQn);

    /* Start timer */
    TMR32B0TCR = 0x01;
}
```

---

## Part 7: Practical Sensor Examples

### Example 1: Temperature Sensor (LM35)

The LM35 outputs 10mV per degree Celsius:

```
LM35 Connection:

    +Vs (3.3V) ────────┬──────────────────
                       │
                  ┌────┴────┐
                  │  LM35   │
                  │ +  V  - │
                  └──┬──┬──┬┘
                     │  │  │
    3.3V ────────────┘  │  └───────── GND
                        │
                        └──────────► AD0

LM35 Output:
  0°C   →  0.00V  → ADC ~0
  25°C  →  0.25V  → ADC ~78
  100°C →  1.00V  → ADC ~310
```

```c
/**
 * Read temperature from LM35
 * @return Temperature in degrees Celsius × 10 (for 0.1°C resolution)
 */
int16_t read_temperature_lm35(void) {
    uint16_t adc_value = adc_read_averaged(16);

    /* Convert to millivolts */
    uint32_t mv = (uint32_t)adc_value * 3300 / 1024;

    /* LM35: 10mV per °C, so mV/10 = °C */
    /* Return °C × 10 for 0.1° resolution */
    return mv;  /* Already in 0.1°C units! */
}

/* Usage */
int16_t temp = read_temperature_lm35();
/* temp = 235 means 23.5°C */
```

### Example 2: Light Sensor (LDR)

A Light Dependent Resistor (LDR) forms a voltage divider:

```
LDR Circuit:

    3.3V ─────────┬─────────────────────
                  │
                 ┌┴┐
                 │ │ LDR (light dependent)
                 └┬┘
                  │
                  ├──────────────► AD0
                  │
                 ┌┴┐
                 │ │ 10kΩ fixed resistor
                 └┬┘
                  │
    GND ──────────┴─────────────────────

Behavior:
  Bright light → LDR low resistance → high voltage → high ADC
  Dark        → LDR high resistance → low voltage → low ADC
```

```c
/**
 * Read light level from LDR
 * @return Light level 0-100%
 */
uint8_t read_light_percent(void) {
    uint16_t adc_value = adc_read_averaged(8);

    /* Map 0-1023 to 0-100 */
    return (uint32_t)adc_value * 100 / 1023;
}

/**
 * Detect if room is dark
 * @return 1 if dark, 0 if light
 */
uint8_t is_dark(void) {
    return read_light_percent() < 20;  /* Below 20% = dark */
}
```

### Example 3: Battery Voltage Monitor

Monitor a Li-Po battery (3.0V - 4.2V) using a voltage divider:

```
Battery Monitor:

    Battery+ (3.0-4.2V) ──┬───────────────
                          │
                         ┌┴┐
                         │ │ R1 = 10kΩ
                         └┬┘
                          │
                          ├────────────► AD0 (0-2.1V)
                          │
                         ┌┴┐
                         │ │ R2 = 10kΩ
                         └┬┘
                          │
    GND ──────────────────┴───────────────

Voltage divider: Vout = Vin × R2/(R1+R2) = Vin × 0.5
  4.2V battery → 2.1V at ADC
  3.0V battery → 1.5V at ADC
```

```c
/**
 * Read battery voltage
 * @return Battery voltage in millivolts
 */
uint16_t read_battery_mv(void) {
    uint16_t adc_value = adc_read_averaged(16);

    /* Convert to ADC millivolts */
    uint32_t adc_mv = (uint32_t)adc_value * 3300 / 1024;

    /* Multiply by 2 for voltage divider */
    return adc_mv * 2;
}

/**
 * Get battery percentage (Li-Po: 3.0V = 0%, 4.2V = 100%)
 */
uint8_t get_battery_percent(void) {
    uint16_t mv = read_battery_mv();

    if (mv < 3000) return 0;
    if (mv > 4200) return 100;

    /* Linear interpolation */
    return (mv - 3000) * 100 / 1200;
}
```

### Example 4: Potentiometer for Menu Selection

Use a potentiometer to select menu options:

```c
/**
 * Get menu selection from potentiometer (1-4)
 */
uint8_t get_menu_selection(void) {
    uint16_t adc = adc_read();

    /* Divide into 4 zones with hysteresis */
    if (adc < 200) return 1;
    if (adc < 450) return 2;
    if (adc < 700) return 3;
    return 4;
}

/* Usage with LEDs */
void show_selection(uint8_t selection) {
    GPIO3DATA |= LED_MASK;  /* All off */
    GPIO3DATA &= ~(1 << (selection - 1));  /* Selected LED on */
}

int main(void) {
    led_init();
    adc_init();

    while (1) {
        uint8_t sel = get_menu_selection();
        show_selection(sel);
        delay(50000);
    }
}
```

---

## Quick Reference

### Register Summary

| Register | Address | Description |
|----------|---------|-------------|
| AD0CR | 0x4001C000 | ADC Control |
| AD0GDR | 0x4001C004 | Global Data (last conversion) |
| AD0INTEN | 0x4001C00C | Interrupt Enable |
| AD0DR0-7 | 0x4001C010-2C | Channel Data Registers |
| AD0STAT | 0x4001C030 | Status |

### AD0CR Bit Fields

| Bits | Name | Description |
|------|------|-------------|
| 7-0 | SEL | Channel select (1 bit per channel) |
| 15-8 | CLKDIV | Clock divider |
| 16 | BURST | Burst mode enable |
| 21 | PDN | Power down (0=off, 1=on) |
| 26-24 | START | Start control |

### Common Formulas

```c
/* ADC clock calculation */
ADC_Clock = System_Clock / (CLKDIV + 1)

/* Conversion to millivolts */
mV = ADC_Value * 3300 / 1024

/* Conversion to voltage (float) */
V = ADC_Value * 3.3f / 1024.0f

/* Temperature from LM35 (°C) */
temp_C = (ADC_Value * 3300 / 1024) / 10

/* Voltage with divider */
Vin = (ADC_Value * 3300 / 1024) * (R1 + R2) / R2
```

### Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Reading always 0 | ADC not powered | Set PDRUNCFG bit 4 = 0 |
| Reading always 1023 | Pin not configured | Set IOCON for analog function |
| Noisy readings | No averaging | Use adc_read_averaged() |
| Wrong channel | SEL bits wrong | Check channel selection in AD0CR |
| No conversion | START not set | Set START bits (24-26) |

---

## Troubleshooting

### ADC Reads 0 or 1023 Always

```c
/* Check power */
if (PDRUNCFG & (1 << 4)) {
    /* ADC is powered down! */
    PDRUNCFG &= ~(1 << 4);
}

/* Check clock */
if (!(SYSAHBCLKCTRL & (1 << 13))) {
    /* ADC clock not enabled! */
    SYSAHBCLKCTRL |= (1 << 13);
}
```

### Readings Are Unstable

```c
/* Solution 1: Average multiple readings */
uint16_t stable = adc_read_averaged(16);

/* Solution 2: Add decoupling capacitor */
/* Hardware: 100nF ceramic cap from AD pin to GND */

/* Solution 3: Reduce ADC clock speed */
AD0CR = (AD0CR & ~(0xFF << 8)) | (143 << 8);  /* Slower clock */
```

### Reading Wrong Channel

```c
/* Verify channel selection */
uint8_t current_channel = AD0CR & 0xFF;
/* Should be (1 << desired_channel) */

/* Verify IOCON configuration */
/* Each channel needs correct IOCON function */
```

---

## What's Next?

With ADC mastered, you can now:
- Read analog sensors (temperature, light, pressure)
- Create analog control interfaces (joysticks, potentiometers)
- Monitor battery levels
- Sample audio signals

**Next Chapter:** [Chapter 8: I2C Communication](08-i2c-communication.md) - Connect to digital sensors, EEPROMs, and displays using the I2C bus.

---

**Navigation:**
- Previous: [Chapter 6: Interrupts and Clocks](06-interrupts-and-clocks.md)
- Next: [Chapter 8: I2C Communication](08-i2c-communication.md)
- [Back to Index](00-index.md)

---

*Chapter 7 of the LPC1343 Embedded C Learning Series*
*Analog-to-Digital Converter: Reading the Analog World*
