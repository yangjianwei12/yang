/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_debug.h"
#include "ccp_init.h"
#include "ccp_common.h"
#include "ccp_msg_handler.h"
#include "gatt_service_discovery_lib.h"


ccp_main_inst *ccp_main;

/******************************************************************************/
void ccpSendInitCfm(CCP * ccp_inst, CcpStatus status)
{
    if(ccp_inst)
    {
        MAKE_CCP_MESSAGE(CcpInitCfm);
        message->id = CCP_INIT_CFM;
        message->status = status;
        message->prflHndl = ccp_inst->ccp_srvc_hdl;
        CcpMessageSend(ccp_inst->appTask, message);
    }
    else
    {
        CCP_DEBUG("Invalid profile handle\n");
    }
}

/****************************************************************************/
static void ccpTbsInitReq(CCP *ccp_inst,
                          CcpInitData *clientInitParams,
                          CcpHandles *deviceData)
{

    GattTelephoneBearerClientDeviceData tbsDeviceData;
    GattTelephoneBearerClientInitData tbsInitData;

    tbsInitData.cid = clientInitParams->cid;
    tbsInitData.startHandle = deviceData->tbsHandle.startHandle;
    tbsInitData.endHandle = deviceData->tbsHandle.endHandle;

    tbsDeviceData.bearerNameHandle = deviceData->tbsHandle.bearerNameCccHandle;
    tbsDeviceData.bearerNameCccHandle = deviceData->tbsHandle.bearerNameCccHandle;
    tbsDeviceData.bearerUciHandle = deviceData->tbsHandle.bearerUciHandle;
    tbsDeviceData.bearerTechHandle = deviceData->tbsHandle.bearerTechHandle;
    tbsDeviceData.bearerTechCccHandle = deviceData->tbsHandle.bearerTechCccHandle;
    tbsDeviceData.bearerUriPrefixListHandle = deviceData->tbsHandle.bearerUriPrefixListHandle;
    tbsDeviceData.signalStrengthHandle = deviceData->tbsHandle.signalStrengthHandle;
    tbsDeviceData.signalStrengthCccHandle = deviceData->tbsHandle.signalStrengthCccHandle;
    tbsDeviceData.signalStrengthIntervalHandle = deviceData->tbsHandle.signalStrengthIntervalHandle;
    tbsDeviceData.listCurrentCallsHandle = deviceData->tbsHandle.listCurrentCallsHandle;
    tbsDeviceData.listCurrentCallsCccHandle = deviceData->tbsHandle.listCurrentCallsCccHandle;
    tbsDeviceData.contentControlIdHandle = deviceData->tbsHandle.contentControlIdHandle;
    tbsDeviceData.statusFlagsHandle = deviceData->tbsHandle.statusFlagsHandle;
    tbsDeviceData.statusFlagsCccHandle = deviceData->tbsHandle.statusFlagsCccHandle;
    tbsDeviceData.incomingTargetBearerUriHandle = deviceData->tbsHandle.incomingTargetBearerUriHandle;
    tbsDeviceData.incomingTargetBearerUriCccHandle = deviceData->tbsHandle.incomingTargetBearerUriCccHandle;
    tbsDeviceData.callStateHandle = deviceData->tbsHandle.callStateHandle;
    tbsDeviceData.callStateCccHandle = deviceData->tbsHandle.callStateCccHandle;
    tbsDeviceData.callControlPointHandle = deviceData->tbsHandle.callControlPointHandle;
    tbsDeviceData.callControlPointCccHandle = deviceData->tbsHandle.callControlPointCccHandle;
    tbsDeviceData.callControlPointOptionalOpcodesHandle = deviceData->tbsHandle.callControlPointOptionalOpcodesHandle;
    tbsDeviceData.terminationReasonHandle = deviceData->tbsHandle.terminationReasonHandle;
    tbsDeviceData.terminationReasonCccHandle = deviceData->tbsHandle.terminationReasonCccHandle;
    tbsDeviceData.incomingCallHandle = deviceData->tbsHandle.incomingCallHandle;
    tbsDeviceData.incomingCallCccHandle = deviceData->tbsHandle.incomingCallCccHandle;
    tbsDeviceData.remoteFriendlyNameHandle = deviceData->tbsHandle.remoteFriendlyNameHandle;
    tbsDeviceData.remoteFriendlyNameCccHandle = deviceData->tbsHandle.remoteFriendlyNameCccHandle;

    GattTelephoneBearerClientInit(ccp_inst->lib_task,
                                  &tbsInitData,
                                  &tbsDeviceData);
}

/***************************************************************************/
void CcpInitReq(AppTask appTask,
                 CcpInitData *clientInitParams,
                 CcpHandles *deviceData,
                 bool tbsRequired)
{
    CCP *ccp_inst = NULL;
    ProfileHandleListElm_t *elem = NULL;
    CcpProfileHandle profile_hndl = 0;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        CCP_PANIC("Application Task NULL\n");
    }

    elem = CCP_ADD_SERVICE_HANDLE(ccp_main->profile_handle_list);
    profile_hndl = (CcpProfileHandle) ServiceHandleNewInstance((void **) &ccp_inst, sizeof(CCP));
    elem->profile_handle = profile_hndl;

    if (profile_hndl)
    {
        /* Reset all the service library memory */
        memset(ccp_inst, 0, sizeof(CCP));

        /* Set up library handler for external messages */
        ccp_inst->lib_task = CSR_BT_CCP_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        ccp_inst->appTask = appTask;

        ccp_inst->cid = clientInitParams->cid;

        ccp_inst->ccp_srvc_hdl = profile_hndl;

        ccp_inst->tbsRequired = tbsRequired;
        if(tbsRequired)
        {
            CCP_PANIC("CcpInitReq -  TBS requested but not currently supported\n");
        }
        ccp_inst->first_tbs_inst = NULL;
        ccp_inst->last_tbs_inst = NULL;
        ccp_inst->first_tbs_srvc_hndl = NULL;
        ccp_inst->last_tbs_srvc_hndl = NULL;

        ccpSendInitCfm(ccp_inst, CCP_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            /* It's a peer device: we already know handles, no need to do discovery */
            ccp_inst->is_peer_device = TRUE;

            ccp_inst->gtbs_start_handle = deviceData->tbsHandle.startHandle;
            ccp_inst->gtbs_end_handle = deviceData->tbsHandle.endHandle;

            /* We can start now the initialisation of all the necessary client:
             * we start with the GTBS Client.
             */
            ccpTbsInitReq(ccp_inst, clientInitParams, deviceData);
        }
        else
        {
            /* Discover GTBS first */

            GattSdSrvcId srvcIds = GATT_SD_TBS_SRVC;
            /* Find handle value range for the TBS from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_CCP_IFACEQUEUE, ccp_inst->cid, srvcIds);
        }
    }
    else
    {
        CCP_PANIC("Memory allocation of CCP Profile instance failed!\n");
        /* Cannot call ccpSendInitCfm as we have no ccp_inst so send message directly */
        MAKE_CCP_MESSAGE(CcpInitCfm);
        message->id = CCP_INIT_CFM;
        message->status = CCP_STATUS_FAILED;
        message->prflHndl = 0;
        CcpMessageSend(appTask, message);
    }
}

/***************************************************************************/
void ccpHandleTbsClientInitResp(CCP *ccp_inst,
                                   const GattTelephoneBearerClientInitCfm * message)
{
    if(message->status == GATT_TELEPHONE_BEARER_CLIENT_STATUS_SUCCESS)
    {
         ccp_inst->tbs_srvc_hdl = message->tbsHandle;

         /*TODO ccp_inst->is_peer_device
            ccp_inst->secondary_service_req

            Discover TBS here if required             
            if (ccp_inst->tbsRequired)  ... */

         /* Send the confirmation of the initialisation  */
         ccpSendInitCfm(ccp_inst, CCP_STATUS_SUCCESS);
     }
     else
     {
         /* The initialisation of TBS Client failed:
          * we need to destroy all the existed instance lists.*/
         ccpDestroyReqAllInstList(ccp_inst);

         ccpSendInitCfm(ccp_inst, CCP_STATUS_FAILED);

         if(!ServiceHandleFreeInstanceData(ccp_inst->ccp_srvc_hdl))
         {
             CCP_PANIC("Freeing Service Handle failed\n");
         }
     }


}

static void InitProfileHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ProfileHandleListElm_t *cElem = (ProfileHandleListElm_t *) elem;

    cElem->profile_handle = 0;
}

void ccp_init(void **gash)
{
    ccp_main = CsrPmemZalloc(sizeof(*ccp_main));
    *gash = ccp_main;

    CsrCmnListInit(&ccp_main->profile_handle_list, 0, InitProfileHandleList, NULL);
    CsrBtGattRegisterReqSend(CSR_BT_CCP_IFACEQUEUE,
                             0);
}

ccp_main_inst *ccpGetMainInstance(void)
{
    return ccp_main;
}



