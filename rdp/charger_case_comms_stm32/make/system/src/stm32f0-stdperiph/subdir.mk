################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
SRCS = \
../system/src/stm32f0-stdperiph/stm32f0xx_adc.c \
../system/src/stm32f0-stdperiph/stm32f0xx_flash.c \
../system/src/stm32f0-stdperiph/stm32f0xx_gpio.c \
../system/src/stm32f0-stdperiph/stm32f0xx_iwdg.c \
../system/src/stm32f0-stdperiph/stm32f0xx_misc.c \
../system/src/stm32f0-stdperiph/stm32f0xx_rcc.c \
../system/src/stm32f0-stdperiph/stm32f0xx_syscfg.c \
../system/src/stm32f0-stdperiph/stm32f0xx_tim.c \
../system/src/stm32f0-stdperiph/stm32f0xx_usart.c 

C_SRCS += $(SRCS)
OBJS += $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.o,$(SRCS)))
C_DEPS+= $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.d,$(SRCS)))
DEFINES = $(foreach def,$(C_DEFS),-D$(def))

# Each subdirectory must supply rules for building sources it contributes
$(build_dir)/system/src/stm32f0-stdperiph/%.o: ../system/src/stm32f0-stdperiph/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	$(CC) $(CFLAGS) $(C_INCLUDES) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
