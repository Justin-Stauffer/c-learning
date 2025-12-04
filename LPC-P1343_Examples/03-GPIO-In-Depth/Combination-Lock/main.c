/**
 * Chapter 3: GPIO In-Depth - Combination Lock Example
 *
 * Simple combination lock using button press timing.
 * Correct pattern: 4 quick presses within time window.
 * Success flashes all LEDs. Wrong pattern shows error flash.
 *
 * Concepts demonstrated:
 *   - GPIO input with polling
 *   - Edge detection (detecting new button presses)
 *   - State machine for sequence tracking
 *   - Timing-based logic
 *   - Success/failure feedback
 *
 * Hardware:
 *   - LEDs on P3.0-P3.3 (active-low)
 *   - Button on P0.1 (active-low)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))

#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define BUTTON_PIN     (1 << 1)

#define SEQUENCE_LENGTH  4
#define TIMEOUT_COUNT    500000  /* Time window for sequence */
#define DEBOUNCE_COUNT   20000

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void delay(volatile uint32_t count) {
    while (count > 0) count--;
}

void set_leds(uint8_t pattern) {
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;
    current &= ~(pattern & LED_MASK);
    GPIO3DATA = current;
}

void init_hardware(void) {
    SYSAHBCLKCTRL |= (1 << 6);

    /* LEDs */
    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;
    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;

    /* Button with pull-up */
    IOCON_PIO0_1 = (0x01 << 0) | (0x02 << 3) | (0x01 << 5);
    GPIO0DIR &= ~BUTTON_PIN;
}

uint8_t button_pressed(void) {
    return !(GPIO0DATA & BUTTON_PIN);
}

void flash_success(void) {
    for (int i = 0; i < 5; i++) {
        set_leds(0x0F);
        delay(100000);
        set_leds(0x00);
        delay(100000);
    }
}

void flash_error(void) {
    for (int i = 0; i < 3; i++) {
        set_leds(0x0F);
        delay(30000);
        set_leds(0x00);
        delay(30000);
    }
}

void show_progress(uint8_t count) {
    /* Light up LEDs to show progress: 1 press = LED0, 2 = LED0+1, etc */
    uint8_t pattern = (1 << count) - 1;
    set_leds(pattern);
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    uint8_t sequence_count = 0;
    uint8_t last_button = 0;
    uint32_t timeout = 0;

    init_hardware();

    while (1) {
        uint8_t current_button = button_pressed();

        /* Edge detection: detect new press */
        uint8_t newly_pressed = current_button && !last_button;

        if (newly_pressed) {
            /* Debounce */
            delay(DEBOUNCE_COUNT);

            /* Confirm still pressed */
            if (button_pressed()) {
                sequence_count++;
                timeout = TIMEOUT_COUNT;  /* Reset timeout */

                /* Show progress */
                show_progress(sequence_count);

                /* Check for success */
                if (sequence_count >= SEQUENCE_LENGTH) {
                    delay(50000);  /* Brief pause */
                    flash_success();
                    sequence_count = 0;
                    set_leds(0x00);
                }

                /* Wait for release */
                while (button_pressed()) {}
                delay(DEBOUNCE_COUNT);
            }
        }

        /* Timeout check */
        if (sequence_count > 0 && timeout > 0) {
            timeout--;
            if (timeout == 0) {
                /* Timed out - fail */
                flash_error();
                sequence_count = 0;
                set_leds(0x00);
            }
        }

        last_button = current_button;
    }

    return 0;
}
