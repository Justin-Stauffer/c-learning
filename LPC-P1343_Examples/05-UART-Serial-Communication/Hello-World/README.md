# Hello-World

Chapter 5: UART/Serial Communication - Hello World Example

## What This Example Demonstrates

- Basic UART peripheral initialization
- Baud rate calculation and configuration
- Polled (blocking) character transmission
- String output functions
- 8N1 serial format configuration

## Hardware

- P1.6: UART RXD (not used in this example)
- P1.7: UART TXD (connect to USB-Serial adapter RXD)
- P3.0: Status LED (blinks during operation)
- USB-to-Serial adapter required

## Hardware Connection

```
LPC1343           USB-Serial Adapter
-------           ------------------
P1.7 (TXD) -----> RXD
P1.6 (RXD) <----- TXD (optional for this example)
GND        -----> GND
```

## Terminal Settings

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: Sends welcome banner over UART
2. **Running**: LED0 blinks, sends "Hello, World! Count: N" every ~1 second
3. **Terminal output**:
   ```
   ================================
   LPC1343 UART Hello World Example
   ================================

   UART configured: 115200 baud, 8N1
   System clock: 72 MHz

   Hello, World! Count: 0
   Hello, World! Count: 1
   Hello, World! Count: 2
   ...
   ```

## Code Highlights

**UART Initialization:**
```c
void uart_init(uint32_t baud) {
    SYSAHBCLKCTRL |= (1 << 12);  /* Enable UART clock */
    UARTCLKDIV = 1;               /* UART clock = system clock */
    IOCON_PIO1_6 = 0x01;          /* P1.6 = RXD */
    IOCON_PIO1_7 = 0x01;          /* P1.7 = TXD */

    U0LCR = 0x80;                 /* DLAB=1 for baud rate */
    uint32_t divisor = 72000000 / (16 * baud);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;
    U0LCR = 0x03;                 /* 8N1, DLAB=0 */
    U0FCR = 0x07;                 /* Enable FIFOs */
}
```

**Polled Transmit:**
```c
void uart_putchar(char c) {
    while (!(U0LSR & (1 << 5)));  /* Wait for THRE */
    U0THR = c;
}
```

## Baud Rate Calculation

At 72 MHz system clock:
- Divisor = 72,000,000 / (16 × 115,200) = 39.0625
- Using divisor = 39 gives actual baud = 115,385 (~0.16% error)

## Key Concepts

1. **DLAB Bit**: The Divisor Latch Access Bit in U0LCR switches between data/interrupt registers and baud rate divisor registers
2. **FIFO Enable**: Setting FCR[0] enables 16-byte TX/RX FIFOs for better performance
3. **Line Status Register**: LSR[5] (THRE) indicates when the transmit holding register is empty
