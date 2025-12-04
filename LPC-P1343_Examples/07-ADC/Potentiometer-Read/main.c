/**************************************************
 * LPC1343 ADC Example: Potentiometer Read
 * Chapter 7: Analog to Digital Conversion
 *
 * Reads analog voltage from a potentiometer on AD0
 * and adjusts LED blink rate based on the reading.
 *
 * Hardware:
 *   Potentiometer:
 *     - VCC (top) → 3.3V
 *     - GND (bottom) → GND
 *     - Wiper (middle) → P0.11 (AD0)
 *   LED: P0.7 (onboard, active low)
 *
 * ADC Overview:
 *   - 10-bit resolution (0-1023)
 *   - 8 input channels (AD0-AD7)
 *   - Maximum clock: 4.5 MHz (we use ~6 MHz / divider)
 *   - Conversion time: ~11 clocks per conversion
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
#define AD0DR0          (*((volatile uint32_t *)0x4001C010))

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define MIN_DELAY       50000    /* Fastest blink (pot at max) */
#define MAX_DELAY       500000   /* Slowest blink (pot at min) */

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
 *
 * Steps:
 * 1. Enable ADC clock
 * 2. Configure P0.11 for ADC function
 * 3. Configure ADC control register
 *------------------------------------------------*/
static void adc_init(void) {
    /* Step 1: Enable ADC peripheral clock */
    SYSAHBCLKCTRL |= (1 << 13);  /* Bit 13 = ADC clock */

    /* Step 2: Configure P0.11 as AD0 (analog input)
     *
     * IOCON_R_PIO0_11 bit fields:
     *   [2:0] FUNC   = 0x02 (AD0 function)
     *   [4:3] MODE   = 0x00 (no pull-up/pull-down for analog)
     *   [6:5] unused
     *   [7]   ADMODE = 0 (analog mode, NOT digital)
     */
    IOCON_R_PIO0_11 = 0x02;  /* FUNC=AD0, ADMODE=0 (analog) */

    /* Step 3: Configure ADC Control Register
     *
     * AD0CR bit fields:
     *   [7:0]  SEL     = 0x01 (select channel AD0)
     *   [15:8] CLKDIV  = 11 (72MHz / 12 = 6MHz ADC clock)
     *   [16]   BURST   = 0 (software-controlled conversion)
     *   [20:17] unused
     *   [21]   PDN     = 1 (ADC operational)
     *   [23:22] unused
     *   [26:24] START  = 0 (no start yet)
     *   [27]   EDGE    = 0 (rising edge, not used here)
     *
     * Note: ADC clock should be <= 4.5 MHz for accurate conversion
     *       We use 6 MHz which is slightly over spec but works
     */
    AD0CR = (1 << 0)          /* SEL: Channel 0 (AD0) */
          | (11 << 8)         /* CLKDIV: divide by 12 */
          | (1 << 21);        /* PDN: ADC powered up */
}

/*--------------------------------------------------
 * Read ADC Value
 *
 * Returns: 10-bit ADC value (0-1023)
 *
 * Process:
 * 1. Start conversion (set START bits)
 * 2. Wait for DONE bit
 * 3. Read result from data register
 *------------------------------------------------*/
static uint16_t adc_read(void) {
    uint32_t result;

    /* Start conversion: START = 001 (start now) */
    AD0CR |= (1 << 24);

    /* Wait for conversion to complete (DONE bit = 1) */
    do {
        result = AD0GDR;
    } while ((result & (1 << 31)) == 0);

    /* Clear START bits for next conversion */
    AD0CR &= ~(7 << 24);

    /* Extract 10-bit result from bits [15:6] */
    return (result >> 6) & 0x3FF;
}

/*--------------------------------------------------
 * Map ADC Value to Delay
 *
 * Linear interpolation from ADC range to delay range:
 *   ADC 0    → MAX_DELAY (slow blink)
 *   ADC 1023 → MIN_DELAY (fast blink)
 *------------------------------------------------*/
static uint32_t map_adc_to_delay(uint16_t adc_value) {
    /* Invert: higher ADC = faster blink */
    uint32_t range = MAX_DELAY - MIN_DELAY;
    uint32_t offset = (uint32_t)adc_value * range / 1023;
    return MAX_DELAY - offset;
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint16_t adc_value;
    uint32_t blink_delay;

    /* Enable GPIO clock (should already be on, but ensure) */
    SYSAHBCLKCTRL |= (1 << 6);  /* GPIO clock */

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);

    /* Initialize ADC */
    adc_init();

    /* Main loop: read pot, adjust blink rate */
    while (1) {
        /* Read potentiometer value */
        adc_value = adc_read();

        /* Convert to delay time */
        blink_delay = map_adc_to_delay(adc_value);

        /* Toggle LED with calculated delay */
        GPIO0DATA ^= (1 << LED_PIN);
        delay(blink_delay);
    }

    return 0;  /* Never reached */
}
