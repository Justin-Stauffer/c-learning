/**
 * Chapter 6: Interrupts and Clocks - SysTick Blink Example
 *
 * Demonstrates the ARM Cortex-M3 SysTick timer for precise
 * millisecond timing without using peripheral timers.
 *
 * Concepts demonstrated:
 *   - SysTick timer configuration
 *   - SysTick interrupt handler
 *   - Millisecond delay function
 *   - System tick counter
 *
 * Hardware:
 *   - LEDs on P3.0-P3.3 (active-low)
 *
 * Note: This example assumes 72 MHz system clock (PLL configured)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

/* System Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/* SysTick Registers (ARM Cortex-M3 core) */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))  /* Control and Status */
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))  /* Reload Value */
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))  /* Current Value */

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR        (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA       (*((volatile uint32_t *)0x50033FFC))

/* IOCON for LED pins */
#define IOCON_PIO3_0    (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1    (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2    (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3    (*((volatile uint32_t *)0x400440AC))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define GPIO_CLK        (1 << 6)
#define LED_MASK        0x0F

/* System clock frequency (after PLL setup) */
#define SYSTEM_CLOCK    72000000UL

/* SysTick Control and Status Register bits */
#define SYST_CSR_ENABLE     (1 << 0)    /* Counter enable */
#define SYST_CSR_TICKINT    (1 << 1)    /* Interrupt enable */
#define SYST_CSR_CLKSOURCE  (1 << 2)    /* Clock source: 1=CPU clock */
#define SYST_CSR_COUNTFLAG  (1 << 16)   /* Count reached zero */

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

volatile uint32_t ms_ticks = 0;

/*******************************************************************************
 * SysTick Interrupt Handler
 ******************************************************************************/

/**
 * Called every 1ms by the SysTick timer
 */
void SysTick_Handler(void) {
    ms_ticks++;
}

/*******************************************************************************
 * SysTick Functions
 ******************************************************************************/

/**
 * Initialize SysTick for 1ms interrupts
 */
void systick_init(void) {
    /* Calculate reload value for 1ms at 72MHz
     * Reload = (72,000,000 / 1000) - 1 = 71,999
     * SysTick counts down from reload to 0, then reloads
     */
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;

    /* Clear current value */
    SYST_CVR = 0;

    /* Enable SysTick with CPU clock and interrupt
     * Bit 0: Enable counter
     * Bit 1: Enable interrupt
     * Bit 2: Use CPU clock (not external reference)
     */
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

/**
 * Get current tick count
 */
uint32_t get_ticks(void) {
    return ms_ticks;
}

/**
 * Blocking delay for specified milliseconds
 */
void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms) {
        /* Wait - could use __WFI() for low power */
    }
}

/**
 * Check if timeout has elapsed (non-blocking)
 */
uint8_t timeout_elapsed(uint32_t start_time, uint32_t timeout_ms) {
    return (ms_ticks - start_time) >= timeout_ms;
}

/*******************************************************************************
 * LED Functions
 ******************************************************************************/

void led_init(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= GPIO_CLK;

    /* Configure pins as GPIO */
    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    /* Set as outputs */
    GPIO3DIR |= LED_MASK;

    /* All LEDs off (active-low) */
    GPIO3DATA |= LED_MASK;
}

void led_set(uint8_t led, uint8_t on) {
    if (led > 3) return;
    if (on) {
        GPIO3DATA &= ~(1 << led);  /* Active-low: clear to turn on */
    } else {
        GPIO3DATA |= (1 << led);   /* Active-low: set to turn off */
    }
}

void led_toggle(uint8_t led) {
    if (led > 3) return;
    GPIO3DATA ^= (1 << led);
}

void led_all(uint8_t on) {
    if (on) {
        GPIO3DATA &= ~LED_MASK;
    } else {
        GPIO3DATA |= LED_MASK;
    }
}

/*******************************************************************************
 * Main Program
 ******************************************************************************/

int main(void) {
    /* Initialize hardware */
    led_init();
    systick_init();

    /* Quick startup flash to show we're running */
    led_all(1);
    delay_ms(200);
    led_all(0);
    delay_ms(200);

    /* Main loop - demonstrate various timing patterns */
    while (1) {
        /* Pattern 1: Simple blink LED0 at 1Hz (500ms on, 500ms off) */
        led_set(0, 1);
        delay_ms(500);
        led_set(0, 0);
        delay_ms(500);

        /* Pattern 2: Fast blink LED1 (100ms) */
        for (int i = 0; i < 5; i++) {
            led_set(1, 1);
            delay_ms(100);
            led_set(1, 0);
            delay_ms(100);
        }

        /* Pattern 3: Sequential light-up */
        for (int i = 0; i < 4; i++) {
            led_set(i, 1);
            delay_ms(150);
        }
        delay_ms(300);
        for (int i = 3; i >= 0; i--) {
            led_set(i, 0);
            delay_ms(150);
        }
        delay_ms(300);

        /* Pattern 4: All blink together */
        for (int i = 0; i < 3; i++) {
            led_all(1);
            delay_ms(200);
            led_all(0);
            delay_ms(200);
        }

        delay_ms(500);  /* Pause before repeating */
    }

    return 0;
}
