######################################################
# Makefile for LPC1343 Project
# GCC ARM Toolchain
######################################################

# Project name
PROJECT = lpc1343_blink

# Build directory
BUILD_DIR = build

######################################################
# Toolchain
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

# ASM sources
ASM_SOURCES = startup_lpc1343_gcc.s

######################################################
# Include Directories
######################################################
C_INCLUDES =

######################################################
# CPU Configuration
######################################################
CPU = -mcpu=cortex-m3

# FPU (none for Cortex-M3)
FPU =

# Float ABI
FLOAT-ABI =

# MCU
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

######################################################
# Compiler Flags
######################################################
# C defines
C_DEFS =

# Compile flags
ASFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

######################################################
# Linker Flags
######################################################
# Linker script
LDSCRIPT = lpc1343_flash.ld

# Libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections

######################################################
# Build Targets
######################################################
# default action: build all
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

######################################################
# Build Objects
######################################################
# list of C objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# list of ASM objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "Compiling C: $<"
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "Assembling: $<"
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	@echo "Linking: $@"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo ""
	@echo "=== Build Summary ==="
	$(SZ) $@
	@echo "====================="

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating HEX: $@"
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating BIN: $@"
	$(CP) -O binary -S $< $@

$(BUILD_DIR):
	mkdir $@

######################################################
# Additional Targets
######################################################
# Print size information
size: $(BUILD_DIR)/$(PROJECT).elf
	@echo ""
	@echo "=== Memory Usage ==="
	$(SZ) --format=berkeley $<
	@echo ""
	$(SZ) --format=sysv $<
	@echo "===================="

# Generate disassembly
disasm: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Generating disassembly..."
	$(OD) -D $(BUILD_DIR)/$(PROJECT).elf > $(BUILD_DIR)/$(PROJECT).lst
	@echo "Disassembly saved to $(BUILD_DIR)/$(PROJECT).lst"

# Clean build artifacts
clean:
	-rm -fR $(BUILD_DIR)
	@echo "Build directory cleaned"

# Program the device (adjust this based on your programmer)
# Example for lpc21isp via serial
flash-serial: $(BUILD_DIR)/$(PROJECT).bin
	@echo "Flashing via serial bootloader..."
	lpc21isp -bin $(BUILD_DIR)/$(PROJECT).bin COM3 115200 12000

# Example for OpenOCD with CMSIS-DAP
flash-openocd: $(BUILD_DIR)/$(PROJECT).bin
	@echo "Flashing via OpenOCD..."
	openocd -f interface/cmsis-dap.cfg -f target/lpc1343.cfg -c "program $(BUILD_DIR)/$(PROJECT).bin verify reset exit 0x00000000"

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build project (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  size       - Show memory usage details"
	@echo "  disasm     - Generate disassembly listing"
	@echo "  flash-serial    - Flash via serial bootloader (lpc21isp)"
	@echo "  flash-openocd   - Flash via OpenOCD"
	@echo "  help       - Show this help message"

######################################################
# Dependencies
######################################################
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean size disasm flash-serial flash-openocd help
