# Tone-Generator

Chapter 4: Timers and PWM - Tone Generator Example

## What This Example Demonstrates

- PWM for audio frequency generation
- Dynamic PWM frequency changes
- Musical note frequency definitions
- Melody playback with note duration
- Timer for precise note timing
- 50% duty cycle square wave audio

## Hardware

- P1.6: PWM output (connect piezo buzzer or small speaker)
- P0.1: Button to replay melody
- P3.0-P3.3: Status LEDs (visual note indicator)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: Plays a C major scale (C4 to C5)
2. **Melody**: Plays "Mary Had a Little Lamb"
3. **Wait**: All LEDs on, waiting for button press
4. **Button press**: Replays the melody

## Hardware Connection

```
P1.6 ──────┬──── Piezo buzzer (+) ──── GND
           │
           └──── (or speaker with series capacitor)

For speaker: P1.6 ── 10µF cap ── Speaker ── GND
```

**Note:** Piezo buzzers work best. For speakers, add a 10µF coupling capacitor to block DC.

## Code Highlights

**Note frequency definitions:**
```c
#define NOTE_C4    262
#define NOTE_D4    294
#define NOTE_E4    330
#define NOTE_G4    392
#define NOTE_REST  0
```

**Tone generation:**
```c
void tone(uint16_t frequency) {
    if (frequency == 0) {
        TMR32B0PWMC = 0;  // Silence
        return;
    }
    uint32_t period = SYSTEM_CLOCK / frequency;
    TMR32B0MR3 = period - 1;     // Period
    TMR32B0MR0 = period / 2;     // 50% duty (square wave)
    TMR32B0PWMC = (1 << 0);      // Enable PWM
}
```

**Melody data structure:**
```c
typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} Note;

const Note melody[] = {
    { NOTE_E4, 300 },
    { NOTE_D4, 300 },
    // ...
};
```

## Musical Note Frequencies

| Note | Frequency (Hz) | PWM Period @ 72MHz |
|------|----------------|---------------------|
| C4   | 262 Hz         | 274,809 ticks       |
| D4   | 294 Hz         | 244,898 ticks       |
| E4   | 330 Hz         | 218,182 ticks       |
| F4   | 349 Hz         | 206,304 ticks       |
| G4   | 392 Hz         | 183,673 ticks       |
| A4   | 440 Hz         | 163,636 ticks       |
| B4   | 494 Hz         | 145,749 ticks       |
| C5   | 523 Hz         | 137,667 ticks       |

## Square Wave Audio

```
50% Duty Cycle Square Wave:

    ┌────┐    ┌────┐    ┌────┐    ┌────┐
    │    │    │    │    │    │    │    │
────┘    └────┘    └────┘    └────┘    └────
    |←─T→|

Frequency = 1/T
At 440 Hz (A4): T = 2.27 ms
```

Square waves contain harmonics that give them a "buzzy" quality, which is why they work well with piezo buzzers.

## Timer Usage

| Timer | Purpose | Configuration |
|-------|---------|---------------|
| CT32B0 | Tone generation | Variable frequency PWM |
| CT32B1 | Note timing | 1ms interrupt tick |

## Variations to Try

1. Add more songs (define new melody arrays)
2. Implement tempo control (multiply all durations)
3. Add note envelope (attack/decay using duty cycle)
4. Create a simple piano with button inputs
5. Add vibrato effect (slight frequency modulation)
6. Play RTTTL ringtones (common ringtone format)
