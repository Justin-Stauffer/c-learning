/**************************************************
 * Command-Line UART Example
 * Chapter 5: UART/Serial Communication
 *
 * Demonstrates line input, string parsing, and
 * command dispatch for a simple CLI interface.
 *
 * Hardware:
 *   P1.6 - UART RXD
 *   P1.7 - UART TXD
 *   P3.0-P3.3 - LEDs (controlled by commands)
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
#define U0FCR          (*((volatile uint32_t *)0x40008008))
#define U0LCR          (*((volatile uint32_t *)0x4000800C))
#define U0LSR          (*((volatile uint32_t *)0x40008014))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define UART_CLK       (1 << 12)

/* LED configuration */
#define LED_MASK       0x0F

/* System clock */
#define SYSTEM_CLOCK   72000000UL

/* Line Status Register bits */
#define LSR_RDR        (1 << 0)
#define LSR_THRE       (1 << 5)

/* Command buffer size */
#define CMD_BUF_SIZE   64

/*--------------------------------------------------
 * Function Prototypes
 *------------------------------------------------*/
void uart_init(uint32_t baud);
void uart_putchar(char c);
void uart_puts(const char *s);
uint8_t uart_rx_ready(void);
char uart_getchar(void);
uint32_t uart_getline(char *buf, uint32_t max);
void led_init(void);
void led_set(uint8_t led, uint8_t on);
void led_all(uint8_t on);
int str_equal(const char *s1, const char *s2);
int str_startswith(const char *str, const char *prefix);
void process_command(char *cmd);
void delay(volatile uint32_t count);

/*--------------------------------------------------
 * UART Functions
 *------------------------------------------------*/

void uart_init(uint32_t baud) {
    SYSAHBCLKCTRL |= UART_CLK;
    UARTCLKDIV = 1;
    IOCON_PIO1_6 = 0x01;
    IOCON_PIO1_7 = 0x01;

    U0LCR = 0x80;
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;
    U0LCR = 0x03;
    U0FCR = 0x07;
}

void uart_putchar(char c) {
    while (!(U0LSR & LSR_THRE));
    U0THR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}

uint8_t uart_rx_ready(void) {
    return (U0LSR & LSR_RDR) ? 1 : 0;
}

char uart_getchar(void) {
    while (!uart_rx_ready());
    return U0RBR;
}

/**
 * Read a line of input with echo and backspace support
 * Returns: number of characters in buffer (excluding null)
 */
uint32_t uart_getline(char *buf, uint32_t max) {
    uint32_t i = 0;

    while (i < max - 1) {
        char c = uart_getchar();

        /* Handle Enter key */
        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            break;
        }

        /* Handle Backspace */
        if (c == '\b' || c == 0x7F) {  /* BS or DEL */
            if (i > 0) {
                i--;
                uart_puts("\b \b");  /* Erase character on terminal */
            }
            continue;
        }

        /* Handle Escape (cancel line) */
        if (c == 0x1B) {
            uart_puts("\r\n[Cancelled]\r\n");
            buf[0] = '\0';
            return 0;
        }

        /* Ignore other control characters */
        if (c < 32) {
            continue;
        }

        /* Echo and store printable characters */
        uart_putchar(c);
        buf[i++] = c;
    }

    buf[i] = '\0';
    return i;
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
        GPIO3DATA &= ~(1 << led);  /* Active-low */
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

/*--------------------------------------------------
 * String Functions
 *------------------------------------------------*/

/**
 * Compare two strings for equality
 */
int str_equal(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

/**
 * Check if string starts with prefix
 */
int str_startswith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

/*--------------------------------------------------
 * Delay Function
 *------------------------------------------------*/

void delay(volatile uint32_t count) {
    while (count--);
}

/*--------------------------------------------------
 * Command Processing
 *------------------------------------------------*/

void process_command(char *cmd) {
    /* Skip leading spaces */
    while (*cmd == ' ') cmd++;

    /* Empty command */
    if (*cmd == '\0') {
        return;
    }

    /* Help command */
    if (str_equal(cmd, "help") || str_equal(cmd, "?")) {
        uart_puts("\r\n");
        uart_puts("Available Commands:\r\n");
        uart_puts("  help        - Show this help message\r\n");
        uart_puts("  led on      - Turn all LEDs on\r\n");
        uart_puts("  led off     - Turn all LEDs off\r\n");
        uart_puts("  led 0-3 on  - Turn specific LED on\r\n");
        uart_puts("  led 0-3 off - Turn specific LED off\r\n");
        uart_puts("  blink       - Blink all LEDs 5 times\r\n");
        uart_puts("  status      - Show system status\r\n");
        uart_puts("  chase       - LED chase pattern\r\n");
        uart_puts("\r\n");
        return;
    }

    /* LED on/off commands */
    if (str_equal(cmd, "led on")) {
        led_all(1);
        uart_puts("All LEDs ON\r\n");
        return;
    }

    if (str_equal(cmd, "led off")) {
        led_all(0);
        uart_puts("All LEDs OFF\r\n");
        return;
    }

    /* Individual LED control: led N on/off */
    if (str_startswith(cmd, "led ")) {
        char *p = cmd + 4;
        if (*p >= '0' && *p <= '3') {
            uint8_t led = *p - '0';
            p++;
            while (*p == ' ') p++;

            if (str_equal(p, "on")) {
                led_set(led, 1);
                uart_puts("LED");
                uart_putchar('0' + led);
                uart_puts(" ON\r\n");
                return;
            }
            if (str_equal(p, "off")) {
                led_set(led, 0);
                uart_puts("LED");
                uart_putchar('0' + led);
                uart_puts(" OFF\r\n");
                return;
            }
        }
        uart_puts("Usage: led on | led off | led 0-3 on | led 0-3 off\r\n");
        return;
    }

    /* Blink command */
    if (str_equal(cmd, "blink")) {
        uart_puts("Blinking LEDs...\r\n");
        for (int i = 0; i < 5; i++) {
            led_all(1);
            delay(1000000);
            led_all(0);
            delay(1000000);
        }
        uart_puts("Done.\r\n");
        return;
    }

    /* Chase command */
    if (str_equal(cmd, "chase")) {
        uart_puts("LED chase pattern...\r\n");
        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 4; i++) {
                led_all(0);
                led_set(i, 1);
                delay(500000);
            }
        }
        led_all(0);
        uart_puts("Done.\r\n");
        return;
    }

    /* Status command */
    if (str_equal(cmd, "status")) {
        uart_puts("\r\n");
        uart_puts("=== System Status ===\r\n");
        uart_puts("MCU: LPC1343\r\n");
        uart_puts("Clock: 72 MHz\r\n");
        uart_puts("UART: 115200 baud, 8N1\r\n");
        uart_puts("LEDs: P3.0-P3.3 (active-low)\r\n");

        /* Show current LED state */
        uint32_t led_state = GPIO3DATA;
        uart_puts("LED States: ");
        for (int i = 0; i < 4; i++) {
            uart_putchar('0' + i);
            uart_putchar('=');
            uart_putchar((led_state & (1 << i)) ? '0' : '1');  /* Inverted (active-low) */
            if (i < 3) uart_putchar(' ');
        }
        uart_puts("\r\n\r\n");
        return;
    }

    /* Unknown command */
    uart_puts("Unknown command: '");
    uart_puts(cmd);
    uart_puts("'\r\n");
    uart_puts("Type 'help' for available commands.\r\n");
}

/*--------------------------------------------------
 * Main Program
 *------------------------------------------------*/

int main(void) {
    char cmd_buf[CMD_BUF_SIZE];

    /* Initialize peripherals */
    led_init();
    uart_init(115200);

    /* Welcome message */
    uart_puts("\r\n");
    uart_puts("====================================\r\n");
    uart_puts("LPC1343 Command Line Interface\r\n");
    uart_puts("====================================\r\n");
    uart_puts("Type 'help' for available commands.\r\n");
    uart_puts("\r\n");

    /* Main command loop */
    while (1) {
        uart_puts("> ");
        uart_getline(cmd_buf, CMD_BUF_SIZE);
        process_command(cmd_buf);
    }

    return 0;
}
