# Chapter 5: UART/Serial Communication

A comprehensive beginner's guide to UART (Universal Asynchronous Receiver/Transmitter) serial communication on the LPC1343 microcontroller.

---

## Chapter Overview

| | |
|---|---|
| **Prerequisites** | Chapters 0-4 (especially Bitwise Ops and Timers) |
| **Time to Complete** | 3-4 hours |
| **Hands-On Projects** | Serial echo, command line interface, data logger |
| **You Will Learn** | Serial protocols, UART setup, printf debugging, ring buffers |

---

## Quick Start: Hello World over Serial

Here's the minimal code to send "Hello World" over UART at 115200 baud:

```c
#define SYSAHBCLKCTRL (*((volatile unsigned int *)0x40048080))
#define IOCON_PIO1_6  (*((volatile unsigned int *)0x400440A4))
#define IOCON_PIO1_7  (*((volatile unsigned int *)0x400440A8))
#define UARTCLKDIV    (*((volatile unsigned int *)0x40048098))
#define U0LCR         (*((volatile unsigned int *)0x4000800C))
#define U0DLL         (*((volatile unsigned int *)0x40008000))
#define U0DLM         (*((volatile unsigned int *)0x40008004))
#define U0FCR         (*((volatile unsigned int *)0x40008008))
#define U0LSR         (*((volatile unsigned int *)0x40008014))
#define U0THR         (*((volatile unsigned int *)0x40008000))

void uart_init(void) {
    SYSAHBCLKCTRL |= (1 << 12);  // Enable UART clock
    IOCON_PIO1_6 = 0x01;         // P1.6 = RXD
    IOCON_PIO1_7 = 0x01;         // P1.7 = TXD
    UARTCLKDIV = 1;              // UART clock = system clock

    U0LCR = 0x80;                // Enable divisor access
    U0DLL = 39;                  // 72MHz / (16 * 115200) = 39.06 ≈ 39 (~1.6% error)
    U0DLM = 0;
    U0LCR = 0x03;                // 8N1, disable divisor access
    U0FCR = 0x07;                // Enable and reset FIFOs
}

void uart_putchar(char c) {
    while (!(U0LSR & (1 << 5))); // Wait for TX ready
    U0THR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}

int main(void) {
    uart_init();
    uart_puts("Hello, World!\r\n");
    while (1);
}
```

**Connect a USB-to-Serial adapter:**
- LPC1343 P1.7 (TXD) → Adapter RXD
- LPC1343 P1.6 (RXD) → Adapter TXD
- GND → GND

Open a terminal at 115200 baud, 8N1.

The rest of this chapter covers interrupts, ring buffers, printf, and more.

---

## Table of Contents

1. [What is UART?](#what-is-uart)
2. [Serial Communication Fundamentals](#serial-communication-fundamentals)
3. [UART Protocol Details](#uart-protocol-details)
4. [LPC1343 UART Hardware](#lpc1343-uart-hardware)
5. [UART Registers](#uart-registers)
6. [Basic UART Configuration](#basic-uart-configuration)
7. [Transmitting Data](#transmitting-data)
8. [Receiving Data](#receiving-data)
9. [Interrupt-Driven UART](#interrupt-driven-uart)
10. [Ring Buffers](#ring-buffers)
11. [Printf Implementation](#printf-implementation)
12. [Error Handling](#error-handling)
13. [Practical Examples](#practical-examples)
14. [Debugging with Serial](#debugging-with-serial)
15. [Common Patterns](#common-patterns)
16. [Troubleshooting](#troubleshooting)
17. [Quick Reference](#quick-reference)

---

## What is UART?

**UART** (Universal Asynchronous Receiver/Transmitter) is a hardware peripheral that converts parallel data from the CPU into serial data for transmission, and vice versa.

### Why UART?

```
The Problem: Microcontroller has 32 data lines, but we want
to communicate over just 2 wires.

┌───────────────┐     32 parallel lines    ┌───────────────┐
│               │═══════════════════════════│               │
│  LPC1343      │     (expensive, bulky)   │  Other Device │
│               │═══════════════════════════│               │
└───────────────┘                           └───────────────┘


The Solution: UART converts to serial (one bit at a time)

┌───────────────┐           TX ────────────→ RX  ┌───────────────┐
│               │                                │               │
│  LPC1343      │                                │  Other Device │
│    UART       │                                │    UART       │
│               │           RX ←──────────── TX  │               │
└───────────────┘                                └───────────────┘
                      Only 2 wires needed!
                      (plus ground)
```

### UART Applications

```
┌─────────────────────────────────────────────────────────────────┐
│  Common UART Uses:                                               │
├─────────────────────────────────────────────────────────────────┤
│  • Debug output (printf to PC terminal)                         │
│  • Communicating with GPS modules                               │
│  • Bluetooth modules (HC-05, etc.)                              │
│  • WiFi modules (ESP8266, etc.)                                 │
│  • GSM/cellular modems                                          │
│  • PC communication (USB-to-Serial adapters)                    │
│  • Inter-processor communication                                │
│  • MIDI (musical instruments)                                   │
│  • RS-232/RS-485 industrial protocols                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## Serial Communication Fundamentals

### Parallel vs Serial

```
PARALLEL COMMUNICATION:
Send 8 bits simultaneously on 8 wires

Clock: ──┐ ┌─┐ ┌─┐ ┌───
         └─┘ └─┘ └─┘
D0:    ═══0═══1═══0═══
D1:    ═══1═══0═══0═══
D2:    ═══1═══1═══1═══
D3:    ═══0═══0═══1═══
D4:    ═══1═══1═══0═══
D5:    ═══0═══0═══1═══
D6:    ═══1═══1═══0═══
D7:    ═══0═══0═══1═══

       Byte1 Byte2 Byte3
       0x56  0x63  0x95

Pros: Fast (8 bits per clock)
Cons: Many wires, synchronization issues at high speed


SERIAL COMMUNICATION (UART):
Send 8 bits one at a time on 1 wire

Data: ─┐ ┌─┐ ┌─────┐ ┌─────────┐ ┌─┐ ┌─┐ ┌─────┐ ┌─
       └─┘ └─┘     └─┘         └─┘ └─┘ └─┘     └─┘
       S 0 1 1 0 1 0 1 0 P S 1 1 0 0 0 1 1 0 P
       │ └───────────┘ │ │ └───────────┘ │ │
       │   Data bits   │ │   Data bits   │ │
      Start    (LSB first) Stop      Start    Stop

Pros: Only 1 wire (+ ground), long distance
Cons: Slower (1 bit at a time)
```

### Asynchronous vs Synchronous

```
SYNCHRONOUS (SPI, I2C):
┌──────────────────────────────────────────────────┐
│  Separate clock line tells when to read data     │
│                                                   │
│  CLK:  ─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌──       │
│         └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘          │
│  DATA: ═══0═══1═══1═══0═══1═══0═══1═══0═══       │
│                                                   │
│  Transmitter and receiver share timing           │
└──────────────────────────────────────────────────┘

ASYNCHRONOUS (UART):
┌──────────────────────────────────────────────────┐
│  NO clock line - both sides agree on timing      │
│                                                   │
│  DATA: ─┐ ┌─┐ ┌─────┐ ┌─────────┐ ┌──            │
│         └─┘ └─┘     └─┘         └─┘              │
│         ↑                                        │
│         Start bit signals "data coming"          │
│                                                   │
│  Both sides must use same baud rate!             │
└──────────────────────────────────────────────────┘
```

---

## UART Protocol Details

### Frame Structure

A UART frame consists of:

```
Complete UART Frame (8N1 - most common):

Idle ──────┐         ┌─────────────────────────── Idle
           │         │
           │ 0 1 2 3 4 5 6 7 │
           └─┴─┴─┴─┴─┴─┴─┴─┴─┘

           │ │       │       │ │
           │ │       │       │ └── Stop bit (1)
           │ │       │       │
           │ └───────┴───────┴──── Data bits (8)
           │                       (LSB first)
           │
           └────────────────────── Start bit (0)

Bit timing at 9600 baud:
Each bit = 1/9600 = 104.17 µs
Total frame = 10 bits × 104.17 µs = 1.042 ms
```

### Key Parameters

#### Baud Rate
Number of signal changes (bits) per second.

```
Common Baud Rates:
┌────────────┬─────────────┬──────────────────────┐
│ Baud Rate  │ Bit Time    │ Typical Use          │
├────────────┼─────────────┼──────────────────────┤
│     300    │  3333 µs    │ Very old equipment   │
│    1200    │   833 µs    │ Old modems           │
│    2400    │   417 µs    │ Low speed sensors    │
│    9600    │   104 µs    │ Default, debug       │
│   19200    │    52 µs    │ Moderate speed       │
│   38400    │    26 µs    │ GPS modules          │
│   57600    │  17.4 µs    │ Faster debug         │
│  115200    │   8.7 µs    │ Fast, common         │
│  230400    │   4.3 µs    │ High speed           │
│  460800    │   2.2 µs    │ Very high speed      │
│  921600    │   1.1 µs    │ Maximum (many UARTs) │
└────────────┴─────────────┴──────────────────────┘

Most common: 9600 (safe default) and 115200 (fast)
```

#### Data Bits
How many bits of data per frame (5, 6, 7, or 8).

```
8 data bits (most common):
Can represent 0-255 (full ASCII + binary)

7 data bits:
Can represent 0-127 (ASCII only)
Was common with parity for error detection
```

#### Parity
Optional error detection bit.

```
No Parity (N): No parity bit added
Even Parity (E): Parity bit makes total 1s even
Odd Parity (O): Parity bit makes total 1s odd

Example: Data = 0b01010011 (has 4 ones)
- Even parity: Add 0 (4+0=4, even) ✓
- Odd parity: Add 1 (4+1=5, odd) ✓

Parity detects single-bit errors only.
Modern systems often skip parity (use checksums instead).
```

#### Stop Bits
Idle time after data (1, 1.5, or 2).

```
1 stop bit: Most common, fastest
2 stop bits: More reliable at slow speeds, long distances
```

### Common Configurations

```
Notation: <data bits><parity><stop bits>

8N1 (Most Common):
- 8 data bits
- No parity
- 1 stop bit
- Total: 10 bits per byte (start + 8 data + stop)

7E1:
- 7 data bits
- Even parity
- 1 stop bit
- Total: 10 bits per byte

8N2:
- 8 data bits
- No parity
- 2 stop bits
- Total: 11 bits per byte

Effective data rate at 115200 baud, 8N1:
115200 / 10 = 11520 bytes/second
```

---

## LPC1343 UART Hardware

### UART Block Diagram

```
                    ┌─────────────────────────────────────────┐
                    │            LPC1343 UART                  │
                    │                                          │
System Clock ──────→│  ┌──────────────────┐                   │
(72 MHz)            │  │   Baud Rate      │                   │
                    │  │   Generator      │                   │
                    │  │   (Dividers)     │                   │
                    │  └────────┬─────────┘                   │
                    │           │                              │
                    │           ↓                              │
                    │  ┌─────────────────┐    ┌────────────┐  │
                    │  │   Transmitter   │───→│ TX Shift   │──┼──→ TXD Pin
                    │  │   (THR/TSR)     │    │ Register   │  │    (P1.7)
                    │  └─────────────────┘    └────────────┘  │
   CPU ←──────────→│                                          │
   Data Bus         │  ┌─────────────────┐    ┌────────────┐  │
                    │  │   Receiver      │←───│ RX Shift   │←─┼─── RXD Pin
                    │  │   (RBR/RSR)     │    │ Register   │  │    (P1.6)
                    │  └─────────────────┘    └────────────┘  │
                    │                                          │
                    │  ┌─────────────────┐                    │
                    │  │   FIFO (16x8)   │                    │
        IRQ ←───────│←─│   TX and RX     │                    │
                    │  │   buffers       │                    │
                    │  └─────────────────┘                    │
                    │                                          │
                    └─────────────────────────────────────────┘
```

### UART Pins

```
LPC1343 has one UART:

Function    Pin       IOCON Register    IOCON Value
────────    ───       ──────────────    ───────────
TXD         P1.7      IOCON_PIO1_7      0x01 (FUNC=001)
RXD         P1.6      IOCON_PIO1_6      0x01 (FUNC=001)

Note: RTS/CTS hardware flow control not available on LPC1343
```

### Hardware Connection

```
LPC1343 to PC via USB-to-Serial Adapter:

┌─────────────┐                    ┌─────────────────────┐
│   LPC1343   │                    │ USB-Serial Adapter  │
│             │                    │   (FTDI, CP2102,    │
│        TXD ├────────────────────→│ RXD   CH340, etc.) │
│   (P1.7)    │                    │                     │
│             │                    │                     │
│        RXD ├←────────────────────┤ TXD                 │
│   (P1.6)    │                    │                     │
│             │                    │                     │
│        GND ├─────────────────────┤ GND                 │
│             │                    │                     │
└─────────────┘                    └──────────┬──────────┘
                                              │
                                              │ USB
                                              ↓
                                        ┌──────────┐
                                        │    PC    │
                                        │ Terminal │
                                        └──────────┘

IMPORTANT: Cross TX/RX connections!
- LPC1343 TXD → Adapter RXD
- LPC1343 RXD ← Adapter TXD
- Connect GND (common ground required)

Voltage levels:
- LPC1343: 3.3V logic (DO NOT connect to RS-232 levels!)
- Most USB adapters: 3.3V compatible (check your adapter)
```

### UART FIFOs

The LPC1343 UART has 16-byte FIFOs for both TX and RX:

```
Transmit FIFO:
┌────────────────────────────────────────────────────┐
│  CPU writes to THR                                  │
│  ↓                                                  │
│  ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐│
│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  ││  16 bytes
│  └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘│
│                                           ↓        │
│                               Shift out to TX pin  │
└────────────────────────────────────────────────────┘

Receive FIFO:
┌────────────────────────────────────────────────────┐
│                       Shift in from RX pin         │
│                               ↓                    │
│  ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐│
│  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  ││  16 bytes
│  └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘│
│  ↓                                                 │
│  CPU reads from RBR                                │
└────────────────────────────────────────────────────┘

Benefits of FIFOs:
- CPU doesn't need to respond to every byte
- Can handle burst traffic
- Reduces interrupt frequency
```

---

## UART Registers

### Register Overview

```
Address     Register    Description
───────     ────────    ────────────────────────────────────────
0x4000 8000 U0RBR       Receive Buffer (read only, DLAB=0)
0x4000 8000 U0THR       Transmit Holding (write only, DLAB=0)
0x4000 8000 U0DLL       Divisor Latch LSB (DLAB=1)
0x4000 8004 U0DLM       Divisor Latch MSB (DLAB=1)
0x4000 8004 U0IER       Interrupt Enable (DLAB=0)
0x4000 8008 U0IIR       Interrupt ID (read only)
0x4000 8008 U0FCR       FIFO Control (write only)
0x4000 800C U0LCR       Line Control
0x4000 8010 U0MCR       Modem Control
0x4000 8014 U0LSR       Line Status
0x4000 8018 U0MSR       Modem Status
0x4000 801C U0SCR       Scratch Pad
0x4000 8028 U0ACR       Auto-baud Control
0x4000 802C U0ICR       IrDA Control
0x4000 8030 U0FDR       Fractional Divider
0x4000 8054 U0TER       Transmit Enable
```

### U0LCR - Line Control Register

```
Bit     Name        Description
───     ────        ────────────────────────────────────────
[1:0]   WLS         Word Length Select
                    00 = 5 bits
                    01 = 6 bits
                    10 = 7 bits
                    11 = 8 bits (most common)

[2]     SBS         Stop Bit Select
                    0 = 1 stop bit
                    1 = 2 stop bits (1.5 for 5-bit)

[3]     PE          Parity Enable
                    0 = Disable parity
                    1 = Enable parity

[4]     PS          Parity Select (when PE=1)
                    0 = Odd parity
                    1 = Even parity

[5]     SP          Stick Parity
                    0 = Disable stick parity
                    1 = Enable stick parity

[6]     BC          Break Control
                    0 = Normal operation
                    1 = Force TX output LOW

[7]     DLAB        Divisor Latch Access Bit
                    0 = Access RBR/THR/IER
                    1 = Access DLL/DLM (baud rate)

Common value: 0x03 = 8N1 (8 data bits, no parity, 1 stop)
```

### U0LSR - Line Status Register

```
Bit     Name    Description
───     ────    ────────────────────────────────────────
[0]     RDR     Receiver Data Ready
                1 = Data available in RBR

[1]     OE      Overrun Error
                1 = Overrun error (data lost)

[2]     PE      Parity Error
                1 = Received byte had parity error

[3]     FE      Framing Error
                1 = Invalid stop bit detected

[4]     BI      Break Interrupt
                1 = Break condition detected

[5]     THRE    Transmit Holding Register Empty
                1 = THR is empty (can write new data)

[6]     TEMT    Transmitter Empty
                1 = THR and TSR both empty

[7]     RXFE    Error in RX FIFO
                1 = At least one error in RX FIFO

Most important bits:
- Check LSR[0] (RDR) before reading RBR
- Check LSR[5] (THRE) before writing THR
```

### U0IER - Interrupt Enable Register

```
Bit     Name    Description
───     ────    ────────────────────────────────────────
[0]     RBRIE   Receive Data Available Interrupt Enable
                Also enables character timeout interrupt

[1]     THREIE  THR Empty Interrupt Enable

[2]     RXLIE   RX Line Status Interrupt Enable
                (overrun, parity, framing, break)

[3]     -       Reserved

[7:4]   -       Reserved

Common values:
0x01 = RX interrupt only
0x03 = RX and TX interrupts
0x07 = RX, TX, and error interrupts
```

### U0IIR - Interrupt ID Register (Read Only)

```
Bit     Name        Description
───     ────        ────────────────────────────────────────
[0]     IntPending  Interrupt Pending
                    0 = Interrupt pending
                    1 = No interrupt pending

[3:1]   IntId       Interrupt Identification
                    011 = RX Line Status (highest priority)
                    010 = RX Data Available
                    110 = Character Timeout
                    001 = THR Empty (lowest priority)

[5:4]   -           Reserved

[7:6]   FIFO        FIFO Enable
                    11 = FIFOs enabled

Priority (highest to lowest):
1. RX Line Status (errors)
2. RX Data Available
3. Character Timeout
4. THR Empty
```

### U0FCR - FIFO Control Register (Write Only)

```
Bit     Name        Description
───     ────        ────────────────────────────────────────
[0]     FIFO_EN     FIFO Enable
                    1 = Enable TX and RX FIFOs

[1]     RX_FIFO_RST RX FIFO Reset
                    1 = Clear RX FIFO (self-clearing)

[2]     TX_FIFO_RST TX FIFO Reset
                    1 = Clear TX FIFO (self-clearing)

[3]     DMA         DMA Mode Select
                    (not used on LPC1343)

[5:4]   -           Reserved

[7:6]   RX_TRIG     RX FIFO Trigger Level
                    00 = 1 byte
                    01 = 4 bytes
                    10 = 8 bytes
                    11 = 14 bytes

Common value: 0x07 = Enable FIFOs, reset both, trigger at 1 byte
             0xC7 = Same but trigger at 14 bytes
```

---

## Basic UART Configuration

### Baud Rate Calculation

```
LPC1343 UART uses fractional baud rate generator:

                   PCLK
Baud Rate = ─────────────────────────────
            16 × (DLM:DLL) × (1 + DivAddVal/MulVal)

Where:
- PCLK = Peripheral clock (usually = system clock = 72 MHz)
- DLM:DLL = 16-bit divisor (DLM is MSB, DLL is LSB)
- DivAddVal, MulVal = Fractional divider (0-14, 1-15)

Simplified (when fractional divider = 1):
            PCLK
Divisor = ─────────────
          16 × Baud Rate

Example at 72 MHz for 115200 baud:
Divisor = 72,000,000 / (16 × 115200) = 39.0625

We need integer divisor, so use fractional divider:
DLL:DLM = 39
DivAddVal = 1, MulVal = 2
Actual baud = 72,000,000 / (16 × 39 × 1.5) = 76,923...

Better combination:
DLL:DLM = 26
DivAddVal = 1, MulVal = 4
Actual baud = 72,000,000 / (16 × 26 × 1.25) = 138,461... still off

Finding exact values can be complex. Common approach:
1. Use lookup tables for standard baud rates
2. Use online calculators
3. Accept small error (< 3% usually works)
```

### Baud Rate Lookup Table

```c
// Common baud rates at 72 MHz system clock
// DLL, DLM, DivAddVal, MulVal

typedef struct {
    uint32_t baud;
    uint8_t dll;
    uint8_t dlm;
    uint8_t divaddval;
    uint8_t mulval;
} BaudConfig;

const BaudConfig baud_table[] = {
    {   9600, 0xE1, 0x01, 0, 1 },  // DL=481
    {  19200, 0xF0, 0x00, 0, 1 },  // DL=240
    {  38400, 0x78, 0x00, 0, 1 },  // DL=120
    {  57600, 0x50, 0x00, 0, 1 },  // DL=80
    { 115200, 0x27, 0x00, 1, 2 },  // DL=39 with fractional
    { 230400, 0x14, 0x00, 5, 8 },  // DL=20 with fractional
};
```

### UART Initialization

```c
// ============================================
// Basic UART Initialization
// ============================================

#define SYSTEM_CLOCK 72000000UL

void uart_init(uint32_t baud_rate) {
    // Step 1: Enable UART clock
    SYSAHBCLKCTRL |= (1 << 12);  // Bit 12 = UART

    // Step 2: Configure UART pins
    // P1.6 = RXD, P1.7 = TXD
    IOCON_PIO1_6 = 0x01;  // FUNC = UART RXD
    IOCON_PIO1_7 = 0x01;  // FUNC = UART TXD

    // Step 3: Configure UART clock divider (if needed)
    UARTCLKDIV = 1;  // UART clock = system clock / 1

    // Step 4: Set DLAB=1 to access divisor latches
    U0LCR = 0x80;  // DLAB = 1

    // Step 5: Set baud rate divisor
    // For 115200 baud at 72 MHz:
    // Divisor = 72,000,000 / (16 × 115200) = 39.0625 ≈ 39
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud_rate);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;

    // Optional: Set fractional divider for more accuracy
    // U0FDR = (mulval << 4) | divaddval;

    // Step 6: Set line control (8N1) and clear DLAB
    U0LCR = 0x03;  // 8 data bits, 1 stop bit, no parity, DLAB=0

    // Step 7: Enable and reset FIFOs
    U0FCR = 0x07;  // Enable FIFOs, reset TX and RX FIFOs

    // Step 8: Clear any pending interrupts
    volatile uint32_t dummy;
    dummy = U0IIR;  // Reading clears some interrupts
    dummy = U0LSR;  // Clear line status
    dummy = U0RBR;  // Clear receive buffer
    (void)dummy;
}

// Usage:
int main(void) {
    uart_init(115200);
    // UART ready to use
}
```

### More Accurate Baud Rate Setup

```c
// ============================================
// Precise baud rate with fractional divider
// ============================================

void uart_set_baud(uint32_t pclk, uint32_t baud) {
    uint32_t divisor;
    uint8_t divaddval = 0;
    uint8_t mulval = 1;

    // Try to find best divisor
    // This is a simplified search - production code should be more thorough

    divisor = pclk / (16 * baud);

    // Check if we need fractional divider
    uint32_t actual_baud = pclk / (16 * divisor);
    int32_t error = actual_baud - baud;

    if (error > baud / 50) {  // More than 2% error
        // Try fractional dividers
        // This is a brute-force search for better accuracy
        uint32_t best_error = error;
        uint8_t best_div = 0, best_mul = 1;

        for (uint8_t mul = 1; mul <= 15; mul++) {
            for (uint8_t div = 0; div < mul; div++) {
                uint32_t test_div = pclk / (16 * baud * mul / (mul + div));
                if (test_div == 0) continue;

                uint32_t test_baud = pclk * (mul + div) / (16 * test_div * mul);
                int32_t test_error = (test_baud > baud) ?
                                     (test_baud - baud) : (baud - test_baud);

                if (test_error < best_error) {
                    best_error = test_error;
                    best_div = div;
                    best_mul = mul;
                    divisor = test_div;
                }
            }
        }
        divaddval = best_div;
        mulval = best_mul;
    }

    // Set DLAB to access divisor
    U0LCR |= 0x80;

    // Set divisor
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;

    // Set fractional divider
    U0FDR = (mulval << 4) | divaddval;

    // Clear DLAB
    U0LCR &= ~0x80;
}
```

---

## Transmitting Data

### Polling-Based Transmit

```c
// ============================================
// Simple polling transmit (blocking)
// ============================================

void uart_putchar(char c) {
    // Wait until transmit buffer is empty
    while (!(U0LSR & (1 << 5))) {
        // Wait for THRE (Transmit Holding Register Empty)
    }

    // Write character to transmit buffer
    U0THR = c;
}

void uart_puts(const char *str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

void uart_write(const uint8_t *data, uint32_t len) {
    while (len--) {
        uart_putchar(*data++);
    }
}

// Usage:
int main(void) {
    uart_init(115200);

    uart_puts("Hello, World!\r\n");

    uint8_t data[] = { 0x01, 0x02, 0x03, 0x04 };
    uart_write(data, sizeof(data));
}
```

### Non-Blocking Transmit Check

```c
// ============================================
// Non-blocking transmit
// ============================================

// Check if we can transmit
uint8_t uart_tx_ready(void) {
    return (U0LSR & (1 << 5)) ? 1 : 0;  // THRE bit
}

// Try to send, return 1 if sent, 0 if buffer full
uint8_t uart_try_putchar(char c) {
    if (uart_tx_ready()) {
        U0THR = c;
        return 1;
    }
    return 0;
}

// Wait with timeout
uint8_t uart_putchar_timeout(char c, uint32_t timeout_ms) {
    uint32_t start = get_ticks();
    while (!uart_tx_ready()) {
        if ((get_ticks() - start) >= timeout_ms) {
            return 0;  // Timeout
        }
    }
    U0THR = c;
    return 1;
}
```

### Formatted Output

```c
// ============================================
// Print integers and hex values
// ============================================

void uart_print_dec(int32_t value) {
    char buffer[12];
    char *ptr = buffer + 11;
    uint8_t negative = 0;

    *ptr = '\0';

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    if (value == 0) {
        *--ptr = '0';
    } else {
        while (value > 0) {
            *--ptr = '0' + (value % 10);
            value /= 10;
        }
    }

    if (negative) {
        *--ptr = '-';
    }

    uart_puts(ptr);
}

void uart_print_hex(uint32_t value, uint8_t digits) {
    const char hex_chars[] = "0123456789ABCDEF";

    uart_puts("0x");

    for (int i = digits - 1; i >= 0; i--) {
        uart_putchar(hex_chars[(value >> (i * 4)) & 0x0F]);
    }
}

void uart_print_bin(uint8_t value) {
    uart_puts("0b");
    for (int i = 7; i >= 0; i--) {
        uart_putchar((value & (1 << i)) ? '1' : '0');
    }
}

// Usage:
int main(void) {
    uart_init(115200);

    uart_puts("Counter: ");
    uart_print_dec(12345);
    uart_puts("\r\n");

    uart_puts("Address: ");
    uart_print_hex(0x50000000, 8);
    uart_puts("\r\n");

    uart_puts("Flags: ");
    uart_print_bin(0xA5);
    uart_puts("\r\n");
}

// Output:
// Counter: 12345
// Address: 0x50000000
// Flags: 0b10100101
```

---

## Receiving Data

### Polling-Based Receive

```c
// ============================================
// Simple polling receive (blocking)
// ============================================

char uart_getchar(void) {
    // Wait until data is available
    while (!(U0LSR & (1 << 0))) {
        // Wait for RDR (Receiver Data Ready)
    }

    // Read and return the character
    return U0RBR;
}

// Check if data is available
uint8_t uart_rx_available(void) {
    return (U0LSR & (1 << 0)) ? 1 : 0;
}

// Non-blocking receive
int16_t uart_try_getchar(void) {
    if (uart_rx_available()) {
        return U0RBR;
    }
    return -1;  // No data
}

// Receive with timeout
int16_t uart_getchar_timeout(uint32_t timeout_ms) {
    uint32_t start = get_ticks();

    while (!uart_rx_available()) {
        if ((get_ticks() - start) >= timeout_ms) {
            return -1;  // Timeout
        }
    }

    return U0RBR;
}
```

### Reading Strings

```c
// ============================================
// Read a line of text (until newline)
// ============================================

uint32_t uart_getline(char *buffer, uint32_t max_len) {
    uint32_t count = 0;

    while (count < max_len - 1) {
        char c = uart_getchar();

        // Echo character back (optional)
        uart_putchar(c);

        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");  // Echo newline
            break;
        }

        if (c == '\b' || c == 0x7F) {  // Backspace or DEL
            if (count > 0) {
                count--;
                uart_puts("\b \b");  // Erase character on terminal
            }
            continue;
        }

        buffer[count++] = c;
    }

    buffer[count] = '\0';
    return count;
}

// Read exact number of bytes
void uart_read(uint8_t *buffer, uint32_t len) {
    while (len--) {
        *buffer++ = uart_getchar();
    }
}

// Usage:
int main(void) {
    char input[64];

    uart_init(115200);
    uart_puts("Enter your name: ");

    uart_getline(input, sizeof(input));

    uart_puts("Hello, ");
    uart_puts(input);
    uart_puts("!\r\n");
}
```

---

## Interrupt-Driven UART

### Why Use Interrupts?

```
POLLING:
┌─────────────────────────────────────────────────────────┐
│  while (1) {                                            │
│      if (uart_rx_available()) {  // Check constantly    │
│          c = uart_getchar();                            │
│      }                                                   │
│      // Do other work                                   │
│  }                                                       │
│                                                          │
│  Problems:                                               │
│  - Wastes CPU cycles checking                           │
│  - May miss data if doing other work                    │
│  - Can't do true multitasking                           │
└─────────────────────────────────────────────────────────┘

INTERRUPTS:
┌─────────────────────────────────────────────────────────┐
│  // ISR runs automatically when data arrives            │
│  void UART_IRQHandler(void) {                           │
│      c = U0RBR;  // Read data                           │
│      buffer_write(c);                                   │
│  }                                                       │
│                                                          │
│  while (1) {                                            │
│      // Do other work freely                            │
│      // Data is collected in background                 │
│  }                                                       │
│                                                          │
│  Benefits:                                               │
│  - CPU free for other tasks                             │
│  - Never miss incoming data                             │
│  - True multitasking                                    │
└─────────────────────────────────────────────────────────┘
```

### Interrupt Setup

```c
// ============================================
// Interrupt-driven UART initialization
// ============================================

#define NVIC_ISER (*((volatile uint32_t *)0xE000E100))

void uart_init_interrupt(uint32_t baud_rate) {
    // Basic init (same as before)
    SYSAHBCLKCTRL |= (1 << 12);
    IOCON_PIO1_6 = 0x01;
    IOCON_PIO1_7 = 0x01;
    UARTCLKDIV = 1;

    U0LCR = 0x80;
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud_rate);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;
    U0LCR = 0x03;
    U0FCR = 0x07;

    // Enable receive interrupt
    U0IER = (1 << 0);  // RBRIE - Receive data available interrupt

    // Enable UART interrupt in NVIC (UART is IRQ 21)
    NVIC_ISER = (1 << 21);
}
```

### Simple Interrupt Handler

```c
// ============================================
// Basic interrupt handler
// ============================================

volatile char rx_char;
volatile uint8_t rx_flag = 0;

void UART_IRQHandler(void) {
    uint32_t iir = U0IIR;

    // Check interrupt source
    if ((iir & 0x0E) == 0x04) {  // RDA interrupt (Receive Data Available)
        rx_char = U0RBR;
        rx_flag = 1;
    }
    else if ((iir & 0x0E) == 0x0C) {  // CTI interrupt (Character Timeout)
        rx_char = U0RBR;
        rx_flag = 1;
    }
}

int main(void) {
    uart_init_interrupt(115200);
    __enable_irq();

    uart_puts("Echo test - type something:\r\n");

    while (1) {
        if (rx_flag) {
            rx_flag = 0;
            uart_putchar(rx_char);  // Echo back
        }
        // Can do other work here
    }
}
```

---

## Ring Buffers

### What is a Ring Buffer?

A **ring buffer** (circular buffer) is a fixed-size buffer that wraps around. Essential for UART because data arrives asynchronously.

```
Ring Buffer Structure:

      Write pointer (head)
              ↓
   ┌───┬───┬───┬───┬───┬───┬───┬───┐
   │ A │ B │ C │   │   │   │ X │ Y │
   └───┴───┴───┴───┴───┴───┴───┴───┘
                           ↑
                 Read pointer (tail)

After writing 'D':
                  ↓
   ┌───┬───┬───┬───┬───┬───┬───┬───┐
   │ A │ B │ C │ D │   │   │ X │ Y │
   └───┴───┴───┴───┴───┴───┴───┴───┘
                           ↑

After reading (returns 'X'):
                  ↓
   ┌───┬───┬───┬───┬───┬───┬───┬───┐
   │ A │ B │ C │ D │   │   │   │ Y │
   └───┴───┴───┴───┴───┴───┴───┴───┘
                               ↑

Wrap around:
   ↓ (wraps from end to beginning)
   ┌───┬───┬───┬───┬───┬───┬───┬───┐
   │   │ B │ C │ D │ E │ F │ G │ H │
   └───┴───┴───┴───┴───┴───┴───┴───┘
       ↑

Key: head == tail means empty (or full, depending on implementation)
```

### Ring Buffer Implementation

```c
// ============================================
// Ring Buffer for UART
// ============================================

#define RX_BUFFER_SIZE 64  // Must be power of 2 for fast modulo
#define TX_BUFFER_SIZE 64

typedef struct {
    volatile uint8_t buffer[RX_BUFFER_SIZE];
    volatile uint16_t head;  // Write position
    volatile uint16_t tail;  // Read position
} RingBuffer;

RingBuffer rx_buffer = { {0}, 0, 0 };
RingBuffer tx_buffer = { {0}, 0, 0 };

// Check if buffer is empty
uint8_t ring_empty(RingBuffer *rb) {
    return (rb->head == rb->tail);
}

// Check if buffer is full
uint8_t ring_full(RingBuffer *rb) {
    return (((rb->head + 1) & (RX_BUFFER_SIZE - 1)) == rb->tail);
}

// Get number of bytes in buffer
uint16_t ring_count(RingBuffer *rb) {
    return (rb->head - rb->tail) & (RX_BUFFER_SIZE - 1);
}

// Write byte to buffer (returns 1 on success, 0 if full)
uint8_t ring_write(RingBuffer *rb, uint8_t data) {
    if (ring_full(rb)) {
        return 0;  // Buffer full
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) & (RX_BUFFER_SIZE - 1);
    return 1;
}

// Read byte from buffer (returns -1 if empty)
int16_t ring_read(RingBuffer *rb) {
    if (ring_empty(rb)) {
        return -1;  // Buffer empty
    }
    uint8_t data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & (RX_BUFFER_SIZE - 1);
    return data;
}

// Peek at next byte without removing
int16_t ring_peek(RingBuffer *rb) {
    if (ring_empty(rb)) {
        return -1;
    }
    return rb->buffer[rb->tail];
}

// Clear buffer
void ring_clear(RingBuffer *rb) {
    rb->head = rb->tail = 0;
}
```

### UART with Ring Buffers

```c
// ============================================
// Complete interrupt-driven UART with ring buffers
// ============================================

volatile uint8_t tx_busy = 0;

void UART_IRQHandler(void) {
    uint32_t iir = U0IIR;
    uint32_t int_id = (iir >> 1) & 0x07;

    switch (int_id) {
        case 0x02:  // RDA - Receive Data Available
        case 0x06:  // CTI - Character Timeout
            // Read all available data
            while (U0LSR & (1 << 0)) {
                uint8_t c = U0RBR;
                ring_write(&rx_buffer, c);
            }
            break;

        case 0x01:  // THRE - Transmit Holding Register Empty
            if (!ring_empty(&tx_buffer)) {
                // Send next byte from TX buffer
                U0THR = ring_read(&tx_buffer);
            } else {
                // No more data to send
                tx_busy = 0;
                // Disable THRE interrupt
                U0IER &= ~(1 << 1);
            }
            break;

        case 0x03:  // RLS - Receive Line Status (errors)
            // Clear errors by reading LSR
            (void)U0LSR;
            break;
    }
}

void uart_init_buffered(uint32_t baud_rate) {
    // Standard init
    SYSAHBCLKCTRL |= (1 << 12);
    IOCON_PIO1_6 = 0x01;
    IOCON_PIO1_7 = 0x01;
    UARTCLKDIV = 1;

    U0LCR = 0x80;
    uint32_t divisor = SYSTEM_CLOCK / (16 * baud_rate);
    U0DLL = divisor & 0xFF;
    U0DLM = (divisor >> 8) & 0xFF;
    U0LCR = 0x03;
    U0FCR = 0x07;

    // Clear buffers
    ring_clear(&rx_buffer);
    ring_clear(&tx_buffer);

    // Enable RX interrupt only (TX enabled when needed)
    U0IER = (1 << 0);

    NVIC_ISER = (1 << 21);  // UART IRQ
}

// ============================================
// User-facing API
// ============================================

// Check if data available
uint16_t uart_available(void) {
    return ring_count(&rx_buffer);
}

// Read one byte (blocking)
char uart_read_byte(void) {
    while (ring_empty(&rx_buffer)) {
        // Wait for data
    }
    return ring_read(&rx_buffer);
}

// Read one byte (non-blocking, returns -1 if no data)
int16_t uart_read_byte_nonblocking(void) {
    return ring_read(&rx_buffer);
}

// Write one byte
void uart_write_byte(uint8_t c) {
    // Wait if TX buffer is full
    while (ring_full(&tx_buffer)) {
        // Could add timeout here
    }

    // Add to buffer
    ring_write(&tx_buffer, c);

    // Start transmission if not already running
    if (!tx_busy) {
        tx_busy = 1;
        // Enable THRE interrupt to start sending
        U0IER |= (1 << 1);
    }
}

// Write string
void uart_write_string(const char *str) {
    while (*str) {
        uart_write_byte(*str++);
    }
}

// Write buffer
void uart_write_buffer(const uint8_t *data, uint32_t len) {
    while (len--) {
        uart_write_byte(*data++);
    }
}

// Flush TX buffer (wait for all data to be sent)
void uart_flush(void) {
    while (!ring_empty(&tx_buffer) || tx_busy) {
        // Wait
    }
}
```

---

## Printf Implementation

### Simple Printf for UART

```c
// ============================================
// Minimal printf implementation
// ============================================

#include <stdarg.h>

void uart_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i': {
                    int32_t val = va_arg(args, int32_t);
                    uart_print_dec(val);
                    break;
                }
                case 'u': {
                    uint32_t val = va_arg(args, uint32_t);
                    uart_print_unsigned(val);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t val = va_arg(args, uint32_t);
                    uart_print_hex(val, 8);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putchar(c);
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char *);
                    uart_puts(s);
                    break;
                }
                case '%':
                    uart_putchar('%');
                    break;
                default:
                    uart_putchar('%');
                    uart_putchar(*fmt);
                    break;
            }
        } else {
            uart_putchar(*fmt);
        }
        fmt++;
    }

    va_end(args);
}

void uart_print_unsigned(uint32_t value) {
    char buffer[12];
    char *ptr = buffer + 11;
    *ptr = '\0';

    if (value == 0) {
        *--ptr = '0';
    } else {
        while (value > 0) {
            *--ptr = '0' + (value % 10);
            value /= 10;
        }
    }
    uart_puts(ptr);
}

// Usage:
int main(void) {
    uart_init(115200);

    int count = 42;
    char *name = "LPC1343";
    uint32_t addr = 0x50000000;

    uart_printf("Hello from %s!\r\n", name);
    uart_printf("Counter value: %d\r\n", count);
    uart_printf("Register address: 0x%x\r\n", addr);
}

// Output:
// Hello from LPC1343!
// Counter value: 42
// Register address: 0x50000000
```

### Retargeting Standard printf (Newlib)

```c
// ============================================
// Retarget printf to UART (for GCC/Newlib)
// ============================================

#include <stdio.h>
#include <sys/stat.h>

// Required by newlib for printf
int _write(int file, char *ptr, int len) {
    (void)file;  // Unused, suppress warning

    for (int i = 0; i < len; i++) {
        uart_putchar(ptr[i]);
    }
    return len;
}

// Required by newlib for scanf
int _read(int file, char *ptr, int len) {
    (void)file;

    for (int i = 0; i < len; i++) {
        ptr[i] = uart_getchar();
        // Echo and handle backspace if desired
    }
    return len;
}

// Other stubs that newlib might need
int _close(int file) { (void)file; return -1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }

// Usage:
#include <stdio.h>

int main(void) {
    uart_init(115200);

    // Now standard printf works!
    printf("Hello, World!\r\n");
    printf("Value: %d, Hex: 0x%08X\r\n", 123, 0xABCD);

    float f = 3.14159;
    printf("Float: %.2f\r\n", f);  // Requires -u _printf_float linker flag
}
```

---

## Error Handling

### UART Errors

```
Common UART Errors:

┌──────────────────────────────────────────────────────────────┐
│ OVERRUN ERROR (OE)                                           │
│ - New data arrived before previous was read                 │
│ - Data lost!                                                 │
│ - Cause: CPU too slow or interrupts disabled too long       │
│ - Fix: Use larger FIFO trigger, faster processing           │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ FRAMING ERROR (FE)                                           │
│ - Stop bit was 0 instead of 1                               │
│ - Cause: Baud rate mismatch, noise, wrong config            │
│ - Fix: Verify baud rate and line settings match             │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ PARITY ERROR (PE)                                            │
│ - Parity bit didn't match data                              │
│ - Cause: Noise, baud rate mismatch                          │
│ - Fix: Check connections, verify parity settings            │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│ BREAK CONDITION (BI)                                         │
│ - RX line held LOW for longer than a frame                  │
│ - Sometimes intentional (used as attention signal)          │
│ - Fix: Usually just ignore and resync                       │
└──────────────────────────────────────────────────────────────┘
```

### Error Detection Code

```c
// ============================================
// Error checking and handling
// ============================================

typedef struct {
    uint32_t overrun_errors;
    uint32_t framing_errors;
    uint32_t parity_errors;
    uint32_t break_conditions;
    uint32_t bytes_received;
    uint32_t bytes_transmitted;
} UartStats;

volatile UartStats uart_stats = {0};

void uart_check_errors(void) {
    uint32_t lsr = U0LSR;

    if (lsr & (1 << 1)) {  // Overrun Error
        uart_stats.overrun_errors++;
    }
    if (lsr & (1 << 2)) {  // Parity Error
        uart_stats.parity_errors++;
    }
    if (lsr & (1 << 3)) {  // Framing Error
        uart_stats.framing_errors++;
    }
    if (lsr & (1 << 4)) {  // Break Interrupt
        uart_stats.break_conditions++;
    }
}

// Enhanced interrupt handler with error checking
void UART_IRQHandler(void) {
    uint32_t iir = U0IIR;
    uint32_t int_id = (iir >> 1) & 0x07;

    if (int_id == 0x03) {  // RLS - Line Status interrupt
        uint32_t lsr = U0LSR;

        if (lsr & (1 << 1)) {
            uart_stats.overrun_errors++;
            // Data was lost - nothing we can do to recover
        }
        if (lsr & (1 << 2)) {
            uart_stats.parity_errors++;
            // Read and discard bad byte
            (void)U0RBR;
        }
        if (lsr & (1 << 3)) {
            uart_stats.framing_errors++;
            // Read and discard bad byte
            (void)U0RBR;
        }
        if (lsr & (1 << 4)) {
            uart_stats.break_conditions++;
            // Break detected - clear by reading RBR
            (void)U0RBR;
        }
        return;
    }

    // Normal RX/TX handling...
    if (int_id == 0x02 || int_id == 0x06) {
        while (U0LSR & (1 << 0)) {
            uint8_t c = U0RBR;
            ring_write(&rx_buffer, c);
            uart_stats.bytes_received++;
        }
    }
}

void uart_print_stats(void) {
    uart_printf("UART Statistics:\r\n");
    uart_printf("  Bytes RX: %u\r\n", uart_stats.bytes_received);
    uart_printf("  Bytes TX: %u\r\n", uart_stats.bytes_transmitted);
    uart_printf("  Overrun errors: %u\r\n", uart_stats.overrun_errors);
    uart_printf("  Framing errors: %u\r\n", uart_stats.framing_errors);
    uart_printf("  Parity errors: %u\r\n", uart_stats.parity_errors);
    uart_printf("  Break conditions: %u\r\n", uart_stats.break_conditions);
}
```

---

## Practical Examples

### Example 1: Command Line Interface

```c
// ============================================
// Simple command interpreter
// ============================================

#define CMD_BUFFER_SIZE 64

void process_command(char *cmd) {
    // Remove trailing whitespace
    char *end = cmd + strlen(cmd) - 1;
    while (end > cmd && (*end == ' ' || *end == '\r' || *end == '\n')) {
        *end-- = '\0';
    }

    if (strcmp(cmd, "help") == 0) {
        uart_puts("Available commands:\r\n");
        uart_puts("  help     - Show this help\r\n");
        uart_puts("  led on   - Turn LED on\r\n");
        uart_puts("  led off  - Turn LED off\r\n");
        uart_puts("  status   - Show system status\r\n");
        uart_puts("  reset    - Reset system\r\n");
    }
    else if (strcmp(cmd, "led on") == 0) {
        LED0_ON();
        uart_puts("LED is ON\r\n");
    }
    else if (strcmp(cmd, "led off") == 0) {
        LED0_OFF();
        uart_puts("LED is OFF\r\n");
    }
    else if (strcmp(cmd, "status") == 0) {
        uart_puts("System Status:\r\n");
        uart_printf("  Uptime: %u ms\r\n", get_ticks());
        uart_printf("  LED: %s\r\n", (GPIO3DATA & (1<<0)) ? "OFF" : "ON");
    }
    else if (strcmp(cmd, "reset") == 0) {
        uart_puts("Resetting...\r\n");
        uart_flush();
        NVIC_SystemReset();
    }
    else if (strlen(cmd) > 0) {
        uart_printf("Unknown command: '%s'\r\n", cmd);
        uart_puts("Type 'help' for available commands.\r\n");
    }
}

void command_loop(void) {
    char cmd_buffer[CMD_BUFFER_SIZE];

    uart_puts("\r\n");
    uart_puts("LPC1343 Command Interface\r\n");
    uart_puts("Type 'help' for commands.\r\n");

    while (1) {
        uart_puts("> ");

        uart_getline(cmd_buffer, sizeof(cmd_buffer));
        process_command(cmd_buffer);
    }
}

int main(void) {
    init_leds();
    uart_init(115200);
    __enable_irq();

    command_loop();
}
```

### Example 2: Data Logger

```c
// ============================================
// Simple data logger with timestamps
// ============================================

void log_data(const char *tag, int32_t value) {
    uart_printf("[%08u] %s: %d\r\n", get_ticks(), tag, value);
}

void log_hex(const char *tag, uint32_t value) {
    uart_printf("[%08u] %s: 0x%08X\r\n", get_ticks(), tag, value);
}

void log_string(const char *tag, const char *message) {
    uart_printf("[%08u] %s: %s\r\n", get_ticks(), tag, message);
}

void log_buffer(const char *tag, uint8_t *data, uint32_t len) {
    uart_printf("[%08u] %s: ", get_ticks(), tag);
    for (uint32_t i = 0; i < len; i++) {
        uart_printf("%02X ", data[i]);
    }
    uart_puts("\r\n");
}

// Usage:
int main(void) {
    uart_init(115200);

    log_string("BOOT", "System starting");
    log_hex("CFG", SYSAHBCLKCTRL);

    int sensor_value = 0;
    while (1) {
        sensor_value = read_adc();  // Hypothetical
        log_data("ADC", sensor_value);
        delay_ms(1000);
    }
}

// Output:
// [00000001] BOOT: System starting
// [00000002] CFG: 0x0001005F
// [00001003] ADC: 2048
// [00002004] ADC: 2051
// ...
```

### Example 3: GPS NMEA Parser

```c
// ============================================
// Parse GPS NMEA sentences
// ============================================

#define NMEA_MAX_LEN 100

char nmea_buffer[NMEA_MAX_LEN];
uint8_t nmea_index = 0;
uint8_t nmea_ready = 0;

// Called from UART RX interrupt or polling
void nmea_process_char(char c) {
    if (c == '$') {
        // Start of new sentence
        nmea_index = 0;
        nmea_buffer[nmea_index++] = c;
    }
    else if (c == '\r' || c == '\n') {
        // End of sentence
        if (nmea_index > 0) {
            nmea_buffer[nmea_index] = '\0';
            nmea_ready = 1;
        }
    }
    else if (nmea_index < NMEA_MAX_LEN - 1) {
        nmea_buffer[nmea_index++] = c;
    }
}

// Parse $GPGGA sentence for position
typedef struct {
    float latitude;
    float longitude;
    char lat_dir;
    char lon_dir;
    uint8_t fix_quality;
    uint8_t satellites;
} GpsPosition;

uint8_t parse_gpgga(const char *sentence, GpsPosition *pos) {
    if (strncmp(sentence, "$GPGGA", 6) != 0) {
        return 0;
    }

    // Simple parsing - production code should be more robust
    char *ptr = (char *)sentence;
    int field = 0;

    while (*ptr && field < 10) {
        if (*ptr == ',') {
            field++;
            ptr++;

            switch (field) {
                case 2:  // Latitude
                    pos->latitude = atof(ptr);
                    break;
                case 3:  // N/S
                    pos->lat_dir = *ptr;
                    break;
                case 4:  // Longitude
                    pos->longitude = atof(ptr);
                    break;
                case 5:  // E/W
                    pos->lon_dir = *ptr;
                    break;
                case 6:  // Fix quality
                    pos->fix_quality = atoi(ptr);
                    break;
                case 7:  // Satellites
                    pos->satellites = atoi(ptr);
                    break;
            }
        } else {
            ptr++;
        }
    }

    return 1;
}

int main(void) {
    GpsPosition gps;

    uart_init(9600);  // GPS typically at 9600 baud
    __enable_irq();

    while (1) {
        // Process incoming GPS data
        while (uart_available()) {
            nmea_process_char(uart_read_byte());
        }

        if (nmea_ready) {
            nmea_ready = 0;

            if (parse_gpgga(nmea_buffer, &gps)) {
                // Debug output (to second UART or debug port)
                // printf("Lat: %.4f %c, Lon: %.4f %c, Sats: %d\n",
                //        gps.latitude, gps.lat_dir,
                //        gps.longitude, gps.lon_dir,
                //        gps.satellites);
            }
        }
    }
}
```

### Example 4: Binary Protocol

```c
// ============================================
// Simple binary protocol with checksums
// ============================================

// Protocol format:
// [SYNC1] [SYNC2] [LENGTH] [CMD] [DATA...] [CHECKSUM]
// 0xAA    0x55    N        cmd   N-2 bytes  XOR of all

#define SYNC1 0xAA
#define SYNC2 0x55

typedef enum {
    PROTO_IDLE,
    PROTO_SYNC2,
    PROTO_LENGTH,
    PROTO_DATA,
    PROTO_CHECKSUM
} ProtoState;

typedef struct {
    uint8_t cmd;
    uint8_t data[32];
    uint8_t length;
} Packet;

ProtoState proto_state = PROTO_IDLE;
Packet rx_packet;
uint8_t rx_index = 0;
uint8_t rx_checksum = 0;

void protocol_process_byte(uint8_t byte) {
    switch (proto_state) {
        case PROTO_IDLE:
            if (byte == SYNC1) {
                proto_state = PROTO_SYNC2;
            }
            break;

        case PROTO_SYNC2:
            if (byte == SYNC2) {
                proto_state = PROTO_LENGTH;
                rx_checksum = 0;
            } else {
                proto_state = PROTO_IDLE;
            }
            break;

        case PROTO_LENGTH:
            rx_packet.length = byte;
            rx_checksum ^= byte;
            rx_index = 0;
            proto_state = PROTO_DATA;
            break;

        case PROTO_DATA:
            rx_checksum ^= byte;
            if (rx_index == 0) {
                rx_packet.cmd = byte;
            } else {
                rx_packet.data[rx_index - 1] = byte;
            }
            rx_index++;
            if (rx_index >= rx_packet.length) {
                proto_state = PROTO_CHECKSUM;
            }
            break;

        case PROTO_CHECKSUM:
            if (byte == rx_checksum) {
                // Valid packet received!
                handle_packet(&rx_packet);
            }
            proto_state = PROTO_IDLE;
            break;
    }
}

void send_packet(uint8_t cmd, uint8_t *data, uint8_t len) {
    uint8_t checksum = 0;

    uart_write_byte(SYNC1);
    uart_write_byte(SYNC2);

    uint8_t total_len = len + 1;  // +1 for cmd
    uart_write_byte(total_len);
    checksum ^= total_len;

    uart_write_byte(cmd);
    checksum ^= cmd;

    for (uint8_t i = 0; i < len; i++) {
        uart_write_byte(data[i]);
        checksum ^= data[i];
    }

    uart_write_byte(checksum);
}

void handle_packet(Packet *pkt) {
    switch (pkt->cmd) {
        case 0x01:  // Read status
            {
                uint8_t response[4];
                response[0] = GPIO3DATA & 0xFF;
                response[1] = (get_ticks() >> 0) & 0xFF;
                response[2] = (get_ticks() >> 8) & 0xFF;
                response[3] = (get_ticks() >> 16) & 0xFF;
                send_packet(0x81, response, 4);
            }
            break;

        case 0x02:  // Set LED
            if (pkt->data[0]) {
                LED0_ON();
            } else {
                LED0_OFF();
            }
            send_packet(0x82, NULL, 0);  // ACK
            break;

        default:
            // Unknown command - send NACK
            send_packet(0xFF, &pkt->cmd, 1);
            break;
    }
}
```

---

## Debugging with Serial

### Debug Macros

```c
// ============================================
// Debug output macros
// ============================================

// Set debug level: 0=off, 1=errors, 2=warnings, 3=info, 4=debug
#define DEBUG_LEVEL 4

#if DEBUG_LEVEL >= 1
    #define DEBUG_ERROR(fmt, ...) uart_printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define DEBUG_ERROR(fmt, ...)
#endif

#if DEBUG_LEVEL >= 2
    #define DEBUG_WARN(fmt, ...) uart_printf("[WARN]  " fmt "\r\n", ##__VA_ARGS__)
#else
    #define DEBUG_WARN(fmt, ...)
#endif

#if DEBUG_LEVEL >= 3
    #define DEBUG_INFO(fmt, ...) uart_printf("[INFO]  " fmt "\r\n", ##__VA_ARGS__)
#else
    #define DEBUG_INFO(fmt, ...)
#endif

#if DEBUG_LEVEL >= 4
    #define DEBUG_DEBUG(fmt, ...) uart_printf("[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define DEBUG_DEBUG(fmt, ...)
#endif

// Usage:
void some_function(int value) {
    DEBUG_DEBUG("Entering some_function with value=%d", value);

    if (value < 0) {
        DEBUG_ERROR("Invalid negative value: %d", value);
        return;
    }

    if (value > 100) {
        DEBUG_WARN("Value %d exceeds recommended maximum", value);
    }

    DEBUG_INFO("Processing value %d", value);

    // ... do work ...

    DEBUG_DEBUG("Exiting some_function");
}
```

### Hex Dump Function

```c
// ============================================
// Memory hex dump for debugging
// ============================================

void hex_dump(const char *label, const void *data, uint32_t len) {
    const uint8_t *bytes = (const uint8_t *)data;

    uart_printf("\r\n%s (%u bytes):\r\n", label, len);

    for (uint32_t i = 0; i < len; i += 16) {
        // Address
        uart_printf("%08X: ", (uint32_t)(bytes + i));

        // Hex bytes
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < len) {
                uart_printf("%02X ", bytes[i + j]);
            } else {
                uart_puts("   ");
            }
            if (j == 7) uart_putchar(' ');  // Extra space in middle
        }

        uart_puts(" |");

        // ASCII
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            char c = bytes[i + j];
            uart_putchar((c >= 32 && c < 127) ? c : '.');
        }

        uart_puts("|\r\n");
    }
}

// Usage:
uint8_t buffer[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x00, 0x57, 0x6F,
                    0x72, 0x6C, 0x64, 0x21, 0x00, 0xFF, 0xAB, 0xCD};
hex_dump("Test Buffer", buffer, sizeof(buffer));

// Output:
// Test Buffer (16 bytes):
// 00000000: 48 65 6C 6C 6F 00 57 6F  72 6C 64 21 00 FF AB CD |Hello.World!....|
```

### Register Dump

```c
// ============================================
// Dump peripheral registers
// ============================================

void dump_gpio_registers(uint8_t port) {
    volatile uint32_t *base;
    const char *name;

    switch (port) {
        case 0: base = (uint32_t *)0x50000000; name = "GPIO0"; break;
        case 1: base = (uint32_t *)0x50010000; name = "GPIO1"; break;
        case 2: base = (uint32_t *)0x50020000; name = "GPIO2"; break;
        case 3: base = (uint32_t *)0x50030000; name = "GPIO3"; break;
        default: return;
    }

    uart_printf("\r\n%s Registers:\r\n", name);
    uart_printf("  DATA: 0x%08X\r\n", *(base + 0x3FFC/4));
    uart_printf("  DIR:  0x%08X\r\n", *(base + 0x8000/4));
    uart_printf("  IS:   0x%08X\r\n", *(base + 0x8004/4));
    uart_printf("  IBE:  0x%08X\r\n", *(base + 0x8008/4));
    uart_printf("  IEV:  0x%08X\r\n", *(base + 0x800C/4));
    uart_printf("  IE:   0x%08X\r\n", *(base + 0x8010/4));
    uart_printf("  RIS:  0x%08X\r\n", *(base + 0x8014/4));
    uart_printf("  MIS:  0x%08X\r\n", *(base + 0x8018/4));
}

void dump_uart_registers(void) {
    uart_puts("\r\nUART Registers:\r\n");

    // Save DLAB state
    uint8_t lcr = U0LCR;

    uart_printf("  LCR: 0x%02X\r\n", lcr);
    uart_printf("  LSR: 0x%02X\r\n", U0LSR);
    uart_printf("  IER: 0x%02X\r\n", U0IER);
    uart_printf("  IIR: 0x%02X\r\n", U0IIR);

    // Read divisor with DLAB=1
    U0LCR |= 0x80;
    uart_printf("  DLL: 0x%02X\r\n", U0DLL);
    uart_printf("  DLM: 0x%02X\r\n", U0DLM);
    U0LCR = lcr;  // Restore

    uart_printf("  FDR: 0x%02X\r\n", U0FDR);
}
```

---

## Common Patterns

### Pattern 1: UART Initialization Template

```c
void uart_template_init(uint32_t baud) {
    // 1. Enable UART peripheral clock
    SYSAHBCLKCTRL |= (1 << 12);

    // 2. Configure UART pins
    IOCON_PIO1_6 = 0x01;  // RXD
    IOCON_PIO1_7 = 0x01;  // TXD

    // 3. Set UART clock divider
    UARTCLKDIV = 1;

    // 4. Configure baud rate
    U0LCR = 0x80;  // DLAB=1
    uint32_t div = SYSTEM_CLOCK / (16 * baud);
    U0DLL = div & 0xFF;
    U0DLM = (div >> 8) & 0xFF;

    // 5. Set line format (8N1)
    U0LCR = 0x03;  // 8 bits, no parity, 1 stop, DLAB=0

    // 6. Enable FIFOs
    U0FCR = 0x07;  // Enable and reset FIFOs

    // 7. (Optional) Enable interrupts
    // U0IER = 0x01;  // RX interrupt
    // NVIC_ISER = (1 << 21);  // UART IRQ
}
```

### Pattern 2: Safe String Transmission

```c
// Send string with maximum length protection
void uart_puts_n(const char *str, uint32_t max_len) {
    uint32_t count = 0;
    while (*str && count < max_len) {
        uart_putchar(*str++);
        count++;
    }
}

// Send string with timeout
uint8_t uart_puts_timeout(const char *str, uint32_t timeout_ms) {
    uint32_t start = get_ticks();
    while (*str) {
        while (!(U0LSR & (1 << 5))) {
            if ((get_ticks() - start) >= timeout_ms) {
                return 0;  // Timeout
            }
        }
        U0THR = *str++;
    }
    return 1;  // Success
}
```

### Pattern 3: Line-Based Protocol Handler

```c
#define LINE_MAX 128

typedef void (*LineHandler)(const char *line);

void line_protocol_loop(LineHandler handler) {
    char line[LINE_MAX];
    uint32_t index = 0;

    while (1) {
        if (uart_available()) {
            char c = uart_read_byte();

            if (c == '\r' || c == '\n') {
                if (index > 0) {
                    line[index] = '\0';
                    handler(line);
                    index = 0;
                }
            } else if (index < LINE_MAX - 1) {
                line[index++] = c;
            }
        }

        // Can do other processing here
    }
}

// Usage:
void my_handler(const char *line) {
    uart_printf("Received: %s\r\n", line);
}

int main(void) {
    uart_init(115200);
    line_protocol_loop(my_handler);
}
```

---

## Troubleshooting

### Problem: No output on terminal

**Check:**
1. Baud rate matches on both ends
2. TX/RX pins are swapped correctly (TX→RX, RX→TX)
3. Ground connected between devices
4. Correct COM port selected in terminal
5. UART clock enabled
6. Pins configured for UART function
7. Terminal set to correct settings (8N1)

```c
// Diagnostic: Send continuous characters
void uart_test_output(void) {
    while (1) {
        for (char c = 'A'; c <= 'Z'; c++) {
            uart_putchar(c);
        }
        uart_puts("\r\n");
        delay_ms(1000);
    }
}
```

### Problem: Garbage characters

**Likely causes:**
1. Baud rate mismatch (most common)
2. Different line settings (parity, stop bits)
3. Wrong voltage levels (3.3V vs 5V)
4. Noise on the line

```c
// Diagnostic: Send known pattern
void uart_test_pattern(void) {
    // Send 0x55 = 01010101 - creates regular square wave
    while (1) {
        uart_putchar(0x55);
        delay_ms(10);
    }
}
// Measure with oscilloscope - should see clean square wave at baud rate
```

### Problem: Missing characters

**Likely causes:**
1. Buffer overflow (data arriving faster than processed)
2. Interrupts disabled too long
3. FIFO not enabled

**Solutions:**
- Use interrupt-driven receive with ring buffer
- Enable and use hardware FIFOs
- Increase buffer sizes
- Use hardware flow control (if available)

### Problem: Receive works, transmit doesn't

**Check:**
1. TX pin configuration (IOCON)
2. THRE bit being checked before write
3. TX enable bit (TER register)
4. Cable/connection for TX line

```c
// Force TX test
void uart_force_tx_test(void) {
    // Directly manipulate TX
    U0TER = 0x80;  // Enable transmitter
    U0THR = 'X';   // Send character

    // Check status
    uart_printf("LSR: 0x%02X\r\n", U0LSR);
}
```

---

## Quick Reference

### UART Clock Enable

```c
SYSAHBCLKCTRL |= (1 << 12);  // Enable UART clock
UARTCLKDIV = 1;               // UART clock = system clock / 1
```

### NVIC Interrupt

```c
#define NVIC_ISER (*((volatile uint32_t *)0xE000E100))
NVIC_ISER = (1 << 21);  // UART IRQ is 21
```

### Pin Configuration

```c
IOCON_PIO1_6 = 0x01;  // P1.6 = RXD
IOCON_PIO1_7 = 0x01;  // P1.7 = TXD
```

### Common Register Values

```c
// Line Control (8N1)
U0LCR = 0x03;  // 8 data bits, no parity, 1 stop bit, DLAB=0
U0LCR = 0x83;  // Same but DLAB=1 (access baud rate)

// FIFO Control
U0FCR = 0x07;  // Enable FIFOs, reset both
U0FCR = 0x87;  // Same + RX trigger at 8 bytes
U0FCR = 0xC7;  // Same + RX trigger at 14 bytes

// Interrupt Enable
U0IER = 0x01;  // RX data available only
U0IER = 0x03;  // RX and TX interrupts
U0IER = 0x07;  // RX, TX, and line status
```

### Line Status Register Bits

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | RDR | Receive Data Ready |
| 1 | OE | Overrun Error |
| 2 | PE | Parity Error |
| 3 | FE | Framing Error |
| 4 | BI | Break Interrupt |
| 5 | THRE | TX Holding Register Empty |
| 6 | TEMT | Transmitter Empty |
| 7 | RXFE | RX FIFO Error |

### Quick Code Snippets

```c
// Wait for TX ready
while (!(U0LSR & (1 << 5)));

// Check RX data available
if (U0LSR & (1 << 0)) { ... }

// Send byte
U0THR = data;

// Read byte
data = U0RBR;

// Check for errors
if (U0LSR & 0x1E) { /* error */ }

// Enable RX interrupt
U0IER |= (1 << 0);
NVIC_ISER = (1 << 21);  // UART IRQ
```

### Baud Rate Quick Reference (72 MHz)

| Baud | DLL | DLM | Divisor | Error |
|------|-----|-----|---------|-------|
| 9600 | 0xD5 | 0x01 | 469 | 0.08% |
| 19200 | 0xEA | 0x00 | 234 | 0.16% |
| 38400 | 0x75 | 0x00 | 117 | 0.16% |
| 57600 | 0x4E | 0x00 | 78 | 0.16% |
| 115200 | 0x27 | 0x00 | 39 | 1.60% |

---

## Further Practice

1. **Terminal Emulator**: Create VT100-compatible terminal output
2. **Bootloader**: Receive new firmware over UART
3. **Modbus Protocol**: Implement Modbus RTU slave
4. **Multi-UART**: Soft UART on GPIO for additional ports
5. **AT Command Parser**: Handle AT commands like modems
6. **SLIP Protocol**: Implement Serial Line IP for networking
7. **Circular Logger**: Store log messages and retrieve on command

---

## What's Next?

You've completed Chapter 5! You now understand serial communication, one of the most valuable debugging and communication tools in embedded development.

**Continue to [Chapter 6: Interrupts and Clocks](06-interrupts-and-clocks.md)** to learn:
- How interrupts make your code efficient
- Clock and PLL configuration for optimal performance
- NVIC (Nested Vector Interrupt Controller) management
- Real-time response patterns

**Or review previous chapters:**
- [Chapter 4: Timers and PWM](04-timers-and-pwm.md) - Time-based operations
- [Chapter 3: GPIO In-Depth](03-gpio-in-depth.md) - Pin control fundamentals
- [Chapter 0: Getting Started](00-getting-started.md) - First program

---

*Chapter 5 of the Embedded C Learning Series*
*Part of the LPC1343 Learning Library*
