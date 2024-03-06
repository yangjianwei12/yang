/* Copyright (c) 2022 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 * Contains the corresponding function.
 */

#include "buffer/buffer_private.h"

/**
 * Returns an 8-bit pointer to read data from a buffer at the tail.
 *
 * WARNING: you are most likely doing something wrong if you are
 * using this function.  Please check buf_raw_read_map_8bit and
 * the header
 */
const uint8 *buf_raw_read_map_8bit_tail(const BUFFER *buf)
{
    mmu_read_port_open(buf->handle);
    return mmu_read_port_map_8bit(buf->handle, buf->tail);
}
