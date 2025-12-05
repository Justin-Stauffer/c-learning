# Building with GCC ARM Toolchain

This project has been set up to build with the GCC ARM toolchain (`arm-none-eabi-*`).

## Prerequisites

1. **Install GCC ARM Toolchain**
   - Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   - Make sure `arm-none-eabi-gcc` is in your PATH

2. **Verify Installation**
   ```bash
   arm-none-eabi-gcc --version
   ```

3. **Install Make** (if not already installed)
   - Windows: Install via MinGW, Cygwin, or use WSL
   - Linux/Mac: Usually pre-installed

## Project Files

- `startup_lpc1343_gcc.s` - Startup code (initializes hardware before main)
- `lpc1343_flash.ld` - Linker script (defines memory layout)
- `Makefile` - Build automation
- `.vscode/tasks.json` - VSCode build tasks
- `main.c` - Your application code

## Building from Command Line

### Build the Project
```bash
make
```
or
```bash
make all
```

This creates:
- `build/lpc1343_getting_started.elf` - Executable with debug symbols
- `build/lpc1343_getting_started.hex` - Intel HEX format (for programming)
- `build/lpc1343_getting_started.bin` - Raw binary format (for programming)
- `build/lpc1343_getting_started.map` - Memory map

### Clean Build Artifacts
```bash
make clean
```

### Rebuild Everything
```bash
make clean && make all
```

### Show Memory Usage
```bash
make size
```

Output example:
```
   text    data     bss     dec     hex filename
   4532     108    2048    6688    1a20 build/lpc1343_getting_started.elf
```
- **text**: Code + constants (goes in Flash)
- **data**: Initialized variables (goes in Flash, copied to RAM)
- **bss**: Uninitialized variables (goes in RAM only)

### Generate Disassembly
```bash
make disasm
```
Creates `build/lpc1343_getting_started.lst` with assembly code

## Building from VSCode

### Quick Build
1. Press `Ctrl+Shift+B` (default build task)
2. Or: Press `F1`, type "Run Build Task", select "Build Project"

### Other Tasks
Press `Ctrl+Shift+P`, type "Run Task", then select:
- **Build Project** - Compile the project
- **Clean Project** - Remove build files
- **Rebuild Project** - Clean then build
- **Show Memory Usage** - Display memory statistics
- **Generate Disassembly** - Create assembly listing
- **Flash via Serial** - Program via UART bootloader
- **Flash via OpenOCD** - Program via debug probe

## Programming the LPC1343

### Method 1: USB Bootloader (Easiest)
1. Hold the bootloader button while plugging in USB
2. A drive named `CRP DISABLD` appears
3. Delete `firmware.bin` on the drive
4. Copy `build/lpc1343_getting_started.bin` to the drive
5. Eject and reset

### Method 2: Serial Bootloader (UART)
```bash
make flash-serial
```
Requires `lpc21isp` tool and UART connection.

Edit the Makefile to change COM port:
```makefile
flash-serial: $(BUILD_DIR)/$(PROJECT).bin
    lpc21isp -bin $(BUILD_DIR)/$(PROJECT).bin COM3 115200 12000
    #                                          ^^^^
    #                                      Change this
```

### Method 3: Debug Probe (JTAG/SWD)
```bash
make flash-openocd
```
Requires OpenOCD and a debug probe (J-Link, ST-Link, CMSIS-DAP, etc.)

## Troubleshooting

### Error: "arm-none-eabi-gcc: command not found"
**Solution:** Add GCC ARM toolchain to your PATH

Windows:
```
set PATH=%PATH%;C:\Program Files\GNU Arm Embedded Toolchain\10 2021.10\bin
```

### Error: "make: command not found"
**Solution:** Install Make
- Windows: Use WSL or MinGW
- Linux: `sudo apt install make`
- Mac: Install Xcode Command Line Tools

### Build succeeds but code doesn't work
**Check:**
1. Memory usage: Run `make size`
   - Text must be < 32KB
   - Data+BSS must be < 8KB
2. Verify linker script matches your chip
3. Check startup code initializes correctly

### Code is too large
**Solutions:**
1. Change optimization level in Makefile:
   ```makefile
   CFLAGS = ... -O2 ...    # Change to -Os (optimize for size)
   ```
2. Remove unused code
3. Use `--gc-sections` linker flag (already enabled)

## ARM Intrinsics Reference

GCC provides built-in functions for common ARM operations:

### Interrupt Control
```c
// Disable/enable interrupts
__disable_irq();
__enable_irq();

// Or inline assembly
__asm volatile ("cpsid i" : : : "memory");  // disable
__asm volatile ("cpsie i" : : : "memory");  // enable
```

### No Operation
```c
__asm volatile ("nop");
```

## Next Steps

1. **Modify `main.c`** to add your application code
2. **Build** with `make`
3. **Check size** with `make size`
4. **Flash** to hardware
5. **Debug** if needed (requires debug probe and GDB)

## Additional Resources

- [GCC ARM Options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)
- [LPC1343 Datasheet](https://www.nxp.com/docs/en/data-sheet/LPC1311_13_42_43.pdf)
- [ARM Cortex-M3 Generic User Guide](https://developer.arm.com/documentation/dui0552/latest/)
- [OpenOCD Documentation](http://openocd.org/documentation/)

## Getting Help

If you encounter issues:
1. Check that all prerequisites are installed
2. Verify PATH includes toolchain binaries
3. Run `make clean` then `make all`
4. Check memory usage with `make size`
5. Review compiler errors carefully
