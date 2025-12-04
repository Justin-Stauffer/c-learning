# Chapter 7: Hardware Setup Guide

## ST-Link Debugger and DSD TECH SH-U09C5 UART Adapter with LPC-P1343

A practical guide to connecting, configuring, and using professional debugging and serial communication tools with your LPC1343 development board.

---

## Chapter Overview

| Section | What You'll Learn |
|---------|-------------------|
| Part 1 | Understanding your hardware |
| Part 2 | ST-Link connection and setup |
| Part 3 | DSD TECH UART adapter setup |
| Part 4 | Software installation |
| Part 5 | Debugging with OpenOCD and GDB |
| Part 6 | Serial communication setup |
| Part 7 | Complete workflow examples |
| Part 8 | Troubleshooting |

**Prerequisites:** Basic understanding of the LPC1343 (Chapters 0-2 recommended)

---

## Part 1: Understanding Your Hardware

### The LPC-P1343 Development Board

The Olimex LPC-P1343 board exposes the LPC1343's debugging interface via a standard ARM JTAG/SWD connector.

```
LPC-P1343 Board Layout (simplified):

    ┌─────────────────────────────────────────────────┐
    │                                                 │
    │   [USB]     [JTAG/SWD]      [LEDs]             │
    │    ║           ║             ○ ○ ○ ○           │
    │    ║           ║             0 1 2 3           │
    │                                                 │
    │   ┌─────────────────────┐                      │
    │   │      LPC1343        │     [Button]        │
    │   │                     │        ○            │
    │   └─────────────────────┘                      │
    │                                                 │
    │   [P1.6 RXD]  [P1.7 TXD]  [GND]  [3.3V]       │
    │       ○           ○         ○      ○          │
    │                                                 │
    └─────────────────────────────────────────────────┘
```

### The ST-Link V2 Debugger

The ST-Link is a debug probe originally designed for STM32 microcontrollers, but it works with any ARM Cortex-M device including the LPC1343.

**Key Features:**
- SWD (Serial Wire Debug) interface - only needs 2 data pins
- JTAG interface - traditional 4-pin debug interface
- Powered via USB
- Works with OpenOCD, enabling GDB debugging
- Very affordable (~$10-20 for clones, ~$35 for official)

There are two common ST-Link V2 form factors:

#### ST-Link V2 Mini Clone (10-pin) - Most Common

The small USB-stick style clones typically have a 10-pin header:

```
ST-Link V2 Mini (10-pin connector):

Looking at the header with USB connector facing left:

    ┌──────────────────────────────────────────┐
    │                                          │
    │   ST-LINK V2         RST  ①②  SWCLK     │
    │   STM8 & STM32       SWIM ③④  SWDIO     │
    │                      GND  ⑤⑥  GND       │
    │                      3.3V ⑦⑧  3.3V      │
    │                      5.0V ⑨⑩  5.0V      │
    │                                 ┌──┐     │
    │   [USB Connector]               │▓▓│     │
    │                                 └──┘     │
    └──────────────────────────────────────────┘

Pinout (as labeled on case):

Pin │ Function       │ Notes
────┼────────────────┼──────────────────────────
 1  │ RST (RESET)    │ Target reset
 2  │ SWCLK          │ SWD clock
 3  │ SWIM           │ STM8 protocol (not used for LPC1343)
 4  │ SWDIO          │ SWD data (bidirectional)
 5  │ GND            │ Ground
 6  │ GND            │ Ground
 7  │ 3.3V           │ Can power target (≤100mA)
 8  │ 3.3V           │ Can power target (≤100mA)
 9  │ 5.0V           │ USB 5V output
 10 │ 5.0V           │ USB 5V output

For SWD mode with LPC1343, you need:
- SWDIO (pin 4)  → data line
- SWCLK (pin 2)  → clock line
- RST (pin 1)    → reset (recommended)
- GND (pin 5 or 6) → ground
```

#### ST-Link V2 Full Size (20-pin)

The official ST-Link and some clones have a 20-pin connector:

```
ST-Link V2 Pinout (20-pin connector):

     ┌───────────────────┐
   1 │ VCC    │    │ VCC │ 2
   3 │ TRST   │    │ GND │ 4
   5 │ TDI    │    │ GND │ 6
   7 │ TMS/SWDIO │ │ GND │ 8
   9 │ TCK/SWCLK │ │ GND │ 10
  11 │ RTCK   │    │ GND │ 12
  13 │ TDO    │    │ GND │ 14
  15 │ RESET  │    │ GND │ 16
  17 │ NC     │    │ GND │ 18
  19 │ NC     │    │ GND │ 20
     └───────────────────┘

For SWD mode:
- Pin 7: SWDIO (data)
- Pin 9: SWCLK (clock)
- Pin 15: RESET (optional but recommended)
- Pin 4/6/8/etc: GND (any ground pin)
```

### The DSD TECH SH-U09C5 USB to UART Adapter

This is a high-quality FTDI-based USB to serial adapter with selectable voltage levels.

**Key Features:**
- FTDI FT232RL chip (genuine, reliable)
- Selectable voltage: 5V, 3.3V, 2.5V, 1.8V
- 6-pin header with all signals
- TX/RX indicator LEDs
- Works on Windows, macOS, Linux

```
DSD TECH SH-U09C5 Pinout:

    ┌─────────────────────────────┐
    │  [USB Connector]            │
    │                             │
    │  Voltage Select Jumper:     │
    │  [5V] [3.3V] [2.5V] [1.8V] │
    │         ▲                   │
    │    Use 3.3V for LPC1343!   │
    │                             │
    │  6-Pin Header:              │
    │  ┌─┬─┬─┬─┬─┬─┐             │
    │  │G│C│V│T│R│R│             │
    │  │N│T│C│X│X│T│             │
    │  │D│S│C│D│D│S│             │
    │  └─┴─┴─┴─┴─┴─┘             │
    └─────────────────────────────┘

Pin definitions:
- GND: Ground
- CTS: Clear To Send (flow control, often unused)
- VCC: Voltage output (3.3V when set to 3.3V mode)
- TXD: Transmit Data (adapter sends data OUT)
- RXD: Receive Data (adapter receives data IN)
- RTS: Request To Send (flow control, often unused)
```

**IMPORTANT: Set the voltage jumper to 3.3V before connecting to the LPC1343!**
The LPC1343 is a 3.3V device. Using 5V could damage it.

---

## Part 2: ST-Link Connection and Setup

### Wiring the ST-Link to LPC-P1343

The LPC-P1343 has a 20-pin JTAG connector. You can use SWD mode which requires only 4 wires.

#### Option A: 10-Pin ST-Link Mini Clone (Most Common)

Use jumper wires from the ST-Link to the LPC-P1343 JTAG header:

```
ST-Link V2 Mini (10-pin)     LPC-P1343 JTAG Header (20-pin)
────────────────────────     ─────────────────────────────
Pin 4 (SWDIO)  ─────────────► Pin 7 (TMS/SWDIO)
Pin 2 (SWCLK)  ─────────────► Pin 9 (TCK/SWCLK)
Pin 1 (RST)    ─────────────► Pin 15 (nRESET)
Pin 5 (GND)    ─────────────► Pin 4, 6, 8, or 20 (GND)
Pin 7 (3.3V)   ─────────────► Pin 1 (VTref/3.3V)  ◄── POWER

Wiring Diagram (matching your ST-Link's labels):

    ST-Link V2 Mini               LPC-P1343 JTAG
    (as labeled on case)          (20-pin header)

    RST  ①─────────────────────────► Pin 15 (nRESET)
    SWCLK②═════════════════════════► Pin 9 (TCK/SWCLK)
    SWIM ③ (not connected)
    SWDIO④═════════════════════════► Pin 7 (TMS/SWDIO)
    GND  ⑤─────────────────────────► Pin 4 (GND)
    GND  ⑥ (optional extra ground)
    3.3V ⑦─────────────────────────► Pin 1 (VTref) ◄── POWER
    3.3V ⑧ (optional - use if more current needed)
    5.0V ⑨ (not connected)
    5.0V ⑩ (not connected)

    ═══ = Data lines (critical - SWCLK and SWDIO)
    ─── = Reset, Ground, and Power

Summary - You need 5 wires:
    ST-Link          LPC-P1343
    ────────         ─────────
    Pin 4 (SWDIO) → Pin 7  (data)
    Pin 2 (SWCLK) → Pin 9  (clock)
    Pin 1 (RST)   → Pin 15 (reset)
    Pin 5 (GND)   → Pin 4  (ground)
    Pin 7 (3.3V)  → Pin 1  (power)
```

**Powering from ST-Link:**
- The ST-Link can supply 3.3V to power the LPC-P1343
- Current limit is approximately 100-200mA (sufficient for basic operation)
- If you're driving lots of LEDs or external hardware, use external power instead
- Do NOT connect both ST-Link power AND USB power simultaneously!

**Physical Connection Tips:**
- Use female-to-female jumper wires
- The LPC-P1343 JTAG header may need male header pins soldered
- The pin numbers on your ST-Link match the diagram above

#### Verifying Your Connection

Once you've connected the wires and plugged the ST-Link into your Mac:

**Step 1: Check if macOS sees the ST-Link**
```bash
# List USB devices
system_profiler SPUSBDataType | grep -A 5 "ST-LINK"
```

You should see something like:
```
ST-LINK/V2:
  Product ID: 0x3748
  Vendor ID: 0x0483  (STMicroelectronics)
  ...
```

**Step 2: Test with OpenOCD**
```bash
# Try to connect to the target
openocd -f interface/stlink.cfg -f target/lpc13xx.cfg -c "init" -c "targets" -c "exit"
```

**If successful, you'll see:**
```
Open On-Chip Debugger 0.12.0
Info : STLINK V2J37S7 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.3V
Info : [lpc1343.cpu] Cortex-M3 r2p0 processor detected
Info : [lpc1343.cpu] target has 6 breakpoints, 4 watchpoints
    TargetName         Type       Endian TapName            State
--  ------------------ ---------- ------ ------------------ --------
 0* lpc1343.cpu        cortex_m   little lpc1343.cpu        running
```

**If it fails, check these things:**

| Error Message | Likely Cause | Fix |
|---------------|--------------|-----|
| `Error: open failed` | ST-Link not detected | Check USB connection, try different port |
| `Target voltage may be too low` | No power to LPC1343 | Check 3.3V wire (Pin 7 → Pin 1) |
| `Error connecting DP` | SWD wires wrong | Check SWDIO (Pin 4 → Pin 7) and SWCLK (Pin 2 → Pin 9) |
| `Cannot identify target` | Multiple issues | Check all wires, ensure good contact |

**Step 3: Quick LED test (if OpenOCD works)**

If OpenOCD connects, try reading the device ID:
```bash
openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
  -c "init" \
  -c "mdw 0x400483F4" \
  -c "exit"
```

This reads the LPC1343's device ID register. You should see a value like `0x2c42502b`.

#### Option B: 20-Pin ST-Link (Full Size)

```
ST-Link V2 (20-pin)      LPC-P1343 JTAG Header
───────────────────      ────────────────────
Pin 7 (SWDIO)  ─────────► Pin 7 (TMS/SWDIO)
Pin 9 (SWCLK)  ─────────► Pin 9 (TCK/SWCLK)
Pin 15 (RESET) ─────────► Pin 15 (RESET)
Pin 4 (GND)    ─────────► Pin 4 (GND)

Optional (for target power sensing):
Pin 1 (VCC)    ─────────► Pin 1 (VTref) - Don't use for power!
```

**Wiring Diagram:**

```
    ST-Link V2                         LPC-P1343
    (20-pin)                          (20-pin JTAG)

    ┌─────────┐                       ┌─────────┐
  1 │ VCC     │─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─│ VTref   │ 1
  2 │ VCC     │                       │ VCC     │ 2
  3 │ TRST    │                       │ nTRST   │ 3
  4 │ GND     │───────────────────────│ GND     │ 4
  5 │ TDI     │                       │ TDI     │ 5
  6 │ GND     │                       │ GND     │ 6
  7 │ SWDIO   │═══════════════════════│ TMS     │ 7  ◄── Data
  8 │ GND     │                       │ GND     │ 8
  9 │ SWCLK   │═══════════════════════│ TCK     │ 9  ◄── Clock
 10 │ GND     │                       │ GND     │ 10
 11 │ RTCK    │                       │ RTCK    │ 11
 12 │ GND     │                       │ GND     │ 12
 13 │ TDO     │                       │ TDO     │ 13
 14 │ GND     │                       │ GND     │ 14
 15 │ RESET   │═══════════════════════│ nRESET  │ 15 ◄── Reset
 16 │ GND     │                       │ GND     │ 16
 17 │ NC      │                       │ NC      │ 17
 18 │ GND     │                       │ GND     │ 18
 19 │ NC      │                       │ NC      │ 19
 20 │ GND     │                       │ GND     │ 20
    └─────────┘                       └─────────┘

═══ = Required connections
─ ─ = Optional (voltage reference sensing)
```

#### Option B: Using Full JTAG

If SWD doesn't work, try full JTAG:

```
ST-Link V2          LPC-P1343 JTAG Header
─────────           ────────────────────
Pin 5 (TDI)    ───► Pin 5 (TDI)
Pin 7 (TMS)    ───► Pin 7 (TMS)
Pin 9 (TCK)    ───► Pin 9 (TCK)
Pin 13 (TDO)   ───► Pin 13 (TDO)
Pin 15 (RESET) ───► Pin 15 (RESET)
Pin 4 (GND)    ───► Pin 4 (GND)
```

### Using a 20-Pin Ribbon Cable

If you have a 20-pin IDC ribbon cable, you can connect the ST-Link directly to the LPC-P1343 JTAG header:

```
┌──────────────┐     20-pin ribbon     ┌──────────────┐
│   ST-Link    │◄═══════════════════►│  LPC-P1343   │
│    V2        │     cable             │  JTAG Header │
└──────────────┘                       └──────────────┘

Note: Make sure pin 1 aligns with pin 1!
The cable should have a red stripe indicating pin 1.
```

### Power Considerations

The ST-Link can provide 3.3V power to the LPC-P1343 board through its 3.3V pins.

**Powering from ST-Link (Recommended for Simple Setups):**
- Connect ST-Link Pin 7 (3.3V) to LPC-P1343 JTAG Pin 1 (VTref)
- Current capacity: ~100-200mA (enough for LPC1343 + a few LEDs)
- Only one USB cable needed!

**When to use external power instead:**
- Driving many LEDs or high-current loads
- Using motors, relays, or other peripherals
- If you experience brownouts or erratic behavior

```
Setup A: ST-Link Powers Everything (Simplest)

┌─────────────────────────────────────────────────────────┐
│                                                         │
│  [Computer USB Port] ──► [ST-Link V2]                  │
│                               │                         │
│                         SWD + Power                     │
│                         (5 wires)                       │
│                               │                         │
│                               ▼                         │
│                         [LPC-P1343]                     │
│                     (no USB needed!)                    │
│                                                         │
└─────────────────────────────────────────────────────────┘

Setup B: Separate Power (For Higher Current Needs)

┌─────────────────────────────────────────────────────────┐
│                                                         │
│  [Computer USB Port 1] ──► [ST-Link V2]                │
│                                  │                      │
│                            SWD Only                     │
│                            (4 wires)                    │
│                                  │                      │
│                                  ▼                      │
│  [Computer USB Port 2] ──► [LPC-P1343 USB] (for power) │
│                                                         │
│  WARNING: Do NOT connect ST-Link 3.3V if using USB!    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## Part 3: DSD TECH UART Adapter Setup

### Setting the Voltage Level

**CRITICAL: Set to 3.3V before connecting!**

The DSD TECH SH-U09C5 has a jumper or switch to select voltage level:

```
Voltage Selection:

┌─────────────────────────────────────┐
│                                     │
│  Jumper positions:                  │
│                                     │
│  [5V]  [3.3V]  [2.5V]  [1.8V]      │
│          ▲                          │
│          │                          │
│    SELECT THIS for LPC1343!         │
│                                     │
└─────────────────────────────────────┘

WARNING: Using 5V with the LPC1343 can damage the chip!
The LPC1343 GPIO pins are NOT 5V tolerant (except specific I2C pins).
```

### Wiring the UART Adapter to LPC-P1343

Connect only 3 wires for basic serial communication:

```
DSD TECH SH-U09C5          LPC-P1343
─────────────────          ─────────
GND  ──────────────────►  GND
TXD  ──────────────────►  P1.6 (RXD)  ◄── Cross-over!
RXD  ◄──────────────────  P1.7 (TXD)  ◄── Cross-over!

IMPORTANT: TX connects to RX and vice versa!
This is because one device's transmit goes to the other's receive.
```

**Wiring Diagram:**

```
    DSD TECH                           LPC-P1343
    SH-U09C5                          Board Pins

    ┌─────────┐
    │   GND   │═══════════════════════► GND
    │   CTS   │ (not connected)
    │   VCC   │ (not connected - board has own power)
    │   TXD   │═══════════════════════► P1.6 (RXD)
    │   RXD   │◄═══════════════════════ P1.7 (TXD)
    │   RTS   │ (not connected)
    └─────────┘

    Remember: TXD → RXD, RXD ← TXD (cross-over)
```

### Finding the UART Pins on LPC-P1343

The UART pins on the LPC-P1343 board are typically exposed on a header or test points:

```
LPC-P1343 UART Pin Locations:

Look for pins labeled:
- RXD or P1.6 - This is the receive pin
- TXD or P1.7 - This is the transmit pin
- GND - Any ground point

If using the EXT connector:
┌─────────────────────────────────────┐
│  EXT Header (check your board)     │
│                                     │
│  ○ P1.6/RXD                        │
│  ○ P1.7/TXD                        │
│  ○ GND                             │
│  ○ 3.3V                            │
│                                     │
└─────────────────────────────────────┘
```

### Complete Hardware Setup

Here's the full setup with both ST-Link and UART connected:

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│                        Your Computer                            │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                                                         │   │
│  │   USB Port 1     USB Port 2      USB Port 3            │   │
│  │       │              │               │                  │   │
│  └───────┼──────────────┼───────────────┼──────────────────┘   │
│          │              │               │                       │
│          ▼              ▼               ▼                       │
│    ┌──────────┐   ┌──────────┐   ┌──────────────┐              │
│    │ ST-Link  │   │LPC-P1343 │   │  DSD TECH    │              │
│    │   V2     │   │   USB    │   │  SH-U09C5    │              │
│    └────┬─────┘   │ (Power)  │   └──────┬───────┘              │
│         │         └────┬─────┘          │                       │
│         │              │                │                       │
│    SWD Cable           │           UART Wires                   │
│    (4 wires)           │           (3 wires)                    │
│         │              │                │                       │
│         ▼              ▼                ▼                       │
│    ┌─────────────────────────────────────────┐                 │
│    │              LPC-P1343                   │                 │
│    │                                          │                 │
│    │  [JTAG]◄──SWD    [USB]      [UART]◄──── │                 │
│    │                  Power      P1.6, P1.7   │                 │
│    │                             GND          │                 │
│    │            [LEDs]  [Button]              │                 │
│    │                                          │                 │
│    └─────────────────────────────────────────┘                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Part 4: Software Installation

### Installing OpenOCD

OpenOCD (Open On-Chip Debugger) communicates with the ST-Link and provides a GDB server.

**macOS:**
```bash
brew install openocd
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install openocd
```

**Windows:**
1. Download from: https://github.com/openocd-org/openocd/releases
2. Extract to `C:\openocd`
3. Add `C:\openocd\bin` to your PATH

**Verify installation:**
```bash
openocd --version
```

### Installing GDB (ARM version)

GDB is the debugger that connects to OpenOCD.

**macOS:**
```bash
brew install arm-none-eabi-gdb
# Or if using the full toolchain:
brew install --cask gcc-arm-embedded
```

**Linux:**
```bash
sudo apt install gdb-multiarch
# Or:
sudo apt install gdb-arm-none-eabi
```

**Windows:**
Included with ARM GCC toolchain from ARM's website.

**Verify installation:**
```bash
arm-none-eabi-gdb --version
# Or on Linux:
gdb-multiarch --version
```

### Installing Serial Terminal Software

You need a terminal program to communicate over UART.

**macOS:**
```bash
# Screen (built-in)
screen /dev/tty.usbserial-* 115200

# Or install minicom
brew install minicom
```

**Linux:**
```bash
# Screen (usually pre-installed)
screen /dev/ttyUSB0 115200

# Or minicom
sudo apt install minicom
minicom -D /dev/ttyUSB0 -b 115200

# Or picocom
sudo apt install picocom
picocom -b 115200 /dev/ttyUSB0
```

**Windows:**
- **PuTTY** (free): https://www.putty.org/
- **Tera Term** (free): https://ttssh2.osdn.jp/
- **RealTerm** (free): https://realterm.sourceforge.io/

### Installing FTDI Drivers (for DSD TECH)

The DSD TECH SH-U09C5 uses an FTDI FT232RL chip.

**macOS:**
- Usually works automatically with built-in drivers
- If not, download from: https://ftdichip.com/drivers/vcp-drivers/

**Linux:**
- Built into the kernel, should work automatically
- Check with: `dmesg | grep -i ftdi` after plugging in

**Windows:**
- Download VCP drivers from: https://ftdichip.com/drivers/vcp-drivers/
- Install and reboot

**Verify the adapter is detected:**

```bash
# macOS
ls /dev/tty.usbserial-*

# Linux
ls /dev/ttyUSB*

# Windows
# Check Device Manager → Ports (COM & LPT)
# Look for "USB Serial Port (COM#)"
```

---

## Part 5: Debugging with OpenOCD and GDB

### OpenOCD Configuration

Create an OpenOCD configuration file for the LPC1343 with ST-Link:

**File: `openocd_lpc1343.cfg`**

```tcl
# OpenOCD configuration for LPC1343 with ST-Link V2

# Interface: ST-Link V2
source [find interface/stlink.cfg]

# Transport: SWD (Serial Wire Debug)
transport select hla_swd

# Target: LPC1343 (Cortex-M3)
set CHIPNAME lpc1343
set WORKAREASIZE 0x2000

source [find target/lpc13xx.cfg]

# Adapter speed (start slow, can increase later)
adapter speed 1000

# Reset configuration
reset_config srst_only srst_nogate
```

Save this file in your project directory.

### Starting OpenOCD

```bash
# From your project directory
openocd -f openocd_lpc1343.cfg
```

**Successful output looks like:**

```
Open On-Chip Debugger 0.12.0
Licensed under GNU GPL v2
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : clock speed 1000 kHz
Info : STLINK V2J37S7 (API v2) VID:PID 0483:3748
Info : Target voltage: 3.3V
Info : [lpc1343.cpu] Cortex-M3 r2p0 processor detected
Info : [lpc1343.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for lpc1343.cpu on 3333
Info : Listening on port 3333 for gdb connections
```

**Keep this terminal open** - OpenOCD needs to run while debugging.

### Connecting GDB

In a **new terminal**:

```bash
# Start GDB with your ELF file
arm-none-eabi-gdb build/lpc1343_getting_started.elf

# Or on Linux with gdb-multiarch:
gdb-multiarch build/lpc1343_getting_started.elf
```

**Inside GDB:**

```gdb
# Connect to OpenOCD
(gdb) target remote localhost:3333

# Reset and halt the target
(gdb) monitor reset halt

# Flash the program
(gdb) load

# Set a breakpoint at main
(gdb) break main

# Run to the breakpoint
(gdb) continue

# Now you're debugging!
```

### Essential GDB Commands

```gdb
# Execution Control
continue (c)          # Run until breakpoint or interrupt
step (s)              # Step one line (into functions)
next (n)              # Step one line (over functions)
finish                # Run until current function returns
Ctrl+C                # Stop execution

# Breakpoints
break main            # Break at function
break main.c:42       # Break at file:line
break *0x00000100     # Break at address
info breakpoints      # List breakpoints
delete 1              # Delete breakpoint #1
disable 2             # Disable breakpoint #2

# Inspection
print variable        # Print variable value
print/x variable      # Print in hexadecimal
print *pointer        # Dereference pointer
info registers        # Show CPU registers
x/10x 0x50030000      # Examine 10 words at address (hex)
x/10i $pc             # Disassemble 10 instructions at PC

# Memory/Registers
set $r0 = 5           # Set register value
set variable = 10     # Set variable value

# Stack
backtrace (bt)        # Show call stack
frame 2               # Select stack frame #2
info locals           # Show local variables

# Target Control (via OpenOCD)
monitor reset halt    # Reset and halt target
monitor reset run     # Reset and run target
monitor halt          # Halt target
monitor reg           # Show registers (OpenOCD format)

# Quitting
quit (q)              # Exit GDB
```

### Flashing Without Debugging

To just flash your code without entering the debugger:

```bash
# Method 1: Using OpenOCD directly
openocd -f openocd_lpc1343.cfg \
  -c "program build/lpc1343_getting_started.elf verify reset exit"

# Method 2: Using GDB script
arm-none-eabi-gdb -batch \
  -ex "target remote localhost:3333" \
  -ex "monitor reset halt" \
  -ex "load" \
  -ex "monitor reset run" \
  -ex "quit" \
  build/lpc1343_getting_started.elf
```

### Adding to Makefile

Add these targets to your Makefile:

```makefile
# OpenOCD configuration file
OPENOCD_CFG = openocd_lpc1343.cfg

# Start OpenOCD server
openocd:
	openocd -f $(OPENOCD_CFG)

# Flash using OpenOCD
flash: $(BUILD_DIR)/$(PROJECT).elf
	openocd -f $(OPENOCD_CFG) \
		-c "program $< verify reset exit"

# Debug: start GDB and connect to OpenOCD
debug: $(BUILD_DIR)/$(PROJECT).elf
	arm-none-eabi-gdb $< \
		-ex "target remote localhost:3333" \
		-ex "monitor reset halt" \
		-ex "load"

# Reset the target
reset:
	openocd -f $(OPENOCD_CFG) \
		-c "init" -c "reset run" -c "exit"
```

**Usage:**
```bash
# Terminal 1: Start OpenOCD (keep running)
make openocd

# Terminal 2: Flash and debug
make flash    # Just flash
make debug    # Flash and start GDB
make reset    # Reset the board
```

---

## Part 6: Serial Communication Setup

### Configuring UART in Your Code

Make sure your LPC1343 code initializes UART correctly:

```c
// UART initialization for 115200 baud at 72MHz
void uart_init(void) {
    // Enable UART clock
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);

    // Configure pins for UART function
    LPC_IOCON->PIO1_6 = 0x01;  // P1.6 = RXD
    LPC_IOCON->PIO1_7 = 0x01;  // P1.7 = TXD

    // UART clock divider
    LPC_SYSCON->UARTCLKDIV = 1;

    // Configure baud rate: 72MHz / (16 * 115200) = 39.06 ≈ 39
    LPC_UART->LCR = 0x83;      // DLAB=1, 8N1
    LPC_UART->DLL = 39;        // Divisor low byte
    LPC_UART->DLM = 0;         // Divisor high byte
    LPC_UART->LCR = 0x03;      // DLAB=0, 8N1

    // Enable FIFOs
    LPC_UART->FCR = 0x07;
}

void uart_putchar(char c) {
    while (!(LPC_UART->LSR & 0x20));  // Wait for THR empty
    LPC_UART->THR = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putchar(*s++);
}
```

### Finding Your Serial Port

**macOS:**
```bash
# List serial ports
ls /dev/tty.usbserial-*

# Example output: /dev/tty.usbserial-A50285BI
```

**Linux:**
```bash
# List serial ports
ls /dev/ttyUSB*

# Or check dmesg after plugging in
dmesg | tail -20

# Example output: /dev/ttyUSB0
```

**Windows:**
1. Open Device Manager
2. Expand "Ports (COM & LPT)"
3. Find "USB Serial Port (COM#)"
4. Note the COM number (e.g., COM3)

### Using Screen (macOS/Linux)

```bash
# Connect to serial port
screen /dev/tty.usbserial-A50285BI 115200

# Or on Linux
screen /dev/ttyUSB0 115200

# To exit screen: Ctrl+A, then K, then Y
```

### Using Minicom (macOS/Linux)

```bash
# First-time setup
minicom -s

# In the menu:
# 1. Select "Serial port setup"
# 2. Set Serial Device to your port (e.g., /dev/ttyUSB0)
# 3. Set Baud rate to 115200
# 4. Set Hardware Flow Control to "No"
# 5. Save as default
# 6. Exit

# Normal usage
minicom -D /dev/ttyUSB0 -b 115200

# Exit minicom: Ctrl+A, then X
```

### Using PuTTY (Windows)

1. Open PuTTY
2. Select "Serial" as connection type
3. Enter COM port (e.g., COM3)
4. Enter speed: 115200
5. Click "Open"

```
PuTTY Configuration:
┌─────────────────────────────────────┐
│ Connection type: ○ Serial           │
│                                     │
│ Serial line: COM3                   │
│ Speed: 115200                       │
│                                     │
│ [Open]                              │
└─────────────────────────────────────┘
```

### Testing Serial Communication

1. Flash your program with UART code
2. Connect the DSD TECH adapter
3. Open your terminal program
4. Reset the LPC1343
5. You should see output!

**Test Program:**

```c
#include "LPC13xx.h"

void uart_init(void);
void uart_puts(const char *s);
void delay(volatile int count);

int main(void) {
    uart_init();

    uart_puts("\r\n\r\n");
    uart_puts("=========================\r\n");
    uart_puts("  LPC1343 Serial Test    \r\n");
    uart_puts("  DSD TECH SH-U09C5      \r\n");
    uart_puts("=========================\r\n");

    int counter = 0;
    while (1) {
        uart_puts("Hello from LPC1343! Count: ");
        // Print counter (simple decimal print)
        char buf[12];
        int i = 0;
        int n = counter;
        if (n == 0) buf[i++] = '0';
        else {
            char tmp[12];
            int j = 0;
            while (n > 0) {
                tmp[j++] = '0' + (n % 10);
                n /= 10;
            }
            while (j > 0) buf[i++] = tmp[--j];
        }
        buf[i] = '\0';
        uart_puts(buf);
        uart_puts("\r\n");

        counter++;
        delay(1000000);
    }

    return 0;
}
```

---

## Part 7: Complete Workflow Examples

### Workflow 1: Build, Flash, and Debug

```bash
# Step 1: Build your project
make clean
make all

# Step 2: Start OpenOCD (in terminal 1)
openocd -f openocd_lpc1343.cfg

# Step 3: Flash and debug (in terminal 2)
arm-none-eabi-gdb build/lpc1343_getting_started.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
# Now step through your code!
```

### Workflow 2: Quick Flash and Run

```bash
# Build and flash in one command
make clean && make all && make flash
```

### Workflow 3: Development with Serial Output

```bash
# Terminal 1: OpenOCD (keep running for flashing)
openocd -f openocd_lpc1343.cfg

# Terminal 2: Serial monitor
screen /dev/tty.usbserial-A50285BI 115200

# Terminal 3: Build and flash
make all && make flash

# Watch serial output in Terminal 2
# Edit code, rebuild, reflash as needed
```

### Workflow 4: VS Code Integration

Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug LPC1343",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/lpc1343_getting_started.elf",
            "configFiles": [
                "${workspaceFolder}/openocd_lpc1343.cfg"
            ],
            "svdFile": "${workspaceFolder}/LPC13xx.svd",
            "runToEntryPoint": "main",
            "showDevDebugOutput": "parsed"
        }
    ]
}
```

Install the **Cortex-Debug** extension, then press F5 to start debugging!

---

## Part 8: Troubleshooting

### ST-Link Issues

#### "Error: open failed" or "STLINK not found"

```
Error: open failed
```

**Solutions:**
1. Check USB connection
2. Try a different USB port
3. Install/reinstall ST-Link drivers

**Linux permissions fix:**
```bash
# Create udev rule for ST-Link
sudo nano /etc/udev/rules.d/99-stlink.rules

# Add this line:
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"

# Reload rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Unplug and replug ST-Link
```

#### "Target voltage may be too low"

```
Error: target voltage may be too low for reliable debugging
```

**Solutions:**
1. Make sure LPC-P1343 is powered (USB connected)
2. Check all ground connections
3. Verify 3.3V is present on the board

#### "Cannot identify target"

```
Error: Cannot identify target as a Cortex-M device
```

**Solutions:**
1. Check SWD wiring (SWDIO, SWCLK, GND)
2. Try slower adapter speed: `adapter speed 100`
3. Check that board is powered
4. Try adding reset wire
5. Make sure chip isn't in deep sleep or locked

#### Clone ST-Link Issues

Some clone ST-Links have compatibility issues:

```bash
# Try specifying the interface explicitly
openocd -f interface/stlink-v2.cfg -f target/lpc13xx.cfg

# Or try different transport
transport select swd
```

### UART Issues

#### No Output on Serial Terminal

**Check:**
1. ✓ Voltage jumper set to 3.3V (not 5V!)
2. ✓ TX/RX crossed correctly (TXD→RXD, RXD→TXD)
3. ✓ Ground connected
4. ✓ Correct COM port / device selected
5. ✓ Baud rate matches (115200)
6. ✓ UART initialized in code before sending
7. ✓ Correct pins configured in IOCON

**Debug steps:**
```bash
# Check if adapter is detected
# macOS
ls /dev/tty.usbserial-*

# Linux
ls /dev/ttyUSB*
dmesg | grep -i ftdi

# Windows
# Device Manager → Ports
```

#### Garbage Characters

```
Output: ÿÿÿÿÿÿ or äääääää
```

**Cause:** Baud rate mismatch

**Solution:** Verify both ends use same baud rate:
- Check your code's baud rate divisor calculation
- Check terminal settings

**Common baud rates at 72MHz:**
| Baud | Divisor (DLL) | Error |
|------|---------------|-------|
| 9600 | 469 (0x1D5) | 0.08% |
| 115200 | 39 (0x27) | 1.6% |

#### Characters Missing or Corrupted

**Possible causes:**
1. Flow control mismatch - disable hardware flow control
2. Buffer overflow - slow down transmission
3. Noise on lines - use shorter wires

### General Debug Tips

#### LED Debugging

When all else fails, use LEDs to debug:

```c
// Toggle LED to confirm code is running
LPC_GPIO3->DIR |= (1 << 0);

// Blink pattern to show progress
LPC_GPIO3->DATA &= ~(1 << 0);  // LED ON = reached point A
delay(100000);
LPC_GPIO3->DATA |= (1 << 0);   // LED OFF

// Different blink patterns for different states
// 1 blink = init complete
// 2 blinks = waiting for input
// 3 blinks = error
```

#### Verifying Connections

```bash
# Test ST-Link connection without flashing
openocd -f openocd_lpc1343.cfg -c "init" -c "targets" -c "exit"

# Should show:
#    TargetName         Type       Endian TapName            State
# -- ------------------ ---------- ------ ------------------ --------
#  0  lpc1343.cpu        cortex_m   little lpc1343.cpu        running
```

#### Reading Device ID

```bash
# Via OpenOCD telnet (while OpenOCD is running)
telnet localhost 4444
> mdw 0x400483F4  # Read device ID register

# LPC1343 should return: 0x2c42502b or similar
```

---

## Quick Reference Card

### Pin Connections

```
ST-Link V2 Mini (10-pin) → LPC-P1343 (SWD + Power)
──────────────────────────────────────────────────
Pin 4 (SWDIO)  → Pin 7 (TMS/SWDIO)
Pin 2 (SWCLK)  → Pin 9 (TCK/SWCLK)
Pin 1 (RST)    → Pin 15 (nRESET)
Pin 5 (GND)    → Pin 4 (GND)
Pin 7 (3.3V)   → Pin 1 (VTref)  ◄── POWER

ST-Link V2 Full (20-pin) → LPC-P1343 (SWD)
──────────────────────────────────────────
Pin 7 (SWDIO)  → Pin 7 (TMS)
Pin 9 (SWCLK)  → Pin 9 (TCK)
Pin 15 (RESET) → Pin 15 (nRESET)
Pin 4 (GND)    → Pin 4 (GND)
Pin 1 (VCC)    → Pin 1 (VTref)  ◄── POWER (optional)

DSD TECH SH-U09C5 → LPC-P1343 (UART)
────────────────────────────────────
GND → GND
TXD → P1.6 (RXD)
RXD → P1.7 (TXD)
Voltage: SET TO 3.3V!
```

### Common Commands

```bash
# Build
make clean && make all

# Flash
openocd -f openocd_lpc1343.cfg -c "program build/lpc1343_getting_started.elf verify reset exit"

# Debug
arm-none-eabi-gdb build/lpc1343_getting_started.elf -ex "target remote :3333"

# Serial (macOS)
screen /dev/tty.usbserial-* 115200

# Serial (Linux)
screen /dev/ttyUSB0 115200
```

### GDB Quick Commands

```
target remote :3333   - Connect to OpenOCD
monitor reset halt    - Reset and stop
load                  - Flash program
break main            - Set breakpoint
continue (c)          - Run
step (s)              - Step into
next (n)              - Step over
print var             - Show variable
info registers        - Show registers
quit                  - Exit
```

---

## What's Next?

With your debugging and serial setup working, you can:

1. **Step through code** to understand execution flow
2. **Inspect variables** and registers in real-time
3. **Use printf-style debugging** over UART
4. **Set breakpoints** to catch bugs
5. **Profile performance** using timer measurements

**Suggested exercises:**
1. Set a breakpoint in your LED blink loop and step through it
2. Watch a counter variable increment in real-time
3. Send sensor data over UART and plot it on your PC
4. Create a simple command-line interface over serial

---

**Return to:** [Index](00-index.md) | [Chapter 5: UART Communication](05-uart-serial-communication.md) | [Chapter 6: Interrupts and Clocks](06-interrupts-and-clocks.md)

---

*Chapter 7 of the Embedded C Learning Series*
*Hardware Setup Guide for LPC-P1343 with ST-Link and DSD TECH UART Adapter*
