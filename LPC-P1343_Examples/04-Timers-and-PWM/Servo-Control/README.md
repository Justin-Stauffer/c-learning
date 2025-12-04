# Servo-Control

Chapter 4: Timers and PWM - Servo Control Example

## What This Example Demonstrates

- 50Hz PWM for standard hobby servos
- Prescaler for microsecond resolution
- Pulse width mapping (1-2ms for 0-180°)
- Angle to pulse width calculation
- Button-controlled position stepping

## Hardware

- P1.6: PWM output (servo signal wire - usually orange or white)
- P0.1: Button input (on-board)
- P3.0-P3.3: Status LEDs showing position
- Servo power: Connect servo power (red) to 5V, ground (brown/black) to GND

**Important:** Servos can draw significant current (500mA+). Use an external power supply for the servo, not the board's 3.3V regulator.

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: Servo sweeps from 0° to 180° and back
2. **Button press**: Steps through positions: 0° → 45° → 90° → 135° → 180° → repeat
3. **Status LEDs**: Indicate current position

## Hardware Connection

```
LPC1343                    Servo
─────────                  ─────
P1.6 ─────────────────────→ Signal (orange/white)
GND ──────────────────────→ Ground (brown/black)
                    5V ───→ Power (red)
                     │
              [External 5V supply recommended]
```

## Code Highlights

**Prescaler for microsecond timing:**
```c
TMR32B0PR = 71;  // 72MHz / 72 = 1MHz (1 tick = 1µs)
```

**50Hz period (20ms):**
```c
TMR32B0MR3 = 20000 - 1;  // 20,000 µs = 20 ms
```

**Angle to pulse width:**
```c
void servo_set_angle(uint16_t angle) {
    // Map 0-180° to 1000-2000 µs
    uint32_t pulse_us = 1000 + (angle * 1000) / 180;
    TMR32B0MR0 = pulse_us;
}
```

## Servo PWM Timing

```
Standard Hobby Servo Requirements:
- Frequency: 50 Hz (20 ms period)
- Pulse width: 1-2 ms

Position vs Pulse Width:
┌─────────────────────────────────────────┐
│ Position    Pulse Width    Our Value    │
├─────────────────────────────────────────┤
│   0°          1.0 ms        1000 µs    │
│  45°          1.25 ms       1250 µs    │
│  90°          1.5 ms        1500 µs    │
│ 135°          1.75 ms       1750 µs    │
│ 180°          2.0 ms        2000 µs    │
└─────────────────────────────────────────┘

PWM Signal at 90° (1.5ms pulse):
    1.5ms     18.5ms
    |←─→|     |←────→|
    ┌───┐                   ┌───┐
────┘   └───────────────────┘   └─────
    |←────── 20ms ─────────→|
```

## Servo Types

This code works with standard hobby servos:
- **Standard servos**: 0-180° rotation
- **Continuous rotation**: 1ms = full speed CW, 1.5ms = stop, 2ms = full speed CCW

Some servos may need different pulse ranges (e.g., 500-2500 µs). Adjust `SERVO_MIN_PULSE_US` and `SERVO_MAX_PULSE_US` if needed.

## Variations to Try

1. Change angle steps (10° instead of 45°)
2. Add smooth movement between positions
3. Control with potentiometer (requires ADC)
4. Add multiple servos on different pins
5. Create a scanning radar pattern
