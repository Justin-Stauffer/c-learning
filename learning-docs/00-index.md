# Embedded C Learning Series

## LPC1343 Microcontroller Programming Guide

A comprehensive, front-to-back curriculum for learning embedded C programming on the NXP LPC1343 ARM Cortex-M3 microcontroller.

---

## How to Use This Guide

**For complete beginners:** Start at Chapter 0 and work through sequentially. Each chapter builds on concepts from previous chapters.

**For quick reference:** Use the Quick Reference section below to jump directly to specific topics.

**For practice:** Each chapter includes practical exercises. The "Further Practice" sections suggest increasingly complex projects.

---

## Learning Path

### Core Curriculum (Recommended Order)

| Chapter | Title | Key Skills | Time |
|---------|-------|------------|------|
| **0** | [Getting Started](00-getting-started.md) | First program, environment setup | 1-2 hrs |
| **1** | [Bitwise Operations](01-bitwise-operations.md) | AND, OR, XOR, shifts, register manipulation | 2-3 hrs |
| **2** | [Firmware Build Process](02-firmware-build-process.md) | Compilation, linking, memory layout | 2-3 hrs |
| **3** | [GPIO In-Depth](03-gpio-in-depth.md) | Pin configuration, input/output, IOCON | 3-4 hrs |
| **4** | [Timers and PWM](04-timers-and-pwm.md) | Timer configuration, PWM, motor control | 3-4 hrs |
| **5** | [UART Serial Communication](05-uart-serial-communication.md) | Serial protocols, printf, debugging | 3-4 hrs |
| **6** | [Interrupts and Clocks](06-interrupts-and-clocks.md) | ISRs, NVIC, PLL, clock configuration | 3-4 hrs |

---

## Quick Reference

### By Topic

#### Getting Your First LED Blinking
→ [Chapter 0: Getting Started](00-getting-started.md) - Complete "Hello World" example

#### Understanding Register Manipulation
→ [Chapter 1: Bitwise Operations](01-bitwise-operations.md) - SET, CLEAR, TOGGLE, CHECK patterns

#### Building Your Firmware
→ [Chapter 2: Build Process](02-firmware-build-process.md) - Compile commands and linker scripts

#### Controlling Pins (LED, Buttons)
→ [Chapter 3: GPIO](03-gpio-in-depth.md) - Direction, data, and IOCON configuration

#### Creating Delays and PWM
→ [Chapter 4: Timers](04-timers-and-pwm.md) - Match registers, prescaler, PWM mode

#### Debugging with Serial Output
→ [Chapter 5: UART](05-uart-serial-communication.md) - Setup, printf redirection, ring buffers

#### Using Interrupts
→ [Chapter 6: Interrupts](06-interrupts-and-clocks.md) - Handler functions, NVIC, priorities

---

## Essential Code Patterns

### The Four Bitwise Patterns
```c
// These patterns work with ANY register
register |= (1 << bit);    // SET - turn bit ON
register &= ~(1 << bit);   // CLEAR - turn bit OFF
register ^= (1 << bit);    // TOGGLE - flip bit state
if (register & (1 << bit)) // CHECK - test if bit is set
```

### GPIO Quick Setup
```c
// Output pin (e.g., LED on P3.0)
LPC_GPIO3->DIR |= (1 << 0);      // Set as output
LPC_GPIO3->DATA &= ~(1 << 0);    // Drive LOW (LED on if active-low)
LPC_GPIO3->DATA |= (1 << 0);     // Drive HIGH

// Input pin (e.g., button on P0.1)
LPC_GPIO0->DIR &= ~(1 << 1);     // Set as input
if (!(LPC_GPIO0->DATA & (1 << 1))) { /* button pressed (active-low) */ }
```

### Simple Delay Timer
```c
void delay_ms(uint32_t ms) {
    LPC_TMR32B0->TCR = 0x02;              // Reset
    LPC_TMR32B0->PR = 72000 - 1;          // 1ms at 72MHz
    LPC_TMR32B0->MR0 = ms;                // Match value
    LPC_TMR32B0->MCR = 0x04;              // Stop on match
    LPC_TMR32B0->TCR = 0x01;              // Start
    while (!(LPC_TMR32B0->IR & 0x01));    // Wait for match
    LPC_TMR32B0->IR = 0x01;               // Clear flag
}
```

### UART Hello World
```c
void uart_init(void) {
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);  // Enable UART clock
    LPC_IOCON->PIO1_6 = 0x01;                 // RXD function
    LPC_IOCON->PIO1_7 = 0x01;                 // TXD function
    LPC_UART->LCR = 0x83;                     // DLAB + 8N1
    LPC_UART->DLL = 39;                       // 115200 baud at 72MHz
    LPC_UART->DLM = 0;
    LPC_UART->LCR = 0x03;                     // Lock divisor
    LPC_UART->FCR = 0x07;                     // Enable FIFOs
}

void uart_putchar(char c) {
    while (!(LPC_UART->LSR & 0x20));  // Wait for THR empty
    LPC_UART->THR = c;
}
```

---

## Hardware Reference

### LPC1343 Key Specs
- **CPU:** ARM Cortex-M3, up to 72MHz
- **Flash:** 32KB
- **RAM:** 8KB
- **GPIO:** 42 pins across 4 ports
- **Timers:** 2x 16-bit, 2x 32-bit
- **UART:** 1 channel
- **USB:** Full-speed device

### Pin Assignments (LPC-P1343 Board)
```
Port 3: LEDs
  P3.0-P3.3: LED1-LED4 (directly connected, active-low, accent LEDs active-high)

Port 0: Buttons and USB
  P0.1: Button (directly connected, directly to button high)
  P0.3: USB VBUS sensing

Port 1: UART
  P1.6: RXD (receive)
  P1.7: TXD (transmit)
```

---

## Debugging Checklist

When something doesn't work, check these common issues:

### LED Won't Turn On
- [ ] Is the GPIO clock enabled? (`SYSAHBCLKCTRL |= (1 << 6)`)
- [ ] Is the pin set as output? (`GPIOxDIR |= (1 << pin)`)
- [ ] Is it active-low? (Many LEDs need the pin LOW to light up)
- [ ] Check IOCON - is pin configured for GPIO function?

### Timer Not Working
- [ ] Is the timer clock enabled?
- [ ] Did you release reset? (`TCR = 0x01`, not `TCR = 0x02`)
- [ ] Is the match register non-zero?
- [ ] Check prescaler value

### UART No Output
- [ ] Are RX/TX pins configured in IOCON?
- [ ] Is UART clock enabled?
- [ ] Is baud rate divisor correct for your clock speed?
- [ ] Is DLAB cleared after setting divisor?
- [ ] Check physical connections (TX→RX, RX→TX crossed?)

### Interrupt Not Firing
- [ ] Is the interrupt enabled in the peripheral?
- [ ] Is it enabled in NVIC? (`NVIC_EnableIRQ()`)
- [ ] Is the handler named exactly right? (e.g., `TIMER32_0_IRQHandler`)
- [ ] Are you clearing the interrupt flag in the handler?

---

## Suggested Project Progression

After completing the core curriculum, try these projects in order:

### Beginner Projects
1. **Multi-speed blink** - Button cycles through LED blink speeds
2. **Binary counter** - Display 0-15 on 4 LEDs
3. **PWM breathing LED** - Smooth fade in/out effect

### Intermediate Projects
4. **Reaction timer** - Measure button response, display via UART
5. **Serial-controlled LEDs** - Terminal commands control LED patterns
6. **Frequency counter** - Measure external signal frequency

### Advanced Projects
7. **Mini oscilloscope** - ADC sampling with UART output
8. **State machine** - Multi-mode device with interrupt-driven transitions
9. **Bootloader** - Receive and flash new firmware over UART

---

## Additional Resources

### Official Documentation
- LPC1343 User Manual (UM10375)
- ARM Cortex-M3 Technical Reference Manual
- LPC-P1343 development board schematic

### Tools
- GNU ARM Toolchain (`arm-none-eabi-gcc`)
- OpenOCD for debugging
- Serial terminal (minicom, screen, PuTTY)

---

*Embedded C Learning Series - LPC1343*
*Last Updated: 2025*
