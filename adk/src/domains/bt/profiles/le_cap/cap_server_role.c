/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_cap
    \brief
*/

#include "cap_server_role_advertising.h"
#include "cap_server_role.h"

void LeCapServer_Init(void)
{
    CapServer_SetupLeAdvertisingData();
}
