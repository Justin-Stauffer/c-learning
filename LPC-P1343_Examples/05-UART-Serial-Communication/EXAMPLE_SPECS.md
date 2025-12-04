# Chapter 5 Example Specifications

Reference file for creating UART/Serial Communication example projects. Read this before creating each example.

## Common Files Needed Per Project

Each example folder needs:
1. `main.c` - The example code
2. `Makefile` - Build configuration (copy template, change PROJECT name)
3. `startup_lpc1343_gcc.s` - Copy from 00-Getting-Started
4. `lpc1343_flash.ld` - Copy from 00-Getting-Started
5. `README.md` - Brief description of the example

## Hardware Configuration

```
System Clock: 72 MHz (assumed)

LEDs (Active-Low):
  P3.0 - LED0
  P3.1 - LED1
  P3.2 - LED2
  P3.3 - LED3
  LED_MASK = 0x0F

Button:
  P0.1 - Main button (active-low)

UART Pins:
  P1.6 - RXD (IOCON = 0x01)
  P1.7 - TXD (IOCON = 0x01)

USB-to-Serial Connection:
  LPC1343 TXD (P1.7) → Adapter RXD
  LPC1343 RXD (P1.6) → Adapter TXD
  GND → GND
```

## Register Addresses

```c
/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))
#define UARTCLKDIV     (*((volatile uint32_t *)0x40048098))

/* IOCON for UART pins */
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))  /* RXD */
#define IOCON_PIO1_7   (*((volatile uint32_t *)0x400440A8))  /* TXD */

/* IOCON for LED pins */
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* IOCON for Button */
#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))

/* UART Registers (active when DLAB=0) */
#define U0RBR          (*((volatile uint32_t *)0x40008000))  /* Receive Buffer (read) */
#define U0THR          (*((volatile uint32_t *)0x40008000))  /* Transmit Holding (write) */
#define U0IER          (*((volatile uint32_t *)0x40008004))  /* Interrupt Enable */

/* UART Registers (active when DLAB=1) */
#define U0DLL          (*((volatile uint32_t *)0x40008000))  /* Divisor Latch LSB */
#define U0DLM          (*((volatile uint32_t *)0x40008004))  /* Divisor Latch MSB */

/* UART Registers (always accessible) */
#define U0IIR          (*((volatile uint32_t *)0x40008008))  /* Interrupt ID (read) */
#define U0FCR          (*((volatile uint32_t *)0x40008008))  /* FIFO Control (write) */
#define U0LCR          (*((volatile uint32_t *)0x4000800C))  /* Line Control */
#define U0LSR          (*((volatile uint32_t *)0x40008014))  /* Line Status */
#define U0FDR          (*((volatile uint32_t *)0x40008028))  /* Fractional Divider */

/* GPIO Port 0 (button) */
#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))

/* GPIO Port 3 (LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))
```

## Clock Enable Bits (SYSAHBCLKCTRL)

```c
#define GPIO_CLK       (1 << 6)
#define UART_CLK       (1 << 12)
```

## NVIC IRQ Numbers

```c
#define UART_IRQn      21
```

## Baud Rate Calculation

```c
/* At 72 MHz system clock:
 * Divisor = 72,000,000 / (16 * baud_rate)
 *
 * 9600:   divisor = 469 (DLL=0xD5, DLM=0x01)
 * 115200: divisor = 39  (DLL=0x27, DLM=0x00) ~1.6% error, acceptable
 */
#define SYSTEM_CLOCK   72000000UL
```

## Line Status Register (U0LSR) Bits

```c
#define LSR_RDR        (1 << 0)  /* Receiver Data Ready */
#define LSR_OE         (1 << 1)  /* Overrun Error */
#define LSR_PE         (1 << 2)  /* Parity Error */
#define LSR_FE         (1 << 3)  /* Framing Error */
#define LSR_THRE       (1 << 5)  /* TX Holding Register Empty */
#define LSR_TEMT       (1 << 6)  /* Transmitter Empty */
```

## Project Naming Convention

- Hello-World: `PROJECT = lpc1343_uart_hello`
- Serial-Echo: `PROJECT = lpc1343_uart_echo`
- Command-Line: `PROJECT = lpc1343_uart_cli`
- Buffered-UART: `PROJECT = lpc1343_uart_buffered`

---

## Example 1: Hello-World

**Status: CREATED**

**Concepts:** Basic UART init, polling TX, string output

**Behavior:**
- Initialize UART at 115200 baud, 8N1
- Send "Hello, World!" message
- Blink LED to show program is running

**Key code:**
```c
void uart_init(uint32_t baud) {
    SYSAHBCLKCTRL |= (1 << 12);  /* UART clock */
    IOCON_PIO1_6 = 0x01;          /* RXD */
    IOCON_PIO1_7 = 0x01;          /* TXD */
    UARTCLKDIV = 1;

    U0LCR = 0x80;                 /* DLAB=1 */
    uint32_t div = SYSTEM_CLOCK / (16 * baud);
    U0DLL = div & 0xFF;
    U0DLM = (div >> 8) & 0xFF;
    U0LCR = 0x03;                 /* 8N1, DLAB=0 */
    U0FCR = 0x07;                 /* Enable FIFOs */
}

void uart_putchar(char c) {
    while (!(U0LSR & (1 << 5)));  /* Wait THRE */
    U0THR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}
```

---

## Example 2: Serial-Echo

**Status: CREATED**

**Concepts:** UART RX polling, character echo, simple interaction

**Behavior:**
- Initialize UART at 115200 baud
- Wait for incoming characters
- Echo each character back
- Toggle LED on each character received

**Key code:**
```c
uint8_t uart_rx_ready(void) {
    return (U0LSR & (1 << 0)) ? 1 : 0;
}

char uart_getchar(void) {
    while (!uart_rx_ready());
    return U0RBR;
}

/* Main loop */
while (1) {
    if (uart_rx_ready()) {
        char c = uart_getchar();
        uart_putchar(c);  /* Echo */
        toggle_led(0);
    }
}
```

---

## Example 3: Command-Line

**Status: CREATED**

**Concepts:** Line input, string parsing, command dispatch

**Behavior:**
- Prompt user with "> "
- Read line of input (with backspace support)
- Parse and execute commands:
  - `help` - Show available commands
  - `led on` / `led off` - Control LED
  - `status` - Show system info
  - `blink` - Blink LEDs demo

**Key code:**
```c
void process_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        uart_puts("Commands: help, led on, led off, status, blink\r\n");
    }
    else if (strcmp(cmd, "led on") == 0) {
        set_led(0, 1);
        uart_puts("LED ON\r\n");
    }
    else if (strcmp(cmd, "led off") == 0) {
        set_led(0, 0);
        uart_puts("LED OFF\r\n");
    }
    /* ... etc */
}

uint32_t uart_getline(char *buf, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1) {
        char c = uart_getchar();
        uart_putchar(c);  /* Echo */
        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            break;
        }
        if (c == '\b' && i > 0) {
            i--;
            uart_puts("\b \b");
            continue;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}
```

---

## Example 4: Buffered-UART

**Status: CREATED**

**Concepts:** Interrupt-driven RX, ring buffer, non-blocking API

**Behavior:**
- Use UART RX interrupt to collect characters
- Store in ring buffer
- Provide non-blocking API
- Main loop processes data when available
- Show buffer statistics

**Key code:**
```c
#define RX_BUF_SIZE 64

volatile uint8_t rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

void UART_IRQHandler(void) {
    if ((U0IIR & 0x0E) == 0x04) {  /* RDA */
        while (U0LSR & (1 << 0)) {
            uint8_t c = U0RBR;
            uint16_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);
            if (next != rx_tail) {
                rx_buf[rx_head] = c;
                rx_head = next;
            }
        }
    }
}

uint16_t uart_available(void) {
    return (rx_head - rx_tail) & (RX_BUF_SIZE - 1);
}

int16_t uart_read(void) {
    if (rx_head == rx_tail) return -1;
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
    return c;
}

void uart_init_interrupt(uint32_t baud) {
    /* ... standard init ... */
    U0IER = (1 << 0);           /* RX interrupt */
    NVIC_ISER = (1 << 21);      /* Enable UART IRQ */
}
```

---

## Makefile Template

Copy from previous examples, change:
- Comment line to match example name
- `PROJECT = lpc1343_uart_<example_name>`

---

## README Template

```markdown
# <Example Name>

Chapter 5: UART/Serial Communication - <Example Name>

## What This Example Demonstrates

- Bullet points

## Hardware

- P1.6: UART RXD
- P1.7: UART TXD
- P3.0-P3.3: Status LEDs
- USB-to-Serial adapter required

## Terminal Settings

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

## Building and Flashing

\`\`\`bash
make clean
make
make flash
\`\`\`

## Expected Behavior

Description...

## Code Highlights

Key code snippets...
```
