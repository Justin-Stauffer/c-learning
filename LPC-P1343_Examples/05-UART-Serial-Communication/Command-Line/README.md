# Command-Line

Chapter 5: UART/Serial Communication - Command Line Interface Example

## What This Example Demonstrates

- Line-based input with echo
- Backspace handling for editing
- String comparison functions
- Command parsing and dispatch
- Interactive LED control
- Building a simple CLI

## Hardware

- P1.6: UART RXD (connect to USB-Serial adapter TXD)
- P1.7: UART TXD (connect to USB-Serial adapter RXD)
- P3.0-P3.3: LEDs (controlled by commands)
- USB-to-Serial adapter required

## Terminal Settings

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None
- Local echo: OFF

## Building and Flashing

```bash
make clean
make
make flash
```

## Available Commands

| Command | Description |
|---------|-------------|
| `help` or `?` | Show available commands |
| `led on` | Turn all LEDs on |
| `led off` | Turn all LEDs off |
| `led 0 on` | Turn LED0 on |
| `led 0 off` | Turn LED0 off |
| `led 1 on` | Turn LED1 on |
| ... | (same for LED 2, 3) |
| `blink` | Blink all LEDs 5 times |
| `chase` | Run LED chase pattern |
| `status` | Show system status |

## Expected Behavior

```
====================================
LPC1343 Command Line Interface
====================================
Type 'help' for available commands.

> help

Available Commands:
  help        - Show this help message
  led on      - Turn all LEDs on
  led off     - Turn all LEDs off
  led 0-3 on  - Turn specific LED on
  led 0-3 off - Turn specific LED off
  blink       - Blink all LEDs 5 times
  status      - Show system status
  chase       - LED chase pattern

> led on
All LEDs ON
> led 2 off
LED2 OFF
> status

=== System Status ===
MCU: LPC1343
Clock: 72 MHz
UART: 115200 baud, 8N1
LEDs: P3.0-P3.3 (active-low)
LED States: 0=1 1=1 2=0 3=1

>
```

## Code Highlights

**Line input with backspace support:**
```c
uint32_t uart_getline(char *buf, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1) {
        char c = uart_getchar();

        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            break;
        }

        if (c == '\b' || c == 0x7F) {  /* Backspace or DEL */
            if (i > 0) {
                i--;
                uart_puts("\b \b");  /* Erase on terminal */
            }
            continue;
        }

        uart_putchar(c);  /* Echo */
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}
```

**String comparison (no stdlib):**
```c
int str_equal(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 0;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}
```

**Command dispatch pattern:**
```c
void process_command(char *cmd) {
    if (str_equal(cmd, "help")) {
        /* ... */
    }
    else if (str_equal(cmd, "led on")) {
        led_all(1);
        uart_puts("All LEDs ON\r\n");
    }
    else if (str_startswith(cmd, "led ")) {
        /* Parse LED number and state */
    }
    else {
        uart_puts("Unknown command\r\n");
    }
}
```

## Key Concepts

1. **Line Buffering**: Accumulate characters until Enter is pressed
2. **Backspace Handling**: `\b \b` sequence erases character on terminal
3. **No Standard Library**: Custom `str_equal()` instead of `strcmp()`
4. **Prefix Matching**: `str_startswith()` for commands with arguments
5. **Escape Handling**: ESC key cancels current line

## Terminal Control Sequences

| Sequence | Effect |
|----------|--------|
| `\r\n` | Carriage return + line feed (new line) |
| `\b \b` | Backspace, space, backspace (erase char) |
| `0x1B` | ESC character (cancel line) |

## Extending the CLI

To add new commands:
1. Add the command name check in `process_command()`
2. Implement the command behavior
3. Add help text in the help handler
