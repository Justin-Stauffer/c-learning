# Chapter 1: Bitwise Operations Example

This example accompanies **Chapter 1: Bitwise Operations** from the learning docs.

## What This Example Demonstrates

This program demonstrates all the key bitwise operations covered in Chapter 1:

| Operation | Code Pattern | Used In |
|-----------|--------------|---------|
| **SET** a bit | `reg \|= (1 << n)` | Turning LEDs off (active-low) |
| **CLEAR** a bit | `reg &= ~(1 << n)` | Turning LEDs on (active-low) |
| **TOGGLE** a bit | `reg ^= (1 << n)` | Toggle demo pattern |
| **CHECK** a bit | `if (reg & (1 << n))` | Direction checking |
| **LEFT SHIFT** | `value << n` | Running light pattern |
| **RIGHT SHIFT** | `value >> n` | (implicit in direction logic) |

## LED Patterns

The program cycles through 5 different patterns:

### 1. Running Light (Knight Rider)
A single lit LED moves back and forth across the pins.
- **Demonstrates:** Shift operators (`<<`, `>>`)
- **Concept:** Moving a bit position left and right

### 2. Binary Counter
Counts from 0 to 15 (0b0000 to 0b1111).
- **Demonstrates:** All 16 possible 4-bit patterns
- **Concept:** Binary representation on LEDs

### 3. Toggle Demo
Each LED toggles at a different rate (1x, 2x, 4x, 8x).
- **Demonstrates:** XOR toggle (`^=`)
- **Concept:** Flipping bits without knowing current state

### 4. Alternating Pattern
Switches between even bits (0,2) and odd bits (1,3).
- **Demonstrates:** Bit masks for groups
- **Concept:** Selecting multiple bits at once

### 5. Fill and Empty
LEDs light up one by one, then turn off one by one.
- **Demonstrates:** OR to set (`|=`), AND with NOT to clear (`&= ~`)
- **Concept:** Building and clearing bit patterns

## Hardware Setup

**Minimum:** LPC-P1343 board with built-in LED on P3.0
- You'll see bit 0 of each pattern

**Full Effect:** Connect 4 LEDs to P3.0, P3.1, P3.2, P3.3
- See the complete 4-bit patterns

### LED Connection (if adding external LEDs)
```
P3.0 ──────┬──── LED0 ──── GND
           │
P3.1 ──────┬──── LED1 ──── GND
           │
P3.2 ──────┬──── LED2 ──── GND
           │
P3.3 ──────┬──── LED3 ──── GND
```

Note: LEDs are active-low on this board (0 = ON, 1 = OFF).
The code handles this inversion automatically.

## Building and Flashing

```bash
# Build the project
make

# Flash to the board (requires OpenOCD + ST-Link)
make flash

# Clean build files
make clean

# Show memory usage
make size
```

## Code Highlights

### Bit Manipulation Macros (main.c lines 44-56)
```c
#define BIT(n)              (1U << (n))
#define SET_BIT(reg, n)     ((reg) |= BIT(n))
#define CLEAR_BIT(reg, n)   ((reg) &= ~BIT(n))
#define TOGGLE_BIT(reg, n)  ((reg) ^= BIT(n))
#define CHECK_BIT(reg, n)   ((reg) & BIT(n))
```

### Running Light Pattern (uses shifts)
```c
uint8_t pattern = (1 << position);  // Create pattern at position
position += direction;               // Move position
```

### Toggle at Different Rates (uses XOR and bit checks)
```c
led_toggle(LED0_PIN);                    // Toggle every time
if ((i & BIT(0)) == 0)                   // Every 2nd time
    led_toggle(LED1_PIN);
if ((i & (BIT(0) | BIT(1))) == 0)        // Every 4th time
    led_toggle(LED2_PIN);
```

### Building Patterns with OR, Clearing with AND
```c
pattern |= (1 << i);              // Add a bit (fill)
pattern &= ~(1 << (NUM_LEDS-1-i)); // Remove a bit (empty)
```

## Related Reading

- **Chapter 1: Bitwise Operations** (`learning-docs/01-bitwise-operations.md`)
  - Section: "Common Patterns" - explains the four essential operations
  - Section: "Real-World Examples" - GPIO pin control examples
  - Section: "Bit Manipulation Cookbook" - quick reference table

## Files

| File | Description |
|------|-------------|
| `main.c` | Main program with all pattern demonstrations |
| `startup_lpc1343_gcc.s` | Startup code (vector table, init) |
| `lpc1343_flash.ld` | Linker script (memory layout) |
| `Makefile` | Build configuration |

## Expected Behavior

When running, you should see:
1. Single LED moving back and forth (3 cycles)
2. Binary counting 0-15 (2 cycles)
3. LEDs toggling at different rates (32 iterations)
4. Alternating even/odd LEDs (4 cycles)
5. LEDs filling up then emptying (2 cycles)
6. Repeat forever

If you only have one LED on P3.0, you'll see it blink in various patterns corresponding to bit 0 of each demonstration.
