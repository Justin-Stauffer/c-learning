/**
 * Chapter 3: GPIO In-Depth - Binary Counter Example
 *
 * Displays a 4-bit binary counter on 4 LEDs.
 * Counts from 0 (0000) to 15 (1111) and repeats.
 *
 * Concepts demonstrated:
 *   - GPIO output configuration
 *   - Binary number representation on LEDs
 *   - Simple counting logic
 *
 * Hardware:
 *   - LEDs on P3.0, P3.1, P3.2, P3.3 (active-low)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define NUM_LEDS       4
#define LED_MASK       0x0F
#define MAX_COUNT      16      /* 0 to 15 */
#define DELAY_COUNT    300000  /* Adjust for desired speed */

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void delay(volatile uint32_t count) {
    while (count > 0) {
        count--;
    }
}

void init_leds(void) {
    SYSAHBCLKCTRL |= (1 << 6);

    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All LEDs off */
}

/**
 * Display a 4-bit value on the LEDs
 * @param value: 0-15, where bit 0 = LED0, bit 3 = LED3
 *               1 = LED on, 0 = LED off (handles active-low)
 */
void display_binary(uint8_t value) {
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;                    /* All off */
    current &= ~(value & LED_MASK);         /* Turn on where value has 1s */
    GPIO3DATA = current;
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    uint8_t count = 0;

    init_leds();

    while (1) {
        /*
         * Display count as binary on LEDs:
         *   count = 0  -> 0000 -> no LEDs on
         *   count = 1  -> 0001 -> LED0 on
         *   count = 5  -> 0101 -> LED0 and LED2 on
         *   count = 15 -> 1111 -> all LEDs on
         */
        display_binary(count);

        delay(DELAY_COUNT);

        /* Increment and wrap */
        count++;
        if (count >= MAX_COUNT) {
            count = 0;
        }
    }

    return 0;
}
