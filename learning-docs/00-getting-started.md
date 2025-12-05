# Chapter 0: Getting Started

Your first steps into embedded C programming with the LPC1343 microcontroller.

---

## What You'll Achieve in This Chapter

By the end of this chapter, you will:
- Understand what embedded programming is
- Have your development environment set up
- Compile and run your first program
- See an LED blink on real hardware
- Know where to go next

**Time required:** 30-60 minutes (depending on setup)

---

## Part 1: What is Embedded Programming?

### The Big Picture

When you write a program for your computer, it runs on top of an operating system (Windows, macOS, Linux) that handles all the low-level details—memory, screen, keyboard, etc.

**Embedded programming is different.** You write code that runs directly on hardware, with no operating system in between. Your code IS the entire software running on the chip.

```
Desktop Programming:                Embedded Programming:

┌─────────────────────┐            ┌─────────────────────┐
│   Your Program      │            │   Your Program      │
├─────────────────────┤            │   (that's it!)      │
│  Operating System   │            │                     │
│  (Windows/Mac/Linux)│            │                     │
├─────────────────────┤            ├─────────────────────┤
│     Hardware        │            │     Hardware        │
│  (CPU, RAM, etc.)   │            │  (LPC1343 chip)     │
└─────────────────────┘            └─────────────────────┘
```

### Why Learn This?

Embedded systems are everywhere:
- The thermostat on your wall
- Your car's engine controller
- Medical devices
- Drones and robots
- IoT devices
- Game controllers
- Washing machines

If it has a chip and isn't a full computer, it's probably embedded.

### What is the LPC1343?

The **LPC1343** is a microcontroller made by NXP. Think of it as a tiny, complete computer on a single chip:

```
┌────────────────────────────────────────┐
│            LPC1343 Chip                │
│                                        │
│  ┌──────────────┐  ┌───────────────┐  │
│  │  ARM Cortex  │  │  32 KB Flash  │  │
│  │  M3 CPU      │  │  (your code)  │  │
│  │  (72 MHz)    │  │               │  │
│  └──────────────┘  └───────────────┘  │
│                                        │
│  ┌──────────────┐  ┌───────────────┐  │
│  │   8 KB RAM   │  │  Peripherals  │  │
│  │  (variables) │  │  (GPIO, UART, │  │
│  │              │  │   Timers...)  │  │
│  └──────────────┘  └───────────────┘  │
│                                        │
│          48 pins to outside world      │
└────────────────────────────────────────┘
```

**Key specs:**
- **CPU:** ARM Cortex-M3 running at up to 72 MHz
- **Flash:** 32 KB (where your program lives)
- **RAM:** 8 KB (for variables while running)
- **GPIO:** 40+ pins you can control
- **Peripherals:** Timers, UART, SPI, I2C, USB, ADC

---

## Part 2: What You Need

### Hardware

**Required:**
1. **LPC-P1343 development board** (from Olimex or similar)
   - Has the LPC1343 chip
   - LEDs and buttons for testing
   - USB connector for programming

2. **USB cable** (Mini-USB or Micro-USB depending on board)

**Optional but recommended:**
- USB-to-Serial adapter (for UART debugging)
- Breadboard and jumper wires (for experiments)
- Extra LEDs and resistors

### Software

You need a **toolchain** to convert your C code into something the chip can run.

**GCC ARM Toolchain (Free, Open Source)**
- Works on Windows, Mac, Linux
- Command-line based (with IDE options like VS Code)
- Industry-standard tools used in professional development

See [BUILD_INSTRUCTIONS_GCC.md](BUILD_INSTRUCTIONS_GCC.md) for detailed setup and build instructions.

---

## Part 3: Setting Up Your Environment

### Step 1: Install GCC ARM Toolchain

**Windows:**
1. Download from: https://developer.arm.com/downloads/-/gnu-rm
2. Run the installer
3. **Important:** Check "Add path to environment variable"
4. Restart your terminal/command prompt

**macOS:**
```bash
# Using Homebrew
brew install --cask gcc-arm-embedded

# Or download from ARM website
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi
```

**Verify installation:**
```bash
arm-none-eabi-gcc --version
```

You should see something like:
```
arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10-2020-q4-major) 10.2.1
```

### Step 2: Install Make

**Windows:**
- Install via [MSYS2](https://www.msys2.org/), or
- Use WSL (Windows Subsystem for Linux), or
- Install [Make for Windows](http://gnuwin32.sourceforge.net/packages/make.htm)

**macOS:**
```bash
xcode-select --install  # Installs make along with other tools
```

**Linux:**
```bash
sudo apt install make  # Usually pre-installed
```

**Verify:**
```bash
make --version
```

### Step 3: Install a Code Editor

Any text editor works, but **VS Code** is recommended:
1. Download from: https://code.visualstudio.com/
2. Install the **C/C++ extension** (by Microsoft)
3. Optionally install **Cortex-Debug** extension (for debugging)

### Step 4: Get the Example Code

Navigate to the Getting-Started example:
```bash
cd /path/to/LPC-P1343_Examples/Getting-Started
```

This folder contains all the files you need:
- `main.c` - The LED blink program
- `startup_lpc1343_gcc.s` - GCC startup code
- `lpc1343_flash.ld` - Linker script
- `Makefile` - Build automation

---

## Part 4: Your First Program

### The Simplest Embedded Program

Here's the absolute minimum program that blinks an LED:

```c
// main.c - Blink an LED on LPC1343

// Register definitions (memory addresses of hardware)
#define GPIO3DIR   (*((volatile unsigned int *)0x50038000))
#define GPIO3DATA  (*((volatile unsigned int *)0x50033FFC))

// Simple delay (not accurate, but works)
void delay(volatile int count) {
    while (count--) {}
}

int main(void) {
    // Set pin P3.0 as output
    GPIO3DIR |= (1 << 0);

    // Blink forever
    while (1) {
        GPIO3DATA &= ~(1 << 0);  // LED ON (active-low)
        delay(500000);

        GPIO3DATA |= (1 << 0);   // LED OFF
        delay(500000);
    }

    return 0;  // Never reached
}
```

**What's happening here?**

1. **Register definitions:** We create pointers to specific memory addresses. Writing to these addresses controls hardware.

2. **GPIO3DIR:** Direction register. Setting bit 0 to 1 makes pin P3.0 an output.

3. **GPIO3DATA:** Data register. Setting bit 0 controls the voltage on pin P3.0.

4. **Active-low LED:** On this board, the LED is wired so that LOW (0) turns it ON. This is common in embedded systems.

5. **Infinite loop:** Embedded programs never exit. There's nowhere to exit TO!

### Understanding the Magic Numbers

```c
#define GPIO3DATA  (*((volatile unsigned int *)0x50033FFC))
```

This looks scary, but let's break it down:

```
0x50033FFC           - A memory address (in hexadecimal)
(unsigned int *)     - Treat it as a pointer to an integer
(volatile ...)       - Don't optimize away reads/writes
*(...)               - Dereference: access what's AT that address
```

The LPC1343's hardware designers decided that memory address `0x50033FFC` would control GPIO Port 3's data output. When you write to this address, you're directly controlling the pins!

### Build and Flash

**Build:**
```bash
make clean
make
```

You should see output ending with:
```
   text    data     bss     dec     hex filename
   1234      12      32    1278     4fe build/lpc1343_getting_started.elf
```

This means:
- `text`: 1234 bytes of code (goes in Flash)
- `data`: 12 bytes of initialized variables
- `bss`: 32 bytes of uninitialized variables
- Total fits easily in 32 KB!

**Flash to hardware:**

*Method 1: USB Bootloader (easiest)*
1. Hold the bootloader button on the board
2. Plug in USB cable (while holding button)
3. A drive called "CRP DISABLD" appears
4. Delete any existing file on the drive
5. Copy `build/lpc1343_getting_started.bin` to the drive
6. Safely eject the drive
7. Press reset button

*Method 2: Using ST-Link debug probe*
```bash
make flash
```

**See it work:**
The LED should now be blinking! About once per second (the exact rate depends on the delay loop).

---

## Part 5: What Just Happened?

Let's trace through what happened from source code to blinking LED:

```
┌──────────────────────────────────────────────────────────────┐
│  1. You wrote main.c                                         │
│     Human-readable C code                                    │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  2. Compiler (arm-none-eabi-gcc) converted to machine code   │
│     Created: main.o (object file)                            │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  3. Linker combined all object files                         │
│     Used: linker script to assign memory addresses           │
│     Created: lpc1343_getting_started.elf                               │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  4. Objcopy extracted raw binary                             │
│     Created: lpc1343_getting_started.bin                               │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  5. You copied .bin file to the LPC1343's Flash memory       │
│     Now the chip has your program stored permanently         │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  6. On reset, CPU starts executing from Flash                │
│     - Startup code runs first (sets up stack, etc.)          │
│     - Then calls your main() function                        │
│     - Your code runs forever in the while(1) loop            │
└──────────────────────────┬───────────────────────────────────┘
                           ↓
┌──────────────────────────────────────────────────────────────┐
│  7. LED blinks!                                              │
│     - Your code writes to GPIO3DATA register                 │
│     - Hardware changes pin voltage                           │
│     - Current flows (or not) through LED                     │
│     - You see light!                                         │
└──────────────────────────────────────────────────────────────┘
```

---

## Part 6: Experiments to Try

Now that you have a working program, try these modifications:

### Experiment 1: Change the Speed

Find the `delay(500000)` lines and change the number:
- `delay(100000)` - faster blinking
- `delay(1000000)` - slower blinking

Rebuild and reflash. Does it behave as expected?

### Experiment 2: Different LED

If your board has multiple LEDs, try P3.1, P3.2, or P3.3:

```c
// Change from pin 0 to pin 1
GPIO3DIR |= (1 << 1);  // P3.1 as output

// In the loop:
GPIO3DATA &= ~(1 << 1);  // P3.1 ON
GPIO3DATA |= (1 << 1);   // P3.1 OFF
```

### Experiment 3: Multiple LEDs

Blink two LEDs alternately:

```c
GPIO3DIR |= (1 << 0) | (1 << 1);  // P3.0 and P3.1 as outputs

while (1) {
    GPIO3DATA &= ~(1 << 0);  // LED0 ON
    GPIO3DATA |= (1 << 1);   // LED1 OFF
    delay(500000);

    GPIO3DATA |= (1 << 0);   // LED0 OFF
    GPIO3DATA &= ~(1 << 1);  // LED1 ON
    delay(500000);
}
```

### Experiment 4: Break It (On Purpose)

Try these to see what happens:
- Remove the `while(1)` loop - what happens when main() returns?
- Use a wrong memory address - does it crash?
- Make the delay 0 - can you see the LED? (Hint: it's on, just dim)

---

## Part 7: Where to Go Next

Congratulations! You've successfully:
- ✅ Set up an embedded development environment
- ✅ Written, compiled, and flashed a program
- ✅ Made hardware do something in the physical world

### The Learning Path

This is **Chapter 0**. Here's the complete book:

| Chapter | Title | What You'll Learn |
|---------|-------|-------------------|
| **0** | Getting Started | ✅ You are here |
| **1** | [Bitwise Operations](01-bitwise-operations.md) | How to manipulate individual bits (essential skill) |
| **2** | [Build Process](02-firmware-build-process.md) | How source code becomes firmware |
| **3** | [GPIO In-Depth](03-gpio-in-depth.md) | Controlling pins, reading buttons, interrupts |
| **4** | [Timers and PWM](04-timers-and-pwm.md) | Precise timing, LED dimming, servo control |
| **5** | [UART Communication](05-uart-serial-communication.md) | Serial output, debugging, talking to PC |
| **6** | [Interrupts and Clocks](06-interrupts-and-clocks.md) | Event-driven programming, system timing |

**Recommended approach:**
1. Read chapters in order for the full learning experience
2. Do the experiments and practice problems in each chapter
3. Return to chapters as reference when building your own projects

### Quick Reference

Each chapter ends with a "Quick Reference" section. These are designed to be useful when you're actively coding and need to look something up quickly.

### Practice Projects

After completing the basics (Chapters 0-3), try these projects:
1. **Traffic Light:** 3 LEDs that cycle through a pattern
2. **Binary Counter:** Display a counting number on 4 LEDs
3. **Reaction Timer:** Measure how fast you can press a button

After Timers (Chapter 4):
4. **LED Dimmer:** Control brightness with PWM
5. **Servo Sweep:** Move a servo motor back and forth
6. **Metronome:** Generate a regular beep at adjustable BPM

After UART (Chapter 5):
7. **Serial Monitor:** Print messages to your computer
8. **Command Line:** Control LEDs via typed commands
9. **Data Logger:** Record sensor values over time

---

## Troubleshooting

### "arm-none-eabi-gcc: command not found"

The toolchain isn't in your PATH.
- **Windows:** Reinstall and check "Add to PATH", or add manually
- **Mac/Linux:** Check installation location, add to `~/.bashrc` or `~/.zshrc`

### "make: command not found"

Make isn't installed. See Step 2 above.

### Build succeeds but nothing happens on the board

1. Check the USB cable (some are charge-only, no data)
2. Make sure you're copying the `.bin` file, not `.elf`
3. Try the reset button after programming
4. Check that the LED isn't burned out (try a different one)

### "CRP DISABLD" drive doesn't appear

1. Make sure you're holding the button WHILE plugging in USB
2. Try a different USB port
3. Check if the board has power (any LEDs lit?)

### LED is always on (or always off)

- Check your bit numbers (P3.0 is bit 0, P3.1 is bit 1, etc.)
- Verify active-low vs active-high for your specific board
- Make sure GPIO3DIR is setting the pin as output

---

## Summary

You've taken your first steps into embedded programming:

1. **Embedded systems** run your code directly on hardware, no OS
2. **The LPC1343** is a microcontroller with CPU, memory, and peripherals
3. **The toolchain** converts C code to machine code for the chip
4. **Memory-mapped I/O** lets you control hardware by writing to special addresses
5. **The build process** compiles, links, and converts your code to a flashable file

Most importantly: **you made an LED blink.** That might seem simple, but you just:
- Wrote code that runs at 72 million operations per second
- Controlled electrical current through a circuit
- Created physical light from software

Everything else builds on these fundamentals.

---

**Next Chapter:** [Bitwise Operations](01-bitwise-operations.md) - Learn how to manipulate individual bits, the fundamental skill for all embedded programming.

---

*Chapter 0 of the LPC1343 Embedded C Programming Guide*
