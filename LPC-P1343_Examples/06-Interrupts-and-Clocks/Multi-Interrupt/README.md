# Multi-Interrupt

Chapter 6: Interrupts and Clocks - Multiple Interrupt Sources Example

## What This Example Demonstrates

- Multiple interrupt handlers running concurrently
- SysTick timer interrupt
- CT32B0 and CT32B1 peripheral timer interrupts
- NVIC configuration for multiple sources
- Independent timing on different LEDs
- CPU sleeping between interrupts (low power)

## Hardware

- P3.0: LED0 - controlled by CT32B0 (250ms toggle)
- P3.1: LED1 - controlled by CT32B1 (500ms toggle)
- P3.2: LED2 - controlled by SysTick (1000ms toggle)
- P3.3: LED3 - always on (main loop indicator)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

All LEDs blink at different rates simultaneously:

| LED | Rate | Source |
|-----|------|--------|
| LED0 | 250ms (4 Hz) | CT32B0 interrupt |
| LED1 | 500ms (2 Hz) | CT32B1 interrupt |
| LED2 | 1000ms (1 Hz) | SysTick interrupt |
| LED3 | Always on | Main loop |

Watch carefully - each LED operates independently. The main loop just sleeps while interrupts do all the work!

## Code Highlights

**Three interrupt handlers:**
```c
void SysTick_Handler(void) {
    systick_count++;
    if ((systick_count % 1000) == 0) {
        GPIO3DATA ^= (1 << 2);  /* Toggle LED2 */
    }
}

void CT32B0_IRQHandler(void) {
    if (TMR32B0IR & 0x01) {
        TMR32B0IR = 0x01;       /* Clear flag */
        GPIO3DATA ^= (1 << 0);  /* Toggle LED0 */
    }
}

void CT32B1_IRQHandler(void) {
    if (TMR32B1IR & 0x01) {
        TMR32B1IR = 0x01;       /* Clear flag */
        GPIO3DATA ^= (1 << 1);  /* Toggle LED1 */
    }
}
```

**NVIC enable for multiple sources:**
```c
NVIC_ISER = (1 << 18);  /* CT32B0 */
NVIC_ISER = (1 << 19);  /* CT32B1 */
/* SysTick doesn't need NVIC_ISER - enabled via SYST_CSR */
```

**Main loop sleeps:**
```c
while (1) {
    __WFI();  /* Wait For Interrupt - CPU sleeps */
}
```

## Interrupt Sources Used

| Source | IRQ # | Handler | Purpose |
|--------|-------|---------|---------|
| SysTick | N/A | SysTick_Handler | 1ms system tick |
| CT32B0 | 18 | CT32B0_IRQHandler | 250ms LED0 toggle |
| CT32B1 | 19 | CT32B1_IRQHandler | 500ms LED1 toggle |

## Timer Configuration

Both CT32B0 and CT32B1 use:
- Prescaler = 71 (72MHz / 72 = 1MHz, 1µs per tick)
- Match interrupt + reset mode
- Different match values for different periods

```c
/* CT32B0: 250ms */
TMR32B0PR = 71;
TMR32B0MR0 = 250000 - 1;

/* CT32B1: 500ms */
TMR32B1PR = 71;
TMR32B1MR0 = 500000 - 1;
```

## Key Concepts

1. **Independent Timing**: Each interrupt source maintains its own timing
2. **Concurrent Operation**: All interrupts can fire independently
3. **Flag Clearing**: Each handler must clear its own interrupt flag
4. **Low Power**: Main loop uses `__WFI()` to sleep between interrupts
5. **NVIC**: Each peripheral interrupt needs to be enabled in NVIC
