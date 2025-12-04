# Binary Counter

Chapter 3: GPIO In-Depth - Binary Counter Example

## What This Example Demonstrates

- GPIO output configuration
- Binary number representation
- 4-bit counting (0-15)
- Pattern display on multiple LEDs

## Hardware

- LEDs on P3.0, P3.1, P3.2, P3.3 (directly on LPC-P1343 board)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

The 4 LEDs display a binary count from 0 to 15:

```
Count  LED3 LED2 LED1 LED0
  0      O    O    O    O
  1      O    O    O    *
  2      O    O    *    O
  3      O    O    *    *
  4      O    *    O    O
  5      O    *    O    *
  ...
 14      *    *    *    O
 15      *    *    *    *
  0      O    O    O    O   (wraps)
```

## Code Highlights

The count value IS the LED pattern:
```c
display_binary(count);  // count directly maps to LED bits
```

## Variations to Try

1. Count down instead of up
2. Count by 2s (even numbers only)
3. Add button to pause/resume counting
