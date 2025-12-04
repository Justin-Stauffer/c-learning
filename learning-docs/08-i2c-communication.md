# Chapter 8: I2C Communication

## Two-Wire Serial Communication with Sensors and Peripherals

Learn how to use the I2C (Inter-Integrated Circuit) bus to communicate with sensors, EEPROMs, displays, and other devices using just two wires.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 1 | I2C fundamentals | Beginner |
| Part 2 | LPC1343 I2C hardware | Beginner |
| Part 3 | Basic I2C transactions | Intermediate |
| Part 4 | Reading sensors (example) | Intermediate |
| Part 5 | Writing to EEPROM | Intermediate |
| Part 6 | I2C interrupts | Advanced |
| Part 7 | Troubleshooting | All levels |

**Prerequisites:** Chapter 0-3 (GPIO), Chapter 6 (Interrupts helpful)

---

## Part 1: I2C Fundamentals

### What is I2C?

I2C (pronounced "I-squared-C" or "I-two-C") is a synchronous, multi-master, multi-slave serial bus invented by Philips (now NXP). It uses just two wires to communicate with multiple devices.

```
I2C Bus Topology:

                    VCC (3.3V)
                      │
                    ┌─┴─┐ ┌─┴─┐
                    │4.7k│ │4.7k│  ← Pull-up resistors
                    └─┬─┘ └─┬─┘
                      │     │
    ┌───────┬─────────┼─────┼─────────┬───────┐
    │       │         │     │         │       │
    │     ┌─┴─┐     ┌─┴─┐ ┌─┴─┐     ┌─┴─┐     │
    │     │SDA│     │SCL│ │SDA│     │SCL│     │
    │     └───┘     └───┘ └───┘     └───┘     │
    │  ┌─────────┐    │  ┌─────────┐    │     │
    │  │ Master  │    │  │ Slave 1 │    │     │
    │  │ LPC1343 │    │  │ Sensor  │    │     │
    │  └─────────┘    │  └─────────┘    │     │
    │                 │                  │     │
    │              ┌──┴──┐            ┌──┴──┐  │
    │              │SDA  │            │SCL  │  │
    │              └─────┘            └─────┘  │
    │           ┌─────────┐       ┌─────────┐  │
    │           │ Slave 2 │       │ Slave 3 │  │
    │           │ EEPROM  │       │ Display │  │
    │           └─────────┘       └─────────┘  │
    │                                          │
    └──────────────────────────────────────────┘
                      GND
```

### Two Wires, Many Devices

| Wire | Name | Description |
|------|------|-------------|
| SDA | Serial Data | Bidirectional data line |
| SCL | Serial Clock | Clock signal (master controls) |

**Key features:**
- Open-drain outputs with pull-up resistors
- Multiple slaves on same bus (addressed individually)
- Standard speeds: 100 kHz, 400 kHz, 1 MHz+
- Each device has a unique 7-bit address

### I2C Protocol Basics

Every I2C transaction follows this pattern:

```
I2C Transaction Flow:

1. START condition (master initiates)
   SDA: HIGH→LOW while SCL is HIGH

2. Address + R/W bit (7 bits + 1 bit)
   - Address identifies which slave to talk to
   - R/W: 0 = Write to slave, 1 = Read from slave

3. ACK/NACK from slave
   - ACK (SDA LOW): Slave acknowledged
   - NACK (SDA HIGH): No slave responded or error

4. Data bytes (8 bits each)
   - Each byte followed by ACK/NACK

5. STOP condition (master terminates)
   SDA: LOW→HIGH while SCL is HIGH


Timing Diagram:

     START   Address + R/W   ACK    Data Byte    ACK    STOP
       │     ┌───────────┐    │    ┌─────────┐    │      │
       │     │ A6..A0  RW│    │    │ D7..D0  │    │      │
       ▼     └───────────┘    ▼    └─────────┘    ▼      ▼
SDA: ─┐_____█████████████____█____████████████____█____┌─
      │                                                │
      └── SDA falls                          SDA rises ┘
          while SCL HIGH                     while SCL HIGH

SCL: ────┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌─┐_┌───
         └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘
```

### Common I2C Addresses

| Device | Typical Address | Description |
|--------|-----------------|-------------|
| EEPROM 24C02 | 0x50-0x57 | Serial EEPROM |
| DS1307 RTC | 0x68 | Real-time clock |
| BMP180/BMP280 | 0x76 or 0x77 | Pressure/temperature sensor |
| SSD1306 OLED | 0x3C or 0x3D | 128x64 OLED display |
| MPU6050 | 0x68 or 0x69 | Accelerometer/gyroscope |
| PCF8574 | 0x20-0x27 | I/O expander |
| AT24C32 | 0x50 | 4KB EEPROM |

**Note:** Addresses are 7-bit. When shifted left and combined with R/W bit, they become 8-bit values on the bus.

---

## Part 2: LPC1343 I2C Hardware

### I2C Pins

The LPC1343 has one I2C port:

```
LPC1343 I2C Pins:

Pin    │ Function │ IOCON Register      │ Notes
───────┼──────────┼─────────────────────┼────────────────
P0.4   │ SCL      │ IOCON_PIO0_4        │ Open-drain
P0.5   │ SDA      │ IOCON_PIO0_5        │ Open-drain

Both pins are 5V tolerant and support I2C fast mode.
```

### I2C Register Addresses

```c
/* I2C Registers */
#define I2C0CONSET      (*((volatile uint32_t *)0x40000000))  /* Control Set */
#define I2C0STAT        (*((volatile uint32_t *)0x40000004))  /* Status */
#define I2C0DAT         (*((volatile uint32_t *)0x40000008))  /* Data */
#define I2C0ADR0        (*((volatile uint32_t *)0x4000000C))  /* Slave Address 0 */
#define I2C0SCLH        (*((volatile uint32_t *)0x40000010))  /* SCL High time */
#define I2C0SCLL        (*((volatile uint32_t *)0x40000014))  /* SCL Low time */
#define I2C0CONCLR      (*((volatile uint32_t *)0x40000018))  /* Control Clear */

/* System registers */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))

/* IOCON for I2C pins */
#define IOCON_PIO0_4    (*((volatile uint32_t *)0x40044030))  /* SCL */
#define IOCON_PIO0_5    (*((volatile uint32_t *)0x40044034))  /* SDA */
```

### I2C Control Register Bits

```
I2C0CONSET / I2C0CONCLR - Control Register

Bit │ Name │ CONSET │ CONCLR │ Description
────┼──────┼────────┼────────┼─────────────────────────
 2  │ AA   │ Set    │ Clear  │ Assert Acknowledge
 3  │ SI   │ Read   │ Clear  │ I2C Interrupt Flag
 4  │ STO  │ Set    │ N/A    │ STOP condition
 5  │ STA  │ Set    │ Clear  │ START condition
 6  │ I2EN │ Set    │ Clear  │ I2C Enable

To set a bit: Write 1 to I2C0CONSET
To clear a bit: Write 1 to I2C0CONCLR

Examples:
  I2C0CONSET = (1 << 6);   // Enable I2C
  I2C0CONSET = (1 << 5);   // Generate START
  I2C0CONCLR = (1 << 3);   // Clear SI flag
```

### I2C Status Codes

The I2C state machine reports status through I2C0STAT:

```
Common Status Codes (Master Transmitter):

Code │ Status
─────┼─────────────────────────────────────────
0x08 │ START transmitted
0x10 │ Repeated START transmitted
0x18 │ SLA+W transmitted, ACK received
0x20 │ SLA+W transmitted, NACK received
0x28 │ Data transmitted, ACK received
0x30 │ Data transmitted, NACK received
0x38 │ Arbitration lost

Common Status Codes (Master Receiver):

Code │ Status
─────┼─────────────────────────────────────────
0x40 │ SLA+R transmitted, ACK received
0x48 │ SLA+R transmitted, NACK received
0x50 │ Data received, ACK returned
0x58 │ Data received, NACK returned
```

### I2C Clock Speed

```
I2C Clock = PCLK / (SCLH + SCLL)

For 100 kHz at 72 MHz PCLK:
  SCLH = SCLL = 360
  72MHz / (360 + 360) = 100 kHz

For 400 kHz at 72 MHz PCLK:
  SCLH = SCLL = 90
  72MHz / (90 + 90) = 400 kHz
```

---

## Part 3: Basic I2C Transactions

### I2C Initialization

```c
#include <stdint.h>

/* I2C Registers */
#define I2C0CONSET      (*((volatile uint32_t *)0x40000000))
#define I2C0STAT        (*((volatile uint32_t *)0x40000004))
#define I2C0DAT         (*((volatile uint32_t *)0x40000008))
#define I2C0SCLH        (*((volatile uint32_t *)0x40000010))
#define I2C0SCLL        (*((volatile uint32_t *)0x40000014))
#define I2C0CONCLR      (*((volatile uint32_t *)0x40000018))

/* System registers */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))

/* IOCON for I2C pins */
#define IOCON_PIO0_4    (*((volatile uint32_t *)0x40044030))
#define IOCON_PIO0_5    (*((volatile uint32_t *)0x40044034))

/* Clock bits */
#define I2C_CLK         (1 << 5)
#define IOCON_CLK       (1 << 16)

/* Control bits */
#define I2C_AA          (1 << 2)
#define I2C_SI          (1 << 3)
#define I2C_STO         (1 << 4)
#define I2C_STA         (1 << 5)
#define I2C_I2EN        (1 << 6)

/**
 * Initialize I2C at specified speed
 * @param speed_khz I2C clock speed in kHz (100 or 400)
 */
void i2c_init(uint32_t speed_khz) {
    uint32_t pclk = 72000000;  /* Assuming 72 MHz */
    uint32_t divider;

    /* Enable I2C clock */
    SYSAHBCLKCTRL |= I2C_CLK | IOCON_CLK;

    /* De-assert I2C reset */
    PRESETCTRL |= (1 << 1);

    /* Configure pins for I2C function (standard I2C mode) */
    IOCON_PIO0_4 = 0x00;  /* SCL - I2C standard mode */
    IOCON_PIO0_5 = 0x00;  /* SDA - I2C standard mode */

    /* Calculate clock divider */
    divider = pclk / (speed_khz * 1000 * 2);

    /* Set SCL high and low times (equal for 50% duty cycle) */
    I2C0SCLH = divider;
    I2C0SCLL = divider;

    /* Clear all flags */
    I2C0CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;

    /* Enable I2C */
    I2C0CONSET = I2C_I2EN;
}
```

### Wait for I2C State

```c
/**
 * Wait for I2C SI flag (state change)
 * @return Current I2C status
 */
uint8_t i2c_wait_si(void) {
    while (!(I2C0CONSET & I2C_SI));
    return I2C0STAT;
}

/**
 * Clear SI flag to continue operation
 */
void i2c_clear_si(void) {
    I2C0CONCLR = I2C_SI;
}
```

### Generate START Condition

```c
/**
 * Generate START condition
 * @return Status code (0x08 = success)
 */
uint8_t i2c_start(void) {
    /* Set STA bit */
    I2C0CONSET = I2C_STA;

    /* Wait for START transmitted */
    uint8_t status = i2c_wait_si();

    /* Clear STA bit */
    I2C0CONCLR = I2C_STA;

    return status;  /* Should be 0x08 or 0x10 */
}
```

### Send Address and R/W Bit

```c
/**
 * Send slave address with R/W bit
 * @param addr 7-bit slave address
 * @param read 0 = write, 1 = read
 * @return Status code
 */
uint8_t i2c_send_addr(uint8_t addr, uint8_t read) {
    /* Load address + R/W into data register */
    I2C0DAT = (addr << 1) | (read ? 1 : 0);

    /* Clear SI to transmit */
    i2c_clear_si();

    /* Wait for transmission complete */
    return i2c_wait_si();
}
```

### Write Data Byte

```c
/**
 * Write a byte to I2C bus
 * @param data Byte to write
 * @return Status code (0x28 = ACK received)
 */
uint8_t i2c_write_byte(uint8_t data) {
    /* Load data */
    I2C0DAT = data;

    /* Clear SI to transmit */
    i2c_clear_si();

    /* Wait for transmission complete */
    return i2c_wait_si();
}
```

### Read Data Byte

```c
/**
 * Read a byte from I2C bus
 * @param ack 1 = send ACK (more bytes to read), 0 = send NACK (last byte)
 * @return Received byte
 */
uint8_t i2c_read_byte(uint8_t ack) {
    if (ack) {
        I2C0CONSET = I2C_AA;  /* Will send ACK */
    } else {
        I2C0CONCLR = I2C_AA;  /* Will send NACK */
    }

    /* Clear SI to receive */
    i2c_clear_si();

    /* Wait for data received */
    i2c_wait_si();

    /* Return received data */
    return I2C0DAT;
}
```

### Generate STOP Condition

```c
/**
 * Generate STOP condition
 */
void i2c_stop(void) {
    /* Set STO bit */
    I2C0CONSET = I2C_STO;

    /* Clear SI */
    i2c_clear_si();

    /* Wait for STOP to complete (STO auto-clears) */
    while (I2C0CONSET & I2C_STO);
}
```

### Complete Write Transaction

```c
/**
 * Write data to I2C slave
 * @param addr 7-bit slave address
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @return 0 = success, non-zero = error
 */
int i2c_write(uint8_t addr, const uint8_t *data, uint32_t len) {
    uint8_t status;

    /* START */
    status = i2c_start();
    if (status != 0x08 && status != 0x10) {
        i2c_stop();
        return -1;
    }

    /* Send address + Write */
    status = i2c_send_addr(addr, 0);
    if (status != 0x18) {  /* SLA+W ACK */
        i2c_stop();
        return -2;
    }

    /* Send data bytes */
    for (uint32_t i = 0; i < len; i++) {
        status = i2c_write_byte(data[i]);
        if (status != 0x28) {  /* Data ACK */
            i2c_stop();
            return -3;
        }
    }

    /* STOP */
    i2c_stop();

    return 0;
}
```

### Complete Read Transaction

```c
/**
 * Read data from I2C slave
 * @param addr 7-bit slave address
 * @param data Pointer to receive buffer
 * @param len Number of bytes to read
 * @return 0 = success, non-zero = error
 */
int i2c_read(uint8_t addr, uint8_t *data, uint32_t len) {
    uint8_t status;

    /* START */
    status = i2c_start();
    if (status != 0x08 && status != 0x10) {
        i2c_stop();
        return -1;
    }

    /* Send address + Read */
    status = i2c_send_addr(addr, 1);
    if (status != 0x40) {  /* SLA+R ACK */
        i2c_stop();
        return -2;
    }

    /* Read data bytes */
    for (uint32_t i = 0; i < len; i++) {
        /* ACK all but last byte */
        data[i] = i2c_read_byte(i < len - 1);
    }

    /* STOP */
    i2c_stop();

    return 0;
}
```

---

## Part 4: Reading Sensors (Example)

### Example: Reading a Temperature Sensor (LM75)

The LM75 is a simple I2C temperature sensor with address 0x48-0x4F.

```
LM75 Connection:

    3.3V ───────┬────────────────────
                │
              ┌─┴─┐ ┌─┴─┐
              │4.7k│ │4.7k│
              └─┬─┘ └─┬─┘
                │     │
    ┌───────────┼─────┼───────────┐
    │           │     │           │
    │   P0.5 ───┴─────┤           │
    │   (SDA)         │           │
    │                 │           │
    │   P0.4 ─────────┴           │
    │   (SCL)                     │
    │                             │
    │   LPC1343          LM75     │
    │                   ┌─────┐   │
    │                   │     │   │
    │               SDA─┤     ├─VCC
    │               SCL─┤     ├─GND
    │              ALERT┤     ├─A0 ─► GND (addr bit)
    │                   └─────┘   │
    └─────────────────────────────┘

LM75 Address: 0x48 (when A0-A2 = GND)
```

```c
#define LM75_ADDR   0x48

/**
 * Read temperature from LM75
 * @return Temperature in degrees C × 2 (0.5°C resolution)
 */
int16_t lm75_read_temp(void) {
    uint8_t data[2];
    int16_t temp;

    /* Read 2 bytes from temperature register */
    if (i2c_read(LM75_ADDR, data, 2) != 0) {
        return -999;  /* Error */
    }

    /* Convert to temperature
     * data[0] = integer part (signed)
     * data[1] bit 7 = 0.5°C bit
     */
    temp = (int8_t)data[0] * 2;
    if (data[1] & 0x80) {
        temp += (temp >= 0) ? 1 : -1;
    }

    return temp;  /* Temperature × 2 */
}

/* Usage */
int16_t temp = lm75_read_temp();
/* temp = 50 means 25.0°C */
/* temp = 51 means 25.5°C */
```

### Example: BMP280 Pressure/Temperature Sensor

The BMP280 is a more complex sensor requiring register writes to configure:

```c
#define BMP280_ADDR     0x76

/* BMP280 Registers */
#define BMP280_ID       0xD0
#define BMP280_CTRL     0xF4
#define BMP280_CONFIG   0xF5
#define BMP280_PRESS    0xF7
#define BMP280_TEMP     0xFA

/**
 * Write to BMP280 register
 */
int bmp280_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write(BMP280_ADDR, data, 2);
}

/**
 * Read from BMP280 register
 */
int bmp280_read_reg(uint8_t reg, uint8_t *value) {
    /* Write register address */
    if (i2c_write(BMP280_ADDR, &reg, 1) != 0) {
        return -1;
    }

    /* Read register value */
    return i2c_read(BMP280_ADDR, value, 1);
}

/**
 * Read multiple bytes from BMP280
 */
int bmp280_read_regs(uint8_t reg, uint8_t *data, uint32_t len) {
    /* Write register address */
    if (i2c_write(BMP280_ADDR, &reg, 1) != 0) {
        return -1;
    }

    /* Read data */
    return i2c_read(BMP280_ADDR, data, len);
}

/**
 * Initialize BMP280
 */
int bmp280_init(void) {
    uint8_t id;

    /* Read and verify chip ID */
    if (bmp280_read_reg(BMP280_ID, &id) != 0) {
        return -1;
    }
    if (id != 0x58) {  /* BMP280 ID */
        return -2;
    }

    /* Configure: Normal mode, oversampling x1 */
    if (bmp280_write_reg(BMP280_CTRL, 0x27) != 0) {
        return -3;
    }

    return 0;
}
```

---

## Part 5: Writing to EEPROM

### Example: 24C02 EEPROM (256 bytes)

```
24C02 Connection:

    3.3V ───────┬────────────────────
                │
              ┌─┴─┐ ┌─┴─┐
              │4.7k│ │4.7k│
              └─┬─┘ └─┬─┘
                │     │
    ┌───────────┼─────┼───────────┐
    │           │     │           │
    │   SDA ────┴─────┤           │
    │   SCL ──────────┴           │
    │                             │
    │         24C02               │
    │        ┌─────┐              │
    │    A0 ─┤     ├─ VCC         │
    │    A1 ─┤     ├─ WP          │
    │    A2 ─┤     ├─ SCL         │
    │   GND ─┤     ├─ SDA         │
    │        └─────┘              │
    └─────────────────────────────┘

Address: 0x50 (when A0-A2 = GND)
WP: Connect to GND for write enable
```

```c
#define EEPROM_ADDR     0x50

/**
 * Write a byte to EEPROM
 * @param addr Memory address (0-255 for 24C02)
 * @param data Byte to write
 * @return 0 = success
 */
int eeprom_write_byte(uint8_t addr, uint8_t data) {
    uint8_t buf[2] = {addr, data};
    int result = i2c_write(EEPROM_ADDR, buf, 2);

    /* Wait for write cycle (5-10ms) */
    for (volatile int i = 0; i < 100000; i++);

    return result;
}

/**
 * Read a byte from EEPROM
 * @param addr Memory address
 * @return Data byte or -1 on error
 */
int eeprom_read_byte(uint8_t addr) {
    uint8_t data;

    /* Write address */
    if (i2c_write(EEPROM_ADDR, &addr, 1) != 0) {
        return -1;
    }

    /* Read data */
    if (i2c_read(EEPROM_ADDR, &data, 1) != 0) {
        return -1;
    }

    return data;
}

/**
 * Write multiple bytes to EEPROM (page write)
 * Note: Page size is 8 bytes for 24C02
 * @param addr Starting address (must be page-aligned)
 * @param data Data buffer
 * @param len Number of bytes (max 8)
 */
int eeprom_write_page(uint8_t addr, const uint8_t *data, uint8_t len) {
    uint8_t buf[9];  /* Address + 8 data bytes max */

    if (len > 8) len = 8;

    buf[0] = addr;
    for (uint8_t i = 0; i < len; i++) {
        buf[i + 1] = data[i];
    }

    int result = i2c_write(EEPROM_ADDR, buf, len + 1);

    /* Wait for write cycle */
    for (volatile int i = 0; i < 100000; i++);

    return result;
}

/**
 * Read multiple bytes from EEPROM
 */
int eeprom_read(uint8_t addr, uint8_t *data, uint8_t len) {
    /* Write starting address */
    if (i2c_write(EEPROM_ADDR, &addr, 1) != 0) {
        return -1;
    }

    /* Read sequential bytes */
    return i2c_read(EEPROM_ADDR, data, len);
}
```

### Example Usage: Storing Settings

```c
/* Settings structure */
typedef struct {
    uint8_t brightness;
    uint8_t volume;
    uint8_t mode;
    uint8_t checksum;
} Settings;

#define SETTINGS_ADDR   0x00

/**
 * Calculate simple checksum
 */
uint8_t calc_checksum(const Settings *s) {
    return s->brightness ^ s->volume ^ s->mode;
}

/**
 * Save settings to EEPROM
 */
int save_settings(const Settings *s) {
    Settings save = *s;
    save.checksum = calc_checksum(s);
    return eeprom_write_page(SETTINGS_ADDR, (uint8_t *)&save, sizeof(save));
}

/**
 * Load settings from EEPROM
 * @return 0 = success, -1 = error or invalid checksum
 */
int load_settings(Settings *s) {
    if (eeprom_read(SETTINGS_ADDR, (uint8_t *)s, sizeof(*s)) != 0) {
        return -1;
    }

    /* Verify checksum */
    if (calc_checksum(s) != s->checksum) {
        /* Invalid - use defaults */
        s->brightness = 50;
        s->volume = 50;
        s->mode = 0;
        return -1;
    }

    return 0;
}
```

---

## Part 6: I2C Interrupts

For non-blocking I2C operations, use the I2C interrupt:

```c
/* NVIC */
#define NVIC_ISER       (*((volatile uint32_t *)0xE000E100))
#define I2C_IRQn        15

/* I2C state machine states */
typedef enum {
    I2C_IDLE,
    I2C_STARTED,
    I2C_ADDR_SENT,
    I2C_WRITING,
    I2C_READING,
    I2C_DONE,
    I2C_ERROR
} I2C_State;

/* I2C transaction context */
volatile struct {
    I2C_State state;
    uint8_t addr;
    uint8_t *data;
    uint32_t len;
    uint32_t index;
    uint8_t read;
    int8_t result;
} i2c_ctx;

/**
 * I2C Interrupt Handler
 */
void I2C_IRQHandler(void) {
    uint8_t status = I2C0STAT;

    switch (status) {
        case 0x08:  /* START transmitted */
        case 0x10:  /* Repeated START */
            I2C0DAT = (i2c_ctx.addr << 1) | i2c_ctx.read;
            I2C0CONCLR = I2C_STA;
            i2c_ctx.state = I2C_ADDR_SENT;
            break;

        case 0x18:  /* SLA+W ACK */
            I2C0DAT = i2c_ctx.data[i2c_ctx.index++];
            i2c_ctx.state = I2C_WRITING;
            break;

        case 0x28:  /* Data ACK */
            if (i2c_ctx.index < i2c_ctx.len) {
                I2C0DAT = i2c_ctx.data[i2c_ctx.index++];
            } else {
                I2C0CONSET = I2C_STO;
                i2c_ctx.state = I2C_DONE;
                i2c_ctx.result = 0;
            }
            break;

        case 0x40:  /* SLA+R ACK */
            if (i2c_ctx.len > 1) {
                I2C0CONSET = I2C_AA;  /* ACK next byte */
            } else {
                I2C0CONCLR = I2C_AA;  /* NACK (last byte) */
            }
            i2c_ctx.state = I2C_READING;
            break;

        case 0x50:  /* Data received, ACK sent */
            i2c_ctx.data[i2c_ctx.index++] = I2C0DAT;
            if (i2c_ctx.index < i2c_ctx.len - 1) {
                I2C0CONSET = I2C_AA;
            } else {
                I2C0CONCLR = I2C_AA;  /* NACK next */
            }
            break;

        case 0x58:  /* Data received, NACK sent */
            i2c_ctx.data[i2c_ctx.index++] = I2C0DAT;
            I2C0CONSET = I2C_STO;
            i2c_ctx.state = I2C_DONE;
            i2c_ctx.result = 0;
            break;

        default:  /* Error */
            I2C0CONSET = I2C_STO;
            i2c_ctx.state = I2C_ERROR;
            i2c_ctx.result = -1;
            break;
    }

    I2C0CONCLR = I2C_SI;  /* Clear interrupt */
}

/**
 * Start async I2C write
 */
void i2c_write_async(uint8_t addr, uint8_t *data, uint32_t len) {
    i2c_ctx.addr = addr;
    i2c_ctx.data = data;
    i2c_ctx.len = len;
    i2c_ctx.index = 0;
    i2c_ctx.read = 0;
    i2c_ctx.state = I2C_STARTED;
    i2c_ctx.result = -2;  /* In progress */

    I2C0CONSET = I2C_STA;  /* Generate START */
}

/**
 * Check if I2C is busy
 */
uint8_t i2c_is_busy(void) {
    return i2c_ctx.state != I2C_IDLE &&
           i2c_ctx.state != I2C_DONE &&
           i2c_ctx.state != I2C_ERROR;
}

/**
 * Get I2C result (call after not busy)
 */
int8_t i2c_get_result(void) {
    return i2c_ctx.result;
}
```

---

## Part 7: Troubleshooting

### Common I2C Problems

#### No ACK from Slave

```
Problem: Status 0x20 or 0x48 (NACK received)

Possible causes:
1. Wrong slave address
2. Slave not powered
3. SDA/SCL wires swapped
4. Missing pull-up resistors
5. Slave in reset or sleep mode

Debug steps:
□ Verify address with logic analyzer or datasheet
□ Check power supply to slave
□ Verify SDA on P0.5, SCL on P0.4
□ Measure pull-up voltage with multimeter
□ Try slower I2C speed (100 kHz)
```

#### Bus Stuck (SDA/SCL Always Low)

```
Problem: I2C bus locked up

Causes:
- Slave holding SDA low (interrupted mid-transfer)
- Short circuit on bus
- Missing pull-ups

Fix: Generate clock pulses to release bus

void i2c_bus_recovery(void) {
    /* Temporarily configure SCL as GPIO output */
    IOCON_PIO0_4 = 0x01;  /* GPIO */
    GPIO0DIR |= (1 << 4);

    /* Generate 9 clock pulses */
    for (int i = 0; i < 9; i++) {
        GPIO0DATA &= ~(1 << 4);  /* SCL low */
        delay_us(5);
        GPIO0DATA |= (1 << 4);   /* SCL high */
        delay_us(5);
    }

    /* Reconfigure as I2C */
    IOCON_PIO0_4 = 0x00;
}
```

#### Erratic Communication

```
Problem: Intermittent failures

Possible causes:
1. Pull-up resistors too weak (too high value)
2. Pull-up resistors too strong (too low value)
3. Bus capacitance too high (long wires)
4. Clock speed too fast
5. Noise on bus

Solutions:
□ Use 2.2kΩ - 4.7kΩ pull-ups for short buses
□ Use 1kΩ pull-ups for faster speeds
□ Keep bus wires short (<50cm)
□ Reduce clock speed
□ Add 100Ω series resistors near master
```

### Debug with LEDs

```c
/**
 * Debug I2C with LED indication
 */
void i2c_debug_led(uint8_t status) {
    /* LED pattern based on status */
    switch (status & 0xF8) {
        case 0x08: /* START */
            led_pattern(0x01);
            break;
        case 0x18: /* SLA+W ACK */
        case 0x40: /* SLA+R ACK */
            led_pattern(0x03);
            break;
        case 0x28: /* Data ACK */
        case 0x50: /* Data received */
            led_pattern(0x07);
            break;
        case 0x20: /* NACK */
        case 0x30:
        case 0x48:
            led_pattern(0x0F);  /* All on = error */
            break;
    }
}
```

### I2C Scan Tool

```c
/**
 * Scan I2C bus for devices
 * Prints found addresses to UART
 */
void i2c_scan(void) {
    uart_puts("I2C Bus Scan:\r\n");

    for (uint8_t addr = 1; addr < 127; addr++) {
        uint8_t status = i2c_start();
        if (status != 0x08) continue;

        status = i2c_send_addr(addr, 0);
        i2c_stop();

        if (status == 0x18) {  /* ACK received */
            uart_puts("  Found device at 0x");
            uart_puthex(addr);
            uart_puts("\r\n");
        }
    }

    uart_puts("Scan complete.\r\n");
}
```

---

## Quick Reference

### I2C Registers

| Register | Address | Description |
|----------|---------|-------------|
| I2C0CONSET | 0x40000000 | Control Set |
| I2C0STAT | 0x40000004 | Status |
| I2C0DAT | 0x40000008 | Data |
| I2C0SCLH | 0x40000010 | SCL High Time |
| I2C0SCLL | 0x40000014 | SCL Low Time |
| I2C0CONCLR | 0x40000018 | Control Clear |

### Control Bits

| Bit | Name | Purpose |
|-----|------|---------|
| 2 | AA | Assert Acknowledge |
| 3 | SI | Interrupt Flag |
| 4 | STO | STOP condition |
| 5 | STA | START condition |
| 6 | I2EN | I2C Enable |

### Common Status Codes

| Code | Meaning |
|------|---------|
| 0x08 | START transmitted |
| 0x18 | SLA+W ACK |
| 0x20 | SLA+W NACK |
| 0x28 | Data ACK (write) |
| 0x40 | SLA+R ACK |
| 0x48 | SLA+R NACK |
| 0x50 | Data received, ACK sent |
| 0x58 | Data received, NACK sent |

---

## What's Next?

With I2C mastered, you can interface with hundreds of sensors and peripherals. Common next steps:
- Add an OLED display (SSD1306)
- Read accelerometer data (MPU6050)
- Store user settings in EEPROM
- Add a real-time clock (DS1307/DS3231)

**Next Chapter:** [Chapter 9: SPI Communication](09-spi-communication.md) - High-speed serial for displays, SD cards, and flash memory.

---

**Navigation:**
- Previous: [Chapter 7: ADC](07-adc-analog-to-digital.md)
- Next: [Chapter 9: SPI Communication](09-spi-communication.md)
- [Back to Index](00-index.md)

---

*Chapter 8 of the LPC1343 Embedded C Learning Series*
*I2C Communication: Two Wires, Endless Possibilities*
