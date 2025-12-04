# Chapter 3 Example Specifications

Reference file for creating the GPIO example projects. Read this before creating each example.

## Common Files Needed Per Project

Each example folder needs:
1. `main.c` - The example code
2. `Makefile` - Build configuration (copy from 00-Getting-Started, change PROJECT name)
3. `startup_lpc1343_gcc.s` - Copy from 00-Getting-Started
4. `lpc1343_flash.ld` - Copy from 00-Getting-Started
5. `README.md` - Brief description of the example

## Hardware Configuration (All Examples)

```
LEDs (Active-Low - 0=ON, 1=OFF):
  P3.0 - LED0
  P3.1 - LED1
  P3.2 - LED2
  P3.3 - LED3
  LED_MASK = 0x0F

Button (Active-Low - 0=pressed, 1=released):
  P0.1 - Main button (directly on LPC-P1343 board)
  BUTTON_PIN = (1 << 1)

Optional additional buttons (if connecting external):
  P2.0 - BTN0
  P2.1 - BTN1
  P2.2 - BTN2
  P2.3 - BTN3
```

## Register Addresses (Copy to each main.c)

```c
/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* IOCON - Pin Configuration */
#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 0 (for button) */
#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))
#define GPIO0IS        (*((volatile uint32_t *)0x50008004))
#define GPIO0IBE       (*((volatile uint32_t *)0x50008008))
#define GPIO0IEV       (*((volatile uint32_t *)0x5000800C))
#define GPIO0IE        (*((volatile uint32_t *)0x50008010))
#define GPIO0MIS       (*((volatile uint32_t *)0x50008018))
#define GPIO0IC        (*((volatile uint32_t *)0x5000801C))

/* GPIO Port 3 (for LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))
```

## Project Naming Convention

- Running-Light: `PROJECT = lpc1343_running_light`
- Binary-Counter: `PROJECT = lpc1343_binary_counter`
- Button-Patterns: `PROJECT = lpc1343_button_patterns`
- Combination-Lock: `PROJECT = lpc1343_combination_lock`

---

## Example 1: Running-Light

**Status: CREATED**

**Concepts:** GPIO output, bit shifting, direction control

**Behavior:**
- Single LED moves back and forth: 0→1→2→3→2→1→0→...
- "Knight Rider" effect

**Key code patterns:**
```c
uint8_t position = 0;
int8_t direction = 1;
pattern = (1 << position);
position += direction;
if (position >= NUM_LEDS-1) direction = -1;
if (position == 0) direction = 1;
```

---

## Example 2: Binary-Counter

**Status: NOT CREATED**

**Concepts:** GPIO output, binary representation, counting

**Behavior:**
- Count from 0 to 15 on 4 LEDs
- Each LED represents a bit (LED0=bit0, LED3=bit3)
- 0000 → 0001 → 0010 → ... → 1111 → 0000

**Key code patterns:**
```c
uint8_t count = 0;
while (1) {
    set_led_pattern(count);  // count IS the pattern
    delay(DELAY_MS);
    count++;
    if (count > 15) count = 0;
}
```

---

## Example 3: Button-Patterns

**Status: NOT CREATED**

**Concepts:** GPIO input, interrupts, debouncing, state machines

**Behavior:**
- Button press cycles through patterns:
  1. All LEDs off
  2. All LEDs on
  3. Alternating (0101 / 1010)
  4. Chase pattern
- Uses GPIO interrupt on P0.1

**Key code patterns:**
```c
typedef enum {
    PATTERN_OFF, PATTERN_ON, PATTERN_ALTERNATE, PATTERN_CHASE
} Pattern;

volatile Pattern current_pattern = PATTERN_OFF;

void PIOINT0_IRQHandler(void) {
    if (GPIO0MIS & BUTTON_PIN) {
        current_pattern = (current_pattern + 1) % NUM_PATTERNS;
        GPIO0IC = BUTTON_PIN;  // Clear interrupt
    }
}

// In main loop: switch on current_pattern
```

**Interrupt setup:**
```c
GPIO0IS &= ~BUTTON_PIN;   // Edge-sensitive
GPIO0IBE &= ~BUTTON_PIN;  // Single edge
GPIO0IEV &= ~BUTTON_PIN;  // Falling edge
GPIO0IC = BUTTON_PIN;     // Clear pending
GPIO0IE |= BUTTON_PIN;    // Enable
NVIC_ISER = (1 << 0);     // Enable PIOINT0 in NVIC (IRQ 0)
```

---

## Example 4: Combination-Lock

**Status: NOT CREATED**

**Concepts:** GPIO input, state machines, sequences, edge detection

**Behavior:**
- Correct sequence: Button must be pressed 4 times in specific timing/pattern
- Simple version: Just press button 4 times correctly
- Success: All LEDs flash
- Wrong: Quick error flash, reset sequence

**For single-button version:**
- Press pattern: short-short-long-short (like morse code)
- Or: 4 quick presses within time window

**For multi-button version (if 4 buttons available):**
- Correct sequence: BTN0 → BTN1 → BTN2 → BTN3
- Wrong button resets sequence

**Key code patterns:**
```c
#define SEQUENCE_LENGTH 4
uint8_t sequence_index = 0;
uint8_t last_button_state = 0;

// Edge detection
uint8_t current = read_button();
uint8_t newly_pressed = current & ~last_button_state;
last_button_state = current;

if (newly_pressed) {
    if (correct) {
        sequence_index++;
        if (sequence_index >= SEQUENCE_LENGTH) {
            // SUCCESS - flash all LEDs
            sequence_index = 0;
        }
    } else {
        // FAIL - error flash, reset
        sequence_index = 0;
    }
}
```

---

## Makefile Template

Copy from `00-Getting-Started/Makefile` and change:
- Line 2: Comment to match example name
- Line 14: `PROJECT = lpc1343_<example_name>`

---

## README Template for Each Example

```markdown
# <Example Name>

Chapter 3: GPIO In-Depth - <Example Name>

## What This Example Demonstrates

- Bullet points of concepts

## Hardware

- LEDs on P3.0-P3.3
- Button on P0.1 (if used)

## Building and Flashing

\`\`\`bash
make clean
make
make flash
\`\`\`

## Expected Behavior

Description of what you should see...

## Code Highlights

Key sections to study in main.c...
```
