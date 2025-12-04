# Serial-Echo

Chapter 5: UART/Serial Communication - Serial Echo Example

## What This Example Demonstrates

- UART receive data polling
- Non-blocking receive check with `uart_rx_ready()`
- Character echo (loopback)
- Visual feedback with LED toggle
- Simple terminal interaction

## Hardware

- P1.6: UART RXD (connect to USB-Serial adapter TXD)
- P1.7: UART TXD (connect to USB-Serial adapter RXD)
- P3.0: LED0 - toggles on each character received
- P3.1: LED1 - toggles on control characters
- USB-to-Serial adapter required

## Hardware Connection

```
LPC1343           USB-Serial Adapter
-------           ------------------
P1.7 (TXD) -----> RXD
P1.6 (RXD) <----- TXD
GND        -----> GND
```

## Terminal Settings

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None
- Local echo: OFF (characters will be echoed by the device)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: Welcome message appears in terminal
2. **Typing**: Each character you type appears once (echoed by LPC1343)
3. **LED0**: Toggles on every character received
4. **Enter**: Shows character count and new prompt

Example session:
```
=================================
LPC1343 UART Serial Echo Example
=================================

Type characters - they will be echoed back.
LED0 toggles on each character received.

> Hello World
[Received 11 chars]
> test
[Received 4 chars]
>
```

## Code Highlights

**Non-blocking receive check:**
```c
uint8_t uart_rx_ready(void) {
    return (U0LSR & (1 << 0)) ? 1 : 0;
}
```

**Blocking receive:**
```c
char uart_getchar(void) {
    while (!uart_rx_ready());  /* Wait for data */
    return U0RBR;              /* Read and return character */
}
```

**Main loop pattern:**
```c
while (1) {
    if (uart_rx_ready()) {
        char c = uart_getchar();
        uart_putchar(c);       /* Echo back */
        led_toggle(0);         /* Visual feedback */
    }
}
```

## Line Status Register (U0LSR)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | RDR | Receiver Data Ready - data available in RBR |
| 1 | OE | Overrun Error - data lost |
| 2 | PE | Parity Error |
| 3 | FE | Framing Error |
| 4 | BI | Break Interrupt |
| 5 | THRE | TX Holding Register Empty - ready to send |
| 6 | TEMT | Transmitter Empty - all data sent |

## Key Concepts

1. **Polling vs Interrupt**: This example uses polling (continuously checking LSR). Works well for simple applications but wastes CPU cycles.
2. **Non-blocking Check**: `uart_rx_ready()` returns immediately, allowing other work in the main loop.
3. **FIFO**: The UART FIFO can buffer up to 16 characters, preventing data loss during brief delays.
