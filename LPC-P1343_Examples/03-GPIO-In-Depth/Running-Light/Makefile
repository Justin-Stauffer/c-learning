######################################################
# Makefile for LPC1343 Running Light Example
# Chapter 3: GPIO In-Depth
# GCC ARM Toolchain
######################################################

# Project name (output files will be named after this)
PROJECT = lpc1343_running_light

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
C_SOURCES = main.c
ASM_SOURCES = startup_lpc1343_gcc.s

######################################################
# CPU Configuration
######################################################
CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

######################################################
# Compiler Flags
######################################################
OPT = -O2
WARNINGS = -Wall -Wextra
CFLAGS = $(MCU) $(OPT) $(WARNINGS)
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
ASFLAGS = $(MCU) $(WARNINGS) -fdata-sections -ffunction-sections

######################################################
# Linker Flags
######################################################
LDSCRIPT = lpc1343_flash.ld
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS)
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref
LDFLAGS += -Wl,--gc-sections

######################################################
# Build Rules
######################################################
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin
	@echo ""
	@echo "Build complete!"
	@echo "  ELF: $(BUILD_DIR)/$(PROJECT).elf"
	@echo "  HEX: $(BUILD_DIR)/$(PROJECT).hex"
	@echo "  BIN: $(BUILD_DIR)/$(PROJECT).bin"

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "CC    $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "AS    $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	@echo "LD    $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo ""
	@echo "=== Memory Usage ==="
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo "HEX   $@"
	@$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "BIN   $@"
	@$(CP) -O binary -S $< $@

$(BUILD_DIR):
	@mkdir -p $@

######################################################
# Utility Targets
######################################################
size: $(BUILD_DIR)/$(PROJECT).elf
	@$(SZ) --format=sysv $<
	@$(SZ) --format=berkeley $<

disasm: $(BUILD_DIR)/$(PROJECT).elf
	@$(OD) -d -S $< > $(BUILD_DIR)/$(PROJECT).lst

clean:
	@rm -rf $(BUILD_DIR)

######################################################
# Flash Targets
######################################################
flash: $(BUILD_DIR)/$(PROJECT).elf
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
		-c "program $< reset exit"

reset:
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg \
		-c "init" -c "reset run" -c "exit"

######################################################
# Dependencies
######################################################
-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean size disasm flash reset
