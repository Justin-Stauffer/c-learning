# SysTick-Blink

Chapter 6: Interrupts and Clocks - SysTick Timer Example

## What This Example Demonstrates

- ARM Cortex-M3 SysTick timer configuration
- SysTick interrupt handler (`SysTick_Handler`)
- Precise millisecond timing
- Blocking delay function
- Non-blocking timeout check

## Hardware

- P3.0-P3.3: LEDs (active-low)

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. **Startup**: All LEDs flash once
2. **Pattern 1**: LED0 blinks at 1Hz (500ms on/off)
3. **Pattern 2**: LED1 blinks fast 5 times (100ms)
4. **Pattern 3**: LEDs light up sequentially, then turn off in reverse
5. **Pattern 4**: All LEDs blink together 3 times
6. Repeat from Pattern 1

## Code Highlights

**SysTick initialization:**
```c
void systick_init(void) {
    /* Reload = 72,000,000 / 1000 - 1 = 71,999 for 1ms */
    SYST_RVR = (SYSTEM_CLOCK / 1000) - 1;
    SYST_CVR = 0;
    SYST_CSR = 0x07;  /* Enable, interrupt, CPU clock */
}
```

**SysTick interrupt handler:**
```c
volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}
```

**Blocking delay:**
```c
void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}
```

**Non-blocking timeout:**
```c
uint8_t timeout_elapsed(uint32_t start, uint32_t timeout_ms) {
    return (ms_ticks - start) >= timeout_ms;
}
```

## SysTick Registers

| Register | Address | Description |
|----------|---------|-------------|
| SYST_CSR | 0xE000E010 | Control and Status |
| SYST_RVR | 0xE000E014 | Reload Value (24-bit) |
| SYST_CVR | 0xE000E018 | Current Value |

## SYST_CSR Bits

| Bit | Name | Description |
|-----|------|-------------|
| 0 | ENABLE | Counter enable |
| 1 | TICKINT | Interrupt on count to 0 |
| 2 | CLKSOURCE | 0=external, 1=CPU clock |
| 16 | COUNTFLAG | Set when counter reaches 0 |

## Why Use SysTick?

- Built into ARM Cortex-M3 core (always available)
- Leaves peripheral timers (CT16B0, CT32B0, etc.) free for other uses
- Standardized across all Cortex-M devices
- Simple 24-bit countdown timer
- Ideal for system "heartbeat" or RTOS tick

## Maximum Interval

At 72 MHz, the 24-bit reload allows:
- Max reload = 16,777,215
- Max interval = 16,777,215 / 72,000,000 = 0.233 seconds

For longer delays, use a peripheral timer or count multiple SysTick intervals.
