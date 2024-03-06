################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
SRCS = \
../usb/src/stm32f0xx_hal_pcd.c \
../usb/src/stm32f0xx_hal_pcd_ex.c \
../usb/src/stm32f0xx_ll_usb.c \
../usb/src/usbd_cdc.c \
../usb/src/usbd_cdc_if.c \
../usb/src/usbd_qcom_usb_debug.c \
../usb/src/usbd_qcom_usb_debug_if.c \
../usb/src/usbd_conf.c \
../usb/src/usbd_core.c \
../usb/src/usbd_ctlreq.c \
../usb/src/usbd_desc.c \
../usb/src/usbd_ioreq.c 

C_SRCS += $(SRCS)
OBJS += $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.o,$(SRCS)))
C_DEPS+= $(patsubst ..%,$(build_dir)%,$(patsubst %.c,%.d,$(SRCS)))

DEFINES = $(foreach def,$(C_DEFS),-D$(def))

# Each subdirectory must supply rules for building sources it contributes
$(build_dir)/usb/src/%.o: ../usb/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU C Compiler'
	$(CC) $(CFLAGS) $(C_INCLUDES) $(DEFINES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
