######################################################
# Makefile for LPC1343 LED-Dimmer Example
# Chapter 4: Timers and PWM
######################################################

PROJECT = lpc1343_led_dimmer
BUILD_DIR = build

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

C_SOURCES = main.c
ASM_SOURCES = startup_lpc1343_gcc.s

CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb

OPT = -O2
CFLAGS = $(MCU) $(OPT) -Wall -Wextra -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
ASFLAGS = $(MCU) -Wall -Wextra -fdata-sections -ffunction-sections

LDSCRIPT = lpc1343_flash.ld
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -lc -lm -lnosys
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref -Wl,--gc-sections

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin
	@echo "Build complete: $(PROJECT)"

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@$(CP) -O binary -S $< $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

flash: $(BUILD_DIR)/$(PROJECT).elf
	openocd -f interface/stlink.cfg -f target/lpc13xx.cfg -c "program $< reset exit"

.PHONY: all clean flash
-include $(wildcard $(BUILD_DIR)/*.d)
