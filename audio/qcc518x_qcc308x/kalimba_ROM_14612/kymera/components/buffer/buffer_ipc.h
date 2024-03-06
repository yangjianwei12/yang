/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file buffer_ipc.h
 * \ingroup buffer
 *
 * Public buffer subsystem header file for multi core
 */

#ifndef BUFFER_IPC_H
#define BUFFER_IPC_H

/****************************************************************************
Include Files
*/

#include "buffer.h"

/****************************************************************************
Public Function Declarations
*/

/**
 * Create a SW cbuffer from shared ram with malloc.
 *
 * This function mallocs a buffer of buffer_size words big for the
 * cbuffer data storage. It also creates a tCbuffer object from the
 * shared ram and populates its fields.
 *
 * \param buffer_size is the requested size of the buffer in words.
 * \param descriptor is the bit field cbuffer descriptor.
 *
 * \return pointer to tCbuffer object or NULL if unsuccessful. Could panic.
 */
extern tCbuffer *cbuffer_create_shared_with_malloc(unsigned int buffer_size,
                                                   unsigned int descriptor);


/**
 * Create a SW cbuffer from shared ram with malloc from shared ram.
 *
 * This function mallocs a buffer of buffer_size words big for the
 * cbuffer data storage,
 * creates a tCbuffer object and populates its fields.
 * This function allocates memory from shared ram if it exists on the device.
 *
 * \param buffer_size is the requested size of the buffer in words.
 * \param descriptor is the bit field cbuffer descriptor.
 *
 * \return pointer to tCbuffer object or NULL if unsuccessful. Could panic.
 */
extern tCbuffer *cbuffer_create_shared_with_malloc_shared(unsigned int buffer_size,
                                                          unsigned int descriptor);

/**
 * Create a SW cbuffer from shared ram with malloc from fast ram
 *
 * This function mallocs a buffer buffer_size words big for the cbuffer
 * data storage,
 * creates a tCbuffer object and populates its fields.
 * This function allocates memory from shared and fast ram if it exists
 * on the device.
 *
 * \param buffer_size is the requested size of the buffer in words.
 * \param descriptor is the bit field cbuffer descriptor.
 *
 * \return pointer to tCbuffer object or NULL if unsuccessful. Could panic.
 */
extern tCbuffer *cbuffer_create_shared_with_malloc_fast(unsigned int buffer_size,
                                                        unsigned int descriptor);

/**
 * cbuffer_create_with_shared_cbuf - used to create SW buffers
 *
 * This function allocates cbuf struct with shared memory preference. No
 * data buffer is allocated, instead the parameter 'cbuffer_data_ptr' is used.
 *
 * \param cbuffer_data_ptr pointer to externally allocated memory to hold the
 *                         cbuffer data for SW buffers
 * \param buffer_size      in words
 * \param descriptor       the bit field cbuffer descriptor.
 *
 * \return If successful pointer to the created cbuffer structure else NULL
 */
extern tCbuffer *cbuffer_create_with_shared_cbuf(void *cbuffer_data_ptr,
                                                 unsigned int buffer_size,
                                                 unsigned int descriptor);

#if !defined(COMMON_SHARED_HEAP)
/**
 * cbuffer_buffer_sync - Synchronise two cbuffers that use the same data buffer
 *
 * This function synchronises two cbuffers, so that the read and the write
 * pointers are identical. It is used for dual-core cbuffers, where there
 * is a cbuf struct in private memory (in one cbuffer) and a cbuf struct
 * in shared memory (in another cbuffer) which both use the same data buffer.
 * The shared memory cbuf is used for multi-cpu access, where one cpu reads
 * and the other writes the cbuffer, and vice versa.
 *
 * \param dst_cbuffer destination cbuffer structure
 * \param src_cbuffer source cbuffer structure
 *
 * NB. expects the two cbuffer structs to point to the same physical buffer;
 *     will panic otherwise.
 */
extern void cbuffer_buffer_sync(tCbuffer *dst_cbuffer, tCbuffer *src_cbuffer);
#endif /* !COMMON_SHARED_HEAP */
#endif /* IPC_BUFFER_H */
