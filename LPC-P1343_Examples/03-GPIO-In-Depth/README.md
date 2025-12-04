# Chapter 3: GPIO In-Depth - Example Projects

This folder contains multiple example projects demonstrating GPIO concepts from Chapter 3.

## Example Projects

| Folder | Description | Concepts Demonstrated |
|--------|-------------|----------------------|
| [Running-Light](./Running-Light/) | LED chaser/Knight Rider effect | GPIO output, bit shifting, patterns |
| [Binary-Counter](./Binary-Counter/) | 4-bit counter displayed on LEDs | GPIO output, binary representation |
| [Button-Patterns](./Button-Patterns/) | Button cycles through LED patterns | GPIO input, interrupts, debouncing |
| [Combination-Lock](./Combination-Lock/) | 4-button combination lock | GPIO input, state machines, sequences |

## Building the Examples

Each subfolder is a complete project. To build any example:

```bash
cd <example-folder>
make clean
make
make flash
```

## Hardware Requirements

All examples use:
- **LEDs**: P3.0, P3.1, P3.2, P3.3 (directly on LPC-P1343 board)
- **Button**: P0.1 (directly on LPC-P1343 board)

Some examples (Button-Patterns, Combination-Lock) can use additional buttons if available.

## Progression

Work through these examples in order:

1. **Running-Light** - Start here to understand basic GPIO output
2. **Binary-Counter** - Practice GPIO output with bit patterns
3. **Button-Patterns** - Learn GPIO input and interrupts
4. **Combination-Lock** - Combine input/output with state machine logic

## Related Documentation

- [Chapter 3: GPIO In-Depth](../../learning-docs/03-gpio-in-depth.md)
