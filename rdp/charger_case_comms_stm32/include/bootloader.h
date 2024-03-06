/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      BOOTLOADER
*/

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

#include <stddef.h>
#include "stm32f0xx.h"
#include "mem.h"

/*-----------------------------------------------------------------------------
------------------ PREPROCESSOR and TYPE DEFINITIONS --------------------------
-----------------------------------------------------------------------------*/

#define IMAGE_A_START _FLASH_A_ORIGIN
#define IMAGE_A_END   (_FLASH_A_ORIGIN + _FLASH_A_LENGTH - 1)
#define IMAGE_B_START _FLASH_B_ORIGIN
#define IMAGE_B_END   (_FLASH_B_ORIGIN + _FLASH_B_LENGTH - 1)
#define FLASH_CFG_START _FLASH_CFG_A_ORIGIN

#define PAGE_SIZE 0x800

#define VECTOR_TABLE_SIZE _VECTOR_TABLE_BOOT_LENGTH

/* This address is reserved for DFU reboot byte, and changing this address here
 * would requires a change in config.c file as well. Take precaution if you
 * ever have to change this address. Refer to config.c for more info.
 */
#define DFU_REBOOT_BYTE_ADDR (_FLASH_CFG_A_ORIGIN + 0xF)
#define DFU_REBOOT_BYTE (*((uint8_t *)(DFU_REBOOT_BYTE_ADDR)))

/*
* This structure exists to facilitate reading certain items from the vector
* table: the stack pointer, the program counter and the image count (which
* occupies an unused location in the table).
*/
typedef struct
{
    uint32_t sp;
    uint32_t pc;
    uint32_t x2;
    uint32_t x3;
    uint32_t x4;
    uint32_t x5;
    uint32_t x6;
    uint32_t image_count;
}
VT_MAP;

#define IMAGE_COUNT_OFFSET (offsetof(VT_MAP, image_count))

#endif /* BOOTLOADER_H_ */
