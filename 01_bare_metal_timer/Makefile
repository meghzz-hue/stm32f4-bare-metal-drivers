######################################
# Target
######################################
TARGET = 01_bare_metal_timer

######################################
# Building variables
######################################
# Debug build?
DEBUG = 1
# Optimization
OPT = -Og

#######################################
# Paths
#######################################
# Build path
BUILD_DIR = build

######################################
# Source
######################################
# C sources
C_SOURCES =  \
Core/Src/main.c \
Core/Src/tim_driver.c \
Core/Src/system_stm32f4xx.c

# ASM sources
ASM_SOURCES =  \
Core/Src/startup_stm32f401xe.s

#######################################
# Binaries
#######################################
PREFIX = arm-none-eabi-

# Verify toolchain is in PATH
ifeq ($(shell which $(PREFIX)gcc 2> /dev/null),)
$(error $(PREFIX)gcc not found in PATH. Install ARM GCC toolchain or add to PATH)
endif

CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
OBJDUMP = $(PREFIX)objdump

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# CPU
CPU = -mcpu=cortex-m4

# FPU
FPU = -mfpu=fpv4-sp-d16

# Float-abi
FLOAT-ABI = -mfloat-abi=hard

# MCU
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DSTM32F401xE \
-DUSE_FULL_ASSERT

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-ICore/Inc

# Compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# Link script
LDSCRIPT = Linker/STM32F401RETX_FLASH.ld

# Libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@echo "============================================"
	@echo "Build complete!"
	@echo "============================================"
	$(SZ) $(BUILD_DIR)/$(TARGET).elf
	@echo "============================================"
	@echo "Output files:"
	@echo "  - $(BUILD_DIR)/$(TARGET).elf"
	@echo "  - $(BUILD_DIR)/$(TARGET).hex"
	@echo "  - $(BUILD_DIR)/$(TARGET).bin"
	@echo "============================================"

#######################################
# Build the application
#######################################
# List of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# List of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo "Compiling: $<"
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "Assembling: $<"
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo "Linking: $(TARGET).elf"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "============================================"
	@echo "Memory usage:"
	@echo "============================================"
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating HEX: $(notdir $@)"
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@echo "Creating BIN: $(notdir $@)"
	$(BIN) $< $@	

$(BUILD_DIR):
	mkdir $@		

#######################################
# Clean
#######################################
clean:
	@echo "Cleaning build directory..."
	-rm -fR $(BUILD_DIR)
	@echo "Clean complete!"

#######################################
# Flash using ST-LINK
#######################################
flash: all
	@echo "============================================"
	@echo "Flashing to STM32F401RE..."
	@echo "============================================"
	st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x8000000
	@echo "Flash complete!"

#######################################
# Flash using OpenOCD
#######################################
flash-openocd: all
	@echo "============================================"
	@echo "Flashing via OpenOCD..."
	@echo "============================================"
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"
	@echo "Flash complete!"

#######################################
# Debug using OpenOCD + GDB
#######################################
debug:
	@echo "Starting OpenOCD server..."
	@echo "Connect with: arm-none-eabi-gdb $(BUILD_DIR)/$(TARGET).elf"
	@echo "Then run: target remote localhost:3333"
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

#######################################
# Disassembly
#######################################
disasm: $(BUILD_DIR)/$(TARGET).elf
	@echo "Creating disassembly..."
	$(OBJDUMP) -d -S $(BUILD_DIR)/$(TARGET).elf > $(BUILD_DIR)/$(TARGET).lst
	@echo "Disassembly saved to: $(BUILD_DIR)/$(TARGET).lst"

#######################################
# Info
#######################################
info:
	@echo "============================================"
	@echo "Project: $(TARGET)"
	@echo "============================================"
	@echo "MCU: STM32F401RETx"
	@echo "Core: ARM Cortex-M4"
	@echo "Flash: 512KB"
	@echo "RAM: 96KB"
	@echo "============================================"
	@echo "Source files:"
	@echo "$(C_SOURCES)" | tr ' ' '\n'
	@echo "$(ASM_SOURCES)"
	@echo "============================================"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Linker script: $(LDSCRIPT)"
	@echo "============================================"

#######################################
# Help
#######################################
help:
	@echo "============================================"
	@echo "Available targets:"
	@echo "============================================"
	@echo "  make              - Build project"
	@echo "  make all          - Build project (same as make)"
	@echo "  make clean        - Remove build files"
	@echo "  make flash        - Flash using st-flash"
	@echo "  make flash-openocd- Flash using OpenOCD"
	@echo "  make debug        - Start OpenOCD for debugging"
	@echo "  make disasm       - Generate disassembly listing"
	@echo "  make info         - Show project information"
	@echo "  make help         - Show this help"
	@echo "============================================"

#######################################
# Dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# Phony targets
.PHONY: all clean flash flash-openocd debug disasm info help


# *** EOF ***
