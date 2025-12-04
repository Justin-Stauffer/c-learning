######################################################
# Makefile for LPC1343 Bitwise Operations Example
# Chapter 1: Bitwise Operations
# GCC ARM Toolchain
#
# Usage:
#   make          - Build the project
#   make clean    - Remove build files
#   make flash    - Flash via OpenOCD + ST-Link
#   make size     - Show memory usage
#   make help     - Show all available targets
######################################################

# Project name (output files will be named after this)
PROJECT = lpc1343_bitwise

# Build directory
BUILD_DIR = build

######################################################
# Toolchain Configuration
######################################################
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
OD = $(PREFIX)objdump

######################################################
# Source Files
######################################################
# C sources
C_SOURCES = main.c

# Assembly sources (startup code)
ASM_SOURCES = startup_lpc1343_gcc.s

######################################################
# CPU Configuration
######################################################
# Cortex-M3 core, thumb instruction set
CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

######################################################
# Compiler Flags
######################################################
# Optimization level: -O0 (none), -O1, -O2, -O3, -Os (size)
OPT = -O2

# Warning flags
WARNINGS = -Wall -Wextra

# C flags
CFLAGS = $(MCU) $(OPT) $(WARNINGS)
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# Assembly flags
ASFLAGS = $(MCU) $(WARNINGS) -fdata-sections -ffunction-sections

######################################################
# Linker Flags
######################################################
# Linker script
LDSCRIPT = lpc1343_flash.ld

# Libraries
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS)
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref
LDFLAGS += -Wl,--gc-sections

######################################################
# Build Rules
######################################################

# Default target: build everything
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin
	@echo ""
	@echo "Build complete!"
	@echo "  ELF: $(BUILD_DIR)/$(PROJECT).elf"
	@echo "  HEX: $(BUILD_DIR)/$(PROJECT).hex"
	@echo "  BIN: $(BUILD_DIR)/$(PROJECT).bin"

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Compile C files
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "CC    $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# Assemble startup code
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "AS    $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	@echo "LD    $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo ""
	@echo "=== Memory Usage ==="
	@$(SZ) $@

# Create HEX file
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo "HEX   $@"
	@$(CP) -O ihex $< $@

# Create BIN file
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "BIN   $@"
	@$(CP) -O binary -S $< $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@

######################################################
# Utility Targets
######################################################

# Show detailed memory usage
size: $(BUILD_DIR)/$(PROJECT).elf
	@echo ""
	@echo "=== Detailed Memory Usage ==="
	@$(SZ) --format=sysv $<
	@echo ""
	@$(SZ) --format=berkeley $<

# Generate disassembly listing
disasm: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Generating disassembly..."
	@$(OD) -d -S $< > $(BUILD_DIR)/$(PROJECT).lst
	@echo "Saved to $(BUILD_DIR)/$(PROJECT).lst"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@echo "Done"

######################################################
# Flash Targets
######################################################

# Flash via OpenOCD with ST-Link
# This assumes you have openocd installed and ST-Link connected
# Note: We skip verify because OpenOCD fixes the LPC checksum during programming,
#       which causes verify to fail (checksum mismatch between file and flash)
flash: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Flashing via OpenOCD + ST-Link..."
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
		-c "program $< reset exit"

# Flash via OpenOCD (alternate command using bin file)
flash-bin: $(BUILD_DIR)/$(PROJECT).bin
	@echo "Flashing via OpenOCD + ST-Link..."
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
		-c "program $< verify reset exit 0x00000000"

# Just reset the target
reset:
	@echo "Resetting target..."
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
		-c "init" -c "reset run" -c "exit"

######################################################
# Help
######################################################
help:
	@echo "LPC1343 Bitwise Operations Example - Makefile Help"
	@echo ""
	@echo "Build targets:"
	@echo "  make          - Build the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make size     - Show detailed memory usage"
	@echo "  make disasm   - Generate disassembly listing"
	@echo ""
	@echo "Flash targets (requires OpenOCD + ST-Link):"
	@echo "  make flash    - Flash and run program"
	@echo "  make reset    - Reset the target"
	@echo ""
	@echo "Output files (in build/ directory):"
	@echo "  $(PROJECT).elf - Executable with debug info"
	@echo "  $(PROJECT).hex - Intel HEX format"
	@echo "  $(PROJECT).bin - Raw binary"
	@echo "  $(PROJECT).map - Memory map"

######################################################
# Dependencies
######################################################
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean size disasm flash flash-bin reset help
