/**
 * Chapter 4: Timers and PWM - LED Dimmer Example
 *
 * Generates PWM on P1.6 (CT32B0_MAT0) at 1kHz to control LED brightness.
 * Button press cycles through brightness levels: 0%, 25%, 50%, 75%, 100%.
 *
 * Concepts demonstrated:
 *   - PWM output configuration
 *   - IOCON pin function selection for timer output
 *   - Match register for period (MR3) and duty cycle (MR0)
 *   - PWM control register
 *   - Button input with polling
 *
 * Hardware:
 *   - P1.6: PWM output (connect external LED with resistor, or use scope)
 *   - P0.1: Button input (on-board)
 *   - P3.0-P3.3: Status LEDs showing current brightness level
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

/* IOCON */
#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 0 (button) */
#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))

/* GPIO Port 3 (status LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* CT32B0 Timer Registers */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))
#define TMR32B0TC      (*((volatile uint32_t *)0x40014008))
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))
#define TMR32B0MR3     (*((volatile uint32_t *)0x40014024))
#define TMR32B0EMR     (*((volatile uint32_t *)0x4001403C))
#define TMR32B0PWMC    (*((volatile uint32_t *)0x40014074))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define BUTTON_PIN     (1 << 1)

#define SYSTEM_CLOCK   72000000UL
#define PWM_FREQUENCY  1000      /* 1 kHz PWM */

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define IOCON_CLK      (1 << 16)
#define CT32B0_CLK     (1 << 9)

/* Brightness levels (percent) */
#define NUM_LEVELS     5
const uint8_t brightness_levels[NUM_LEVELS] = { 0, 25, 50, 75, 100 };

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

uint8_t current_level = 2;  /* Start at 50% */
uint32_t pwm_period;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void delay(volatile uint32_t count) {
    while (count > 0) count--;
}

/*******************************************************************************
 * LED Functions
 ******************************************************************************/

void leds_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;

    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All off */
}

void show_level(uint8_t level) {
    /* Show brightness level on status LEDs:
     * Level 0 (0%):   0 LEDs on
     * Level 1 (25%):  1 LED on
     * Level 2 (50%):  2 LEDs on
     * Level 3 (75%):  3 LEDs on
     * Level 4 (100%): 4 LEDs on
     */
    uint8_t pattern = (1 << level) - 1;  /* 0, 1, 3, 7, 15 */
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;
    current &= ~(pattern & LED_MASK);
    GPIO3DATA = current;
}

/*******************************************************************************
 * Button Functions
 ******************************************************************************/

void button_init(void) {
    /* Configure P0.1: GPIO, pull-up, hysteresis */
    IOCON_PIO0_1 = (0x01 << 0)    /* FUNC = GPIO */
                 | (0x02 << 3)    /* MODE = Pull-up */
                 | (0x01 << 5);   /* HYS = enabled */

    GPIO0DIR &= ~BUTTON_PIN;  /* Input */
}

uint8_t button_pressed(void) {
    return !(GPIO0DATA & BUTTON_PIN);  /* Active-low */
}

/*******************************************************************************
 * PWM Functions
 ******************************************************************************/

void pwm_init(uint32_t frequency) {
    /* Enable CT32B0 clock */
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Configure P1.6 as CT32B0_MAT0 (PWM output) */
    IOCON_PIO1_6 = 0x02;  /* FUNC = CT32B0_MAT0 */

    /* Reset timer */
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    /* No prescaler - full resolution */
    TMR32B0PR = 0;

    /* Calculate period for desired frequency
     * Period = SystemClock / Frequency
     * For 1 kHz: 72,000,000 / 1000 = 72,000 ticks
     */
    pwm_period = SYSTEM_CLOCK / frequency;
    TMR32B0MR3 = pwm_period - 1;

    /* Initial duty cycle (50%) */
    TMR32B0MR0 = pwm_period / 2;

    /* Reset timer on MR3 match (end of period) */
    TMR32B0MCR = (1 << 10);  /* MR3R */

    /* Enable PWM mode on channel 0 */
    TMR32B0PWMC = (1 << 0);

    /* Start timer */
    TMR32B0TCR = 0x01;
}

void pwm_set_duty(uint8_t percent) {
    if (percent > 100) percent = 100;

    /* Calculate duty cycle match value
     * Higher MR0 = longer HIGH time before going LOW
     * MR0 = (period * percent) / 100
     */
    uint32_t duty = (pwm_period * percent) / 100;
    TMR32B0MR0 = duty;
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    uint8_t last_button = 0;

    leds_init();
    button_init();
    pwm_init(PWM_FREQUENCY);

    /* Set initial brightness */
    pwm_set_duty(brightness_levels[current_level]);
    show_level(current_level);

    while (1) {
        uint8_t current_button = button_pressed();

        /* Edge detection: new button press */
        if (current_button && !last_button) {
            /* Debounce delay */
            delay(50000);

            /* Confirm still pressed */
            if (button_pressed()) {
                /* Advance to next brightness level */
                current_level++;
                if (current_level >= NUM_LEVELS) {
                    current_level = 0;
                }

                /* Update PWM and status LEDs */
                pwm_set_duty(brightness_levels[current_level]);
                show_level(current_level);

                /* Wait for button release */
                while (button_pressed()) {}
                delay(50000);  /* Release debounce */
            }
        }

        last_button = current_button;
    }

    return 0;
}
