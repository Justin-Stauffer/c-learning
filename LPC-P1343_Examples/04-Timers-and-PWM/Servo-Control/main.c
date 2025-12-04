/**
 * Chapter 4: Timers and PWM - Servo Control Example
 *
 * Controls a standard hobby servo motor using PWM.
 * Servos require 50Hz PWM with 1-2ms pulse width for 0-180 degrees.
 * Button press steps through preset positions.
 *
 * Concepts demonstrated:
 *   - 50Hz PWM for servo control
 *   - Pulse width calculation (1ms to 2ms)
 *   - Angle to pulse width mapping
 *   - Prescaler for microsecond resolution
 *
 * Hardware:
 *   - P1.6: PWM output (connect servo signal wire)
 *   - P0.1: Button input (on-board)
 *   - P3.0-P3.3: Status LEDs showing current position
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

/* CT32B0 Timer Registers */
#define TMR32B0IR      (*((volatile uint32_t *)0x40014000))
#define TMR32B0TCR     (*((volatile uint32_t *)0x40014004))
#define TMR32B0PR      (*((volatile uint32_t *)0x4001400C))
#define TMR32B0MCR     (*((volatile uint32_t *)0x40014014))
#define TMR32B0MR0     (*((volatile uint32_t *)0x40014018))
#define TMR32B0MR3     (*((volatile uint32_t *)0x40014024))
#define TMR32B0PWMC    (*((volatile uint32_t *)0x40014074))

/*******************************************************************************
 * Configuration
 ******************************************************************************/

#define LED_MASK       0x0F
#define BUTTON_PIN     (1 << 1)

#define SYSTEM_CLOCK   72000000UL

/* Servo timing constants (in microseconds) */
#define SERVO_PERIOD_US    20000   /* 50Hz = 20ms period */
#define SERVO_MIN_PULSE_US 1000    /* 1ms = 0 degrees */
#define SERVO_MAX_PULSE_US 2000    /* 2ms = 180 degrees */
#define SERVO_CENTER_US    1500    /* 1.5ms = 90 degrees */

/* Clock enable bits */
#define GPIO_CLK       (1 << 6)
#define CT32B0_CLK     (1 << 9)

/* Preset servo positions */
#define NUM_POSITIONS  5
const uint16_t servo_angles[NUM_POSITIONS] = { 0, 45, 90, 135, 180 };

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

uint8_t current_position = 2;  /* Start at 90 degrees (center) */

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

void delay(volatile uint32_t count) {
    while (count > 0) count--;
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

void show_position(uint8_t pos) {
    /* Show position on status LEDs:
     * Position 0 (0°):   LED0 only
     * Position 1 (45°):  LED1 only
     * Position 2 (90°):  LED0 + LED3 (center indicator)
     * Position 3 (135°): LED2 only
     * Position 4 (180°): LED3 only
     */
    uint8_t pattern = 0;
    switch (pos) {
        case 0: pattern = 0x01; break;  /* 0° - far left */
        case 1: pattern = 0x02; break;  /* 45° */
        case 2: pattern = 0x09; break;  /* 90° - center (LEDs 0 and 3) */
        case 3: pattern = 0x04; break;  /* 135° */
        case 4: pattern = 0x08; break;  /* 180° - far right */
    }
    uint32_t current = GPIO3DATA;
    current |= LED_MASK;
    current &= ~pattern;
    GPIO3DATA = current;
}

/*******************************************************************************
 * Button Functions
 ******************************************************************************/

void button_init(void) {
    IOCON_PIO0_1 = (0x01 << 0)    /* FUNC = GPIO */
                 | (0x02 << 3)    /* MODE = Pull-up */
                 | (0x01 << 5);   /* HYS = enabled */

    GPIO0DIR &= ~BUTTON_PIN;
}

uint8_t button_pressed(void) {
    return !(GPIO0DATA & BUTTON_PIN);
}

/*******************************************************************************
 * Servo Functions
 ******************************************************************************/

void servo_init(void) {
    SYSAHBCLKCTRL |= CT32B0_CLK;

    /* Configure P1.6 as CT32B0_MAT0 */
    IOCON_PIO1_6 = 0x02;

    TMR32B0TCR = 0x02;
    TMR32B0TCR = 0x00;

    /* Prescaler for 1 microsecond resolution:
     * Timer clock = 72 MHz / 72 = 1 MHz (1 tick = 1 µs)
     */
    TMR32B0PR = 71;

    /* Period: 20000 µs = 20 ms = 50 Hz */
    TMR32B0MR3 = SERVO_PERIOD_US - 1;

    /* Initial position: center (1500 µs) */
    TMR32B0MR0 = SERVO_CENTER_US;

    /* Reset on MR3 match */
    TMR32B0MCR = (1 << 10);

    /* Enable PWM on channel 0 */
    TMR32B0PWMC = (1 << 0);

    /* Start timer */
    TMR32B0TCR = 0x01;
}

void servo_set_angle(uint16_t angle) {
    /* Clamp angle to 0-180 */
    if (angle > 180) angle = 180;

    /* Map 0-180 degrees to 1000-2000 µs pulse width
     * pulse = min + (angle / 180) * (max - min)
     * pulse = 1000 + (angle * 1000) / 180
     */
    uint32_t pulse_us = SERVO_MIN_PULSE_US +
                        ((uint32_t)angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / 180;

    TMR32B0MR0 = pulse_us;
}

void servo_set_pulse_us(uint16_t pulse_us) {
    /* Clamp to valid servo range */
    if (pulse_us < SERVO_MIN_PULSE_US) pulse_us = SERVO_MIN_PULSE_US;
    if (pulse_us > SERVO_MAX_PULSE_US) pulse_us = SERVO_MAX_PULSE_US;

    TMR32B0MR0 = pulse_us;
}

/*******************************************************************************
 * Demo Functions
 ******************************************************************************/

void sweep_demo(void) {
    /* Sweep servo from 0 to 180 and back */
    for (int angle = 0; angle <= 180; angle += 5) {
        servo_set_angle(angle);
        delay(50000);
    }
    delay(200000);
    for (int angle = 180; angle >= 0; angle -= 5) {
        servo_set_angle(angle);
        delay(50000);
    }
    delay(200000);
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

int main(void) {
    uint8_t last_button = 0;

    leds_init();
    button_init();
    servo_init();

    /* Initial position */
    servo_set_angle(servo_angles[current_position]);
    show_position(current_position);

    /* Startup demo: sweep once */
    sweep_demo();

    /* Return to initial position */
    servo_set_angle(servo_angles[current_position]);
    show_position(current_position);

    while (1) {
        uint8_t current_button = button_pressed();

        /* Edge detection */
        if (current_button && !last_button) {
            delay(50000);  /* Debounce */

            if (button_pressed()) {
                /* Advance to next position */
                current_position++;
                if (current_position >= NUM_POSITIONS) {
                    current_position = 0;
                }

                /* Update servo and LEDs */
                servo_set_angle(servo_angles[current_position]);
                show_position(current_position);

                /* Wait for release */
                while (button_pressed()) {}
                delay(50000);
            }
        }

        last_button = current_button;
    }

    return 0;
}
