/**************************************************
 * Serial-Echo UART Example
 * Chapter 5: UART/Serial Communication
 *
 * Demonstrates UART receive polling and character
 * echo. Received characters are sent back.
 *
 * Hardware:
 *   P1.6 - UART RXD (input from terminal)
 *   P1.7 - UART TXD (output to terminal)
 *   P3.0-P3.3 - LEDs (toggle on character received)
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
#define LSR_RDR        (1 << 0)  /* Receiver Data Ready */
#define LSR_THRE       (1 << 5)  /* TX Holding Register Empty */

/*--------------------------------------------------
 * Function Prototypes
 *------------------------------------------------*/
void uart_init(uint32_t baud);
void uart_putchar(char c);
void uart_puts(const char *s);
uint8_t uart_rx_ready(void);
char uart_getchar(void);
void led_init(void);
void led_toggle(uint8_t led);

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

    /* Set UART clock divider to 1 */
    UARTCLKDIV = 1;

    /* Configure UART pins */
    IOCON_PIO1_6 = 0x01;  /* P1.6 = RXD */
    IOCON_PIO1_7 = 0x01;  /* P1.7 = TXD */

    /* Set DLAB=1 to access divisor latches */
    U0LCR = 0x80;

    /* Calculate and set baud rate divisor */
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;

    /* Set 8N1, DLAB=0 */
    U0LCR = 0x03;

    /* Enable and reset FIFOs */
    U0FCR = 0x07;
}

/**
 * Transmit a single character (blocking)
 */
void uart_putchar(char c) {
    while (!(U0LSR & LSR_THRE));
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

/**
 * Check if receive data is available
 * Returns: 1 if data ready, 0 otherwise
 */
uint8_t uart_rx_ready(void) {
    return (U0LSR & LSR_RDR) ? 1 : 0;
}

/**
 * Receive a character (blocking)
 */
char uart_getchar(void) {
    while (!uart_rx_ready());
    return U0RBR;
}

/*--------------------------------------------------
 * LED Functions
 *------------------------------------------------*/

/**
 * Initialize LED GPIO pins
 */
void led_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;
    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;  /* All LEDs off (active-low) */
}

/**
 * Toggle an LED
 */
void led_toggle(uint8_t led) {
    if (led > 3) return;
    GPIO3DATA ^= (1 << led);
}

/*--------------------------------------------------
 * Main Program
 *------------------------------------------------*/

int main(void) {
    uint32_t rx_count = 0;

    /* Initialize peripherals */
    led_init();
    uart_init(115200);

    /* Send startup message */
    uart_puts("\r\n");
    uart_puts("=================================\r\n");
    uart_puts("LPC1343 UART Serial Echo Example\r\n");
    uart_puts("=================================\r\n");
    uart_puts("\r\n");
    uart_puts("Type characters - they will be echoed back.\r\n");
    uart_puts("LED0 toggles on each character received.\r\n");
    uart_puts("\r\n");
    uart_puts("> ");

    /* Main loop - echo received characters */
    while (1) {
        if (uart_rx_ready()) {
            char c = uart_getchar();

            /* Echo the character back */
            uart_putchar(c);

            /* Handle Enter key - show count and new prompt */
            if (c == '\r') {
                uart_puts("\n");

                /* Show character count */
                uart_puts("[Received ");

                /* Simple number to string */
                char buf[12];
                uint32_t n = rx_count;
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
                uart_puts(" chars]\r\n> ");
            }

            /* Toggle LED on each character */
            led_toggle(0);

            /* Count characters (excluding CR) */
            if (c != '\r') {
                rx_count++;
            }

            /* Show receive activity on LED1 for special chars */
            if (c < 32 && c != '\r' && c != '\n') {
                led_toggle(1);  /* Control character indicator */
            }
        }
    }

    return 0;
}
