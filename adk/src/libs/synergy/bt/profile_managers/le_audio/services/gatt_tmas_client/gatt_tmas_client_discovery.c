/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/

#include "gatt_tmas_client_discovery.h"
#include "gatt_tmas_client_debug.h"
#include "gatt_tmas_client.h"
#include "gatt_tmas_client_uuid.h"
#include "gatt_tmas_client_init.h"
#include "gatt_tmas_client_common_util.h"


/****************************************************************************/
void gattTmasClientHandleDiscoverAllTmasCharacteristicsResp(GTMASC *gattTmasClient,
                                                            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    bool initFailed = FALSE;
    GATT_TMAS_CLIENT_INFO("GTMASC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            switch (cfm->uuid[0])
            {
                case GATT_CHARACTERISTIC_UUID_ROLE:
                {
                    gattTmasClient->handles.roleHandle = cfm->handle;
                    GATT_TMAS_CLIENT_DEBUG("GTMASC: TMAS Role cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                default:
                    break;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gattTmasClient->handles.roleHandle)
            {
                /* Mandatory TMAS characteristic is not found, initialisation incomplete */
                gattTmasClientSendInitCfm(gattTmasClient, GATT_TMAS_CLIENT_STATUS_DISCOVERY_ERR);
                initFailed = TRUE;
            }
            else
            {
                gattTmasClientSendInitCfm(gattTmasClient, GATT_TMAS_CLIENT_STATUS_SUCCESS);
                return;
            }
        }
    }
    else
    {
        gattTmasClientSendInitCfm(gattTmasClient, GATT_TMAS_CLIENT_STATUS_DISCOVERY_ERR);
        initFailed = TRUE;
    }

    if (initFailed == TRUE)
    {
        GattTmasClient *tmasClientMain = gattTmasClientGetMainInstance();
        ServiceHandle clientHndl = gattTmasClient->srvcElem->service_handle;
    
        CsrBtGattUnregisterReqSend(gattTmasClient->srvcElem->gattId);
    
        /* Remove the serevice element from main list */
        if (tmasClientMain)
            GATT_TMAS_CLIENT_REMOVE_SERVICE_HANDLE(tmasClientMain->serviceHandleList, gattTmasClient->srvcElem);
    
        /* Free the service instance memory */
        ServiceHandleFreeInstanceData(clientHndl);
    }

}
