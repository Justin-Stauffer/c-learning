/**
 * Chapter 1: Bitwise Operations - LED Pattern Demo
 *
 * This example demonstrates all the key bitwise operations from Chapter 1:
 *   - SET a bit:    register |= (1 << n)
 *   - CLEAR a bit:  register &= ~(1 << n)
 *   - TOGGLE a bit: register ^= (1 << n)
 *   - CHECK a bit:  if (register & (1 << n))
 *   - LEFT SHIFT:   value << n
 *   - RIGHT SHIFT:  value >> n
 *
 * The program cycles through several LED patterns:
 *   1. Running light (Knight Rider style) - uses shifts
 *   2. Binary counter - shows all bit combinations
 *   3. Toggle pattern - demonstrates XOR
 *   4. Alternating pattern - even/odd bits
 *
 * Hardware: LPC-P1343 board with LED on P3.0
 *           (For full effect, connect LEDs to P3.0-P3.3)
 * Toolchain: GCC ARM (arm-none-eabi-gcc)
 *
 * Build: make
 * Flash: make flash (requires OpenOCD + ST-Link)
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* GPIO Port 3 Registers */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))  /* Direction */
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))  /* Data (all pins) */

/*******************************************************************************
 * Bit Manipulation Macros
 *
 * These macros implement the four essential patterns from Chapter 1.
 * Using macros makes code more readable and less error-prone.
 ******************************************************************************/

/* Create a bit mask for bit position n */
#define BIT(n)              (1U << (n))

/* Set bit n in register (turn ON) */
#define SET_BIT(reg, n)     ((reg) |= BIT(n))

/* Clear bit n in register (turn OFF) */
#define CLEAR_BIT(reg, n)   ((reg) &= ~BIT(n))

/* Toggle bit n in register (flip state) */
#define TOGGLE_BIT(reg, n)  ((reg) ^= BIT(n))

/* Check if bit n is set (returns non-zero if set) */
#define CHECK_BIT(reg, n)   ((reg) & BIT(n))

/*******************************************************************************
 * Hardware Configuration
 *
 * The LPC-P1343 board has an LED on P3.0.
 * For this demo, we'll use P3.0-P3.3 (4 bits) to show patterns.
 * If you only have one LED, you'll see bit 0 blinking in various patterns.
 ******************************************************************************/

/* LED pins on Port 3 */
#define LED0_PIN        0
#define LED1_PIN        1
#define LED2_PIN        2
#define LED3_PIN        3

/* Mask for all 4 LED pins */
#define LED_MASK        (BIT(LED0_PIN) | BIT(LED1_PIN) | BIT(LED2_PIN) | BIT(LED3_PIN))
#define NUM_LEDS        4

/* Clock enable bit for GPIO */
#define GPIO_CLOCK_BIT  6

/*******************************************************************************
 * Delay Function
 ******************************************************************************/
void delay(volatile uint32_t count) {
    while (count > 0) {
        count--;
    }
}

/* Timing constants (approximate, depends on clock speed) */
#define DELAY_FAST      100000
#define DELAY_MEDIUM    200000
#define DELAY_SLOW      400000

/*******************************************************************************
 * LED Control Functions
 *
 * Note: LEDs on LPC-P1343 are active-low (0 = ON, 1 = OFF)
 * These functions handle the inversion for you.
 ******************************************************************************/

/**
 * Turn ON a specific LED
 * For active-low LEDs, we CLEAR the bit to turn ON
 */
void led_on(uint8_t led_num) {
    CLEAR_BIT(GPIO3DATA, led_num);  /* Clear bit = LED ON (active-low) */
}

/**
 * Turn OFF a specific LED
 * For active-low LEDs, we SET the bit to turn OFF
 */
void led_off(uint8_t led_num) {
    SET_BIT(GPIO3DATA, led_num);    /* Set bit = LED OFF (active-low) */
}

/**
 * Toggle a specific LED
 * XOR flips the bit regardless of current state
 */
void led_toggle(uint8_t led_num) {
    TOGGLE_BIT(GPIO3DATA, led_num);
}

/**
 * Check if LED is ON
 * For active-low: bit=0 means ON, bit=1 means OFF
 */
uint8_t led_is_on(uint8_t led_num) {
    return !CHECK_BIT(GPIO3DATA, led_num);  /* Invert for active-low */
}

/**
 * Set all LEDs to a pattern
 * @param pattern: 4-bit pattern (bit 0 = LED0, etc.)
 *                 1 = LED ON, 0 = LED OFF (logical, not electrical)
 */
void led_set_pattern(uint8_t pattern) {
    /*
     * For active-low LEDs, we need to invert the pattern:
     *   - User says 1 (ON) -> we write 0 (active-low ON)
     *   - User says 0 (OFF) -> we write 1 (active-low OFF)
     *
     * Steps:
     * 1. Clear all LED bits: GPIO3DATA |= LED_MASK (all LEDs OFF)
     * 2. Set the inverted pattern: GPIO3DATA &= ~(pattern & LED_MASK)
     *
     * Or more simply:
     * - Read current value
     * - Clear the LED bits
     * - Set the inverted pattern
     */
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;                    /* All LEDs OFF first */
    current &= ~(pattern & LED_MASK);       /* Turn ON selected LEDs */
    GPIO3DATA = current;
}

/**
 * Turn all LEDs OFF
 */
void led_all_off(void) {
    GPIO3DATA |= LED_MASK;  /* Set all LED bits = all OFF (active-low) */
}

/*******************************************************************************
 * Pattern Demonstrations
 ******************************************************************************/

/**
 * Pattern 1: Running Light (Knight Rider / KITT style)
 *
 * Demonstrates: LEFT SHIFT (<<) and RIGHT SHIFT (>>)
 *
 * A single lit LED moves back and forth across the pins.
 * This uses bit shifting to move the lit position.
 */
void pattern_running_light(uint8_t cycles) {
    uint8_t position = 0;
    int8_t direction = 1;  /* 1 = moving right (higher bits), -1 = moving left */

    for (uint8_t cycle = 0; cycle < cycles * (NUM_LEDS * 2 - 2); cycle++) {
        /* Create pattern with single bit set at current position */
        uint8_t pattern = (1 << position);

        /* Display the pattern */
        led_set_pattern(pattern);
        delay(DELAY_MEDIUM);

        /* Move to next position */
        position += direction;

        /*
         * CHECK if we've reached the end - demonstrates bit checking
         * When position reaches NUM_LEDS-1 or 0, reverse direction
         */
        if (position >= NUM_LEDS - 1) {
            direction = -1;  /* Start moving left */
        } else if (position <= 0) {
            direction = 1;   /* Start moving right */
        }
    }

    led_all_off();
}

/**
 * Pattern 2: Binary Counter
 *
 * Demonstrates: All bit patterns, binary representation
 *
 * Counts from 0 to 15 (0b0000 to 0b1111), displaying each
 * number on the LEDs. This shows all possible 4-bit combinations.
 */
void pattern_binary_counter(uint8_t cycles) {
    for (uint8_t cycle = 0; cycle < cycles; cycle++) {
        /* Count from 0 to 15 (all 4-bit patterns) */
        for (uint8_t count = 0; count < 16; count++) {
            /*
             * The count variable IS the pattern:
             *   count = 0  -> 0b0000 -> no LEDs on
             *   count = 1  -> 0b0001 -> LED0 on
             *   count = 5  -> 0b0101 -> LED0 and LED2 on
             *   count = 15 -> 0b1111 -> all LEDs on
             */
            led_set_pattern(count);
            delay(DELAY_SLOW);
        }
    }

    led_all_off();
}

/**
 * Pattern 3: Toggle Demo
 *
 * Demonstrates: XOR toggle operation
 *
 * Each LED toggles at a different rate, creating an interesting
 * pattern. This shows how XOR flips bits without needing to
 * know the current state.
 */
void pattern_toggle_demo(uint8_t iterations) {
    /* Start with all LEDs off */
    led_all_off();

    for (uint8_t i = 0; i < iterations; i++) {
        /*
         * Toggle different LEDs at different rates:
         * - LED0 toggles every iteration (fastest)
         * - LED1 toggles every 2 iterations
         * - LED2 toggles every 4 iterations
         * - LED3 toggles every 8 iterations (slowest)
         *
         * This creates a binary counter effect using only toggles!
         */

        /* Always toggle LED0 */
        led_toggle(LED0_PIN);

        /* Toggle LED1 every 2nd time (when bit 0 of i is 0) */
        if ((i & BIT(0)) == 0) {
            led_toggle(LED1_PIN);
        }

        /* Toggle LED2 every 4th time (when bits 0-1 of i are 0) */
        if ((i & (BIT(0) | BIT(1))) == 0) {
            led_toggle(LED2_PIN);
        }

        /* Toggle LED3 every 8th time (when bits 0-2 of i are 0) */
        if ((i & (BIT(0) | BIT(1) | BIT(2))) == 0) {
            led_toggle(LED3_PIN);
        }

        delay(DELAY_MEDIUM);
    }

    led_all_off();
}

/**
 * Pattern 4: Alternating Pattern
 *
 * Demonstrates: Bit masks for even/odd positions
 *
 * Alternates between even bits (0,2) and odd bits (1,3).
 * Uses pre-defined masks to select groups of bits.
 */
void pattern_alternating(uint8_t cycles) {
    /*
     * Define masks for even and odd bit positions:
     * EVEN_MASK = 0b0101 = bits 0 and 2
     * ODD_MASK  = 0b1010 = bits 1 and 3
     */
    #define EVEN_MASK   (BIT(0) | BIT(2))   /* 0b0101 = 0x05 */
    #define ODD_MASK    (BIT(1) | BIT(3))   /* 0b1010 = 0x0A */

    for (uint8_t cycle = 0; cycle < cycles; cycle++) {
        /* Light up even-numbered LEDs (0 and 2) */
        led_set_pattern(EVEN_MASK);
        delay(DELAY_SLOW);

        /* Light up odd-numbered LEDs (1 and 3) */
        led_set_pattern(ODD_MASK);
        delay(DELAY_SLOW);
    }

    led_all_off();

    #undef EVEN_MASK
    #undef ODD_MASK
}

/**
 * Pattern 5: Shift Register Effect
 *
 * Demonstrates: Combined shift and OR operations
 *
 * LEDs light up one by one (filling up), then turn off one by one.
 * Uses shifts to build up and tear down the pattern.
 */
void pattern_fill_and_empty(uint8_t cycles) {
    for (uint8_t cycle = 0; cycle < cycles; cycle++) {
        uint8_t pattern = 0;

        /* Fill: add one LED at a time using OR */
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            /*
             * Set the next bit using OR:
             * pattern |= (1 << i)
             *
             * i=0: pattern = 0b0001
             * i=1: pattern = 0b0011
             * i=2: pattern = 0b0111
             * i=3: pattern = 0b1111
             */
            pattern |= (1 << i);
            led_set_pattern(pattern);
            delay(DELAY_MEDIUM);
        }

        /* Small pause when full */
        delay(DELAY_SLOW);

        /* Empty: remove one LED at a time using AND with inverted mask */
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            /*
             * Clear bits from the top using AND:
             * pattern &= ~(1 << (NUM_LEDS - 1 - i))
             *
             * i=0: clear bit 3 -> 0b0111
             * i=1: clear bit 2 -> 0b0011
             * i=2: clear bit 1 -> 0b0001
             * i=3: clear bit 0 -> 0b0000
             */
            pattern &= ~(1 << (NUM_LEDS - 1 - i));
            led_set_pattern(pattern);
            delay(DELAY_MEDIUM);
        }

        /* Small pause when empty */
        delay(DELAY_SLOW);
    }
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/
int main(void) {
    /*
     * Step 1: Enable GPIO clock
     * Uses SET_BIT macro to set bit 6 in SYSAHBCLKCTRL
     */
    SET_BIT(SYSAHBCLKCTRL, GPIO_CLOCK_BIT);

    /*
     * Step 2: Configure LED pins as outputs
     * Set bits 0-3 in GPIO3DIR register
     */
    GPIO3DIR |= LED_MASK;

    /*
     * Step 3: Start with all LEDs off
     */
    led_all_off();

    /*
     * Step 4: Cycle through all patterns forever
     * Each pattern demonstrates different bitwise operations
     */
    while (1) {
        /*
         * Pattern 1: Running Light
         * Demonstrates: << and >> shift operations
         * Watch the LED move back and forth
         */
        pattern_running_light(3);
        delay(DELAY_SLOW);

        /*
         * Pattern 2: Binary Counter
         * Demonstrates: All 4-bit patterns (0-15)
         * Count in binary on the LEDs
         */
        pattern_binary_counter(2);
        delay(DELAY_SLOW);

        /*
         * Pattern 3: Toggle Demo
         * Demonstrates: XOR toggle (^=)
         * Each LED toggles at different rates
         */
        pattern_toggle_demo(32);
        delay(DELAY_SLOW);

        /*
         * Pattern 4: Alternating
         * Demonstrates: Bit masks for groups
         * Even bits, then odd bits
         */
        pattern_alternating(4);
        delay(DELAY_SLOW);

        /*
         * Pattern 5: Fill and Empty
         * Demonstrates: |= to set, &= ~ to clear
         * LEDs fill up then empty
         */
        pattern_fill_and_empty(2);
        delay(DELAY_SLOW);
    }

    return 0;
}
