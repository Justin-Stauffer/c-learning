# Combination Lock

Chapter 3: GPIO In-Depth - Combination Lock Example

## What This Example Demonstrates

- GPIO input polling
- Edge detection (detecting button press transitions)
- Sequence/state tracking
- Timeout-based logic
- Success/failure visual feedback
- Debouncing

## Hardware

- LEDs on P3.0-P3.3
- Button on P0.1 (directly on LPC-P1343 board)

## Building and Flashing

```bash
make clean
make
make flash
```

## How to Use

**To unlock:** Press the button 4 times quickly (within the timeout window).

**Progress indicator:** LEDs light up to show your progress:
- 1 press: LED0 on
- 2 presses: LED0 + LED1 on
- 3 presses: LED0 + LED1 + LED2 on
- 4 presses: SUCCESS!

**Success:** All LEDs flash 5 times slowly.

**Failure (timeout):** All LEDs flash 3 times quickly, then reset.

## Code Highlights

**Edge detection:**
```c
uint8_t newly_pressed = current_button && !last_button;
```

**Progress display:**
```c
uint8_t pattern = (1 << count) - 1;  // Creates 0001, 0011, 0111, 1111
```

**Timeout countdown:**
```c
if (sequence_count > 0) {
    timeout--;
    if (timeout == 0) {
        // Reset sequence
    }
}
```

## Variations to Try

1. Change sequence length
2. Require specific timing pattern (short-short-long-short)
3. Add sound feedback via buzzer
4. Store unlock count in RAM
