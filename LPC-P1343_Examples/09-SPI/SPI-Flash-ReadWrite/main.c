/**************************************************
 * LPC1343 SPI Example: Flash Read/Write
 * Chapter 9: SPI Communication
 *
 * Demonstrates writing data to W25Q16 SPI flash
 * and reading it back for verification.
 *
 * Hardware:
 *   W25Q16 Module:
 *     VCC   → 3.3V
 *     GND   → GND
 *     CLK   → P0.6 (SCK)
 *     DO    → P0.8 (MISO)
 *     DI    → P0.9 (MOSI)
 *     /CS   → P0.2 (GPIO)
 *     /WP   → 3.3V
 *     /HOLD → 3.3V
 *
 *   LED: P0.7 (onboard, active low)
 *
 * Flash Memory Notes:
 *   - Must ERASE before WRITE (flash can only 1→0)
 *   - Erase is per sector (4KB minimum)
 *   - Write is per page (256 bytes maximum)
 *   - Operations require WRITE ENABLE first
 *
 * Result:
 *   Success: 5 slow blinks
 *   Failure: Rapid blink pattern
 **************************************************/

#include <stdint.h>

/*--------------------------------------------------
 * System Control Registers
 *------------------------------------------------*/
#define SYSAHBCLKCTRL   (*((volatile uint32_t *)0x40048080))
#define SSP0CLKDIV      (*((volatile uint32_t *)0x40048094))
#define PRESETCTRL      (*((volatile uint32_t *)0x40048004))

/*--------------------------------------------------
 * IOCON Registers (Pin Configuration)
 *------------------------------------------------*/
#define IOCON_SCK_LOC   (*((volatile uint32_t *)0x400440B0))
#define IOCON_PIO0_6    (*((volatile uint32_t *)0x40044060))
#define IOCON_PIO0_8    (*((volatile uint32_t *)0x40044068))
#define IOCON_PIO0_9    (*((volatile uint32_t *)0x4004406C))
#define IOCON_PIO0_2    (*((volatile uint32_t *)0x4004401C))

/*--------------------------------------------------
 * GPIO Registers
 *------------------------------------------------*/
#define GPIO0DIR        (*((volatile uint32_t *)0x50008000))
#define GPIO0DATA       (*((volatile uint32_t *)0x50003FFC))

/*--------------------------------------------------
 * SSP0 Registers (SPI)
 *------------------------------------------------*/
#define SSP0CR0         (*((volatile uint32_t *)0x40040000))
#define SSP0CR1         (*((volatile uint32_t *)0x40040004))
#define SSP0DR          (*((volatile uint32_t *)0x40040008))
#define SSP0SR          (*((volatile uint32_t *)0x4004000C))
#define SSP0CPSR        (*((volatile uint32_t *)0x40040010))

/*--------------------------------------------------
 * SSP Status Bits
 *------------------------------------------------*/
#define SSP_TFE         (1 << 0)
#define SSP_TNF         (1 << 1)
#define SSP_RNE         (1 << 2)
#define SSP_BSY         (1 << 4)

/*--------------------------------------------------
 * W25Q16 Commands
 *------------------------------------------------*/
#define W25Q_WRITE_ENABLE   0x06
#define W25Q_WRITE_DISABLE  0x04
#define W25Q_READ_STATUS1   0x05
#define W25Q_READ_DATA      0x03
#define W25Q_PAGE_PROGRAM   0x02
#define W25Q_SECTOR_ERASE   0x20
#define W25Q_JEDEC_ID       0x9F

/* Status Register 1 bits */
#define W25Q_BUSY           (1 << 0)
#define W25Q_WEL            (1 << 1)   /* Write Enable Latch */

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define CS_PIN          2
#define TEST_ADDR       0x000000   /* Test at sector 0 */
#define TEST_SIZE       16         /* Bytes to test */

/*--------------------------------------------------
 * Test Data
 *------------------------------------------------*/
static const uint8_t test_pattern[TEST_SIZE] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

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
 * Chip Select Control
 *------------------------------------------------*/
static void cs_low(void) {
    GPIO0DATA &= ~(1 << CS_PIN);
}

static void cs_high(void) {
    GPIO0DATA |= (1 << CS_PIN);
}

/*--------------------------------------------------
 * Initialize SPI (SSP0)
 *------------------------------------------------*/
static void spi_init(void) {
    SYSAHBCLKCTRL |= (1 << 11);
    SSP0CLKDIV = 1;
    PRESETCTRL |= (1 << 0);

    IOCON_SCK_LOC = 0x02;
    IOCON_PIO0_6 = 0x02;
    IOCON_PIO0_8 = 0x01;
    IOCON_PIO0_9 = 0x01;

    IOCON_PIO0_2 = 0x00;
    GPIO0DIR |= (1 << CS_PIN);
    cs_high();

    SSP0CR0 = 0x07 | (0 << 4) | (0 << 6) | (0 << 7) | (35 << 8);
    SSP0CPSR = 2;
    SSP0CR1 = (1 << 1);
}

/*--------------------------------------------------
 * SPI Transfer Single Byte
 *------------------------------------------------*/
static uint8_t spi_transfer(uint8_t data) {
    while ((SSP0SR & SSP_TNF) == 0);
    SSP0DR = data;
    while (SSP0SR & SSP_BSY);
    while ((SSP0SR & SSP_RNE) == 0);
    return SSP0DR;
}

/*--------------------------------------------------
 * Read W25Q16 Status Register 1
 *------------------------------------------------*/
static uint8_t w25q_read_status(void) {
    uint8_t status;

    cs_low();
    spi_transfer(W25Q_READ_STATUS1);
    status = spi_transfer(0xFF);
    cs_high();

    return status;
}

/*--------------------------------------------------
 * Wait for W25Q16 to Complete Operation
 *------------------------------------------------*/
static void w25q_wait_busy(void) {
    while (w25q_read_status() & W25Q_BUSY) {
        /* Wait for BUSY bit to clear */
    }
}

/*--------------------------------------------------
 * Enable Write Operations
 *------------------------------------------------*/
static void w25q_write_enable(void) {
    cs_low();
    spi_transfer(W25Q_WRITE_ENABLE);
    cs_high();

    /* Verify WEL bit is set */
    while ((w25q_read_status() & W25Q_WEL) == 0);
}

/*--------------------------------------------------
 * Erase 4KB Sector
 *
 * Flash can only change 1→0, so we must erase
 * (set all bits to 1) before writing new data.
 *------------------------------------------------*/
static void w25q_erase_sector(uint32_t addr) {
    w25q_write_enable();

    cs_low();
    spi_transfer(W25Q_SECTOR_ERASE);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);
    cs_high();

    /* Sector erase takes up to 400ms */
    w25q_wait_busy();
}

/*--------------------------------------------------
 * Write Data (Page Program)
 *
 * Maximum 256 bytes per write, must not cross
 * page boundary.
 *------------------------------------------------*/
static void w25q_write_page(uint32_t addr, const uint8_t *data, uint8_t len) {
    w25q_write_enable();

    cs_low();
    spi_transfer(W25Q_PAGE_PROGRAM);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);

    for (uint8_t i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    cs_high();

    /* Page program takes up to 3ms */
    w25q_wait_busy();
}

/*--------------------------------------------------
 * Read Data
 *------------------------------------------------*/
static void w25q_read_data(uint32_t addr, uint8_t *data, uint8_t len) {
    cs_low();
    spi_transfer(W25Q_READ_DATA);
    spi_transfer((addr >> 16) & 0xFF);
    spi_transfer((addr >> 8) & 0xFF);
    spi_transfer(addr & 0xFF);

    for (uint8_t i = 0; i < len; i++) {
        data[i] = spi_transfer(0xFF);
    }
    cs_high();
}

/*--------------------------------------------------
 * Verify JEDEC ID
 *------------------------------------------------*/
static uint8_t w25q_verify_id(void) {
    uint8_t mfr, type, cap;

    cs_low();
    spi_transfer(W25Q_JEDEC_ID);
    mfr = spi_transfer(0xFF);
    type = spi_transfer(0xFF);
    cap = spi_transfer(0xFF);
    cs_high();

    return (mfr == 0xEF) && (type == 0x40) && (cap == 0x15);
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint8_t read_buffer[TEST_SIZE];
    uint8_t success = 1;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);
    led_off();

    /* Initialize SPI */
    spi_init();
    delay(100000);

    /* Step 1: Verify flash chip */
    if (!w25q_verify_id()) {
        success = 0;
        goto result;
    }

    /* Quick blink to show ID verified */
    led_on();
    delay(100000);
    led_off();
    delay(100000);

    /* Step 2: Erase sector (required before write) */
    w25q_erase_sector(TEST_ADDR);

    /* Quick blink to show erase complete */
    led_on();
    delay(100000);
    led_off();
    delay(100000);

    /* Step 3: Write test pattern */
    w25q_write_page(TEST_ADDR, test_pattern, TEST_SIZE);

    /* Quick blink to show write complete */
    led_on();
    delay(100000);
    led_off();
    delay(100000);

    /* Step 4: Read back data */
    w25q_read_data(TEST_ADDR, read_buffer, TEST_SIZE);

    /* Step 5: Verify data */
    for (uint8_t i = 0; i < TEST_SIZE; i++) {
        if (read_buffer[i] != test_pattern[i]) {
            success = 0;
            break;
        }
    }

result:
    /* Visual feedback */
    while (1) {
        if (success) {
            /* Success: 5 slow blinks */
            for (int i = 0; i < 5; i++) {
                led_on();
                delay(300000);
                led_off();
                delay(300000);
            }
            delay(1000000);
        } else {
            /* Failure: rapid blink */
            led_on();
            delay(50000);
            led_off();
            delay(50000);
        }
    }

    return 0;
}
