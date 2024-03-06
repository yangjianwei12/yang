/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #3 $
******************************************************************************/

#include "gatt_gmas_client_discovery.h"
#include "gatt_gmas_client_debug.h"
#include "gatt_gmas_client.h"
#include "gatt_gmas_client_uuid.h"
#include "gatt_gmas_client_init.h"
#include "gatt_gmas_client_common_util.h"


/****************************************************************************/
void gattGmasClientHandleDiscoverAllGmasCharacteristicsResp(GGMASC *gattGmasClient,
                                                            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    bool initFailed = FALSE;
    GATT_GMAS_CLIENT_INFO("GGMASC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
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
                case GATT_CHARACTERISTIC_GMAS_UUID_ROLE:
                {
                    gattGmasClient->handles.roleHandle = cfm->handle;
                    GATT_GMAS_CLIENT_DEBUG("GGMASC: GMAS Role cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_GMAS_UUID_UGG_FEATURES:
                {
                    gattGmasClient->handles.uggFeaturesHandle = cfm->handle;
                    GATT_GMAS_CLIENT_DEBUG("GGMASC: GMAS UGG Features cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_GMAS_UUID_UGT_FEATURES:
                {
                    gattGmasClient->handles.ugtFeaturesHandle = cfm->handle;
                    GATT_GMAS_CLIENT_DEBUG("GGMASC: GMAS UGT Features cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_GMAS_UUID_BGS_FEATURES:
                {
                    gattGmasClient->handles.bgsFeaturesHandle = cfm->handle;
                    GATT_GMAS_CLIENT_DEBUG("GGMASC: GMAS BGS Features cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_GMAS_UUID_BGR_FEATURES:
                {
                    gattGmasClient->handles.bgrFeaturesHandle = cfm->handle;
                    GATT_GMAS_CLIENT_DEBUG("GGMASC: GMAS BGR Features cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                default:
                    break;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gattGmasClient->handles.roleHandle)
            {
                /* Mandatory GMAS characteristic is not found, initialisation incomplete */
                gattGmasClientSendInitCfm(gattGmasClient, GATT_GMAS_CLIENT_STATUS_DISCOVERY_ERR);
                initFailed = TRUE;
            }
            else
            {
                gattGmasClientSendInitCfm(gattGmasClient, GATT_GMAS_CLIENT_STATUS_SUCCESS);
                return;
            }
        }
    }
    else
    {
        gattGmasClientSendInitCfm(gattGmasClient, GATT_GMAS_CLIENT_STATUS_DISCOVERY_ERR);
        initFailed = TRUE;
    }

    if (initFailed == TRUE)
    {
        GattGmasClient *gmasClientMain = gattGmasClientGetMainInstance();
        ServiceHandle clientHndl = gattGmasClient->srvcElem->service_handle;
    
        CsrBtGattUnregisterReqSend(gattGmasClient->srvcElem->gattId);
    
        /* Remove the serevice element from main list */
        if (gmasClientMain)
            GATT_GMAS_CLIENT_REMOVE_SERVICE_HANDLE(gmasClientMain->serviceHandleList, gattGmasClient->srvcElem);
    
        /* Free the service instance memory */
        ServiceHandleFreeInstanceData(clientHndl);
    }
}
