# -------------------------------------------------------------------------
# Filename   :   Makefile.inc
# Version    :   v1.0
# Function   :   Generic  Makfile Template
# -------------------------------------------------------------------------

################################ Target Variables #########################
TOPDIR ?= $(shell pwd)
#************************ Sources
C_SRC +=
CPP_SRC +=
ASM_SRC +=
#************************ Include
INCLUDE_DIR += \
$(TOPDIR)/imx6ul \
$(TOPDIR)/bsp/beep \
$(TOPDIR)/bsp/clk \
$(TOPDIR)/bsp/delay \
$(TOPDIR)/bsp/gpio \
$(TOPDIR)/bsp/key \
$(TOPDIR)/bsp/led \
# Includes
INCLUDES += $(patsubst %,-I%,$(INCLUDE_DIR))

#************************ Macros
MACROS += \

# Macro definitions
MACRO_DEFS += $(patsubst %,-D%,$(MACROS))

# Build directory
BUILD_DIR = $(TOPDIR)/build
LIB_DIR = $(TOPDIR)/out

############################## Toolchain Variables ########################
#************************ Make flags
# Quiet mode
Q ?=
# Debug mode (verbose print message, with debug info)
DEBUG = 1
# Dump disassembly flags
DUMP_FLAG = 1
# Hex file output
HEX_FLAG = 1
# Bin file output
BIN_FLAG = 1

#************************ Toolchain
BINPATH = $(dir $(shell which arm-linux-gnueabihf-gcc))
PREFIX = arm-linux-gnueabihf-
MKDIR = mkdir -p
RM = rm -rf
MAKE = make
FIND = find . -type f -name
# Compile tools
CC = $(BINPATH)/$(PREFIX)gcc
CPP = $(BINPATH)/$(PREFIX)g++
AS = $(BINPATH)/$(PREFIX)gcc -x assembler-with-cpp
OBJDUMP = $(BINPATH)/$(PREFIX)objdump -hdC
OBJCOPY = $(BINPATH)/$(PREFIX)objcopy
AR = $(BINPATH)/$(PREFIX)ar -cvsr
SZ = $(BINPATH)/$(PREFIX)size
HEX = $(OBJCOPY) -O ihex
BIN = $(OBJCOPY) -O binary -S

################################ Compile Flags ############################
#************************ Optimization
# Optimization level
OPT ?= -O2
EXTRA_OPT += -ffunction-sections -fdata-sections
EXTRA_OPT += -nostartfiles

OPT_FLAGS += $(OPT) $(EXTRA_OPT)
ifeq ($(DEBUG), 1)
OPT_FLAGS += -g
endif

#************************ Architecture
# cpu
CPU = -mcpu=cortex-a7
# fpu
FPU =
# float-abi
FLOAT_ABI =
# mcu
MCU = $(CPU) $(FPU) $(FLOAT_ABI)

#************************ Warning Flags
WARN_FLAGS = -Wall

#************************ Compiler Falgs
# Compiler gcc flags
ASMFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)
CFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)
CPPFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)

#************************ Dependency Flags
DEP_FLAG = -MM -MP -MT"$@ $(@:%.d=%.o)" $(MACRO_DEFS) $(INCLUDES)

#************************ Linker Flags
# Link script
LDSCRIPT = $(wildcard $(TOPDIR)/*.ld)
# LDFLAGS
LDFLAGS += $(MCU) $(OPT_FLAGS) $(WARN_FLAGS)
# Libraries
LIBS = -lc -lm
# Library path
LIBDIR = -L$(TOPDIR)/lib
LDFLAGS += -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,--build-id=none \
           -Wl,-Map=$(TARGET).map,--cref -Wl,--gc-sections

#************************ Linker Flags
# List of asm objects
ASM_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SRC:.s=.o)))
# List of c objects
C_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(C_SRC:.c=.o)))
# List of cpp objects
CPP_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SRC:.cpp=.o)))
# append vpath
vpath %s $(addprefix :,$(dir $(ASM_SRC)))
vpath %c $(addprefix :,$(dir $(C_SRC)))
vpath %cpp $(addprefix :,$(dir $(CPP_SRC)))
# List of all objects
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS)
# Dependency
DEPENDS = $(OBJECTS:.o=.d)
# Subdirectory
SUBDIR = $(filter-out .,$(patsubst %/,%,$(dir $(shell $(FIND) Makefile))))

TARGET := $(if $(TARGET),$(BUILD_DIR)/$(TARGET))
LIB := $(if $(LIB),$(if $(findstring Linux,$(shell uname)),\
       $(LIB_DIR)/lib$(LIB).a,$(LIB_DIR)/lib$(LIB).dll))

################################ Dependency Rule ##########################
# Default action: build all
all: $(TARGET) $(LIB) $(OBJECTS)

# Build subdir
$(BUILD_DIR)/subdir: $(SUBDIR) | $(BUILD_DIR)
	@echo "Build subdir"
	@for dir in $(SUBDIR); do \
                $(MAKE) all -C $${dir} || exit 1; \
            done
	@echo > $@

# Build objects
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo "\033[1;33m    CC $<\033[0m"
	$(Q)$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	@echo "\033[1;33m    CPP $<\033[0m"
	$(Q)$(CPP) -c $(CPPFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "\033[1;33m    ASM $<\033[0m"
	$(Q)$(AS) -c $(ASMFLAGS) $< -o $@

# Link library
$(LIB): $(BUILD_DIR)/subdir $(OBJECTS) Makefile | $(LIB_DIR)
	@echo "\033[1;35m    Linking... \033[0m"
	$(Q)$(AR) $@ $(BUILD_DIR)/*.o

# Link target
$(TARGET): $(BUILD_DIR)/subdir $(OBJECTS) Makefile | $(BUILD_DIR)
	@echo "\033[1;35m    Linking... \033[0m"
	$(Q)$(CC) $(BUILD_DIR)/*.o $(LDFLAGS) -o $@
	$(Q)$(SZ) $@
	$(Q)$(if $(DUMP_FLAG), $(OBJDUMP) $@ > $(TARGET).asm)
	$(Q)$(if $(HEX_FLAG), $(HEX) $@ $(TARGET).hex)
	$(Q)$(if $(BIN_FLAG), $(BIN) $@ $(TARGET).bin)
	@echo "\033[1;35mCompleted!\033[0m"

link:
	@echo "\033[1;35m    Linking... \033[0m"
	$(Q)$(CC) $(BUILD_DIR)/*.o $(LDFLAGS) -o $@
	$(Q)$(SZ) $@
	$(Q)$(if $(DUMP_FLAG), $(OBJDUMP) $@ > $(TARGET).asm)
	$(Q)$(if $(HEX_FLAG), $(HEX) $@ $(TARGET).hex)
	$(Q)$(if $(BIN_FLAG), $(BIN) $@ $(TARGET).bin)
	@echo "\033[1;35mCompleted!\033[0m"

# Build dependencies
$(BUILD_DIR)/%.d: %.s Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR)/%.d: %.c Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR)/%.d: %.cpp Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR):
	@echo "\033[1;35m    Compiling \033[0m"
	@$(MKDIR) $@

$(LIB_DIR):
	@$(MKDIR) $@

.PHONY: clean install link print

print:
	@echo $(TARGET)
	@echo $(LIB)
	@echo $(INCLUDE_DIR)

install: $(LIB)
	@echo "\033[1;35m    Install to sd card \033[0m"
	@./imxdownload $(TARGET).bin /dev/sdc

# Clean up
clean:
	@echo "\033[1;31m    Clean up all \033[0m"
	$(Q)$(RM) $(BUILD_DIR) $(LIB_DIR)

# Source file dependencies
-include $(DEPENDS)

# *** EOF ***
