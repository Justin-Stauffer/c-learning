# Appendix E: Reading Datasheets and User Manuals

How to find register addresses, bit definitions, and peripheral information in manufacturer documentation.

---

## Why This Matters

When you see code like this:

```c
#define SYSAHBCLKCTRL (*((volatile unsigned int *)0x40048080))
```

You might wonder: "Where did `0x40048080` come from?"

The answer is always: **the manufacturer's documentation**. Learning to read these documents is an essential embedded programming skill.

---

## The Two Key Documents

For the LPC1343, NXP provides two main documents:

### 1. User Manual (UM10375)

**Download:** https://www.nxp.com/docs/en/user-guide/UM10375.pdf

This is your primary reference. It contains:
- Complete register descriptions
- Bit field definitions
- Peripheral operation details
- Code examples
- Timing diagrams

**Size:** ~400 pages

### 2. Datasheet (LPC1311/13/42/43)

**Download:** https://www.nxp.com/docs/en/data-sheet/LPC1311_13_42_43.pdf

This covers:
- Electrical specifications (voltage, current limits)
- Pin descriptions and alternate functions
- Package dimensions
- Absolute maximum ratings
- DC/AC characteristics

**Size:** ~80 pages

### When to Use Which

| Need to know... | Use |
|-----------------|-----|
| Register addresses and bits | User Manual |
| How a peripheral works | User Manual |
| Maximum current per pin | Datasheet |
| Pin alternate functions | Datasheet (table), User Manual (details) |
| Operating voltage range | Datasheet |
| Timing requirements | Both (Datasheet for specs, UM for registers) |

---

## User Manual Organization

The LPC1343 User Manual (UM10375) is organized by peripheral:

| Chapter | Topic | Key Information |
|---------|-------|-----------------|
| 1 | Introduction | Memory map, block diagram |
| 2 | LPC1343 Features | Overview of all peripherals |
| 3 | System Configuration | Clocks, power, reset, IOCON |
| 4 | Nested Vectored Interrupt Controller | Interrupts, priorities |
| 5 | System Tick Timer | SysTick for OS/delays |
| 7 | I/O Configuration | Pin function selection (IOCON) |
| 9 | GPIO | General purpose I/O |
| 10 | USB | USB device controller |
| 12 | 16-bit Timers | CT16B0, CT16B1 |
| 13 | 32-bit Timers | CT32B0, CT32B1 |
| 14 | UART | Serial communication |
| 17 | ADC | Analog-to-digital converter |
| 18 | Flash Programming | In-system programming |

*Note: Chapter numbers may vary slightly between manual revisions.*

---

## Finding a Register Address

Every register address is calculated as: **Base Address + Offset**

### Step 1: Find the Peripheral Base Address

Chapter 1 of the User Manual contains the **Memory Map**. Here are the key base addresses:

```
Peripheral              Base Address
─────────────────────────────────────
Flash                   0x00000000
SRAM                    0x10000000
GPIO Port 0             0x50000000
GPIO Port 1             0x50010000
GPIO Port 2             0x50020000
GPIO Port 3             0x50030000
System Control          0x40048000
IOCON                   0x40044000
UART                    0x40008000
I2C                     0x40000000
SSP0 (SPI)              0x40040000
SSP1 (SPI)              0x40058000
Timer CT16B0            0x4000C000
Timer CT16B1            0x40010000
Timer CT32B0            0x40014000
Timer CT32B1            0x40018000
ADC                     0x4001C000
USB                     0x40020000
```

### Step 2: Find the Register Offset

Each peripheral chapter has a **Register Overview** table. For example, in Chapter 3 (System Configuration), you'll find:

```
Register         Offset    Description
────────────────────────────────────────────────
SYSMEMREMAP      0x000     System memory remap
PRESETCTRL       0x004     Peripheral reset control
SYSPLLCTRL       0x008     System PLL control
SYSPLLSTAT       0x00C     System PLL status
...
SYSAHBCLKCTRL    0x080     AHB clock control
...
```

### Step 3: Calculate the Address

```
SYSAHBCLKCTRL = System Control Base + Offset
              = 0x40048000 + 0x080
              = 0x40048080
```

That's where `0x40048080` comes from!

---

## Reading Register Descriptions

Each register has a detailed description. Here's how to read it:

### Example: SYSAHBCLKCTRL (Section 3.5.14)

The manual shows:

```
Table 21. System AHB clock control register (SYSAHBCLKCTRL, address 0x4004 8080) bit description

Bit    Symbol      Value   Description                     Reset Value
─────────────────────────────────────────────────────────────────────────
0      SYS         1       Enables clock for AHB, APB      1
                           bridges, processor, etc.
                   0       Disabled
1      ROM         1       Enables clock for ROM           1
                   0       Disabled
2      RAM         1       Enables clock for RAM           1
                   0       Disabled
...
6      GPIO        1       Enables clock for GPIO          0
                   0       Disabled
...
```

### Key Information to Extract

1. **Bit Number** - Which bit to set/clear (used in `(1 << bit)`)
2. **Symbol** - Human-readable name for the bit
3. **Values** - What 0 and 1 mean
4. **Reset Value** - Default state after power-on

### Translating to Code

From the table above:

```c
// Enable GPIO clock (bit 6)
SYSAHBCLKCTRL |= (1 << 6);

// Or with a named constant:
#define SYSAHBCLKCTRL_GPIO  (1 << 6)
SYSAHBCLKCTRL |= SYSAHBCLKCTRL_GPIO;
```

---

## Understanding Reset Values

The **Reset Value** column tells you the register's state after power-on or reset.

### Example: SYSAHBCLKCTRL Reset Value = 0x085

Converting to binary:
```
0x085 = 0000 0000 1000 0101
              │       │  │
              │       │  └─ Bit 0 (SYS) = 1
              │       └──── Bit 2 (RAM) = 1
              └──────────── Bit 7 (CT16B0) = 1
```

This means after reset:
- System clock is enabled (required)
- RAM clock is enabled (required)
- CT16B0 clock is enabled (for some reason)
- GPIO clock is **disabled** (you must enable it!)

---

## Common Register Patterns

### Read-Only Registers

Some registers (or bits) are read-only. The manual indicates this with:
- "RO" (Read Only) in the access column
- "Read-only" in the description

```c
// Status registers are often read-only
uint32_t status = SOMESTATUS_REG;  // Read is OK
SOMESTATUS_REG = 0;                 // Write has no effect
```

### Write-Only Registers

Some registers are write-only:
- "WO" (Write Only) in the access column
- Reading returns undefined value

### Set/Clear Registers

Some peripherals have separate SET and CLR registers:

```c
// GPIO example - three ways to do the same thing:

// Method 1: Read-modify-write (standard)
GPIO3DATA |= (1 << 0);    // Set bit 0
GPIO3DATA &= ~(1 << 0);   // Clear bit 0

// Method 2: Masked access (LPC1343 feature)
// Write to DATA register with mask in address bits
*((volatile uint32_t *)0x50033FFC) = value;  // All 12 bits

// Some MCUs have dedicated SET/CLR registers (LPC1343 GPIO doesn't)
```

### Reserved Bits

Many registers have **Reserved** bits. The rules:
- **Read:** Value is undefined (ignore it)
- **Write:** Write the reset value (usually 0)

```c
// DON'T set reserved bits
SOME_REG = 0xFFFFFFFF;  // Bad! Sets reserved bits

// DO preserve reserved bits with read-modify-write
SOME_REG |= (1 << 5);   // Good! Only changes bit 5
```

---

## GPIO Data Register: A Special Case

The LPC1343 GPIO DATA register uses a clever trick. The address bits select which pins to affect:

```
GPIO3DATA base address: 0x50030000
GPIO3DATA full access:  0x50033FFC

The address encodes a 12-bit mask:
0x50030000 + (mask << 2)

0x50033FFC = 0x50030000 + (0xFFF << 2)
           = access all 12 pins of Port 3
```

This allows atomic read-modify-write of specific pins:

```c
// Write only to pin 0 (mask = 0x001)
*((volatile uint32_t *)0x50030004) = 0x001;  // Sets P3.0

// Write to pins 0-3 (mask = 0x00F)
*((volatile uint32_t *)0x5003003C) = 0x005;  // Sets P3.0, P3.2
```

This is documented in Chapter 9, Section 9.4 of the User Manual.

---

## IOCON: Pin Configuration

Every pin can have multiple functions. IOCON registers select the function:

### Finding Pin Functions

The datasheet has a **Pin Description** table showing alternate functions:

```
Pin     Function 0    Function 1    Function 2    Function 3
──────────────────────────────────────────────────────────────
PIO1_6  PIO1_6        RXD           CT32B0_MAT0   -
PIO1_7  PIO1_7        TXD           CT32B0_MAT1   -
```

### IOCON Register Format

Each pin has its own IOCON register. The User Manual Chapter 7 shows:

```
Bits    Name    Description
─────────────────────────────────────
2:0     FUNC    Pin function select
                000 = Function 0 (GPIO)
                001 = Function 1 (e.g., UART)
                010 = Function 2 (e.g., Timer)
4:3     MODE    Pull-up/pull-down
                00 = Inactive (no pull)
                01 = Pull-down
                10 = Pull-up
                11 = Repeater mode
7       ADMODE  Analog/digital mode (ADC pins only)
                0 = Analog input
                1 = Digital mode
```

### Example: Configure UART Pins

```c
// PIO1_6 as RXD (Function 1)
// Address from UM10375 Table 79: 0x400440A4
*((volatile uint32_t *)0x400440A4) = 0x01;

// PIO1_7 as TXD (Function 1)
// Address from UM10375 Table 79: 0x400440A8
*((volatile uint32_t *)0x400440A8) = 0x01;
```

---

## Quick Reference: Common Registers

### System Control (Base: 0x40048000)

| Register | Offset | Address | Purpose |
|----------|--------|---------|---------|
| SYSMEMREMAP | 0x000 | 0x40048000 | Memory remapping |
| PRESETCTRL | 0x004 | 0x40048004 | Peripheral reset |
| SYSPLLCTRL | 0x008 | 0x40048008 | PLL configuration |
| SYSPLLSTAT | 0x00C | 0x4004800C | PLL lock status |
| SYSAHBCLKCTRL | 0x080 | 0x40048080 | Peripheral clock enable |
| PDRUNCFG | 0x238 | 0x40048238 | Power control |

### GPIO (Base: 0x5003X000 where X = port number)

| Register | Offset | Purpose |
|----------|--------|---------|
| DATA | 0x3FFC | Read/write pin values |
| DIR | 0x8000 | Direction (0=input, 1=output) |
| IS | 0x8004 | Interrupt sense |
| IEV | 0x800C | Interrupt event |
| IE | 0x8010 | Interrupt enable |

Example for GPIO Port 3:
- GPIO3DATA: 0x50033FFC
- GPIO3DIR: 0x50038000

### UART (Base: 0x40008000)

| Register | Offset | Address | Purpose |
|----------|--------|---------|---------|
| RBR/THR/DLL | 0x000 | 0x40008000 | Receive/Transmit/Divisor Low |
| DLM/IER | 0x004 | 0x40008004 | Divisor High/Interrupt Enable |
| FCR/IIR | 0x008 | 0x40008008 | FIFO Control/Interrupt ID |
| LCR | 0x00C | 0x4000800C | Line Control |
| LSR | 0x014 | 0x40008014 | Line Status |

### Timers (Example: CT32B0, Base: 0x40014000)

| Register | Offset | Address | Purpose |
|----------|--------|---------|---------|
| IR | 0x000 | 0x40014000 | Interrupt flags |
| TCR | 0x004 | 0x40014004 | Timer control |
| TC | 0x008 | 0x40014008 | Timer counter |
| PR | 0x00C | 0x4001400C | Prescaler |
| MR0 | 0x018 | 0x40014018 | Match register 0 |
| MR1 | 0x01C | 0x4001401C | Match register 1 |
| MCR | 0x014 | 0x40014014 | Match control |

---

## CMSIS Headers: Pre-Defined Registers

Instead of defining all addresses yourself, you can use **CMSIS headers** from NXP. These define everything for you:

```c
// Without CMSIS (manual definitions):
#define SYSAHBCLKCTRL (*((volatile unsigned int *)0x40048080))
SYSAHBCLKCTRL |= (1 << 6);

// With CMSIS (pre-defined):
#include "LPC13xx.h"
LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 6);
```

### CMSIS Structure

The headers define structures for each peripheral:

```c
typedef struct {
    __IO uint32_t SYSMEMREMAP;     // 0x000
    __IO uint32_t PRESETCTRL;      // 0x004
    __IO uint32_t SYSPLLCTRL;      // 0x008
    __I  uint32_t SYSPLLSTAT;      // 0x00C (read-only)
    // ... more registers ...
    __IO uint32_t SYSAHBCLKCTRL;   // 0x080
    // ... more registers ...
} LPC_SYSCON_TypeDef;

#define LPC_SYSCON ((LPC_SYSCON_TypeDef *) 0x40048000)
```

### Where to Get CMSIS Headers

- NXP's MCUXpresso SDK
- LPCOpen libraries
- Various GitHub repositories

### Why This Curriculum Uses Raw Addresses

We use raw address definitions because:
1. You learn what's really happening
2. No external dependencies
3. Easier to understand the hardware
4. Matches the User Manual directly

Once you understand the fundamentals, using CMSIS is recommended for larger projects.

---

## Tips for Reading Datasheets

### 1. Start with the Block Diagram

Chapter 1 usually has a block diagram showing how peripherals connect. This gives you the big picture.

### 2. Use the Table of Contents

Don't read front-to-back. Jump to the peripheral you need.

### 3. Check the Register Summary First

Each chapter has a register summary table. Scan it to find what you need, then read the detailed description.

### 4. Pay Attention to Notes and Warnings

Look for boxes marked "Note:" or "Important:" - these often contain critical information.

### 5. Cross-Reference with Examples

The manual sometimes includes code snippets. Compare them with your code.

### 6. Check Errata

Manufacturers publish errata documents listing bugs in the silicon. Always check if strange behavior might be a known issue.

**LPC1343 Errata:** Search NXP's website for "LPC1343 errata"

---

## Summary

1. **User Manual (UM10375)** is your primary reference for registers
2. **Datasheet** is for electrical specs and pin information
3. Register Address = **Base Address + Offset**
4. Read the **bit field tables** to understand what each bit does
5. Check **reset values** to know the default state
6. Respect **reserved bits** - don't modify them
7. Consider **CMSIS headers** for larger projects

---

## Additional Resources

- [LPC1343 User Manual (UM10375)](https://www.nxp.com/docs/en/user-guide/UM10375.pdf)
- [LPC1343 Datasheet](https://www.nxp.com/docs/en/data-sheet/LPC1311_13_42_43.pdf)
- [ARM Cortex-M3 Technical Reference Manual](https://developer.arm.com/documentation/ddi0337/latest/)
- [ARM Cortex-M3 Generic User Guide](https://developer.arm.com/documentation/dui0552/latest/)

---

*Appendix E of the LPC1343 Embedded C Programming Guide*
