# Chapter 3: GPIO In-Depth

A comprehensive beginner's guide to General Purpose Input/Output (GPIO) on the LPC1343 microcontroller.

---

## Chapter Overview

| | |
|---|---|
| **Prerequisites** | Chapters 0-2 (Getting Started, Bitwise Ops, Build Process) |
| **Time to Complete** | 3-4 hours |
| **Hands-On Projects** | LED control, button input, running light, combination lock |
| **You Will Learn** | How to control pins, read inputs, and handle GPIO interrupts |

---

## Quick Start: LED and Button in 30 Seconds

Here's the minimal code to control an LED and read a button:

```c
// Register addresses
#define GPIO3DIR   (*((volatile unsigned int *)0x50038000))
#define GPIO3DATA  (*((volatile unsigned int *)0x50033FFC))
#define GPIO0DIR   (*((volatile unsigned int *)0x50008000))
#define GPIO0DATA  (*((volatile unsigned int *)0x50003FFC))

void gpio_quick_example(void) {
    // LED on P3.0 (output)
    GPIO3DIR |= (1 << 0);       // Set as output
    GPIO3DATA &= ~(1 << 0);     // LED ON (active-low)
    GPIO3DATA |= (1 << 0);      // LED OFF

    // Button on P0.1 (input with external pull-up)
    GPIO0DIR &= ~(1 << 1);      // Set as input

    // Read button (active-low: pressed = 0)
    if (!(GPIO0DATA & (1 << 1))) {
        // Button is pressed
        GPIO3DATA &= ~(1 << 0); // LED ON
    }
}
```

**Key patterns:**
- `GPIOxDIR |= (1 << n)` → Set pin n as output
- `GPIOxDIR &= ~(1 << n)` → Set pin n as input
- `GPIOxDATA |= (1 << n)` → Set pin n HIGH
- `GPIOxDATA &= ~(1 << n)` → Set pin n LOW
- `if (GPIOxDATA & (1 << n))` → Check if pin n is HIGH

The rest of this chapter explains pull-up/down resistors, IOCON configuration, interrupts, debouncing, and more.

---

## Table of Contents

1. [What is GPIO?](#what-is-gpio)
2. [GPIO Hardware Basics](#gpio-hardware-basics)
3. [LPC1343 GPIO Architecture](#lpc1343-gpio-architecture)
4. [Pin Configuration (IOCON)](#pin-configuration-iocon)
5. [GPIO Registers](#gpio-registers)
6. [Output Mode: Driving LEDs and Loads](#output-mode-driving-leds-and-loads)
7. [Input Mode: Reading Buttons and Sensors](#input-mode-reading-buttons-and-sensors)
8. [GPIO Interrupts](#gpio-interrupts)
9. [Button Debouncing](#button-debouncing)
10. [Practical Examples](#practical-examples)
11. [Common Patterns and Best Practices](#common-patterns-and-best-practices)
12. [Troubleshooting](#troubleshooting)
13. [Quick Reference](#quick-reference)

---

## What is GPIO?

**GPIO** stands for **General Purpose Input/Output**. These are the physical pins on a microcontroller that you can control from software to interact with the outside world.

### The Two Fundamental Modes

```
OUTPUT MODE                          INPUT MODE
┌─────────────┐                     ┌─────────────┐
│   LPC1343   │                     │   LPC1343   │
│             │                     │             │
│  Software   │──→ Pin ──→ LED      │  Software   │←── Pin ←── Button
│  controls   │                     │  reads      │
│  voltage    │                     │  voltage    │
└─────────────┘                     └─────────────┘
   YOU control                         YOU read
   the outside                         the outside
   world                               world
```

**Output Mode:**
- Software controls the pin voltage (HIGH = 3.3V, LOW = 0V)
- Used to drive LEDs, relays, transistors, other chips
- "Push" information out to the world

**Input Mode:**
- Software reads the pin voltage
- External circuits set the voltage
- Used to read buttons, switches, sensors
- "Pull" information in from the world

### Why GPIO Matters

GPIO is the foundation of all hardware interaction. Before you can:
- Blink an LED → you need GPIO output
- Read a button → you need GPIO input
- Use SPI/I2C/UART → those are special GPIO functions
- Control motors → you need GPIO (often with PWM)

**Master GPIO first, and everything else becomes easier.**

---

## GPIO Hardware Basics

### Digital Logic Levels

GPIO pins work with **digital signals** - they're either HIGH or LOW, nothing in between (that's what ADC is for).

```
Voltage Levels for LPC1343 (3.3V logic):

3.3V ─────────────────────── HIGH (logic 1)
       │
       │  "Valid HIGH" zone
       │  (typically > 2.0V)
2.0V ─ ┼ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
       │
       │  "Undefined" zone
       │  (avoid this region!)
0.8V ─ ┼ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
       │
       │  "Valid LOW" zone
       │  (typically < 0.8V)
0.0V ─────────────────────── LOW (logic 0)
```

**Important:** Never apply more than 3.3V to a GPIO pin! The LPC1343 has limited 5V tolerance—only specific pins (like P0.4, P0.5 for I2C) are 5V tolerant. Check the datasheet for your specific pin.

### Output Driver Types

GPIO outputs can be configured in different ways:

#### Push-Pull (Default)

```
         3.3V
          │
         ┌┴┐
         │P│ ← P-channel transistor (pulls HIGH)
         └┬┘
          ├──────→ Pin Output
         ┌┴┐
         │N│ ← N-channel transistor (pulls LOW)
         └┬┘
          │
         GND

Output HIGH: P-transistor ON, N-transistor OFF → Pin connects to 3.3V
Output LOW:  P-transistor OFF, N-transistor ON → Pin connects to GND
```

**Push-Pull characteristics:**
- Can actively drive HIGH or LOW
- Strong output (can source and sink current)
- Most common mode
- Good for driving LEDs, logic inputs

#### Open-Drain

```
         3.3V
          │
         ┌┴┐
         │ │ External pull-up resistor (optional)
         └┬┘
          ├──────→ Pin Output
         ┌┴┐
         │N│ ← N-channel transistor only
         └┬┘
          │
         GND

Output LOW:  N-transistor ON → Pin connects to GND
Output HIGH: N-transistor OFF → Pin floats (needs external pull-up)
```

**Open-Drain characteristics:**
- Can only actively pull LOW
- HIGH state requires external pull-up resistor
- Used for I2C, wired-OR buses
- Allows multiple devices to share a line

### Input Circuit

```
                    ┌─────────────────┐
External    ───────→│  Input Buffer   │───→ To GPIO data register
Signal              │  (Schmitt       │      (software reads this)
                    │   trigger)      │
                    └─────────────────┘
                           │
                    ┌──────┴──────┐
                    │   Optional   │
                    │  Pull-up or  │
                    │  Pull-down   │
                    │  resistor    │
                    └─────────────┘
```

### Pull-Up and Pull-Down Resistors

When an input pin is **floating** (not connected to anything), it picks up electrical noise and gives random readings. Pull resistors solve this:

```
PULL-UP RESISTOR                    PULL-DOWN RESISTOR

    3.3V                                3.3V
      │                                   │
     ┌┴┐                                  │
     │R│ Pull-up                          │
     └┬┘ (internal or external)           │
      │                                   │
      ├────→ GPIO Pin                     ├────→ GPIO Pin
      │                                   │
    ──┴── Button to GND                 ──┴── Button to 3.3V
                                         ┌┴┐
                                         │R│ Pull-down
                                         └┬┘
                                          │
                                         GND

Button open: Pin reads HIGH            Button open: Pin reads LOW
Button pressed: Pin reads LOW          Button pressed: Pin reads HIGH
```

**When to use which:**
- **Pull-up**: Button connects to GND (most common, "active low")
- **Pull-down**: Button connects to 3.3V ("active high")

The LPC1343 has **internal pull-up and pull-down resistors** that you can enable in software, saving external components!

---

## LPC1343 GPIO Architecture

### GPIO Ports Overview

The LPC1343 has **4 GPIO ports**, each with multiple pins:

```
┌─────────────────────────────────────────────────────────────┐
│                        LPC1343                               │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Port 0 (GPIO0): 12 pins  (P0.0 - P0.11)                    │
│  Port 1 (GPIO1): 12 pins  (P1.0 - P1.11)                    │
│  Port 2 (GPIO2): 12 pins  (P2.0 - P2.11)                    │
│  Port 3 (GPIO3):  4 pins  (P3.0 - P3.3)                     │
│                                                              │
│  Total: 40 GPIO pins (but some shared with other functions) │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Pin Multiplexing

Most pins can serve multiple functions. For example, P1.6 can be:
- GPIO pin (general I/O)
- UART RXD (serial receive)
- CT32B0_MAT0 (timer output)

You must configure which function each pin performs using the **IOCON** (I/O Configuration) registers.

```
                    ┌─────────────┐
UART RXD ──────────→│             │
                    │             │
GPIO ──────────────→│  IOCON MUX  │───────→ Physical Pin P1.6
                    │  (selector) │
Timer Output ──────→│             │
                    └─────────────┘
                          ↑
                    Software selects
                    which function
```

### Memory-Mapped GPIO

GPIO registers are at these base addresses:

```
Port    Base Address    Example: Data Register
────    ────────────    ──────────────────────
GPIO0   0x50000000      GPIO0DATA at 0x50003FFC
GPIO1   0x50010000      GPIO1DATA at 0x50013FFC
GPIO2   0x50020000      GPIO2DATA at 0x50023FFC
GPIO3   0x50030000      GPIO3DATA at 0x50033FFC
```

**Note on GPIO Data Register Address:**
The data register has an unusual address (0x50003FFC instead of 0x50000000) because of a hardware feature called **bit masking** that allows atomic bit operations. We'll cover this later.

---

## Pin Configuration (IOCON)

Before using a GPIO pin, you must configure it using the **IOCON** (I/O Configuration) block.

### IOCON Register Location

Each pin has its own IOCON register:

```
IOCON Base Address: 0x40044000

Pin         Register Name       Address
────        ─────────────       ───────
P0.0        IOCON_RESET_PIO0_0  0x4004400C
P0.1        IOCON_PIO0_1        0x40044010
P0.2        IOCON_PIO0_2        0x4004401C
...
P1.6        IOCON_PIO1_6        0x400440A4
P1.7        IOCON_PIO1_7        0x400440A8
...
P3.0        IOCON_PIO3_0        0x40044084
P3.1        IOCON_PIO3_1        0x40044088
P3.2        IOCON_PIO3_2        0x4004409C
P3.3        IOCON_PIO3_3        0x400440AC
```

### IOCON Register Bit Fields

Each IOCON register has this layout (for standard GPIO pins):

```
Bit     Name        Description
────    ────        ───────────────────────────────────────
[2:0]   FUNC        Function select (which alternate function)
[4:3]   MODE        Pull-up/pull-down resistor mode
[5]     HYS         Hysteresis (Schmitt trigger) enable
[6]     -           Reserved
[7]     ADMODE      Analog/Digital mode (for analog-capable pins)
[8]     -           Reserved
[9]     OD          Open-drain mode enable
[31:10] -           Reserved
```

### FUNC - Function Select

```
FUNC Value   Meaning (varies by pin, check datasheet!)
──────────   ────────────────────────────────────────
   000       Primary function (often GPIO)
   001       First alternate function
   010       Second alternate function
   011       Third alternate function
```

**Example for P1.6:**
```
FUNC = 000: Reserved
FUNC = 001: GPIO (PIO1_6)
FUNC = 010: UART RXD
FUNC = 011: CT32B0_MAT0
```

### MODE - Pull Resistor Configuration

```
MODE Value   Meaning
──────────   ───────────────────────────────
   00        Inactive (no pull resistor)
   01        Pull-down resistor enabled
   10        Pull-up resistor enabled
   11        Repeater mode (bus-keeper)
```

**Code Example:**
```c
// Configure P1.6 as GPIO with internal pull-up
IOCON_PIO1_6 = (0x01 << 0)    // FUNC = 001 (GPIO)
             | (0x02 << 3)    // MODE = 10 (pull-up)
             | (0x01 << 5);   // HYS = 1 (hysteresis on)
```

### Complete IOCON Configuration Example

```c
// ============================================
// Configure P3.0 as GPIO output for LED
// ============================================

// Step 1: Set pin function to GPIO
// For P3.0, FUNC=001 selects GPIO function
IOCON_PIO3_0 = 0x00000001;  // FUNC=001, MODE=00 (no pull), standard output

// Step 2: Enable GPIO clock (usually done once at startup)
SYSAHBCLKCTRL |= (1 << 6);  // Enable GPIO clock

// Step 3: Set as output
GPIO3DIR |= (1 << 0);  // P3.0 = output

// Step 4: Drive the pin
GPIO3DATA |= (1 << 0);   // Set HIGH
GPIO3DATA &= ~(1 << 0);  // Set LOW
```

```c
// ============================================
// Configure P0.1 as GPIO input with pull-up (for button)
// ============================================

// Step 1: Set pin function to GPIO with pull-up
IOCON_PIO0_1 = (0x01 << 0)    // FUNC = 001 (GPIO)
             | (0x02 << 3)    // MODE = 10 (pull-up enabled)
             | (0x01 << 5);   // HYS = 1 (Schmitt trigger for clean edges)

// Step 2: Set as input
GPIO0DIR &= ~(1 << 1);  // P0.1 = input (0 = input)

// Step 3: Read the pin
if (GPIO0DATA & (1 << 1)) {
    // Pin is HIGH (button not pressed, due to pull-up)
} else {
    // Pin is LOW (button pressed, connects to GND)
}
```

---

## GPIO Registers

Each GPIO port has several registers for control:

### Register Summary

```
Offset    Register      Description
──────    ────────      ─────────────────────────────────────
0x0000    GPIOnDATA     Data register (masked access)
0x3FFC    GPIOnDATA     Data register (all bits, common usage)
0x8000    GPIOnDIR      Direction register (0=input, 1=output)
0x8004    GPIOnIS       Interrupt sense (0=edge, 1=level)
0x8008    GPIOnIBE      Interrupt both edges
0x800C    GPIOnIEV      Interrupt event (rising/high or falling/low)
0x8010    GPIOnIE       Interrupt mask (enable)
0x8014    GPIOnRIS      Raw interrupt status
0x8018    GPIOnMIS      Masked interrupt status
0x801C    GPIOnIC       Interrupt clear (write 1 to clear)
```

### GPIOnDIR - Direction Register

Controls whether each pin is input or output.

```
Bit = 0: Pin is INPUT  (default after reset)
Bit = 1: Pin is OUTPUT

Example: GPIO3DIR
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ 11 │ 10 │  9 │  8 │  7 │  6 │  5 │  4 │  3 │  2 │  1 │  0 │
├────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┤
│  0    0    0    0    0    0    0    0    0    0    0    0  │ = All inputs
│  0    0    0    0    0    0    0    0    0    0    0    1  │ = P3.0 output
│  0    0    0    0    0    0    0    0    1    1    1    1  │ = P3.0-3.3 outputs
└───────────────────────────────────────────────────────────┘
```

**Code Examples:**
```c
// Set P3.0 as output, leave others unchanged
GPIO3DIR |= (1 << 0);

// Set P3.0-P3.3 as outputs
GPIO3DIR |= 0x0000000F;

// Set P0.1 as input
GPIO0DIR &= ~(1 << 1);

// Set all of Port 3 as output
GPIO3DIR = 0x0000000F;  // Only 4 pins on Port 3
```

### GPIOnDATA - Data Register

Read or write pin values.

**For outputs:** Write to set pin voltage
**For inputs:** Read to get pin voltage

```c
// Write to outputs
GPIO3DATA |= (1 << 0);    // Set P3.0 HIGH
GPIO3DATA &= ~(1 << 0);   // Set P3.0 LOW
GPIO3DATA ^= (1 << 0);    // Toggle P3.0

// Read inputs
uint32_t pin_state = GPIO0DATA & (1 << 1);  // Read P0.1
if (pin_state) {
    // P0.1 is HIGH
}
```

### Bit-Banding / Masked Access (Advanced)

The LPC1343 GPIO data registers support **masked access** - a hardware feature that allows atomic read-modify-write on specific bits.

```
Address Range: 0x50000000 to 0x50003FFC (for GPIO0)

The lower 12 bits of the address act as a mask:
- Bits set in address[11:2] determine which data bits are affected
- This allows atomic bit operations without read-modify-write in software

Standard access (all bits):
Address 0x50003FFC = mask 0xFFF = all 12 bits accessible

Single-bit access:
Address 0x50000004 = mask 0x001 = only bit 0 accessible
Address 0x50000008 = mask 0x002 = only bit 1 accessible
Address 0x50000010 = mask 0x004 = only bit 2 accessible
```

**Practical Example:**
```c
// Standard way (requires read-modify-write, not atomic)
GPIO3DATA |= (1 << 0);  // Set bit 0

// Masked access way (atomic, hardware handles it)
// Write directly to masked address
*((volatile uint32_t *)(0x50030000 + (1 << 2))) = 0xFFFFFFFF;  // Set bit 0 only
```

For most applications, the standard read-modify-write approach is fine. Masked access is useful for interrupt safety and multi-threaded code.

---

## Output Mode: Driving LEDs and Loads

### Basic LED Circuit

```
ACTIVE-LOW LED (Common on dev boards)

    LPC1343                      LED
    ┌─────┐                    ┌──►│──┐
    │     │                    │  ───  │
    │ P3.0├────────────────────┤       │
    │     │                    │   R   │ Current-limiting
    │     │                    │  ┌┴┐  │ resistor
    │     │                    │  │ │  │ (220-470Ω typical)
    │     │                    │  └┬┘  │
    │ GND ├────────────────────┴───┴───┘
    └─────┘                          │
                                    ─┴─ GND
                                     ▼

Pin LOW (0V)  → Current flows → LED ON
Pin HIGH (3.3V) → No current → LED OFF


ACTIVE-HIGH LED

    LPC1343                      LED
    ┌─────┐                    ┌──►│──┐
    │     │                    │  ───  │
    │ P3.0├────────────────────┤       │
    │     │                    │   R   │
    │     │                    │  ┌┴┐  │
    │ 3.3V├────────────────────┴──┴┬┴──┘
    └─────┘                         │
                                   ─┴─
                                    ▼ To supply

Pin HIGH (3.3V) → Current flows → LED ON
Pin LOW (0V) → No current → LED OFF
```

### LED Control Code

```c
// ============================================
// LED Definitions (Active-Low, like LPC-P1343 board)
// ============================================

// On this board, LEDs are on Port 3, pins 0-3
// LEDs are active-LOW: clear bit to turn ON, set bit to turn OFF

#define LED0_PIN    (1 << 0)  // P3.0
#define LED1_PIN    (1 << 1)  // P3.1
#define LED2_PIN    (1 << 2)  // P3.2
#define LED3_PIN    (1 << 3)  // P3.3
#define ALL_LEDS    (LED0_PIN | LED1_PIN | LED2_PIN | LED3_PIN)

// Active-low macros
#define LED0_ON()   (GPIO3DATA &= ~LED0_PIN)
#define LED0_OFF()  (GPIO3DATA |= LED0_PIN)
#define LED0_TOGGLE() (GPIO3DATA ^= LED0_PIN)

#define LED1_ON()   (GPIO3DATA &= ~LED1_PIN)
#define LED1_OFF()  (GPIO3DATA |= LED1_PIN)

// Turn all LEDs on/off
#define ALL_LEDS_ON()  (GPIO3DATA &= ~ALL_LEDS)
#define ALL_LEDS_OFF() (GPIO3DATA |= ALL_LEDS)

// ============================================
// Initialization
// ============================================

void init_leds(void) {
    // Enable GPIO clock
    SYSAHBCLKCTRL |= (1 << 6);

    // Configure IOCON for GPIO function (if needed)
    // P3.0-P3.3 default to GPIO, but be explicit
    IOCON_PIO3_0 = 0x00000001;  // GPIO function
    IOCON_PIO3_1 = 0x00000001;
    IOCON_PIO3_2 = 0x00000001;
    IOCON_PIO3_3 = 0x00000001;

    // Set as outputs
    GPIO3DIR |= ALL_LEDS;

    // Start with all LEDs off
    ALL_LEDS_OFF();
}

// ============================================
// Usage Examples
// ============================================

void led_demo(void) {
    LED0_ON();       // Turn on LED0
    delay_ms(500);
    LED0_OFF();      // Turn off LED0
    delay_ms(500);
    LED0_TOGGLE();   // Toggle LED0
}
```

### Driving Higher Current Loads

GPIO pins can typically source/sink 4-20mA. For higher loads, use a transistor:

```
GPIO driving a relay/motor through transistor:

    LPC1343                NPN Transistor         Load
    ┌─────┐                    ┌───┐           ┌─────┐
    │     │     1kΩ           C│   │           │     │
    │ P3.0├────┬┬┬────────────┤   │───────────┤     │
    │     │                   B│   │           │     │
    │     │                    │   │E          │     │
    │     │                    └─┬─┘           │     │
    │ GND ├──────────────────────┴─────────────┤ GND │
    └─────┘                                    └─────┘

Pin HIGH → Transistor ON → Current flows through load
Pin LOW → Transistor OFF → No current
```

---

## Input Mode: Reading Buttons and Sensors

### Basic Button Circuit

```
Button with Pull-Up (Active-Low)

    LPC1343
    ┌─────┐
    │     │    Internal or     Button
    │ 3.3V├───┬┬┬──────────┬───┴ ┴───┐
    │     │   Pull-up      │          │
    │     │   resistor     │          │
    │ P0.1├────────────────┘          │
    │     │                           │
    │ GND ├───────────────────────────┘
    └─────┘

Button open:   Pin pulled HIGH by resistor → reads 1
Button pressed: Pin connected to GND → reads 0
```

### Button Reading Code

```c
// ============================================
// Button Definitions
// ============================================

#define BUTTON_PIN    (1 << 1)  // P0.1
#define BUTTON_PORT   GPIO0DATA

// Check if button is pressed (active-low)
#define BUTTON_PRESSED()  (!(BUTTON_PORT & BUTTON_PIN))

// ============================================
// Initialization
// ============================================

void init_button(void) {
    // Enable GPIO clock
    SYSAHBCLKCTRL |= (1 << 6);

    // Configure IOCON: GPIO function, pull-up, hysteresis
    IOCON_PIO0_1 = (0x01 << 0)    // FUNC = GPIO
                 | (0x02 << 3)    // MODE = Pull-up
                 | (0x01 << 5);   // HYS = Hysteresis enabled

    // Set as input (clear direction bit)
    GPIO0DIR &= ~BUTTON_PIN;
}

// ============================================
// Simple Polling
// ============================================

void poll_button_example(void) {
    if (BUTTON_PRESSED()) {
        LED0_ON();
    } else {
        LED0_OFF();
    }
}

// Wait for button press
void wait_for_press(void) {
    while (!BUTTON_PRESSED()) {
        // Wait for button to be pressed
    }
}

// Wait for button release
void wait_for_release(void) {
    while (BUTTON_PRESSED()) {
        // Wait for button to be released
    }
}
```

### Reading Multiple Inputs

```c
// ============================================
// Multiple Button Setup
// ============================================

#define BTN0_PIN    (1 << 0)  // P2.0
#define BTN1_PIN    (1 << 1)  // P2.1
#define BTN2_PIN    (1 << 2)  // P2.2
#define BTN3_PIN    (1 << 3)  // P2.3
#define ALL_BTNS    (BTN0_PIN | BTN1_PIN | BTN2_PIN | BTN3_PIN)

void init_buttons(void) {
    // Configure all as inputs with pull-ups
    GPIO2DIR &= ~ALL_BTNS;

    // IOCON configuration for each pin...
}

uint8_t read_all_buttons(void) {
    // Read and invert (so 1 = pressed for active-low buttons)
    return (~GPIO2DATA) & ALL_BTNS;
}

void button_handler(void) {
    uint8_t buttons = read_all_buttons();

    if (buttons & BTN0_PIN) {
        // Button 0 pressed
    }
    if (buttons & BTN1_PIN) {
        // Button 1 pressed
    }
    // etc.
}
```

---

## GPIO Interrupts

Instead of constantly polling buttons, you can configure GPIO pins to generate interrupts on state changes.

### Interrupt Types

```
EDGE-TRIGGERED INTERRUPTS

Rising Edge:        Falling Edge:       Both Edges:
    ┌───            ───┐                ┌───┐
    │                  │                │   │
────┘               └────           ────┘   └────
    ↑ IRQ              ↑ IRQ            ↑   ↑ IRQ


LEVEL-TRIGGERED INTERRUPTS

High Level:         Low Level:
────┬───┬────       ────┬   ┬────
    │   │               │   │
    │   │           ────┘   └────
    └───┘
  IRQ active        IRQ active
  entire time       entire time
```

### GPIO Interrupt Registers

```
Register    Purpose
────────    ──────────────────────────────────────────
GPIOnIS     Interrupt Sense: 0=edge, 1=level
GPIOnIBE    Interrupt Both Edges: 1=both, 0=use IEV
GPIOnIEV    Interrupt Event: rising/high or falling/low
GPIOnIE     Interrupt Enable: 1=enabled
GPIOnRIS    Raw Interrupt Status (before masking)
GPIOnMIS    Masked Interrupt Status (enabled only)
GPIOnIC     Interrupt Clear (write 1 to clear)
```

### Configuration Combinations

```
IS  IBE  IEV  │ Interrupt Trigger
────────────────────────────────────
0   0    0   │ Falling edge
0   0    1   │ Rising edge
0   1    X   │ Both edges (IEV ignored)
1   X    0   │ Low level
1   X    1   │ High level
```

### GPIO Interrupt Code Example

```c
// ============================================
// Configure P0.1 for falling-edge interrupt (button press)
// ============================================

#define BUTTON_PIN  (1 << 1)  // P0.1

void init_button_interrupt(void) {
    // Configure pin as input with pull-up (as before)
    IOCON_PIO0_1 = (0x01 << 0) | (0x02 << 3) | (0x01 << 5);
    GPIO0DIR &= ~BUTTON_PIN;

    // Configure interrupt
    GPIO0IS &= ~BUTTON_PIN;   // Edge-sensitive
    GPIO0IBE &= ~BUTTON_PIN;  // Not both edges
    GPIO0IEV &= ~BUTTON_PIN;  // Falling edge (button press)

    // Clear any pending interrupt
    GPIO0IC = BUTTON_PIN;

    // Enable interrupt for this pin
    GPIO0IE |= BUTTON_PIN;

    // Enable GPIO Port 0 interrupt in NVIC
    // PIO0 IRQ is interrupt 31 (see IRQ table below)
    NVIC_ISER = (1 << 31);
}

// ============================================
// Interrupt Handler for GPIO Port 0
// ============================================

void PIOINT0_IRQHandler(void) {
    // Check which pin caused the interrupt
    if (GPIO0MIS & BUTTON_PIN) {
        // Button was pressed!
        LED0_TOGGLE();

        // Clear the interrupt flag (IMPORTANT!)
        GPIO0IC = BUTTON_PIN;
    }
}

// ============================================
// Main program
// ============================================

int main(void) {
    init_leds();
    init_button_interrupt();

    // Enable global interrupts
    __enable_irq();

    while (1) {
        // CPU can do other work or sleep
        // Button presses are handled by interrupt
        __WFI();  // Wait For Interrupt (low power)
    }
}
```

### NVIC Configuration for GPIO

```c
// NVIC Interrupt Set Enable Register
#define NVIC_ISER (*((volatile uint32_t *)0xE000E100))

// GPIO interrupt numbers in LPC1343
// PIO3_IRQn = 27  (GPIO Port 3)
// PIO2_IRQn = 28  (GPIO Port 2)
// PIO1_IRQn = 29  (GPIO Port 1)
// PIO0_IRQn = 31  (GPIO Port 0)

// Enable interrupt in NVIC
void enable_gpio_interrupt(uint8_t port) {
    switch (port) {
        case 0: NVIC_ISER = (1 << 31); break;  // PIO0
        case 1: NVIC_ISER = (1 << 29); break;  // PIO1
        case 2: NVIC_ISER = (1 << 28); break;  // PIO2
        case 3: NVIC_ISER = (1 << 27); break;  // PIO3
    }
}
```

---

## Button Debouncing

### The Problem: Mechanical Bounce

When you press a button, the contacts don't make a clean connection. They "bounce" multiple times:

```
What you think happens:       What actually happens:

Button Press:                 Button Press:
    ┌────────                     ┌─┐ ┌─┐ ┌────────
    │                             │ │ │ │ │
────┘                         ────┘ └─┘ └─┘
    1 clean                       Multiple bounces
    transition                    over 1-50ms!
```

Without debouncing, one button press might trigger multiple interrupts or be read as multiple presses.

### Software Debouncing Methods

#### Method 1: Simple Delay

```c
// Crude but effective
if (BUTTON_PRESSED()) {
    delay_ms(50);  // Wait for bouncing to stop
    if (BUTTON_PRESSED()) {  // Confirm still pressed
        // Handle button press
    }
}
```

#### Method 2: State Machine with Timer

```c
// More sophisticated debouncing state machine

typedef enum {
    BTN_IDLE,
    BTN_PRESS_DEBOUNCE,
    BTN_PRESSED,
    BTN_RELEASE_DEBOUNCE
} ButtonState;

typedef struct {
    ButtonState state;
    uint32_t timestamp;
    uint8_t pressed;  // Debounced output
} Button;

#define DEBOUNCE_TIME_MS 20

void button_update(Button *btn, uint8_t raw_input, uint32_t current_time) {
    switch (btn->state) {
        case BTN_IDLE:
            if (raw_input) {  // Button pressed (active high after inversion)
                btn->state = BTN_PRESS_DEBOUNCE;
                btn->timestamp = current_time;
            }
            break;

        case BTN_PRESS_DEBOUNCE:
            if (!raw_input) {
                // Bounce - go back to idle
                btn->state = BTN_IDLE;
            } else if ((current_time - btn->timestamp) >= DEBOUNCE_TIME_MS) {
                // Stable press
                btn->state = BTN_PRESSED;
                btn->pressed = 1;
            }
            break;

        case BTN_PRESSED:
            if (!raw_input) {
                btn->state = BTN_RELEASE_DEBOUNCE;
                btn->timestamp = current_time;
            }
            break;

        case BTN_RELEASE_DEBOUNCE:
            if (raw_input) {
                // Bounce - still pressed
                btn->state = BTN_PRESSED;
            } else if ((current_time - btn->timestamp) >= DEBOUNCE_TIME_MS) {
                // Stable release
                btn->state = BTN_IDLE;
                btn->pressed = 0;
            }
            break;
    }
}
```

#### Method 3: Integration/Counter Method

```c
// Count consecutive readings

#define DEBOUNCE_COUNT 10

typedef struct {
    uint8_t count;      // Integration counter
    uint8_t pressed;    // Debounced output
} Button;

// Call this from a regular timer interrupt (e.g., every 1ms)
void button_sample(Button *btn, uint8_t raw_input) {
    if (raw_input) {
        // Button appears pressed
        if (btn->count < DEBOUNCE_COUNT) {
            btn->count++;
        }
        if (btn->count >= DEBOUNCE_COUNT) {
            btn->pressed = 1;
        }
    } else {
        // Button appears released
        if (btn->count > 0) {
            btn->count--;
        }
        if (btn->count == 0) {
            btn->pressed = 0;
        }
    }
}
```

#### Method 4: Interrupt + Timer Debounce

```c
// Disable interrupt during debounce period

volatile uint8_t button_flag = 0;

void PIOINT0_IRQHandler(void) {
    if (GPIO0MIS & BUTTON_PIN) {
        // Disable this pin's interrupt temporarily
        GPIO0IE &= ~BUTTON_PIN;

        // Clear interrupt
        GPIO0IC = BUTTON_PIN;

        // Start debounce timer (e.g., 50ms one-shot)
        start_debounce_timer(50);
    }
}

void debounce_timer_callback(void) {
    // Check button state after debounce period
    if (BUTTON_PRESSED()) {
        button_flag = 1;  // Valid press detected
    }

    // Re-enable button interrupt
    GPIO0IC = BUTTON_PIN;  // Clear any pending
    GPIO0IE |= BUTTON_PIN;  // Re-enable
}

int main(void) {
    // ...
    while (1) {
        if (button_flag) {
            button_flag = 0;
            // Handle button press
            LED0_TOGGLE();
        }
    }
}
```

---

## Practical Examples

### Example 1: Running Light (LED Chaser)

```c
// LEDs light up in sequence: 0 → 1 → 2 → 3 → 0 → ...

#include <stdint.h>

#define LED_MASK 0x0F
#define NUM_LEDS 4

void running_light(void) {
    uint8_t current_led = 0;

    while (1) {
        // Turn off all LEDs, then turn on current one
        GPIO3DATA |= LED_MASK;           // All off (active low)
        GPIO3DATA &= ~(1 << current_led); // Current on

        delay_ms(200);

        // Move to next LED
        current_led++;
        if (current_led >= NUM_LEDS) {
            current_led = 0;
        }
    }
}

// Alternative using bit rotation
void running_light_v2(void) {
    uint8_t pattern = 0x01;  // Start with LED0

    while (1) {
        // Write inverted pattern (active-low LEDs)
        GPIO3DATA = (GPIO3DATA & ~LED_MASK) | (~pattern & LED_MASK);

        delay_ms(200);

        // Rotate pattern
        pattern <<= 1;
        if (pattern > 0x08) {
            pattern = 0x01;
        }
    }
}
```

### Example 2: Binary Counter on LEDs

```c
// Display a 4-bit counter on 4 LEDs

void binary_counter(void) {
    uint8_t count = 0;

    while (1) {
        // Display count on LEDs (inverted for active-low)
        GPIO3DATA = (GPIO3DATA & ~LED_MASK) | (~count & LED_MASK);

        delay_ms(500);

        count++;
        if (count > 15) {
            count = 0;
        }
    }
}
```

### Example 3: Button-Controlled LED Pattern

```c
// Button cycles through different LED patterns

typedef enum {
    PATTERN_ALL_OFF,
    PATTERN_ALL_ON,
    PATTERN_ALTERNATE,
    PATTERN_CHASE,
    NUM_PATTERNS
} Pattern;

volatile Pattern current_pattern = PATTERN_ALL_OFF;
volatile uint8_t pattern_changed = 0;

void PIOINT0_IRQHandler(void) {
    if (GPIO0MIS & BUTTON_PIN) {
        current_pattern++;
        if (current_pattern >= NUM_PATTERNS) {
            current_pattern = PATTERN_ALL_OFF;
        }
        pattern_changed = 1;

        GPIO0IC = BUTTON_PIN;
    }
}

int main(void) {
    static uint8_t chase_pos = 0;

    init_leds();
    init_button_interrupt();
    __enable_irq();

    while (1) {
        switch (current_pattern) {
            case PATTERN_ALL_OFF:
                GPIO3DATA |= LED_MASK;  // All off
                break;

            case PATTERN_ALL_ON:
                GPIO3DATA &= ~LED_MASK;  // All on
                break;

            case PATTERN_ALTERNATE:
                GPIO3DATA = (GPIO3DATA & ~LED_MASK) | 0x05;  // 0101
                delay_ms(200);
                GPIO3DATA = (GPIO3DATA & ~LED_MASK) | 0x0A;  // 1010
                delay_ms(200);
                break;

            case PATTERN_CHASE:
                GPIO3DATA |= LED_MASK;
                GPIO3DATA &= ~(1 << chase_pos);
                delay_ms(100);
                chase_pos = (chase_pos + 1) % NUM_LEDS;
                break;
        }
    }
}
```

### Example 4: GPIO Port Mirroring

```c
// Read inputs from Port 0, mirror to outputs on Port 3

void port_mirror(void) {
    // Configure Port 0 lower nibble as inputs
    GPIO0DIR &= ~0x0F;

    // Configure Port 3 as outputs
    GPIO3DIR |= 0x0F;

    while (1) {
        // Read Port 0, write to Port 3
        uint32_t input = GPIO0DATA & 0x0F;
        GPIO3DATA = (GPIO3DATA & ~0x0F) | input;
    }
}
```

### Example 5: Combination Lock

```c
// Simple 4-button combination lock
// Correct sequence: BTN0, BTN1, BTN2, BTN3
// Wrong sequence resets

#define SEQUENCE_LENGTH 4
const uint8_t correct_sequence[SEQUENCE_LENGTH] = {
    BTN0_PIN, BTN1_PIN, BTN2_PIN, BTN3_PIN
};

void combination_lock(void) {
    uint8_t sequence_index = 0;
    uint8_t last_buttons = 0;

    while (1) {
        uint8_t buttons = read_all_buttons();
        uint8_t newly_pressed = buttons & ~last_buttons;  // Edge detection

        if (newly_pressed) {
            if (newly_pressed == correct_sequence[sequence_index]) {
                // Correct button in sequence
                sequence_index++;

                if (sequence_index >= SEQUENCE_LENGTH) {
                    // Unlock!
                    ALL_LEDS_ON();
                    delay_ms(2000);
                    ALL_LEDS_OFF();
                    sequence_index = 0;
                } else {
                    // Flash to indicate progress
                    LED0_ON();
                    delay_ms(100);
                    LED0_OFF();
                }
            } else {
                // Wrong button - reset sequence
                sequence_index = 0;
                // Flash error
                for (int i = 0; i < 3; i++) {
                    ALL_LEDS_ON();
                    delay_ms(50);
                    ALL_LEDS_OFF();
                    delay_ms(50);
                }
            }
        }

        last_buttons = buttons;
        delay_ms(10);  // Simple debounce
    }
}
```

---

## Common Patterns and Best Practices

### Pattern 1: Initialization Template

```c
// Standard GPIO initialization sequence

void gpio_init(void) {
    // 1. Enable GPIO clock (once for all ports)
    SYSAHBCLKCTRL |= (1 << 6);

    // 2. Configure IOCON for each pin
    // (function, pull-up/down, hysteresis, etc.)

    // 3. Set pin directions
    // GPIOxDIR |= output_pins;
    // GPIOxDIR &= ~input_pins;

    // 4. Set initial output values

    // 5. Configure interrupts if needed
}
```

### Pattern 2: Pin Abstraction

```c
// Define pins in one place for easy changes

// Pin definitions
typedef struct {
    volatile uint32_t *data_reg;
    volatile uint32_t *dir_reg;
    uint32_t pin_mask;
} GPIO_Pin;

const GPIO_Pin LED0 = { &GPIO3DATA, &GPIO3DIR, (1 << 0) };
const GPIO_Pin LED1 = { &GPIO3DATA, &GPIO3DIR, (1 << 1) };
const GPIO_Pin BUTTON = { &GPIO0DATA, &GPIO0DIR, (1 << 1) };

// Generic functions
void pin_set_output(const GPIO_Pin *pin) {
    *pin->dir_reg |= pin->pin_mask;
}

void pin_set_input(const GPIO_Pin *pin) {
    *pin->dir_reg &= ~pin->pin_mask;
}

void pin_write_high(const GPIO_Pin *pin) {
    *pin->data_reg |= pin->pin_mask;
}

void pin_write_low(const GPIO_Pin *pin) {
    *pin->data_reg &= ~pin->pin_mask;
}

void pin_toggle(const GPIO_Pin *pin) {
    *pin->data_reg ^= pin->pin_mask;
}

uint8_t pin_read(const GPIO_Pin *pin) {
    return (*pin->data_reg & pin->pin_mask) ? 1 : 0;
}

// Usage
void example(void) {
    pin_set_output(&LED0);
    pin_write_low(&LED0);  // LED on (active-low)

    pin_set_input(&BUTTON);
    if (!pin_read(&BUTTON)) {  // Active-low button
        // Button pressed
    }
}
```

### Pattern 3: Atomic Bit Operations

```c
// When ISR and main code access same GPIO

// WRONG - not atomic, can be interrupted mid-operation
GPIO3DATA |= (1 << 0);  // Read-modify-write

// BETTER - disable interrupts during modification
__disable_irq();
GPIO3DATA |= (1 << 0);
__enable_irq();

// BEST for LPC1343 - use masked write (hardware atomic)
// Write to specific masked address
#define GPIO3_BIT0_ADDR  (0x50030000 + (1 << 2))
*((volatile uint32_t *)GPIO3_BIT0_ADDR) = 0xFFFFFFFF;  // Set bit 0
*((volatile uint32_t *)GPIO3_BIT0_ADDR) = 0x00000000;  // Clear bit 0
```

### Pattern 4: Bit Field Macros

```c
// Reusable macros for any GPIO port

#define GPIO_SET_BIT(port, bit)    ((port) |= (1 << (bit)))
#define GPIO_CLEAR_BIT(port, bit)  ((port) &= ~(1 << (bit)))
#define GPIO_TOGGLE_BIT(port, bit) ((port) ^= (1 << (bit)))
#define GPIO_READ_BIT(port, bit)   (((port) >> (bit)) & 1)

#define GPIO_SET_MASK(port, mask)    ((port) |= (mask))
#define GPIO_CLEAR_MASK(port, mask)  ((port) &= ~(mask))
#define GPIO_WRITE_MASK(port, mask, value) \
    ((port) = ((port) & ~(mask)) | ((value) & (mask)))

// Usage
GPIO_SET_BIT(GPIO3DATA, 0);      // Set bit 0
GPIO_CLEAR_BIT(GPIO3DATA, 0);    // Clear bit 0
GPIO_TOGGLE_BIT(GPIO3DATA, 0);   // Toggle bit 0
uint8_t val = GPIO_READ_BIT(GPIO0DATA, 1);  // Read bit 1
```

### Best Practice Summary

1. **Always enable GPIO clock** before accessing GPIO registers
2. **Configure IOCON first**, then direction, then data
3. **Use #defines for pin assignments** - easy to change if hardware changes
4. **Document active-low vs active-high** in comments
5. **Debounce all mechanical switches**
6. **Keep ISRs short** - set flags, don't do lengthy processing
7. **Clear interrupt flags** before returning from ISR
8. **Use pull-up/pull-down resistors** on all floating inputs
9. **Consider interrupt safety** when sharing GPIO between ISR and main

---

## Troubleshooting

### Problem: Pin doesn't change state

**Possible causes:**
1. GPIO clock not enabled
   ```c
   // Fix: Enable GPIO clock
   SYSAHBCLKCTRL |= (1 << 6);
   ```

2. Pin direction wrong
   ```c
   // Fix: Set as output
   GPIO3DIR |= (1 << pin);
   ```

3. IOCON not configured for GPIO function
   ```c
   // Fix: Check FUNC bits are set for GPIO
   IOCON_PIO3_0 = 0x01;  // FUNC = GPIO
   ```

4. Another peripheral using the pin
   ```c
   // Check IOCON FUNC setting - might be UART, SPI, etc.
   ```

### Problem: Input always reads the same value

**Possible causes:**
1. Pin set as output instead of input
   ```c
   // Fix: Set as input
   GPIO0DIR &= ~(1 << pin);
   ```

2. No pull-up/pull-down and pin is floating
   ```c
   // Fix: Enable internal pull-up
   IOCON_PIO0_1 |= (0x02 << 3);  // Pull-up mode
   ```

3. External circuit issue - check wiring

### Problem: GPIO interrupt doesn't fire

**Possible causes:**
1. Interrupt not enabled in GPIO
   ```c
   // Fix: Enable in GPIOxIE register
   GPIO0IE |= (1 << pin);
   ```

2. Interrupt not enabled in NVIC
   ```c
   // Fix: Enable in NVIC (PIO0 is IRQ 31)
   NVIC_ISER = (1 << 31);
   ```

3. Global interrupts disabled
   ```c
   // Fix: Enable global interrupts
   __enable_irq();
   ```

4. Wrong edge/level configuration
   ```c
   // Check IS, IBE, IEV register settings
   ```

5. Interrupt flag not cleared in previous ISR
   ```c
   // Fix: Clear interrupt flag
   GPIO0IC = (1 << pin);
   ```

### Problem: Multiple interrupts from one button press

**Cause:** Button bounce

**Fix:** Implement debouncing (see Debouncing section)

### Problem: LED brightness varies

**Possible causes:**
1. PWM unintentionally enabled - check timer configuration
2. Code toggling too fast - check timing
3. Insufficient current-limiting resistor - LED may be overdriven

---

## Quick Reference

### GPIO Register Summary

| Register | Address Offset | Description |
|----------|----------------|-------------|
| GPIOnDATA | 0x3FFC | Data (read/write pins) |
| GPIOnDIR | 0x8000 | Direction (0=in, 1=out) |
| GPIOnIS | 0x8004 | Interrupt sense (0=edge, 1=level) |
| GPIOnIBE | 0x8008 | Both edges (1=both) |
| GPIOnIEV | 0x800C | Event (0=fall/low, 1=rise/high) |
| GPIOnIE | 0x8010 | Interrupt enable |
| GPIOnRIS | 0x8014 | Raw interrupt status |
| GPIOnMIS | 0x8018 | Masked interrupt status |
| GPIOnIC | 0x801C | Interrupt clear |

### NVIC Interrupt Numbers for GPIO

```c
#define NVIC_ISER (*((volatile uint32_t *)0xE000E100))

// GPIO Port IRQ Numbers
// PIO3_IRQn = 27
// PIO2_IRQn = 28
// PIO1_IRQn = 29
// PIO0_IRQn = 31

NVIC_ISER = (1 << 31);  // Enable PIO0 interrupt
NVIC_ISER = (1 << 29);  // Enable PIO1 interrupt
NVIC_ISER = (1 << 28);  // Enable PIO2 interrupt
NVIC_ISER = (1 << 27);  // Enable PIO3 interrupt
```

### IOCON Mode Settings

| MODE [4:3] | Pull Resistor |
|------------|---------------|
| 00 | Inactive (floating) |
| 01 | Pull-down |
| 10 | Pull-up |
| 11 | Repeater |

### Common Code Snippets

```c
// === OUTPUT OPERATIONS ===
GPIO3DIR |= (1 << n);        // Set pin n as output
GPIO3DATA |= (1 << n);       // Set pin n HIGH
GPIO3DATA &= ~(1 << n);      // Set pin n LOW
GPIO3DATA ^= (1 << n);       // Toggle pin n

// === INPUT OPERATIONS ===
GPIO0DIR &= ~(1 << n);       // Set pin n as input
if (GPIO0DATA & (1 << n))    // Check if pin n is HIGH
if (!(GPIO0DATA & (1 << n))) // Check if pin n is LOW

// === INTERRUPT SETUP (falling edge) ===
GPIO0IS &= ~(1 << n);        // Edge-sensitive
GPIO0IBE &= ~(1 << n);       // Single edge
GPIO0IEV &= ~(1 << n);       // Falling edge
GPIO0IC = (1 << n);          // Clear pending
GPIO0IE |= (1 << n);         // Enable interrupt
NVIC_ISER = (1 << 31);       // Enable PIO0 in NVIC (IRQ 31)

// === IN ISR ===
if (GPIO0MIS & (1 << n)) {   // Check interrupt source
    // Handle interrupt
    GPIO0IC = (1 << n);      // Clear interrupt flag
}
```

### Pin Function Quick Reference

```
Port 3 (4 pins) - Commonly used for LEDs:
P3.0: GPIO / AD5 / CT16B0_MAT0
P3.1: GPIO / AD4 / CT16B0_MAT1
P3.2: GPIO / AD3 / CT16B0_MAT2
P3.3: GPIO / AD2 / CT16B0_MAT3

Port 0 (commonly used for buttons/inputs):
P0.1: GPIO / CLKOUT / CT32B0_MAT2
P0.2: GPIO / SSEL / CT16B0_CAP0
...

(See LPC1343 datasheet for complete pinout)
```

---

## Further Practice

1. **LED Dice**: Use 7 LEDs arranged like dice dots, show random number on button press
2. **Reaction Timer**: Measure time between LED lighting and button press
3. **Simon Says**: Memory game with 4 LEDs and 4 buttons
4. **Morse Code**: Input text via serial, output on LED as Morse code
5. **PWM Simulation**: Toggle GPIO very fast in timer ISR to dim LED
6. **State Machine**: Implement a traffic light controller

---

## What's Next?

You can now control any GPIO pin on the LPC1343. But the software delays we've been using are imprecise and waste CPU cycles. Time to learn about hardware timers!

**Next Chapter:** [Chapter 4: Timers and PWM](04-timers-and-pwm.md) - Generate precise delays, create PWM signals for LED dimming and motor control.

---

*Chapter 3 of the LPC1343 Embedded C Programming Guide*
