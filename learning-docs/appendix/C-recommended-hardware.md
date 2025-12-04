# Appendix C: Recommended Hardware

## Components for Hands-On Learning with This Curriculum

This appendix lists the recommended hardware for completing the examples in Chapters 7-10 (ADC, I2C, SPI, Power Management).

---

## Recommended Sensor Kit

### SunFounder Universal Maker Sensor Kit

**Amazon:** [SunFounder Universal Maker Sensor Kit](https://www.amazon.com/SunFounder-Sensor-Kit-Compatible-MicroPython/dp/B0D3GWJK82)

**Price:** ~$70 USD

**Why this kit?**
- **39 components** including sensors, displays, and actuators
- **No microcontroller included** - you're not paying for a board you won't use
- **Excellent I2C coverage** - BMP280, MPU6050, OLED, LCD, and more
- **Well documented** - [Official documentation](https://docs.sunfounder.com/projects/umsk/en/latest/)
- **Multi-platform compatible** - Works with any 3.3V microcontroller

### Kit Contents Relevant to This Curriculum

| Chapter | Components from Kit |
|---------|---------------------|
| **Ch 7: ADC** | Potentiometer, Photoresistor, Joystick |
| **Ch 8: I2C** | BMP280, MPU6050, SSD1306 OLED, LCD1602, VL53L0X, MAX30102, PCF8591, DS1302 RTC |
| **Ch 9: SPI** | (See additional component below) |
| **Ch 10: Power** | Button, PIR sensor (for wake-up examples) |

---

## Additional Recommended Component

### W25Q16 SPI Flash Module

**Why needed:** The sensor kit focuses on I2C devices. For complete SPI examples, add this flash memory module.

**Where to buy:**
- Amazon: Search "W25Q16 flash module" (~$2-5 for a pack)
- AliExpress: Even cheaper in bulk
- Adafruit/SparkFun: Higher quality breakout boards (~$5-8)

**Specifications:**
- 16 Mbit (2 MB) SPI flash memory
- 3.3V compatible (important for LPC1343!)
- Standard SPI interface
- Great for learning data logging and storage

---

## Complete Shopping List

### Required (Already Have)
- LPC-P1343 Development Board
- ST-Link V2 Debugger
- USB-to-UART Adapter (DSD TECH SH-U09C5 or similar)
- Breadboard (included in sensor kit)
- Jumper Wires (included in sensor kit)

### Recommended Purchase
| Item | Approximate Price | Notes |
|------|-------------------|-------|
| SunFounder Universal Maker Sensor Kit | $40-50 | Main component kit |
| W25Q16 Flash Module | $2-5 | For SPI examples |
| **Total** | **~$45-55** | |

### Optional but Useful
| Item | Price | Use Case |
|------|-------|----------|
| Multimeter with µA range | $15-30 | Measuring power consumption (Ch 10) |
| Logic Analyzer (8-channel) | $10-15 | Debugging I2C/SPI signals |
| Extra LEDs + Resistors | $5 | More visual feedback options |
| CR2032 Battery + Holder | $3-5 | Testing battery-powered operation |

---

## Hardware Compatibility Notes

### Voltage Levels

The LPC1343 operates at **3.3V**. Most components in the SunFounder kit are 3.3V compatible, but always verify:

```
✅ Safe for LPC1343 (3.3V):
- BMP280 (3.3V module)
- MPU6050 (3.3V module)
- SSD1306 OLED (3.3V module)
- W25Q16 Flash (3.3V native)
- DHT11 (3.3V compatible)
- Photoresistor circuit (with proper resistor)
- Potentiometer (voltage divider)

⚠️ Check Module Version:
- Some modules have 5V level shifters
- Look for "3.3V" or "3V3" labels
- When in doubt, check with multimeter
```

### I2C Pull-up Resistors

Many I2C modules have built-in pull-ups. If you're connecting multiple I2C devices:

```
Multiple I2C Devices:

If each module has 4.7kΩ pull-ups:
- 2 modules: 2.35kΩ effective (OK)
- 3 modules: 1.57kΩ effective (might be too strong)
- 4+ modules: Consider removing some pull-ups

Symptoms of too-strong pull-ups:
- Signal edges too sharp
- Ringing on oscilloscope
- Communication errors at high speed
```

---

## Wiring Quick Reference

### I2C Bus (Chapter 8)

```
LPC1343          I2C Device
─────────        ──────────
P0.4 (SCL) ────► SCL
P0.5 (SDA) ────► SDA
GND ───────────► GND
3.3V ──────────► VCC (or VIN if module has regulator)

Don't forget pull-up resistors if not on module!
```

### SPI Bus (Chapter 9)

```
LPC1343          SPI Device (e.g., W25Q16)
─────────        ─────────────────────────
P0.6 (SCK) ────► CLK
P0.8 (MISO) ◄─── DO (Data Out)
P0.9 (MOSI) ───► DI (Data In)
P0.2 (GPIO) ───► /CS (Chip Select)
GND ───────────► GND
3.3V ──────────► VCC
```

### ADC Input (Chapter 7)

```
Potentiometer:              Photoresistor (LDR):

3.3V ────┬────              3.3V ────┬────
         │                           │
        ┌┴┐                        ┌─┴─┐
        │ │ 10K Pot                │LDR│
        └┬┘                        └─┬─┘
         │                           │
         ├──► P0.11 (AD0)            ├──► P0.11 (AD0)
         │                           │
GND ─────┴────                     ┌─┴─┐
                                   │10K│
                                   └─┬─┘
                                     │
                           GND ──────┴────
```

---

## Where to Buy

### United States
- **Amazon** - Fast shipping, easy returns
- **Adafruit** - High quality, great tutorials
- **SparkFun** - Quality components, good documentation
- **DigiKey/Mouser** - Professional grade, larger quantities

### International
- **AliExpress** - Lowest prices, slower shipping (2-4 weeks)
- **Banggood** - Similar to AliExpress
- **Local electronics stores** - Check availability

---

## Troubleshooting New Hardware

### Component Doesn't Work

1. **Check wiring** - Most problems are wiring issues
2. **Verify voltage** - Is it getting 3.3V?
3. **Check I2C address** - Use I2C scanner code
4. **Try different pins** - Rule out dead GPIO
5. **Test with known-good code** - Arduino examples often work as reference

### I2C Device Not Responding

```c
/* I2C Scanner - Find connected devices */
void i2c_scan(void) {
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (i2c_probe(addr)) {
            /* Device found at 'addr' */
            led_blink(addr & 0x0F);  /* Visual feedback */
        }
    }
}
```

### SPI Device Not Responding

1. Check chip select (CS) is going LOW
2. Verify clock polarity (CPOL) and phase (CPHA)
3. Confirm MOSI/MISO aren't swapped
4. Try slower clock speed first

---

## Resources

- [SunFounder Kit Documentation](https://docs.sunfounder.com/projects/umsk/en/latest/)
- [SunFounder GitHub](https://github.com/sunfounder/universal-maker-sensor-kit)
- [LPC1343 User Manual](https://www.nxp.com/docs/en/user-guide/UM10375.pdf)
- [I2C Specification](https://www.nxp.com/docs/en/user-guide/UM10204.pdf)

---

**Return to:** [Index](../00-index.md) | [Appendix A: Hardware Setup](A-hardware-setup-stlink-uart.md)

---

*Appendix C of the Embedded C Learning Series*
*Recommended Hardware for Hands-On Learning*
