# Chapter 9: SPI Examples Specifications

## Hardware Requirements
- LPC-P1343 development board
- W25Q16 SPI Flash module (16 Mbit / 2 MB)
- Jumper wires

---

## Example 1: SPI-Flash-ID
**Status:** CREATED

### Purpose
Read the manufacturer and device ID from W25Q16 flash chip to verify SPI communication.

### Hardware Connections
```
W25Q16 Module:
  VCC  → 3.3V
  GND  → GND
  CLK  → P0.6 (SCK)
  DO   → P0.8 (MISO)
  DI   → P0.9 (MOSI)
  /CS  → P0.2 (GPIO, active low)
  /WP  → 3.3V (disable write protect)
  /HOLD → 3.3V (disable hold)
```

### Key Concepts
- SSP0 peripheral initialization
- SPI mode 0 (CPOL=0, CPHA=0)
- Chip select (CS) management
- Command-based protocol

### W25Q16 Commands
```c
#define W25Q_JEDEC_ID       0x9F    /* Read JEDEC ID */
#define W25Q_READ_DATA      0x03    /* Read data */
#define W25Q_PAGE_PROGRAM   0x02    /* Write page */
#define W25Q_WRITE_ENABLE   0x06    /* Enable writing */
#define W25Q_READ_STATUS1   0x05    /* Read status reg 1 */
#define W25Q_SECTOR_ERASE   0x20    /* Erase 4KB sector */
```

### Expected Behavior
- Read JEDEC ID: Manufacturer=0xEF, Device=0x4015
- LED indicates success (3 blinks) or failure (rapid blink)

---

## Example 2: SPI-Flash-ReadWrite
**Status:** CREATED

### Purpose
Write data to flash, read it back, and verify.

### Key Concepts
- Flash memory architecture (pages, sectors)
- Erase-before-write requirement
- Status polling for operation complete
- Data verification

### Expected Behavior
- Erase sector 0
- Write test pattern
- Read back and verify
- LED indicates success/failure

---

## Common SPI (SSP0) Register Definitions

```c
/* System Clock Control */
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define SSP0CLKDIV      (*((volatile uint32_t *)0x40048094))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))

/* IOCON for SPI pins */
#define IOCON_SCK_LOC   (*((volatile uint32_t *)0x400440B0))  /* SCK location */
#define IOCON_PIO0_6    (*((volatile uint32_t *)0x40044060))  /* SCK */
#define IOCON_PIO0_8    (*((volatile uint32_t *)0x40044068))  /* MISO */
#define IOCON_PIO0_9    (*((volatile uint32_t *)0x4004406C))  /* MOSI */
#define IOCON_PIO0_2    (*((volatile uint32_t *)0x4004401C))  /* CS (GPIO) */

/* SSP0 Registers */
#define SSP0CR0         (*((volatile uint32_t *)0x40040000))
#define SSP0CR1         (*((volatile uint32_t *)0x40040004))
#define SSP0DR          (*((volatile uint32_t *)0x40040008))
#define SSP0SR          (*((volatile uint32_t *)0x4004000C))
#define SSP0CPSR        (*((volatile uint32_t *)0x40040010))

/* SSP Status bits */
#define SSP_TFE         (1 << 0)   /* TX FIFO empty */
#define SSP_TNF         (1 << 1)   /* TX FIFO not full */
#define SSP_RNE         (1 << 2)   /* RX FIFO not empty */
#define SSP_RFF         (1 << 3)   /* RX FIFO full */
#define SSP_BSY         (1 << 4)   /* SSP busy */
```

---

## SPI Modes

| Mode | CPOL | CPHA | Description |
|------|------|------|-------------|
| 0 | 0 | 0 | Clock idle low, sample on rising edge |
| 1 | 0 | 1 | Clock idle low, sample on falling edge |
| 2 | 1 | 0 | Clock idle high, sample on falling edge |
| 3 | 1 | 1 | Clock idle high, sample on rising edge |

W25Q16 uses Mode 0 or Mode 3.

---

## W25Q16 Flash Memory Details

| Parameter | Value |
|-----------|-------|
| Capacity | 16 Mbit (2 MB) |
| Page Size | 256 bytes |
| Sector Size | 4 KB (16 pages) |
| Block Size | 64 KB |
| Max SPI Clock | 104 MHz |
| Operating Voltage | 2.7V - 3.6V |

### Memory Organization
```
Address Range       Size
0x000000 - 0x1FFFFF  2 MB total
Each sector: 4 KB (must erase before write)
Each page: 256 bytes (max write size)
```

---

## Build Notes
- Each example uses the same Makefile template
- startup_lpc1343_gcc.s and lpc1343_flash.ld copied to each folder
- Build with: `make`
- Flash with: `make flash`

---

## Testing Checklist

- [ ] Example 1: JEDEC ID read correctly (0xEF, 0x40, 0x15)
- [ ] Example 2: Write and read back verified

---

*Chapter 9 of the LPC1343 Examples Series*
*SPI Communication*
