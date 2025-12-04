/**************************************************
 * Hello-World UART Example
 * Chapter 5: UART/Serial Communication
 *
 * Demonstrates basic UART initialization and
 * polled transmit for sending messages.
 *
 * Hardware:
 *   P1.6 - UART RXD (unused in this example)
 *   P1.7 - UART TXD (output to terminal)
 *   P3.0-P3.3 - LEDs (status indicator)
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * LPC1343 Register Definitions
 *------------------------------------------------*/

/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))
#define UARTCLKDIV     (*((volatile uint32_t *)0x40048098))

/* IOCON for UART pins */
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))  /* RXD */
#define IOCON_PIO1_7   (*((volatile uint32_t *)0x400440A8))  /* TXD */

/* UART Registers */
#define U0RBR          (*((volatile uint32_t *)0x40008000))  /* Receive Buffer (DLAB=0, read) */
#define U0THR          (*((volatile uint32_t *)0x40008000))  /* Transmit Holding (DLAB=0, write) */
#define U0DLL          (*((volatile uint32_t *)0x40008000))  /* Divisor Latch LSB (DLAB=1) */
#define U0DLM          (*((volatile uint32_t *)0x40008004))  /* Divisor Latch MSB (DLAB=1) */
#define U0IER          (*((volatile uint32_t *)0x40008004))  /* Interrupt Enable (DLAB=0) */
#define U0FCR          (*((volatile uint32_t *)0x40008008))  /* FIFO Control (write) */
#define U0LCR          (*((volatile uint32_t *)0x4000800C))  /* Line Control */
#define U0LSR          (*((volatile uint32_t *)0x40008014))  /* Line Status */

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define UART_CLK       (1 << 12)

/* LED configuration (active-low) */
#define LED_MASK       0x0F

/* System clock frequency */
#define SYSTEM_CLOCK   72000000UL

/* Line Status Register bits */
#define LSR_THRE       (1 << 5)  /* TX Holding Register Empty */

/*--------------------------------------------------
 * Function Prototypes
 *------------------------------------------------*/
void uart_init(uint32_t baud);
void uart_putchar(char c);
void uart_puts(const char *s);
void led_init(void);
void led_set(uint8_t led, uint8_t on);
void delay(volatile uint32_t count);

/*--------------------------------------------------
 * UART Functions
 *------------------------------------------------*/

/**
 * Initialize UART for specified baud rate
 * Configuration: 8 data bits, no parity, 1 stop bit (8N1)
 */
void uart_init(uint32_t baud) {
    /* Enable UART clock */
    SYSAHBCLKCTRL |= UART_CLK;

    /* Set UART clock divider to 1 (full speed) */
    UARTCLKDIV = 1;

    /* Configure UART pins */
    IOCON_PIO1_6 = 0x01;  /* P1.6 = RXD function */
    IOCON_PIO1_7 = 0x01;  /* P1.7 = TXD function */

    /* Set DLAB=1 to access divisor latches */
    U0LCR = 0x80;

    /* Calculate and set baud rate divisor
     * Divisor = PCLK / (16 * baud_rate)
     * At 72MHz: 115200 baud -> divisor = 39
     */
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud);
    U0DLL = divisor & 0xFF;         /* LSB */
    U0DLM = (divisor >> 8) & 0xFF;  /* MSB */

    /* Set DLAB=0, configure 8N1 format
     * Bits [1:0] = 11 -> 8 data bits
     * Bit 2 = 0 -> 1 stop bit
     * Bit 3 = 0 -> no parity
     * Bit 7 = 0 -> DLAB disabled
     */
    U0LCR = 0x03;

    /* Enable and reset FIFOs
     * Bit 0 = 1 -> Enable FIFOs
     * Bit 1 = 1 -> Reset RX FIFO
     * Bit 2 = 1 -> Reset TX FIFO
     */
    U0FCR = 0x07;
}

/**
 * Transmit a single character (blocking)
 */
void uart_putchar(char c) {
    /* Wait until TX holding register is empty */
    while (!(U0LSR & LSR_THRE));

    /* Write character to transmit register */
    U0THR = c;
}

/**
 * Transmit a null-terminated string
 */
void uart_puts(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

/*--------------------------------------------------
 * LED Functions
 *------------------------------------------------*/

/**
 * Initialize LED GPIO pins
 */
void led_init(void) {
    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= GPIO_CLK;

    /* Set LED pins as outputs */
    GPIO3DIR |= LED_MASK;

    /* Turn off all LEDs (active-low, so set high) */
    GPIO3DATA |= LED_MASK;
}

/**
 * Control an individual LED
 * led: 0-3
 * on: 0=off, 1=on
 */
void led_set(uint8_t led, uint8_t on) {
    if (led > 3) return;

    if (on) {
        GPIO3DATA &= ~(1 << led);  /* Active-low: clear to turn on */
    } else {
        GPIO3DATA |= (1 << led);   /* Active-low: set to turn off */
    }
}

/**
 * Simple delay loop
 */
void delay(volatile uint32_t count) {
    while (count--);
}

/*--------------------------------------------------
 * Main Program
 *------------------------------------------------*/

int main(void) {
    uint32_t count = 0;

    /* Initialize peripherals */
    led_init();
    uart_init(115200);

    /* Send startup message */
    uart_puts("\r\n");
    uart_puts("================================\r\n");
    uart_puts("LPC1343 UART Hello World Example\r\n");
    uart_puts("================================\r\n");
    uart_puts("\r\n");
    uart_puts("UART configured: 115200 baud, 8N1\r\n");
    uart_puts("System clock: 72 MHz\r\n");
    uart_puts("\r\n");

    /* Main loop - blink LED and send periodic messages */
    while (1) {
        /* Toggle LED0 */
        led_set(0, 1);
        delay(2000000);

        /* Send heartbeat message */
        uart_puts("Hello, World! Count: ");

        /* Simple number to string conversion */
        char buf[12];
        uint32_t n = count;
        int i = 0;

        if (n == 0) {
            buf[i++] = '0';
        } else {
            char temp[12];
            int j = 0;
            while (n > 0) {
                temp[j++] = '0' + (n % 10);
                n /= 10;
            }
            while (j > 0) {
                buf[i++] = temp[--j];
            }
        }
        buf[i] = '\0';

        uart_puts(buf);
        uart_puts("\r\n");

        count++;

        /* Toggle LED0 off */
        led_set(0, 0);
        delay(2000000);
    }

    return 0;
}
