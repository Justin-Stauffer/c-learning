/**************************************************
 * LPC1343 SPI Example: Flash ID Read
 * Chapter 9: SPI Communication
 *
 * Reads the JEDEC ID from a W25Q16 SPI flash chip
 * to verify SPI communication is working.
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
 * W25Q16 JEDEC ID:
 *   Manufacturer: 0xEF (Winbond)
 *   Device ID: 0x4015 (W25Q16)
 *
 * Result:
 *   Success: 3 slow blinks
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
#define IOCON_PIO0_6    (*((volatile uint32_t *)0x40044060))  /* SCK */
#define IOCON_PIO0_8    (*((volatile uint32_t *)0x40044068))  /* MISO */
#define IOCON_PIO0_9    (*((volatile uint32_t *)0x4004406C))  /* MOSI */
#define IOCON_PIO0_2    (*((volatile uint32_t *)0x4004401C))  /* CS (GPIO) */

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
#define SSP_TFE         (1 << 0)   /* TX FIFO empty */
#define SSP_TNF         (1 << 1)   /* TX FIFO not full */
#define SSP_RNE         (1 << 2)   /* RX FIFO not empty */
#define SSP_BSY         (1 << 4)   /* SSP busy */

/*--------------------------------------------------
 * W25Q16 Commands
 *------------------------------------------------*/
#define W25Q_JEDEC_ID   0x9F       /* Read JEDEC ID */

/*--------------------------------------------------
 * Constants
 *------------------------------------------------*/
#define LED_PIN         7
#define CS_PIN          2          /* Chip select on P0.2 */

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
    GPIO0DATA &= ~(1 << CS_PIN);   /* Select chip */
}

static void cs_high(void) {
    GPIO0DATA |= (1 << CS_PIN);    /* Deselect chip */
}

/*--------------------------------------------------
 * Initialize SPI (SSP0)
 *
 * Configuration:
 * - Mode 0 (CPOL=0, CPHA=0)
 * - 8-bit data
 * - Master mode
 * - ~1 MHz clock (72 MHz / 72)
 *------------------------------------------------*/
static void spi_init(void) {
    /* Enable SSP0 clock */
    SYSAHBCLKCTRL |= (1 << 11);    /* Bit 11 = SSP0 */

    /* Set SSP0 clock divider */
    SSP0CLKDIV = 1;                /* PCLK = system clock */

    /* De-assert SSP0 reset */
    PRESETCTRL |= (1 << 0);        /* Bit 0 = SSP0 reset */

    /* Configure pins for SSP0 function:
     *
     * SCK location: P0.6 (location 2)
     * IOCON values: FUNC=0x02 for SSP0
     */
    IOCON_SCK_LOC = 0x02;          /* SCK on P0.6 */
    IOCON_PIO0_6 = 0x02;           /* P0.6 = SCK */
    IOCON_PIO0_8 = 0x01;           /* P0.8 = MISO */
    IOCON_PIO0_9 = 0x01;           /* P0.9 = MOSI */

    /* Configure CS pin as GPIO output */
    IOCON_PIO0_2 = 0x00;           /* P0.2 = GPIO */
    GPIO0DIR |= (1 << CS_PIN);     /* Output */
    cs_high();                     /* Deselect by default */

    /* Configure SSP0 Control Register 0:
     *
     * [3:0]  DSS  = 0x07 (8-bit data)
     * [5:4]  FRF  = 0x00 (SPI frame format)
     * [6]    CPOL = 0 (clock idle low)
     * [7]    CPHA = 0 (sample on rising edge)
     * [15:8] SCR  = 35 (serial clock rate)
     *
     * SPI clock = PCLK / (CPSR * (SCR + 1))
     *           = 72 MHz / (2 * 36) = 1 MHz
     */
    SSP0CR0 = 0x07                 /* 8-bit data */
            | (0 << 4)             /* SPI format */
            | (0 << 6)             /* CPOL = 0 */
            | (0 << 7)             /* CPHA = 0 */
            | (35 << 8);           /* SCR = 35 */

    /* Set clock prescaler (must be even, >= 2) */
    SSP0CPSR = 2;

    /* Enable SSP0:
     * [0] LBM = 0 (normal mode, not loopback)
     * [1] SSE = 1 (SSP enabled)
     * [2] MS  = 0 (master mode)
     */
    SSP0CR1 = (1 << 1);
}

/*--------------------------------------------------
 * SPI Transfer Single Byte
 *
 * Sends a byte and returns the received byte.
 * SPI is full-duplex: every send is also a receive.
 *------------------------------------------------*/
static uint8_t spi_transfer(uint8_t data) {
    /* Wait for TX FIFO not full */
    while ((SSP0SR & SSP_TNF) == 0);

    /* Send byte */
    SSP0DR = data;

    /* Wait for transfer complete */
    while (SSP0SR & SSP_BSY);

    /* Wait for RX FIFO not empty */
    while ((SSP0SR & SSP_RNE) == 0);

    /* Return received byte */
    return SSP0DR;
}

/*--------------------------------------------------
 * Read W25Q16 JEDEC ID
 *
 * Returns 3 bytes: manufacturer ID, memory type, capacity
 * Expected: 0xEF, 0x40, 0x15 for W25Q16
 *------------------------------------------------*/
static void w25q_read_jedec_id(uint8_t *mfr, uint8_t *type, uint8_t *cap) {
    cs_low();

    /* Send JEDEC ID command */
    spi_transfer(W25Q_JEDEC_ID);

    /* Read 3 bytes of ID */
    *mfr = spi_transfer(0xFF);     /* Manufacturer ID */
    *type = spi_transfer(0xFF);    /* Memory type */
    *cap = spi_transfer(0xFF);     /* Capacity */

    cs_high();
}

/*--------------------------------------------------
 * Main Function
 *------------------------------------------------*/
int main(void) {
    uint8_t manufacturer, mem_type, capacity;
    uint8_t success;

    /* Enable GPIO clock */
    SYSAHBCLKCTRL |= (1 << 6);

    /* Configure LED pin as output */
    GPIO0DIR |= (1 << LED_PIN);
    led_off();

    /* Initialize SPI */
    spi_init();

    /* Small delay for flash chip to power up */
    delay(100000);

    /* Read JEDEC ID */
    w25q_read_jedec_id(&manufacturer, &mem_type, &capacity);

    /* Verify ID:
     * Manufacturer: 0xEF (Winbond)
     * Memory Type:  0x40
     * Capacity:     0x15 (16 Mbit)
     */
    success = (manufacturer == 0xEF) &&
              (mem_type == 0x40) &&
              (capacity == 0x15);

    /* Visual feedback */
    while (1) {
        if (success) {
            /* Success: 3 slow blinks */
            for (int i = 0; i < 3; i++) {
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
