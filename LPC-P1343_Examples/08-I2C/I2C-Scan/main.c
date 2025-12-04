/**************************************************
 * LPC1343 I2C Example: Bus Scanner
 * Chapter 8: I2C Communication
 *
 * Scans the I2C bus for connected devices and
 * provides visual feedback via LED blinks.
 *
 * Hardware:
 *   I2C Bus:
 *     P0.4 (SCL) → SCL on all I2C devices
 *     P0.5 (SDA) → SDA on all I2C devices
 *     3.3V → VCC
 *     GND → GND
 *
 *   LED: P0.7 (onboard, active low)
 *
 * Result:
 *   LED blinks N times for each device found,
 *   where N = lower nibble of address.
 *   Example: Device at 0x76 → blinks 6 times
 *
 * Common I2C Addresses (SunFounder kit):
 *   0x76/0x77 - BMP280 (pressure sensor)
 *   0x68/0x69 - MPU6050 (accelerometer)
 *   0x3C/0x3D - SSD1306 (OLED display)
 *   0x48      - PCF8591 (ADC/DAC)
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * System Control Registers
 *------------------------------------------------*/
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))

/*--------------------------------------------------
 * IOCON Registers (Pin Configuration)
 *------------------------------------------------*/
#define IOCON_PIO0_4    (*((volatile uint32_t *)0x40044030))  /* SCL */
#define IOCON_PIO0_5    (*((volatile uint32_t *)0x40044034))  /* SDA */

/*--------------------------------------------------
 * GPIO Registers
 *------------------------------------------------*/
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))

/*--------------------------------------------------
 * I2C Registers
 *------------------------------------------------*/
#define I2C0CONSET      (*((volatile uint32_t *)0x40000000))
#define I2C0CONCLR      (*((volatile uint32_t *)0x40000018))
#define I2C0STAT        (*((volatile uint32_t *)0x40000004))
#define I2C0DAT         (*((volatile uint32_t *)0x40000008))
#define I2C0SCLH        (*((volatile uint32_t *)0x40000010))
#define I2C0SCLL        (*((volatile uint32_t *)0x40000014))

/*--------------------------------------------------
 * I2C Control Bits
 *------------------------------------------------*/
#define I2C_AA          (1 << 2)   /* Assert Acknowledge */
#define I2C_SI          (1 << 3)   /* Interrupt flag */
#define I2C_STO         (1 << 4)   /* STOP condition */
#define I2C_STA         (1 << 5)   /* START condition */
#define I2C_I2EN        (1 << 6)   /* I2C Enable */

/*--------------------------------------------------
 * I2C Status Codes
 *------------------------------------------------*/
#define I2C_START_SENT      0x08
#define I2C_REP_START_SENT  0x10
#define I2C_SLA_W_ACK       0x18   /* Address+Write sent, ACK received */
#define I2C_SLA_W_NACK      0x20   /* Address+Write sent, NACK received */

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7

/*--------------------------------------------------
 * Simple Delay Function
 *------------------------------------------------*/
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

/*--------------------------------------------------
 * LED Control
 *------------------------------------------------*/
static void led_on(void) {
    GPIO0DATA &= ~(1 << LED_PIN);
}

static void led_off(void) {
    GPIO0DATA |= (1 << LED_PIN);
}

static void led_blink(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        led_on();
        delay(200000);
        led_off();
        delay(200000);
    }
}

/*--------------------------------------------------
 * Wait for I2C Interrupt Flag
 *------------------------------------------------*/
static void i2c_wait(void) {
    while ((I2C0CONSET & I2C_SI) == 0);
}

/*--------------------------------------------------
 * Initialize I2C
 *
 * Configuration:
 * - Standard mode (100 kHz)
 * - P0.4 = SCL, P0.5 = SDA
 *------------------------------------------------*/
static void i2c_init(void) {
    /* Enable I2C peripheral clock */
    SYSAHBCLKCTRL |= (1 << 5);   /* Bit 5 = I2C clock */

    /* De-assert I2C reset */
    PRESETCTRL |= (1 << 1);     /* Bit 1 = I2C reset control */

    /* Configure P0.4 as SCL and P0.5 as SDA
     *
     * IOCON bit fields:
     *   [2:0] FUNC = 0x01 (I2C function)
     *   [7:3] Standard I2C mode
     *   [8]   I2CMODE = 0 (Standard/Fast mode I2C)
     */
    IOCON_PIO0_4 = 0x01;        /* SCL function */
    IOCON_PIO0_5 = 0x01;        /* SDA function */

    /* Set I2C clock rate for 100 kHz
     *
     * I2C clock = PCLK / (SCLH + SCLL)
     * Assuming PCLK = 72 MHz:
     *   100 kHz = 72 MHz / 720
     *   SCLH = 360, SCLL = 360
     */
    I2C0SCLH = 360;
    I2C0SCLL = 360;

    /* Clear all flags and enable I2C */
    I2C0CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;
    I2C0CONSET = I2C_I2EN;
}

/*--------------------------------------------------
 * Probe I2C Address
 *
 * Returns: 1 if device responds, 0 if no response
 *
 * Protocol:
 * 1. Send START
 * 2. Send address + Write bit
 * 3. Check for ACK
 * 4. Send STOP
 *------------------------------------------------*/
static uint8_t i2c_probe(uint8_t address) {
    uint8_t status;
    uint8_t found = 0;

    /* Send START condition */
    I2C0CONSET = I2C_STA;
    i2c_wait();

    status = I2C0STAT;
    if (status != I2C_START_SENT && status != I2C_REP_START_SENT) {
        /* START failed */
        I2C0CONCLR = I2C_SI | I2C_STA;
        I2C0CONSET = I2C_STO;
        return 0;
    }

    /* Clear START flag */
    I2C0CONCLR = I2C_STA;

    /* Send address with Write bit (address << 1 | 0) */
    I2C0DAT = (address << 1) | 0;
    I2C0CONCLR = I2C_SI;
    i2c_wait();

    status = I2C0STAT;
    if (status == I2C_SLA_W_ACK) {
        /* Device responded with ACK */
        found = 1;
    }

    /* Send STOP condition */
    I2C0CONSET = I2C_STO;
    I2C0CONCLR = I2C_SI;

    /* Wait for STOP to complete */
    while (I2C0CONSET & I2C_STO);

    return found;
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint8_t addr;
    uint8_t devices_found = 0;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);
    led_off();

    /* Initialize I2C */
    i2c_init();

    /* Initial pause - fast blinks to indicate scan starting */
    for (int i = 0; i < 5; i++) {
        led_on();
        delay(50000);
        led_off();
        delay(50000);
    }
    delay(500000);

    /* Scan all valid I2C addresses (0x08 to 0x77)
     *
     * Addresses 0x00-0x07 and 0x78-0x7F are reserved
     */
    for (addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_probe(addr)) {
            devices_found++;

            /* Visual feedback: blink lower nibble times */
            uint8_t blinks = addr & 0x0F;
            if (blinks == 0) blinks = 16;  /* Address ends in 0 */

            led_blink(blinks);
            delay(1000000);  /* Pause between devices */
        }
    }

    /* Indicate scan complete */
    delay(1000000);

    /* Final status: long blink for each device found */
    while (1) {
        if (devices_found > 0) {
            /* Slow blinks = number of devices */
            for (uint8_t i = 0; i < devices_found; i++) {
                led_on();
                delay(500000);
                led_off();
                delay(500000);
            }
            delay(2000000);
        } else {
            /* No devices: rapid blink pattern */
            for (int i = 0; i < 10; i++) {
                led_on();
                delay(50000);
                led_off();
                delay(50000);
            }
            delay(2000000);
        }
    }

    return 0;
}
