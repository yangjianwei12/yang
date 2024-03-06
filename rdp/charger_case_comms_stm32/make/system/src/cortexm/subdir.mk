################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
SRCS = \
../system/src/cortexm/_initialize_hardware.c \
../system/src/cortexm/_reset_hardware.c \
../system/src/cortexm/exception_handlers.c 

C_SRCS += $(SRCS)
OBJS += $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.o,$(SRCS)))
C_DEPS+= $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.d,$(SRCS)))
DEFINES = $(foreach def,$(C_DEFS),-D$(def))

# Each subdirectory must supply rules for building sources it contributes
$(build_dir)/system/src/cortexm/%.o: ../system/src/cortexm/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	$(CC) $(CFLAGS) $(C_INCLUDES) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
