/**
 * Chapter 3: GPIO In-Depth - Running Light Example
 *
 * A "Knight Rider" / LED chaser effect where a single LED appears
 * to move back and forth across the available LEDs.
 *
 * Concepts demonstrated:
 *   - GPIO output configuration
 *   - IOCON pin configuration
 *   - Bit shifting for LED patterns
 *   - Active-low LED handling
 *
 * Hardware:
 *   - LEDs on P3.0, P3.1, P3.2, P3.3 (directly on LPC-P1343 board)
 *   - LEDs are active-low (0 = ON, 1 = OFF)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* IOCON - Pin Configuration */
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 3 */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define NUM_LEDS       4
#define LED_MASK       0x0F    /* Bits 0-3 for P3.0-P3.3 */

/* Delay timing */
#define DELAY_MS       100000  /* Adjust for desired speed */

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * Simple delay loop
 */
void delay(volatile uint32_t count) {
    while (count > 0) {
        count--;
    }
}

/**
 * Initialize GPIO for LED output
 */
void init_leds(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure IOCON for GPIO function on P3.0-P3.3 */
    IOCON_PIO3_0 = 0x01;  /* FUNC = GPIO */
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    /* Set P3.0-P3.3 as outputs */
    GPIO3DIR |= LED_MASK;

    /* Start with all LEDs off (set bits high for active-low) */
    GPIO3DATA |= LED_MASK;
}

/**
 * Set LED pattern
 * @param pattern: 4-bit pattern where 1 = LED on (handles active-low inversion)
 */
void set_led_pattern(uint8_t pattern) {
    /* For active-low LEDs: invert the pattern
     * User says 1 (ON) -> we write 0
     * User says 0 (OFF) -> we write 1
     */
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;                    /* All LEDs off first */
    current &= ~(pattern & LED_MASK);       /* Turn on selected LEDs */
    GPIO3DATA = current;
}

/*******************************************************************************
 * Main Running Light Pattern
 ******************************************************************************/

/**
 * Running light pattern - LED moves back and forth
 *
 * Position sequence: 0 -> 1 -> 2 -> 3 -> 2 -> 1 -> 0 -> 1 -> ...
 */
void running_light(void) {
    uint8_t position = 0;
    int8_t direction = 1;  /* 1 = moving right, -1 = moving left */

    while (1) {
        /* Create pattern with single LED at current position */
        uint8_t pattern = (1 << position);

        /* Display the pattern */
        set_led_pattern(pattern);

        /* Wait */
        delay(DELAY_MS);

        /* Move to next position */
        position += direction;

        /* Reverse direction at the ends */
        if (position >= NUM_LEDS - 1) {
            direction = -1;  /* Hit right end, go left */
        } else if (position == 0) {
            direction = 1;   /* Hit left end, go right */
        }
    }
}

/**
 * Alternative: Running light using bit rotation
 * This version keeps a 4-bit pattern and rotates it
 */
void running_light_rotate(void) {
    uint8_t pattern = 0x01;  /* Start with LED0 */
    int8_t direction = 1;    /* 1 = shift left, -1 = shift right */

    while (1) {
        /* Display current pattern */
        set_led_pattern(pattern);
        delay(DELAY_MS);

        /* Shift pattern */
        if (direction == 1) {
            pattern <<= 1;
            if (pattern >= 0x08) {  /* Hit LED3 */
                direction = -1;
            }
        } else {
            pattern >>= 1;
            if (pattern <= 0x01) {  /* Hit LED0 */
                direction = 1;
            }
        }
    }
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    /* Initialize LEDs */
    init_leds();

    /* Run the pattern forever */
    running_light();

    /* Alternative version (uncomment to try):
     * running_light_rotate();
     */

    return 0;  /* Never reached */
}
