# Chapter 2: From Source Code to Firmware

A complete beginner's guide to understanding how your C code becomes firmware that runs on the LPC1343 microcontroller.

---

## Chapter Overview

| | |
|---|---|
| **Prerequisites** | Chapter 0-1 (Getting Started, Bitwise Operations) |
| **Time to Complete** | 1-2 hours |
| **Hands-On Projects** | Build from command line, examine output files |
| **You Will Learn** | How the toolchain transforms code into firmware |

---

## Quick Start: Build Commands You Need

Here's how to build your project from the command line:

```bash
# Build everything
make

# See what got created
ls build/
# lpc1343_getting_started.elf  - Executable with debug info
# lpc1343_getting_started.hex  - Intel HEX for programming
# lpc1343_getting_started.bin  - Raw binary for programming
# lpc1343_getting_started.map  - Memory layout

# Check memory usage
make size
#    text    data     bss     dec     hex filename
#    4532     108    2048    6688    1a20 build/lpc1343_getting_started.elf
# text = code (Flash), data = initialized vars, bss = uninitialized vars

# Clean and rebuild
make clean && make

# Flash to hardware
make flash    # via debug probe (ST-Link)
# OR copy build/lpc1343_getting_started.bin to the CRP DISABLD drive
```

The rest of this chapter explains what these commands actually do and why.

---

## Part 1: Understanding the Files in Your Project

Every example in this curriculum contains these files:

### Source Code Files

#### **main.c** - Your Application Code
```c
int main(void) {
    init_devices();
    while(1) { }
}
```
- Human-readable C code
- Contains your program logic
- Gets compiled into machine code
- This is where YOU write your program

#### **startup_lpc1343_gcc.s** - Startup Code (Assembly)
```assembly
g_pfnVectors:
    .word   _estack                 ; Top of Stack
    .word   Reset_Handler           ; Reset Handler
    .word   NMI_Handler             ; NMI Handler
    ...
```
- Written in ARM assembly language (GNU syntax)
- Runs **BEFORE** your `main()` function
- Creates the **Vector Table** (list of interrupt handler addresses)
- Sets up the stack pointer
- Initializes global variables
- Jumps to your `main()` function

**Why is this needed?**
When the microcontroller powers on or resets:
1. CPU reads address `0x00000000` to get initial stack pointer
2. CPU reads address `0x00000004` to get reset handler address
3. CPU jumps to reset handler (startup code)
4. Startup code initializes everything
5. Finally jumps to your `main()`

For a deep dive into startup assembly, see [Appendix D: Startup Assembly Explained](appendix/D-startup-assembly-explained.md).

### Configuration Files

#### **lpc1343_flash.ld** - Linker Script

This file tells the **linker** where to place code and data in memory.

```
Memory Layout for LPC1343:
┌─────────────────────────────┐ 0x00000000
│  Interrupt Vector Table     │ (first 0x124 bytes = 292 bytes)
├─────────────────────────────┤ 0x00000124
│                             │
│  Flash Memory (ROM)         │ Your program code lives here
│  Program Code               │ Read-only, survives power loss
│  Const Data                 │ 32 KB total (0x8000 bytes)
│                             │
└─────────────────────────────┘ 0x00007FFF

┌─────────────────────────────┐ 0x10000000
│                             │
│  RAM (SRAM)                 │ Variables, stack, heap
│  Global Variables           │ Read/write, lost on power loss
│  Stack (grows down)         │ 8 KB total (0x2000 bytes)
│  Heap (malloc area)         │
│                             │
└─────────────────────────────┘ 0x10001FFF
```

**Key memory regions:**
```
ROM: 0x00000000 to 0x00007FFF  (32 KB) - Where your code lives
RAM: 0x10000000 to 0x10001FFF  (8 KB)  - Where variables live
Stack: ~2 KB                           - For function calls
Heap: ~512 bytes                       - For malloc() (if used)
```

#### **Makefile** - Build Automation

The Makefile automates the entire build process:

```makefile
# Key variables
PROJECT = lpc1343_example
CC = arm-none-eabi-gcc
LDSCRIPT = lpc1343_flash.ld

# Build targets
all: $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin
clean: rm -rf build/
flash: openocd -c "program $(PROJECT).elf reset exit"
```

Instead of typing long compiler commands, you just type `make`.

---

## Part 2: The Build Process (Compilation Pipeline)

Here's what happens when you run `make`:

```
Step 1: PREPROCESSING
main.c + header files
        ↓
    [Preprocessor]
        ↓
Expanded .c file
(macros expanded, #includes inserted)

Step 2: COMPILATION
Expanded .c file
        ↓
    [Compiler]
        ↓
main.o (object file)
Machine code, not yet linked

Step 3: ASSEMBLY (for startup code)
startup_lpc1343_gcc.s
        ↓
    [Assembler]
        ↓
startup.o (object file)

Step 4: LINKING
main.o + startup.o + libraries
        ↓
    [Linker] (uses lpc1343_flash.ld)
        ↓
program.elf (executable)
Complete program with addresses resolved

Step 5: CONVERSION
program.elf
        ↓
    [objcopy]
        ↓
program.hex or program.bin
Ready to flash to microcontroller!
```

### Detailed Explanation of Each Step

#### Step 1: Preprocessing
```bash
# Preprocessor handles:
#include <stdint.h>           → Inserts file contents
#define LED_PIN 7             → Text substitution
#ifdef DEBUG ... #endif       → Conditional compilation
```

**Output:** Pure C code with all macros expanded

#### Step 2: Compilation
```bash
# Compiler translates C to machine code:

C Code:
    GPIO0DATA |= (1 << 7);

Assembly equivalent (ARM Thumb-2):
    LDR  R0, =GPIO0DATA   ; Load address of GPIO0DATA
    LDR  R1, [R0]         ; Read current value
    ORR  R1, R1, #0x80    ; Set bit 7
    STR  R1, [R0]         ; Write back
```

**Output:** `.o` object files (machine code, but addresses not final)

#### Step 3: Linking

The **linker** combines all object files into one executable:

```
What the linker does:
1. Combines all .o files
2. Resolves function calls (connects main() to init_devices())
3. Assigns final memory addresses using linker script
4. Adds startup code
5. Creates complete memory image

Example:
Before linking:
    Function init_devices() at relative address 0x???
    Call to init_devices() from main() at 0x???

After linking:
    Function init_devices() at absolute address 0x000001A4
    Call instruction updated to jump to 0x000001A4
```

**Output:** `.elf` file (Executable and Linkable Format)

#### Step 4: Format Conversion

Microcontrollers need firmware in specific formats:

**ELF file** (.elf) - Contains:
- Executable code
- Debug symbols (variable names, line numbers)
- Section information
- Used for debugging

**HEX file** (.hex) - Intel Hex Format:
```
:10000000B0050020C9010000D70100001D020000EE
:10001000000000000000000000000000000000003C
```
- Text format (human-readable addresses)
- Contains: address + data + checksum
- Used by most programmer tools

**BIN file** (.bin) - Raw Binary:
```
B0 05 00 20 C9 01 00 00 D7 01 00 00 1D 02 00 00
```
- Pure binary data
- Exact image of what goes in Flash
- Smallest file size

---

## Part 3: Common Output File Types

| Extension | Name | Description | Usage |
|-----------|------|-------------|-------|
| `.c` | C Source | Human-readable source code | Your code |
| `.h` | Header | Function declarations, macros | #included in .c files |
| `.s` | Assembly | Low-level assembly code | Startup, critical routines |
| `.o` | Object | Compiled but not linked | Intermediate build output |
| `.elf` | Executable | Complete program with debug info | Debugging |
| `.hex` | Intel Hex | Text format firmware | Programming/flashing |
| `.bin` | Binary | Raw binary firmware | Programming/flashing |
| `.map` | Map File | Memory layout report | See what's where |
| `.d` | Dependencies | Header file dependencies | Incremental builds |

---

## Part 4: How to Build Firmware (Step by Step)

### Build from Command Line

**1. Build the Project**
```bash
make
```
or
```bash
make all
```

This automatically:
- Compiles `main.c` → `main.o`
- Assembles `startup_lpc1343_gcc.s` → `startup_lpc1343_gcc.o`
- Links all objects → `project.elf`
- Converts to HEX → `project.hex`
- Converts to BIN → `project.bin`

**2. Build Output Files Created**
```
build/ folder:
├── project.elf           (executable with debug info)
├── project.hex           (Intel HEX for programming)
├── project.bin           (raw binary for programming)
├── project.map           (memory map)
├── main.o                (compiled main.c)
├── startup_lpc1343_gcc.o (compiled startup)
└── *.d                   (dependency files)
```

**3. View Memory Usage**
```bash
make size
```

Output:
```
   text    data     bss     dec     hex filename
    748       8    1568    2324     914 build/project.elf

text:  748 bytes (Flash) - Your code + constants
data:    8 bytes (Flash→RAM) - Initialized variables
bss:  1568 bytes (RAM) - Uninitialized variables + stack
```

**4. Clean Build Artifacts**
```bash
make clean
```

**5. Rebuild Everything**
```bash
make clean && make
```

### Build from VSCode

**Quick Build:**
- Press `Ctrl+Shift+B` (runs default build task)

**Other Tasks:**
Press `Ctrl+Shift+P`, type "Run Task", select:
- **Build Project** - Compile everything
- **Clean Project** - Remove build files
- **Rebuild Project** - Clean then build
- **Flash Project** - Program the microcontroller

### Manual Build Commands (Without Makefile)

If you want to understand what the Makefile does, here are the manual commands:

```bash
# 1. Create build directory
mkdir build

# 2. Compile C files to object files
arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb -O2 -Wall \
    -fdata-sections -ffunction-sections \
    main.c -o build/main.o

# 3. Assemble startup code
arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb \
    -x assembler-with-cpp \
    startup_lpc1343_gcc.s -o build/startup_lpc1343_gcc.o

# 4. Link everything together
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb \
    -specs=nano.specs -T lpc1343_flash.ld \
    -Wl,-Map=build/program.map,--cref -Wl,--gc-sections \
    build/main.o build/startup_lpc1343_gcc.o \
    -o build/program.elf

# 5. Convert to hex format
arm-none-eabi-objcopy -O ihex build/program.elf build/program.hex

# 6. Convert to binary format
arm-none-eabi-objcopy -O binary build/program.elf build/program.bin

# 7. Show size
arm-none-eabi-size build/program.elf
```

**Explanation of key flags:**
- `-mcpu=cortex-m3` - Target ARM Cortex-M3 processor
- `-mthumb` - Use Thumb instruction set (16/32-bit mixed)
- `-O2` - Optimization level 2 (balance speed/size)
- `-fdata-sections -ffunction-sections` - Put each function/data in its own section
- `-Wl,--gc-sections` - Remove unused sections (linker garbage collection)
- `-specs=nano.specs` - Use newlib-nano (smaller C library)
- `-T lpc1343_flash.ld` - Use this linker script

---

## Part 5: Programming (Flashing) the Microcontroller

Once you have a `.hex` or `.bin` file, you need to **flash** it to the microcontroller's Flash memory.

### Method 1: Using ST-Link Debug Probe (Recommended)

**Hardware needed:**
- ST-Link V2 debug probe (~$10-15)
- Connects to SWD pins on the LPC-P1343 board

**Flash with make:**
```bash
make flash
```

**Or manually with OpenOCD:**
```bash
openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
    -c "program build/project.elf reset exit"
```

**How it works:**
```
PC ←→ [USB] ←→ [ST-Link] ←→ [SWD] ←→ [LPC1343]
```

The ST-Link:
- Writes firmware into Flash memory
- Can set breakpoints for debugging
- Can read/write RAM and registers in real-time

### Method 2: USB Bootloader (No Debug Probe Needed)

The LPC1343 has a built-in USB bootloader:

1. **Enter Bootloader Mode:**
   - Hold the ISP/bootloader button while plugging in USB
   - Or short ISP pin to GND while resetting
   - Board appears as USB Mass Storage device

2. **Copy Firmware:**
   - A drive appears: `CRP DISABLD` or similar
   - Delete existing `firmware.bin` (if present)
   - Drag and drop your `.bin` file to the drive

3. **Reset:**
   - Eject the drive safely
   - Press reset button or re-plug USB
   - Your code runs!

**Note:** This is the easiest method and requires no additional hardware!

### Method 3: Serial Bootloader (UART)

Flash via UART using the `lpc21isp` tool:

```bash
# Syntax: lpc21isp -bin <file> <port> <baud> <crystal_khz>
lpc21isp -bin build/project.bin /dev/ttyUSB0 115200 12000

# Windows example:
lpc21isp -bin build/project.bin COM3 115200 12000
```

**Hardware Connection:**
```
USB-UART Adapter    LPC1343
TX   →              RX (P1.6)
RX   ←              TX (P1.7)
GND  ←→             GND
```

---

## Part 6: The Complete Flow Diagram

```
┌──────────────────────────────────────────────────────┐
│                  DEVELOPMENT PHASE                    │
└──────────────────────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Write Code: main.c, headers, etc.   │
    └──────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────┐
│                    BUILD PHASE                        │
└──────────────────────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Preprocess: Expand macros           │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Compile: C → ARM machine code       │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Assemble: startup.s → machine code  │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Link: Combine all .o files          │
    │  Output: program.elf                 │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Convert: .elf → .hex and .bin       │
    └──────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────┐
│                  PROGRAMMING PHASE                    │
└──────────────────────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Connect ST-Link or USB cable        │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Flash firmware to microcontroller   │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Reset microcontroller               │
    └──────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Your code runs on hardware!         │
    └──────────────────────────────────────┘
```

---

## Part 7: Understanding Memory After Programming

After flashing, this is what's in the LPC1343's memory:

### Flash Memory (Non-volatile - Survives Power Off)
```
Address      Content
0x00000000:  [Vector Table]
             - Stack pointer: 0x10002000 (top of RAM)
             - Reset handler: 0x000001C5 (bit 0 set = Thumb mode)
             - Interrupt handlers...

0x00000124:  [Your Code]
             - main() function
             - All your C code compiled to ARM instructions

0x00003000:  [Constant Data]
             - String literals: "Hello"
             - const variables
```

### RAM (Volatile - Lost on Power Off)
```
Address      Content (after startup, before main)
0x10000000:  [Global Variables]
             - Initialized to values from Flash

0x10001000:  [Heap]
             - Free memory (if using malloc)

0x10001800:  [Stack]
             - Grows downward from 0x10002000
             - Function call frames
             - Local variables
```

### Peripheral Registers (Memory-Mapped)
```
Address      Peripheral
0x50000000+: GPIO ports
0x40000000+: Timers, UART, I2C, SPI, etc.
```

---

## Part 8: Troubleshooting Build Issues

### Common Build Errors

**Error: "undefined reference to `function_name`"**
- Function is declared but not defined
- Forgot to add a .c file to the Makefile
- Misspelled function name

**Error: "region FLASH overflowed"**
- Your code is too big for Flash (>32 KB)
- Need to optimize or remove features
- Try higher optimization level (`-Os` for size)

**Error: "region RAM overflowed"**
- Too many global variables or large arrays
- Reduce variable sizes or use `const` for read-only data

**Error: "multiple definition of `variable`"**
- Variable defined in .h file (should only declare with `extern`)
- Same variable defined in multiple .c files

**Warning: "implicit declaration of function"**
- Missing `#include` for the header that declares the function
- Function used before it's declared

### Common Flashing Issues

**ST-Link not detected:**
- Check USB cable (some are charge-only)
- Install ST-Link drivers
- Try different USB port

**OpenOCD can't connect:**
- Verify wiring (SWDIO, SWCLK, GND)
- Check target power
- Try slower speed: add `-c "adapter speed 100"` to OpenOCD command

**USB bootloader drive doesn't appear:**
- Hold ISP button while connecting USB
- Check if CRP (Code Read Protection) is enabled

---

## Part 9: Verification and Testing

### After Programming

1. **Visual Check:**
   - Does LED blink?
   - Expected behavior?

2. **Serial Output (if available):**
   - Connect UART to PC
   - Use terminal program (minicom, screen, PuTTY)
   - Print debug messages

3. **Debugger Check (with ST-Link + GDB):**
   ```bash
   # Terminal 1: Start OpenOCD
   openocd -f interface/stlink.cfg -f target/lpc13xx.cfg

   # Terminal 2: Start GDB
   arm-none-eabi-gdb build/project.elf
   (gdb) target remote :3333
   (gdb) monitor reset halt
   (gdb) break main
   (gdb) continue
   ```

4. **Memory Verification:**
   - In GDB: `x/10x 0x00000000` to view vector table
   - Should see stack pointer and reset handler addresses

---

## Key Takeaways

1. **Source files** (.c, .s) are human-readable code you write

2. **Build process** compiles → assembles → links → converts to create firmware

3. **Linker script** (.ld) defines where code/data goes in memory

4. **Firmware file** (.hex or .bin) is what you flash to the microcontroller

5. **Programming** transfers firmware from PC to Flash memory on the chip

6. **After reset**, the microcontroller executes your code from Flash

7. **RAM** is used for variables during execution

8. **Flash** stores your program permanently (even when powered off)

---

## Further Learning

1. **Read map file** (`.map`) to see exactly where everything is in memory
2. **Examine startup code** - See [Appendix D](appendix/D-startup-assembly-explained.md)
3. **Learn ARM Cortex-M3 assembly** to understand what compiler generates
4. **Study linker scripts** to understand advanced memory layouts
5. **Explore optimization flags** (-O0, -O1, -O2, -O3, -Os) and their effects

---

## What's Next?

You now understand how your code becomes firmware. Time to start controlling hardware directly!

**Next Chapter:** [Chapter 3: GPIO In-Depth](03-gpio-in-depth.md) - Learn to control pins, read buttons, and use GPIO interrupts.

---

*Chapter 2 of the LPC1343 Embedded C Programming Guide*
