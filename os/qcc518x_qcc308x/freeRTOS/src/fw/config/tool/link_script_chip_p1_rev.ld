/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

OUTPUT_ARCH(QCC517x_apps)

/* Link script for QCC517x and QCC307x Processor P1 FreeRTOS */

$DATA_RAM_START_ADDRESS = 0x00040000;
$DATA_RAM_SIZE_BYTES    = 160 * 1024;

/* FreeRTOS uses this stack for boot and interrupts,
   task stacks are allocated separately. */
$STACK_SIZE_BYTES = 700; /* If you change this, change
                          * blow_the_stack_away() in excep_test.c
                          * accordingly (needs to work for p0 and p1)
                          */

INCLUDE link_script_chip_p1.ld /* No revision specific defines required */
