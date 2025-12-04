/**************************************************
 * LPC1343 Power Management: Sleep with Wakeup
 * Chapter 10: Power Management
 *
 * Demonstrates entering Sleep mode and waking up
 * via button interrupt.
 *
 * Hardware:
 *   Button: P0.1 (onboard BUT1, active low)
 *   LED:    P0.7 (onboard, active low)
 *
 * Sleep Mode Overview:
 *   - CPU clock stopped (WFI instruction)
 *   - Peripherals continue running
 *   - Any enabled interrupt wakes CPU
 *   - Power reduction: ~10mA → ~3-5mA
 *
 * Behavior:
 *   1. LED blinks 3 times (startup indicator)
 *   2. MCU enters sleep mode
 *   3. Button press triggers GPIO interrupt
 *   4. MCU wakes, LED blinks once
 *   5. Returns to sleep
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
 * IOCON Registers
 *------------------------------------------------*/
#define IOCON_PIO0_1    (*((volatile uint32_t *)0x40044010))

/*--------------------------------------------------
 * GPIO Registers
 *------------------------------------------------*/
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))
#define GPIO0IS         (*((volatile uint32_t *)0x50008004))
#define GPIO0IBE        (*((volatile uint32_t *)0x50008008))
#define GPIO0IEV        (*((volatile uint32_t *)0x5000800C))
#define GPIO0IE         (*((volatile uint32_t *)0x50008010))
#define GPIO0RIS        (*((volatile uint32_t *)0x50008014))
#define GPIO0MIS        (*((volatile uint32_t *)0x50008018))
#define GPIO0IC         (*((volatile uint32_t *)0x5000801C))

/*--------------------------------------------------
 * NVIC Registers
 *------------------------------------------------*/
#define ISER0           (*((volatile uint32_t *)0xE000E100))
#define ICER0           (*((volatile uint32_t *)0xE000E180))
#define ICPR0           (*((volatile uint32_t *)0xE000E280))

/*--------------------------------------------------
 * System Control Block (for Sleep)
 *------------------------------------------------*/
#define SCB_SCR         (*((volatile uint32_t *)0xE000ED10))

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define BUTTON_PIN      1
#define PIO0_IRQn       31      /* GPIO Port 0 interrupt */

/*--------------------------------------------------
 * Global Variables
 *------------------------------------------------*/
volatile uint8_t button_pressed = 0;

/*--------------------------------------------------
 * Simple Delay Function
 *------------------------------------------------*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

/*--------------------------------------------------
 * LED Control
 *------------------------------------------------*/
static void led_on(void) {
    GPIO0DATA &= ~(1 << LED_PIN);
}

static void led_off(void) {
    GPIO0DATA |= (1 << LED_PIN);
}

static void led_blink(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        led_on();
        delay(200000);
        led_off();
        delay(200000);
    }
}

/*--------------------------------------------------
 * GPIO Interrupt Handler (PIO0)
 *
 * This handler is called when the button is pressed.
 * It clears the interrupt and sets a flag.
 *------------------------------------------------*/
void PIO0_IRQHandler(void) {
    /* Check if P0.1 triggered the interrupt */
    if (GPIO0MIS & (1 << BUTTON_PIN)) {
        /* Clear the interrupt */
        GPIO0IC = (1 << BUTTON_PIN);

        /* Set flag for main loop */
        button_pressed = 1;
    }
}

/*--------------------------------------------------
 * Configure Button Interrupt
 *
 * Configure P0.1 to generate interrupt on falling
 * edge (button press).
 *------------------------------------------------*/
static void button_interrupt_init(void) {
    /* Configure P0.1 as GPIO with pull-up */
    IOCON_PIO0_1 = 0x10;        /* GPIO, pull-up enabled */

    /* Configure as input (clear direction bit) */
    GPIO0DIR &= ~(1 << BUTTON_PIN);

    /* Configure interrupt:
     * IS  = 0: Edge sensitive
     * IBE = 0: Single edge (controlled by IEV)
     * IEV = 0: Falling edge (button press)
     */
    GPIO0IS &= ~(1 << BUTTON_PIN);   /* Edge sensitive */
    GPIO0IBE &= ~(1 << BUTTON_PIN);  /* Single edge */
    GPIO0IEV &= ~(1 << BUTTON_PIN);  /* Falling edge */

    /* Clear any pending interrupt */
    GPIO0IC = (1 << BUTTON_PIN);

    /* Enable interrupt for P0.1 */
    GPIO0IE |= (1 << BUTTON_PIN);

    /* Clear pending in NVIC */
    ICPR0 = (1 << PIO0_IRQn);

    /* Enable GPIO Port 0 interrupt in NVIC */
    ISER0 = (1 << PIO0_IRQn);
}

/*--------------------------------------------------
 * Enter Sleep Mode
 *
 * Sleep mode stops the CPU clock but keeps
 * peripherals running. Any enabled interrupt
 * will wake the CPU.
 *------------------------------------------------*/
static void enter_sleep(void) {
    /* Clear SLEEPDEEP bit for normal Sleep mode
     * (not Deep Sleep) */
    SCB_SCR &= ~(1 << 2);

    /* Wait For Interrupt - CPU sleeps until interrupt */
    __WFI();

    /* Execution continues here after wake-up */
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);
    led_off();

    /* Initialize button interrupt */
    button_interrupt_init();

    /* Startup indicator: 3 blinks */
    led_blink(3);
    delay(500000);

    /* Main loop: sleep until button press */
    while (1) {
        /* Clear flag before sleeping */
        button_pressed = 0;

        /* Enter sleep mode */
        enter_sleep();

        /* Woke up from interrupt */
        if (button_pressed) {
            /* Blink LED to indicate wake-up */
            led_blink(1);

            /* Small debounce delay */
            delay(200000);
        }
    }

    return 0;
}
