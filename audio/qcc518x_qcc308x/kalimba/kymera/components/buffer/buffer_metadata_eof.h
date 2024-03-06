/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  buffer_metadata_eof.h
 *
 * \ingroup buffer
 *
 * Metadata end-of-file handling
 *
 */
#ifndef BUFFER_METADATA_EOF_H
#define BUFFER_METADATA_EOF_H
/****************************************************************************
Include Files
*/
#include "buffer_private.h"

/****************************************************************************
Public Type Definitions
*/
struct metadata_eof_callback_ref
{
    unsigned ref_count_local;
    unsigned ref_count_remote;

    eof_callback callback;              /**< callback function */
    unsigned data;                      /**< data to pass to callback */
};

struct metadata_eof_callback
{
    metadata_eof_callback_ref *ref;     /**< local reference for tracking */

#if defined(SUPPORTS_MULTI_CORE)
    /** These bits are only relevant for tags that cross cores */
    PROC_ID_NUM proc_id; /* Originating core */
    void* parent; /* Pointer to the metadata_eof_callback_ref on the originating
                   * core. This is stored so that we can restore ref field if the
                   * tag goes back to the originating core */
    bool last_remote_copy;
#endif /* defined(SUPPORTS_MULTI_CORE) */
};


/****************************************************************************
Module Scope Function Declarations
*/
extern void metadata_eof_delete_final(metadata_eof_callback_ref *cb_ref);

#if defined(SUPPORTS_MULTI_CORE)
extern void metadata_eof_send_over_kip(metadata_eof_callback *cb);
#else
static inline void metadata_eof_send_over_kip(metadata_eof_callback *cb)
{
    (void)cb;
}
#endif


#endif /* BUFFER_METADATA_EOF_H */
