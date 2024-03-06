################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# C source files included in all builds.
SRCS = \
../src/clock.c \
../src/uart.c

# C source files included in all non-bootloader builds.
ifneq ($(strip $(BANK)),BOOT)
SRCS += \
../src/adc.c \
../src/auth.c \
../src/battery.c \
../src/case.c \
../src/case_channel.c \
../src/case_charger.c \
../src/ccp.c \
../src/charger.c \
../src/charger_detect.c \
../src/cli.c \
../src/cli_parse.c \
../src/cli_txf.c \
../src/config.c \
../src/crc.c \
../src/debug.c \
../src/dfu.c \
../src/earbud.c \
../src/fake_earbud.c \
../src/flash.c \
../src/gpio.c \
../src/interrupt.c \
../src/led.c \
../src/main.c \
../src/memory.c \
../src/pfn.c \
../src/power.c \
../src/rtc.c \
../src/timer.c \
../src/usb.c \
../src/vreg.c \
../src/wdog.c \
../src/wire.c 
endif

# Bootloader specific files
ifeq ($(strip $(BANK)),BOOT)
SRCS += \
../src/bootloader.c 
else

# CB variant specific files.
ifeq ($(strip $(VARIANT)),CB)
SRCS += \
../src/charger_comms.c \
../src/charger_comms_device.c \
../src/current_senses.c
endif

# Specific files for all variants except CB
ifneq ($(strip $(VARIANT)),CB)
SRCS += \
../src/charger_comms_b.c \
../src/dfu_earbud_if.c \
../src/qcom_debug_channel.c
endif

endif

C_SRCS += $(SRCS)
OBJS += $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.o,$(SRCS)))
C_DEPS+= $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.d,$(SRCS)))
DEFINES = $(foreach def,$(C_DEFS),-D$(def))

# Each subdirectory must supply rules for building sources it contributes
$(build_dir)/src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	$(CC) $(CFLAGS) $(C_INCLUDES) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

