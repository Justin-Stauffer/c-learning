# From Source Code to Firmware: Build Process Explained

A complete beginner's guide to understanding how your C code becomes firmware that runs on the LPC1343 microcontroller.

---

## Part 1: Understanding the Files in Your Project

### Source Code Files

#### **main.c** - Your Application Code
```c
void main(void) {
    init_devices();
    while(1) { }
}
```
- Human-readable C code
- Contains your program logic
- Gets compiled into machine code
- This is where YOU write your program

#### **cstartup_M.s** - Startup Code (Assembly)
```assembly
__vector_table
    DCD     sfe(CSTACK)              ; Top of Stack
    DCD     __iar_program_start      ; Reset Handler
    DCD     NMI_Handler              ; NMI Handler
    ...
```
- Written in ARM assembly language
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

### Configuration Files

#### **LPC1343_Flash.icf** - Linker Configuration File

This file tells the **linker** where to place code and data in memory.

```
Memory Layout for LPC1343:
┌─────────────────────────────┐ 0x00000000
│  Interrupt Vector Table     │ (first 292 bytes)
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

**Key sections from LPC1343_Flash.icf:**
```
ROM: 0x00000124 to 0x00007FFF  (~32 KB)  - Where your code lives
RAM: 0x10000000 to 0x10001FFF  (8 KB)    - Where variables live
Stack: 0x800 bytes (2 KB)                - For function calls
Heap: 0x400 bytes (1 KB)                 - For malloc() (if used)
```

#### **Project Files (.ewp, .ewd, .eww)**

These are specific to **IAR Embedded Workbench** IDE:
- `.ewp` = Project settings (which files to compile, compiler options)
- `.ewd` = Debugger settings (how to connect to hardware)
- `.eww` = Workspace (can contain multiple projects)

**Note:** These are IDE-specific. Other toolchains use different project files:
- GCC/Make: `Makefile`
- Keil MDK: `.uvproj`
- Eclipse: `.project` and `.cproject`

---

## Part 1.5: IAR vs GCC - Which Files to Use?

This example project includes files for **both IAR and GCC toolchains**. Use the files that match your toolchain!

### IAR Embedded Workbench Files
If you're using IAR EWARM:
- ✅ `cstartup_M.s` - IAR startup code
- ✅ `LPC1343_Flash.icf` - IAR linker script
- ✅ `*.ewp`, `*.ewd`, `*.eww` - IAR project files
- ✅ `main.c` - Your code (works with both)

### GCC ARM Toolchain Files
If you're using `arm-none-eabi-gcc`:
- ✅ `startup_lpc1343_gcc.s` - GCC startup code
- ✅ `lpc1343_flash.ld` - GCC linker script
- ✅ `Makefile` - Build automation
- ✅ `.vscode/tasks.json` - VSCode integration
- ✅ `main.c` - Your code (works with both)

### Key Differences

| Aspect | IAR | GCC |
|--------|-----|-----|
| **Startup File** | `cstartup_M.s` (IAR syntax) | `startup_lpc1343_gcc.s` (GNU syntax) |
| **Linker Script** | `LPC1343_Flash.icf` | `lpc1343_flash.ld` |
| **Project File** | `.ewp` XML | `Makefile` |
| **Build Command** | Click "Build" in IDE | `make` in terminal |
| **Assembly Syntax** | `DCD`, `SECTION`, `PUBWEAK` | `.word`, `.section`, `.weak` |
| **Intrinsics** | `__disable_interrupt()` | `__disable_irq()` |

### Why This Matters

**The main.c code is ~95% compatible** between toolchains, but:
- **Startup code is NOT compatible** - different assembly syntax
- **Linker scripts are NOT compatible** - different formats
- **Project files are NOT compatible** - different tools

**Bottom line:** If you're using GCC, use the GCC files. Don't try to mix them!

---

## Part 2: The Build Process (Compilation Pipeline)

Here's what happens when you click "Build" in your IDE:

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
cstartup_M.s
        ↓
    [Assembler]
        ↓
cstartup_M.o (object file)

Step 4: LINKING
main.o + cstartup_M.o + libraries
        ↓
    [Linker] (uses LPC1343_Flash.icf)
        ↓
program.elf (executable)
Complete program with addresses resolved

Step 5: CONVERSION
program.elf
        ↓
    [objcopy/ielftool]
        ↓
program.hex or program.bin
Ready to flash to microcontroller!
```

### Detailed Explanation of Each Step

#### Step 1: Preprocessing
```bash
# Preprocessor handles:
#include <nxp/iolpc1343.h>  → Inserts file contents
#define LED0_ON GPIO3DATA &= ~0x01;  → Text substitution
#ifdef DEBUG ... #endif      → Conditional compilation
```

**Output:** Pure C code with all macros expanded

#### Step 2: Compilation
```bash
# Compiler translates C to machine code:

C Code:
    LED0_ON;

Assembly equivalent:
    LDR  R0, =GPIO3DATA   ; Load address of GPIO3DATA
    LDR  R1, [R0]         ; Read current value
    BIC  R1, R1, #0x01    ; Clear bit 0 (bit clear)
    STR  R1, [R0]         ; Write back

Machine Code (hex):
    48 05 68 01 F0 21 01 60 01
```

**Output:** `.o` object files (machine code, but addresses not final)

#### Step 3: Linking

The **linker** combines all object files into one executable:

```
What the linker does:
1. Combines all .o files
2. Resolves function calls (connects main() to init_devices())
3. Assigns final memory addresses using .icf file
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
- Too much info for simple programmers

**HEX file** (.hex) - Intel Hex Format:
```
:10000000B0050020C9010000D70100001D020000EE
:10001000000000000000000000000000000000003C
```
- Text format (human-readable)
- Contains: address + data + checksum
- Used by most programmer tools

**BIN file** (.bin) - Raw Binary:
```
B0 05 00 20 C9 01 00 00 D7 01 00 00 1D 02 00 00
```
- Pure binary data
- Exact image of what goes in Flash
- Smallest file size
- Just bytes, no structure

---

## Part 3: Common Output File Types

| Extension | Name | Description | Usage |
|-----------|------|-------------|-------|
| `.c` | C Source | Human-readable source code | Your code |
| `.h` | Header | Function declarations, macros | #included in .c files |
| `.s` | Assembly | Low-level assembly code | Startup, critical routines |
| `.o` / `.obj` | Object | Compiled but not linked | Intermediate build output |
| `.a` / `.lib` | Library | Collection of .o files | Reusable code modules |
| `.elf` | Executable | Complete program with debug info | Debugging in IDE |
| `.hex` | Intel Hex | Text format firmware | Programming/flashing |
| `.bin` | Binary | Raw binary firmware | Programming/flashing |
| `.map` | Map File | Memory layout report | See what's where |
| `.lst` | Listing | Assembly with addresses | Debug/optimization |

---

## Part 4: How to Build Firmware (Step by Step)

### Using IAR Embedded Workbench (Your Current Setup)

1. **Open the Workspace**
   - Double-click `LPC-P1343_LEDs_Running_Light.eww`
   - IAR Embedded Workbench IDE opens

2. **Build the Project**
   - Press `F7` or click "Project → Make"
   - IDE runs all build steps automatically
   - Watch the output window for errors

3. **Build Output Files Created**
   ```
   Debug/ or Release/ folder:
   ├── LPC-P1343_LEDs_Running_Light.elf  (main executable)
   ├── LPC-P1343_LEDs_Running_Light.hex  (for programming)
   ├── LPC-P1343_LEDs_Running_Light.map  (memory map)
   ├── main.o                            (compiled main.c)
   └── cstartup_M.o                      (compiled startup)
   ```

4. **Verify Build Success**
   - Look for: `0 errors, 0 warnings`
   - Check file sizes make sense (code < 32KB, data < 8KB)

### Using GCC ARM Toolchain (arm-none-eabi-*)

If you're using the GCC ARM toolchain with VSCode:

#### Prerequisites
1. Install GCC ARM toolchain: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
2. Ensure `arm-none-eabi-gcc` is in your PATH
3. Install Make (MinGW, Cygwin, or WSL on Windows)

#### Build from Command Line

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
- Links all objects → `lpc1343_blink.elf`
- Converts to HEX → `lpc1343_blink.hex`
- Converts to BIN → `lpc1343_blink.bin`

**2. Build Output Files Created**
```
build/ folder:
├── lpc1343_blink.elf     (executable with debug info)
├── lpc1343_blink.hex     (Intel HEX for programming)
├── lpc1343_blink.bin     (raw binary for programming)
├── lpc1343_blink.map     (memory map)
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
=== Memory Usage ===
   text    data     bss     dec     hex filename
   4532     108    2048    6688    1a20 build/lpc1343_blink.elf

text: 4532 bytes (Flash) - Your code + constants
data:  108 bytes (Flash→RAM) - Initialized variables
bss:  2048 bytes (RAM) - Uninitialized variables + stack
```

**4. Clean Build Artifacts**
```bash
make clean
```

**5. Rebuild Everything**
```bash
make clean && make all
```

#### Build from VSCode

**Quick Build:**
- Press `Ctrl+Shift+B` (runs default build task)

**Other Tasks:**
Press `Ctrl+Shift+P`, type "Run Task", select:
- **Build Project** - Compile everything
- **Clean Project** - Remove build files
- **Rebuild Project** - Clean then build
- **Show Memory Usage** - Display detailed memory stats
- **Generate Disassembly** - Create assembly listing

#### Manual Build Commands (Without Makefile)

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

### Method 1: Using a Debug Probe (Recommended)

The LPC-P1343 board likely has a debug connector (JTAG or SWD).

**Hardware needed:**
- Debug probe (examples: J-Link, ST-Link, LPC-Link)
- Connects PC to microcontroller debug pins

**In IAR Embedded Workbench:**
1. Click "Project → Download and Debug" (`Ctrl+D`)
2. IDE automatically:
   - Compiles code
   - Converts to flashable format
   - Programs the microcontroller via debug probe
   - Starts debugging session

**With GCC + OpenOCD:**
```bash
make flash-openocd
```

Or manually:
```bash
openocd -f interface/cmsis-dap.cfg -f target/lpc1343.cfg \
    -c "program build/lpc1343_blink.bin verify reset exit 0x00000000"
```

Common OpenOCD interface configs:
- `interface/cmsis-dap.cfg` - Generic CMSIS-DAP probe
- `interface/jlink.cfg` - Segger J-Link
- `interface/stlink.cfg` - ST-Link (with adapters)

**How it works:**
```
PC ←→ [USB] ←→ [Debug Probe] ←→ [SWD/JTAG] ←→ [LPC1343]
```

The debug probe:
- Writes `.hex` or `.bin` data into Flash memory
- Can set breakpoints
- Can read/write RAM and registers in real-time

### Method 2: USB Bootloader (LPC1343 Specific)

The LPC1343 has a built-in USB bootloader. **Works with both IAR and GCC** - just use the `.bin` file:

1. **Enter Bootloader Mode:**
   - Hold bootloader button while plugging in USB
   - Or short ISP pin to GND while resetting
   - Board appears as USB Mass Storage device

2. **Copy Firmware:**
   - A drive appears: `CRP DISABLD` or similar
   - Delete existing `firmware.bin` (if present)
   - Drag and drop your `.bin` file to the drive
     - IAR: `Debug/LPC-P1343_LEDs_Running_Light.bin`
     - GCC: `build/lpc1343_blink.bin`
   - Board automatically programs itself (LED may blink)

3. **Reset:**
   - Eject the drive safely
   - Press reset button or re-plug USB
   - Your code runs!

**Note:** This is the easiest method and requires no additional hardware!

### Method 3: Serial Bootloader (UART)

Flash via UART using the `lpc21isp` tool. Requires UART connection to the board.

**With GCC (using Makefile):**
```bash
make flash-serial
```

**Manual command:**
```bash
# Syntax: lpc21isp -bin <file> <port> <baud> <crystal_khz>
lpc21isp -bin build/lpc1343_blink.bin COM3 115200 12000

# Linux/Mac example:
lpc21isp -bin build/lpc1343_blink.bin /dev/ttyUSB0 115200 12000
```

**Parameters:**
- `build/lpc1343_blink.bin` - Your firmware binary
- `COM3` (Windows) or `/dev/ttyUSB0` (Linux) - Serial port
- `115200` - Baud rate
- `12000` - Crystal frequency in KHz (12 MHz = 12000)

**Hardware Connection:**
```
PC UART ←→ LPC1343 UART
TX   →   RX (P1.6)
RX   ←   TX (P1.7)
GND  ←→  GND
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
    ┌──────────────────────────────────────┐
    │  Configure: .icf linker script       │
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
    │  Convert: .elf → .hex or .bin        │
    └──────────────────────────────────────┘
                         ↓
┌──────────────────────────────────────────────────────┐
│                  PROGRAMMING PHASE                    │
└──────────────────────────────────────────────────────┘
                         ↓
    ┌──────────────────────────────────────┐
    │  Connect debug probe or USB cable    │
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
    │  LED blinks! 🎉                      │
    └──────────────────────────────────────┘
```

---

## Part 7: Understanding Memory After Programming

After flashing, this is what's in the LPC1343's memory:

### Flash Memory (Non-volatile - Survives Power Off)
```
Address      Content
0x00000000:  [Vector Table]
             - Stack pointer: 0x10002000
             - Reset handler: 0x000001C5
             - Interrupt handlers...

0x00000124:  [Your Code]
             - main() function
             - init_devices() function
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
0x40000000+: Timers, UART, etc.
```

---

## Part 8: Troubleshooting Build Issues

### Common Build Errors

**Error: "undefined reference to `function_name`"**
- Function is declared but not defined
- Forgot to add a .c file to the project
- Misspelled function name

**Error: "region ROM overflowed"**
- Your code is too big for Flash (>32 KB)
- Need to optimize or remove features

**Error: "region RAM overflowed"**
- Too many global variables or large arrays
- Reduce variable sizes or use Flash for const data

**Error: "multiple definition of `variable`"**
- Variable defined in .h file (should only declare)
- Same variable defined in multiple .c files

---

## Part 9: Verification and Testing

### After Programming

1. **Visual Check:**
   - Does LED blink?
   - Expected behavior?

2. **Debugger Check:**
   - Set breakpoint in `main()`
   - Press F5 to run
   - Hit breakpoint?
   - Step through code with F10

3. **Memory Verification:**
   - View memory window at 0x00000000
   - Should see vector table
   - View memory at 0x10000000
   - Should see RAM variables

4. **Serial Output (if available):**
   - Connect UART to PC
   - Use terminal program
   - Print debug messages

---

## Part 10: IAR vs GCC - Complete Comparison

### Side-by-Side Comparison

| Feature | IAR EWARM | GCC ARM (arm-none-eabi) |
|---------|-----------|--------------------------|
| **Cost** | $3,000-6,000/license | **FREE** (open source) |
| **IDE** | Integrated (IAR EWARM) | Use any (VSCode, Eclipse, CLI) |
| **Startup File** | `cstartup_M.s` (IAR syntax) | `startup_lpc1343_gcc.s` (GNU syntax) |
| **Linker Script** | `.icf` format | `.ld` format |
| **Build System** | `.ewp` project files | `Makefile` or CMake |
| **Build Command** | F7 or click Build | `make` or `make all` |
| **Code Size** | Smaller (5-15% advantage) | Slightly larger |
| **Optimization** | Excellent | Very good |
| **C Standard** | C99/C11/C++14 | C99/C11/C++17/C++20 |
| **Support** | Commercial ($$$) | Community (free) |
| **Debugging** | Integrated debugger | GDB + OpenOCD |
| **Industry Use** | Professional/commercial | Open source + commercial |

### What's Compatible Between Toolchains

✅ **These work with BOTH:**
- `main.c` and most application code (95%+)
- Hardware register definitions
- Algorithm and logic code
- Most C standard library functions
- Memory layout concepts (Flash at 0x00000000, RAM at 0x10000000)

❌ **These are INCOMPATIBLE:**
- Startup assembly files (different syntax)
- Linker scripts (different formats)
- Project/workspace files (completely different)
- Compiler-specific intrinsics (`__disable_interrupt()` vs `__disable_irq()`)
- Assembly syntax (`DCD` vs `.word`, `SECTION` vs `.section`)

### When to Use IAR

Choose IAR if:
- Your company already has licenses
- You need commercial support
- Working on safety-critical systems (automotive, medical)
- Code size optimization is critical (every byte counts)
- You prefer integrated all-in-one IDE
- Budget is not a constraint

### When to Use GCC

Choose GCC if:
- You're learning or hobbyist (it's free!)
- Working on open source projects
- You prefer command-line tools or VSCode
- Need latest C/C++ standards
- Want complete control over build process
- Contributing to community projects

### Real-World Perspective

**Both toolchains are professional-grade and widely used:**

**GCC ARM is used in:**
- Raspberry Pi Pico SDK
- Arduino ARM boards
- Zephyr RTOS
- Most open-source ARM projects
- Many commercial products

**IAR is used in:**
- Automotive ECUs
- Medical devices
- Industrial control systems
- Aerospace applications
- Products requiring DO-178B/ISO 26262 certification

### Performance Comparison

Using this LPC1343 LED blink example:

| Metric | IAR (estimated) | GCC -O2 (estimated) | Difference |
|--------|---------|------------|-----------|
| Code Size | ~4.2 KB | ~4.5 KB | +7% |
| RAM Usage | Same | Same | None |
| Execution Speed | Slightly faster | Very close | ~2-3% |
| Compilation Time | Fast | Fast | Similar |

**Bottom Line:** GCC produces excellent code that's 5-10% larger than IAR. For a 32KB Flash chip, this rarely matters.

### Converting IAR Projects to GCC

If you have IAR code and want to use GCC:

1. **Keep as-is:**
   - `main.c` and application code
   - Header files

2. **Replace:**
   - `cstartup_M.s` → `startup_lpc1343_gcc.s`
   - `LPC1343_Flash.icf` → `lpc1343_flash.ld`
   - `.ewp` files → `Makefile`

3. **Modify (if needed):**
   ```c
   // IAR-specific                 // GCC equivalent
   __disable_interrupt()     →     __disable_irq()
   __enable_interrupt()      →     __enable_irq()
   __no_operation()          →     __asm volatile("nop")
   ```

4. **Test and verify:**
   - Build with `make`
   - Check memory usage: `make size`
   - Flash and test on hardware

### Recommendation for Beginners

**Start with GCC** because:
1. It's free - no licensing hassles
2. Large community and lots of tutorials
3. Works on all platforms (Windows, Linux, Mac)
4. Skills transfer to other projects
5. Understanding Makefiles teaches build process
6. VSCode integration is excellent

You can always learn IAR later if your job requires it. The concepts (interrupts, timers, peripherals, memory layout) are identical - only the tools differ.

---

## Key Takeaways

1. **Source files** (.c, .s) are human-readable code you write

2. **Build process** compiles → assembles → links → converts to create firmware

3. **Linker script** (.icf) defines where code/data goes in memory

4. **Firmware file** (.hex or .bin) is what you flash to the microcontroller

5. **Programming** transfers firmware from PC to Flash memory on the chip

6. **After reset**, the microcontroller executes your code from Flash

7. **RAM** is used for variables during execution

8. **Flash** stores your program permanently (even when powered off)

---

## Further Learning

1. **Read map file** (`.map`) to see exactly where everything is in memory
2. **Examine .lst file** to see assembly code generated from your C code
3. **Learn ARM Cortex-M3 assembly** to understand what compiler generates
4. **Study linker scripts** to understand advanced memory layouts
5. **Explore optimization flags** (-O0, -O1, -O2, -O3) and their effects

---

*Companion guide to the LPC-P1343 Interrupt and Clock examples*
