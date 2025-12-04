/**************************************************
 * Startup code for LPC1343 (ARM Cortex-M3)
 * GCC ARM Toolchain version
 *
 * This file contains:
 * - Vector table (interrupt handlers)
 * - Reset handler (initialization code)
 * - Default handler for unused interrupts
 **************************************************/

    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

/* Stack Configuration */
.global _estack
_estack = 0x10002000    /* End of RAM (8KB) */

/* Entry point */
.global Reset_Handler
.global Default_Handler

/**************************************************
 * Vector Table
 * Located at address 0x00000000
 **************************************************/

    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    /* Core Cortex-M3 Exceptions */
    .word   _estack                     /* 0x0000: Top of Stack */
    .word   Reset_Handler               /* 0x0004: Reset Handler */
    .word   NMI_Handler                 /* 0x0008: NMI Handler */
    .word   HardFault_Handler           /* 0x000C: Hard Fault Handler */
    .word   MemManage_Handler           /* 0x0010: MPU Fault Handler */
    .word   BusFault_Handler            /* 0x0014: Bus Fault Handler */
    .word   UsageFault_Handler          /* 0x0018: Usage Fault Handler */
    .word   0                           /* 0x001C: Reserved */
    .word   0                           /* 0x0020: Reserved */
    .word   0                           /* 0x0024: Reserved */
    .word   0                           /* 0x0028: Reserved */
    .word   SVC_Handler                 /* 0x002C: SVCall Handler */
    .word   DebugMon_Handler            /* 0x0030: Debug Monitor Handler */
    .word   0                           /* 0x0034: Reserved */
    .word   PendSV_Handler              /* 0x0038: PendSV Handler */
    .word   SysTick_Handler             /* 0x003C: SysTick Handler */

    /* LPC1343-Specific External Interrupts */
    .word   WAKE_UP0_IRQHandler         /* 0x0040: WAKEUP0 Handler */
    .word   WAKE_UP1_IRQHandler         /* 0x0044: WAKEUP1 Handler */
    .word   WAKE_UP2_IRQHandler         /* 0x0048: WAKEUP2 Handler */
    .word   WAKE_UP3_IRQHandler         /* 0x004C: WAKEUP3 Handler */
    .word   WAKE_UP4_IRQHandler         /* 0x0050: WAKEUP4 Handler */
    .word   WAKE_UP5_IRQHandler         /* 0x0054: WAKEUP5 Handler */
    .word   WAKE_UP6_IRQHandler         /* 0x0058: WAKEUP6 Handler */
    .word   WAKE_UP7_IRQHandler         /* 0x005C: WAKEUP7 Handler */
    .word   WAKE_UP8_IRQHandler         /* 0x0060: WAKEUP8 Handler */
    .word   WAKE_UP9_IRQHandler         /* 0x0064: WAKEUP9 Handler */
    .word   WAKE_UP10_IRQHandler        /* 0x0068: WAKEUP10 Handler */
    .word   WAKE_UP11_IRQHandler        /* 0x006C: WAKEUP11 Handler */
    .word   WAKE_UP12_IRQHandler        /* 0x0070: WAKEUP12 Handler */
    .word   WAKE_UP13_IRQHandler        /* 0x0074: WAKEUP13 Handler */
    .word   WAKE_UP14_IRQHandler        /* 0x0078: WAKEUP14 Handler */
    .word   WAKE_UP15_IRQHandler        /* 0x007C: WAKEUP15 Handler */
    .word   WAKE_UP16_IRQHandler        /* 0x0080: WAKEUP16 Handler */
    .word   WAKE_UP17_IRQHandler        /* 0x0084: WAKEUP17 Handler */
    .word   WAKE_UP18_IRQHandler        /* 0x0088: WAKEUP18 Handler */
    .word   WAKE_UP19_IRQHandler        /* 0x008C: WAKEUP19 Handler */
    .word   WAKE_UP20_IRQHandler        /* 0x0090: WAKEUP20 Handler */
    .word   WAKE_UP21_IRQHandler        /* 0x0094: WAKEUP21 Handler */
    .word   WAKE_UP22_IRQHandler        /* 0x0098: WAKEUP22 Handler */
    .word   WAKE_UP23_IRQHandler        /* 0x009C: WAKEUP23 Handler */
    .word   WAKE_UP24_IRQHandler        /* 0x00A0: WAKEUP24 Handler */
    .word   WAKE_UP25_IRQHandler        /* 0x00A4: WAKEUP25 Handler */
    .word   WAKE_UP26_IRQHandler        /* 0x00A8: WAKEUP26 Handler */
    .word   WAKE_UP27_IRQHandler        /* 0x00AC: WAKEUP27 Handler */
    .word   WAKE_UP28_IRQHandler        /* 0x00B0: WAKEUP28 Handler */
    .word   WAKE_UP29_IRQHandler        /* 0x00B4: WAKEUP29 Handler */
    .word   WAKE_UP30_IRQHandler        /* 0x00B8: WAKEUP30 Handler */
    .word   WAKE_UP31_IRQHandler        /* 0x00BC: WAKEUP31 Handler */
    .word   WAKE_UP32_IRQHandler        /* 0x00C0: WAKEUP32 Handler */
    .word   WAKE_UP33_IRQHandler        /* 0x00C4: WAKEUP33 Handler */
    .word   WAKE_UP34_IRQHandler        /* 0x00C8: WAKEUP34 Handler */
    .word   WAKE_UP35_IRQHandler        /* 0x00CC: WAKEUP35 Handler */
    .word   WAKE_UP36_IRQHandler        /* 0x00D0: WAKEUP36 Handler */
    .word   WAKE_UP37_IRQHandler        /* 0x00D4: WAKEUP37 Handler */
    .word   WAKE_UP38_IRQHandler        /* 0x00D8: WAKEUP38 Handler */
    .word   WAKE_UP39_IRQHandler        /* 0x00DC: WAKEUP39 Handler */
    .word   I2C0_IRQHandler             /* 0x00E0: I2C0 Handler */
    .word   CT16B0_IRQHandler           /* 0x00E4: 16-bit Timer0 Handler */
    .word   CT16B1_IRQHandler           /* 0x00E8: 16-bit Timer1 Handler */
    .word   CT32B0_IRQHandler           /* 0x00EC: 32-bit Timer0 Handler */
    .word   CT32B1_IRQHandler           /* 0x00F0: 32-bit Timer1 Handler */
    .word   SSP0_IRQHandler             /* 0x00F4: SSP0 Handler */
    .word   UART0_IRQHandler            /* 0x00F8: UART Handler */
    .word   USBIRQ_IRQHandler           /* 0x00FC: USB IRQ Handler */
    .word   USBFIQ_IRQHandler           /* 0x0100: USB FIQ Handler */
    .word   ADC_IRQHandler              /* 0x0104: A/D Converter Handler */
    .word   WDT_IRQHandler              /* 0x0108: Watchdog Timer Handler */
    .word   BOD_IRQHandler              /* 0x010C: Brown Out Detect Handler */
    .word   0                           /* 0x0110: Reserved */
    .word   PIO3_IRQHandler             /* 0x0114: PIO3 Handler */
    .word   PIO2_IRQHandler             /* 0x0118: PIO2 Handler */
    .word   PIO1_IRQHandler             /* 0x011C: PIO1 Handler */
    .word   PIO0_IRQHandler             /* 0x0120: PIO0 Handler */

/**************************************************
 * Reset Handler
 * This code runs after power-on/reset
 **************************************************/

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Copy the data segment initializers from flash to SRAM */
    movs r1, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r3, =_sidata
    ldr r3, [r3, r1]
    str r3, [r0, r1]
    adds r1, r1, #4

LoopCopyDataInit:
    ldr r0, =_sdata
    ldr r3, =_edata
    adds r2, r0, r1
    cmp r2, r3
    bcc CopyDataInit
    ldr r2, =_sbss
    b LoopFillZerobss

/* Zero fill the bss segment */
FillZerobss:
    movs r3, #0
    str r3, [r2], #4

LoopFillZerobss:
    ldr r3, = _ebss
    cmp r2, r3
    bcc FillZerobss

    /* Call static constructors (if any) */
    bl __libc_init_array

    /* Call the application's entry point */
    bl main
    bx lr
.size Reset_Handler, .-Reset_Handler

/**************************************************
 * Default Interrupt Handler
 * Infinite loop for unhandled interrupts
 **************************************************/

    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler

/**************************************************
 * Weak Aliases for Interrupt Handlers
 * Can be overridden by user-defined handlers
 **************************************************/

/* Core Cortex-M3 Exceptions */
    .weak NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler

    .weak MemManage_Handler
    .thumb_set MemManage_Handler,Default_Handler

    .weak BusFault_Handler
    .thumb_set BusFault_Handler,Default_Handler

    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler,Default_Handler

    .weak SVC_Handler
    .thumb_set SVC_Handler,Default_Handler

    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler,Default_Handler

    .weak PendSV_Handler
    .thumb_set PendSV_Handler,Default_Handler

    .weak SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler

/* LPC1343-Specific Interrupts */
    .weak WAKE_UP0_IRQHandler
    .thumb_set WAKE_UP0_IRQHandler,Default_Handler

    .weak WAKE_UP1_IRQHandler
    .thumb_set WAKE_UP1_IRQHandler,Default_Handler

    .weak WAKE_UP2_IRQHandler
    .thumb_set WAKE_UP2_IRQHandler,Default_Handler

    .weak WAKE_UP3_IRQHandler
    .thumb_set WAKE_UP3_IRQHandler,Default_Handler

    .weak WAKE_UP4_IRQHandler
    .thumb_set WAKE_UP4_IRQHandler,Default_Handler

    .weak WAKE_UP5_IRQHandler
    .thumb_set WAKE_UP5_IRQHandler,Default_Handler

    .weak WAKE_UP6_IRQHandler
    .thumb_set WAKE_UP6_IRQHandler,Default_Handler

    .weak WAKE_UP7_IRQHandler
    .thumb_set WAKE_UP7_IRQHandler,Default_Handler

    .weak WAKE_UP8_IRQHandler
    .thumb_set WAKE_UP8_IRQHandler,Default_Handler

    .weak WAKE_UP9_IRQHandler
    .thumb_set WAKE_UP9_IRQHandler,Default_Handler

    .weak WAKE_UP10_IRQHandler
    .thumb_set WAKE_UP10_IRQHandler,Default_Handler

    .weak WAKE_UP11_IRQHandler
    .thumb_set WAKE_UP11_IRQHandler,Default_Handler

    .weak WAKE_UP12_IRQHandler
    .thumb_set WAKE_UP12_IRQHandler,Default_Handler

    .weak WAKE_UP13_IRQHandler
    .thumb_set WAKE_UP13_IRQHandler,Default_Handler

    .weak WAKE_UP14_IRQHandler
    .thumb_set WAKE_UP14_IRQHandler,Default_Handler

    .weak WAKE_UP15_IRQHandler
    .thumb_set WAKE_UP15_IRQHandler,Default_Handler

    .weak WAKE_UP16_IRQHandler
    .thumb_set WAKE_UP16_IRQHandler,Default_Handler

    .weak WAKE_UP17_IRQHandler
    .thumb_set WAKE_UP17_IRQHandler,Default_Handler

    .weak WAKE_UP18_IRQHandler
    .thumb_set WAKE_UP18_IRQHandler,Default_Handler

    .weak WAKE_UP19_IRQHandler
    .thumb_set WAKE_UP19_IRQHandler,Default_Handler

    .weak WAKE_UP20_IRQHandler
    .thumb_set WAKE_UP20_IRQHandler,Default_Handler

    .weak WAKE_UP21_IRQHandler
    .thumb_set WAKE_UP21_IRQHandler,Default_Handler

    .weak WAKE_UP22_IRQHandler
    .thumb_set WAKE_UP22_IRQHandler,Default_Handler

    .weak WAKE_UP23_IRQHandler
    .thumb_set WAKE_UP23_IRQHandler,Default_Handler

    .weak WAKE_UP24_IRQHandler
    .thumb_set WAKE_UP24_IRQHandler,Default_Handler

    .weak WAKE_UP25_IRQHandler
    .thumb_set WAKE_UP25_IRQHandler,Default_Handler

    .weak WAKE_UP26_IRQHandler
    .thumb_set WAKE_UP26_IRQHandler,Default_Handler

    .weak WAKE_UP27_IRQHandler
    .thumb_set WAKE_UP27_IRQHandler,Default_Handler

    .weak WAKE_UP28_IRQHandler
    .thumb_set WAKE_UP28_IRQHandler,Default_Handler

    .weak WAKE_UP29_IRQHandler
    .thumb_set WAKE_UP29_IRQHandler,Default_Handler

    .weak WAKE_UP30_IRQHandler
    .thumb_set WAKE_UP30_IRQHandler,Default_Handler

    .weak WAKE_UP31_IRQHandler
    .thumb_set WAKE_UP31_IRQHandler,Default_Handler

    .weak WAKE_UP32_IRQHandler
    .thumb_set WAKE_UP32_IRQHandler,Default_Handler

    .weak WAKE_UP33_IRQHandler
    .thumb_set WAKE_UP33_IRQHandler,Default_Handler

    .weak WAKE_UP34_IRQHandler
    .thumb_set WAKE_UP34_IRQHandler,Default_Handler

    .weak WAKE_UP35_IRQHandler
    .thumb_set WAKE_UP35_IRQHandler,Default_Handler

    .weak WAKE_UP36_IRQHandler
    .thumb_set WAKE_UP36_IRQHandler,Default_Handler

    .weak WAKE_UP37_IRQHandler
    .thumb_set WAKE_UP37_IRQHandler,Default_Handler

    .weak WAKE_UP38_IRQHandler
    .thumb_set WAKE_UP38_IRQHandler,Default_Handler

    .weak WAKE_UP39_IRQHandler
    .thumb_set WAKE_UP39_IRQHandler,Default_Handler

    .weak I2C0_IRQHandler
    .thumb_set I2C0_IRQHandler,Default_Handler

    .weak CT16B0_IRQHandler
    .thumb_set CT16B0_IRQHandler,Default_Handler

    .weak CT16B1_IRQHandler
    .thumb_set CT16B1_IRQHandler,Default_Handler

    .weak CT32B0_IRQHandler
    .thumb_set CT32B0_IRQHandler,Default_Handler

    .weak CT32B1_IRQHandler
    .thumb_set CT32B1_IRQHandler,Default_Handler

    .weak SSP0_IRQHandler
    .thumb_set SSP0_IRQHandler,Default_Handler

    .weak UART0_IRQHandler
    .thumb_set UART0_IRQHandler,Default_Handler

    .weak USBIRQ_IRQHandler
    .thumb_set USBIRQ_IRQHandler,Default_Handler

    .weak USBFIQ_IRQHandler
    .thumb_set USBFIQ_IRQHandler,Default_Handler

    .weak ADC_IRQHandler
    .thumb_set ADC_IRQHandler,Default_Handler

    .weak WDT_IRQHandler
    .thumb_set WDT_IRQHandler,Default_Handler

    .weak BOD_IRQHandler
    .thumb_set BOD_IRQHandler,Default_Handler

    .weak PIO3_IRQHandler
    .thumb_set PIO3_IRQHandler,Default_Handler

    .weak PIO2_IRQHandler
    .thumb_set PIO2_IRQHandler,Default_Handler

    .weak PIO1_IRQHandler
    .thumb_set PIO1_IRQHandler,Default_Handler

    .weak PIO0_IRQHandler
    .thumb_set PIO0_IRQHandler,Default_Handler
