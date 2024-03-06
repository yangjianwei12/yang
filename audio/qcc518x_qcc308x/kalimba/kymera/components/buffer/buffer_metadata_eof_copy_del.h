/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  buffer_metadata_eof_copy_del.h
 *
 * \ingroup buffer
 *
 * Inline fragments for end-of-file tag private data copy and delete handling
 *
 */
#ifndef BUFFER_METADATA_EOF_COPY_DEL_H
#define BUFFER_METADATA_EOF_COPY_DEL_H
/****************************************************************************
Include Files
*/
#include "buffer_metadata_eof.h"

/****************************************************************************
Private Macro Definitions
*/
#if defined(SUPPORTS_MULTI_CORE)
#define ON_SAME_CORE(x) PROC_ON_SAME_CORE(x->proc_id)
#else
#define ON_SAME_CORE(x) TRUE
#endif

/****************************************************************************
Inline Function Definitions
*/
/* Fragment of tag_deletion_scan */
static inline void metadata_eof_handle_deletion(void* cb_data, metadata_tag* tag, unsigned item_length, void* item_data)
{
    if (METADATA_STREAM_END(tag))
    {
        metadata_eof_callback *cb = item_data;
        PL_ASSERT(item_length == sizeof(metadata_eof_callback));
        /**
         * ref pointer can be NULL when the tag is crossing cores but
         * buff_metadata_delete_tag() shouldn't call this function in that
         * scenario.
         */
        PL_ASSERT(cb->ref != NULL);

        cb->ref->ref_count_local -= 1;
        if ((cb->ref->ref_count_local == 0) &&
            (cb->ref->ref_count_remote == 0))
        {
#if defined(COMMON_SHARED_HEAP)
            if (ON_SAME_CORE(cb))
#else
            if (!ON_SAME_CORE(cb))
            {
                metadata_eof_send_over_kip(cb);
            }
            else
#endif /* COMMON_SHARED_HEAP */
            {
                metadata_eof_delete_final(cb->ref);
            }
        }
    }
}

/* Fragment of tag_copy_scan */
static inline void metadata_eof_handle_copy(void* cb_data, metadata_tag* tag, unsigned item_length, void* item_data)
{
    if (METADATA_STREAM_END(tag))
    {
        metadata_eof_callback *cb = item_data;
        PL_ASSERT(item_length == sizeof(metadata_eof_callback));
        PL_ASSERT(cb->ref != NULL);
#if defined(SUPPORTS_MULTI_CORE)
        bool remote_copy = (bool)(uintptr_t)cb_data;
        if (remote_copy)
        {
            cb->ref->ref_count_local--;

            if (PROC_ON_SAME_CORE(cb->proc_id))
            {
                cb->ref->ref_count_remote++;
            }
            /* Appending to the originating core (assuming no more than 2 cores) */
            else /* if (!PROC_ON_SAME_CORE(cb->proc_id)) */
            {
                if (cb->ref->ref_count_local == 0)
                {
                    pdelete(cb->ref);
                    cb->last_remote_copy =  TRUE;
                }
                else
                {
                    cb->last_remote_copy =  FALSE;
                }
            }

            /**
             * NB There's nothing to be done if we're appending from the originating core.
             * cb->parent and cb->proc_id should already be set.
             */

            /**
             * This is not exactly necessary as the other core uses the proc_id to decide
             * if it needs to allocate its own local cb->ref but let's set it to NULL to
             * be safe.
             */
            cb->ref = NULL;
        }
        else
#endif
        {
            cb->ref->ref_count_local++;
        }
    }
}

#endif /* BUFFER_METADATA_EOF_H */
