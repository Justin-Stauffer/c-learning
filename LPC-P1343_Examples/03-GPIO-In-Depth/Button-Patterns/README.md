# Button-Controlled Patterns

Chapter 3: GPIO In-Depth - Button-Controlled Patterns Example

## What This Example Demonstrates

- GPIO input configuration with internal pull-up
- GPIO falling-edge interrupt
- NVIC interrupt enable
- Interrupt-based debouncing
- Pattern state machine

## Hardware

- LEDs on P3.0-P3.3
- Button on P0.1 (directly on LPC-P1343 board)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

Press button to cycle through patterns:

1. **All Off** - All LEDs off
2. **All On** - All LEDs on
3. **Alternate** - LEDs 0,2 then 1,3 blink alternately
4. **Chase** - Single LED runs around

Each button press advances to next pattern.

## Code Highlights

**Interrupt configuration:**
```c
GPIO0IS &= ~BUTTON_PIN;   // Edge-sensitive
GPIO0IBE &= ~BUTTON_PIN;  // Single edge
GPIO0IEV &= ~BUTTON_PIN;  // Falling edge
GPIO0IE |= BUTTON_PIN;    // Enable
NVIC_ISER = (1 << 0);     // Enable in NVIC
```

**ISR with debounce:**
```c
void PIOINT0_IRQHandler(void) {
    GPIO0IE &= ~BUTTON_PIN;  // Disable during debounce
    current_pattern++;
    GPIO0IC = BUTTON_PIN;    // Clear flag
    delay(100000);           // Debounce wait
    GPIO0IE |= BUTTON_PIN;   // Re-enable
}
```
