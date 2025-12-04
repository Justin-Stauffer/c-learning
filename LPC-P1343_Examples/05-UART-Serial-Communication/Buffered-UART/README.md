# Buffered-UART

Chapter 5: UART/Serial Communication - Interrupt-Driven Buffered UART Example

## What This Example Demonstrates

- UART RX interrupt handler
- Ring buffer implementation
- Non-blocking receive API
- Interrupt enable via NVIC
- Buffer overflow detection
- Visual buffer status feedback

## Hardware

- P1.6: UART RXD (connect to USB-Serial adapter TXD)
- P1.7: UART TXD (connect to USB-Serial adapter RXD)
- P3.0: LED0 - Buffer has data (>0%)
- P3.1: LED1 - Buffer >=25% full
- P3.2: LED2 - Buffer >=50% full
- P3.3: LED3 - Buffer >=75% full (warning)
- USB-to-Serial adapter required

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

## Commands

| Key | Action |
|-----|--------|
| `s` | Show buffer status |
| `f` | Flush (clear) receive buffer |
| `t` | Run test (sends data while you type) |
| Any | Echo character back |

## Expected Behavior

```
======================================
LPC1343 Interrupt-Driven UART Example
======================================

Features:
  - Interrupt-driven RX with ring buffer
  - Non-blocking read API
  - Buffer fill level on LEDs

Commands:
  Type any text - buffered and echoed
  's' - Show buffer status
  'f' - Flush receive buffer
  't' - Test: send burst of data

LEDs show buffer fill level:
  LED0=data, LED1=25%+, LED2=50%+, LED3=75%+

> s

=== Buffer Status ===
Buffer size: 64 bytes
Data in buffer: 0 bytes
Total received: 1 chars
Overrun count: 0
Head index: 1, Tail index: 1

>
```

## Code Highlights

**Ring Buffer Structure:**
```c
#define RX_BUF_SIZE 64  /* Must be power of 2 */

volatile uint8_t rx_buffer[RX_BUF_SIZE];
volatile uint16_t rx_head = 0;  /* Write index (ISR) */
volatile uint16_t rx_tail = 0;  /* Read index (main) */
```

**UART Interrupt Handler:**
```c
void UART0_IRQHandler(void) {
    uint32_t iir = U0IIR;

    if (!(iir & 0x01)) {  /* Interrupt pending? */
        while (U0LSR & (1 << 0)) {  /* Data ready? */
            uint8_t c = U0RBR;
            uint16_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);

            if (next != rx_tail) {
                rx_buffer[rx_head] = c;
                rx_head = next;
            } else {
                rx_overrun++;  /* Buffer full */
            }
        }
    }
}
```

**Non-blocking Read:**
```c
int16_t uart_read(void) {
    if (rx_head == rx_tail) return -1;  /* Empty */

    uint8_t c = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
    return c;
}
```

**Enable UART Interrupt:**
```c
U0IER = (1 << 0);           /* Enable RX interrupt */
NVIC_ISER = (1 << 21);      /* Enable UART IRQ in NVIC */
```

## Ring Buffer Operation

```
Empty buffer:
  head = tail

              tail    head
                ↓       ↓
Buffer:  [ ][ ][ ][ ][ ][ ][ ][ ]

After receiving "ABC":
                      tail          head
                        ↓             ↓
Buffer:  [ ][ ][ ][ ][ ][A][B][C][ ][ ]

After reading "A":
                          tail      head
                            ↓         ↓
Buffer:  [ ][ ][ ][ ][ ][ ][B][C][ ][ ]

Wrap-around:
        head              tail
          ↓                 ↓
Buffer:  [X][Y][ ][ ][ ][ ][D][E][F][G]
```

## Key Concepts

1. **Interrupt vs Polling**: ISR captures data immediately, main loop processes when ready
2. **Power of 2 Buffer**: Enables fast `& (SIZE-1)` modulo instead of `% SIZE`
3. **volatile Variables**: Required for data shared between ISR and main
4. **Non-blocking API**: `uart_read()` returns -1 instead of waiting
5. **Overrun Detection**: Count lost characters when buffer full

## UART Interrupt Enable

| Register | Bit | Name | Description |
|----------|-----|------|-------------|
| U0IER | 0 | RBRIE | RX Data Available Interrupt Enable |
| U0IER | 1 | THREIE | TX Holding Empty Interrupt Enable |
| U0IER | 2 | RXLIE | RX Line Status Interrupt Enable |
| NVIC_ISER | 21 | - | UART IRQ Enable in NVIC |

## Advantages Over Polling

- **No data loss**: ISR captures characters while main loop is busy
- **CPU efficiency**: Main loop can do other work
- **Deterministic latency**: Data captured immediately on arrival
- **Higher baud rates**: Handles continuous data streams
