################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

# General OS utilities.
ifeq ($(OS), Windows_NT)
RM := rmdir /s /q
MKDIR := mkdir
else
RM := rm -rf
MKDIR := mkdir -p
endif

# C toolchain
CC := arm-none-eabi-gcc
CXX := arm-none-eabi-g++
LD := arm-none-eabi-g++
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

# Flags common to C and C++ compilers.
CCXFLAGS := \
-mcpu=cortex-m0 \
-mthumb \
-Os \
-fmessage-length=0 \
-fsigned-char \
-ffunction-sections \
-fdata-sections \
-ffreestanding \
-fno-move-loop-invariants \
-Wall \
-Wextra

# C compiler flags
CFLAGS := \
-std=gnu11
CFLAGS += $(CCXFLAGS)

# C++ compiler flags
CPPFLAGS := \
-std=gnu++11 \
-fabi-version=0 \
-fno-exceptions \
-fno-rtti \
-fno-use-cxa-atexit \
-fno-threadsafe-statics 
CPPFLAGS += $(CCXFLAGS)

C_INCS := \
"../include/override" \
"../include" \
"../usb/include" \
"../system/include" \
"../system/include/cmsis" \
"../system/include/stm32f0-stdperiph" 

C_INCLUDES = $(foreach def,$(C_INCS),-I$(def))

ifneq ($(C_DEFS),)
C_DEFS := 
endif

# Establish which variant we are building.
ifeq ($(strip $(BANK)),BOOT)
VARIANT = 
endif
ifeq ($(strip $(VARIANT)),CB)
override C_DEFS += VARIANT_CB
endif
ifeq ($(strip $(VARIANT)),ST2)
override C_DEFS += VARIANT_ST2
endif
ifeq ($(strip $(VARIANT)),MON)
override C_DEFS += VARIANT_MON
endif

# C defines for all builds. 
override C_DEFS += \
DEBUG \
USE_FULL_ASSERT \
USE_STDPERIPH_DRIVER \
HSE_VALUE=16000000 \
STM32F070xB

# Define build directory
build_root := build
ifneq ($(VARIANT), )
build_dir := $(build_root)/$(VARIANT)/$(BANK)
else
build_dir := $(build_root)/$(BANK)
endif

# All of the sources participating in the build are defined here
-include sources.mk

# USB support is not included in the bootloader.
ifneq ($(strip $(BANK)),BOOT)
-include usb/src/subdir.mk
endif

-include system/src/stm32f0-stdperiph/subdir.mk
-include system/src/newlib/subdir.mk
-include system/src/diag/subdir.mk
-include system/src/cortexm/subdir.mk
-include system/src/cmsis/subdir.mk
-include src/subdir.mk
-include subdir.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += charger-comms-stm32f0.srec
SECONDARY_SIZE += charger-comms-stm32f0.siz

MEMORY_MAP :=
LINK_SECTIONS :=

# All Target
all: charger-comms-stm32f0.elf secondary-outputs


checkbank:
ifeq ($(strip $(BANK)),)
	$(error Must define BANK variable)
endif
ifeq ($(strip $(BANK)),BOOT)
    MEMORY_MAP = mem_boot.ld
    LINK_SECTIONS = sections_bootloader.ld
endif
ifeq ($(strip $(BANK)),A)
    MEMORY_MAP = mem_bank_a.ld
    LINK_SECTIONS = sections.ld
endif
ifeq ($(strip $(BANK)),B)
    MEMORY_MAP = mem_bank_b.ld
    LINK_SECTIONS = sections.ld
endif

# Create build paths.
# We must adjust path seperators for Windows.
ifeq ($(OS), Windows_NT)
BDIR := $(subst /,\,$(build_dir))
makefolders:
	$(shell $(MKDIR) $(BDIR))
	$(foreach dir,$(SUBDIRS),$(shell $(MKDIR) $(BDIR)\$(subst /,\,$(dir))))
else
makefolders:
	$(shell $(MKDIR) $(build_dir))
	$(foreach dir,$(SUBDIRS),$(shell $(MKDIR) $(build_dir)/$(dir)))
endif

# Tool invocations
charger-comms-stm32f0.elf: checkbank makefolders $(OBJS) 
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM GNU C++ Linker'
	$(LD) $(CCXFLAGS) -g3 -T $(MEMORY_MAP) -T libs.ld -T $(LINK_SECTIONS) -nostartfiles -Xlinker --gc-sections -Wl,-Map,"$(build_dir)/charger-comms-stm32f0.map" --specs=nano.specs -o "$(build_dir)/charger-comms-stm32f0.elf" $(OBJS) 
	@echo 'Finished building target: $@'
	@echo ' '

charger-comms-stm32f0.srec: charger-comms-stm32f0.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	$(OBJCOPY) -O srec --srec-len=128 "$(build_dir)/charger-comms-stm32f0.elf"  "$(build_dir)/charger-comms-stm32f0.srec"
	@echo 'Finished building: $@'
	@echo ' '

charger-comms-stm32f0.siz: charger-comms-stm32f0.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	$(SIZE) --format=berkeley "$(build_dir)/charger-comms-stm32f0.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(build_root)

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents checkbank makefolders memory_defines 

-include ../makefile.targets
