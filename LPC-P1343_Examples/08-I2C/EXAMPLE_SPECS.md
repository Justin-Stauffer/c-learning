# Chapter 8: I2C Examples Specifications

## Hardware Requirements
- LPC-P1343 development board
- BMP280 barometric pressure sensor - from SunFounder kit
- SSD1306 OLED display (optional) - from SunFounder kit
- Pull-up resistors (if not on module) - 4.7K typical

---

## Example 1: I2C-Scan
**Status:** CREATED

### Purpose
Scan the I2C bus to detect all connected devices and their addresses.

### Hardware Connections
```
I2C Bus:
  P0.4 (SCL) ──► SCL on all I2C devices
  P0.5 (SDA) ◄─► SDA on all I2C devices
  3.3V ────────► VCC
  GND ─────────► GND

Pull-up resistors (4.7K) on SCL and SDA to 3.3V
(Many modules have built-in pull-ups)
```

### Key Concepts
- I2C bus initialization
- Address probing (0x01-0x7F)
- ACK/NACK detection
- LED feedback for found devices

### Expected Behavior
- Scans all 127 possible addresses
- LED blinks for each device found
- Example: BMP280 at 0x76 or 0x77

---

## Example 2: BMP280-Read
**Status:** CREATED

### Purpose
Read temperature and pressure from BMP280 sensor using I2C.

### Hardware Connections
```
BMP280 Module:
  VCC → 3.3V
  GND → GND
  SCL → P0.4
  SDA → P0.5
  CSB → 3.3V (for I2C mode)
  SDO → GND (address 0x76) or 3.3V (address 0x77)
```

### Key Concepts
- I2C multi-byte read/write
- Sensor register access
- Calibration data reading
- Temperature/pressure calculation

### Register Map (BMP280)
```c
#define BMP280_ADDR       0x76  /* or 0x77 */
#define BMP280_ID_REG     0xD0  /* Chip ID = 0x58 */
#define BMP280_CTRL_MEAS  0xF4  /* Control register */
#define BMP280_TEMP_MSB   0xFA  /* Temperature data */
#define BMP280_PRESS_MSB  0xF7  /* Pressure data */
```

### Expected Behavior
- Initialize BMP280 in normal mode
- Read raw temperature and pressure
- LED blinks based on temperature range

---

## Example 3: I2C-EEPROM
**Status:** PENDING

### Purpose
Read/write data to I2C EEPROM (24C02 or similar if available).

### Key Concepts
- Page writes
- Sequential reads
- Write cycle timing
- Data verification

### Expected Behavior
- Write test pattern
- Read back and verify
- LED indicates success/failure

---

## Common I2C Register Definitions

```c
/* System Clock Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))

/* IOCON for I2C pins */
#define IOCON_PIO0_4    (*((volatile uint32_t *)0x40044030))  /* SCL */
#define IOCON_PIO0_5    (*((volatile uint32_t *)0x40044034))  /* SDA */

/* I2C Registers */
#define I2C0CONSET      (*((volatile uint32_t *)0x40000000))
#define I2C0CONCLR      (*((volatile uint32_t *)0x40000018))
#define I2C0STAT        (*((volatile uint32_t *)0x40000004))
#define I2C0DAT         (*((volatile uint32_t *)0x40000008))
#define I2C0SCLH        (*((volatile uint32_t *)0x40000010))
#define I2C0SCLL        (*((volatile uint32_t *)0x40000014))

/* I2C Control bits */
#define I2C_AA          (1 << 2)   /* Assert Acknowledge */
#define I2C_SI          (1 << 3)   /* Interrupt flag */
#define I2C_STO         (1 << 4)   /* STOP condition */
#define I2C_STA         (1 << 5)   /* START condition */
#define I2C_I2EN        (1 << 6)   /* I2C Enable */
```

---

## I2C Status Codes

| Code | Meaning |
|------|---------|
| 0x08 | START transmitted |
| 0x10 | Repeated START transmitted |
| 0x18 | SLA+W transmitted, ACK received |
| 0x20 | SLA+W transmitted, NACK received |
| 0x28 | Data transmitted, ACK received |
| 0x30 | Data transmitted, NACK received |
| 0x40 | SLA+R transmitted, ACK received |
| 0x48 | SLA+R transmitted, NACK received |
| 0x50 | Data received, ACK returned |
| 0x58 | Data received, NACK returned |

---

## Common I2C Device Addresses (SunFounder Kit)

| Device | Address | Notes |
|--------|---------|-------|
| BMP280 | 0x76/0x77 | Barometric pressure |
| MPU6050 | 0x68/0x69 | Accelerometer/Gyro |
| SSD1306 | 0x3C/0x3D | OLED Display |
| PCF8591 | 0x48 | ADC/DAC |
| DS1302 | N/A | Uses 3-wire, not I2C |

---

## Build Notes
- Each example uses the same Makefile template
- startup_lpc1343_gcc.s and lpc1343_flash.ld copied to each folder
- Build with: `make`
- Flash with: `make flash`

---

## Testing Checklist

- [ ] Example 1: I2C scan finds connected devices
- [ ] Example 2: BMP280 reads temperature correctly
- [ ] Example 3: EEPROM read/write verified

---

*Chapter 8 of the LPC1343 Examples Series*
*I2C Communication*
