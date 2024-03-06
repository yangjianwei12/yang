/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file const_data_private.h
 * \ingroup const_data
 *
 * Header file for some of the internals of the const data module
 */

#ifndef CONSTANT_DATA_PRIVATE_H
#define CONSTANT_DATA_PRIVATE_H

#include "const_data.h"
#include "string.h"
#include "panic/panic.h"

/**
 * \brief Access constant data.
 *
 * Copies a block of constant data, usually from ROM to RAM.
 *
 * \param src  Address of packed data.
 * \param size The size of the data to be unpacked.
 * \param dest Address to unpack data into.
 *
 * \return void
 */
extern void mem_ext_copy_to_ram(char *src, unsigned size, unsigned *dest);

/**
 * \brief Access constant data.
 *
 * Unpack signed octets (int8) to 32-bit words, usually from ROM to RAM.
 *
 * \param src  Address of packed data.
 * \param size The size of the data to be unpacked.
 * \param dest Address to unpack data into.
 *
 * \return void
 */
extern void mem_ext_unpack8_to_ram(char *src, unsigned size, unsigned *dest);


/* empty function, not really used, TODO B-303156 */
extern void mem_ext_window_copy_to_ram(char *src, unsigned size, unsigned *dest);

#endif /* CONSTANT_DATA_PRIVATE_H */

