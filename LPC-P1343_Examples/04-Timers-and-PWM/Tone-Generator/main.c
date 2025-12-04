/**
 * Chapter 4: Timers and PWM - Tone Generator Example
 *
 * Generates musical tones using PWM at audio frequencies.
 * Plays a simple melody ("Mary Had a Little Lamb").
 * Uses 50% duty cycle square wave for audio.
 *
 * Concepts demonstrated:
 *   - PWM for audio frequency generation
 *   - Dynamic frequency changes
 *   - Musical note frequencies
 *   - Melody playback with note duration
 *   - Timer-based delays
 *
 * Hardware:
 *   - P1.6: PWM output (connect piezo buzzer or speaker)
 *   - P0.1: Button to restart melody
 *   - P3.0-P3.3: Status LEDs (visual feedback)
 *
 * Build: make
 * Flash: make flash
 */

#include <stdint.h>

/*******************************************************************************
 * Register Definitions
 ******************************************************************************/

/* System Control */
#define SYSAHBCLKCTRL  (*((volatile uint32_t *)0x40048080))

/* IOCON */
#define IOCON_PIO0_1   (*((volatile uint32_t *)0x40044010))
#define IOCON_PIO1_6   (*((volatile uint32_t *)0x400440A4))
#define IOCON_PIO3_0   (*((volatile uint32_t *)0x40044084))
#define IOCON_PIO3_1   (*((volatile uint32_t *)0x40044088))
#define IOCON_PIO3_2   (*((volatile uint32_t *)0x4004409C))
#define IOCON_PIO3_3   (*((volatile uint32_t *)0x400440AC))

/* GPIO Port 0 (button) */
#define GPIO0DIR       (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA      (*((volatile uint32_t *)0x50003FFC))

/* GPIO Port 3 (status LEDs) */
#define GPIO3DIR       (*((volatile uint32_t *)0x50038000))
#define GPIO3DATA      (*((volatile uint32_t *)0x50033FFC))

/* CT32B0 Timer Registers (tone generation) */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))
#define TMR32B0MR3     (*((volatile uint32_t *)0x40014024))
#define TMR32B0PWMC    (*((volatile uint32_t *)0x40014074))

/* CT32B1 Timer Registers (delay timing) */
#define TMR32B1IR      (*((volatile uint32_t *)0x40018000))
#define TMR32B1TCR     (*((volatile uint32_t *)0x40018004))
#define TMR32B1PR      (*((volatile uint32_t *)0x4001800C))
#define TMR32B1MCR     (*((volatile uint32_t *)0x40018014))
#define TMR32B1MR0     (*((volatile uint32_t *)0x40018018))

/* NVIC */
#define NVIC_ISER      (*((volatile uint32_t *)0xE000E100))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define BUTTON_PIN     (1 << 1)

#define SYSTEM_CLOCK   72000000UL

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define CT32B0_CLK     (1 << 9)
#define CT32B1_CLK     (1 << 10)

/* CT32B1 IRQ number */
#define CT32B1_IRQn    19

/*******************************************************************************
 * Musical Note Frequencies (Hz)
 ******************************************************************************/

#define NOTE_REST  0
#define NOTE_C4    262
#define NOTE_D4    294
#define NOTE_E4    330
#define NOTE_F4    349
#define NOTE_G4    392
#define NOTE_A4    440
#define NOTE_B4    494
#define NOTE_C5    523
#define NOTE_D5    587
#define NOTE_E5    659

/*******************************************************************************
 * Note Structure and Melody
 ******************************************************************************/

typedef struct {
    uint16_t frequency;    /* Note frequency in Hz (0 = rest) */
    uint16_t duration_ms;  /* Note duration in milliseconds */
} Note;

/* "Mary Had a Little Lamb" */
const Note melody[] = {
    { NOTE_E4, 300 },   /* Ma- */
    { NOTE_D4, 300 },   /* -ry */
    { NOTE_C4, 300 },   /* had */
    { NOTE_D4, 300 },   /* a */
    { NOTE_E4, 300 },   /* lit- */
    { NOTE_E4, 300 },   /* -tle */
    { NOTE_E4, 600 },   /* lamb */

    { NOTE_D4, 300 },   /* lit- */
    { NOTE_D4, 300 },   /* -tle */
    { NOTE_D4, 600 },   /* lamb */

    { NOTE_E4, 300 },   /* lit- */
    { NOTE_G4, 300 },   /* -tle */
    { NOTE_G4, 600 },   /* lamb */

    { NOTE_E4, 300 },   /* Ma- */
    { NOTE_D4, 300 },   /* -ry */
    { NOTE_C4, 300 },   /* had */
    { NOTE_D4, 300 },   /* a */
    { NOTE_E4, 300 },   /* lit- */
    { NOTE_E4, 300 },   /* -tle */
    { NOTE_E4, 300 },   /* lamb */
    { NOTE_E4, 300 },   /* whose */

    { NOTE_D4, 300 },   /* fleece */
    { NOTE_D4, 300 },   /* was */
    { NOTE_E4, 300 },   /* white */
    { NOTE_D4, 300 },   /* as */
    { NOTE_C4, 900 },   /* snow */

    { NOTE_REST, 500 }, /* End pause */
    { 0, 0 }            /* End marker */
};

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

volatile uint32_t ms_ticks = 0;

/*******************************************************************************
 * Interrupt Handler
 ******************************************************************************/

void CT32B1_IRQHandler(void) {
    if (TMR32B1IR & (1 << 0)) {
        TMR32B1IR = (1 << 0);
        ms_ticks++;
    }
}

/*******************************************************************************
 * Timer Functions
 ******************************************************************************/

void delay_timer_init(void) {
    SYSAHBCLKCTRL |= CT32B1_CLK;

    TMR32B1TCR = 0x02;
    TMR32B1TCR = 0x00;

    TMR32B1PR = 71;      /* 1 MHz (1 µs per tick) */
    TMR32B1MR0 = 999;    /* 1 ms period */

    TMR32B1MCR = (1 << 0) | (1 << 1);
    TMR32B1IR = 0x1F;

    NVIC_ISER = (1 << CT32B1_IRQn);
    TMR32B1TCR = 0x01;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

/*******************************************************************************
 * LED Functions
 ******************************************************************************/

void leds_init(void) {
    SYSAHBCLKCTRL |= GPIO_CLK;

    IOCON_PIO3_0 = 0x01;
    IOCON_PIO3_1 = 0x01;
    IOCON_PIO3_2 = 0x01;
    IOCON_PIO3_3 = 0x01;

    GPIO3DIR |= LED_MASK;
    GPIO3DATA |= LED_MASK;
}

void set_leds(uint8_t pattern) {
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;
    current &= ~(pattern & LED_MASK);
    GPIO3DATA = current;
}

void show_note_visual(uint16_t freq) {
    /* Simple visualization: higher notes light more LEDs */
    uint8_t pattern = 0;
    if (freq == 0) {
        pattern = 0;
    } else if (freq < 300) {
        pattern = 0x01;  /* Low note */
    } else if (freq < 350) {
        pattern = 0x03;
    } else if (freq < 400) {
        pattern = 0x07;
    } else {
        pattern = 0x0F;  /* High note */
    }
    set_leds(pattern);
}

/*******************************************************************************
 * Button Functions
 ******************************************************************************/

void button_init(void) {
    IOCON_PIO0_1 = (0x01 << 0)
                 | (0x02 << 3)
                 | (0x01 << 5);
    GPIO0DIR &= ~BUTTON_PIN;
}

uint8_t button_pressed(void) {
    return !(GPIO0DATA & BUTTON_PIN);
}

/*******************************************************************************
 * Tone Functions
 ******************************************************************************/

void tone_init(void) {
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Configure P1.6 as CT32B0_MAT0 */
    IOCON_PIO1_6 = 0x02;

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    /* No prescaler for maximum frequency resolution */
    TMR32B0PR = 0;

    /* Initial: no tone */
    TMR32B0MR3 = 0xFFFFFFFF;
    TMR32B0MR0 = 0;
    TMR32B0MCR = (1 << 10);  /* Reset on MR3 */
    TMR32B0PWMC = 0;         /* PWM disabled initially */

    TMR32B0TCR = 0x01;
}

void tone(uint16_t frequency) {
    if (frequency == 0) {
        /* Silence: disable PWM output */
        TMR32B0PWMC = 0;
        return;
    }

    /* Calculate period for desired frequency
     * Period = SystemClock / Frequency
     */
    uint32_t period = SYSTEM_CLOCK / frequency;

    TMR32B0MR3 = period - 1;       /* Period */
    TMR32B0MR0 = period / 2;       /* 50% duty cycle (square wave) */
    TMR32B0PWMC = (1 << 0);        /* Enable PWM */
}

void no_tone(void) {
    TMR32B0PWMC = 0;
}

void play_note(uint16_t frequency, uint16_t duration_ms) {
    show_note_visual(frequency);
    tone(frequency);
    delay_ms(duration_ms);
    no_tone();
    delay_ms(50);  /* Small gap between notes */
    set_leds(0);
}

/*******************************************************************************
 * Melody Player
 ******************************************************************************/

void play_melody(const Note *notes) {
    while (notes->duration_ms != 0) {
        play_note(notes->frequency, notes->duration_ms);
        notes++;
    }
}

void play_scale(void) {
    /* Play a simple scale for testing */
    const uint16_t scale[] = {
        NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
    };

    for (int i = 0; i < 8; i++) {
        play_note(scale[i], 200);
    }
    delay_ms(500);
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    leds_init();
    button_init();
    delay_timer_init();
    tone_init();

    /* Play scale demo first */
    play_scale();

    while (1) {
        /* Play melody */
        play_melody(melody);

        /* Wait for button press to play again */
        set_leds(0x0F);  /* All LEDs on to indicate ready */

        while (!button_pressed()) {
            /* Wait for button */
        }

        /* Debounce and wait for release */
        delay_ms(50);
        while (button_pressed()) {}
        delay_ms(200);

        set_leds(0);
    }

    return 0;
}
