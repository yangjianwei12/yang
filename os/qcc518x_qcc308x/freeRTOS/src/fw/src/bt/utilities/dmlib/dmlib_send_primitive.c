/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_send_primitive
 *
 *  DESCRIPTION
 *      Send DM primitive into the stack. This is for use when sending
 *      primitives that were built but not sent by an earlier call to one of
 *      the DM Library functions, or when sending primitives that have been
 *      constructed by the application.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_send_primitive(
    DM_UPRIM_T *p_prim
    )
{
    if (p_prim)
    {
        DM_PutMsg(p_prim);
    }
}

