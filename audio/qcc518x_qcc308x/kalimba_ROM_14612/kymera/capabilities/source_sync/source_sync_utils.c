/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  source_sync_utils.c
 * \ingroup  capabilities
 *
 *  src_sync operator
 *  small datatypes implementation
 *
 */

#include "source_sync_defs.h"

static unsigned src_sync_get_buffer_history_minmax(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age, bool get_max);

/**
 * SRC_SYNC_BUFFER_LEVEL_HISTORY structures maintain a circular buffer
 * of unsigned values with timestamps for the time they were recorded.
 * They allow calculating the minimum/maximum of the saved values which
 * are not older than a given limit.
 */
bool src_sync_alloc_buffer_history(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, unsigned num_entries)
{
    if (hist->num_entries > 0)
    {
        PL_ASSERT(hist->entries != NULL);
    }

    if (num_entries > SRC_SYNC_BUFFER_LEVEL_HISTORY_MAX)
    {
        return FALSE;
    }

    if (hist->num_entries >= num_entries)
    {
        /* Not much to do. Record the new size as it will be used when reading.
         */
        hist->num_entries = num_entries;
        return TRUE;
    }
    else
    {
        SRC_SYNC_BUFFER_LEVEL_RECORD* old_records;
        SRC_SYNC_BUFFER_LEVEL_RECORD* new_records = xzpnewn(num_entries, SRC_SYNC_BUFFER_LEVEL_RECORD);

        if (new_records != NULL)
        {
            unsigned i;
            TIME past;
            /* Calculate a clock value a second ago; expected to wrap
             * during the first second after the clock itself has
             * started/wrapped.
             */
            past = time_get_time() - SECOND;

            for (i = 0; i < num_entries; ++i)
            {
                new_records[i].timestamp = past;
            }

            old_records = hist->entries;
            hist->num_entries = num_entries;
            hist->next_wr_pos = 0;
            hist->entries = new_records;

            pfree(old_records);

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

void src_sync_free_buffer_history(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist)
{
    SRC_SYNC_BUFFER_LEVEL_RECORD* old_records;

    old_records = hist->entries;
    hist->entries = NULL;
    hist->num_entries = 0;

    pfree(old_records);
}

void src_sync_put_buffer_history(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME t, unsigned amount)
{
    SRC_SYNC_BUFFER_LEVEL_RECORD* rec;

    PL_ASSERT((hist->num_entries > 0) && (hist->entries != NULL));

    rec = &(hist->entries[hist->next_wr_pos]);
    rec->timestamp = t;
    rec->amount = amount;
    hist->next_wr_pos += 1;
    if (hist->next_wr_pos >= hist->num_entries)
    {
        hist->next_wr_pos = 0;
    }
}

static unsigned src_sync_get_buffer_history_minmax(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age, bool get_max)
{
    SRC_SYNC_BUFFER_LEVEL_RECORD* rec;
    unsigned i;
    unsigned min_amount;
    unsigned max_amount;

    PL_ASSERT((hist->num_entries > 0) && (hist->entries != NULL));

    max_amount = 0;
    min_amount = MAXINT;

    for ( i = 0, rec = &hist->entries[0];
          i < hist->num_entries;
          i += 1, rec += 1 )
    {
        /* rec->timestamp values are expected to be from the recent past
         * (some milliseconds). So wrapping occurs naturally when the
         * hardware clock wraps, and the correct result is a small positive
         * number. If the result's MSB is set, this is very old
         * (> about 35min), not a reversed timeline.
         */
        TIME age = now - rec->timestamp;
        if (age <= max_age)
        {
            min_amount = pl_min(min_amount, rec->amount);
            max_amount = pl_max(max_amount, rec->amount);
        }
    }

    if (get_max)
    {
        return max_amount;
    }
    else
    {
        return min_amount;
    }
}

unsigned src_sync_get_buffer_history_min(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age)
{
    return src_sync_get_buffer_history_minmax(hist, now, max_age, FALSE);
}

unsigned src_sync_get_buffer_history_max(SRC_SYNC_BUFFER_LEVEL_HISTORY* hist, TIME now, TIME max_age)
{
    return src_sync_get_buffer_history_minmax(hist, now, max_age, TRUE);
}

char src_sync_sign_and_magnitude(int value, unsigned* magnitude)
{
    if (value >= 0)
    {
        *magnitude = value;
        return '+';
    }
    else
    {
        *magnitude = (unsigned)(-value);
        return '-';
    }
}

/** Macro to get terminal group element from array. */
#define GET_ELEMENT_IN_ARRAY(ARRAY, POS, ELM_SIZE)  ((SRC_SYNC_TERMINAL_GROUP*)((uintptr_t)ARRAY + (POS)*(ELM_SIZE)))

/**
 * Function allocates source and sink groups. It also
 * sets up the group elements in a linked list.
 *
 * \param num_groups number of groups to allocate.
 * \param is_sink    Boolean showing if Sink or Source groups
 *                   should be allocated.
 * \return Pointer to the group.
 */
SRC_SYNC_TERMINAL_GROUP* src_sync_alloc_groups(unsigned num_groups, unsigned struct_size)
{
    SRC_SYNC_TERMINAL_GROUP* groups;
    SRC_SYNC_TERMINAL_GROUP* grp_ptr;
    unsigned i;

    /* Fixup opportunity.
     * If patch needs bigger source or sink group change the struct_size. */
    patch_fn_shared(src_sync_cfg);

    /* For now xzpnewn is defined as
     * #define xzpnewn(n, t)       ((t *)( xzpmalloc ( sizeof(t) * (n) )))
     * However, this might change in the future. Keep this
     * code up to date with pl_malloc.h  */
    groups = xzpmalloc(struct_size*num_groups);
    if (groups == NULL)
    {
        return NULL;
    }

    /* Set up the array elements as a list.
     * The last element's next will be set to NULL after the loop.
     * Hence the loop goes until the penultimate element. */
    for (i = 0; i < num_groups - 1 ; i++)
    {
        grp_ptr = GET_ELEMENT_IN_ARRAY(groups, i, struct_size);
        grp_ptr->next = GET_ELEMENT_IN_ARRAY(groups, i+1, struct_size);
    }
    /* Mark the last element as the tail. */
    grp_ptr = GET_ELEMENT_IN_ARRAY(groups, num_groups-1, struct_size);
    grp_ptr->next = NULL;
    return groups;
}

/* Parameter conversions from kick-period-relative to fractional seconds */
RATE_SECOND_INTERVAL src_sync_get_period(const SRC_SYNC_OP_DATA *op_extra_data)
{
    return frac_mult(op_extra_data->cur_params.OFFSET_SS_PERIOD, op_extra_data->system_kick_period);
}

RATE_SECOND_INTERVAL src_sync_get_max_period(const SRC_SYNC_OP_DATA *op_extra_data)
{
    return frac_mult(op_extra_data->cur_params.OFFSET_SS_MAX_PERIOD, op_extra_data->system_kick_period << 5);
}

RATE_SECOND_INTERVAL src_sync_get_max_latency(const SRC_SYNC_OP_DATA *op_extra_data)
{
    return frac_mult(op_extra_data->cur_params.OFFSET_SS_MAX_LATENCY, op_extra_data->system_kick_period << 5);
}

RATE_SECOND_INTERVAL src_sync_get_stall_recovery_default_fill(const SRC_SYNC_OP_DATA *op_extra_data)
{
    return frac_mult(op_extra_data->cur_params.OFFSET_SS_STALL_RECOVERY_DEFAULT_FILL, op_extra_data->system_kick_period << 5);
}
