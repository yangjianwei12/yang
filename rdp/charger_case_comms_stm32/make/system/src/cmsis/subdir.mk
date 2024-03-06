################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
SRCS = \
../system/src/cmsis/vectors_stm32f0xx.c 

C_SRCS += $(SRCS)
OBJS += $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.o,$(SRCS)))
C_DEPS+= $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.d,$(SRCS)))
DEFINES = $(foreach def,$(C_DEFS),-D$(def))

# Each subdirectory must supply rules for building sources it contributes
$(build_dir)/system/src/cmsis/%.o: ../system/src/cmsis/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	$(CC) $(CFLAGS) $(C_INCLUDES) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
