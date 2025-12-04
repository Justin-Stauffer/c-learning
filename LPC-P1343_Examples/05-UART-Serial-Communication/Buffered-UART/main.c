/**************************************************
 * Buffered-UART Example
 * Chapter 5: UART/Serial Communication
 *
 * Demonstrates interrupt-driven UART receive with
 * ring buffer for non-blocking operation.
 *
 * Hardware:
 *   P1.6 - UART RXD
 *   P1.7 - UART TXD
 *   P3.0-P3.3 - LEDs (buffer status indicators)
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * LPC1343 Register Definitions
 *------------------------------------------------*/

/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))
#define UARTCLKDIV     (*((volatile uint32_t *)0x40048098))

/* IOCON for UART pins */
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))
#define IOCON_PIO1_7   (*((volatile uint32_t *)0x400440A8))

/* UART Registers */
#define U0RBR          (*((volatile uint32_t *)0x40008000))
#define U0THR          (*((volatile uint32_t *)0x40008000))
#define U0DLL          (*((volatile uint32_t *)0x40008000))
#define U0DLM          (*((volatile uint32_t *)0x40008004))
#define U0IER          (*((volatile uint32_t *)0x40008004))
#define U0IIR          (*((volatile uint32_t *)0x40008008))
#define U0FCR          (*((volatile uint32_t *)0x40008008))
#define U0LCR          (*((volatile uint32_t *)0x4000800C))
#define U0LSR          (*((volatile uint32_t *)0x40008014))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define UART_CLK       (1 << 12)

/* IRQ Numbers */
#define UART_IRQn      21

/* LED configuration */
#define LED_MASK       0x0F

/* System clock */
#define SYSTEM_CLOCK   72000000UL

/* UART status bits */
#define LSR_RDR        (1 << 0)
#define LSR_THRE       (1 << 5)

/* Interrupt Enable Register bits */
#define IER_RBR        (1 << 0)  /* RX Data Available Interrupt */

/* Interrupt Identification Register */
#define IIR_PEND       (1 << 0)  /* Interrupt pending (0 = pending) */
#define IIR_ID_MASK    0x0E      /* Interrupt ID mask */
#define IIR_RDA        0x04      /* Receive Data Available */
#define IIR_CTI        0x0C      /* Character Timeout Indicator */

/* Ring buffer configuration - must be power of 2 */
#define RX_BUF_SIZE    64

/*--------------------------------------------------
 * Ring Buffer
 *------------------------------------------------*/

volatile uint8_t rx_buffer[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;  /* Write index (ISR) */
volatile uint16_t rx_tail = 0;  /* Read index (main) */
volatile uint32_t rx_overrun = 0;  /* Overrun counter */
volatile uint32_t rx_total = 0;    /* Total chars received */

/*--------------------------------------------------
 * Function Prototypes
 *------------------------------------------------*/
void uart_init(uint32_t baud);
void uart_putchar(char c);
void uart_puts(const char *s);
uint16_t uart_available(void);
int16_t uart_read(void);
void uart_flush(void);
void led_init(void);
void led_set(uint8_t led, uint8_t on);
void delay(volatile uint32_t count);
void print_number(uint32_t n);
void print_hex(uint32_t n);

/*--------------------------------------------------
 * UART Interrupt Handler
 *------------------------------------------------*/

void UART0_IRQHandler(void) {
    uint32_t iir = U0IIR;

    /* Check interrupt pending bit (active low) */
    if (iir & IIR_PEND) {
        return;  /* No interrupt pending */
    }

    uint32_t int_id = iir & IIR_ID_MASK;

    /* Handle RX Data Available or Character Timeout */
    if (int_id == IIR_RDA || int_id == IIR_CTI) {
        /* Read all available characters from FIFO */
        while (U0LSR & LSR_RDR) {
            uint8_t c = U0RBR;

            /* Calculate next head position */
            uint16_t next_head = (rx_head + 1) & (RX_BUF_SIZE - 1);

            if (next_head != rx_tail) {
                /* Buffer not full - store character */
                rx_buffer[rx_head] = c;
                rx_head = next_head;
            } else {
                /* Buffer full - count overrun */
                rx_overrun++;
            }

            rx_total++;
        }
    }
}

/*--------------------------------------------------
 * UART Functions
 *------------------------------------------------*/

void uart_init(uint32_t baud) {
    /* Enable UART clock */
    SYSAHBCLKCTRL |= UART_CLK;
    UARTCLKDIV = 1;

    /* Configure pins */
    IOCON_PIO1_6 = 0x01;  /* RXD */
    IOCON_PIO1_7 = 0x01;  /* TXD */

    /* Set baud rate */
    U0LCR = 0x80;  /* DLAB=1 */
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;
    U0LCR = 0x03;  /* 8N1, DLAB=0 */

    /* Enable FIFO with RX trigger level = 1 char */
    U0FCR = 0x01;

    /* Enable RX interrupt */
    U0IER = IER_RBR;

    /* Enable UART interrupt in NVIC */
    NVIC_ISER = (1 << UART_IRQn);
}

void uart_putchar(char c) {
    while (!(U0LSR & LSR_THRE));
    U0THR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}

/**
 * Get number of characters available in buffer
 */
uint16_t uart_available(void) {
    return (rx_head - rx_tail) & (RX_BUF_SIZE - 1);
}

/**
 * Read a character from buffer (non-blocking)
 * Returns: character (0-255) or -1 if buffer empty
 */
int16_t uart_read(void) {
    if (rx_head == rx_tail) {
        return -1;  /* Buffer empty */
    }

    uint8_t c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
    return c;
}

/**
 * Discard all data in receive buffer
 */
void uart_flush(void) {
    rx_tail = rx_head;
}

/*--------------------------------------------------
 * LED Functions
 *------------------------------------------------*/

void led_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;
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

/*--------------------------------------------------
 * Utility Functions
 *------------------------------------------------*/

void delay(volatile uint32_t count) {
    while (count--);
}

void print_number(uint32_t n) {
    char buf[12];
    int i = 0;

    if (n == 0) {
        uart_putchar('0');
        return;
    }

    char temp[12];
    int j = 0;
    while (n > 0) {
        temp[j++] = '0' + (n % 10);
        n /= 10;
    }
    while (j > 0) {
        buf[i++] = temp[--j];
    }
    buf[i] = '\0';
    uart_puts(buf);
}

void print_hex(uint32_t n) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 7; i >= 0; i--) {
        uart_putchar(hex[(n >> (i * 4)) & 0xF]);
    }
}

/*--------------------------------------------------
 * Buffer Status Display
 *------------------------------------------------*/

void update_buffer_leds(void) {
    uint16_t used = uart_available();
    uint16_t percent = (used * 100) / (RX_BUF_SIZE - 1);

    /* LED indicators for buffer fill level:
     * LED0: >0%  (any data)
     * LED1: >25%
     * LED2: >50%
     * LED3: >75% (nearly full warning)
     */
    led_set(0, used > 0);
    led_set(1, percent >= 25);
    led_set(2, percent >= 50);
    led_set(3, percent >= 75);
}

void print_status(void) {
    uart_puts("\r\n=== Buffer Status ===\r\n");

    uart_puts("Buffer size: ");
    print_number(RX_BUF_SIZE);
    uart_puts(" bytes\r\n");

    uart_puts("Data in buffer: ");
    print_number(uart_available());
    uart_puts(" bytes\r\n");

    uart_puts("Total received: ");
    print_number(rx_total);
    uart_puts(" chars\r\n");

    uart_puts("Overrun count: ");
    print_number(rx_overrun);
    uart_puts("\r\n");

    uart_puts("Head index: ");
    print_number(rx_head);
    uart_puts(", Tail index: ");
    print_number(rx_tail);
    uart_puts("\r\n\r\n");
}

/*--------------------------------------------------
 * Main Program
 *------------------------------------------------*/

int main(void) {
    uint32_t last_report = 0;
    uint32_t loop_count = 0;

    /* Initialize peripherals */
    led_init();
    uart_init(115200);

    /* Welcome message */
    uart_puts("\r\n");
    uart_puts("======================================\r\n");
    uart_puts("LPC1343 Interrupt-Driven UART Example\r\n");
    uart_puts("======================================\r\n");
    uart_puts("\r\n");
    uart_puts("Features:\r\n");
    uart_puts("  - Interrupt-driven RX with ring buffer\r\n");
    uart_puts("  - Non-blocking read API\r\n");
    uart_puts("  - Buffer fill level on LEDs\r\n");
    uart_puts("\r\n");
    uart_puts("Commands:\r\n");
    uart_puts("  Type any text - buffered and echoed\r\n");
    uart_puts("  's' - Show buffer status\r\n");
    uart_puts("  'f' - Flush receive buffer\r\n");
    uart_puts("  't' - Test: send burst of data\r\n");
    uart_puts("\r\n");
    uart_puts("LEDs show buffer fill level:\r\n");
    uart_puts("  LED0=data, LED1=25%+, LED2=50%+, LED3=75%+\r\n");
    uart_puts("\r\n> ");

    /* Main loop */
    while (1) {
        /* Process any available characters */
        int16_t c = uart_read();
        if (c >= 0) {
            /* Handle special commands */
            if (c == 's' || c == 'S') {
                print_status();
                uart_puts("> ");
            }
            else if (c == 'f' || c == 'F') {
                uart_flush();
                uart_puts("\r\n[Buffer flushed]\r\n> ");
            }
            else if (c == 't' || c == 'T') {
                uart_puts("\r\n[Sending test pattern - type fast to fill buffer!]\r\n");
                for (int i = 0; i < 10; i++) {
                    uart_puts("Test line ");
                    print_number(i);
                    uart_puts(": ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n");
                    delay(100000);  /* Small delay to allow typing */
                }
                uart_puts("[Test complete]\r\n> ");
            }
            else if (c == '\r') {
                uart_puts("\r\n> ");
            }
            else {
                /* Echo character */
                uart_putchar(c);
            }
        }

        /* Update LED indicators periodically */
        loop_count++;
        if (loop_count - last_report >= 100000) {
            update_buffer_leds();
            last_report = loop_count;
        }

        /* Main loop can do other work here while
         * interrupts handle incoming data */
    }

    return 0;
}
