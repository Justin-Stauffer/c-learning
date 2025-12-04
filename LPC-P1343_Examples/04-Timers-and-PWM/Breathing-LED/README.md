# Breathing-LED

Chapter 4: Timers and PWM - Breathing LED Example

## What This Example Demonstrates

- PWM for smooth brightness transitions
- Timer interrupt for precise timing
- Gamma correction for perceived linear brightness
- Using multiple timers (CT32B0 for PWM, CT32B1 for timing)
- Smooth animation effects

## Hardware

- P1.6: PWM output (connect external LED with 330Ω resistor)
- P3.0-P3.3: Status LEDs (show breathing phase)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

The LED on P1.6 smoothly fades in and out like breathing:

1. **Inhale** (fade up): ~1.5 seconds
2. **Brief hold** at peak brightness: ~100ms
3. **Exhale** (fade down): ~1.5 seconds
4. **Rest** at minimum: ~300ms
5. Repeat

Status LEDs indicate the current phase.

## Hardware Connection

```
P1.6 ──────┬──── LED (+) ──── 330Ω ──── GND
           │
           └──── (or scope probe)
```

## Code Highlights

**Gamma correction table:**
```c
// Maps linear 0-100 to perceptually linear PWM values
const uint8_t gamma_table[101] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 3,
    // ... continues to 179 at index 100
};
```

**Applying gamma correction:**
```c
void pwm_set_duty_gamma(uint8_t linear_percent) {
    uint8_t corrected = gamma_table[linear_percent];
    pwm_set_duty(corrected);
}
```

**Breathing loop:**
```c
for (int duty = 0; duty <= 100; duty++) {
    pwm_set_duty_gamma(duty);
    delay_ms(15);  // 15ms × 101 steps = ~1.5s
}
```

## Why Gamma Correction?

Human eyes perceive brightness logarithmically. Without gamma correction:
- 0% to 50% looks like a huge jump
- 50% to 100% barely looks different

With gamma correction, each step looks equally spaced.

```
Linear PWM:
0%  ████████████████████████████████████████ 50%  ████████ 100%
    │←───── appears dramatic ────→│←─ subtle ─→│

Gamma-corrected:
0%  ████████████████ 50% ████████████████ 100%
    │←── even steps ──→│←── even steps ──→│
```

## Timer Usage

This example uses two timers:

| Timer | Purpose | Configuration |
|-------|---------|---------------|
| CT32B0 | PWM output on P1.6 | 1kHz, variable duty cycle |
| CT32B1 | Delay timing | 1ms interrupt tick |

## Variations to Try

1. Adjust `step_delay` for faster/slower breathing
2. Modify `pause_at_bottom` for different rhythm
3. Try without gamma correction to see the difference
4. Add multiple LEDs with offset phases
5. Make the breathing pattern respond to button input
