/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include <string.h>
#include <stdio.h>

#include "gatt_telephone_bearer_client_private.h"

/****************************************************************************/
bool GattTelephoneBearerClientDestroy(const ServiceHandle tbsHandle)
{
    bool result = FALSE;
    GTBSC *tbs_client = ServiceHandleGetInstanceData(tbsHandle);

    /* Check parameters */
    if (tbs_client == NULL)
    {
        GATT_TBS_CLIENT_PANIC("GTBSC: Invalid parameters - Destroy\n");
        return FALSE;
    }

/*     Register with the GATT Manager and verify the result
    result = (GattManagerUnregisterClient(&tbs_client->lib_task) == gatt_manager_status_success);
        
     Clear pending messages
    MessageFlushTask(&tbs_client->lib_task);*/

    result = ServiceHandleFreeInstanceData(tbsHandle);
    
    return result;
}
