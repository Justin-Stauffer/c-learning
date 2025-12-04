# Chapter 1: Bitwise Operations

A comprehensive beginner's guide to understanding bitwise operations and why they're essential in embedded systems.

---

## Chapter Overview

| | |
|---|---|
| **Prerequisites** | Chapter 0 (Getting Started) |
| **Time to Complete** | 2-3 hours |
| **Hands-On Projects** | LED patterns, binary counter |
| **You Will Learn** | How to manipulate individual bits in registers |

---

## Quick Start: The Four Patterns You'll Use 90% of the Time

Before diving deep, here are the four bit operations you'll use constantly:

```c
// 1. SET a bit (turn something ON)
register |= (1 << bit_number);
// Example: Turn on bit 5
GPIO3DATA |= (1 << 5);

// 2. CLEAR a bit (turn something OFF)
register &= ~(1 << bit_number);
// Example: Turn off bit 5
GPIO3DATA &= ~(1 << 5);

// 3. TOGGLE a bit (flip its state)
register ^= (1 << bit_number);
// Example: Toggle bit 5
GPIO3DATA ^= (1 << 5);

// 4. CHECK if a bit is set
if (register & (1 << bit_number)) {
    // bit is 1
}
// Example: Check bit 5
if (GPIO3DATA & (1 << 5)) {
    // Pin 5 is HIGH
}
```

**That's it.** These four patterns will cover almost every situation. The rest of this chapter explains *why* they work and shows more advanced uses.

---

## Table of Contents

1. [Why Bitwise Operations?](#why-bitwise-operations)
2. [Binary Basics Refresher](#binary-basics-refresher)
3. [Left Shift (<<)](#left-shift-)
4. [Right Shift (>>)](#right-shift-)
5. [AND (&)](#and-)
6. [OR (|)](#or-)
7. [XOR (^)](#xor-)
8. [NOT (~)](#not-)
9. [Compound Assignment Operators](#compound-assignment-operators)
10. [Common Patterns](#common-patterns)
11. [Real-World Examples](#real-world-examples)
12. [Bit Manipulation Cookbook](#bit-manipulation-cookbook)
13. [Practice Problems](#practice-problems)

---

## Why Bitwise Operations?

In embedded systems, you often need to control **individual bits** in hardware registers. Each bit might control a different hardware feature:

```
Hardware Register (8 bits):
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
└───┴───┴───┴───┴───┴───┴───┴───┘
  │   │   │   │   │   │   │   └──── Feature A: ON/OFF
  │   │   │   │   │   │   └──────── Feature B: ON/OFF
  │   │   │   │   │   └──────────── Feature C: ON/OFF
  │   │   │   │   └──────────────── Feature D: ON/OFF
  │   │   │   └──────────────────── Feature E: ON/OFF
  │   │   └──────────────────────── Feature F: ON/OFF
  │   └──────────────────────────── Feature G: ON/OFF
  └──────────────────────────────── Feature H: ON/OFF
```

**Bitwise operations let you:**
- Turn individual features ON/OFF without affecting others
- Read the state of individual features
- Toggle features
- Create bit masks
- Extract specific bit fields

---

## Binary Basics Refresher

### Bit Positions

Bits are numbered from **right to left**, starting at 0:

```
Binary:   1 0 1 0 0 1 1 0
Position: 7 6 5 4 3 2 1 0

Bit 0 = 0 (rightmost, least significant bit - LSB)
Bit 7 = 1 (leftmost, most significant bit - MSB)
```

### Number Systems

```
Decimal | Binary      | Hexadecimal
--------|-------------|------------
    0   | 0000 0000   | 0x00
    1   | 0000 0001   | 0x01
    2   | 0000 0010   | 0x02
    4   | 0000 0100   | 0x04
    8   | 0000 1000   | 0x08
   15   | 0000 1111   | 0x0F
   16   | 0001 0000   | 0x10
   32   | 0010 0000   | 0x20
   64   | 0100 0000   | 0x40
  128   | 1000 0000   | 0x80
  255   | 1111 1111   | 0xFF
```

---

## Left Shift (<<)

**Symbol:** `<<`
**What it does:** Shifts all bits to the left, filling with zeros on the right

### Syntax
```c
result = value << n;  // Shift 'value' left by 'n' positions
```

### Visual Example

```
Original:   0000 0001  (1 in decimal)
            ^^^^^^^^
            76543210   (bit positions)

1 << 0:     0000 0001  = 1
1 << 1:     0000 0010  = 2
1 << 2:     0000 0100  = 4
1 << 3:     0000 1000  = 8
1 << 4:     0001 0000  = 16
1 << 5:     0010 0000  = 32
1 << 6:     0100 0000  = 64
1 << 7:     1000 0000  = 128
```

### Step-by-Step: `1 << 5`

```
Step 1: Start with 1
Binary: 0000 0001

Step 2: Shift left 5 times
Shift 1: 0000 0010  (×2)
Shift 2: 0000 0100  (×2)
Shift 3: 0000 1000  (×2)
Shift 4: 0001 0000  (×2)
Shift 5: 0010 0000  (×2)

Result: 0010 0000 = 0x20 = 32
```

### Mathematical View
```c
n << x  =  n × 2^x

Examples:
1 << 5  =  1 × 2^5  =  1 × 32   = 32
3 << 2  =  3 × 2^2  =  3 × 4    = 12
7 << 1  =  7 × 2^1  =  7 × 2    = 14
```

### Common Use Cases

**1. Create Bit Masks**
```c
#define BIT0  (1 << 0)  // 0x01
#define BIT1  (1 << 1)  // 0x02
#define BIT5  (1 << 5)  // 0x20
```

**2. Set Specific Bits**
```c
GPIO_DATA |= (1 << 5);  // Turn on bit 5 (pin 5)
```

**3. Create Multi-Bit Masks**
```c
// Enable bits 2, 4, and 6
uint8_t mask = (1 << 2) | (1 << 4) | (1 << 6);
// Result: 0101 0100 = 0x54
```

---

## Right Shift (>>)

**Symbol:** `>>`
**What it does:** Shifts all bits to the right, discarding bits that fall off

### Syntax
```c
result = value >> n;  // Shift 'value' right by 'n' positions
```

### Visual Example

```
Original:   1000 0000  (128 in decimal)
            ^^^^^^^^
            76543210

128 >> 0:   1000 0000  = 128
128 >> 1:   0100 0000  = 64
128 >> 2:   0010 0000  = 32
128 >> 3:   0001 0000  = 16
128 >> 4:   0000 1000  = 8
128 >> 5:   0000 0100  = 4
128 >> 6:   0000 0010  = 2
128 >> 7:   0000 0001  = 1
```

### Mathematical View
```c
n >> x  =  n ÷ 2^x  (integer division)

Examples:
32 >> 1  =  32 ÷ 2   = 16
32 >> 2  =  32 ÷ 4   = 8
32 >> 5  =  32 ÷ 32  = 1
```

### Common Use Cases

**1. Extract Bit Fields**
```c
// Register layout: [7:4] = channel, [3:0] = value
uint8_t reg = 0xA5;  // 1010 0101

uint8_t channel = (reg >> 4) & 0x0F;  // Extract upper nibble
// (0xA5 >> 4) = 0x0A, then & 0x0F = 0x0A (channel 10)

uint8_t value = reg & 0x0F;  // Extract lower nibble
// 0xA5 & 0x0F = 0x05 (value 5)
```

**2. Check Specific Bit**
```c
// Check if bit 3 is set
if ((register_value >> 3) & 1) {
    // Bit 3 is 1
}
```

**3. Divide by Powers of 2 (Fast)**
```c
uint32_t divided_by_2   = value >> 1;  // Faster than value / 2
uint32_t divided_by_4   = value >> 2;  // Faster than value / 4
uint32_t divided_by_256 = value >> 8;  // Faster than value / 256
```

---

## AND (&)

**Symbol:** `&`
**What it does:** Compares bits; result is 1 only if **BOTH** bits are 1

### Truth Table
```
A  &  B  =  Result
0  &  0  =    0
0  &  1  =    0
1  &  0  =    0
1  &  1  =    1      ← Only case that gives 1
```

### Visual Example

```
Value A:    1010 1100  (0xAC)
Value B:    1111 0000  (0xF0 - mask)
          & ──────────
Result:     1010 0000  (0xA0)

Bit by bit:
Bit 7: 1 & 1 = 1
Bit 6: 0 & 1 = 0
Bit 5: 1 & 1 = 1
Bit 4: 0 & 1 = 0
Bit 3: 1 & 0 = 0  ← Masked out
Bit 2: 1 & 0 = 0  ← Masked out
Bit 1: 0 & 0 = 0  ← Masked out
Bit 0: 0 & 0 = 0  ← Masked out
```

### Common Use Cases

**1. Clear Specific Bits (Bit Masking)**
```c
// Clear bit 5 (set it to 0)
GPIO_DATA &= ~(1 << 5);

// Explanation:
// (1 << 5)  = 0010 0000
// ~(1 << 5) = 1101 1111  (inverts to all 1s except bit 5)
// Original:   1111 1111
// AND:        1101 1111
// Result:     1101 1111  (bit 5 is now 0, others unchanged)
```

**2. Extract Specific Bits**
```c
// Extract lower 4 bits (nibble)
uint8_t lower_nibble = value & 0x0F;

// Example:
// value = 0xA5 = 1010 0101
// 0x0F =         0000 1111
// Result =       0000 0101 = 0x05
```

**3. Check if Bit is Set**
```c
// Check if bit 3 is set
if (register_value & (1 << 3)) {
    // Bit 3 is set (equals 1)
}

// Example:
// register_value = 0000 1100
// (1 << 3)       = 0000 1000
// AND result     = 0000 1000  (non-zero = true)
```

**4. Keep Only Specific Bits**
```c
// Keep only bits 4, 5, 6
uint8_t masked = value & 0x70;  // 0x70 = 0111 0000
```

---

## OR (|)

**Symbol:** `|`
**What it does:** Compares bits; result is 1 if **EITHER OR BOTH** bits are 1

### Truth Table
```
A  |  B  =  Result
0  |  0  =    0      ← Only case that gives 0
0  |  1  =    1
1  |  0  =    1
1  |  1  =    1
```

### Visual Example

```
Value A:    1010 0000  (0xA0)
Value B:    0000 1100  (0x0C)
          | ──────────
Result:     1010 1100  (0xAC)

Bit by bit:
Bit 7: 1 | 0 = 1
Bit 6: 0 | 0 = 0
Bit 5: 1 | 0 = 1
Bit 4: 0 | 0 = 0
Bit 3: 0 | 1 = 1  ← Set by OR
Bit 2: 0 | 1 = 1  ← Set by OR
Bit 1: 0 | 0 = 0
Bit 0: 0 | 0 = 0
```

### Common Use Cases

**1. Set Specific Bits**
```c
// Set bit 5 to 1 (without affecting others)
GPIO_DATA |= (1 << 5);

// Example:
// Original:  0000 0000
// (1 << 5):  0010 0000
// OR result: 0010 0000  (bit 5 is now 1)

// If bit 5 was already 1, it stays 1:
// Original:  0010 0000
// (1 << 5):  0010 0000
// OR result: 0010 0000  (no change)
```

**2. Set Multiple Bits**
```c
// Set bits 2, 5, and 7
register_value |= (1 << 2) | (1 << 5) | (1 << 7);

// Calculation:
// (1 << 2) = 0000 0100
// (1 << 5) = 0010 0000
// (1 << 7) = 1000 0000
// Combined = 1010 0100 = 0xA4
```

**3. Combine Bit Masks**
```c
#define ENABLE_UART   (1 << 0)
#define ENABLE_TIMER  (1 << 1)
#define ENABLE_GPIO   (1 << 2)

// Enable all three peripherals
peripheral_enable |= ENABLE_UART | ENABLE_TIMER | ENABLE_GPIO;
```

**4. Set Configuration Bits**
```c
// Configure timer: enable + interrupt + auto-reload
TMR_CONFIG = TMR_ENABLE | TMR_INT_EN | TMR_AUTO_RELOAD;
```

---

## XOR (^)

**Symbol:** `^`
**What it does:** Compares bits; result is 1 if bits are **DIFFERENT**

### Truth Table
```
A  ^  B  =  Result
0  ^  0  =    0      (same = 0)
0  ^  1  =    1      (different = 1)
1  ^  0  =    1      (different = 1)
1  ^  1  =    0      (same = 0)
```

### Visual Example

```
Value A:    1010 1100  (0xAC)
Value B:    1111 0000  (0xF0)
          ^ ──────────
Result:     0101 1100  (0x5C)

Bit by bit:
Bit 7: 1 ^ 1 = 0  (same)
Bit 6: 0 ^ 1 = 1  (different)
Bit 5: 1 ^ 1 = 0  (same)
Bit 4: 0 ^ 1 = 1  (different)
Bit 3: 1 ^ 0 = 1  (different)
Bit 2: 1 ^ 0 = 1  (different)
Bit 1: 0 ^ 0 = 0  (same)
Bit 0: 0 ^ 0 = 0  (same)
```

### Common Use Cases

**1. Toggle Specific Bits**
```c
// Toggle bit 5 (flip it: 0→1 or 1→0)
GPIO_DATA ^= (1 << 5);

// Example - first toggle:
// Original:  0000 0000
// (1 << 5):  0010 0000
// XOR:       0010 0000  (bit 5 flipped to 1)

// Example - second toggle:
// Original:  0010 0000
// (1 << 5):  0010 0000
// XOR:       0000 0000  (bit 5 flipped back to 0)
```

**2. Swap Values Without Temporary Variable**
```c
// Classic XOR swap trick
a = a ^ b;
b = a ^ b;  // b now has original a
a = a ^ b;  // a now has original b
```

**3. Simple Encryption (XOR Cipher)**
```c
uint8_t encrypt(uint8_t data, uint8_t key) {
    return data ^ key;
}

uint8_t decrypt(uint8_t encrypted, uint8_t key) {
    return encrypted ^ key;  // XOR twice returns original
}
```

**4. Detect Changes**
```c
// Find which bits changed between old and new value
uint8_t changed_bits = old_value ^ new_value;
// Result: 1 where bits differ, 0 where they're same
```

---

## NOT (~)

**Symbol:** `~`
**What it does:** Inverts all bits (0→1, 1→0)

### Truth Table
```
~A  =  Result
~0  =    1
~1  =    0
```

### Visual Example

```
Original:   1010 0101  (0xA5)
          ~ ──────────
Result:     0101 1010  (0x5A)

Bit by bit:
~1 = 0
~0 = 1
~1 = 0
~0 = 1
~0 = 1
~1 = 0
~0 = 1
~1 = 0
```

### Important: NOT Operates on All Bits!

```c
uint8_t value = 0x05;  // 0000 0101
uint8_t result = ~value;

// Result = 1111 1010 = 0xFA (for 8-bit)
// NOT: 0xFFFFFFA (for 32-bit - inverts all 32 bits!)
```

### Common Use Cases

**1. Create Inverse Masks**
```c
// Clear bit 5 (set to 0)
register_value &= ~(1 << 5);

// Explanation:
// (1 << 5)  = 0000 0000 0000 0000 0000 0000 0010 0000
// ~(1 << 5) = 1111 1111 1111 1111 1111 1111 1101 1111
// AND with this mask clears only bit 5
```

**2. Clear Multiple Bits**
```c
// Clear bits 2, 4, and 6
register_value &= ~((1 << 2) | (1 << 4) | (1 << 6));

// (1<<2)|(1<<4)|(1<<6) = 0101 0100
// Inverted              = 1010 1011
// AND clears those 3 bits
```

**3. Invert All Bits**
```c
// Flip all bits in a byte
uint8_t inverted = ~value;
```

---

## Compound Assignment Operators

These combine an operation with assignment for shorter, clearer code.

### The Operators

```c
|=    OR and assign
&=    AND and assign
^=    XOR and assign
<<=   Left shift and assign
>>=   Right shift and assign
```

### Examples

**Without compound operators:**
```c
value = value | (1 << 5);
value = value & ~(1 << 3);
value = value ^ (1 << 7);
value = value << 2;
```

**With compound operators (cleaner):**
```c
value |= (1 << 5);   // Set bit 5
value &= ~(1 << 3);  // Clear bit 3
value ^= (1 << 7);   // Toggle bit 7
value <<= 2;         // Shift left by 2
```

---

## Common Patterns

### Pattern 1: Set a Bit
```c
register |= (1 << bit_number);

// Example:
GPIO_DATA |= (1 << 5);  // Set bit 5 to 1
```

### Pattern 2: Clear a Bit
```c
register &= ~(1 << bit_number);

// Example:
GPIO_DATA &= ~(1 << 5);  // Clear bit 5 to 0
```

### Pattern 3: Toggle a Bit
```c
register ^= (1 << bit_number);

// Example:
GPIO_DATA ^= (1 << 5);  // Flip bit 5
```

### Pattern 4: Check if Bit is Set
```c
if (register & (1 << bit_number)) {
    // Bit is set (1)
}

// Example:
if (GPIO_DATA & (1 << 5)) {
    // Pin 5 is HIGH
}
```

### Pattern 5: Check if Bit is Clear
```c
if (!(register & (1 << bit_number))) {
    // Bit is clear (0)
}

// Or:
if ((register & (1 << bit_number)) == 0) {
    // Bit is clear (0)
}
```

### Pattern 6: Set Multiple Bits
```c
register |= (1 << bit1) | (1 << bit2) | (1 << bit3);

// Example:
GPIO_DATA |= (1 << 2) | (1 << 5) | (1 << 7);
```

### Pattern 7: Clear Multiple Bits
```c
register &= ~((1 << bit1) | (1 << bit2));

// Example:
GPIO_DATA &= ~((1 << 2) | (1 << 5));
```

### Pattern 8: Modify Specific Bit Field
```c
// Clear field, then set new value
register = (register & ~FIELD_MASK) | (new_value << FIELD_SHIFT);

// Example: Set bits [6:4] to value 5
#define FIELD_MASK  (0x07 << 4)  // 0111 0000
#define FIELD_SHIFT 4
register = (register & ~FIELD_MASK) | (5 << FIELD_SHIFT);
```

---

## Real-World Examples

### Example 1: GPIO Pin Control (LPC1343)

```c
// Configure pin as output
GPIO3DIR |= (1 << 5);  // Set bit 5 in direction register

// Turn LED ON (active-low)
GPIO3DATA &= ~(1 << 5);  // Clear bit 5

// Turn LED OFF (active-low)
GPIO3DATA |= (1 << 5);   // Set bit 5

// Toggle LED
GPIO3DATA ^= (1 << 5);   // Flip bit 5

// Check if pin is high
if (GPIO3DATA & (1 << 5)) {
    // Pin is HIGH
}
```

### Example 2: Timer Configuration

```c
// Timer Control Register bit definitions
#define TMR_ENABLE    (1 << 0)  // Bit 0: Enable timer
#define TMR_INT_EN    (1 << 1)  // Bit 1: Enable interrupt
#define TMR_AUTO_RLD  (1 << 2)  // Bit 2: Auto-reload
#define TMR_RESET     (1 << 3)  // Bit 3: Reset counter

// Enable timer with interrupt and auto-reload
TMR_CTRL = TMR_ENABLE | TMR_INT_EN | TMR_AUTO_RLD;

// Or step by step:
TMR_CTRL = 0;              // Clear all
TMR_CTRL |= TMR_ENABLE;    // Enable timer
TMR_CTRL |= TMR_INT_EN;    // Enable interrupt
TMR_CTRL |= TMR_AUTO_RLD;  // Enable auto-reload

// Disable interrupt but keep timer running
TMR_CTRL &= ~TMR_INT_EN;

// Reset timer (pulse reset bit)
TMR_CTRL |= TMR_RESET;   // Set reset bit
TMR_CTRL &= ~TMR_RESET;  // Clear reset bit
```

### Example 3: Status Register Checking

```c
// Status register bit definitions
#define STATUS_READY    (1 << 0)
#define STATUS_ERROR    (1 << 1)
#define STATUS_BUSY     (1 << 2)
#define STATUS_COMPLETE (1 << 3)

uint8_t status = read_status_register();

// Check individual flags
if (status & STATUS_READY) {
    // Device is ready
}

if (status & STATUS_ERROR) {
    // Error occurred
}

// Check multiple conditions
if ((status & STATUS_READY) && !(status & STATUS_BUSY)) {
    // Ready and not busy - safe to proceed
}

// Clear error flag (write 1 to clear - common pattern)
write_status_register(STATUS_ERROR);
```

### Example 4: Configuration Bit Fields

```c
// Register layout: [7:6]=mode, [5:4]=speed, [3:0]=channel

#define MODE_SHIFT    6
#define MODE_MASK     (0x3 << MODE_SHIFT)    // 1100 0000
#define SPEED_SHIFT   4
#define SPEED_MASK    (0x3 << SPEED_SHIFT)   // 0011 0000
#define CHANNEL_MASK  0x0F                    // 0000 1111

// Set mode to 2, speed to 3, channel to 5
uint8_t config = 0;
config |= (2 << MODE_SHIFT);    // Mode = 2
config |= (3 << SPEED_SHIFT);   // Speed = 3
config |= 5;                     // Channel = 5
// Result: 1011 0101 = 0xB5

// Extract values
uint8_t mode    = (config & MODE_MASK) >> MODE_SHIFT;
uint8_t speed   = (config & SPEED_MASK) >> SPEED_SHIFT;
uint8_t channel = config & CHANNEL_MASK;

// Change just the speed to 1
config = (config & ~SPEED_MASK) | (1 << SPEED_SHIFT);
```

### Example 5: Interrupt Enable/Disable

```c
// Safe interrupt disable/enable pattern
void critical_section(void) {
    // Save interrupt state
    uint32_t irq_state = __get_PRIMASK();  // Read current state

    // Disable interrupts
    __disable_irq();

    // Critical code here
    // ...

    // Restore interrupt state
    __set_PRIMASK(irq_state);
}

// Or using inline assembly
#define DISABLE_INTERRUPTS()  __asm volatile ("cpsid i" : : : "memory")
#define ENABLE_INTERRUPTS()   __asm volatile ("cpsie i" : : : "memory")
```

---

## Bit Manipulation Cookbook

### Quick Reference Table

| Task | Code | Example |
|------|------|---------|
| **Set bit n** | `x \|= (1 << n)` | `x \|= (1 << 5)` |
| **Clear bit n** | `x &= ~(1 << n)` | `x &= ~(1 << 5)` |
| **Toggle bit n** | `x ^= (1 << n)` | `x ^= (1 << 5)` |
| **Check if bit n is set** | `x & (1 << n)` | `if (x & (1 << 5))` |
| **Check if bit n is clear** | `!(x & (1 << n))` | `if (!(x & (1 << 5)))` |
| **Set bits n to m** | `x \|= mask` | `x \|= 0x70` (bits 4-6) |
| **Clear bits n to m** | `x &= ~mask` | `x &= ~0x70` |
| **Isolate bit n** | `(x >> n) & 1` | `bit = (x >> 5) & 1` |
| **Create mask for bit n** | `(1 << n)` | `mask = (1 << 5)` |
| **Create mask for bits n to m** | `((1 << (m-n+1))-1) << n` | bits 3-5: `(0x7 << 3)` |
| **Extract n bits from position p** | `(x >> p) & ((1 << n) - 1)` | `(x >> 4) & 0x0F` |
| **Check if any bits in mask are set** | `x & mask` | `if (x & 0x70)` |
| **Check if all bits in mask are set** | `(x & mask) == mask` | `if ((x & 0x70) == 0x70)` |

### Macros for Reusable Code

```c
// Bit manipulation macros
#define BIT(n)              (1U << (n))
#define SET_BIT(reg, bit)   ((reg) |= BIT(bit))
#define CLEAR_BIT(reg, bit) ((reg) &= ~BIT(bit))
#define TOGGLE_BIT(reg, bit) ((reg) ^= BIT(bit))
#define READ_BIT(reg, bit)  (((reg) >> (bit)) & 1U)
#define CHECK_BIT(reg, bit) ((reg) & BIT(bit))

// Multi-bit field macros
#define MASK(width, shift)  (((1U << (width)) - 1) << (shift))
#define READ_FIELD(reg, mask, shift)  (((reg) & (mask)) >> (shift))
#define WRITE_FIELD(reg, mask, shift, val) \
    ((reg) = ((reg) & ~(mask)) | (((val) << (shift)) & (mask)))

// Usage examples:
SET_BIT(GPIO_DATA, 5);      // Set bit 5
CLEAR_BIT(GPIO_DATA, 3);    // Clear bit 3
if (CHECK_BIT(STATUS, 0)) { // Check bit 0
    // Bit is set
}

// Field operations (bits 6:4)
#define SPEED_MASK  MASK(3, 4)  // 3 bits starting at position 4
#define SPEED_SHIFT 4

uint8_t speed = READ_FIELD(config, SPEED_MASK, SPEED_SHIFT);
WRITE_FIELD(config, SPEED_MASK, SPEED_SHIFT, 3);
```

---

## Practice Problems

### Problem 1: LED Pattern
Write code to turn on LEDs 0, 2, 4, 6 (even numbered) and turn off LEDs 1, 3, 5, 7.

<details>
<summary>Solution</summary>

```c
// Method 1: Individual operations
GPIO_DATA |= (1 << 0);   // LED 0 on
GPIO_DATA &= ~(1 << 1);  // LED 1 off
GPIO_DATA |= (1 << 2);   // LED 2 on
GPIO_DATA &= ~(1 << 3);  // LED 3 off
GPIO_DATA |= (1 << 4);   // LED 4 on
GPIO_DATA &= ~(1 << 5);  // LED 5 off
GPIO_DATA |= (1 << 6);   // LED 6 on
GPIO_DATA &= ~(1 << 7);  // LED 7 off

// Method 2: Combined mask (better!)
#define EVEN_LEDS 0x55  // 0101 0101 (bits 0,2,4,6)
#define ODD_LEDS  0xAA  // 1010 1010 (bits 1,3,5,7)

GPIO_DATA = (GPIO_DATA & ~ODD_LEDS) | EVEN_LEDS;
// Or simpler:
GPIO_DATA = 0x55;  // If we can overwrite all bits
```
</details>

### Problem 2: Extract Nibbles
Given `uint8_t value = 0xA5`, extract the upper and lower nibbles (4-bit groups).

<details>
<summary>Solution</summary>

```c
uint8_t value = 0xA5;  // 1010 0101

// Extract upper nibble (bits 7:4)
uint8_t upper = (value >> 4) & 0x0F;  // Result: 0x0A (10)

// Extract lower nibble (bits 3:0)
uint8_t lower = value & 0x0F;  // Result: 0x05 (5)

// Alternative for upper nibble:
uint8_t upper_alt = value >> 4;  // Works if we don't care about upper bits
```
</details>

### Problem 3: Swap Nibbles
Swap the upper and lower nibbles of a byte. Example: `0xA5` → `0x5A`

<details>
<summary>Solution</summary>

```c
uint8_t swap_nibbles(uint8_t value) {
    return (value >> 4) | (value << 4);
}

// Example:
// value = 0xA5 = 1010 0101
// value >> 4 = 0000 1010
// value << 4 = 0101 0000
// OR result  = 0101 1010 = 0x5A
```
</details>

### Problem 4: Count Set Bits
Count how many bits are set to 1 in a byte.

<details>
<summary>Solution</summary>

```c
uint8_t count_set_bits(uint8_t value) {
    uint8_t count = 0;
    for (int i = 0; i < 8; i++) {
        if (value & (1 << i)) {
            count++;
        }
    }
    return count;
}

// Or using Brian Kernighan's algorithm (faster):
uint8_t count_set_bits_fast(uint8_t value) {
    uint8_t count = 0;
    while (value) {
        value &= (value - 1);  // Clear lowest set bit
        count++;
    }
    return count;
}

// Example: 0xA5 = 1010 0101 has 4 bits set
```
</details>

### Problem 5: Reverse Bits
Reverse the bit order in a byte. Example: `0xA5` (1010 0101) → `0xA5` (1010 0101)

<details>
<summary>Solution</summary>

```c
uint8_t reverse_bits(uint8_t value) {
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 1;              // Shift result left
        result |= (value & 1);     // Copy lowest bit
        value >>= 1;               // Shift value right
    }
    return result;
}

// Example: 0x85 = 1000 0101 → 1010 0001 = 0xA1
```
</details>

---

## Common Pitfalls and Tips

### Pitfall 1: Operator Precedence

**Problem:**
```c
if (flags & MASK == MASK)  // WRONG! == has higher precedence
```

**Solution:**
```c
if ((flags & MASK) == MASK)  // Correct - use parentheses
```

### Pitfall 2: Signed vs Unsigned

**Problem:**
```c
int8_t value = 0x80;  // -128 in signed
value >> 1;           // Arithmetic shift, fills with sign bit
// Result: 0xC0 (-64), not 0x40!
```

**Solution:**
```c
uint8_t value = 0x80;  // Use unsigned for bit operations
value >> 1;            // Logical shift, fills with 0
// Result: 0x40 (64)
```

### Pitfall 3: Forgetting NOT is Full-Width

**Problem:**
```c
uint32_t reg = 0x12345678;
reg &= ~0xFF;  // Trying to clear lower byte
// ~0xFF = 0xFFFFFF00 (32-bit), not 0x00 !
```

**Solution:**
```c
reg &= ~(0xFF);  // This works correctly
// Or:
reg &= 0xFFFFFF00;  // Be explicit
```

### Pitfall 4: Bit Shift Overflow

**Problem:**
```c
uint8_t value = 1 << 8;  // Shifts out of 8-bit range!
// Undefined behavior or wraps to 0
```

**Solution:**
```c
// Be aware of variable size
uint32_t value = 1UL << 31;  // OK for 32-bit
// Use UL suffix for unsigned long
```

### Tip 1: Use #define for Magic Numbers

**Bad:**
```c
GPIO_DATA |= 0x20;  // What is 0x20?
```

**Good:**
```c
#define LED_PIN 5
GPIO_DATA |= (1 << LED_PIN);  // Clear!
```

### Tip 2: Comment Bit Layouts

```c
// CTRL Register Layout:
// [7] - Enable
// [6] - Interrupt Enable
// [5:4] - Mode (00=off, 01=low, 10=med, 11=high)
// [3:0] - Channel (0-15)

#define CTRL_ENABLE     (1 << 7)
#define CTRL_INT_EN     (1 << 6)
#define CTRL_MODE_SHIFT 4
#define CTRL_MODE_MASK  (0x3 << CTRL_MODE_SHIFT)
```

### Tip 3: Volatile for Hardware Registers

```c
// Hardware registers should be volatile
#define GPIO_DATA (*(volatile uint32_t *)0x50000000)

// Prevents compiler optimization that might skip register access
```

---

## Summary

### The Six Operators

| Operator | Symbol | Purpose | Example |
|----------|--------|---------|---------|
| **Left Shift** | `<<` | Shift bits left, multiply by 2^n | `1 << 5 = 32` |
| **Right Shift** | `>>` | Shift bits right, divide by 2^n | `32 >> 2 = 8` |
| **AND** | `&` | Mask/extract bits | `x & 0x0F` |
| **OR** | `\|` | Set bits | `x \| (1<<5)` |
| **XOR** | `^` | Toggle/compare bits | `x ^ (1<<5)` |
| **NOT** | `~` | Invert all bits | `~0x0F` |

### Essential Patterns

```c
SET BIT:      register |= (1 << n);
CLEAR BIT:    register &= ~(1 << n);
TOGGLE BIT:   register ^= (1 << n);
CHECK BIT:    if (register & (1 << n))
```

### Why This Matters

In embedded programming, you're constantly:
- Controlling hardware by setting/clearing bits in registers
- Reading status by checking individual bits
- Configuring peripherals using bit fields
- Optimizing code size and speed

**Mastering bitwise operations is essential for embedded development!**

---

## Further Practice

Try these exercises with your LPC1343:

1. **Blink Pattern**: Create a running light pattern using bit shifts
2. **Button Debounce**: Use bit operations to implement button state machine
3. **PWM Simulation**: Toggle a pin using bit operations in a timer interrupt
4. **Status Display**: Use 8 LEDs to display a binary counter
5. **Configuration Menu**: Set/clear configuration bits based on button presses

---

## What's Next?

You now have the foundational skill for all embedded programming. Every chapter from here on uses bitwise operations to control hardware.

**Next Chapter:** [Chapter 2: Build Process](02-firmware-build-process.md) - Understand how your code becomes firmware that runs on the chip.

---

*Chapter 1 of the LPC1343 Embedded C Programming Guide*
