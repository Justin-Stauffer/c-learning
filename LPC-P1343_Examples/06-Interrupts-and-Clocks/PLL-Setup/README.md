# PLL-Setup

Chapter 6: Interrupts and Clocks - PLL Configuration Example

## What This Example Demonstrates

- LPC1343 clock system architecture
- PLL configuration from 12 MHz to 72 MHz
- Main clock source switching
- Visual demonstration of clock speed change

## Hardware

- P3.0-P3.3: LEDs (active-low)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Phase 1 (12 MHz IRC)**: LED0 blinks 5 times - relatively slow
2. **PLL Switch**: Brief pause while PLL locks
3. **Phase 2 (72 MHz PLL)**: LED1 blinks 5 times with same delay count - visibly faster!
4. **Running**: LED2 and LED3 alternate to show system is running at 72 MHz

The speed difference is dramatic: the same delay loop count results in 6x faster blinking after PLL activation.

## Code Highlights

**PLL configuration for 72 MHz:**
```c
void pll_init_72mhz(void) {
    /* Power up PLL */
    PDRUNCFG &= ~(1 << 7);

    /* Configure: MSEL=5 (×6), PSEL=1 (P=2) */
    SYSPLLCTRL = (1 << 5) | 5;  /* 0x25 */

    /* Wait for PLL lock */
    while (!(SYSPLLSTAT & 0x01));

    /* Set divider to 1 */
    SYSAHBCLKDIV = 1;

    /* Select PLL output as main clock */
    MAINCLKSEL = 0x03;
    MAINCLKUEN = 0;
    MAINCLKUEN = 1;
    while (!(MAINCLKUEN & 0x01));
}
```

## PLL Math

```
Input:  12 MHz (IRC)
Output: 72 MHz

MSEL = (F_out / F_in) - 1 = (72 / 12) - 1 = 5

CCO must be 156-320 MHz:
CCO = F_out × 2 × P (where P = 2^PSEL)

With PSEL=1 (P=2):
CCO = 72 × 2 × 2 = 288 MHz  ✓ (in valid range)
```

## Clock Registers

| Register | Address | Description |
|----------|---------|-------------|
| SYSPLLCTRL | 0x40048008 | PLL control (MSEL, PSEL) |
| SYSPLLSTAT | 0x4004800C | PLL status (lock bit) |
| MAINCLKSEL | 0x40048070 | Main clock source select |
| MAINCLKUEN | 0x40048074 | Main clock update enable |
| SYSAHBCLKDIV | 0x40048078 | AHB bus clock divider |
| PDRUNCFG | 0x40048238 | Power-down configuration |

## MAINCLKSEL Values

| Value | Clock Source |
|-------|--------------|
| 0 | IRC Oscillator (12 MHz) |
| 1 | PLL Input |
| 2 | Watchdog Oscillator |
| 3 | PLL Output |

## Power-Down Configuration (PDRUNCFG)

| Bit | Peripheral |
|-----|------------|
| 5 | System oscillator |
| 7 | System PLL |

Write 0 to power up, 1 to power down.

## Why 72 MHz?

- Maximum rated speed for LPC1343
- 6x faster than 12 MHz IRC
- Better performance for computation
- Higher baud rates possible for UART
- Required for USB operation
