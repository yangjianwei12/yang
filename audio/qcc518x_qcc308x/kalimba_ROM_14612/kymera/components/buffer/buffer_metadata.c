/****************************************************************************
 * Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file buffer_metadata.c
 * \ingroup buffer
 *
 * Buffer metadata implementation
 */

/****************************************************************************
Include Files
*/
#include "buffer_private.h"
#include <string.h>
#include "sections.h"

#if defined(COMMON_SHARED_HEAP)
#include "hal/hal_hwsemaphore.h"
#include "hal/hal_dm_sections.h"
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
#include "platform/pl_hwlock.h"
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Private Constant Declarations
*/
#if defined(COMMON_SHARED_HEAP)
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
enum {
    METADATA_TAG_LOCK = 0,
    METADATA_TRANSPORT_LOCK = 1,
    METADATA_NUM_LOCKS
};

/* BUFF_METADATA_HW_LOCK_RETRIES = BUFF_METADATA_SEMAPHORE_RETRIES + 5 */
#define BUFF_METADATA_HW_LOCK_RETRIES      55
#else
#define BUFF_METADATA_SEMAPHORE_RETRIES    50
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Private Macro Declarations
*/

/* Attempt to limit the total number of allocated tags
 * This number is checked against the allocation count in buff_metadata_tag_threshold_exceeded()
 * Note: Not static or the compiler will optimise it out, and we want it in memory for easy patchability
 */
#ifndef DEFAULT_TAG_ALLOC_COUNT
#error Builds with metadata must define DEFAULT_TAG_ALLOC_COUNT
#else
#if defined(COMMON_SHARED_HEAP)
DM_SHARED unsigned tag_alloc_threshold = DEFAULT_TAG_ALLOC_COUNT - TAG_ALLOC_THRESHOLD_TOLERANCE;
#else
unsigned tag_alloc_threshold = DEFAULT_TAG_ALLOC_COUNT - TAG_ALLOC_THRESHOLD_TOLERANCE;
#endif /* COMMON_SHARED_HEAP */
#endif

/****************************************************************************
Private Variable Definitions
*/

#if defined(COMMON_SHARED_HEAP)
/* Count of currently-allocated tags */
DM_SHARED static unsigned tag_alloc_count = 0;

DM_SHARED static metadata_tag *tag_list_head = NULL;

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
DM_SHARED_ZI static unsigned metadata_locks[METADATA_NUM_LOCKS];
/* The number of HW lock retries when allocating/freeing memory in the
 * shared heap. */
unsigned buff_metadata_num_retries = BUFF_METADATA_HW_LOCK_RETRIES;
#else
/* The number of semaphore retries when allocating/freeing memory in the
 * shared heap. */
unsigned buff_metadata_num_retries = BUFF_METADATA_SEMAPHORE_RETRIES;
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#else
static unsigned tag_alloc_count = 0;
#endif /* COMMON_SHARED_HEAP */
/****************************************************************************
Private Function Declarations
*/

/****************************************************************************
Private Function Definitions
*/

static void buff_metadata_align_to_buff_ptrs_core(tCbuffer *buff, bool read, bool write)
{
    if(buff != NULL)
    {
        metadata_list *mlist;
        if (BUF_DESC_BUFFER_TYPE_MMU(buff->descriptor))
        {
            /* This function only knows how to operate on SW buffers. */
            panic_diatribe(PANIC_AUDIO_METADATA_HW_BUFFERS_NOT_SUPPORTED, (DIATRIBE_TYPE)(uintptr_t)buff);
        }

        mlist = buff->metadata;
        if (mlist != NULL)
        {
            unsigned wr_offset, rd_offset;
            unsigned octets_per_word = cbuffer_get_usable_octets(buff);
            /* Calculate the offset in addressable units and then convert this
             * to octets. */
            if (write)
            {
                wr_offset = (unsigned)(uintptr_t)(buff->write_ptr) - (unsigned)(uintptr_t)(buff->base_addr);
                mlist->prev_wr_index = (wr_offset * octets_per_word) / sizeof(int);
#ifdef METADATA_DEBUG_TRANSPORT
                mlist->next_tag_index = mlist->prev_wr_index;
                mlist->last_tag_still_covers = 0;
#endif /* METADATA_DEBUG_TRANSPORT */
            }
            if (read)
            {
                rd_offset = (unsigned)(uintptr_t)(buff->read_ptr) - (unsigned)(uintptr_t)(buff->base_addr);
                mlist->prev_rd_index = (rd_offset * octets_per_word) / sizeof(int);
            }
        }
    }
}

#if defined(COMMON_SHARED_HEAP)
/*
 * \brief A number of tag_alloc_threshold tags has been allocated in memory
 *        and this list must be linked.
 */
static void buff_metadata_link_list(unsigned count)
{
    unsigned i;
    metadata_tag *tag_list = tag_list_head;
    /* Make each tag in the list point to the next one.
     * (except for the last, which will be left with the
     * NULL initialisation value)
     */
    for (i = 0; i < (count - 1); i++)
    {
        tag_list[i].next = &tag_list[i + 1];
    }
}

/*
 * \brief Get a free tag from the metadata list.
 */
static metadata_tag *buff_metadata_get_tag_from_list(void)
{
    metadata_tag *temp;

    if (tag_list_head != NULL)
    {
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
        pl_hwlock_get_with_retry(&metadata_locks[METADATA_TAG_LOCK], buff_metadata_num_retries);
#else
        hal_hwsemaphore_get_with_retry(HWSEMIDX_METADATA_TAG, buff_metadata_num_retries);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
        temp = tag_list_head;
        tag_list_head = temp->next;
        temp->next = NULL;
        tag_alloc_count++;

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
        pl_hwlock_rel(&metadata_locks[METADATA_TAG_LOCK]);
#else
        hal_hwsemaphore_rel(HWSEMIDX_METADATA_TAG);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */

        return temp;
    }
    else
    {
        L2_DBG_MSG("buff_metadata_get_tag_from_list: no more tags");
        fault_diatribe(FAULT_AUDIO_METADATA_TAG_ALLOCATION_FAILED, 0);
        return NULL;
    }
}

/*
 * \brief Free a tag by adding it to the metadata list of free tags.
 */
static void buff_metadata_add_tag_to_list(metadata_tag *tag)
{
    memset(tag, 0, sizeof(metadata_tag));

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
    pl_hwlock_get_with_retry(&metadata_locks[METADATA_TAG_LOCK], buff_metadata_num_retries);
#else
    hal_hwsemaphore_get_with_retry(HWSEMIDX_METADATA_TAG, buff_metadata_num_retries);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */

    if (tag_alloc_count > 0)
    {
        tag_alloc_count--;
    }
    else
    {
        /* Not much we can do here except maybe fault ? */
        L2_DBG_MSG("Metadata tag deleted but count is already zero?");
    }
    tag->next = tag_list_head;
    tag_list_head = tag;

#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
    pl_hwlock_rel(&metadata_locks[METADATA_TAG_LOCK]);
#else
    hal_hwsemaphore_rel(HWSEMIDX_METADATA_TAG);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */

}
#endif /* COMMON_SHARED_HEAP */

/****************************************************************************
Public Function Definitions
*/

void buff_metadata_init(unsigned count)
{
#if defined(COMMON_SHARED_HEAP)
    patch_fn_shared(buff_metadata);
    /* Initialise the metadata system,
     * making space for the number of tags specified by count
     */

    tag_list_head = zppmalloc(sizeof(metadata_tag) * count,
                               MALLOC_PREFERENCE_SHARED);

    buff_metadata_link_list(count);
    tag_alloc_threshold = count - TAG_ALLOC_THRESHOLD_TOLERANCE;
#endif /* COMMON_SHARED_HEAP */

     /*
      * Without COMMON_SHARED_HEAP defined, the tags are allocated and freed
      * on the fly, so there's nothing to do in init().
     */

}

/*
 * buff_metadata_tag_threshold_exceeded
 */
bool buff_metadata_tag_threshold_exceeded(void)
{
    return (tag_alloc_count > tag_alloc_threshold);
}

metadata_tag *buff_metadata_new_tag(void)
{
    patch_fn_shared(buff_metadata);
#if defined(COMMON_SHARED_HEAP)
    return buff_metadata_get_tag_from_list();
#elif defined(METADATA_USE_PMALLOC)
    metadata_tag *tag;

    /* See above, just use the normal dynamic memory system for now */
    tag = xzpnew(metadata_tag);
    if (tag != NULL)
    {
        LOCK_INTERRUPTS;
        tag_alloc_count++;
        UNLOCK_INTERRUPTS;

        return tag;
    }
    fault_diatribe(FAULT_AUDIO_METADATA_TAG_ALLOCATION_FAILED, 0);

    return NULL;
#else
    return NULL;
#endif /* METADATA_USE_PMALLOC */
}

void buff_metadata_delete_tag(metadata_tag *tag, bool process_private_data)
{
    patch_fn_shared(buff_metadata);

    if (tag != NULL)
    {
        /* Check if this tag has an end-of-file callback */
        if (process_private_data)
        {
            metadata_handle_tag_deletion(tag);
        }
        pdelete(tag->xdata);
#if defined(COMMON_SHARED_HEAP)
        buff_metadata_add_tag_to_list(tag);
#elif defined(METADATA_USE_PMALLOC)
        /* See above, just use the normal dynamic memory system for now */
        LOCK_INTERRUPTS;
        if (tag_alloc_count > 0)
        {
            tag_alloc_count--;
        }
        else
        {
            /* Not much we can do here except maybe fault? */
#ifndef UNIT_TEST_BUILD
            L2_DBG_MSG("Metadata tag deleted but count is already zero?");
#endif
        }
        UNLOCK_INTERRUPTS;

        pdelete(tag);
#endif /* METADATA_USE_PMALLOC */
    }
}

void buff_metadata_tag_list_delete(metadata_tag *list)
{
    metadata_tag *t;

    while (list != NULL)
    {
        t = list;
        list = list->next;
        buff_metadata_delete_tag(t, TRUE);
    }
}


metadata_tag *buff_metadata_copy_tag(metadata_tag *tag)
{
    metadata_tag *new_cpy;

    patch_fn_shared(buff_metadata);

    if (tag == NULL)
    {
        return NULL;
    }

    new_cpy = buff_metadata_new_tag();

    if (new_cpy != NULL)
    {
        *new_cpy = *tag;
        if (tag->xdata != NULL)
        {
            unsigned length = buff_metadata_get_priv_data_length(tag);
#if defined(COMMON_SHARED_HEAP)
            metadata_priv_data *new_data = (metadata_priv_data *)xppmalloc(
                                                    length,
                                                    MALLOC_PREFERENCE_SHARED);
#else
            metadata_priv_data *new_data = (metadata_priv_data *)xpmalloc(length);
#endif /* COMMON_SHARED_HEAP */

            /* If there isn't enough RAM for this, tough the data gets lost */
            if (new_data != NULL)
            {
                memcpy(new_data, tag->xdata, length);
                metadata_handle_tag_copy(tag, FALSE);
            }
            new_cpy->xdata = new_data;
        }
    }
    return new_cpy;
}


unsigned buff_metadata_get_buffer_size(tCbuffer *buff)
{
    return buff->metadata->buffer_size;
}

void buff_metadata_set_buffer_size(tCbuffer *buff, unsigned usable_octets)
{
    if (buff != NULL && buff_has_metadata(buff))
    {
        buff->metadata->buffer_size = cbuffer_get_size_in_words(buff) * usable_octets;
    }
}


void buff_metadata_align_to_buff_ptrs(tCbuffer *buff)
{
    buff_metadata_align_to_buff_ptrs_core(buff, TRUE, TRUE);
}

void buff_metadata_align_to_buff_read_ptr(tCbuffer *buff)
{
    buff_metadata_align_to_buff_ptrs_core(buff, TRUE, FALSE);
}

void buff_metadata_align_to_buff_write_ptr(tCbuffer *buff)
{
    buff_metadata_align_to_buff_ptrs_core(buff, FALSE, TRUE);
}


RUN_FROM_PM_RAM
#ifdef METADATA_DEBUG_TRANSPORT
bool buff_metadata_append_dbg(tCbuffer *buff, metadata_tag *tag,
             unsigned octets_pre_written, unsigned octets_post_written,
             unsigned caller_addr)
#else /* METADATA_DEBUG_TRANSPORT */
bool buff_metadata_append(tCbuffer *buff, metadata_tag *tag,
             unsigned octets_pre_written, unsigned octets_post_written)
#endif /* METADATA_DEBUG_TRANSPORT */
{
#ifdef METADATA_DEBUG_TRANSPORT
    unsigned return_addr = pl_get_return_addr();
#endif /* METADATA_DEBUG_TRANSPORT */

    metadata_list *mlist;
    metadata_list *mlist_start;

    patch_fn(metadata_append);

    if (buff == NULL)
    {
        buff_metadata_tag_list_delete(tag);
        return TRUE;
    }

    mlist_start = mlist = buff->metadata;

    if (mlist == NULL)
    {
        buff_metadata_tag_list_delete(tag);
        return TRUE;
    }

    /* We shouldn't write more than the buffer size in one go */
#ifdef METADATA_DEBUG_TRANSPORT
    if (octets_pre_written + octets_post_written > mlist->buffer_size)
    {
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
        fault_diatribe(
            FAULT_AUDIO_METADATA_APPEND_EXCEEDS_SPACE,
            ((caller_addr!=0)?caller_addr:return_addr));
#else
        L2_DBG_MSG2("AUDIO_METADATA_APPEND_EXCEEDS_SPACE buff 0x%08x caller 0x%08x",
                    buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT*/
    }
#endif /* METADATA_DEBUG_TRANSPORT */

    PL_ASSERT(octets_pre_written + octets_post_written <= mlist->buffer_size);

    if (tag != NULL)
    {
#ifdef METADATA_DEBUG_TRANSPORT
#if ((METADATA_DEBUG_TRANSPORT+0) > 1)
        /* An extra check to make sure the tags to be written fit in the
         * destination buffer. It is normally disabled to avoid the CPU cost.
         * Define METADATA_DEBUG_TRANSPORT to 2 or a larger number to enable.
         */
        {
            metadata_tag *temp_tag = tag;
            unsigned available_space = buff_metadata_available_space(buff);
            unsigned total_length = octets_pre_written + octets_post_written;
            while (temp_tag != NULL)
            {
                if (temp_tag->next != NULL)
                {
                    total_length += temp_tag->length;
                }
                temp_tag = temp_tag->next;
            }
            if (available_space < total_length)
            {
                L2_DBG_MSG5("buff_metadata_append: wp %d rp %d sz %d ob %d oa %d",
                            mlist->prev_wr_index, mlist->prev_rd_index,
                            mlist->buffer_size, octets_pre_written,
                            octets_post_written);
                L2_DBG_MSG2( "buff_metadata_append: "
                             "available_space %d < total_length %d",
                             available_space, total_length);
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                fault_diatribe(FAULT_AUDIO_METADATA_APPEND_EXCEEDS_SPACE,
                               ((caller_addr != 0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                L2_DBG_MSG2("AUDIO_METADATA_APPEND_EXCEEDS_SPACE buff 0x%08x caller 0x%08x",
                            buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
            }
        }
#endif /* METADATA_DEBUG_TRANSPORT > 1 */
#endif /* METADATA_DEBUG_TRANSPORT */

        do
        {
            metadata_tag *lp_tag;
            unsigned buffsize, index;

#ifdef METADATA_DEBUG_TRANSPORT
            switch (mlist->metadbg_transport_state)
            {
            case METADBG_TRANSPORT_STARTING:
                mlist->metadbg_transport_state = METADBG_TRANSPORT_NORMAL;
                break;
            case METADBG_TRANSPORT_EMPTY:
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                fault_diatribe(FAULT_AUDIO_METADATA_APPEND_MISALIGNED,
                            ((caller_addr != 0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                L2_DBG_MSG2("AUDIO_METADATA_APPEND_MISALIGNED buff 0x%08x caller 0x%08x",
                            buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
                break;
            case METADBG_TRANSPORT_NORMAL:
            default:
                break;
            }
#endif

            /* If this is the last one in the CLL then re-use the existing tags,
             * otherwise we make a copy. */
            if (mlist->next == mlist_start)
            {
                lp_tag = tag;
            }
            else
            {
                /* Make a copy of the list. The existing list is NULL terminated so
                 * don't need to do that, it'll naturally get copied as NULL.*/
                metadata_tag *new, *tail, *tag_2_cpy = tag;
                tail = lp_tag = NULL;

                while (tag_2_cpy != NULL)
                {
                    new = buff_metadata_copy_tag(tag_2_cpy);

                    if (lp_tag == NULL)
                    {
                        tail = lp_tag = new;
                    }
                    else
                    {
                        tail->next = new;
                        tail = new;
                    }
                    tag_2_cpy = tag_2_cpy->next;
                }
            }

            buffsize = mlist->buffer_size;
            index = mlist->prev_wr_index + octets_pre_written;

            if (index >= buffsize)
            {
                index -= buffsize;
            }
#ifdef METADATA_DEBUG_TRANSPORT
            if (mlist->last_tag_still_covers > octets_pre_written)
            {
                L2_DBG_MSG3("buff_metadata_append: new tag in previous tag's "
                            "range: still_covered %d oct_bf %d tag_len %d",
                            mlist->last_tag_still_covers, octets_pre_written,
                            tag->length);
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                fault_diatribe(FAULT_AUDIO_METADATA_APPEND_MISALIGNED,
                               ((caller_addr != 0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                L2_DBG_MSG2("AUDIO_METADATA_APPEND_MISALIGNED buff 0x%08x caller 0x%08x",
                            buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
            }
            else if (index != mlist->next_tag_index)
            {
                L2_DBG_MSG5("buff_metadata_append: "
                            "next_tag_index %d != index %d "
                            "(prev_wr_index %d oct_pre %d bsz %d)",
                            mlist->next_tag_index, index, mlist->prev_wr_index,
                            octets_pre_written, buffsize);
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                fault_diatribe(FAULT_AUDIO_METADATA_APPEND_MISALIGNED,
                               ((caller_addr != 0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                L2_DBG_MSG2("AUDIO_METADATA_APPEND_MISALIGNED buff 0x%08x caller 0x%08x",
                            buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
            }
#endif /* METADATA_DEBUG_TRANSPORT */
#if defined(COMMON_SHARED_HEAP)
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
            pl_hwlock_get_with_retry(&metadata_locks[METADATA_TRANSPORT_LOCK],
                                     buff_metadata_num_retries);
#else
            hal_hwsemaphore_get_with_retry(HWSEMIDX_METADATA_TRANSPORT, buff_metadata_num_retries);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#else
            LOCK_INTERRUPTS;
#endif /* COMMON_SHARED_HEAP */
            while (lp_tag != NULL)
            {
                metadata_tag *nxt_tag;
                lp_tag->index = index;

                /* effectively appending the tag */
                if (mlist->tags.head == NULL)
                {
                    /* Empty list, tail should also be NULL */
                    PL_ASSERT(mlist->tags.tail == NULL);
                    mlist->tags.head = mlist->tags.tail = lp_tag;
                }
                else
                {
                    /* Non-empty list, tail won't be NULL */
                    PL_ASSERT(mlist->tags.tail != NULL);
                    mlist->tags.tail->next = lp_tag;
                    mlist->tags.tail = lp_tag;
                }

                nxt_tag = lp_tag->next;

#ifdef METADATA_DEBUG_TRANSPORT
                {
                    unsigned next_index = index + lp_tag->length;
                    if (next_index >= buffsize)
                    {
                        next_index -= buffsize;
                    }
                    if (nxt_tag != NULL)
                    {
                        index = next_index;
                    }
                    else
                    {
                        /* If tag spans beyond end of this transfer,
                         * it may wrap.
                         */
                        if (lp_tag->length < octets_post_written)
                        {
                            mlist->last_tag_still_covers = 0;
                            L2_DBG_MSG2("buff_metadata_append: last tag "
                                        "leaves gap last_tag->len %d < "
                                        "oct_post %d",
                                        lp_tag->length, octets_post_written);
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                            fault_diatribe(
                                FAULT_AUDIO_METADATA_APPEND_MISALIGNED,
                                ((caller_addr!=0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                            L2_DBG_MSG2("AUDIO_METADATA_APPEND_MISALIGNED buff 0x%08xcaller 0x%08x",
                                        buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
                        }
                        else
                        {
                            mlist->last_tag_still_covers =
                                    lp_tag->length - octets_post_written;
                        }
                        mlist->next_tag_index = next_index % buffsize;
                    }
                }
#else /* METADATA_DEBUG_TRANSPORT */
                if (nxt_tag != NULL)
                {
                    index += lp_tag->length;
                    if (index >= buffsize)
                    {
                        index -= buffsize;
                    }
                }
#endif /* METADATA_DEBUG_TRANSPORT */
                lp_tag = nxt_tag;
            }

            index = index + octets_post_written;
            if (index >= buffsize)
            {
                index -= buffsize;
            }
            mlist->prev_wr_index = index;
#if defined(COMMON_SHARED_HEAP)
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
            pl_hwlock_rel(&metadata_locks[METADATA_TRANSPORT_LOCK]);
#else
            hal_hwsemaphore_rel(HWSEMIDX_METADATA_TRANSPORT);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#else
            UNLOCK_INTERRUPTS;
#endif /* COMMON_SHARED_HEAP */
        mlist = mlist->next;
        } while (mlist != mlist_start);
    }
    else /* The tag is NULL so just update the write index */
    {
        do
        {
            unsigned index = mlist->prev_wr_index + octets_pre_written + octets_post_written;
            unsigned buffsize = mlist->buffer_size;
            if (index >= buffsize)
            {
                index -= buffsize;
            }

#ifdef METADATA_DEBUG_TRANSPORT
            /* If this is the first time mlist is accessed, update from  */
            /* starting to empty (we expect only NULL tags from now on). */
            if (mlist->metadbg_transport_state == METADBG_TRANSPORT_STARTING)
            {
                mlist->metadbg_transport_state = METADBG_TRANSPORT_EMPTY;
            }
            else
            if (mlist->metadbg_transport_state == METADBG_TRANSPORT_EMPTY)
            {
                /* NULL tag, empty transport, nothing to update */
            }
            else
            if (mlist->last_tag_still_covers >=
                    (octets_pre_written + octets_post_written))
            {
                mlist->last_tag_still_covers -=
                        (octets_pre_written + octets_post_written);
            }
            else
            {
                mlist->last_tag_still_covers = 0;
                L2_DBG_MSG5("buff_metadata_append: expected next tag at "
                            "next_idx %d prev_wr %d bsz %d "
                            "oct_bf %d oct_af %d",
                            mlist->next_tag_index, mlist->prev_wr_index,
                            buffsize, octets_pre_written, octets_post_written);
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
                fault_diatribe(
                    FAULT_AUDIO_METADATA_APPEND_MISALIGNED,
                    ((caller_addr!=0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
                L2_DBG_MSG2("AUDIO_METADATA_APPEND_MISALIGNED buff 0x%08x caller 0x%08x",
                            buff, ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
            }
#endif /* METADATA_DEBUG_TRANSPORT */

            mlist->prev_wr_index = index;

            mlist = mlist->next;
        } while (mlist != mlist_start);
    }
    return TRUE;
}

metadata_tag *buff_metadata_peek(tCbuffer *buff)
{
    if (buff != NULL && buff->metadata != NULL)
    {
        return buff->metadata->tags.head;
    }

    return NULL;
}

metadata_tag *buff_metadata_peek_ex(tCbuffer *buff, unsigned *octets_b4idx)
{
    if (buff != NULL && buff->metadata != NULL)
    {
        metadata_list *mlist = buff->metadata;

        metadata_tag* tag = mlist->tags.head;
        if (tag != NULL)
        {
            int before_idx = tag->index - mlist->prev_rd_index;
            if (before_idx < 0)
            {
                before_idx += mlist->buffer_size;
            }
            *octets_b4idx = before_idx;
        }
        return tag;
    }
    return NULL;
}

/**
 * \brief  Search for METADATA_PACKET_START_MASK and METADATA_PACKET_END_MASK
 *         flags to compute the frame length based on the tags length. The tags
 *         are not removed from the buffer. The first verified tag must have
 *         METADATA_PACKET_START_MASK set. Also, it is valid for one tag to
 *         have both flags set.
 *
 * \param  buff          Pointer to buffer
 * \param  octets_b4idx  Number of bytes before the first tag having
 *                       METADATA_PACKET_START_MASK flag set.
 *
 * \return  The calculated frame size by summing up the tags length.
 */
unsigned buff_metadata_peek_frame_bytes(tCbuffer *buff, unsigned *octets_b4idx)
{
    metadata_tag *tag;
    unsigned frame_len = 0;
    bool found_start = FALSE, found_stop = FALSE;

    *octets_b4idx = 0;

    tag = buff_metadata_peek_ex(buff, octets_b4idx);
    if (tag == NULL)
    {
        return frame_len;
    }

    for (; tag != NULL; tag = tag->next)
    {
        if (METADATA_PACKET_START(tag))
        {
            if (!found_start)
            {
                found_start = TRUE;
            }
            else
            {
                /* This is not the first PACKET_START flag that we encounter,
                 * so add the octets up to this point to the discard pile.
                 * Then, start counting the octets corresponding to a frame,
                 * again.
                 */
                *octets_b4idx += frame_len;
                frame_len = 0;
            }
        }
        if (found_start)
        {
            frame_len += tag->length;
            if (METADATA_PACKET_END(tag))
            {
                found_stop = TRUE;
                break;
            }
        }
        else
        {
            *octets_b4idx += tag->length;
        }
    }

    if (!found_stop)
    {
        frame_len = 0;
    }

    return frame_len;
}


unsigned buff_metadata_available_octets(tCbuffer *buff)
{
    unsigned available_octets;
    metadata_list *mlist;
    unsigned buffsize;

    /* Sanity check. */
    PL_ASSERT(buff != NULL);

    mlist = buff->metadata;
    PL_ASSERT(mlist != NULL);
    buffsize = mlist->buffer_size;

    /* This test below relies on integer underflow and the fact that addition
     * of buffsize will result in integer overflow and yield the desired result. */
    available_octets = mlist->prev_wr_index - mlist->prev_rd_index;
    if (available_octets >= buffsize)
    {
        available_octets += buffsize;
    }

    return available_octets;
}

unsigned buff_metadata_available_space(tCbuffer *buff)
{
    unsigned available_space;
    metadata_list *mlist;
    unsigned buffsize;

    /* Sanity check. */
    PL_ASSERT(buff != NULL);

    mlist = buff->metadata;
    PL_ASSERT(mlist != NULL);
    buffsize = mlist->buffer_size;

    available_space = buffsize - buff_metadata_available_octets(buff) - 1;

    return available_space;
}


RUN_FROM_PM_RAM
#ifdef METADATA_DEBUG_TRANSPORT
metadata_tag *buff_metadata_remove_dbg(tCbuffer *buff, unsigned octets_consumed, unsigned *octets_b4idx, unsigned *octets_afteridx, unsigned caller_addr)
#else /* METADATA_DEBUG_TRANSPORT */
metadata_tag *buff_metadata_remove(tCbuffer *buff,
                         unsigned octets_consumed, unsigned *octets_b4idx, unsigned *octets_afteridx)
#endif /* METADATA_DEBUG_TRANSPORT */
{
#ifdef METADATA_DEBUG_TRANSPORT
    unsigned return_addr = pl_get_return_addr();
#endif /* METADATA_DEBUG_TRANSPORT */
    metadata_list *mlist;

    patch_fn(metadata_remove);

    if (buff == NULL)
    {
        return NULL;
    }

    mlist = buff->metadata;

    if (mlist != NULL)
    {
        unsigned last_tag_idx = 0;
        unsigned prev_idx = mlist->prev_rd_index;
        unsigned rd_idx = prev_idx + octets_consumed;
        unsigned buffsize = mlist->buffer_size;
        metadata_tag **new_head = &(mlist->tags.head);

        /* The operation shouldn't be trying to wrap the buffer. */
        PL_ASSERT(octets_consumed <= buffsize);

        /* Check if there is enough data.*/
        unsigned avail = buff_metadata_available_octets(buff);
        if (avail < octets_consumed)
        {
            /* This used to be an ASSERT, but it fired too often,
                * so for now just log the error condition and hope we recover
                */
            L2_DBG_MSG3("buff_metadata_remove buff 0x%08x consumed = %d avail = %d", buff, octets_consumed, avail);
#ifdef METADATA_DEBUG_TRANSPORT
#ifdef METADATA_DEBUG_TRANSPORT_FAULT
            fault_diatribe(FAULT_AUDIO_METADATA_REMOVE_EXCEEDS_AVAILABLE,
                           ((caller_addr != 0)?caller_addr:return_addr));
#else /* METADATA_DEBUG_TRANSPORT_FAULT */
            L2_DBG_MSG1("AUDIO_METADATA_REMOVE_EXCEEDS_AVAILABLE caller 0x%08x",
                        ((caller_addr != 0)?caller_addr:return_addr));
#endif /* METADATA_DEBUG_TRANSPORT_FAULT */
#endif /* METADATA_DEBUG_TRANSPORT */
        }

        if (*new_head != NULL)
        {
            int octets_2idx = (*new_head)->index - prev_idx;

            if (octets_2idx < 0)
            {
                octets_2idx += buffsize;
            }
            *octets_b4idx = octets_2idx;

            /* If this remove will wrap the index then find all tags that
             * precede the end of the buffer before looking for those that
             * come post wrap */
            if (rd_idx >= buffsize)
            {
                rd_idx -= buffsize;
                /* prev_idx points to the next place to read hence >= */
                while (*new_head != NULL && (*new_head)->index >= prev_idx)
                {
                    last_tag_idx = (*new_head)->index;
                    new_head = &(*new_head)->next;
                }
                prev_idx = 0;
            }
            /* Look for any tags between the current position and where
             * we're removing up to */
            while (*new_head != NULL
                  && (*new_head)->index >= prev_idx
                  && (*new_head)->index < rd_idx )
            {
                last_tag_idx = (*new_head)->index;
                new_head = &(*new_head)->next;
            }
            /* also remove any zero-length tags (such as STREAM_END) right at the end */
            if( (*new_head) != NULL  && 0 == (*new_head)->length
                    && (*new_head)->index == rd_idx )
            {
                last_tag_idx = (*new_head)->index;
                new_head = &(*new_head)->next;
            }

            mlist->prev_rd_index = rd_idx;
            /* If the new head value is the same as the old one then nothing to remove */
            if (mlist->tags.head == *new_head)
            {
                *octets_b4idx = octets_consumed;
                *octets_afteridx = 0;
                return NULL;
            }
            else
            {
                metadata_tag *ret_mtag = mlist->tags.head;

                if ((*new_head) == NULL)
                {
                    metadata_tag * volatile * vol_new_head = new_head;
                    /* Removing the last element in the list need to do this atomically
                     * otherwise an append could pre-empt us and the pointer gets lost */
#if defined(COMMON_SHARED_HEAP)
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
                    pl_hwlock_get_with_retry(&metadata_locks[METADATA_TRANSPORT_LOCK],
                                             buff_metadata_num_retries);
#else
                    hal_hwsemaphore_get_with_retry(HWSEMIDX_METADATA_TRANSPORT,
                                                       buff_metadata_num_retries);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#else
                    LOCK_INTERRUPTS;
#endif /* COMMON_SHARED_HEAP */
                    /* If append pre-empted the last check before interrupts were locked
                     * then the tail will have been updated and we should leave it alone
                     * and this becomes a snip the list operation. */
                    if ((mlist->tags.head = (metadata_tag *)(*vol_new_head)) == NULL)
                    {
                        mlist->tags.tail = NULL;
                    }
                    else
                    {
                        *new_head = NULL;
                    }
#if defined(COMMON_SHARED_HEAP)
#if CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1
                    pl_hwlock_rel(&metadata_locks[METADATA_TRANSPORT_LOCK]);
#else
                    hal_hwsemaphore_rel(HWSEMIDX_METADATA_TRANSPORT);
#endif /* CHIP_HAS_LIGHTWEIGHT_HW_LOCK == 1 */
#else
                    UNLOCK_INTERRUPTS;
#endif /* COMMON_SHARED_HEAP */
                }
                else
                {
                    /* Snip the list */
                    mlist->tags.head = *new_head;
                    *new_head = NULL;
                }
                if (rd_idx < last_tag_idx)
                {
                    rd_idx += buffsize;
                }
                *octets_afteridx = rd_idx - last_tag_idx;
                return ret_mtag;
            }
        }

        if (rd_idx >= buffsize)
        {
            rd_idx -= buffsize;
        }
        mlist->prev_rd_index = rd_idx;

    }
    *octets_b4idx = octets_consumed;
    *octets_afteridx = 0;
    return NULL;
}


/*
 * buff_metadata_enable
 * NB. returns buff->metadata=NULL if not enough memory for allocation.
 */
metadata_list *buff_metadata_enable(tCbuffer *buff)
{
    PL_ASSERT(buff != NULL);
    PL_ASSERT(buff->metadata == NULL);
#if defined(COMMON_SHARED_HEAP)
    buff->metadata = xzppnew(metadata_list, MALLOC_PREFERENCE_SHARED);
#else
    buff->metadata = xzpnew(metadata_list);
#endif /* COMMON_SHARED_HEAP */
    if (buff->metadata != NULL)
    {
        /* link to self and cnt to 1, i.e. no multi-channel */
        buff->metadata->next = buff->metadata;
        buff->metadata->ref_cnt = 1;
        /* store size locally, used in sw cbuffer wrap round adjustments */
        buff->metadata->buffer_size = cbuffer_get_size_in_octets(buff);
        /* NB. possibly not all octets are usable, when this is the case
            usable_octets must be configured (cbuffer_set_usable_octets)
            before consuming any data from this buffer */
    }
    return buff->metadata;
}

/*
 * buff_metadata_get_read_offset
 */
unsigned buff_metadata_get_read_offset(tCbuffer *buff)
{
    PL_ASSERT(buff != NULL);
    return buff->metadata->prev_rd_index;
}

/*
 * buff_metadata_get_write_offset
 */
unsigned buff_metadata_get_write_offset(tCbuffer *buff)
{
    PL_ASSERT(buff != NULL);
    return buff->metadata->prev_wr_index;
}

/*
 * buff_metadata_get_head
 */
metadata_tag* buff_metadata_get_head(tCbuffer *buff)
{
    PL_ASSERT(buff != NULL);
    return buff->metadata->tags.head;
}

#if defined(COMMON_SHARED_HEAP)
bool buff_metadata_connect_existing(tCbuffer *buff, tCbuffer *src_meta_buff, tCbuffer *sink_meta_buff)
{
    metadata_list *src_meta;
    metadata_list *sink_meta;

    patch_fn_shared(buff_metadata);

    src_meta = (src_meta_buff == NULL) ? NULL : src_meta_buff->metadata;
    sink_meta = (sink_meta_buff == NULL) ? NULL : sink_meta_buff->metadata;

    PL_ASSERT(buff != NULL);

    /* This function is called ONLY on P0. It is meant to complement the logic
     * of buff_metadata_connect() which is only called on P1 in multi-core
     * connections.
     * It is based on the following assumptions:
     * - the shadow source and sink return the metadata buffer of the head in
     * the sync list when their buffer details is called. When the metadata
     * associated with a shadow endpoint is NULL, this is the first connection
     * or the endpoints are not synced.
     * - if the metadata associated with the shadow endpoint on P0 is not NULL,
     * then it was not NULL on P1 either.
     *
     * The cases that can ocur:
     *
     * P0 SOURCE i.e. sink_meta is for the shadow EP
     * - sink_meta == NULL (which means src_meta on P1 was NULL).
     *      If the metadata buffer returned is NULL, then this connection is
     *      the first one in the sync list. P1 might have or might have not
     *      already allocated the metadata list (buff->metadata here).
     *
     *      Sub-cases:
     *      - src_meta == NULL.
     *              buff->metadata == NULL.
     *                  P1 didn't allocate any metadata. This means that
     *                  sink_meta was not NULL at that time. This is possible if
     *                  the operator on P1 was already connected to something
     *                  else. Nothing should be done here.
     *              buff->metadata != NULL.
     *                  This is the first connection between two operators.
     *                  Nothing should be done in this case, P1 took care of
     *                  everything (because src_meta would be NULL on P1 as well).
     *      - src_meta != NULL.
     *              buff->metadata == NULL.
     *                  P1 did not allocate a list because sink_meta_P1 != NULL.
     *                  Nothing to be done here, it means that both operators
     *                  on P0 and P1 have other connections, but this is the
     *                  first one with each other.
     *                  The single core scenario checks if they are part of the
     *                  same linked list. TODO COMMON_SHARED_HEAP check what
     *                  happens here. If they are, then buff->metadata is
     *                  assigned the sink_meta (which here is not available). P1
     *                  did not take care of this because src_meta was NULL at
     *                  that point (because sink_meta - shadow EP -is NULL here).
     *              buff->metadata != NULL.
     *                  This could have happened either because sink_meta was
     *                  NULL or because sink_meta and src_meta matched on P1.
     *                  However, src_meta was NULL, so the latter drops.
     *                  According to the single core scenario, if src_meta is not
     *                  NULL, then the circular linked list needs to be updated.
     *                  The single core case always allocates a list in this case,
     *                  therefore P0 must do the linking (and this is done in
     *                  this function).
     *                  Can src_meta == buff->metadata happen here? Probably not
     *                  because connect() is called after for each endpoint,
     *                  which usually sets the buffers for the endpoints.
     * - sink_meta != NULL.
     *      - src_meta == NULL.
     *              buff->metadata == NULL.
     *                  src_meta and sink_meta were != NULL on P1 and did not
     *                  match. This is an impossible case because if the shadow
     *                  endpoints are synced and have metadata, then the
     *                  operators should as well.
     *              buff->metadata != NULL.
     *                  buff->metadata is not NULL either because sink_meta on
     *                  P1 was NULL or because sink_meta and src_meta on P1
     *                  matched. The first situation is impossible because the
     *                  shadow endpoints know about a metadata list, and the
     *                  operators do not. The second case is impossible as well
     *                  because the shadow endpoints know about some metadata
     *                  and one of the operators doesn't.
     *      - src_meta != NULL.
     *             buff->metadata == NULL.
     *                  The sink_meta on P1 was not NULL and src_meta != NULL.
     *                  This means that they did not match. The chances are
     *                  that src_meta == sink_meta on P0 and nothing needs to
     *                  be done.
     *             buff->metadata != NULL.
     *                  This means that either the sink_meta on P1 was NULL
     *                  or the sink_meta and src_meta on P1 were not NULL and
     *                  matched. The first case is impossible because the
     *                  shadow endpoints are synced, which means that this is
     *                  a multi-channel connection between the same operators.
     *                  src_meta, sink_meta and buff->metadata are probably
     *                  the same.
     *
     * P0 SINK i.e. src_meta is for the shadow EP.
     *
     * The shadow source operator is different compared to the shadow sink
     * because its buffer details return the connection buffer for the metadata
     * buffer.
     * - sink_meta == NULL.
     *      This means that this is the first connection for the operator.
     *      - src_meta == NULL.
     *              buff->metadata == NULL.
     *                  This case is not possible because if src_meta here is
     *                  NULL, then sink_meta was NULL as well on P1.
     *                  buff_metadata_connect() always allocates a metadata list
     *                  if sink_meta is NULL.
     *              buff->metadata != NULL.
     *                  buff->metadata == src_meta when P0 is sink. Impossible.
     *      - src_meta != NULL.
     *              buff->metadata == NULL.
     *                  This case is not possible because this is not the first
     *                  connection between the two operators because the shadow
     *                  endpoints are synchronised.
     *              buff->metadata != NULL.
     *                  buff->metadata == src_meta.
     *                      This is the first connection between the two
     *                      operators. sink_meta is NULL, therefore this list
     *                      was allocated correctly. This shouldn't be
     *                      overwritten by the first if statement below, so
     *                      add the condition that buff->metadata and src_meta
     *                      must not be the same.
     *                  buff->metadata != src_meta.
     *                      A list was allocated on P1, which is impossible
     *                      because sink_meta_P1 was not NULL. Impossible case.
     * - sink_meta != NULL.
     *      - src_meta == NULL.
     *              buff->metadata == NULL.
     *                  This is an impossible case because sink_meta on P1
     *                  would have been NULL as well, so a buff->metadata
     *                  would have been allocated there.
     *              buff->metadata != NULL.
     *                  Not possible because src_meta cannot be NULL if
     *                  buff->metadata != NULL.
     *      - src_meta != NULL.
     *              buff->metadata != NULL.
     *                  buff->metadata = src_meta
     *                      On P1, sink_meta was NULL.
     *                      Therefore, a new list was created and linked.
     *                      However, the sink_meta (on P0) is not NULL,
     *                      therefore the list shouldn't have been allocated.
     *                      The question remains whether the src_meta on P1 was
     *                      NULL or not. Luckily, the src_meta from P1 has been
     *                      linked to this list, so it can be checked if the
     *                      src_meta from P1 matches sink_meta on P0.
     *                      buff->metadata should be deleted and the check should
     *                      be made for src_meta(P1) and sink_meta(P0).
     *                  buff->metadata = src_meta = sink_meta.
     *                      This is not the first connection between the two
     *                      operators. Don't do anything.
     *                  buff->metadata != src_meta.
     *                      This case is impossible because sink_meta_P1 would
     *                      not have been NULL, so a new list would not have
     *                      been allocated (the shadow endpoints are synced).
     *                      If sink_meta_P1 matches src_meta_P1, then
     *                      buff->metadata = sink_meta_p1, which in turn is
     *                      equal to src_meta_P0.
     *              buff->metadata == NULL.
     *                  This case is not possible because this is not the first
     *                  connection between the two operators because the shadow
     *                  endpoints are synchronised.
     */
    if (sink_meta == NULL)
    {
        /* Update the circular linked list */
        if ((src_meta != NULL) && (buff->metadata != NULL) && (src_meta != buff->metadata))
        {
            /* In a single core scenario,  */
            /* Insert into the list */
            buff->metadata->next = src_meta->next;
            src_meta->next = buff->metadata;
        }
    }
    else
    {
        /* This case can only happen when P0 is sink. Check above. */
        if ((src_meta == buff->metadata) && (buff->metadata != NULL) &&
                (src_meta != sink_meta))
        {
            metadata_list *next_list;
            /* next_list is used to represent the src_meta on P1. */
            next_list = buff->metadata->next;

            /* Release the list that was not supposed to be allocated. */
            buff_metadata_release(buff);

            if (next_list != NULL)
            {
                /* Need to check here if src & sink are in the same list */
                metadata_list *start = sink_meta;
                bool match = FALSE;
                do
                {
                    if (next_list == sink_meta)
                    {
                        match = TRUE;
                        /* The metadata is to be shared with other buffers so re-use the
                         * metadata provided by the sink in the connection as that's the
                         * one in the list that represents this direction of flow. */
                        buff->metadata = start;
                        break;
                    }
                    sink_meta = sink_meta->next;
                } while (sink_meta != start);

                if (match)
                {
                    buff->metadata->ref_cnt++;
                }
            }
        }
    }

    return TRUE;
}
#endif /* COMMON_SHARED_HEAP */

bool buff_metadata_connect(tCbuffer *buff, tCbuffer *src_meta_buff, tCbuffer *sink_meta_buff)
{
    metadata_list *src_meta;
    metadata_list *sink_meta;

    patch_fn_shared(buff_metadata);

    /* The buffer pointers can be NULL as well as the metadata itself being NULL
     * luckily the logic is exactly the same whether the buffer or the metadata
     * pointer is NULL, what is really needed is the metadata pointer if it is
     * non-NULL. Extract this first and then work with the result as it makes
     * the rest of the logic a lot less convoluted. */
    src_meta = (src_meta_buff == NULL) ? NULL : src_meta_buff->metadata;
    sink_meta = (sink_meta_buff == NULL) ? NULL : sink_meta_buff->metadata;

    PL_ASSERT(buff != NULL);
    PL_ASSERT(buff->metadata == NULL);

    if (sink_meta == NULL)
    {
        buff->metadata = buff_metadata_enable(buff);
        if (buff->metadata == NULL)
        {
            return FALSE;
        }

        /* Update the circular linked list */
        if (src_meta == NULL)
        {
            /** It's only thing in the list, make it point to itself */
            buff->metadata->next = buff->metadata;
        }
        else
        {
            /* Insert into the list */
            buff->metadata->next = src_meta->next;
            src_meta->next = buff->metadata;
        }
    }
    else
    {
        /* Need to check here if src & sink are in the same list */
        metadata_list *start = sink_meta;
        bool match = FALSE;
        do
        {
            if (src_meta == sink_meta)
            {
                match = TRUE;
                /* The metadata is to be shared with other buffers so re-use the
                 * metadata provided by the sink in the connection as that's the
                 * one in the list that represents this direction of flow. */
                buff->metadata = start;
                break;
            }
            sink_meta = sink_meta->next;
        } while (sink_meta != start);

        if (match)
        {
            buff->metadata->ref_cnt++;
        }
        /* else (if not 'match') means there is already metadata
         * being supplied to this entity and it can't
         * accept any more on this connection, so don't configure the buffer for
         * metadata and append will release the tags as they are appended to this
         * buffer. */
    }
    return TRUE;
}

void buff_metadata_release(tCbuffer *buff)
{
    metadata_list *meta;

    patch_fn_shared(buff_metadata);

    if (buff == NULL || !buff_has_metadata(buff))
    {
        return;
    }

    /* Metadata list pointer should be valid if we make it to here */
    PL_ASSERT(buff->metadata != NULL);

    meta = buff->metadata;
    buff->metadata = NULL;

    meta->ref_cnt--;
    if (meta->ref_cnt <= 0)
    {
        /* To remove the element from the circularly LL we need to find the
         * element that precedes meta and make it point to the next element. */
        metadata_list *end = meta->next;
        while (end->next != meta)
        {
            end = end->next;
        }
        end->next = meta->next;

        /* If there are any meta data tags attached free them before we release
         * the metadata_list structure. */
        buff_metadata_tag_list_delete(meta->tags.head);

        /* Now that meta is no longer in the Circular LL it can be freed */
        pdelete(meta);
    }
}


/* metadata_strict_transport
    for operators that have an opinion about how much they want to process
 */
void metadata_strict_transport(tCbuffer *src, tCbuffer *dst, unsigned trans_octets)
{
    metadata_strict_transport_filter(src, dst, trans_octets, NULL);
}

/* metadata_strict_transport_filter
    for operators that have an opinion about how much they want to process
 */
void metadata_strict_transport_filter( tCbuffer *src, tCbuffer *dst, unsigned trans_octets, metadata_tag_list_filter* filter)
{
#ifdef METADATA_DEBUG_TRANSPORT
    unsigned return_addr = pl_get_return_addr();
#endif /* METADATA_DEBUG_TRANSPORT */
    metadata_tag *ret_mtag;
    unsigned b4idx, afteridx;
    bool has_output_metadata = (dst != NULL)&&buff_has_metadata(dst);

    patch_fn_shared(buff_metadata);

    if (trans_octets == 0)
    {
        L2_DBG_MSG("metadata_strict_transport: ignoring zero transfer");
        return;
    }

    if ((src != NULL)&&buff_has_metadata(src))
    {
        /* transport metadata, first (attempt to) consume tag associated with src */
#ifdef METADATA_DEBUG_TRANSPORT
        ret_mtag = buff_metadata_remove_dbg(src, trans_octets, &b4idx,
                                            &afteridx, return_addr);
#else /* METADATA_DEBUG_TRANSPORT */
        ret_mtag = buff_metadata_remove(src, trans_octets, &b4idx, &afteridx);
#endif /* METADATA_DEBUG_TRANSPORT */
    }
    else
    {
        b4idx = 0;
        afteridx = trans_octets;
        if (has_output_metadata)
        {
            /* Create a tag for the transfered data.
             * The created tag is an empty tag and it should be left like that. */
            ret_mtag = buff_metadata_new_tag();
            /* Set the tag size. */
            ret_mtag->length = trans_octets;
        }
        else
        {
            /* No need to create a tag because the destination has no metadata.*/
            ret_mtag = NULL;
        }
    }

    if (has_output_metadata)
    {
        /* Optionally pass the tags through the supplied filter */
        if (NULL != filter)
        {
            (filter->filter)(filter->data, ret_mtag, &b4idx, &afteridx);
        }

        /* Even if the src is a NULL buffer we append to dst. It makes no sense
         * for the current configuration. However if another connection is made
         * later to the src which does support metadata the dst metadata write
         * pointer needs to be at the right offset. */
#ifdef METADATA_DEBUG_TRANSPORT
        buff_metadata_append_dbg(dst, ret_mtag, b4idx, afteridx, return_addr);
#else /* METADATA_DEBUG_TRANSPORT */
        buff_metadata_append(dst, ret_mtag, b4idx, afteridx);
#endif /* METADATA_DEBUG_TRANSPORT */
    }
    else
    {
        buff_metadata_tag_list_delete(ret_mtag);
    }
}

#if !defined(COMMON_SHARED_HEAP)
#if defined(SUPPORTS_MULTI_CORE)
metadata_tag* buff_metadata_pop_tags_from_KIP(tCbuffer* shared_buffer,
                                              metadata_tag** tail)
{
    metadata_tag *first_t = buff_metadata_kip_tag_from_buff(shared_buffer);
    metadata_tag *t = first_t;

    patch_fn_shared(kip_metadata);

    while (t != NULL)
    {
        if (METADATA_STREAM_END(t))
        {
            buff_metadata_kip_prepare_eof_after_remove(t);
        }

        t->next = buff_metadata_kip_tag_from_buff(shared_buffer);

        /* This bit is not necessary for this while-loop but we need the tail
         * pointer for the next step. */
        if (t->next == NULL)
        {
            break;
        }
        else
        {
            t = t->next;
        }
    }

    *tail = t;
    return first_t;
}

metadata_tag* buff_metadata_push_tags_to_KIP(tCbuffer* shared_buffer,
                                             metadata_tag* first_tag)
{
    metadata_tag* temp_tag;

    patch_fn_shared(kip_metadata);

    metadata_tag* tag = first_tag;
    while (NULL != tag &&
           buff_metadata_kip_tag_to_buff(shared_buffer, tag))
    {
        temp_tag = tag->next;
        buff_metadata_delete_tag(tag, FALSE);
        tag = temp_tag;
    }

    /* If we manage to push everything the KIP buffer this will be NULL */
    return tag;
}
#endif /* defined(SUPPORTS_MULTI_CORE) */
#endif /* !COMMON_SHARED_HEAP */

#ifdef DESKTOP_TEST_BUILD
void print_metadata(metadata_list *meta)
{
    if (meta == NULL)
    {
        printf("Metadata is NULL\n");
        return;
    }
    printf("Metadata with address %p\n", meta);
    printf("\tTag list tags.head: %p, tail: %p\n", meta->tags.head, meta->tags.tail);
    printf("\tReference count: %d\n", meta->ref_cnt);
    printf("\tNext Metadata: %p\n", meta->next);

}

void print_metadata_tag(metadata_tag *tag)
{
    if (tag == NULL)
    {
        printf("Tag is NULL\n");
        return;
    }
    printf("Metadata tag with address %p\n", tag);
    printf("\tNext Tag: %p\n", tag->next);
    printf("\tBuffer index (octets): %d\n", tag->index);
    printf("\tData length: %d\n", tag->length);
    printf("\tFlags: %x\n", tag->flags);
    printf("\tTimestamp: %d\n", tag->timestamp);
}

#endif /* DESKTOP_TEST_BUILD */
