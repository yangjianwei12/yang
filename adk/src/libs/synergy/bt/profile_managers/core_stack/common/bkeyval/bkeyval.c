/*******************************************************************************

Copyright (C) 2008 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "csr_synergy.h"

#include <stdarg.h>
#include "csr_util.h"
#include "csr_bt_bluestack_types.h"
#include "bluetooth.h"
#include "csr_bt_common.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "bkeyval.h"

/*! \brief Conditional iterator progression */
#define BKV_Progress(iterator, size, skip) \
    ((iterator) += (((iterator) + (skip) < (size)) ? (skip) : 0))

/*! \brief Count number of blocks in a kv buffer */
uint16_t BKV_CountBlocks(uint16_t *block, const uint16_t size, bool_t cross)
{
    uint16_t i;
    uint16_t c;

    for(i=0, c=0; (block != NULL) && (i<size); /*empty*/)
    {
        /* End separator */
        if(block[i] == BKV_BARRIER)
        {
            if(!cross)
            {
                /* Don't go beyond barrier */
                break;
            }
            ++i;
        }
        /* Separator */
        else if(block[i] & BKV_SEPARATOR)
        {
            ++i;
            ++c;
        }
        /* Key,value */
        else
        {
            i += BKV_KV_SPACE(block[i]);
        }
    }

    return c;
}

/*! \brief Set iterator to start of a numbered block */
bool_t BKV_JumpToBlock(BKV_ITERATOR_T *iterator, const uint16_t num)
{
    uint16_t c = 0;
    uint16_t i = 0;
    uint16_t *block = iterator->block;

    if (block != NULL)
    {
        while (i < iterator->size && c < num)
        {
            /* End separator */
            if(block[i] == BKV_BARRIER)
            {
                ++i;
            }
            /* Separator */
            else if(block[i] & BKV_SEPARATOR)
            {
                ++c;
                ++i;
            }
            /* Key,value */
            else
            {
                i += BKV_KV_SPACE(block[i]);
            }
        }
    }

    /* Only update iterator if found */
    if(c == num)
    {
        iterator->iterator = i;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*! \brief Look for the given key, alloing it to be read next time */
#ifndef BUILD_FOR_HOST
static
#endif
bool_t BKV_JumpToKey(BKV_ITERATOR_T *iterator, uint16_t key, bool_t cross)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Sanity check */
    if(iter.block == NULL || iter.size == 0)
    {
        return FALSE;
    }

    while (iter.iterator < iter.size)
    {
        /* End separator */
        if(iter.block[iter.iterator] == BKV_BARRIER)
        {
            if(!cross)
            {
                /* Not allowed to cross - bail out */
                break;
            }
            ++iter.iterator;
        }
        /* Separator */
        else if(iter.block[iter.iterator] & BKV_SEPARATOR)
        {
            ++iter.iterator;
        }
        /* Check for matching key */
        else if(iter.block[iter.iterator] == key)
        {
            break;
        }
        else
        {
            /* Proceed to next key/separator */
            iter.iterator += BKV_KV_SPACE(iter.block[iter.iterator]);
        }
    }

    /* Only update iterator if found and we're still within bounds */
    if(iter.iterator < iter.size && iter.block[iter.iterator] == key)
    {
        iterator->iterator = iter.iterator;
        return TRUE;
    }

    return FALSE;
}

/*! \brief Read separator. Update iterator */
bool_t BKV_ReadSeparator(uint16_t *block, const uint16_t size, uint16_t *iterator,
                         uint16_t *ref)
{
    if((block == NULL) || (size == 0))
    {
        return FALSE;
    }

    if((*iterator < size) &&
       (block[*iterator] & BKV_SEPARATOR))
    {
        *ref = UINT16_R(&block[*iterator], 0) & BKV_MASK_VALUE;
        BKV_Progress(*iterator, size, 1);
        return TRUE;
    }
    return FALSE;
}

/*! \brief Validate entire buffer */
bool_t BKV_Validate(uint16_t *block, const uint16_t size)
{
    uint16_t i;

    /* Empty blocks are per-se not invalid, but it doesn't make sense
     * to validate nothing... */
    if((block == NULL) || (size == 0))
    {
        return TRUE;
    }

    /* Sanity checks: Size must be enough to hold one separator and
     * one key.  Further, first item must be an iterator */
    if((size < 2) || !(block[0] & BKV_SEPARATOR))
    {
        return FALSE;
    }

    for(i=0; i<size; /*empty*/)
    {
        /* Separator */
        if(block[i] & BKV_SEPARATOR)
        {
            ++i;
        }
        /* Key,value */
        else
        {
            i += BKV_KV_SPACE(block[i]);
        }
    }

    /* Buffer valid if we've reached the end */
    return i == size;
}

/*! \brief Scan for key adhering to block referers. Updates iterator. */
static bool_t BKV_ScanBlocks(BKV_ITERATOR_T *iterator, uint16_t key)
{
    BKV_ITERATOR_T iter;
    uint16_t ref;

    /* Scan local block first as that takes precedence. Note that
     * JumpToKey will fail if block is NULL or size is 0 */
    if(BKV_JumpToKey(iterator, key, FALSE))
        return TRUE;

    /* Jump to referer if found - i.e. recursive scan! */
    iter = *iterator;
    ref = 0;
    if(BKV_ReadSeparator(iter.block, iter.size, &iter.iterator, &ref))
    {
        if(ref != 0 && BKV_JumpToBlock(&iter, ref))
        {
            if(BKV_ScanBlocks(&iter, key))
            {
                /* Found, bail out */
                iterator->iterator = iter.iterator;
                return TRUE;
            }
        }
    }

    /* Not found */
    return FALSE;
}

/*! \brief Does named key exist in this block or refered blocks */
bool_t BKV_KeyExists(const BKV_ITERATOR_T *iterator, uint16_t key)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Note that ScanBlocks is NULL/zero-size safe */
    return BKV_ScanBlocks(&iter, key);
}

static void BKV_WriteUint32(const uint16_t *block, uint32_t *value)
{
    *value = ((uint32_t)(block[0]) << 16) | block[1];
}

/*! \brief Scan for and read value for 32 bit single key */
bool_t BKV_Scan32Single(const BKV_ITERATOR_T *iterator, uint16_t key, uint32_t *value)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Note that ScanBlocks is NULL/zero-size safe */
    if(BKV_ScanBlocks(&iter, key) && iter.iterator + 2 < iter.size)
    {
        BKV_WriteUint32(iter.block + iter.iterator + 1, value);
        return TRUE;
    }

    *value = 0;
    return FALSE;
}

/*! \brief Scan for and read value for 16 bit range key */
bool_t BKV_Scan32Range(const BKV_ITERATOR_T *iterator, uint16_t key,  uint32_t *min, uint32_t *max)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Note that ScanBlocks is NULL/zero-size safe */
    if(BKV_ScanBlocks(&iter, key) && iter.iterator + 4 < iter.size)
    {
        BKV_WriteUint32(iter.block + iter.iterator + 1, min);
        BKV_WriteUint32(iter.block + iter.iterator + 3, max);

        return TRUE;
    }

    *min = 0;
    *max = 0;

    return FALSE;
}

/*! \brief Scan for and read value for 16 bit single key */
bool_t BKV_Scan16Single(const BKV_ITERATOR_T *iterator, uint16_t key, uint16_t *value)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Note that ScanBlocks is NULL/zero-size safe */
    if(BKV_ScanBlocks(&iter, key) && iter.iterator + 1 < iter.size)
    {
        *value = iter.block[iter.iterator+1];
        return TRUE;
    }

    *value = 0;
    return FALSE;
}

/*! \brief Scan for and read value for 16 bit range key */
bool_t BKV_Scan16Range(const BKV_ITERATOR_T *iterator, uint16_t key, uint16_t *min, uint16_t *max)
{
    BKV_ITERATOR_T iter = *iterator;

    /* Note that ScanBlocks is NULL/zero-size safe */
    if(BKV_ScanBlocks(&iter, key) && iter.iterator + 2 < iter.size)
    {
        *min = iter.block[iter.iterator+1];
        *max = iter.block[iter.iterator+2];
        return TRUE;
    }

    *min = 0;
    *max = 0;
    return FALSE;
}
