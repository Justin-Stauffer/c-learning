/**************************************************
 * LPC1343 Power Management: Low-Power Blink
 * Chapter 10: Power Management
 *
 * Demonstrates power-efficient LED blinking by
 * sleeping between toggles instead of busy-waiting.
 *
 * Hardware:
 *   LED: P0.7 (onboard, active low)
 *
 * Comparison:
 *   Polling delay: CPU runs continuously at ~10mA
 *   Sleep between toggles: CPU sleeps at ~3mA
 *
 * This example uses SysTick to wake up periodically.
 * The CPU sleeps between SysTick interrupts, reducing
 * average power consumption significantly.
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * Wait For Interrupt Macro
 *------------------------------------------------*/
#define __WFI() __asm volatile ("wfi")

/*--------------------------------------------------
 * System Control Registers
 *------------------------------------------------*/
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/*--------------------------------------------------
 * GPIO Registers
 *------------------------------------------------*/
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))

/*--------------------------------------------------
 * SysTick Registers
 *------------------------------------------------*/
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))

/*--------------------------------------------------
 * System Control Block
 *------------------------------------------------*/
#define SCB_SCR         (*((volatile uint32_t *)0xE000ED10))

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define SYSTEM_CLOCK    12000000    /* 12 MHz (IRC default) */
#define BLINK_INTERVAL  500         /* milliseconds */

/*--------------------------------------------------
 * Global Variables
 *------------------------------------------------*/
volatile uint8_t systick_fired = 0;

/*--------------------------------------------------
 * SysTick Handler
 *
 * Called every BLINK_INTERVAL milliseconds.
 * Just sets a flag - processing done in main loop.
 *------------------------------------------------*/
void SysTick_Handler(void) {
    systick_fired = 1;
}

/*--------------------------------------------------
 * LED Control
 *------------------------------------------------*/
static void led_toggle(void) {
    GPIO0DATA ^= (1 << LED_PIN);
}

/*--------------------------------------------------
 * Initialize SysTick for Periodic Wake-up
 *
 * Configure SysTick to fire every BLINK_INTERVAL ms.
 *------------------------------------------------*/
static void systick_init(void) {
    /* Calculate reload value:
     * SYSTEM_CLOCK / 1000 = ticks per millisecond
     * Multiply by BLINK_INTERVAL for total ticks
     */
    uint32_t ticks = (SYSTEM_CLOCK / 1000) * BLINK_INTERVAL;

    /* Set reload value (24-bit counter, max ~16.7M) */
    SYST_RVR = ticks - 1;

    /* Clear current value */
    SYST_CVR = 0;

    /* Enable SysTick:
     * Bit 0: ENABLE = 1
     * Bit 1: TICKINT = 1 (enable interrupt)
     * Bit 2: CLKSOURCE = 1 (use processor clock)
     */
    SYST_CSR = (1 << 0) | (1 << 1) | (1 << 2);
}

/*--------------------------------------------------
 * Enter Sleep Until SysTick
 *
 * Puts CPU to sleep until next SysTick interrupt.
 * This is much more power-efficient than busy-waiting.
 *------------------------------------------------*/
static void sleep_until_tick(void) {
    /* Clear flag */
    systick_fired = 0;

    /* Clear SLEEPDEEP for normal sleep */
    SCB_SCR &= ~(1 << 2);

    /* Wait for interrupt (sleep) */
    while (!systick_fired) {
        __WFI();
    }
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);

    /* Start with LED off */
    GPIO0DATA |= (1 << LED_PIN);

    /* Initialize SysTick for periodic wake-up */
    systick_init();

    /* Main loop: toggle LED, then sleep until next tick
     *
     * Power efficiency comparison:
     *
     * Polling approach (bad):
     *   while(1) {
     *       toggle();
     *       for(i=0; i<big_number; i++); // wastes power!
     *   }
     *
     * Sleep approach (good):
     *   while(1) {
     *       toggle();
     *       WFI(); // CPU sleeps, saves power
     *   }
     *
     * With sleep: ~3mA average instead of ~10mA
     */
    while (1) {
        /* Toggle LED */
        led_toggle();

        /* Sleep until next SysTick fires */
        sleep_until_tick();
    }

    return 0;
}
