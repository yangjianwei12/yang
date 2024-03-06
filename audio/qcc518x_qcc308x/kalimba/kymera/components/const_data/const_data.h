/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file const_data.h
 * \ingroup const_data
 *
 * This module handles platform independent constant data access.
 */

#ifndef CONSTANT_DATA_H
#define CONSTANT_DATA_H

#include "types.h"
#include "hal_const_sections.h"

/** 
  * Constant data memory types (regions)
  *
  * This is a placeholder for defining all possible constant data memory regions.
  * \note Make sure these regions are correctly mapped according to the
  *       chip's linkscript.
  */
typedef enum
{
    /* DMCONST const data regions */
    MEM_TYPE_DMCONST  = 1,
    MEM_TYPE_DMCONST16 = 2,
    MEM_TYPE_DMCONST_WINDOWED16 = 3,

    /* External / Non-DM const data regions */
    MEM_TYPE_CONST = 4,
    MEM_TYPE_CONST16 = 5,

    /* DM internal statically or dynamically allocated, currently unsupported */
    MEM_TYPE_INTERNAL = 0
} const_data_mem_type;

/** Data format encodings (packing types)
  * NOTE: the format should be platform independent */
typedef enum
{
    /* To be resolved to the default platform/chip format */
    /* NOTE: if source data width is less than DSP word data will be left aligned */
    FORMAT_DSP_NATIVE = 0,

    /* 16bit data to be right aligned and zero-padded to the DSP native format */
    FORMAT_16BIT_ZERO_PAD = 1,

    /* 16bit data to be right aligned and sign extended to the DSP native format */
    FORMAT_16BIT_SIGN_EXT = 2,

    /* 2 24bit DSP words packed into 3x16bits
     *   NOTES: Currently not valid on DRAM targets, may be added in future
     *          since we have some HW support for unpacking (3x32bit -> 4x24bit words) */
    FORMAT_PACKED16 = 3,
    /* 1 32-bit DSP word in one word (32 bits platforms) */
    FORMAT_UNPACKED32 = 4
} const_data_format;

/** The descriptor structure that holds the information (location and format)
  * about the source constant data */
typedef struct
{
    /* The memory type: this must match the region where address points to */
    const_data_mem_type type : 4;

    /* platform specific format; must match the data type pointed to by address */
    const_data_format format : 4;

    /* Address/offset in the memory (platform specific, 32bit on Crescendo, 24bit elsewhere)
       **Note: on Atlas this is a relative offset, not the actual physical address */
    void * address;
} const_data_descriptor;

#define DEFINE_CONST_DATA_DESCRIPTOR(type, format, const_data) { type, format, const_data }

/**
 * \brief Copy constant data.
 *
 * Constant data will be copied from the source location to the 'destination'.
 *
 * \param source      The source descriptor containing information about the
 *                    constant data location and format.
 * \param offset      The offset in octets within the source memory where the
 *                    data read should start from.
 * \param destination The destination memory block pointer, should be at least
 *                    'size' memory units long.
 * \param size        The number of octets of the data to be transferred.
 *
 * \return TRUE when successful.
 *
 * \note The user is responsible for allocation and freeing of the memory
 *       pointed by 'destination'.
 */
extern bool const_data_copy(const_data_descriptor *source,
                            unsigned offset,
                            void *destination,
                            unsigned size);

/**
 * \brief Access constant data.
 *
 * \param source   The source descriptor containing information about the
 *                 constant data location and format.
 * \param offset   The offset in octets within the source memory where the data
 *                 read should start from.
 * \param work_buf The memory block which will act as a working buffer; if this
 *                 parameter is NULL a new buffer will be allocated, on
 *                 successive calls this parameter should contain the returned
 *                 value from the first call to const_data_access. This is
 *                 required due to Kalimba platform differences where locations
 *                 may or may not be memory mapped (or directly accessible).
 * \param size     The size of the data to be transferred.
 *
 * \return A pointer to a block of constant data described by 'source' or NULL.
 */
extern void *const_data_access(const_data_descriptor *source,
                               unsigned offset,
                               void *work_buf,
                               unsigned size);

/**
 * \brief Release (free) memory block allocated by const_data_access function.
 *
 * \param ptr Pointer to the memory as returned by const_data_access.
 */
extern void const_data_release(void *ptr);

#endif /* CONSTANT_DATA_H */
