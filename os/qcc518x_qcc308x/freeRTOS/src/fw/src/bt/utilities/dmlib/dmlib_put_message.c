/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

void dm_put_message(void *p_prim, const dm_prim_t type)
{
    *((dm_prim_t*)p_prim) = type;
    DM_PutMsg(p_prim);
}
