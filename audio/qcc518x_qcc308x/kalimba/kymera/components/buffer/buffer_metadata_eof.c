/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file buffer_metadata_eof.c
 * \ingroup buffer
 *
 * Metadata end-of-file handling
 */

/****************************************************************************
Include Files
*/
#include "buffer_metadata_eof.h"

/****************************************************************************
Module Scope Function Definitions
*/

/*
 * metadata_eof_delete_final
 *
 * Called when all local and remote references have gone away. This is only
 * called on the EOF-originating core.
 */
void metadata_eof_delete_final(metadata_eof_callback_ref *cb_ref)
{
    PL_ASSERT(cb_ref->ref_count_local == 0);
    PL_ASSERT(cb_ref->ref_count_remote == 0);

    PL_ASSERT(cb_ref->callback != NULL);

    cb_ref->callback(cb_ref->data);

    pdelete(cb_ref);
}

#if defined(SUPPORTS_MULTI_CORE)
#if !defined(COMMON_SHARED_HEAP)
void metadata_eof_send_over_kip(metadata_eof_callback *cb)
{
    CONNECTION_LINK conn_id;
    ADAPTOR_MSGID msg_id;
    KIP_MSG_HANDLE_EOF_REQ kip_msg;

    pdelete(cb->ref);
    conn_id = PACK_CONID_PROCID(proc_get_processor_id(), cb->proc_id);
    msg_id = KIP_MSG_ID_TO_ADAPTOR_MSGID(KIP_MSG_ID_HANDLE_EOF_REQ);
    KIP_MSG_HANDLE_EOF_REQ_CB_REF_SET(&kip_msg, (uintptr_t) cb->parent);

    adaptor_send_message(conn_id, msg_id,
                         KIP_MSG_HANDLE_EOF_REQ_WORD_SIZE,
                         (ADAPTOR_DATA) &kip_msg);
}

/****************************************************************************
Public Function Definitions
*/

/*
 * buff_metadata_eof_remote_cb
 *
 * This is used as an intermediate callback when a final deletion happens
 * on a remote (not the EOF originator) core.
 *
 * The data parameter is the original local reference pointer.
 *
 */
void buff_metadata_eof_remote_cb(metadata_eof_callback_ref *cb_ref)
{
    PL_ASSERT(cb_ref != NULL);

    if ((--cb_ref->ref_count_remote == 0) && (cb_ref->ref_count_local == 0))
    {
        metadata_eof_delete_final(cb_ref);
    }
}

void buff_metadata_kip_prepare_eof_after_remove(metadata_tag* tag)
{
    PL_ASSERT(METADATA_STREAM_END(tag));

    metadata_eof_callback *cb;
    unsigned length;

    if (buff_metadata_find_private_data(tag, META_PRIV_KEY_EOF_CALLBACK, &length, (void**)&cb))
    {
        PL_ASSERT(length == sizeof(metadata_eof_callback));
        PL_ASSERT(cb->ref == NULL);
    }
    else {
        PL_ASSERT(0); //TODO: Do we expect this to ever happen?
    }

    if (PROC_ON_SAME_CORE(cb->proc_id)) /* Tag coming back home */
    {
        cb->ref = cb->parent;

        //TODO: Is it important to increment first? Do we have to worry about things being atomic?
        cb->ref->ref_count_local++;

        /**
         * We can only decrement our remote counter when we know there are no
         * copies of it left on the remote processor. This is not relevant when
         * we get an explicit KIP message.
         */
        if (cb->last_remote_copy)
        {
            cb->ref->ref_count_remote--;
        }
    }
    else
    {
        if ((cb->ref = xzpnew(metadata_eof_callback_ref)) != NULL)
        {
            cb->ref->ref_count_local = 1;

            /** These are not going to get used on this core anyway */
            cb->ref->callback = NULL;
            cb->ref->data = 0;
        }
    }
}
#endif /* !COMMON_SHARED_HEAP */
#endif /* defined(SUPPORTS_MULTI_CORE) */



/*
 * \brief Add EOF callback struct to a metadata tag
 */
bool buff_metadata_add_eof_callback(metadata_tag *tag, eof_callback callback, unsigned eof_data)
{
    metadata_eof_callback cb;
    patch_fn_shared(buff_metadata);
    METADATA_STREAM_END_SET(tag);
#if defined(COMMON_SHARED_HEAP)
    if ((cb.ref = xzppnew(metadata_eof_callback_ref, MALLOC_PREFERENCE_SHARED)) == NULL)
#else
    if ((cb.ref = xzpnew(metadata_eof_callback_ref)) == NULL)
#endif /* COMMON_SHARED_HEAP */
    {
        return FALSE;
    }

    cb.ref->ref_count_local = 1;
    cb.ref->callback = callback;
    cb.ref->data = eof_data;

#if defined(SUPPORTS_MULTI_CORE)
    cb.proc_id = proc_get_processor_id();
    cb.parent = cb.ref;
#endif

    return (buff_metadata_add_private_data(tag, META_PRIV_KEY_EOF_CALLBACK, sizeof(metadata_eof_callback), &cb) != NULL);
}

/*
 * \brief Retrieve EOF callback struct from a metadata tag
 */
bool buff_metadata_get_eof_callback(metadata_tag *tag, metadata_eof_callback *cb_struct)
{
    unsigned length;
    metadata_eof_callback *cb;

    patch_fn_shared(buff_metadata);

    if (buff_metadata_find_private_data(tag, META_PRIV_KEY_EOF_CALLBACK, &length, (void **)&cb))
    {
        PL_ASSERT(length == sizeof(metadata_eof_callback));
        *cb_struct = *cb;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

