# Chapter 9: SPI Communication

## High-Speed Serial Communication for Displays, Flash, and More

Learn how to use SPI (Serial Peripheral Interface) for fast, efficient communication with displays, SD cards, flash memory, and sensors.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 1 | SPI fundamentals | Beginner |
| Part 2 | LPC1343 SPI hardware | Beginner |
| Part 3 | Basic SPI transactions | Intermediate |
| Part 4 | SPI modes and timing | Intermediate |
| Part 5 | Practical examples | Intermediate |
| Part 6 | DMA with SPI | Advanced |
| Part 7 | Troubleshooting | All levels |

**Prerequisites:** Chapter 0-3 (GPIO), understanding of serial communication helpful

---

## Part 1: SPI Fundamentals

### What is SPI?

SPI (Serial Peripheral Interface) is a synchronous serial bus that uses separate lines for data in each direction, plus a clock. It's faster and simpler than I2C but requires more wires.

```
SPI Bus Topology:

         ┌─────────────┐
         │   Master    │
         │  (LPC1343)  │
         │             │
         │ MOSI ───────┼──┬─────────┬─────────┬─────────►
         │ MISO ◄──────┼──┼─────────┼─────────┼─────────
         │ SCK ────────┼──┼─────────┼─────────┼─────────►
         │             │  │         │         │
         │ SS0 ────────┼──┼────►    │         │
         │ SS1 ────────┼──┼─────────┼────►    │
         │ SS2 ────────┼──┼─────────┼─────────┼────►
         └─────────────┘  │         │         │
                          ▼         ▼         ▼
                       ┌─────┐   ┌─────┐   ┌─────┐
                       │Slave│   │Slave│   │Slave│
                       │  1  │   │  2  │   │  3  │
                       └─────┘   └─────┘   └─────┘

MOSI = Master Out, Slave In (data from master to slave)
MISO = Master In, Slave Out (data from slave to master)
SCK  = Serial Clock (always from master)
SS   = Slave Select (active low, one per slave)
```

### SPI vs I2C

| Feature | SPI | I2C |
|---------|-----|-----|
| Wires | 4+ (MOSI, MISO, SCK, SS per slave) | 2 (SDA, SCL) |
| Speed | 1-50+ MHz | 100 kHz - 3.4 MHz |
| Full duplex | Yes | No |
| Addressing | Hardware (SS pins) | Software (7-bit address) |
| Slaves | Limited by SS pins | Up to 127 |
| Complexity | Simple | More complex |
| Use cases | Displays, flash, SD cards | Sensors, EEPROMs |

### How SPI Works

SPI transfers data simultaneously in both directions on every clock edge:

```
SPI Data Transfer:

Master sends: 0xA5  (10100101)
Slave sends:  0x3C  (00111100)

        Clock
SCK     ─┐┌┐┌┐┌┐┌┐┌┐┌┐┌┐┌─
         └┘└┘└┘└┘└┘└┘└┘└┘

         1 0 1 0 0 1 0 1
MOSI    ─█_█_█___█___█_─   Master→Slave
         │ │ │   │   │
         0 0 1 1 1 1 0 0
MISO    ─___█_█_█_█___─    Slave→Master

SS      ─┐               ┌─  (low during transfer)
         └───────────────┘

Data shifts on one edge, samples on the other.
MSB is sent first (usually).
```

### Slave Select (SS/CS)

Each slave has its own select line:
- Active LOW: Pull low to select, high to deselect
- Only one slave selected at a time
- Slave ignores bus activity when not selected

```
Selecting Slave 2:

SS0  ─────────────────────   (high = deselected)
SS1  ─┐                   ┌─ (low = selected)
      └───────────────────┘
SS2  ─────────────────────   (high = deselected)
      │                   │
      │←── Slave 2 active ─►│
```

---

## Part 2: LPC1343 SPI Hardware

### SPI Pins

The LPC1343 has one SSP (Synchronous Serial Port) that can be configured as SPI:

```
LPC1343 SPI/SSP0 Pins:

Pin    │ Function │ IOCON Register      │ Func Value
───────┼──────────┼─────────────────────┼────────────
P0.6   │ SCK0     │ IOCON_SCK0_LOC      │ See note
P0.8   │ MISO0    │ IOCON_PIO0_8        │ 0x01
P0.9   │ MOSI0    │ IOCON_PIO0_9        │ 0x01
P0.2   │ SSEL0    │ IOCON_PIO0_2        │ 0x01 (hardware)
       │          │                     │ GPIO for software SS

Note: SCK0 location is configurable via IOCON_SCK0_LOC register.
Default location is P0.6 (value 0x02 in SCK0_LOC).
```

### SSP Register Addresses

```c
/* SSP0 Registers */
#define SSP0CR0         (*((volatile uint32_t *)0x40040000))  /* Control 0 */
#define SSP0CR1         (*((volatile uint32_t *)0x40040004))  /* Control 1 */
#define SSP0DR          (*((volatile uint32_t *)0x40040008))  /* Data */
#define SSP0SR          (*((volatile uint32_t *)0x4004000C))  /* Status */
#define SSP0CPSR        (*((volatile uint32_t *)0x40040010))  /* Clock Prescaler */
#define SSP0IMSC        (*((volatile uint32_t *)0x40040014))  /* Interrupt Mask */
#define SSP0RIS         (*((volatile uint32_t *)0x40040018))  /* Raw Interrupt Status */
#define SSP0MIS         (*((volatile uint32_t *)0x4004001C))  /* Masked Interrupt Status */
#define SSP0ICR         (*((volatile uint32_t *)0x40040020))  /* Interrupt Clear */

/* System registers */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))
#define SSP0CLKDIV      (*((volatile uint32_t *)0x40048094))

/* IOCON registers for SPI pins */
#define IOCON_PIO0_8    (*((volatile uint32_t *)0x40044060))  /* MISO */
#define IOCON_PIO0_9    (*((volatile uint32_t *)0x40044064))  /* MOSI */
#define IOCON_SCK0_LOC  (*((volatile uint32_t *)0x400440B0))  /* SCK location */
#define IOCON_SWCLK_PIO0_10 (*((volatile uint32_t *)0x40044068)) /* SCK on P0.10 */
#define IOCON_PIO0_6    (*((volatile uint32_t *)0x40044018))  /* SCK on P0.6 */
#define IOCON_PIO0_2    (*((volatile uint32_t *)0x4004400C))  /* SSEL */
```

### SSP Control Register 0 (SSP0CR0)

```
SSP0CR0 - Control Register 0 (0x40040000)

Bits 15-8: SCR    - Serial Clock Rate (divider)
Bit  7:    CPHA   - Clock Phase (0 or 1)
Bit  6:    CPOL   - Clock Polarity (0 or 1)
Bits 5-4:  FRF    - Frame Format (00 = SPI)
Bits 3-0:  DSS    - Data Size Select (0111 = 8-bit)

Common value for 8-bit SPI Mode 0:
SSP0CR0 = 0x0007  (8-bit, SPI, CPOL=0, CPHA=0, SCR=0)
```

### SSP Control Register 1 (SSP0CR1)

```
SSP0CR1 - Control Register 1 (0x40040004)

Bit 3:    SOD    - Slave Output Disable
Bit 2:    MS     - Master/Slave (0 = Master, 1 = Slave)
Bit 1:    SSE    - SSP Enable
Bit 0:    LBM    - Loopback Mode

For Master mode:
SSP0CR1 = 0x02  (Master, enabled)
```

### SSP Status Register (SSP0SR)

```
SSP0SR - Status Register (0x4004000C)

Bit 4:    BSY    - Busy (transmitting/receiving)
Bit 3:    RFF    - Receive FIFO Full
Bit 2:    RNE    - Receive FIFO Not Empty
Bit 1:    TNF    - Transmit FIFO Not Full
Bit 0:    TFE    - Transmit FIFO Empty
```

### SPI Clock Speed

```
SPI Clock = PCLK / (CPSDVSR × (SCR + 1))

Where:
  PCLK = System Clock / SSP0CLKDIV
  CPSDVSR = SSP0CPSR (even value, 2-254)
  SCR = bits 15-8 of SSP0CR0 (0-255)

Example at 72 MHz:
  SSP0CLKDIV = 1 (PCLK = 72 MHz)
  CPSDVSR = 2
  SCR = 0
  SPI Clock = 72MHz / (2 × 1) = 36 MHz (max)

For 1 MHz:
  CPSDVSR = 72
  SCR = 0
  SPI Clock = 72MHz / (72 × 1) = 1 MHz
```

---

## Part 3: Basic SPI Transactions

### SPI Initialization

```c
#include <stdint.h>

/* SSP0 Registers */
#define SSP0CR0         (*((volatile uint32_t *)0x40040000))
#define SSP0CR1         (*((volatile uint32_t *)0x40040004))
#define SSP0DR          (*((volatile uint32_t *)0x40040008))
#define SSP0SR          (*((volatile uint32_t *)0x4004000C))
#define SSP0CPSR        (*((volatile uint32_t *)0x40040010))

/* System registers */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))
#define SSP0CLKDIV      (*((volatile uint32_t *)0x40048094))

/* IOCON */
#define IOCON_PIO0_8    (*((volatile uint32_t *)0x40044060))
#define IOCON_PIO0_9    (*((volatile uint32_t *)0x40044064))
#define IOCON_SCK0_LOC  (*((volatile uint32_t *)0x400440B0))
#define IOCON_PIO0_6    (*((volatile uint32_t *)0x40044018))
#define IOCON_PIO0_2    (*((volatile uint32_t *)0x4004400C))

/* GPIO for software SS */
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))

/* Clock bits */
#define SSP0_CLK        (1 << 11)
#define IOCON_CLK       (1 << 16)
#define GPIO_CLK        (1 << 6)

/* SS pin (using P0.2 as GPIO) */
#define SS_PIN          (1 << 2)

/**
 * Initialize SPI (SSP0) as master
 * @param speed_khz SPI clock speed in kHz
 */
void spi_init(uint32_t speed_khz) {
    uint32_t prescaler;

    /* Enable clocks */
    SYSAHBCLKCTRL |= SSP0_CLK | IOCON_CLK | GPIO_CLK;

    /* De-assert SSP0 reset */
    PRESETCTRL |= (1 << 0);

    /* SSP0 clock divider = 1 */
    SSP0CLKDIV = 1;

    /* Configure pins for SSP0 function */
    IOCON_SCK0_LOC = 0x02;       /* SCK0 on P0.6 */
    IOCON_PIO0_6 = 0x02;         /* P0.6 = SCK0 */
    IOCON_PIO0_8 = 0x01;         /* P0.8 = MISO0 */
    IOCON_PIO0_9 = 0x01;         /* P0.9 = MOSI0 */

    /* Configure P0.2 as GPIO for software SS */
    IOCON_PIO0_2 = 0x00;         /* GPIO function */
    GPIO0DIR |= SS_PIN;          /* Output */
    GPIO0DATA |= SS_PIN;         /* High (deselected) */

    /* Calculate prescaler for desired speed */
    /* SPI_CLK = PCLK / (CPSR * (SCR + 1)) */
    /* With SCR = 0: SPI_CLK = PCLK / CPSR */
    prescaler = 72000 / speed_khz;
    if (prescaler < 2) prescaler = 2;
    if (prescaler > 254) prescaler = 254;
    if (prescaler & 1) prescaler++;  /* Must be even */

    /* Configure SSP0 */
    SSP0CPSR = prescaler;        /* Clock prescaler */
    SSP0CR0 = 0x0007;            /* 8-bit, SPI mode 0 */
    SSP0CR1 = 0x02;              /* Master mode, enable */
}

/**
 * Select SPI slave (pull SS low)
 */
void spi_select(void) {
    GPIO0DATA &= ~SS_PIN;
}

/**
 * Deselect SPI slave (pull SS high)
 */
void spi_deselect(void) {
    GPIO0DATA |= SS_PIN;
}
```

### Transfer a Single Byte

```c
/**
 * Transfer one byte (send and receive simultaneously)
 * @param data Byte to send
 * @return Received byte
 */
uint8_t spi_transfer(uint8_t data) {
    /* Wait for TX FIFO not full */
    while (!(SSP0SR & (1 << 1)));

    /* Send byte */
    SSP0DR = data;

    /* Wait for RX FIFO not empty */
    while (!(SSP0SR & (1 << 2)));

    /* Return received byte */
    return SSP0DR;
}

/**
 * Send a byte (ignore received data)
 */
void spi_send(uint8_t data) {
    spi_transfer(data);
}

/**
 * Receive a byte (send 0xFF as dummy)
 */
uint8_t spi_receive(void) {
    return spi_transfer(0xFF);
}
```

### Transfer Multiple Bytes

```c
/**
 * Transfer multiple bytes
 * @param tx_buf Transmit buffer (can be NULL to send 0xFF)
 * @param rx_buf Receive buffer (can be NULL to discard)
 * @param len Number of bytes
 */
void spi_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t tx = tx_buf ? tx_buf[i] : 0xFF;
        uint8_t rx = spi_transfer(tx);
        if (rx_buf) rx_buf[i] = rx;
    }
}

/**
 * Send multiple bytes
 */
void spi_send_buf(const uint8_t *data, uint32_t len) {
    spi_transfer_buf(data, NULL, len);
}

/**
 * Receive multiple bytes
 */
void spi_receive_buf(uint8_t *data, uint32_t len) {
    spi_transfer_buf(NULL, data, len);
}
```

### Wait for SPI Idle

```c
/**
 * Wait until SPI is not busy
 */
void spi_wait_idle(void) {
    while (SSP0SR & (1 << 4));  /* Wait while BSY */
}
```

---

## Part 4: SPI Modes and Timing

### The Four SPI Modes

SPI has four modes defined by CPOL (clock polarity) and CPHA (clock phase):

```
SPI Mode │ CPOL │ CPHA │ Description
─────────┼──────┼──────┼─────────────────────────────────
   0     │  0   │  0   │ Clock idle low, sample on rising edge
   1     │  0   │  1   │ Clock idle low, sample on falling edge
   2     │  1   │  0   │ Clock idle high, sample on falling edge
   3     │  1   │  1   │ Clock idle high, sample on rising edge


Mode 0 (CPOL=0, CPHA=0) - Most common:

          ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
SCK  ─────┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─────
          ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
          │   │   │   │   │   │   │   │
          Sample points (rising edge)

MOSI ─────<D7><D6><D5><D4><D3><D2><D1><D0>────
          Data changes on falling edge


Mode 3 (CPOL=1, CPHA=1):

     ─────┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌────
SCK       └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘
            ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
            Sample points (rising edge)
```

### Setting SPI Mode

```c
/**
 * Set SPI mode
 * @param mode SPI mode (0-3)
 */
void spi_set_mode(uint8_t mode) {
    uint32_t cr0 = SSP0CR0 & ~(0xC0);  /* Clear CPOL and CPHA */

    switch (mode) {
        case 0:  /* CPOL=0, CPHA=0 */
            cr0 |= (0 << 6) | (0 << 7);
            break;
        case 1:  /* CPOL=0, CPHA=1 */
            cr0 |= (0 << 6) | (1 << 7);
            break;
        case 2:  /* CPOL=1, CPHA=0 */
            cr0 |= (1 << 6) | (0 << 7);
            break;
        case 3:  /* CPOL=1, CPHA=1 */
            cr0 |= (1 << 6) | (1 << 7);
            break;
    }

    /* Disable SSP, change mode, re-enable */
    SSP0CR1 &= ~(1 << 1);
    SSP0CR0 = cr0;
    SSP0CR1 |= (1 << 1);
}
```

### Common Device SPI Modes

| Device | Mode | CPOL | CPHA | Max Speed |
|--------|------|------|------|-----------|
| SD Card | 0 | 0 | 0 | 25 MHz |
| W25Q Flash | 0 or 3 | 0/1 | 0/1 | 80+ MHz |
| SSD1306 OLED | 0 | 0 | 0 | 10 MHz |
| MAX7219 LED | 0 | 0 | 0 | 10 MHz |
| MCP3008 ADC | 0 | 0 | 0 | 3.6 MHz |

---

## Part 5: Practical Examples

### Example 1: SPI Flash Memory (W25Q16)

```
W25Q16 Connection:

    3.3V ─────────────────────────────
         │
         ├─────► VCC
         │
    LPC1343          W25Q16
    ┌─────┐          ┌─────┐
    │ P0.2├─── SS ──►│/CS  │
    │ P0.6├─── SCK ─►│CLK  │
    │ P0.9├─── MOSI ►│DI   │
    │ P0.8│◄── MISO ─│DO   │
    └─────┘          │/WP  ├──► 3.3V
                     │/HOLD├──► 3.3V
                     │VSS  ├──► GND
                     └─────┘
```

```c
/* W25Q Commands */
#define W25Q_WRITE_ENABLE   0x06
#define W25Q_WRITE_DISABLE  0x04
#define W25Q_READ_STATUS    0x05
#define W25Q_READ_DATA      0x03
#define W25Q_PAGE_PROGRAM   0x02
#define W25Q_SECTOR_ERASE   0x20
#define W25Q_CHIP_ERASE     0xC7
#define W25Q_READ_ID        0x9F

/**
 * Read W25Q manufacturer and device ID
 */
uint32_t w25q_read_id(void) {
    uint32_t id;

    spi_select();
    spi_send(W25Q_READ_ID);
    id = spi_receive() << 16;
    id |= spi_receive() << 8;
    id |= spi_receive();
    spi_deselect();

    return id;  /* Should be 0xEF4015 for W25Q16 */
}

/**
 * Read status register
 */
uint8_t w25q_read_status(void) {
    uint8_t status;

    spi_select();
    spi_send(W25Q_READ_STATUS);
    status = spi_receive();
    spi_deselect();

    return status;
}

/**
 * Wait until flash is not busy
 */
void w25q_wait_busy(void) {
    while (w25q_read_status() & 0x01);
}

/**
 * Enable writing
 */
void w25q_write_enable(void) {
    spi_select();
    spi_send(W25Q_WRITE_ENABLE);
    spi_deselect();
}

/**
 * Read data from flash
 * @param addr 24-bit address
 * @param data Buffer to receive data
 * @param len Number of bytes to read
 */
void w25q_read(uint32_t addr, uint8_t *data, uint32_t len) {
    spi_select();
    spi_send(W25Q_READ_DATA);
    spi_send((addr >> 16) & 0xFF);
    spi_send((addr >> 8) & 0xFF);
    spi_send(addr & 0xFF);
    spi_receive_buf(data, len);
    spi_deselect();
}

/**
 * Write a page (up to 256 bytes)
 * @param addr 24-bit address (must be page-aligned for full page)
 * @param data Data to write
 * @param len Number of bytes (max 256)
 */
void w25q_write_page(uint32_t addr, const uint8_t *data, uint32_t len) {
    w25q_write_enable();

    spi_select();
    spi_send(W25Q_PAGE_PROGRAM);
    spi_send((addr >> 16) & 0xFF);
    spi_send((addr >> 8) & 0xFF);
    spi_send(addr & 0xFF);
    spi_send_buf(data, len);
    spi_deselect();

    w25q_wait_busy();
}

/**
 * Erase a 4KB sector
 */
void w25q_erase_sector(uint32_t addr) {
    w25q_write_enable();

    spi_select();
    spi_send(W25Q_SECTOR_ERASE);
    spi_send((addr >> 16) & 0xFF);
    spi_send((addr >> 8) & 0xFF);
    spi_send(addr & 0xFF);
    spi_deselect();

    w25q_wait_busy();
}
```

### Example 2: MAX7219 LED Display Driver

```c
/* MAX7219 Registers */
#define MAX7219_DECODE      0x09
#define MAX7219_INTENSITY   0x0A
#define MAX7219_SCAN_LIMIT  0x0B
#define MAX7219_SHUTDOWN    0x0C
#define MAX7219_TEST        0x0F

/**
 * Write to MAX7219 register
 */
void max7219_write(uint8_t reg, uint8_t data) {
    spi_select();
    spi_send(reg);
    spi_send(data);
    spi_deselect();
}

/**
 * Initialize MAX7219
 */
void max7219_init(void) {
    max7219_write(MAX7219_TEST, 0x00);       /* Normal operation */
    max7219_write(MAX7219_SCAN_LIMIT, 0x07); /* Display all 8 digits */
    max7219_write(MAX7219_DECODE, 0x00);     /* No decode (raw segments) */
    max7219_write(MAX7219_INTENSITY, 0x08);  /* Medium brightness */
    max7219_write(MAX7219_SHUTDOWN, 0x01);   /* Normal operation */
}

/**
 * Display digit on position
 * @param pos Position (1-8)
 * @param segments Segment pattern (bits: DP-A-B-C-D-E-F-G)
 */
void max7219_display(uint8_t pos, uint8_t segments) {
    max7219_write(pos, segments);
}

/* 7-segment patterns for 0-9 */
const uint8_t digits[] = {
    0x7E, /* 0 */
    0x30, /* 1 */
    0x6D, /* 2 */
    0x79, /* 3 */
    0x33, /* 4 */
    0x5B, /* 5 */
    0x5F, /* 6 */
    0x70, /* 7 */
    0x7F, /* 8 */
    0x7B  /* 9 */
};

/**
 * Display a number (0-99999999)
 */
void max7219_display_number(uint32_t num) {
    for (int i = 1; i <= 8; i++) {
        max7219_display(i, digits[num % 10]);
        num /= 10;
        if (num == 0 && i > 1) {
            /* Blank leading zeros */
            for (int j = i + 1; j <= 8; j++) {
                max7219_display(j, 0x00);
            }
            break;
        }
    }
}
```

### Example 3: MCP3008 ADC (8-channel, 10-bit)

```c
/**
 * Read MCP3008 ADC channel
 * @param channel ADC channel (0-7)
 * @return 10-bit ADC value
 */
uint16_t mcp3008_read(uint8_t channel) {
    uint8_t cmd = 0x18 | (channel & 0x07);  /* Start + single-ended + channel */
    uint8_t b1, b2;

    spi_select();
    spi_send(0x01);           /* Start bit */
    b1 = spi_transfer(cmd);   /* Command, returns null bits + 2 MSB */
    b2 = spi_transfer(0x00);  /* Returns 8 LSB */
    spi_deselect();

    /* Combine result (10 bits) */
    return ((b1 & 0x03) << 8) | b2;
}
```

---

## Part 6: DMA with SPI

For high-speed transfers, use DMA to avoid CPU overhead:

```c
/* DMA is covered in Chapter 11: Direct Memory Access */
/* Basic concept: */

void spi_transfer_dma(const uint8_t *tx, uint8_t *rx, uint32_t len) {
    /* Configure DMA channel for TX */
    /* Configure DMA channel for RX */
    /* Start transfer */
    /* Wait for completion or use interrupt */
}
```

---

## Part 7: Troubleshooting

### Common SPI Problems

#### No Communication

```
Problem: Device not responding

Checklist:
□ Correct pin connections (MOSI, MISO, SCK, SS)
□ SS is pulled LOW during transfer
□ Clock speed not too fast for device
□ SPI mode matches device requirements
□ Device powered correctly
□ SSP clock enabled in SYSAHBCLKCTRL
□ SSP not in reset (PRESETCTRL)
```

#### Corrupted Data

```
Problem: Data shifted or wrong

Causes:
1. Wrong SPI mode (CPOL/CPHA)
2. Clock too fast
3. Missing or late SS assertion
4. FIFO overflow

Solutions:
□ Check device datasheet for mode
□ Reduce clock speed
□ Assert SS before first clock edge
□ Read data before FIFO fills
```

#### MISO Always High or Low

```
Problem: MISO stuck

Causes:
1. MISO not connected or wrong pin
2. Slave not selected (SS high)
3. Pull-up/pull-down interfering
4. Slave not powered

Debug:
□ Check continuity with multimeter
□ Verify SS goes low
□ Check IOCON for pull resistors
□ Measure slave power
```

### Debug with Logic Analyzer

Best tool for SPI debugging:

```
What to check:
1. SS goes low before first clock edge
2. Correct number of clock pulses
3. Data stable during sample edge
4. MISO responds after command sent
5. SS goes high after transfer complete

Timing:
       ___                                        ___
SS        \______________________________________/

              ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐  ...
SCK  ─────────┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└───────────

MOSI ─────────<─────── Command ───────>─────────

MISO ───────────────────<── Response ──>────────
```

### Debug with LEDs

```c
/**
 * Debug SPI with LED patterns
 */
void spi_debug_transfer(uint8_t data) {
    /* LED pattern before */
    led_pattern(0x01);

    spi_select();
    /* LED pattern during */
    led_pattern(0x03);

    uint8_t result = spi_transfer(data);

    /* LED pattern after */
    led_pattern(0x07);
    spi_deselect();

    /* Show result on LEDs (lower 4 bits) */
    led_pattern(result & 0x0F);
    delay(500000);
}
```

---

## Quick Reference

### SSP Registers

| Register | Address | Description |
|----------|---------|-------------|
| SSP0CR0 | 0x40040000 | Control Register 0 |
| SSP0CR1 | 0x40040004 | Control Register 1 |
| SSP0DR | 0x40040008 | Data Register |
| SSP0SR | 0x4004000C | Status Register |
| SSP0CPSR | 0x40040010 | Clock Prescaler |

### Status Bits (SSP0SR)

| Bit | Name | Description |
|-----|------|-------------|
| 4 | BSY | Busy |
| 3 | RFF | RX FIFO Full |
| 2 | RNE | RX FIFO Not Empty |
| 1 | TNF | TX FIFO Not Full |
| 0 | TFE | TX FIFO Empty |

### SPI Modes

| Mode | CPOL | CPHA | Clock Idle | Sample Edge |
|------|------|------|------------|-------------|
| 0 | 0 | 0 | Low | Rising |
| 1 | 0 | 1 | Low | Falling |
| 2 | 1 | 0 | High | Falling |
| 3 | 1 | 1 | High | Rising |

### Pin Configuration

| Pin | Function | IOCON Value |
|-----|----------|-------------|
| P0.6 | SCK0 | 0x02 (+ SCK0_LOC = 0x02) |
| P0.8 | MISO0 | 0x01 |
| P0.9 | MOSI0 | 0x01 |
| P0.2 | SSEL0 | 0x01 (or GPIO) |

---

## What's Next?

With SPI mastered, you can interface with:
- Flash memory for data logging
- SD cards for file storage
- TFT displays for graphics
- High-speed sensors

**Next Chapter:** [Chapter 10: Power Management](10-power-management.md) - Sleep modes and low-power techniques.

---

**Navigation:**
- Previous: [Chapter 8: I2C Communication](08-i2c-communication.md)
- Next: [Chapter 10: Power Management](10-power-management.md)
- [Back to Index](00-index.md)

---

*Chapter 9 of the LPC1343 Embedded C Learning Series*
*SPI Communication: Speed When You Need It*
