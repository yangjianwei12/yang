/*!
\copyright  Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_data_block.h
\brief      A set of functions to manage creation, deletion and indexing of multiple data blocks
            maintained in a single slot.
*/

#include <a2dp_typedef.h>
#include <av_typedef.h>
#include "csrtypes.h"
#include <string.h>
#include <stdlib.h>

#ifndef A2DP_PROFILE_DATA_BLOCK_H_
#define A2DP_PROFILE_DATA_BLOCK_H_

#define DATA_BLOCK_INDEX_NEXT       0xFF
#define DATA_BLOCK_INDEX_PREVIOUS   0xFE

#define A2DP_PROFILE_NUM_DATA_BLOCK 5

/****************************************************************************
NAME
    appA2dpBlockInit

DESCRIPTION
    Initialise the data block manager

    Returns TRUE on successful initialisation.  Otherwise, returns FALSE.
*/
bool appA2dpBlockInit(avInstanceTaskData* av_inst);

/****************************************************************************
NAME
    appA2dpBlockDestroy

DESCRIPTION
    Frees the memory assigned to A2dp data blocks
*/
void appA2dpBlockDestroy(avInstanceTaskData* av_inst);

/****************************************************************************
NAME
    appA2dpBlockAdd

DESCRIPTION
    Add a data block, of a specified size, to the existing pool of data blocks.
    A pointer to the base of the data block is returned on success, otherwise NULL is returned.
    
    Note: The returned pointer will only be guaranteed to remain valid until a subsequent call to
          blocksAdd or blockRemove is made.
*/
uint8 *appA2dpBlockAdd(avInstanceTaskData* av_inst, a2dpDataBlockId id, uint8 elementCount, uint8 elementSize);

/****************************************************************************
NAME
    blockRemove

DESCRIPTION
    Remove an existing data block from the pool.
*/
void appA2dpBlockRemove(avInstanceTaskData* av_inst, a2dpDataBlockId id);

/****************************************************************************
NAME
    blockGetBase

DESCRIPTION
    Return pointer to base of requested data block.
    NULL is retured if the data block does not exist.
    
    Note: The base of a data block is equilalent to element zero.
*/
uint8 *appA2dpBlockGetBase(const avInstanceTaskData* av_inst, a2dpDataBlockId id);

/****************************************************************************
NAME
    blockGetCurrent

DESCRIPTION
    Return a pointer to the current element within the data block.
    NULL is retured if the data block does not exist.
*/
uint8 *appA2dpBlockGetCurrent(const avInstanceTaskData* av_inst, a2dpDataBlockId id);

/****************************************************************************
NAME
    blockSetCurrent

DESCRIPTION
    Set which element is marked as the current element in the sepcified data block.
    Returns TRUE on success or FALSE otherwise.
*/
uint8 *appA2dpBlockSetCurrent(const avInstanceTaskData* av_inst, a2dpDataBlockId id, uint8 element);

/****************************************************************************
NAME
    blockGetIndexed

DESCRIPTION
    Return a pointer to the specified element within a data block.  
    NULL is returned if the data block does not exist or the element index is out of range.
    
    Note:  An element index of zero is equilalent to the base address of the data block.
*/
uint8 *appA2dpBlockGetIndexed(const avInstanceTaskData* av_inst, a2dpDataBlockId id, uint8 element);

/****************************************************************************
NAME
    blockGetSize

DESCRIPTION
    Returns the size, in bytes, of the specified data block.
    A size of zero will be returned for blocks that do not exist.
*/
uint16 appA2dpBlockGetSize(const avInstanceTaskData* av_inst, a2dpDataBlockId id);

#endif /* A2DP_PROFILE_DATA_BLOCK_H_ */
