/**
 * Chapter 6: Interrupts and Clocks - Multi-Interrupt Example
 *
 * Demonstrates multiple interrupt sources running simultaneously:
 * - SysTick: System tick counter (1ms)
 * - CT32B0: Toggle LED0 every 250ms
 * - CT32B1: Toggle LED1 every 500ms
 *
 * Concepts demonstrated:
 *   - Multiple interrupt handlers
 *   - NVIC configuration for multiple sources
 *   - Independent timer interrupts
 *   - Interrupt flag clearing
 *
 * Hardware:
 *   - LEDs on P3.0-P3.3 (active-low)
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

/* Timer CT32B0 Registers */
#define TMR32B0IR       (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR      (*((volatile uint32_t *)0x40014004))
#define TMR32B0TC       (*((volatile uint32_t *)0x40014008))
#define TMR32B0PR       (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR      (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0      (*((volatile uint32_t *)0x40014018))

/* Timer CT32B1 Registers */
#define TMR32B1IR       (*((volatile uint32_t *)0x40018000))
#define TMR32B1TCR      (*((volatile uint32_t *)0x40018004))
#define TMR32B1TC       (*((volatile uint32_t *)0x40018008))
#define TMR32B1PR       (*((volatile uint32_t *)0x4001800C))
#define TMR32B1MCR      (*((volatile uint32_t *)0x40018014))
#define TMR32B1MR0      (*((volatile uint32_t *)0x40018018))

/* SysTick Registers */
#define SYST_CSR        (*((volatile uint32_t *)0xE000E010))
#define SYST_RVR        (*((volatile uint32_t *)0xE000E014))
#define SYST_CVR        (*((volatile uint32_t *)0xE000E018))

/* NVIC */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))

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
#define CT32B0_CLK      (1 << 9)
#define CT32B1_CLK      (1 << 10)

#define LED_MASK        0x0F

#define CT32B0_IRQn     18
#define CT32B1_IRQn     19

#define SYSTEM_CLOCK    72000000UL

/*******************************************************************************
 * Global Variables - Interrupt Counters
 ******************************************************************************/

volatile uint32_t systick_count = 0;
volatile uint32_t timer0_count = 0;
volatile uint32_t timer1_count = 0;

/*******************************************************************************
 * Interrupt Handlers
 ******************************************************************************/

/**
 * SysTick Handler - fires every 1ms
 * Used for system timekeeping
 */
void SysTick_Handler(void) {
    systick_count++;

    /* Toggle LED2 every 1000ms (1 second) */
    if ((systick_count % 1000) == 0) {
        GPIO3DATA ^= (1 << 2);
    }
}

/**
 * CT32B0 Handler - fires every 250ms
 * Toggles LED0
 */
void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & 0x01) {
        TMR32B0IR = 0x01;  /* Clear MR0 interrupt flag */
        timer0_count++;
        GPIO3DATA ^= (1 << 0);  /* Toggle LED0 */
    }
}

/**
 * CT32B1 Handler - fires every 500ms
 * Toggles LED1
 */
void CT32B1_IRQHandler(void) {
    if (TMR32B1IR & 0x01) {
        TMR32B1IR = 0x01;  /* Clear MR0 interrupt flag */
        timer1_count++;
        GPIO3DATA ^= (1 << 1);  /* Toggle LED1 */
    }
}

/*******************************************************************************
 * Initialization Functions
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

/**
 * Initialize SysTick for 1ms interrupts
 */
void systick_init(void) {
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;  /* 1ms at 72MHz */
    SYST_CVR = 0;
    SYST_CSR = 0x07;  /* Enable, interrupt, CPU clock */
}

/**
 * Initialize CT32B0 for 250ms interrupts
 */
void timer0_init(void) {
    /* Enable timer clock */
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Reset timer */
    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    /* Prescaler: 72MHz / 72 = 1MHz (1µs per tick) */
    TMR32B0PR = 71;

    /* Match at 250,000 µs = 250ms */
    TMR32B0MR0 = 250000 - 1;

    /* Interrupt on MR0, reset on MR0 */
    TMR32B0MCR = (1 << 0) | (1 << 1);

    /* Clear pending interrupts */
    TMR32B0IR = 0x1F;

    /* Enable in NVIC */
    NVIC_ISER = (1 << CT32B0_IRQn);

    /* Start timer */
    TMR32B0TCR = 0x01;
}

/**
 * Initialize CT32B1 for 500ms interrupts
 */
void timer1_init(void) {
    /* Enable timer clock */
    SYSAHBCLKCTRL |= CT32B1_CLK;

    /* Reset timer */
    TMR32B1TCR = 0x02;
    TMR32B1TCR = 0x00;

    /* Prescaler: 72MHz / 72 = 1MHz (1µs per tick) */
    TMR32B1PR = 71;

    /* Match at 500,000 µs = 500ms */
    TMR32B1MR0 = 500000 - 1;

    /* Interrupt on MR0, reset on MR0 */
    TMR32B1MCR = (1 << 0) | (1 << 1);

    /* Clear pending interrupts */
    TMR32B1IR = 0x1F;

    /* Enable in NVIC */
    NVIC_ISER = (1 << CT32B1_IRQn);

    /* Start timer */
    TMR32B1TCR = 0x01;
}

/*******************************************************************************
 * Main Program
 ******************************************************************************/

int main(void) {
    /* Initialize hardware */
    led_init();

    /* Initialize all interrupt sources */
    systick_init();
    timer0_init();
    timer1_init();

    /* Turn on LED3 to show we're running */
    GPIO3DATA &= ~(1 << 3);

    /* Main loop - the LEDs are controlled entirely by interrupts!
     * LED0: Toggles every 250ms (CT32B0)
     * LED1: Toggles every 500ms (CT32B1)
     * LED2: Toggles every 1000ms (SysTick)
     * LED3: Always on (main loop indicator)
     *
     * Watch the LEDs - they blink at different rates independently!
     */
    while (1) {
        /* CPU can sleep here - interrupts do the work */
        __WFI();  /* Wait For Interrupt */
    }

    return 0;
}
