/**************************************************
 * LPC1343 ADC Example: Light Sensor
 * Chapter 7: Analog to Digital Conversion
 *
 * Reads light level from a photoresistor (LDR) and
 * turns on LED when ambient light is low (nightlight).
 * Includes hysteresis to prevent flickering.
 *
 * Hardware (Voltage Divider):
 *
 *   3.3V ────┬────
 *            │
 *          ┌─┴─┐
 *          │LDR│  (photoresistor)
 *          └─┬─┘
 *            │
 *            ├──► P0.11 (AD0)
 *            │
 *          ┌─┴─┐
 *          │10K│  (pull-down resistor)
 *          └─┬─┘
 *            │
 *   GND ─────┴────
 *
 * How it works:
 *   - Dark: LDR resistance HIGH → voltage at AD0 LOW
 *   - Light: LDR resistance LOW → voltage at AD0 HIGH
 *   - The 10K resistor forms a voltage divider with the LDR
 *
 * LED: P0.7 (onboard, active low)
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * System Control Registers
 *------------------------------------------------*/
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/*--------------------------------------------------
 * IOCON Registers (Pin Configuration)
 *------------------------------------------------*/
#define IOCON_R_PIO0_11 (*((volatile uint32_t *)0x4004407C))

/*--------------------------------------------------
 * GPIO Registers
 *------------------------------------------------*/
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))

/*--------------------------------------------------
 * ADC Registers
 *------------------------------------------------*/
#define AD0CR           (*((volatile uint32_t *)0x4001C000))
#define AD0GDR          (*((volatile uint32_t *)0x4001C004))

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7

/* Threshold settings (adjust based on your LDR)
 *
 * ADC values (0-1023):
 *   0 = dark (LED ON)
 *   1023 = bright (LED OFF)
 *
 * Hysteresis prevents flickering at threshold:
 *   - Turn ON when light drops below LOW threshold
 *   - Turn OFF when light rises above HIGH threshold
 */
#define LIGHT_THRESHOLD_LOW   300   /* Turn ON below this */
#define LIGHT_THRESHOLD_HIGH  400   /* Turn OFF above this */

/*--------------------------------------------------
 * State tracking for hysteresis
 *------------------------------------------------*/
static uint8_t led_state = 0;  /* 0 = OFF, 1 = ON */

/*--------------------------------------------------
 * Simple Delay Function
 *------------------------------------------------*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

/*--------------------------------------------------
 * Initialize ADC
 *------------------------------------------------*/
static void adc_init(void) {
    /* Enable ADC peripheral clock */
    SYSAHBCLKCTRL |= (1 << 13);

    /* Configure P0.11 as AD0 (analog input) */
    IOCON_R_PIO0_11 = 0x02;

    /* Configure ADC: Channel 0, ~6MHz clock, powered on */
    AD0CR = (1 << 0)          /* SEL: Channel 0 */
          | (11 << 8)         /* CLKDIV: 72MHz/12 = 6MHz */
          | (1 << 21);        /* PDN: powered up */
}

/*--------------------------------------------------
 * Read ADC Value
 * Returns: 10-bit ADC value (0-1023)
 *------------------------------------------------*/
static uint16_t adc_read(void) {
    uint32_t result;

    /* Start conversion */
    AD0CR |= (1 << 24);

    /* Wait for conversion complete */
    do {
        result = AD0GDR;
    } while ((result & (1 << 31)) == 0);

    /* Clear start bits */
    AD0CR &= ~(7 << 24);

    /* Return 10-bit result */
    return (result >> 6) & 0x3FF;
}

/*--------------------------------------------------
 * LED Control (active low)
 *------------------------------------------------*/
static void led_on(void) {
    GPIO0DATA &= ~(1 << LED_PIN);  /* Clear = ON */
    led_state = 1;
}

static void led_off(void) {
    GPIO0DATA |= (1 << LED_PIN);   /* Set = OFF */
    led_state = 0;
}

/*--------------------------------------------------
 * Update LED with Hysteresis
 *
 * Hysteresis prevents flickering when light level
 * hovers near the threshold. We use two thresholds:
 *
 *            THRESHOLD_HIGH
 *   ─────────────────────────────  OFF zone
 *            hysteresis band
 *   ─────────────────────────────
 *            THRESHOLD_LOW        ON zone
 *
 * If LED is ON:  only turn OFF if above HIGH threshold
 * If LED is OFF: only turn ON if below LOW threshold
 *------------------------------------------------*/
static void update_light_control(uint16_t light_level) {
    if (led_state == 0) {
        /* LED is currently OFF */
        if (light_level < LIGHT_THRESHOLD_LOW) {
            led_on();  /* Dark enough, turn on */
        }
    } else {
        /* LED is currently ON */
        if (light_level > LIGHT_THRESHOLD_HIGH) {
            led_off();  /* Bright enough, turn off */
        }
    }
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint16_t light_level;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);

    /* Start with LED off */
    led_off();

    /* Initialize ADC */
    adc_init();

    /* Main loop: read light, control LED */
    while (1) {
        /* Read light sensor */
        light_level = adc_read();

        /* Update LED based on light level with hysteresis */
        update_light_control(light_level);

        /* Small delay to avoid constant ADC reads */
        delay(50000);
    }

    return 0;
}
