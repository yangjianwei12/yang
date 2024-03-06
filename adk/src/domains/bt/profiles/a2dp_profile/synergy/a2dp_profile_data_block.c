/*!
\copyright  Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_data_block.c
\brief
            This file merges different blocks of data for an a2dp link into one, to
            reduce the number of allocated malloc blocks, a restriction on all 
            supported chips due to the approach to memory fragmentation.

            Data alignment needs to be taken into account.
            * Some supported processors have constraints on memory alignment
            * Some of the data allocated is measured in bytes (potentially odd)

            Where the SIZE of an element is passed in, it is assumed the sizeof()
            operator will have been used, ensuring padding.

            Padding the total size of each block for alignment ensures that 
            the rest of the code can ignore this, and only allocation/de-allocation
            is affected.
*/

#ifdef INCLUDE_AV

#include "a2dp_profile_data_block.h"
#include "a2dp_profile.h"
#include "av_instance.h"
#include "av.h"
#include <stdlib.h>
#include <panic.h>
#include <logging.h>
#include <device_list.h>

/* Multiple memory slot operation.  Data blocks reside in their own memory slots */
#define DBLK_TYPE               a2dpDataBlockHeader
#define DBLK_HDR_SIZE           sizeof(DBLK_TYPE)
#define DBLK_BASE(av_inst)      av_inst->a2dp.data_blocks[0]
#define DBLK(av_inst)           av_inst->a2dp.data_blocks[0]->block
#define DBLK_DATA_SIZE(av_inst) av_inst->a2dp.data_blocks[0]->blockSizePadded

/* If a chip has alignment constraints, need to take these into account in the 
 * allocation sizes. Have a function to round up.
 */
#if !defined(__XAP__) || defined(HOSTED_TEST_ENVIRONMENT)
/* For Kalimba and host, pad to uint32 */
# define DBLK_PAD(x)    ((((x) + sizeof(uint32) - 1)/sizeof(uint32))*sizeof(uint32))
#else
/* No padding for XAP */
# define DBLK_PAD(x)    (x)
#endif

bool appA2dpBlockInit(avInstanceTaskData* av_inst)
{
    if (DBLK_BASE(av_inst) != NULL)
    {
        return FALSE;
    }

    DEBUG_LOG("appA2dpBlockInit");

    DBLK_BASE(av_inst) = (DBLK_TYPE *)PanicNull(malloc(DBLK_HDR_SIZE));
    memset((uint8 *)DBLK_BASE(av_inst), 0, DBLK_HDR_SIZE);

    return TRUE;
}

void appA2dpBlockDestroy(avInstanceTaskData* av_inst)
{
    if (DBLK_BASE(av_inst) == NULL)
    {
        return;
    }

    DEBUG_LOG("appA2dpBlockDestroy");

    free(DBLK_BASE(av_inst));
    DBLK_BASE(av_inst) = NULL;
}

uint8 *appA2dpBlockAdd(avInstanceTaskData* av_inst, a2dpDataBlockId blockId, uint8 elementCount, uint8 elementSize)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        a2dpDataBlockInfo *data_block = &DBLK(av_inst)[blockId];

        if (!data_block->offset && elementCount && elementSize)
        {
            unsigned block_size = elementSize * elementCount;
            unsigned padded_block_size = DBLK_PAD(block_size);
            unsigned offset = DBLK_HDR_SIZE + DBLK_DATA_SIZE(av_inst); /* New block added at end of any existing ones */
            DBLK_TYPE *new_data_pool = (DBLK_TYPE *)realloc(DBLK_BASE(av_inst), offset + padded_block_size);

            if (new_data_pool)
            {
                DBLK_BASE(av_inst) = new_data_pool;
                memset(((uint8 *)DBLK_BASE(av_inst))+offset, 0, padded_block_size);

                data_block                 = &DBLK(av_inst)[blockId];
                DBLK_DATA_SIZE(av_inst)          += (uint16)padded_block_size;
                data_block->offset         = offset;
                data_block->blockSize      = block_size;
                data_block->elementSize    = elementSize;
                data_block->currentElement = 0;

                return ((uint8 *)DBLK_BASE(av_inst))+offset;
            }
        }
    }

    /* Failed - block already exists  or  unable to allocate memory */
    DEBUG_LOG("[NULL]\n");
    return 0;
}

void appA2dpBlockRemove(avInstanceTaskData* av_inst, a2dpDataBlockId blockId)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        a2dpDataBlockInfo *data_block = &DBLK(av_inst)[blockId];

        if (data_block)
        {
             unsigned offset = data_block->offset;

             if (offset)
             {
                 DBLK_TYPE *new_data_pool;
                 unsigned padded_block_size = DBLK_PAD(data_block->blockSize);

                 a2dpDataBlockInfo *block = &DBLK(av_inst)[0];
                 /* Reduce offsets of all blocks positioned above the block being removed */
                 do
                 {
                     if (offset < block->offset)
                     {
                         block->offset -= padded_block_size;
                     }
                 }
                 while (++block <= &DBLK(av_inst)[DATA_BLOCK_MAX-1]);

                 /* Zero info parameters of block being removed */
                 memset(data_block, 0, sizeof(a2dpDataBlockInfo));

                 /* Reduce overall size of all blocks */
                 DBLK_DATA_SIZE(av_inst) -= (uint16)padded_block_size;

                 /* Shift blocks above removed block down by the appropriate amount.
                  * For debug purposes, fill the now unused area at top of memory area
                  */
                 memmove((uint8*)DBLK_BASE(av_inst)+offset, (uint8*)DBLK_BASE(av_inst)+offset+padded_block_size, DBLK_DATA_SIZE(av_inst)+DBLK_HDR_SIZE-offset);
                 memset((uint8*)DBLK_BASE(av_inst)+DBLK_DATA_SIZE(av_inst)+DBLK_HDR_SIZE, 0xFF, padded_block_size);

                 if ((new_data_pool = (DBLK_TYPE *)realloc(DBLK_BASE(av_inst), DBLK_HDR_SIZE+DBLK_DATA_SIZE(av_inst))) != NULL)
                 {
                     DBLK_BASE(av_inst) = new_data_pool;
                 }
                 /* No need to worry about a failed realloc, old one will still exist and be valid */
             }
        }
        else
        {
            DEBUG_LOG("appA2dpBlockRemove: Not Found data_block");
        }
    }
}

uint8 *appA2dpBlockGetBase(const avInstanceTaskData* av_inst, a2dpDataBlockId blockId)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        unsigned offset = DBLK(av_inst)[blockId].offset;

        if (offset)
        {
            return ((uint8*)DBLK_BASE(av_inst))+offset;
        }
    }

    return NULL;
}

uint8 *appA2dpBlockGetIndexed(const avInstanceTaskData* av_inst, a2dpDataBlockId blockId, uint8 element)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        a2dpDataBlockInfo *data_block = &DBLK(av_inst)[blockId];
        unsigned offset = data_block->elementSize * element;

        offset += data_block->offset;

        if (offset)
        {
            return ((uint8*)DBLK_BASE(av_inst))+offset;
        }
    }

    return NULL;
}

uint8 *appA2dpBlockGetCurrent(const avInstanceTaskData* av_inst, a2dpDataBlockId blockId)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        a2dpDataBlockInfo *data_block = &DBLK(av_inst)[blockId];
        unsigned offset = data_block->elementSize * data_block->currentElement;

        offset += data_block->offset;

        if (offset)
        {
            return ((uint8*)DBLK_BASE(av_inst))+offset;
        }
    }

    return NULL;
}

uint8 *appA2dpBlockSetCurrent(const avInstanceTaskData* av_inst, a2dpDataBlockId blockId, uint8 element)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        a2dpDataBlockInfo *data_block = &DBLK(av_inst)[blockId];

        if (element == DATA_BLOCK_INDEX_NEXT)
        {
            element = (uint8)(data_block->currentElement + 1);
        }
        else if (element == DATA_BLOCK_INDEX_PREVIOUS)
        {
            element = (uint8)(data_block->currentElement - 1);
        }

        if ((data_block->elementSize * element) < data_block->blockSize)
        {
            data_block->currentElement = element;

            return appA2dpBlockGetCurrent(av_inst, blockId);
        }
    }

    return NULL;
}

uint16 appA2dpBlockGetSize(const avInstanceTaskData* av_inst, a2dpDataBlockId blockId)
{
    if (blockId < DATA_BLOCK_MAX)
    {
        return (uint16)DBLK(av_inst)[blockId].blockSize;
    }

    return 0;
}

extern uint32 marshalA2dpBlockHdr_cb(const void *parent,
                                     const marshal_member_descriptor_t *member_descriptor,
                                     uint32 array_element)
{
    a2dpDataBlockHeader *blockHeader = (a2dpDataBlockHeader *)parent;

    UNUSED(member_descriptor);
    UNUSED(array_element);

    return (blockHeader->blockSizePadded + (sizeof(marshalA2dpBlockInf) * A2DP_PROFILE_NUM_DATA_BLOCK));
}

#endif
