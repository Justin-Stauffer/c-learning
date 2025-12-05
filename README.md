# Embedded C Learning Series

**A comprehensive, hands-on curriculum for learning embedded C programming on the NXP LPC1343 ARM Cortex-M3 microcontroller.**

This repository contains everything you need to go from zero embedded experience to confidently programming microcontrollers: complete documentation, working code examples, and a progression of projects that build on each other.

---

## Quick Start

**New to embedded programming?** Start here:

1. Read [Chapter 0: Getting Started](learning-docs/00-getting-started.md)
2. Set up your toolchain following the instructions
3. Build and flash your first LED blink program
4. Continue through the chapters in order

**Prefer an ebook?** Download the complete guide: [Embedded-C-Learning-LPC1343.epub](ebook/Embedded-C-Learning-LPC1343.epub)

---

## What's Included

### Learning Documentation

Complete curriculum covering embedded C fundamentals:

| Chapter | Topic | What You'll Learn |
|---------|-------|-------------------|
| 0 | [Getting Started](learning-docs/00-getting-started.md) | Environment setup, first program |
| 1 | [Bitwise Operations](learning-docs/01-bitwise-operations.md) | AND, OR, XOR, shifts, register manipulation |
| 2 | [Firmware Build Process](learning-docs/02-firmware-build-process.md) | Compilation, linking, memory layout |
| 3 | [GPIO In-Depth](learning-docs/03-gpio-in-depth.md) | Pin control, buttons, interrupts |
| 4 | [Timers and PWM](learning-docs/04-timers-and-pwm.md) | Delays, PWM, motor control |
| 5 | [UART Serial Communication](learning-docs/05-uart-serial-communication.md) | Serial output, debugging, printf |
| 6 | [Interrupts and Clocks](learning-docs/06-interrupts-and-clocks.md) | ISRs, NVIC, PLL configuration |
| 7 | [ADC: Analog to Digital](learning-docs/07-adc-analog-to-digital.md) | Sensors, voltage measurement |
| 8 | [I2C Communication](learning-docs/08-i2c-communication.md) | I2C protocol, sensors, EEPROM |
| 9 | [SPI Communication](learning-docs/09-spi-communication.md) | SPI protocol, flash memory |
| 10 | [Power Management](learning-docs/10-power-management.md) | Sleep modes, low-power design |

### Appendices

| Appendix | Topic | Description |
|----------|-------|-------------|
| A | [Hardware Setup](learning-docs/appendix/A-hardware-setup-stlink-uart.md) | ST-Link and UART adapter configuration |
| B | [Breadboard Basics](learning-docs/appendix/B-breadboard-basics.md) | Wiring and prototyping fundamentals |
| C | [Recommended Hardware](learning-docs/appendix/C-recommended-hardware.md) | Sensors and components for learning |
| D | [Startup Assembly](learning-docs/appendix/D-startup-assembly-explained.md) | Understanding the startup code |
| E | [Reading Datasheets](learning-docs/appendix/E-reading-datasheets.md) | Finding register addresses in docs |

### Code Examples

The `LPC-P1343_Examples/` folder contains complete, buildable projects for each chapter:

```
LPC-P1343_Examples/
├── Getting-Started/        # LED blink (Chapter 0)
├── 01-Bitwise-Operations/  # Bit manipulation demos
├── 02-Build-Process/       # Makefile and linker examples
├── ...                     # More examples per chapter
```

Each example includes:
- `main.c` - Documented source code
- `Makefile` - Build automation
- `startup_lpc1343_gcc.s` - Startup code
- `lpc1343_flash.ld` - Linker script

---

## Hardware Requirements

### Required

- **Olimex LPC-P1343** development board (or compatible LPC1343 board)
- **USB cable** (Mini-USB)

### Recommended

- **ST-Link V2** debug probe (for debugging and faster flashing)
- **USB-to-Serial adapter** (for UART debugging, e.g., FTDI or CP2102)
- **Breadboard and jumper wires** (for experiments)
- **Basic components** (LEDs, resistors, buttons)

See [Appendix C: Recommended Hardware](learning-docs/appendix/C-recommended-hardware.md) for a complete shopping list.

---

## Software Requirements

### GCC ARM Toolchain

This project uses the free, open-source GCC ARM toolchain.

**Installation:**

```bash
# macOS (Homebrew)
brew install --cask gcc-arm-embedded

# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# Windows
# Download from: https://developer.arm.com/downloads/-/gnu-rm
```

**Verify installation:**

```bash
arm-none-eabi-gcc --version
```

### Additional Tools

- **Make** - Build automation (usually pre-installed on Mac/Linux)
- **OpenOCD** - For debugging with ST-Link (optional)

See [BUILD_INSTRUCTIONS_GCC.md](learning-docs/BUILD_INSTRUCTIONS_GCC.md) for detailed setup.

---

## Building and Flashing

### Build an Example

```bash
cd LPC-P1343_Examples/Getting-Started
make clean
make
```

This produces:
- `build/lpc1343_getting_started.elf` - Debug executable
- `build/lpc1343_getting_started.bin` - Binary for flashing

### Flash to Hardware

**Method 1: USB Bootloader (no extra hardware)**
1. Hold the bootloader button while plugging in USB
2. A drive named "CRP DISABLD" appears
3. Delete any existing file on the drive
4. Copy the `.bin` file to the drive
5. Eject and press reset

**Method 2: ST-Link (faster, enables debugging)**
```bash
make flash
```

---

## Project Structure

```
c-learning/
├── README.md                    # This file
├── ebook/
│   └── Embedded-C-Learning-LPC1343.epub  # Complete guide as ebook
├── learning-docs/
│   ├── 00-index.md              # Master index and quick reference
│   ├── 00-getting-started.md    # Chapter 0
│   ├── 01-bitwise-operations.md # Chapter 1
│   ├── ...                      # Chapters 2-10
│   ├── appendix/                # Supplementary guides
│   └── BUILD_INSTRUCTIONS_GCC.md
└── LPC-P1343_Examples/
    ├── Getting-Started/         # Example projects
    ├── 01-Bitwise-Operations/
    └── ...
```

---

## Learning Path

### For Complete Beginners

1. Start with **Chapter 0** - get your first LED blinking
2. Work through chapters **1-6** in order (core fundamentals)
3. Do the experiments at the end of each chapter
4. Build the suggested projects to reinforce learning
5. Continue to chapters **7-10** for peripheral communication

### For Experienced Developers

- Use the [Master Index](learning-docs/00-index.md) as a quick reference
- Jump directly to topics you need
- Reference the code examples as templates

---

## Target Hardware: LPC1343

| Specification | Value |
|---------------|-------|
| CPU | ARM Cortex-M3 |
| Clock Speed | Up to 72 MHz |
| Flash | 32 KB |
| RAM | 8 KB |
| GPIO | 42 pins (4 ports) |
| Timers | 2x 16-bit, 2x 32-bit |
| Communication | UART, I2C, SPI, USB |
| ADC | 10-bit, 8 channels |

---

## Contributing

Found an error? Have a suggestion? Contributions are welcome!

- Open an issue for bugs or questions
- Submit a pull request for improvements
- Share your projects built with this curriculum

---

## License

This educational material is provided for learning purposes. Code examples may be used freely in your own projects.

---

## Acknowledgments

- **NXP Semiconductors** - LPC1343 documentation and support
- **Olimex** - LPC-P1343 development board
- **ARM** - Cortex-M3 architecture and documentation
- **GNU** - GCC ARM toolchain

---

*Created by Justin Stauffer + Claude.ai*
*Last Updated: December 2025*
