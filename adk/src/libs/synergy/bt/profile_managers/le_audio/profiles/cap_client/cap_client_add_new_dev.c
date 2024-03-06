/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_service_discovery_lib.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_common.h"
#include "cap_client_util.h"
#include "cap_client_new_device_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientSendAddNewDeviceCfm( ServiceHandle groupId,
                             uint8 deviceCount,
                             CapClientResult result,
                             AppTask appTask,
                             CapClientGroupInstance *elem)
{
    CSR_UNUSED(elem);
    MAKE_CAP_CLIENT_MESSAGE(CapClientAddNewDevCfm);
    message->groupId = groupId;
    message->deviceCount = deviceCount;
    message->result = result;

    CapClientMessageSend(appTask, CAP_CLIENT_ADD_NEW_DEV_CFM ,message);
}

void handleCapClientAddnewDevReq(CAP_INST  *const inst,const Msg msg)
{
    CapClientGroupInstance *capClient;
    CapClientInternalAddNewDevReq *req = NULL;

    if(msg == NULL)
    {
        /* Send CAP_RESULT_FAILURE */
        capClientSendAddNewDeviceCfm(inst->activeGroupId,
                              0,
                              CAP_CLIENT_RESULT_INVALID_PARAMETER,
                              inst->appTask,
                              NULL);
        return;
    }

    req = (CapClientInternalAddNewDevReq*)msg;

    /* first verify if the GroupId belongs is same as current Active Group */
    inst->addNewDevice = TRUE;
    inst->deviceCount++;

    /* if groupId is not same Switch the group
     * Note: capClientSetNewActiveGroup Internally sends CapActiveGroupChangeInd to the
     * application internally */

    capClient = capClientSetNewActiveGroup(inst, req->groupId, req->discoveryComplete);

    if(capClient == NULL)
    {
         /* Device in the list but instance unavailable
          * CAP instance can never be NULL
          * Send CAP_RESULT_FAILURE */
        capClientSendAddNewDeviceCfm(inst->activeGroupId,
                              0,
                              CAP_CLIENT_RESULT_INVALID_GROUPID,
                              inst->appTask,
                              NULL);
        CAP_CLIENT_ERROR("\n handleCapClientAddnewDevReq: Encountered Null CAP instance \n");
        return;
    }

    /* if discoveryComplete Send Add Device Cfm */

    if(req->discoveryComplete)
    {
        /* update Set Size with available device count??, Indicating end of
         * CSIP discovery */
        capClient->currentDeviceCount = inst->deviceCount;
        inst->addNewDevice = FALSE;
        capClientSendAddNewDeviceCfm(inst->activeGroupId,
                             0,
                             CAP_CLIENT_RESULT_SUCCESS,
                             capClient->appTask,
                             (CapClientGroupInstance*)capClient);
        return;
    }

    if(inst->deviceCount > capClient->setSize)
    {
        /* Send INVALID operation Error message and
         * reject the connection */
        inst->addNewDevice = FALSE;
        capClient->currentDeviceCount = capClient->setSize;
        inst->deviceCount = capClient->setSize;
        capClientSendAddNewDeviceCfm(inst->activeGroupId,
                             0,
                             CAP_CLIENT_RESULT_INVALID_OPERATION,
                             capClient->appTask,
                             NULL);
        return;
    }

    if (req->initData)
    {
        /* add all the devices in the InitData*/
        capClient->requestCid = req->initData->cid;
        capClientInitAddProfileAndServices(capClient, req->initData->cid,
                                    req->initData->handles);

        /* Start CAS service discovery and CSIS Secondary service Discovery */

        inst->discoveryRequestCount++;
        GattServiceDiscoveryFindServiceRange(capClient->libTask,
                                           capClient->requestCid,
                                           GATT_SD_CAS_SRVC);

        CsrPmemFree(req->initData->handles);
        req->initData->handles = NULL;

        CsrPmemFree(req->initData);
        req->initData = NULL;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
