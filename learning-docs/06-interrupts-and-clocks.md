# Chapter 6: Interrupts and Clocks

A comprehensive guide to interrupt-driven programming and clock/PLL configuration for efficient embedded systems.

---

## Chapter Overview

| Section | What You'll Learn | Difficulty |
|---------|-------------------|------------|
| Part 0 | Essential fundamentals | ⭐ Beginner |
| Part 1 | How clocks work | ⭐⭐ Intermediate |
| Part 2 | PLL configuration | ⭐⭐ Intermediate |
| Part 3 | Interrupt basics | ⭐⭐ Intermediate |
| Part 4 | NVIC deep dive | ⭐⭐⭐ Advanced |
| Part 5 | Complete examples | ⭐⭐⭐ Advanced |

**Prerequisites:** [Chapter 4: Timers and PWM](04-timers-and-pwm.md) recommended

---

## Quick Start: Your First Interrupt

Before diving deep, let's see a working interrupt-driven LED blink:

```c
#include "LPC13xx.h"

volatile uint32_t ms_ticks = 0;

// Interrupt handler - called every 1ms
void SysTick_Handler(void) {
    ms_ticks++;
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

int main(void) {
    // Configure SysTick for 1ms interrupts (at 72MHz)
    SysTick->LOAD = 72000 - 1;  // Count to 72000 (72MHz/72000 = 1kHz = 1ms)
    SysTick->VAL = 0;            // Clear current value
    SysTick->CTRL = 0x07;        // Enable, use CPU clock, enable interrupt

    // Configure LED (P3.0)
    LPC_GPIO3->DIR |= (1 << 0);  // Output

    while (1) {
        LPC_GPIO3->DATA &= ~(1 << 0);  // LED ON
        delay_ms(500);
        LPC_GPIO3->DATA |= (1 << 0);   // LED OFF
        delay_ms(500);
    }
}
```

**What makes this different from polling?**
- The CPU isn't stuck in a loop counting
- `SysTick_Handler` is called automatically every 1ms
- Your main code can do other work between delays
- More accurate timing (hardware-driven)

**Key concepts you'll learn:**
1. **Clock sources** - Where the timing comes from
2. **PLL** - How to multiply the clock for higher speeds
3. **Interrupts** - How hardware notifies your code
4. **NVIC** - Managing multiple interrupt sources

---

## Part 0: Essential Fundamentals

Before diving into interrupts and clocks, let's cover some basic concepts you'll need to understand embedded systems code.

### What is a Microcontroller?

A **microcontroller** is a small computer on a single chip that includes:
- **CPU** (Central Processing Unit): The brain that executes instructions
- **Memory**: RAM (temporary storage) and Flash (program storage)
- **Peripherals**: Built-in hardware modules (timers, GPIO, UART, etc.)
- **Clock System**: Timing circuitry to control CPU speed

Think of it as a mini-computer designed to control things (like LEDs, motors, sensors) rather than run general software.

### Hexadecimal Numbers

You'll see numbers like `0x00000001` everywhere in embedded code. The `0x` prefix means it's **hexadecimal** (base-16).

**Why hexadecimal?** It maps perfectly to binary, which is how computers actually work:

```
Decimal | Hexadecimal | Binary
--------|-------------|----------
    0   |     0x0     | 0000
    1   |     0x1     | 0001
    8   |     0x8     | 1000
   15   |     0xF     | 1111
   16   |    0x10     | 0001 0000
  255   |    0xFF     | 1111 1111
```

**Examples from the code:**
- `0x00000001` = binary `00000000000000000000000000000001` (bit 0 is set)
- `0x00000020` = binary `00000000000000000000000000100000` (bit 5 is set)
- `0x000000F0` = binary `00000000000000000000000011110000` (bits 4-7 are set)

### Bitwise Operations

Embedded code manipulates individual **bits** (0s and 1s) to control hardware. Here are the key operators:

#### OR operator: `|` (turns bits ON)
```c
GPIO3DATA |= 0x00000001;  // Set bit 0 to 1 (turn LED OFF in this case)

Before:  0000 0000
OR       0000 0001
Result:  0000 0001  (bit 0 is now 1)
```

#### AND operator: `&` (can turn bits OFF)
```c
GPIO3DATA &= ~0x00000001;  // Clear bit 0 to 0 (turn LED ON)

Before:  0000 0001
AND      1111 1110  (the ~ inverts 0x01 to 0xFE)
Result:  0000 0000  (bit 0 is now 0)
```

#### NOT operator: `~` (inverts all bits)
```c
~0x00000001  →  0xFFFFFFFE

0000 0001 → 1111 1110
```

#### Left shift: `<<` (moves bits left)
```c
1 << 5  →  0x00000020

0000 0001 << 5 = 0010 0000  (bit moved from position 0 to 5)
```

**Why use bitwise operations?** To control individual hardware features without affecting others!

### Registers: Controlling Hardware

A **register** is a special memory location that controls hardware. When you write a value to a register, you're directly controlling hardware behavior.

**Key concept: Memory-Mapped I/O**
In microcontrollers, hardware peripherals are controlled by reading/writing to specific memory addresses, as if they were regular variables.

```c
GPIO3DATA |= 0x0000000F;  // This writes to a hardware register

// GPIO3DATA is actually at memory address like 0x50003FFC
// Writing to this address changes the output pins!
```

**Common registers in this code:**
- `GPIO3DATA`: Controls the state of GPIO Port 3 pins (main.c:26)
- `GPIO3DIR`: Sets pins as inputs or outputs (main.c:307)
- `TMR32B0MR0`: Timer match value (main.c:274)
- `PDRUNCFG`: Power down/up configuration (main.c:71)

### GPIO (General Purpose Input/Output)

**GPIO** pins are the physical pins on the microcontroller chip that you can control from software.

- **Output mode**: You control the voltage (HIGH=3.3V or LOW=0V) to drive LEDs, motors, etc.
- **Input mode**: You read external signals from buttons, sensors, etc.

```c
// Set pins as outputs (1 = output, 0 = input)
GPIO3DIR |= 0x0000000F;  // Set pins 0-3 as outputs

// Write data to outputs
GPIO3DATA |= 0x00000001;  // Set pin 0 HIGH
GPIO3DATA &= ~0x00000001; // Set pin 0 LOW
```

### LEDs and Active-Low Logic

An **LED** (Light Emitting Diode) is a component that emits light when current flows through it.

In this board, LEDs are connected **active-low**, meaning:
- **0V (LOW)** = LED turns ON (current flows)
- **3.3V (HIGH)** = LED turns OFF (no current)

This seems backwards at first! It's a common circuit design where the LED connects between the pin and power supply.

```c
#define LED0_ON   GPIO3DATA &= ~0x00000001;  // Clear bit (0V) = ON
#define LED0_OFF  GPIO3DATA |= 0x00000001;   // Set bit (3.3V) = OFF
```

### Peripherals

**Peripherals** are specialized hardware modules built into the microcontroller. Instead of doing everything in software, you configure peripherals to handle tasks automatically.

**Examples:**
- **Timers**: Count clock cycles and generate interrupts (main.c:258)
- **UART**: Send/receive serial data
- **SPI/I2C**: Communication with other chips
- **ADC**: Read analog voltages (like from sensors)
- **PWM**: Generate variable-speed signals (for motor control)

### The `volatile` Keyword

You'll see this keyword in embedded code:

```c
void Delay(volatile unsigned long cycles) {
    while(cycles) { cycles--; }
}
```

**What does `volatile` mean?**
It tells the compiler: "This variable might change unexpectedly, so always read it from memory - don't optimize it away!"

**Why is it needed?**
- Hardware registers can change without the CPU's direct involvement
- Variables shared between main code and interrupt handlers
- Prevents compiler optimization bugs

### Understanding Code Structure

Typical embedded program structure:

```c
// 1. Include hardware definitions
#include <nxp/iolpc1343.h>

// 2. Define macros for convenience
#define LED0_ON GPIO3DATA &= ~0x00000001;

// 3. Function prototypes (declarations)
void init_devices(void);

// 4. Initialization function
void init_devices(void) {
    // Configure hardware once at startup
}

// 5. Main function
void main(void) {
    init_devices();
    while(1) {
        // Infinite loop (embedded systems never "exit")
    }
}

// 6. Interrupt handlers
void TIMER_IRQHandler(void) {
    // Runs automatically when timer fires
}
```

### Quick Reference: Common Terms

| Term | Meaning |
|------|---------|
| **Bit** | Single binary digit (0 or 1) |
| **Byte** | 8 bits (e.g., 0xFF = 255) |
| **Word** | 32 bits on this processor |
| **Register** | Hardware control memory location |
| **Peripheral** | Built-in hardware module (timer, UART, etc.) |
| **GPIO** | General Purpose Input/Output pin |
| **ISR** | Interrupt Service Routine (interrupt handler function) |
| **MCU** | Microcontroller Unit |
| **SoC** | System on Chip |

### Reading Hardware Register Operations

When you see code like this:

```c
PDRUNCFG &= ~0x00000080;  // Clear bit 7
```

Break it down:
1. `PDRUNCFG` = A hardware register (Power Down Run Configuration)
2. `&=` = AND operation that modifies the register
3. `~0x00000080` = Inverted 0x80 = bit pattern with bit 7 clear, all others set
4. **Result**: Clears only bit 7, leaves all other bits unchanged

```c
TMR32B0TCR_bit.CE = 1;  // Set the CE bit to 1
```

This is a **bit-field** syntax:
- `TMR32B0TCR` = Timer Control Register
- `_bit` = Access individual bits by name
- `.CE` = Counter Enable bit
- `= 1` = Set it to 1 (enable)

---

## Part 1: Understanding Interrupts

### What is an Interrupt?

Think of an interrupt like a doorbell. You're sitting in your house doing something, and when someone rings the doorbell, you:
1. **Stop** what you're doing
2. **Save** your place (so you can return)
3. **Go answer** the door
4. **Return** to what you were doing

In a microcontroller, an interrupt is a signal that makes the CPU temporarily stop its current task and execute a special function called an **Interrupt Service Routine (ISR)** or **Interrupt Handler**.

### Two Approaches: Polling vs Interrupts

#### Polling (the inefficient way)

```c
void main(void) {
    init_devices();
    unsigned long counter = 0;

    while(1) {
        counter++;
        if(counter >= 18000000) {  // Check constantly!
            // Toggle LED
            counter = 0;
        }
        // CPU is ALWAYS busy checking this counter
    }
}
```

**Problems:**
- CPU wastes 100% of its time checking "is it time yet? is it time yet?"
- Can't do other tasks
- Burns power unnecessarily

#### Interrupt-Driven (the smart way)

```c
void main(void) {
    init_devices();
    while(1) {
        // CPU can sleep or do other work
        // Timer hardware counts in the background
        // When timer expires, interrupt automatically fires
    }
}

// This ONLY runs when the timer says "time's up!"
void CT32B0_IRQHandler(void) {
    // Toggle LED
}
```

**Benefits:**
- CPU can sleep (save power)
- CPU can do other tasks while waiting
- Hardware does the timing for you
- More efficient and responsive

### How Interrupts Work in This Code

1. **Timer Hardware** (CT32B0) counts clock cycles in the background
2. When counter reaches match value (MR0), timer triggers an interrupt
3. **NVIC** (Nested Vectored Interrupt Controller) receives the signal
4. CPU automatically:
   - Saves its current state
   - Jumps to `CT32B0_IRQHandler()` (main.c:346)
   - Executes the handler (toggle LED)
   - Returns to main loop
5. Timer resets and starts counting again

---

## Part 2: Understanding Clocks and Oscillators

### What is a Clock?

A clock is a repeating electrical signal that pulses on/off at a regular frequency. Think of it like a metronome for musicians - it keeps time.

```
Clock Signal:
     ___     ___     ___     ___
    |   |   |   |   |   |   |   |
___|   |___|   |___|   |___|   |___
    ^   ^   ^   ^   ^   ^   ^   ^
    Each pulse = 1 "clock cycle"
```

The CPU performs operations synchronized to these pulses. **Faster clock = more operations per second**.

### Clock Frequency

Measured in Hz (Hertz) = cycles per second
- 1 MHz = 1,000,000 cycles/second
- 12 MHz = 12,000,000 cycles/second
- 72 MHz = 72,000,000 cycles/second

### Oscillators: Where Clocks Come From

An **oscillator** is a circuit that generates a clock signal. The LPC1343 has two options:

#### 1. Internal RC Oscillator (IRC)
- Built into the chip
- 12 MHz
- ±1% accuracy (not super precise)
- Available immediately on power-up
- No external components needed

#### 2. External Crystal Oscillator
- Uses a physical quartz crystal (external component on PCB)
- Also 12 MHz in this board
- Very accurate (±0.001%)
- Better for timing-critical applications
- Needs external parts

In this code (main.c:298), it uses `'E'` = External oscillator

---

## Part 3: Understanding the PLL (Phase-Locked Loop)

### The Problem

The oscillators only provide 12 MHz, but modern microcontrollers need higher speeds to:
- Execute more instructions per second
- Process data faster
- Drive peripherals at higher speeds

This chip can run at up to **72 MHz** - that's **6 times faster** than the oscillator!

### What is a PLL?

A **PLL (Phase-Locked Loop)** is a special circuit that multiplies the frequency of an input clock to create a faster output clock.

Think of it like a gear system:

```
Input: 12 MHz        PLL         Output: 72 MHz
    --------→ [Multiply by 6] --------→
    (slow)                           (fast)
```

### How the PLL Works (Simplified)

The PLL has two key parameters:

#### 1. MSEL (Multiplier Select)
- Multiplies the input frequency
- Formula: `Output_Frequency = Input_Frequency × (MSEL + 1)`
- Example: If MSEL = 5, then 12 MHz × 6 = 72 MHz

#### 2. PSEL (Post-divider Select)
- Used to keep an internal frequency (CCO) in a safe range (156-320 MHz)
- The PLL actually creates a very high frequency internally, then divides it down

### Visual Example

```
Step 1: Input
12 MHz from crystal
    ↓
Step 2: PLL Internal (CCO)
12 MHz × 16 = 192 MHz (very fast, internal only)
    ↓
Step 3: Post-divider
192 MHz ÷ 2 (PSEL) = 96 MHz
Actually, code uses different math, but concept is same
    ↓
Step 4: System Clock Divider
Can divide again if needed (this code uses ÷1, main.c:154)
    ↓
Final: 72 MHz to CPU and peripherals
```

---

## Part 4: Following the Clock Setup in This Code

Let me trace through what happens when the code runs:

### Step 1: Default State (main.c:296-297)

```c
// On power-up, chip runs at 12 MHz from internal RC oscillator
// We want 72 MHz from external crystal instead
Init_System_Clock(System_Clock, 'E');
//                 72000000      External
```

### Step 2: External Oscillator Setup (main.c:132-141)

```c
PDRUNCFG |= 0x00000020;      // Turn OFF external oscillator
SYSOSCCLTRL = 0x00000000;    // Configure it: 1-20 MHz range
PDRUNCFG &= ~0x00000020;     // Turn ON external oscillator
// Now we have stable 12 MHz from crystal

Init_PLL(12000000, 'E', 72000000);
//       Input     External  Desired Output
```

### Step 3: PLL Calculation (main.c:66-101)

The code calculates MSEL and PSEL:

```c
// Goal: 12 MHz input → 72 MHz output

// MSEL calculation (main.c:93-94)
MSEL = PLL_Fclkout / PLL_Fclkin;
MSEL = 72000000 / 12000000 = 6
MSEL = (MSEL - 1) = 5  // Hardware adds 1 back

// PSEL calculation (main.c:86-90)
// PLL needs internal CCO between 156-320 MHz
// PSEL determines the divider to get there
PSEL = 96000000 / PLL_Fclkout
PSEL = 96000000 / 72000000 = 1.33...
// So PSEL = 01 (meaning divide by 2)

// Internal CCO will be: 72 MHz × 2 = 144 MHz
// (Actually more complex, but this is the idea)
```

### Step 4: Enable PLL and Wait for Lock (main.c:98-100)

```c
PDRUNCFG &= ~0x00000080;           // Turn ON PLL
while(!(SYSPLLSTAT & 0x1)){}       // Wait until stable
```

The PLL takes a few microseconds to "lock" - to stabilize at the right frequency. You must wait for this!

### Step 5: Switch Main Clock (main.c:153-159)

```c
SYSAHBCLKDIV |= 0x1;          // Use divider of 1 (no division)
MAINCLKSEL = 0x00000003;      // Select PLL output as main clock
MAINCLKUEN |= 0x00000001;     // Apply the change

// NOW the CPU runs at 72 MHz!
```

---

## Part 5: How Timer Uses the Clock

### Timer Configuration (main.c:318)

```c
CT32B0_Init(4);  // Initialize with divider of 4
```

### Timer Math (main.c:274)

```c
TMR32B0MR0 = (System_Clock / SYSAHBCLKDIV) / Match_Level_Divider;
TMR32B0MR0 = (72,000,000 / 1) / 4
TMR32B0MR0 = 18,000,000
```

This means:
- Timer counts from 0 to 18,000,000
- Each count = 1 clock cycle = 1/72,000,000 second
- Time per interrupt = 18,000,000 ÷ 72,000,000 = **0.25 seconds**

**So LED0 toggles every 0.25 seconds** (4 times per second)

Since toggle means ON→OFF or OFF→ON, the LED completes a full blink cycle (ON and OFF) every **0.5 seconds**, or **2 Hz**.

---

## Summary Flowchart

```
Power On
    ↓
[12 MHz Internal RC Oscillator] ← Default
    ↓
init_devices() called
    ↓
Init_System_Clock(72MHz, 'E')
    ↓
[12 MHz External Crystal] ← More stable
    ↓
Init_PLL() multiplies by 6
    ↓
[72 MHz PLL Output]
    ↓
Main Clock = 72 MHz ← CPU runs at this speed
    ↓
CT32B0 Timer uses this clock
    ↓
Counts to 18,000,000 (0.25 sec)
    ↓
INTERRUPT! → CT32B0_IRQHandler()
    ↓
Toggle LED0
    ↓
Clear interrupt, return to main
    ↓
(Repeat forever)
```

---

## Key Takeaways

1. **Interrupts** let hardware notify the CPU when something happens, so the CPU doesn't waste time checking constantly

2. **Oscillators** create the base clock signal (12 MHz)

3. **PLL** multiplies that slow clock to a faster speed (72 MHz) so the CPU can work faster

4. **Timer** counts clock cycles in hardware and generates interrupts at precise intervals

5. **Interrupt Handler** is a function that runs automatically when the interrupt fires

---

## Additional Notes

### Why Use Interrupts?

Interrupts are essential in embedded systems for:
- **Power efficiency**: CPU can sleep between events
- **Responsiveness**: React immediately to external events
- **Multitasking**: Handle multiple tasks without explicitly checking each one
- **Precise timing**: Hardware timers are more accurate than software delays

### Common Interrupt Sources

- **Timers**: Generate periodic interrupts (like in this code)
- **GPIO pins**: External buttons, sensors
- **UART/SPI/I2C**: Communication interfaces
- **ADC**: Analog-to-digital conversion complete
- **DMA**: Data transfer complete

### NVIC (Nested Vectored Interrupt Controller)

The NVIC is the hardware that manages interrupts on ARM Cortex-M processors:
- Can handle multiple interrupt sources
- Prioritizes interrupts (higher priority can interrupt lower priority)
- Automatically saves/restores CPU state
- Provides pending/active status for each interrupt

### Best Practices for ISRs

1. **Keep them SHORT**: Do minimal work in the handler
2. **Don't block**: No delays or waiting in ISRs
3. **Set flags**: Often ISRs just set a flag that main loop checks
4. **Clear flags**: Always clear the interrupt flag before returning
5. **Volatile variables**: Use `volatile` keyword for variables shared between ISR and main

---

## Further Learning

To deepen your understanding:

1. Read the LPC1343 User Manual (chapters on Clock Generation and Timers)
2. Study ARM Cortex-M3 documentation (the CPU core used in LPC1343)
3. Experiment with changing timer dividers to see different blink rates
4. Try adding interrupts from GPIO pins (button presses)
5. Learn about Real-Time Operating Systems (RTOS) which build on these concepts

---

## What's Next?

Congratulations! You've completed the core curriculum of the Embedded C Learning Series!

**You now understand:**
- Bitwise operations for register manipulation
- The firmware build process
- GPIO for input/output control
- Timers and PWM for time-based operations
- UART for serial communication
- Interrupts and clock configuration

**Continue your journey:**
- [Return to Index](00-index.md) - Review the complete learning path
- [Chapter 0: Getting Started](00-getting-started.md) - Quick reference
- Build your own projects combining these concepts!

**Suggested projects to solidify your learning:**
1. **Reaction timer** - Measure button press reaction time with UART output
2. **PWM LED with serial control** - Adjust brightness via terminal commands
3. **Multi-channel data logger** - ADC sampling with timestamped UART output
4. **State machine** - Interrupt-driven mode switching

---

*Chapter 6 of the Embedded C Learning Series*
*Part of the LPC1343 Learning Library*
