# LED-Dimmer

Chapter 4: Timers and PWM - LED Dimmer Example

## What This Example Demonstrates

- PWM output configuration on CT32B0_MAT0 (P1.6)
- IOCON pin function selection for timer match output
- Period control with MR3
- Duty cycle control with MR0
- PWM control register (PWMC) enable
- Button input for brightness control

## Hardware

- P1.6: PWM output (connect external LED with 330Ω resistor, or use oscilloscope)
- P0.1: Button input (on-board)
- P3.0-P3.3: Status LEDs showing current brightness level

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: PWM output at 50% duty cycle, 2 status LEDs lit
2. **Button press**: Cycles through brightness levels:
   - 0% (off) - 0 status LEDs
   - 25% - 1 status LED
   - 50% - 2 status LEDs
   - 75% - 3 status LEDs
   - 100% (full on) - 4 status LEDs
3. After 100%, wraps back to 0%

## Hardware Connection

```
P1.6 ──────┬──── LED (+) ──── 330Ω ──── GND
           │
           └──── Oscilloscope (optional)
```

## Code Highlights

**Pin configuration for PWM:**
```c
IOCON_PIO1_6 = 0x02;  // FUNC = CT32B0_MAT0
```

**PWM initialization:**
```c
pwm_period = SYSTEM_CLOCK / frequency;  // 72000 for 1kHz
TMR32B0MR3 = pwm_period - 1;            // Period register
TMR32B0MR0 = pwm_period / 2;            // 50% duty cycle
TMR32B0MCR = (1 << 10);                 // Reset on MR3
TMR32B0PWMC = (1 << 0);                 // Enable PWM ch0
TMR32B0TCR = 0x01;                      // Start
```

**Setting duty cycle:**
```c
void pwm_set_duty(uint8_t percent) {
    uint32_t duty = (pwm_period * percent) / 100;
    TMR32B0MR0 = duty;
}
```

## PWM Timing Diagram

```
PWM Period (MR3)
|←─────────────────→|

┌────────────┐      ┌────────────┐
│            │      │            │
│    HIGH    │ LOW  │    HIGH    │
└────────────┘──────└────────────┘
             ↑
          TC = MR0
        (output LOW)

Duty Cycle = MR0 / MR3 × 100%
```

## Key Concepts

### PWM Resolution

At 72 MHz with 1 kHz PWM:
- Period = 72,000 ticks
- Resolution = ~16.1 bits (65536 possible duty levels)
- Step size = 100% / 72000 = 0.0014%

### Why P1.6?

The LPC1343 has fixed pin assignments for timer outputs. CT32B0_MAT0 can only appear on P1.6 when IOCON is set to function 0x02.

## Variations to Try

1. Change PWM frequency (try 100 Hz vs 20 kHz)
2. Add smooth brightness transitions instead of step changes
3. Use analog input (potentiometer) to control brightness
4. Implement multiple PWM channels for RGB LED
