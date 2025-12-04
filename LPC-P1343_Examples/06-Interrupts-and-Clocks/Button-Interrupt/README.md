# Button-Interrupt

Chapter 6: Interrupts and Clocks - Button Interrupt Example

## What This Example Demonstrates

- GPIO interrupt configuration for button input
- Edge-triggered interrupt detection (falling edge)
- NVIC enable for GPIO port interrupts
- Software debouncing using SysTick timer
- Interrupt-driven button handling (no polling)

## Hardware

- P0.1: Button (active-low, directly connected)
- P3.0: LED0 - toggles on each press
- P3.1: LED1 - bit 0 of press counter
- P3.2: LED2 - bit 1 of press counter
- P3.3: LED3 - bit 2 of press counter

## Building and Flashing

```bash
make clean
make
make flash
```

## Expected Behavior

1. All LEDs flash briefly on startup
2. Press the button to toggle LED0 and increment counter
3. LED1-LED3 show 3-bit binary count of presses:

| Presses | LED3 | LED2 | LED1 |
|---------|------|------|------|
| 0       | off  | off  | off  |
| 1       | off  | off  | ON   |
| 2       | off  | ON   | off  |
| 3       | off  | ON   | ON   |
| 4       | ON   | off  | off  |
| 5       | ON   | off  | ON   |
| 6       | ON   | ON   | off  |
| 7       | ON   | ON   | ON   |
| 8       | off  | off  | off  | (wraps)

## Code Highlights

**GPIO interrupt configuration:**
```c
/* Configure interrupt:
 * GPIO0IS  = 0: Edge sensitive (not level)
 * GPIO0IBE = 0: Single edge (not both edges)
 * GPIO0IEV = 0: Falling edge (button press pulls low)
 */
GPIO0IS  &= ~BUTTON_PIN;  /* Edge sensitive */
GPIO0IBE &= ~BUTTON_PIN;  /* Single edge */
GPIO0IEV &= ~BUTTON_PIN;  /* Falling edge */

/* Enable interrupt for this pin */
GPIO0IE |= BUTTON_PIN;

/* Enable in NVIC (PIO0 is IRQ 31) */
NVIC_ISER = (1 << 31);
```

**Interrupt handler with debouncing:**
```c
void PIO0_IRQHandler(void) {
    if (GPIO0MIS & BUTTON_PIN) {
        GPIO0IC = BUTTON_PIN;  /* Clear interrupt */

        /* Debounce: ignore if too soon */
        if ((systick_count - last_press_time) > DEBOUNCE_MS) {
            last_press_time = systick_count;
            button_count++;
            GPIO3DATA ^= (1 << 0);  /* Toggle LED0 */
        }
    }
}
```

## GPIO Interrupt Registers

| Register | Address | Purpose |
|----------|---------|---------|
| GPIO0IS  | 0x50008004 | Interrupt sense (0=edge, 1=level) |
| GPIO0IBE | 0x50008008 | Both edges (0=single, 1=both) |
| GPIO0IEV | 0x5000800C | Event (0=fall/low, 1=rise/high) |
| GPIO0IE  | 0x50008010 | Interrupt enable mask |
| GPIO0RIS | 0x50008014 | Raw interrupt status |
| GPIO0MIS | 0x50008018 | Masked interrupt status |
| GPIO0IC  | 0x5000801C | Interrupt clear (write 1 to clear) |

## Key Concepts

1. **Edge vs Level**: Edge-triggered fires once per transition; level-triggered fires continuously while condition is true
2. **Falling Edge**: Detects high-to-low transition (button press)
3. **Interrupt Clear**: Must write 1 to GPIO0IC to clear the pending interrupt
4. **Software Debounce**: Use SysTick to track time and ignore bounces
5. **NVIC Enable**: GPIO Port 0 interrupt is IRQ 31
