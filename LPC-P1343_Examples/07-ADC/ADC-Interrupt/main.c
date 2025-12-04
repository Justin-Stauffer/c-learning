/**************************************************
 * LPC1343 ADC Example: Interrupt-Driven ADC
 * Chapter 7: Analog to Digital Conversion
 *
 * Uses ADC interrupt to read potentiometer value
 * without blocking the main loop. The ADC runs
 * continuously in burst mode, and the ISR updates
 * a global variable with the latest reading.
 *
 * Hardware:
 *   Potentiometer → P0.11 (AD0)
 *   LED → P0.7 (onboard, active low)
 *
 * Key Concepts:
 *   - Burst mode (continuous conversions)
 *   - ADC interrupt (IRQ 24)
 *   - Non-blocking ADC reads
 *   - volatile variable for ISR communication
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
#define AD0INTEN        (*((volatile uint32_t *)0x4001C00C))
#define AD0DR0          (*((volatile uint32_t *)0x4001C010))
#define AD0STAT         (*((volatile uint32_t *)0x4001C030))

/*--------------------------------------------------
 * NVIC Registers (Interrupt Controller)
 *------------------------------------------------*/
#define ISER0           (*((volatile uint32_t *)0xE000E100))
#define ICER0           (*((volatile uint32_t *)0xE000E180))

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define ADC_IRQn        24      /* ADC interrupt number */

#define MIN_DELAY       50000   /* Fastest blink */
#define MAX_DELAY       500000  /* Slowest blink */

/*--------------------------------------------------
 * Global Variables (volatile for ISR access)
 *
 * The ISR writes to adc_value, main loop reads it.
 * volatile ensures the compiler doesn't optimize
 * away the reads in the main loop.
 *------------------------------------------------*/
volatile uint16_t adc_value = 0;
volatile uint8_t adc_ready = 0;

/*--------------------------------------------------
 * Simple Delay Function
 *------------------------------------------------*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

/*--------------------------------------------------
 * ADC Interrupt Handler
 *
 * Called automatically when ADC conversion completes.
 * Reads the result and stores in global variable.
 *
 * The handler name must match the vector table entry
 * in startup_lpc1343_gcc.s (ADC_IRQHandler).
 *------------------------------------------------*/
void ADC_IRQHandler(void) {
    uint32_t stat = AD0STAT;
    uint32_t data;

    /* Check if channel 0 has data */
    if (stat & (1 << 0)) {
        /* Read channel 0 data register (clears interrupt) */
        data = AD0DR0;

        /* Extract 10-bit result from bits [15:6] */
        adc_value = (data >> 6) & 0x3FF;
        adc_ready = 1;
    }
}

/*--------------------------------------------------
 * Initialize ADC with Interrupt
 *------------------------------------------------*/
static void adc_init_interrupt(void) {
    /* Enable ADC peripheral clock */
    SYSAHBCLKCTRL |= (1 << 13);

    /* Configure P0.11 as AD0 (analog input) */
    IOCON_R_PIO0_11 = 0x02;

    /* Configure ADC for burst mode:
     *
     * AD0CR bit fields:
     *   [7:0]  SEL    = 0x01 (channel 0)
     *   [15:8] CLKDIV = 11 (72MHz/12 = 6MHz)
     *   [16]   BURST  = 1 (continuous mode)
     *   [21]   PDN    = 1 (powered up)
     *   [26:24] START = 000 (use burst mode instead)
     */
    AD0CR = (1 << 0)          /* SEL: Channel 0 */
          | (11 << 8)         /* CLKDIV: divide by 12 */
          | (1 << 16)         /* BURST: continuous mode */
          | (1 << 21);        /* PDN: powered up */

    /* Enable ADC interrupt for channel 0
     *
     * AD0INTEN bit fields:
     *   [0] ADINTEN0 = 1 (interrupt when AD0 done)
     *   [8] ADGINTEN = 0 (use individual channel interrupts)
     */
    AD0INTEN = (1 << 0);

    /* Enable ADC interrupt in NVIC */
    ISER0 = (1 << ADC_IRQn);
}

/*--------------------------------------------------
 * Map ADC Value to Delay
 *------------------------------------------------*/
static uint32_t map_adc_to_delay(uint16_t value) {
    uint32_t range = MAX_DELAY - MIN_DELAY;
    uint32_t offset = (uint32_t)value * range / 1023;
    return MAX_DELAY - offset;
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint32_t blink_delay;
    uint16_t current_value;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);

    /* Initialize ADC with interrupts */
    adc_init_interrupt();

    /* Main loop: use latest ADC value for LED timing
     *
     * The ADC runs continuously in the background.
     * We just read the latest value and use it.
     */
    while (1) {
        /* Safely read the volatile value */
        current_value = adc_value;

        /* Convert to delay time */
        blink_delay = map_adc_to_delay(current_value);

        /* Toggle LED */
        GPIO0DATA ^= (1 << LED_PIN);

        /* Wait based on potentiometer position */
        delay(blink_delay);
    }

    return 0;
}
