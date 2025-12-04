/**************************************************
 * LPC1343 I2C Example: BMP280 Sensor Read
 * Chapter 8: I2C Communication
 *
 * Reads temperature and pressure from the BMP280
 * barometric pressure sensor using I2C.
 *
 * Hardware:
 *   BMP280 Module:
 *     VCC → 3.3V
 *     GND → GND
 *     SCL → P0.4
 *     SDA → P0.5
 *     CSB → 3.3V (for I2C mode)
 *     SDO → GND (address 0x76) or 3.3V (0x77)
 *
 *   LED: P0.7 (onboard, active low)
 *
 * BMP280 Overview:
 *   - Temperature range: -40 to +85°C
 *   - Pressure range: 300 to 1100 hPa
 *   - I2C address: 0x76 or 0x77
 *   - Chip ID: 0x58
 *
 * Result:
 *   LED blink rate indicates temperature:
 *   - Fast blink = hot (>30°C)
 *   - Slow blink = cold (<15°C)
 *   - Medium blink = comfortable (15-30°C)
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
#define I2C_AA          (1 << 2)
#define I2C_SI          (1 << 3)
#define I2C_STO         (1 << 4)
#define I2C_STA         (1 << 5)
#define I2C_I2EN        (1 << 6)

/*--------------------------------------------------
 * I2C Status Codes
 *------------------------------------------------*/
#define I2C_START_SENT      0x08
#define I2C_REP_START_SENT  0x10
#define I2C_SLA_W_ACK       0x18
#define I2C_SLA_W_NACK      0x20
#define I2C_DATA_W_ACK      0x28
#define I2C_DATA_W_NACK     0x30
#define I2C_SLA_R_ACK       0x40
#define I2C_SLA_R_NACK      0x48
#define I2C_DATA_R_ACK      0x50
#define I2C_DATA_R_NACK     0x58

/*--------------------------------------------------
 * BMP280 Constants
 *------------------------------------------------*/
#define BMP280_ADDR         0x76    /* or 0x77 */
#define BMP280_ID_REG       0xD0    /* Chip ID register */
#define BMP280_ID_VALUE     0x58    /* Expected chip ID */
#define BMP280_CTRL_MEAS    0xF4    /* Control register */
#define BMP280_CONFIG       0xF5    /* Config register */
#define BMP280_TEMP_MSB     0xFA    /* Temperature MSB */
#define BMP280_CALIB_START  0x88    /* Calibration data start */

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7

/*--------------------------------------------------
 * Calibration Data (from BMP280)
 *------------------------------------------------*/
static uint16_t dig_T1;
static int16_t dig_T2;
static int16_t dig_T3;

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

/*--------------------------------------------------
 * Wait for I2C Interrupt Flag
 *------------------------------------------------*/
static void i2c_wait(void) {
    while ((I2C0CONSET & I2C_SI) == 0);
}

/*--------------------------------------------------
 * Initialize I2C (100 kHz)
 *------------------------------------------------*/
static void i2c_init(void) {
    SYSAHBCLKCTRL |= (1 << 5);   /* I2C clock */
    PRESETCTRL |= (1 << 1);      /* De-assert reset */

    IOCON_PIO0_4 = 0x01;         /* SCL function */
    IOCON_PIO0_5 = 0x01;         /* SDA function */

    /* 100 kHz: 72 MHz / 720 */
    I2C0SCLH = 360;
    I2C0SCLL = 360;

    I2C0CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;
    I2C0CONSET = I2C_I2EN;
}

/*--------------------------------------------------
 * I2C Write Byte to Register
 *------------------------------------------------*/
static uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t data) {
    uint8_t status;

    /* START */
    I2C0CONSET = I2C_STA;
    i2c_wait();
    status = I2C0STAT;
    if (status != I2C_START_SENT && status != I2C_REP_START_SENT) {
        I2C0CONCLR = I2C_SI | I2C_STA;
        I2C0CONSET = I2C_STO;
        return 0;
    }
    I2C0CONCLR = I2C_STA;

    /* Address + Write */
    I2C0DAT = (addr << 1) | 0;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_SLA_W_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* Register address */
    I2C0DAT = reg;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_DATA_W_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* Data byte */
    I2C0DAT = data;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_DATA_W_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* STOP */
    I2C0CONSET = I2C_STO;
    I2C0CONCLR = I2C_SI;
    while (I2C0CONSET & I2C_STO);

    return 1;
}

/*--------------------------------------------------
 * I2C Read Bytes from Register
 *------------------------------------------------*/
static uint8_t i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t status;

    /* START */
    I2C0CONSET = I2C_STA;
    i2c_wait();
    status = I2C0STAT;
    if (status != I2C_START_SENT && status != I2C_REP_START_SENT) {
        I2C0CONCLR = I2C_SI | I2C_STA;
        I2C0CONSET = I2C_STO;
        return 0;
    }
    I2C0CONCLR = I2C_STA;

    /* Address + Write (to send register address) */
    I2C0DAT = (addr << 1) | 0;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_SLA_W_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* Register address */
    I2C0DAT = reg;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_DATA_W_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* Repeated START */
    I2C0CONSET = I2C_STA;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    status = I2C0STAT;
    if (status != I2C_REP_START_SENT) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI | I2C_STA;
        return 0;
    }
    I2C0CONCLR = I2C_STA;

    /* Address + Read */
    I2C0DAT = (addr << 1) | 1;
    I2C0CONCLR = I2C_SI;
    i2c_wait();
    if (I2C0STAT != I2C_SLA_R_ACK) {
        I2C0CONSET = I2C_STO;
        I2C0CONCLR = I2C_SI;
        return 0;
    }

    /* Read data bytes */
    for (uint8_t i = 0; i < len; i++) {
        if (i < len - 1) {
            /* More bytes to read: ACK */
            I2C0CONSET = I2C_AA;
        } else {
            /* Last byte: NACK */
            I2C0CONCLR = I2C_AA;
        }
        I2C0CONCLR = I2C_SI;
        i2c_wait();

        buf[i] = I2C0DAT;
    }

    /* STOP */
    I2C0CONSET = I2C_STO;
    I2C0CONCLR = I2C_SI;
    while (I2C0CONSET & I2C_STO);

    return 1;
}

/*--------------------------------------------------
 * Read Single Register
 *------------------------------------------------*/
static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg) {
    uint8_t data = 0;
    i2c_read_regs(addr, reg, &data, 1);
    return data;
}

/*--------------------------------------------------
 * BMP280: Read Calibration Data
 *
 * Calibration data is stored in registers 0x88-0x9F
 * We only read temperature calibration (6 bytes)
 *------------------------------------------------*/
static void bmp280_read_calibration(void) {
    uint8_t calib[6];

    i2c_read_regs(BMP280_ADDR, BMP280_CALIB_START, calib, 6);

    /* Temperature calibration (little-endian) */
    dig_T1 = (uint16_t)(calib[1] << 8) | calib[0];
    dig_T2 = (int16_t)(calib[3] << 8) | calib[2];
    dig_T3 = (int16_t)(calib[5] << 8) | calib[4];
}

/*--------------------------------------------------
 * BMP280: Initialize Sensor
 *
 * Configure for:
 * - Temperature oversampling x1
 * - Pressure oversampling x1
 * - Normal mode (continuous measurement)
 *------------------------------------------------*/
static uint8_t bmp280_init(void) {
    uint8_t id;

    /* Read and verify chip ID */
    id = i2c_read_reg(BMP280_ADDR, BMP280_ID_REG);
    if (id != BMP280_ID_VALUE) {
        return 0;  /* Wrong chip or not found */
    }

    /* Read calibration data */
    bmp280_read_calibration();

    /* Configure: osrs_t=001 (x1), osrs_p=001 (x1), mode=11 (normal)
     * CTRL_MEAS = 0x27 */
    i2c_write_reg(BMP280_ADDR, BMP280_CTRL_MEAS, 0x27);

    /* Configure filter and standby time
     * CONFIG = 0x00 (no filter, 0.5ms standby) */
    i2c_write_reg(BMP280_ADDR, BMP280_CONFIG, 0x00);

    return 1;
}

/*--------------------------------------------------
 * BMP280: Read Raw Temperature
 *
 * Temperature is 20-bit value in registers:
 * 0xFA (MSB), 0xFB (LSB), 0xFC (XLSB, bits 7:4)
 *------------------------------------------------*/
static int32_t bmp280_read_raw_temp(void) {
    uint8_t data[3];
    int32_t raw;

    i2c_read_regs(BMP280_ADDR, BMP280_TEMP_MSB, data, 3);

    raw = ((int32_t)data[0] << 12) |
          ((int32_t)data[1] << 4) |
          ((int32_t)data[2] >> 4);

    return raw;
}

/*--------------------------------------------------
 * BMP280: Calculate Temperature
 *
 * Uses Bosch compensation formula from datasheet.
 * Returns temperature in 0.01 degrees C
 * (e.g., 2534 = 25.34°C)
 *------------------------------------------------*/
static int32_t bmp280_calc_temp(int32_t adc_T) {
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
              ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
            ((int32_t)dig_T3)) >> 14;

    T = (var1 + var2) * 5 + 128;
    T = T >> 8;

    return T;  /* Temperature in 0.01°C */
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    int32_t raw_temp;
    int32_t temp_c;
    uint32_t blink_delay;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);
    led_off();

    /* Initialize I2C */
    i2c_init();

    /* Initialize BMP280 */
    if (!bmp280_init()) {
        /* BMP280 not found - rapid blink error */
        while (1) {
            led_on();
            delay(50000);
            led_off();
            delay(50000);
        }
    }

    /* Success indicator: 3 slow blinks */
    for (int i = 0; i < 3; i++) {
        led_on();
        delay(300000);
        led_off();
        delay(300000);
    }
    delay(500000);

    /* Main loop: read temperature, adjust blink rate */
    while (1) {
        /* Read temperature */
        raw_temp = bmp280_read_raw_temp();
        temp_c = bmp280_calc_temp(raw_temp);

        /* Convert 0.01°C to whole degrees for threshold */
        int16_t temp_whole = temp_c / 100;

        /* Set blink rate based on temperature:
         * Hot (>30°C): fast blink
         * Cold (<15°C): slow blink
         * Comfortable: medium blink
         */
        if (temp_whole > 30) {
            blink_delay = 100000;   /* Fast */
        } else if (temp_whole < 15) {
            blink_delay = 500000;   /* Slow */
        } else {
            blink_delay = 250000;   /* Medium */
        }

        /* Toggle LED */
        led_on();
        delay(blink_delay);
        led_off();
        delay(blink_delay);
    }

    return 0;
}
