# Timer-Delay

Chapter 4: Timers and PWM - Timer-Delay Example

## What This Example Demonstrates

- CT32B0 timer peripheral configuration
- Timer prescaler to achieve 1 MHz timer clock
- Match register for periodic 1ms interrupts
- NVIC interrupt enable for timer
- Interrupt handler implementation
- Blocking `delay_ms()` function
- Non-blocking delay using tick counter comparison

## Hardware

- LEDs on P3.0-P3.3

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup flash**: All 4 LEDs blink 6 times rapidly (using blocking delay)
2. **Running light**: Single LED moves across the 4 LEDs every 250ms (using non-blocking delay)

## Code Highlights

**Timer initialization:**
```c
TMR32B0PR = 71;           // 72MHz / 72 = 1MHz timer clock
TMR32B0MR0 = 999;         // Match after 1000 ticks = 1ms
TMR32B0MCR = 0x03;        // Interrupt + Reset on MR0
NVIC_ISER = (1 << 18);    // Enable CT32B0 interrupt
TMR32B0TCR = 0x01;        // Start timer
```

**Interrupt handler:**
```c
void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & (1 << 0)) {
        TMR32B0IR = (1 << 0);  // Clear flag (write 1 to clear)
        ms_ticks++;
    }
}
```

**Non-blocking delay pattern:**
```c
if ((get_ticks() - last_toggle) >= 250) {
    last_toggle = get_ticks();
    // Do periodic task
}
// CPU free to do other work here
```

## Key Concepts

### Why Use Hardware Timers?

Software delay loops (`while(count--)`) waste CPU cycles and vary with optimization settings. Hardware timers count in the background, leaving the CPU free to do other work.

### Timer Math

```
Timer Clock = System Clock / (Prescaler + 1)
            = 72 MHz / 72 = 1 MHz

Time per tick = 1 / 1 MHz = 1 µs

Match time = (Match Value + 1) × Time per tick
           = 1000 × 1 µs = 1 ms
```

## Variations to Try

1. Change LED blink rate by modifying `delay_ms()` parameter
2. Create multi-rate tasks (one at 100ms, another at 500ms)
3. Add button input checking in the main loop's free time
4. Implement a countdown timer with LED feedback
