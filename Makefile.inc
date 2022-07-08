# -------------------------------------------------------------------------
# Filename   :   Makefile.inc
# Version    :   v1.0
# Author     :   Penn
# Function   :   Generic  Makfile Template
# -------------------------------------------------------------------------


################################ Target Variables #########################
TOPDIR ?= $(shell pwd)
#************* Sources *************
C_SRC_DIR +=
CPP_SRC_DIR +=
ASM_SRC_DIR +=

C_SRC +=
CPP_SRC +=
ASM_SRC +=

C_SRC += $(foreach dir,$(C_SRC_DIR),$(shell find $(dir) -type f -name *.c))
CPP_SRC += $(foreach dir,$(CPP_SRC_DIR),$(shell find $(dir) -type f -name *.cpp))
ASM_SRC += $(foreach dir,$(ASM_SRC_DIR),$(shell find $(dir) -type f -name *.s))
#************* Include *************
INCLUDE_DIR += \

# Includes
INCLUDES += $(patsubst %,-I%,$(INCLUDE_DIR))
INCLUDES += $(patsubst %,-I%,$(dir $(C_SRC)))
INCLUDES += $(patsubst %,-I%,$(dir $(CPP_SRC)))
INCLUDES += $(patsubst %,-I%,$(dir $(ASM_SRC)))

#************* Macros *************
MACROS += \

# Macro definitions
MACRO_DEFS += $(patsubst %,-D%,$(MACROS))

# Build directory
BUILD_DIR ?= $(TOPDIR)/build


############################## Toolchain Variables ########################
#************* Make flags *************
# Quiet mode
Q_FLAG ?= 1
# Debug mode (verbose print message, with debug info)
DEBUG = 1
# Dump disassembly flags
DUMP_FLAG = 1
# Hex file output
HEX_FLAG = 1
# Bin file output
BIN_FLAG = 1

Q = $(if $(Q_FLAG),@)
export CORENUM ?= $(shell cat /proc/cpuinfo | grep "processor" | wc -l)
export BUILD_TIME ?= $(shell date +%Y%m%d%H%M%S)
ifeq ($(shell git rev-parse --is-inside-work-tree 2> /dev/null),true)
export COMMIT_ID ?= $(shell git rev-parse --short HEAD)
else
export COMMIT_ID ?= unknown
endif
MACROS += BUILD_INFO=$(COMMIT_ID)_$(BUILD_TIME)
#************* Toolchain *************
BINPATH ?= $(dir $(shell which arm-linux-gnueabihf-gcc))
PREFIX ?= arm-linux-gnueabihf-
MKDIR = mkdir -p
RM = rm -rf
MAKE = make
FIND = find . -type f -name
# Build tools
CC = $(BINPATH)/$(PREFIX)gcc
CPP = $(BINPATH)/$(PREFIX)g++
AS = $(BINPATH)/$(PREFIX)gcc -x assembler-with-cpp
OBJDUMP = $(BINPATH)/$(PREFIX)objdump -hdC
OBJCOPY = $(BINPATH)/$(PREFIX)objcopy
AR = $(BINPATH)/$(PREFIX)ar -cvsr
SZ = $(BINPATH)/$(PREFIX)size
HEX = $(OBJCOPY) -O ihex
BIN = $(OBJCOPY) -O binary -S


################################ Build Flags ##############################
#************* Optimization *************
# Optimization level
OPT ?= -O2
EXTRA_OPT += -ffunction-sections -fdata-sections
EXTRA_OPT +=

OPT_FLAGS += $(OPT) $(EXTRA_OPT)
OPT_FLAGS += $(FPIC_OPT)
OPT_FLAGS += $(if $(DEBUG),-g)

export FPIC_OPT ?= $(if $(DYNAMIC_LIB),-fPIC)
#************* Architecture *************
# cpu
CPU = -mcpu=cortex-a7
# fpu
FPU =
# float-abi
FLOAT_ABI =
# mcu
MCU = $(CPU) $(FPU) $(FLOAT_ABI)

#************* Warning Flags *************
WARN_FLAGS = -Wall

#************* Compiler Falgs *************
# Compiler gcc flags
ASMFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)
CFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)
CPPFLAGS += $(MCU) $(MACRO_DEFS) $(INCLUDES) $(OPT_FLAGS) $(WARN_FLAGS)

#************* Dependency Flags *************
DEP_FLAG = -MM -MP -MT"$@ $(@:%.d=%.o)" $(MACRO_DEFS) $(INCLUDES)

#************* Linker Flags *************
# Link script
LDSCRIPT = $(wildcard $(TOPDIR)/*.ld)
# LDFLAGS
LDFLAGS += $(MCU) $(OPT_FLAGS) $(WARN_FLAGS)
# Libraries
LIBS = -lc -lm
# Library path
LIBDIR = $(TOPDIR)/lib
LDFLAGS += $(LDSCRIPT:%=-T%) $(LIBDIR:%=-L%) $(LIBS) -Wl,--build-id=none \
           $(TARGET:%=-Wl,-Map=%.map,--cref -Wl,--gc-sections)

#************* Linker Flags *************
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
# Submodules
SUBDIR = $(filter-out .,$(patsubst %/,%,$(dir $(shell $(FIND) Makefile))))

# Executable
TARGET := $(if $(TARGET),$(BUILD_DIR)/$(TARGET))
# Dynamic library
DYNAMIC_LIB := $(if $(DYNAMIC_LIB),$(if $(findstring Linux,$(shell uname)),\
       $(BUILD_DIR)/lib$(DYNAMIC_LIB).so,$(BUILD_DIR)/lib$(DYNAMIC_LIB).dll))
# Static library
STATIC_LIB := $(if $(STATIC_LIB),$(if $(findstring Linux,$(shell uname)),\
       $(BUILD_DIR)/lib$(STATIC_LIB).a,$(BUILD_DIR)/lib$(STATIC_LIB).lib))


################################ Dependency Rule ##########################
# Default action: build all
all: $(TARGET) $(DYNAMIC_LIB) $(STATIC_LIB) $(OBJECTS)
.PHONY: all submodule clean cleanall install echo

# Build submodules
submodule:
	@for dir in $(SUBDIR); do \
	  echo -e "\033[1;36m    Building submodule... \033[0m"; \
	  $(MAKE) all -C $${dir} || exit 1; \
	done

# Build objects
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@echo -e "\033[1;33m    CC $<\033[0m"
	$(Q)$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	@echo -e "\033[1;33m    CXX $<\033[0m"
	$(Q)$(CPP) -c $(CPPFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo -e "\033[1;33m    ASM $<\033[0m"
	$(Q)$(AS) -c $(ASMFLAGS) $< -o $@

# Build library
$(STATIC_LIB): submodule $(OBJECTS) Makefile | $(BUILD_DIR)
	@echo -e "\033[1;36m    Archiving... \033[0m"
	$(Q)$(AR) $@ $(BUILD_DIR)/*.o

$(DYNAMIC_LIB): submodule $(OBJECTS) Makefile | $(BUILD_DIR)
	@echo -e "\033[1;36m    Linking... \033[0m"
	$(Q)$(CC) $(BUILD_DIR)/*.o $(LDFLAGS) -shared -o $@

# Build target
$(TARGET): submodule $(OBJECTS) Makefile | $(BUILD_DIR)
	@echo -e "\033[1;36m    Linking... \033[0m"
	$(Q)$(CC) $(BUILD_DIR)/*.o $(LDFLAGS) -o $@
	$(Q)$(if $(DUMP_FLAG),$(OBJDUMP) $@ > $(TARGET).asm)
	$(Q)$(if $(HEX_FLAG),$(HEX) $@ $(TARGET).hex)
	$(Q)$(if $(BIN_FLAG),$(BIN) $@ $(TARGET).bin)
	@echo "======================================================="
	$(Q)$(SZ) $@
	@echo "======================================================="
	@echo -e "\033[1;32mBuild Completed!\033[0m"

# Build dependencies
$(BUILD_DIR)/%.d: %.s Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR)/%.d: %.c Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR)/%.d: %.cpp Makefile | $(BUILD_DIR)
	$(Q)$(CC) $(DEP_FLAG) $< > $@

$(BUILD_DIR):
	@$(MKDIR) $@

echo:
	@echo target: $(TARGET)
	@echo dynamic_lib: $(DYNAMIC_LIB)
	@echo static_lib: $(STATIC_LIB)
	@echo include_dir: $(INCLUDE_DIR)
	@echo objects: $(OBJECTS)
	@echo cflags: $(CFLAGS)
	@echo cppflags: $(CPPFLAGS)
	@echo asmflags: $(ASMFLAGS)
	@echo ldflags: $(LDFLAGS)
	@echo corenum: $(CORENUM)

install: all
	@echo -e "\033[1;36m    Installing... \033[0m"
	@$(if $(TARGET),cp $(TARGET) $(TOPDIR)/)
	@$(if $(DYNAMIC_LIB),cp $(DYNAMIC_LIB) $(TOPDIR)/)
	@$(if $(STATIC_LIB),cp $(STATIC_LIB) $(TOPDIR)/)

# Clean up
clean:
	@echo -e "\033[1;31m    Clean up... \033[0m"
	$(Q)$(RM) $(TARGET) $(DYNAMIC_LIB) $(STATIC_LIB) $(OBJECTS)

cleanall:
	@echo -e "\033[1;31m    Clean up all... \033[0m"
	$(Q)$(RM) $(BUILD_DIR)

# Source file dependencies
-include $(DEPENDS)

# *** EOF ***
