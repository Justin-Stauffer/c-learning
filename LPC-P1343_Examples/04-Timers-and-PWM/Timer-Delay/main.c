/**
 * Chapter 4: Timers and PWM - Timer-Delay Example
 *
 * Uses CT32B0 timer interrupt to create a precise 1ms system tick.
 * Provides delay_ms() function for timing operations.
 * LEDs blink using timer-based delays instead of software loops.
 *
 * Concepts demonstrated:
 *   - Timer peripheral clock enable
 *   - Timer prescaler configuration
 *   - Match register for periodic interrupt
 *   - NVIC interrupt enable
 *   - Interrupt handler
 *   - Non-blocking delay using tick counter
 *
 * Hardware:
 *   - LEDs on P3.0-P3.3 (active-low)
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

/* IOCON for LED pins */
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* CT32B0 Timer Registers */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))  /* Interrupt Register */
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))  /* Timer Control */
#define TMR32B0TC      (*((volatile uint32_t *)0x40014008))  /* Timer Counter */
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))  /* Prescaler */
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))  /* Match Control */
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))  /* Match Register 0 */

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define SYSTEM_CLOCK   72000000UL  /* 72 MHz assumed */

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define CT32B0_CLK     (1 << 9)

/* CT32B0 IRQ number */
#define CT32B0_IRQn    18

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

volatile uint32_t ms_ticks = 0;

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {  /* MR0 interrupt */
        TMR32B0IR = (1 << 0);    /* Clear interrupt flag */
        ms_ticks++;
    }
}

/*******************************************************************************
 * Timer Functions
 ******************************************************************************/

void timer_init(void) {
    /* Enable CT32B0 clock */
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Reset timer */
    TMR32B0TCR = 0x02;  /* Assert reset */
    TMR32B0TCR = 0x00;  /* Release reset */

    /* Configure for 1ms interrupt:
     * System clock = 72 MHz
     * Prescaler = 71 -> Timer clock = 72MHz / 72 = 1 MHz (1 µs per tick)
     * Match = 999 -> 1000 ticks = 1 ms
     */
    TMR32B0PR = 71;
    TMR32B0MR0 = 999;

    /* Interrupt on MR0, reset counter on MR0 */
    TMR32B0MCR = (1 << 0) | (1 << 1);  /* MR0I | MR0R */

    /* Clear any pending interrupts */
    TMR32B0IR = 0x1F;

    /* Enable CT32B0 interrupt in NVIC */
    NVIC_ISER = (1 << CT32B0_IRQn);

    /* Start timer */
    TMR32B0TCR = 0x01;
}

uint32_t get_ticks(void) {
    return ms_ticks;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms) {
        /* Wait - CPU is free to do other things in a more complex app */
    }
}

/*******************************************************************************
 * LED Functions
 ******************************************************************************/

void leds_init(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= GPIO_CLK;

    /* Configure LED pins as GPIO */
    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    /* Set as outputs */
    GPIO3DIR |= LED_MASK;

    /* All LEDs off (active-low) */
    GPIO3DATA |= LED_MASK;
}

void set_leds(uint8_t pattern) {
    /* Active-low: clear bits to turn ON, set bits to turn OFF */
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;               /* All off */
    current &= ~(pattern & LED_MASK);  /* Turn on selected */
    GPIO3DATA = current;
}

void toggle_led(uint8_t led) {
    GPIO3DATA ^= (1 << led);
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    uint8_t led_state = 0;
    uint32_t last_toggle = 0;

    leds_init();
    timer_init();

    /* Demo 1: Blocking delay - blink all LEDs */
    for (int i = 0; i < 6; i++) {
        set_leds(0x0F);
        delay_ms(200);
        set_leds(0x00);
        delay_ms(200);
    }

    /* Demo 2: Non-blocking delay - running light */
    last_toggle = get_ticks();

    while (1) {
        /* Check if 250ms has elapsed */
        if ((get_ticks() - last_toggle) >= 250) {
            last_toggle = get_ticks();

            /* Advance to next LED */
            set_leds(1 << led_state);
            led_state = (led_state + 1) % 4;
        }

        /* CPU is free here to do other tasks! */
        /* In a real application, you could:
         *   - Check buttons
         *   - Process serial data
         *   - Update displays
         *   - etc.
         */
    }

    return 0;
}
