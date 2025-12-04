/**
 * Chapter 6: Interrupts and Clocks - Button Interrupt Example
 *
 * Demonstrates GPIO interrupt handling for button presses.
 * Button press toggles LED state using edge-triggered interrupt.
 *
 * Concepts demonstrated:
 *   - GPIO interrupt configuration
 *   - Edge detection (falling edge for button press)
 *   - NVIC enable for GPIO port
 *   - Interrupt flag clearing
 *   - Software debouncing with SysTick
 *
 * Hardware:
 *   - Button on P0.1 (active-low, directly connected)
 *   - LEDs on P3.0-P3.3 (directly connected, active-low)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/* ARM Cortex-M3 intrinsic for Wait For Interrupt */
#define __WFI() __asm volatile ("wfi")

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

/* System Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/* SysTick Registers */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))

/* NVIC */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))

/* GPIO Port 0 (Button) */
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))
#define GPIO0IS         (*((volatile uint32_t *)0x50008004))
#define GPIO0IBE        (*((volatile uint32_t *)0x50008008))
#define GPIO0IEV        (*((volatile uint32_t *)0x5000800C))
#define GPIO0IE         (*((volatile uint32_t *)0x50008010))
#define GPIO0RIS        (*((volatile uint32_t *)0x50008014))
#define GPIO0MIS        (*((volatile uint32_t *)0x50008018))
#define GPIO0IC         (*((volatile uint32_t *)0x5000801C))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR        (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA       (*((volatile uint32_t *)0x50033FFC))

/* IOCON for pins */
#define IOCON_PIO0_1    (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO3_0    (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1    (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2    (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3    (*((volatile uint32_t *)0x400440AC))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define GPIO_CLK        (1 << 6)
#define IOCON_CLK       (1 << 16)

#define BUTTON_PIN      (1 << 1)
#define LED_MASK        0x0F

#define PIO0_IRQn       31

#define SYSTEM_CLOCK    72000000UL
#define DEBOUNCE_MS     50

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

volatile uint32_t systick_count = 0;
volatile uint32_t last_press_time = 0;
volatile uint32_t button_count = 0;

/*******************************************************************************
 * Interrupt Handlers
 ******************************************************************************/

/**
 * SysTick Handler - fires every 1ms
 * Used for debounce timing
 */
void SysTick_Handler(void) {
    systick_count++;
}

/**
 * GPIO Port 0 Handler - fires on button press
 * Uses software debouncing
 */
void PIO0_IRQHandler(void) {
    /* Check if our button caused the interrupt */
    if (GPIO0MIS & BUTTON_PIN) {
        /* Clear the interrupt flag first */
        GPIO0IC = BUTTON_PIN;

        /* Software debounce: ignore if too soon after last press */
        if ((systick_count - last_press_time) > DEBOUNCE_MS) {
            last_press_time = systick_count;
            button_count++;

            /* Toggle LED0 */
            GPIO3DATA ^= (1 << 0);

            /* Update LED pattern based on count */
            /* LED1 shows bit 0 of count */
            if (button_count & 0x01) {
                GPIO3DATA &= ~(1 << 1);  /* LED1 on */
            } else {
                GPIO3DATA |= (1 << 1);   /* LED1 off */
            }

            /* LED2 shows bit 1 of count */
            if (button_count & 0x02) {
                GPIO3DATA &= ~(1 << 2);  /* LED2 on */
            } else {
                GPIO3DATA |= (1 << 2);   /* LED2 off */
            }

            /* LED3 shows bit 2 of count */
            if (button_count & 0x04) {
                GPIO3DATA &= ~(1 << 3);  /* LED3 on */
            } else {
                GPIO3DATA |= (1 << 3);   /* LED3 off */
            }
        }
    }
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/

/**
 * Initialize LEDs on P3.0-P3.3
 */
void led_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK | IOCON_CLK;

    /* Configure pins as GPIO */
    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    /* Set as outputs */
    GPIO3DIR |= LED_MASK;

    /* All LEDs off (high = off for active-low) */
    GPIO3DATA |= LED_MASK;
}

/**
 * Initialize button on P0.1 with interrupt
 */
void button_init(void) {
    /* Configure P0.1 as GPIO input */
    IOCON_PIO0_1 = 0x00;  /* GPIO function, no pull-up/down */
    GPIO0DIR &= ~BUTTON_PIN;  /* Input */

    /* Configure interrupt:
     * GPIO0IS  = 0: Edge sensitive (not level)
     * GPIO0IBE = 0: Single edge (not both edges)
     * GPIO0IEV = 0: Falling edge (button press pulls low)
     */
    GPIO0IS  &= ~BUTTON_PIN;  /* Edge sensitive */
    GPIO0IBE &= ~BUTTON_PIN;  /* Single edge */
    GPIO0IEV &= ~BUTTON_PIN;  /* Falling edge */

    /* Clear any pending interrupt */
    GPIO0IC = BUTTON_PIN;

    /* Enable interrupt for this pin */
    GPIO0IE |= BUTTON_PIN;

    /* Enable GPIO Port 0 interrupt in NVIC (PIO0 is IRQ 31) */
    NVIC_ISER = (1 << PIO0_IRQn);
}

/**
 * Initialize SysTick for 1ms interrupts (for debouncing)
 */
void systick_init(void) {
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;  /* 1ms at 72MHz */
    SYST_CVR = 0;
    SYST_CSR = 0x07;  /* Enable, interrupt, CPU clock */
}

/*******************************************************************************
 * Main Program
 ******************************************************************************/

int main(void) {
    /* Initialize hardware */
    led_init();
    systick_init();
    button_init();

    /* Flash all LEDs briefly to show we're running */
    GPIO3DATA &= ~LED_MASK;  /* All on */
    for (volatile int i = 0; i < 500000; i++);  /* Delay */
    GPIO3DATA |= LED_MASK;   /* All off */

    /* Main loop - button handling is done by interrupt!
     *
     * Each button press:
     * - Toggles LED0
     * - Updates LED1-LED3 as 3-bit binary counter
     *
     * Press count 0: all off
     * Press count 1: LED1 on
     * Press count 2: LED2 on
     * Press count 3: LED1+LED2 on
     * Press count 4: LED3 on
     * ... and so on
     */
    while (1) {
        /* CPU sleeps until interrupt wakes it */
        __WFI();
    }

    return 0;
}
