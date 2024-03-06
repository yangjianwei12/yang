/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\ingroup    multidevice
\brief      Indicates if this device belongs to a pair of devices.

*/

#include "multidevice.h"

#include <panic.h>

typedef struct
{
    multidevice_type_t type;
    multidevice_side_t side;
} multidevice_ctx_t;

static multidevice_ctx_t ctx;

void Multidevice_SetType(multidevice_type_t type)
{
    ctx.type = type;
}

multidevice_type_t Multidevice_GetType(void)
{
    return ctx.type;
}

void Multidevice_SetSide(multidevice_side_t side)
{
    ctx.side = side;
}

multidevice_side_t Multidevice_GetSide(void)
{
    return ctx.side;
}

multidevice_side_t Multidevice_GetPairSide(void)
{
    multidevice_side_t pair_side = ctx.side;

    if(Multidevice_IsPair())
    {
        if(ctx.side == multidevice_side_left)
        {
            pair_side = multidevice_side_right;
        }
        else if(ctx.side == multidevice_side_right)
        {
            pair_side = multidevice_side_left;
        }
    }

    return pair_side;
}

bool Multidevice_IsPair(void)
{
    if(ctx.type == multidevice_type_pair)
    {
        return TRUE;
    }
    return FALSE;
}

bool Multidevice_IsLeft(void)
{
    if(Multidevice_IsPair())
    {
        if(ctx.side == multidevice_side_left)
        {
            return TRUE;
        }
        else if(ctx.side == multidevice_side_right)
        {
            return FALSE;
        }
    }

    Panic();
    return FALSE;
}

bool Multidevice_IsDeviceStereo(void)
{
    return ctx.side == multidevice_side_both;
}

bool Multidevice_IsSameAsOurSide(multidevice_side_t side)
{
    return side == multidevice_side_both || side == ctx.side;
}

bool Multidevice_IsSameAsPairSide(multidevice_side_t side)
{
    return side == multidevice_side_both || side == Multidevice_GetPairSide();
}

