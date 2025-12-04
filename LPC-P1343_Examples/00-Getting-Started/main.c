/**
 * Getting Started - Blink an LED on LPC1343
 *
 * This is the simplest possible LED blinking program.
 * It demonstrates direct register access without any libraries.
 *
 * Hardware: LPC-P1343 board with LED on P3.0
 * Toolchain: GCC ARM (arm-none-eabi-gcc)
 *
 * Build: make
 * Flash: make flash (requires OpenOCD + ST-Link)
 */

/*******************************************************************************
 * Register Definitions
 *
 * These are memory-mapped hardware registers. Writing to these addresses
 * directly controls the hardware.
 ******************************************************************************/

/* System Control Registers */
#define SYSAHBCLKCTRL  (*((volatile unsigned int *)0x40048080))

/* GPIO Port 3 Registers */
#define GPIO3DIR       (*((volatile unsigned int *)0x50038000))  /* Direction */
#define GPIO3DATA      (*((volatile unsigned int *)0x50033FFC))  /* Data */

/*******************************************************************************
 * Simple Delay Function
 *
 * This is a crude delay using a busy loop. Not accurate, but works for blinking.
 * For precise timing, use hardware timers (see Chapter 4).
 ******************************************************************************/
void delay(volatile int count) {
    while (count > 0) {
        count--;
    }
}

/*******************************************************************************
 * Main Function
 *
 * Embedded programs run forever - there's no operating system to return to!
 ******************************************************************************/
int main(void) {
    /*
     * Step 1: Enable GPIO clock
     *
     * The LPC1343 has clock gating to save power. We must enable the clock
     * to GPIO before we can use it. Bit 6 controls the GPIO clock.
     */
    SYSAHBCLKCTRL |= (1 << 6);  /* Enable GPIO clock */

    /*
     * Step 2: Configure P3.0 as output
     *
     * GPIO3DIR controls the direction of Port 3 pins:
     *   0 = Input (default after reset)
     *   1 = Output
     *
     * We set bit 0 to make P3.0 an output.
     */
    GPIO3DIR |= (1 << 0);  /* P3.0 = output */

    /*
     * Step 3: Blink forever!
     *
     * The LED on the LPC-P1343 board is "active-low", meaning:
     *   - Write 0 (LOW)  = LED turns ON  (current flows)
     *   - Write 1 (HIGH) = LED turns OFF (no current)
     *
     * GPIO3DATA controls the output state of Port 3 pins.
     */
    while (1) {
        /* LED ON: Clear bit 0 (set P3.0 LOW) */
        GPIO3DATA &= ~(1 << 0);
        delay(500000);

        /* LED OFF: Set bit 0 (set P3.0 HIGH) */
        GPIO3DATA |= (1 << 0);
        delay(500000);
    }

    /* This line is never reached */
    return 0;
}
