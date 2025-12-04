/**
 * Chapter 6: Interrupts and Clocks - PLL Setup Example
 *
 * Demonstrates configuring the PLL to run the LPC1343 at 72 MHz
 * from the 12 MHz internal RC oscillator.
 *
 * Concepts demonstrated:
 *   - Clock source selection
 *   - PLL configuration and lock wait
 *   - Main clock switching
 *   - Visual demonstration of clock speed change
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

/* System Control - Clock Configuration */
#define SYSPLLCTRL      (*((volatile uint32_t *)0x40048008))  /* PLL Control */
#define SYSPLLSTAT      (*((volatile uint32_t *)0x4004800C))  /* PLL Status */
#define MAINCLKSEL      (*((volatile uint32_t *)0x40048070))  /* Main Clock Select */
#define MAINCLKUEN      (*((volatile uint32_t *)0x40048074))  /* Main Clock Update */
#define SYSAHBCLKDIV    (*((volatile uint32_t *)0x40048078))  /* AHB Clock Divider */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))  /* AHB Clock Control */
#define PDRUNCFG        (*((volatile uint32_t *)0x40048238))  /* Power-Down Config */

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

/* PLL Configuration for 72 MHz from 12 MHz IRC
 *
 * Formula: F_out = F_in × (MSEL + 1)
 * 72 MHz = 12 MHz × 6
 * MSEL = 5
 *
 * CCO frequency must be 156-320 MHz
 * CCO = F_out × 2 × P (where P = 2^PSEL)
 * With PSEL=1 (P=2): CCO = 72 × 2 × 2 = 288 MHz (valid)
 */
#define PLL_MSEL        5       /* Multiply by 6 */
#define PLL_PSEL        1       /* P = 2 */

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * Simple delay loop (timing depends on current clock speed!)
 */
void delay(volatile uint32_t count) {
    while (count > 0) count--;
}

/*******************************************************************************
 * LED Functions
 ******************************************************************************/

void led_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;

    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All off */
}

void led_set(uint8_t led, uint8_t on) {
    if (led > 3) return;
    if (on) {
        GPIO3DATA &= ~(1 << led);
    } else {
        GPIO3DATA |= (1 << led);
    }
}

void led_all(uint8_t on) {
    if (on) {
        GPIO3DATA &= ~LED_MASK;
    } else {
        GPIO3DATA |= LED_MASK;
    }
}

/*******************************************************************************
 * Clock Configuration Functions
 ******************************************************************************/

/**
 * Blink pattern at current clock speed
 * Same delay count will appear faster at higher clock
 */
void blink_demo(uint8_t led, uint32_t delay_count, uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        led_set(led, 1);
        delay(delay_count);
        led_set(led, 0);
        delay(delay_count);
    }
}

/**
 * Configure PLL for 72 MHz output from 12 MHz IRC
 */
void pll_init_72mhz(void) {
    /* Step 1: Power up the system PLL
     * PDRUNCFG bit 7 = 0 powers up the PLL
     */
    PDRUNCFG &= ~(1 << 7);

    /* Step 2: Configure PLL
     * MSEL = 5 (bits 4:0) -> multiply by 6
     * PSEL = 1 (bits 6:5) -> P = 2, CCO = 288 MHz
     * SYSPLLCTRL = (PSEL << 5) | MSEL = 0x25
     */
    SYSPLLCTRL = (PLL_PSEL << 5) | PLL_MSEL;

    /* Step 3: Wait for PLL to lock
     * SYSPLLSTAT bit 0 = 1 when locked
     */
    while (!(SYSPLLSTAT & 0x01)) {
        /* Waiting for lock... */
    }

    /* Step 4: Set system AHB clock divider to 1 (no division) */
    SYSAHBCLKDIV = 1;

    /* Step 5: Select PLL output as main clock
     * MAINCLKSEL values:
     *   0 = IRC oscillator (12 MHz)
     *   1 = PLL input (IRC in this case)
     *   2 = Watchdog oscillator
     *   3 = PLL output
     */
    MAINCLKSEL = 0x03;

    /* Step 6: Update main clock selection
     * Toggle MAINCLKUEN to apply the selection
     */
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;

    /* Wait for update to complete */
    while (!(MAINCLKUEN & 0x01)) {
        /* Waiting for clock switch... */
    }

    /* Now running at 72 MHz! */
}

/*******************************************************************************
 * Main Program
 ******************************************************************************/

int main(void) {
    /* At this point, system is running at 12 MHz (IRC default) */

    /* Initialize LEDs */
    led_init();

    /* Demonstrate 12 MHz speed:
     * Blink LED0 with a fixed delay count.
     * At 12 MHz this will appear relatively slow.
     */
    led_all(0);
    led_set(0, 1);  /* LED0 on to indicate "before PLL" */
    delay(500000);
    led_set(0, 0);

    /* Blink pattern at 12 MHz */
    blink_demo(0, 100000, 5);  /* LED0: 5 blinks at 12 MHz */

    /* Short pause */
    delay(500000);

    /* ============================================
     * NOW SWITCH TO 72 MHz via PLL
     * ============================================
     */
    pll_init_72mhz();

    /* Demonstrate 72 MHz speed:
     * Same delay count, but now runs 6x faster!
     * The blinking will be visibly faster.
     */
    led_set(1, 1);  /* LED1 on to indicate "after PLL" */
    delay(500000);  /* Same count, but faster! */
    led_set(1, 0);

    /* Blink pattern at 72 MHz - same delay count, 6x faster */
    blink_demo(1, 100000, 5);  /* LED1: 5 blinks at 72 MHz */

    /* Main loop: alternate between LED2 and LED3 to show we're running */
    uint8_t toggle = 0;
    while (1) {
        /* At 72 MHz, need larger delay for visible blink */
        led_set(2, toggle);
        led_set(3, !toggle);
        delay(3000000);  /* ~250ms at 72 MHz */
        toggle = !toggle;
    }

    return 0;
}
