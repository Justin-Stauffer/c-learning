# Getting Started - LED Blink Example

The simplest possible LED blinking program for the LPC1343.

## What This Example Does

Blinks LED0 (P3.0) on the LPC-P1343 board approximately twice per second.

## Prerequisites

1. **GCC ARM Toolchain** installed and in PATH
   ```bash
   # Verify installation
   arm-none-eabi-gcc --version
   ```

2. **OpenOCD** installed (for flashing via ST-Link)
   ```bash
   # macOS
   brew install openocd

   # Verify installation
   openocd --version
   ```

3. **ST-Link V2** connected to LPC-P1343 (see Chapter 7 in learning-docs)

## Building

```bash
# Build the project
make

# You should see output like:
#   CC    main.c
#   AS    startup_lpc1343_gcc.s
#   LD    build/lpc1343_blink.elf
#   === Memory Usage ===
#      text    data     bss     dec     hex filename
#       xxx      xx      xx     xxx     xxx build/lpc1343_blink.elf
```

## Flashing

```bash
# Flash via ST-Link
make flash

# If successful, LED should start blinking!
```

## Files

| File | Description |
|------|-------------|
| `main.c` | Application code - the LED blink logic |
| `startup_lpc1343_gcc.s` | Startup code - vector table and initialization |
| `lpc1343_flash.ld` | Linker script - memory layout for LPC1343 |
| `Makefile` | Build automation |

## Troubleshooting

### "arm-none-eabi-gcc: command not found"
The toolchain is not in your PATH. Install it or add to PATH.

### OpenOCD can't connect
1. Check ST-Link USB connection
2. Check wiring (SWDIO, SWCLK, GND, RST, 3.3V)
3. Try: `openocd -f interface/stlink.cfg -f target/lpc13xx.cfg`

### LED doesn't blink
1. Verify the flash succeeded (OpenOCD shows "verified")
2. Press the reset button on the board
3. Check that you're looking at the correct LED (P3.0 = LED0)

## Next Steps

After getting this working, proceed to:
- [Chapter 1: Bitwise Operations](../../learning-docs/01-bitwise-operations.md)
- [Chapter 3: GPIO In-Depth](../../learning-docs/03-gpio-in-depth.md)
