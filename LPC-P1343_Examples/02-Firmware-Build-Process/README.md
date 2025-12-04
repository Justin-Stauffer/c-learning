# Chapter 2: Firmware Build Process

This chapter focuses on understanding the build process rather than introducing new code.

## No Dedicated Example Project

Chapter 2 explains how source code becomes firmware using the examples from:

- **[00-Getting-Started](../00-Getting-Started/)** - The primary example used throughout Chapter 2
- **[01-Bitwise-Operations](../01-Bitwise-Operations/)** - Additional reference for build concepts

## What Chapter 2 Covers

1. **Compilation** - How C code becomes object files
2. **Assembly** - How startup code is assembled
3. **Linking** - How object files combine into an executable
4. **Output Formats** - ELF, HEX, and BIN files
5. **Memory Layout** - Flash and RAM organization
6. **Programming Methods** - USB bootloader, debug probe, serial bootloader

## Hands-On Activities

To follow along with Chapter 2, use the Getting-Started example:

```bash
cd ../00-Getting-Started

# Build and observe the output
make clean
make

# Examine the memory usage
make size

# Generate disassembly to see compiled code
make disasm

# View the map file
cat build/lpc1343_getting_started.map
```

## Key Files to Study

In the `00-Getting-Started` folder:

| File | Purpose |
|------|---------|
| `main.c` | Your application code |
| `startup_lpc1343_gcc.s` | Startup/initialization code |
| `lpc1343_flash.ld` | Linker script (memory layout) |
| `Makefile` | Build automation |
| `build/*.elf` | Executable with debug info |
| `build/*.bin` | Raw binary for flashing |
| `build/*.map` | Memory map (symbol locations) |

## Related Documentation

- [Chapter 2: Firmware Build Process](../../learning-docs/02-firmware-build-process.md)
- [BUILD_INSTRUCTIONS_GCC.md](../../learning-docs/BUILD_INSTRUCTIONS_GCC.md)
