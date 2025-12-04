/**
 * Chapter 4: Timers and PWM - Breathing LED Example
 *
 * Creates a smooth "breathing" effect on an LED using PWM.
 * Combines timer-based delays with PWM output.
 * Includes gamma correction for perceived linear brightness.
 *
 * Concepts demonstrated:
 *   - PWM for LED brightness control
 *   - Timer interrupt for timing
 *   - Smooth transitions (ramping)
 *   - Gamma correction for human perception
 *   - Combining multiple timer uses
 *
 * Hardware:
 *   - P1.6: PWM output (connect external LED with resistor)
 *   - P3.0-P3.3: Status LEDs (optional visual indicator)
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
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 3 (status LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* CT32B0 Timer Registers (PWM) */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))
#define TMR32B0MR3     (*((volatile uint32_t *)0x40014024))
#define TMR32B0PWMC    (*((volatile uint32_t *)0x40014074))

/* CT32B1 Timer Registers (delay timing) */
#define TMR32B1IR      (*((volatile uint32_t *)0x40018000))
#define TMR32B1TCR     (*((volatile uint32_t *)0x40018004))
#define TMR32B1PR      (*((volatile uint32_t *)0x4001800C))
#define TMR32B1MCR     (*((volatile uint32_t *)0x40018014))
#define TMR32B1MR0     (*((volatile uint32_t *)0x40018018))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define SYSTEM_CLOCK   72000000UL
#define PWM_FREQUENCY  1000

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define CT32B0_CLK     (1 << 9)
#define CT32B1_CLK     (1 << 10)

/* CT32B1 IRQ number */
#define CT32B1_IRQn    19

/*******************************************************************************
 * Gamma Correction Table
 *
 * Human eyes perceive brightness logarithmically, not linearly.
 * This table maps linear 0-100 values to PWM values that appear linear.
 * Formula: output = (input/100)^2.2 * 100
 ******************************************************************************/

const uint8_t gamma_table[101] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      1,   1,   1,   1,   1,   2,   2,   2,   2,   3,
      3,   4,   4,   4,   5,   5,   6,   6,   7,   8,
      8,   9,  10,  10,  11,  12,  13,  14,  15,  16,
     17,  18,  19,  20,  21,  22,  24,  25,  26,  28,
     29,  31,  32,  34,  36,  37,  39,  41,  43,  45,
     47,  49,  51,  53,  55,  58,  60,  62,  65,  67,
     70,  73,  75,  78,  81,  84,  87,  90,  93,  96,
    100, 103, 106, 110, 113, 117, 121, 124, 128, 132,
    136, 140, 144, 148, 152, 156, 161, 165, 170, 174,
    179
};

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

volatile uint32_t ms_ticks = 0;
uint32_t pwm_period;

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void CT32B1_IRQHandler(void) {
    if (TMR32B1IR & (1 << 0)) {
        TMR32B1IR = (1 << 0);
        ms_ticks++;
    }
}

/*******************************************************************************
 * Timer Functions
 ******************************************************************************/

void delay_timer_init(void) {
    /* Use CT32B1 for delay timing */
    SYSAHBCLKCTRL |= CT32B1_CLK;

    TMR32B1TCR = 0x02;
    TMR32B1TCR = 0x00;

    /* 1ms tick: prescaler 71, match 999 */
    TMR32B1PR = 71;
    TMR32B1MR0 = 999;

    TMR32B1MCR = (1 << 0) | (1 << 1);  /* Interrupt + Reset on MR0 */
    TMR32B1IR = 0x1F;

    NVIC_ISER = (1 << CT32B1_IRQn);
    TMR32B1TCR = 0x01;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

/*******************************************************************************
 * PWM Functions
 ******************************************************************************/

void pwm_init(void) {
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Configure P1.6 as CT32B0_MAT0 */
    IOCON_PIO1_6 = 0x02;

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    TMR32B0PR = 0;

    pwm_period = SYSTEM_CLOCK / PWM_FREQUENCY;
    TMR32B0MR3 = pwm_period - 1;
    TMR32B0MR0 = 0;  /* Start at 0% */

    TMR32B0MCR = (1 << 10);  /* Reset on MR3 */
    TMR32B0PWMC = (1 << 0);  /* Enable PWM ch0 */

    TMR32B0TCR = 0x01;
}

void pwm_set_duty(uint8_t percent) {
    if (percent > 100) percent = 100;
    TMR32B0MR0 = (pwm_period * percent) / 100;
}

void pwm_set_duty_gamma(uint8_t linear_percent) {
    /* Apply gamma correction for perceived linear brightness */
    if (linear_percent > 100) linear_percent = 100;
    uint8_t corrected = gamma_table[linear_percent];
    pwm_set_duty(corrected);
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
    GPIO3DATA |= LED_MASK;
}

void show_breathing_phase(uint8_t phase) {
    /* Phase indicator on status LEDs:
     * 0 = inhale start (1 LED)
     * 1 = inhale peak (2 LEDs)
     * 2 = exhale start (2 LEDs)
     * 3 = exhale end (1 LED)
     */
    uint8_t pattern = 0;
    switch (phase) {
        case 0: pattern = 0x01; break;  /* Inhale start */
        case 1: pattern = 0x03; break;  /* Peak */
        case 2: pattern = 0x03; break;  /* Exhale start */
        case 3: pattern = 0x01; break;  /* Rest */
    }
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;
    current &= ~pattern;
    GPIO3DATA = current;
}

/*******************************************************************************
 * Breathing Effect
 ******************************************************************************/

void breathing_effect(void) {
    const uint16_t step_delay = 15;      /* ms per step */
    const uint16_t pause_at_bottom = 300; /* ms pause when LED is off */

    while (1) {
        /* Breathe in (fade up) */
        show_breathing_phase(0);
        for (int duty = 0; duty <= 100; duty++) {
            pwm_set_duty_gamma(duty);
            delay_ms(step_delay);
        }

        /* Brief pause at peak */
        show_breathing_phase(1);
        delay_ms(100);

        /* Breathe out (fade down) */
        show_breathing_phase(2);
        for (int duty = 100; duty >= 0; duty--) {
            pwm_set_duty_gamma(duty);
            delay_ms(step_delay);
        }

        /* Rest at bottom */
        show_breathing_phase(3);
        delay_ms(pause_at_bottom);
    }
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    leds_init();
    delay_timer_init();
    pwm_init();

    /* Run the breathing effect forever */
    breathing_effect();

    return 0;
}
