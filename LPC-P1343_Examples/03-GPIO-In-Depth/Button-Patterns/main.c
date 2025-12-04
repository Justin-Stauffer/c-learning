/**
 * Chapter 3: GPIO In-Depth - Button-Controlled Patterns Example
 *
 * Button press cycles through different LED patterns.
 * Demonstrates GPIO input, interrupts, and simple debouncing.
 *
 * Concepts demonstrated:
 *   - GPIO input configuration with pull-up
 *   - GPIO interrupt (falling edge)
 *   - NVIC interrupt enable
 *   - Pattern state machine
 *   - Simple debounce via interrupt disable
 *
 * Hardware:
 *   - LEDs on P3.0-P3.3 (active-low)
 *   - Button on P0.1 (active-low, directly on LPC-P1343 board)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* IOCON */
#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 0 (button) */
#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))
#define GPIO0IS        (*((volatile uint32_t *)0x50008004))
#define GPIO0IBE       (*((volatile uint32_t *)0x50008008))
#define GPIO0IEV       (*((volatile uint32_t *)0x5000800C))
#define GPIO0IE        (*((volatile uint32_t *)0x50008010))
#define GPIO0MIS       (*((volatile uint32_t *)0x50008018))
#define GPIO0IC        (*((volatile uint32_t *)0x5000801C))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define BUTTON_PIN     (1 << 1)  /* P0.1 */

#define DELAY_FAST     50000
#define DELAY_MEDIUM   100000

/*******************************************************************************
 * Pattern State
 ******************************************************************************/

typedef enum {
    PATTERN_ALL_OFF,
    PATTERN_ALL_ON,
    PATTERN_ALTERNATE,
    PATTERN_CHASE,
    NUM_PATTERNS
} Pattern;

volatile Pattern current_pattern = PATTERN_ALL_OFF;
volatile uint8_t pattern_changed = 0;

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

void init_leds(void) {
    SYSAHBCLKCTRL |= (1 << 6);

    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All off */
}

void init_button_interrupt(void) {
    /* Configure P0.1: GPIO function, pull-up, hysteresis */
    IOCON_PIO0_1 = (0x01 << 0)    /* FUNC = GPIO */
                 | (0x02 << 3)    /* MODE = Pull-up */
                 | (0x01 << 5);   /* HYS = enabled */

    /* Set as input */
    GPIO0DIR &= ~BUTTON_PIN;

    /* Configure interrupt: falling edge (button press) */
    GPIO0IS &= ~BUTTON_PIN;   /* Edge-sensitive */
    GPIO0IBE &= ~BUTTON_PIN;  /* Single edge */
    GPIO0IEV &= ~BUTTON_PIN;  /* Falling edge */

    /* Clear pending and enable */
    GPIO0IC = BUTTON_PIN;
    GPIO0IE |= BUTTON_PIN;

    /* Enable GPIO Port 0 interrupt in NVIC (PIO0 is IRQ 31) */
    NVIC_ISER = (1 << 31);
}

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void PIOINT0_IRQHandler(void) {
    if (GPIO0MIS & BUTTON_PIN) {
        /* Disable interrupt for debounce */
        GPIO0IE &= ~BUTTON_PIN;

        /* Advance to next pattern */
        current_pattern++;
        if (current_pattern >= NUM_PATTERNS) {
            current_pattern = PATTERN_ALL_OFF;
        }
        pattern_changed = 1;

        /* Clear interrupt flag */
        GPIO0IC = BUTTON_PIN;

        /* Simple debounce delay */
        delay(100000);

        /* Re-enable interrupt */
        GPIO0IC = BUTTON_PIN;  /* Clear any pending */
        GPIO0IE |= BUTTON_PIN;
    }
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    static uint8_t chase_pos = 0;

    init_leds();
    init_button_interrupt();

    while (1) {
        switch (current_pattern) {
            case PATTERN_ALL_OFF:
                set_leds(0x00);
                break;

            case PATTERN_ALL_ON:
                set_leds(0x0F);
                break;

            case PATTERN_ALTERNATE:
                set_leds(0x05);  /* 0101 */
                delay(DELAY_MEDIUM);
                if (current_pattern != PATTERN_ALTERNATE) break;
                set_leds(0x0A);  /* 1010 */
                delay(DELAY_MEDIUM);
                break;

            case PATTERN_CHASE:
                set_leds(1 << chase_pos);
                delay(DELAY_FAST);
                chase_pos = (chase_pos + 1) % 4;
                break;

            default:
                break;
        }
    }

    return 0;
}
