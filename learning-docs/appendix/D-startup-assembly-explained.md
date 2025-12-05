# Appendix D: Startup Assembly Explained

## What Runs Before main()?

When you power on a microcontroller, your `main()` function doesn't run immediately. The **startup assembly file** runs first, handling critical initialization that C code cannot do.

---

## The Three Jobs of Startup Code

1. **Vector Table** - A lookup table telling the CPU where to jump for interrupts
2. **Reset Handler** - Initializes memory, then calls `main()`
3. **Default Handlers** - Placeholder functions for unused interrupts

---

## Part 1: Assembler Directives

```asm
    .syntax unified      /* Use modern ARM syntax */
    .cpu cortex-m3       /* Target CPU type */
    .fpu softvfp         /* No hardware floating point */
    .thumb               /* Use Thumb instruction set (required for Cortex-M) */
```

These tell the assembler what CPU we're targeting. You find this in the chip's datasheet - the LPC1343 uses a Cortex-M3 core.

---

## Part 2: Stack Pointer

```asm
_estack = 0x10002000    /* End of RAM (8KB) */
```

**Where does 0x10002000 come from?**

From the LPC1343 User Manual memory map:

```
SRAM Start:  0x10000000
SRAM Size:   8KB = 8192 bytes = 0x2000
SRAM End:    0x10000000 + 0x2000 = 0x10002000
```

The stack grows **downward** in ARM, so we start at the top of RAM:

```
0x10002000  ┌──────────────┐  ← Stack starts here (_estack)
            │    Stack     │    grows downward ↓
            │      ↓       │
            │              │
            │      ↑       │
            │    Heap      │    grows upward ↑
            │──────────────│
            │     BSS      │    Uninitialized globals
            │──────────────│
            │    Data      │    Initialized globals
0x10000000  └──────────────┘  ← RAM starts here
```

---

## Part 3: Vector Table

The vector table is a list of addresses at the very beginning of Flash memory. When the CPU starts or an interrupt fires, it looks here to find which function to call.

```asm
    .section .isr_vector,"a",%progbits

g_pfnVectors:
    .word   _estack              /* 0x0000: Initial stack pointer */
    .word   Reset_Handler        /* 0x0004: Reset - where execution starts */
    .word   NMI_Handler          /* 0x0008: Non-maskable interrupt */
    .word   HardFault_Handler    /* 0x000C: Hard fault */
    .word   MemManage_Handler    /* 0x0010: Memory management fault */
    .word   BusFault_Handler     /* 0x0014: Bus fault */
    .word   UsageFault_Handler   /* 0x0018: Usage fault */
    .word   0                    /* 0x001C: Reserved */
    .word   0                    /* 0x0020: Reserved */
    .word   0                    /* 0x0024: Reserved */
    .word   0                    /* 0x0028: Reserved */
    .word   SVC_Handler          /* 0x002C: Supervisor call */
    .word   DebugMon_Handler     /* 0x0030: Debug monitor */
    .word   0                    /* 0x0034: Reserved */
    .word   PendSV_Handler       /* 0x0038: Pendable service request */
    .word   SysTick_Handler      /* 0x003C: System tick timer */

    /* Chip-specific interrupts start at 0x0040 */
    .word   IRQ0_Handler         /* 0x0040: First device interrupt */
    .word   IRQ1_Handler         /* 0x0044: Second device interrupt */
    /* ... more interrupts ... */
```

### Vector Table Layout

| Address | Vector # | Name | Description |
|---------|----------|------|-------------|
| 0x0000 | - | Initial SP | Stack pointer value loaded at reset |
| 0x0004 | 1 | Reset | Where code execution begins |
| 0x0008 | 2 | NMI | Non-maskable interrupt |
| 0x000C | 3 | HardFault | All unhandled faults |
| 0x0010 | 4 | MemManage | Memory protection fault |
| 0x0014 | 5 | BusFault | Bus error |
| 0x0018 | 6 | UsageFault | Undefined instruction, etc. |
| 0x001C-0x0028 | - | Reserved | Must be zero |
| 0x002C | 11 | SVCall | Supervisor call (for RTOS) |
| 0x0030 | 12 | DebugMon | Debug monitor |
| 0x0034 | - | Reserved | Must be zero |
| 0x0038 | 14 | PendSV | Pendable service (for RTOS) |
| 0x003C | 15 | SysTick | System tick timer |
| 0x0040+ | 16+ | IRQn | Device-specific interrupts |

### Where to Find Vector Information

1. **ARM Cortex-M3 Technical Reference Manual** - Defines the first 16 entries (standard for all Cortex-M3 chips)
2. **LPC1343 User Manual (UM10375), Chapter 4** - Defines chip-specific interrupts (IRQ 0-31)

### LPC1343 Device Interrupts

| IRQ # | Vector Addr | Handler Name | Source |
|-------|-------------|--------------|--------|
| 0-39 | 0x0040-0x00DC | WAKEUP0-39_IRQHandler | GPIO wake-up |
| 40 | 0x00E0 | I2C0_IRQHandler | I2C |
| 41 | 0x00E4 | CT16B0_IRQHandler | 16-bit Timer 0 |
| 42 | 0x00E8 | CT16B1_IRQHandler | 16-bit Timer 1 |
| 43 | 0x00EC | CT32B0_IRQHandler | 32-bit Timer 0 |
| 44 | 0x00F0 | CT32B1_IRQHandler | 32-bit Timer 1 |
| 45 | 0x00F4 | SSP0_IRQHandler | SPI |
| 46 | 0x00F8 | UART0_IRQHandler | UART |
| 47 | 0x00FC | USBIRQ_IRQHandler | USB IRQ |
| 48 | 0x0100 | USBFIQ_IRQHandler | USB FIQ |
| 49 | 0x0104 | ADC_IRQHandler | ADC |
| 50 | 0x0108 | WDT_IRQHandler | Watchdog |
| 51 | 0x010C | BOD_IRQHandler | Brown-out detect |
| 53 | 0x0114 | PIO3_IRQHandler | GPIO Port 3 |
| 54 | 0x0118 | PIO2_IRQHandler | GPIO Port 2 |
| 55 | 0x011C | PIO1_IRQHandler | GPIO Port 1 |
| 56 | 0x0120 | PIO0_IRQHandler | GPIO Port 0 |

---

## Part 4: Reset Handler

The Reset Handler is the first code that runs. It prepares the C runtime environment:

```asm
Reset_Handler:
    /* Step 1: Copy initialized data from Flash to RAM */
    movs r1, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r3, =_sidata        /* Source: data in Flash */
    ldr r3, [r3, r1]        /* Load word from Flash */
    str r3, [r0, r1]        /* Store to RAM */
    adds r1, r1, #4         /* Next word */

LoopCopyDataInit:
    ldr r0, =_sdata         /* Destination: start of .data in RAM */
    ldr r3, =_edata         /* End of .data in RAM */
    adds r2, r0, r1
    cmp r2, r3
    bcc CopyDataInit        /* Continue if not done */

    /* Step 2: Zero the BSS section */
    ldr r2, =_sbss
    b LoopFillZerobss

FillZerobss:
    movs r3, #0
    str r3, [r2], #4        /* Store zero, increment pointer */

LoopFillZerobss:
    ldr r3, =_ebss
    cmp r2, r3
    bcc FillZerobss         /* Continue if not done */

    /* Step 3: Call C library init (for static constructors) */
    bl __libc_init_array

    /* Step 4: Call main() */
    bl main

    /* Step 5: Loop forever if main() returns */
    bx lr
```

### Why Copy Data from Flash to RAM?

When you write:
```c
int my_variable = 42;  /* Initial value stored in Flash */
```

The value `42` is stored in Flash (read-only). At startup, the Reset Handler copies it to RAM so your code can modify it later.

```
Flash (Read-Only)              RAM (Read-Write)
┌──────────────┐              ┌──────────────┐
│ Code (.text) │              │              │
│──────────────│              │              │
│ Initial data │ ──copy───>   │ .data section│
│ values       │              │ (modifiable) │
└──────────────┘              └──────────────┘
```

### Why Zero the BSS Section?

The C standard says uninitialized global variables must be zero:

```c
int counter;  /* Must be 0, not random garbage */
```

The startup code sets the entire `.bss` section to zero to guarantee this.

### Memory Symbols from Linker Script

The symbols `_sdata`, `_edata`, `_sbss`, `_ebss`, `_sidata` come from the **linker script** (`lpc1343_flash.ld`):

| Symbol | Meaning |
|--------|---------|
| `_sdata` | Start of .data section in RAM |
| `_edata` | End of .data section in RAM |
| `_sbss` | Start of .bss section in RAM |
| `_ebss` | End of .bss section in RAM |
| `_sidata` | Location of .data initial values in Flash |

---

## Part 5: Default Handler and Weak Aliases

```asm
Default_Handler:
Infinite_Loop:
    b Infinite_Loop    /* Loop forever */

/* Weak aliases - can be overridden */
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak UART0_IRQHandler
    .thumb_set UART0_IRQHandler, Default_Handler
```

### What Does "Weak" Mean?

A **weak symbol** can be overridden by a **strong symbol** of the same name.

```c
/* In startup.s - weak definition */
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler

/* In your main.c - strong definition (overrides weak) */
void SysTick_Handler(void) {
    /* Your interrupt code */
}
```

This is why you can write an interrupt handler in C and it "magically" gets called - your function overrides the weak default.

### Why Use an Infinite Loop?

If an unexpected interrupt fires and you haven't written a handler, the `Default_Handler` catches it. The infinite loop:

1. Prevents random code execution
2. Makes debugging easier (you can break here)
3. Indicates something unexpected happened

---

## How to Create a Startup File for a New Chip

### Step 1: Gather Information

You need two documents:

1. **ARM Cortex-M Technical Reference Manual**
   - Defines standard exceptions (Reset, NMI, HardFault, SysTick, etc.)
   - Available from developer.arm.com

2. **Chip User Manual** (e.g., LPC1343 UM10375)
   - Memory map (RAM/Flash addresses and sizes)
   - Interrupt sources and IRQ numbers

### Step 2: Extract Key Values

| Information | Where to Find | LPC1343 Value |
|-------------|---------------|---------------|
| RAM Start | Memory Map chapter | 0x10000000 |
| RAM Size | Memory Map chapter | 8 KB |
| Flash Start | Memory Map chapter | 0x00000000 |
| Flash Size | Memory Map chapter | 32 KB |
| # of IRQs | Interrupt chapter | 32 device-specific |
| IRQ Names | Interrupt chapter | Table 54 in UM10375 |

### Step 3: Use a Template

The easiest approach is to start with an existing startup file and modify it:

**Sources for startup files:**

1. **Chip Vendor SDK** - NXP provides LPCOpen with startup files
2. **CMSIS** - ARM's Cortex Microcontroller Software Interface Standard
3. **Other projects** - GitHub has many examples

### Step 4: Minimal Startup Template

Here's the absolute minimum for any Cortex-M3:

```asm
    .syntax unified
    .cpu cortex-m3
    .thumb

/* Calculate from datasheet: RAM_START + RAM_SIZE */
_estack = 0x10002000

    .section .isr_vector
g_pfnVectors:
    .word _estack           /* Initial stack pointer */
    .word Reset_Handler     /* Reset vector */
    .word Default_Handler   /* NMI */
    .word Default_Handler   /* HardFault */
    .word Default_Handler   /* MemManage */
    .word Default_Handler   /* BusFault */
    .word Default_Handler   /* UsageFault */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word Default_Handler   /* SVCall */
    .word Default_Handler   /* DebugMon */
    .word 0                 /* Reserved */
    .word Default_Handler   /* PendSV */
    .word Default_Handler   /* SysTick */
    /* Add chip-specific IRQs here */

    .section .text
Reset_Handler:
    /* Minimal version - just call main */
    ldr r0, =main
    bx r0

Default_Handler:
    b .    /* Infinite loop */

    .weak Reset_Handler
```

---

## Common Assembly Instructions Reference

| Instruction | Meaning | Example |
|-------------|---------|---------|
| `.word` | Store 32-bit value | `.word 0x10002000` |
| `.section` | Switch to named section | `.section .text` |
| `.weak` | Define weak symbol | `.weak SysTick_Handler` |
| `.thumb_set` | Create alias | `.thumb_set NMI_Handler, Default_Handler` |
| `.global` | Make symbol visible | `.global Reset_Handler` |
| `ldr` | Load register | `ldr r0, =_sdata` |
| `str` | Store register | `str r3, [r2]` |
| `movs` | Move immediate | `movs r1, #0` |
| `adds` | Add | `adds r1, r1, #4` |
| `cmp` | Compare | `cmp r2, r3` |
| `b` | Branch always | `b LoopStart` |
| `bcc` | Branch if carry clear | `bcc CopyLoop` |
| `bl` | Branch with link (call) | `bl main` |
| `bx` | Branch exchange | `bx lr` |

---

## Debugging Startup Issues

### Symptom: Code Never Reaches main()

1. Check vector table is at address 0x0000
2. Verify `_estack` is correct (top of RAM)
3. Check linker script symbols match startup expectations

### Symptom: HardFault Immediately

1. Stack pointer might be wrong
2. Vector table might be corrupted
3. Flash might not be programmed correctly

### Symptom: Interrupts Don't Work

1. Check handler name matches vector table exactly
2. Verify handler is not declared `static`
3. Ensure NVIC is enabled for that interrupt

### Using the Debugger

Set breakpoints at:
- `Reset_Handler` - Verify startup runs
- `main` - Verify initialization completes
- `Default_Handler` - Catch unexpected interrupts

---

## Resources

- [ARM Cortex-M3 Technical Reference Manual](https://developer.arm.com/documentation/ddi0337/latest/)
- [LPC1343 User Manual (UM10375)](https://www.nxp.com/docs/en/user-guide/UM10375.pdf)
- [ARM CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)

---

**Return to:** [Index](../00-index.md) | [Chapter 2: Firmware Build Process](../02-firmware-build-process.md)

---

*Appendix D of the Embedded C Learning Series*
*Understanding Startup Assembly Code*
