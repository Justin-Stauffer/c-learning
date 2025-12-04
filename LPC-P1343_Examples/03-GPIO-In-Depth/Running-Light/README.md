# Running Light

Chapter 3: GPIO In-Depth - Running Light Example

## What This Example Demonstrates

- GPIO output configuration
- IOCON pin function selection
- Bit shifting to create LED patterns
- Active-low LED handling
- Direction reversal at pattern boundaries

## Hardware

- LEDs on P3.0, P3.1, P3.2, P3.3 (directly on LPC-P1343 board)
- No button input required

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

A single LED "runs" back and forth across the 4 LEDs:

```
LED3  LED2  LED1  LED0
 O     O     O     *    <- Start
 O     O     *     O
 O     *     O     O
 *     O     O     O    <- Reverse direction
 O     *     O     O
 O     O     *     O
 O     O     O     *    <- Reverse direction
 ...
```

The effect looks like a "Knight Rider" scanner.

## Code Highlights

**Pattern creation using bit shift:**
```c
uint8_t pattern = (1 << position);
```

**Direction reversal:**
```c
if (position >= NUM_LEDS - 1) direction = -1;
if (position == 0) direction = 1;
```

**Active-low LED handling:**
```c
current |= LED_MASK;                  // All LEDs off
current &= ~(pattern & LED_MASK);     // Turn on selected LEDs
```

## Variations to Try

1. Change `DELAY_MS` for faster/slower movement
2. Try the `running_light_rotate()` function for an alternative implementation
3. Modify to have 2 LEDs chasing each other
