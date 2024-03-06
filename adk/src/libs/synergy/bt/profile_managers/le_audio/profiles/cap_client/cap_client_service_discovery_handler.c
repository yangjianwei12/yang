/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_util.h"
#include "cap_client_init.h"
#include "gatt_service_discovery_lib.h"
#include "cap_client_service_discovery_handler.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_debug.h"
#include "cap_client_micp_operation_req.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientUpdateGroupInstance(CapClientGroupInstance *gInst)
{
    memset(gInst->sirk, 0, CAP_CLIENT_SIRK_SIZE);
    gInst->setSize = 1;
}

void capClientHandleGattSrvcDiscMsg(CAP_INST  *const inst, const Msg msg)
{
    CsipInstElement *csip = NULL;
    CsrBtGattPrim *prim = (CsrBtGattPrim *)msg;
    CapClientGroupInstance *gInst = CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if(!gInst)
    {
        /* TODO: Check if AddnewDevice or init api, Send error
         * corresponding message and return*/
        CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: gInst is NULL\n");
        return;
    }

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
            uint8 i;
            bool serviceFound = FALSE;
#endif
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                         (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(gInst->csipList, cfm->cid);

            csip->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_CSIP);


            if ((cfm->result != GATT_SD_RESULT_SUCCESS) || cfm->srvcInfoCount == 0)
            {
                CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: srvcInfoCount: %d, Result: 0x%x \n", cfm->srvcInfoCount, cfm->result);
                inst->discoveryRequestCount = 0;

                if (inst->addNewDevice)
                {
                    inst->addNewDevice = FALSE;
                    capClientSendAddNewDeviceCfm(
                        gInst->groupId,
                        inst->deviceCount,
                        CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR,
                        gInst->appTask,
                        gInst);
                }
                else
                    capClientSendInitCfm(inst, CAP_CLIENT_RESULT_SUCCESS_DISCOVERY_ERR);
                return;

            }

            inst->discoveryRequestCount--;

            CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: discoveryRequestCount: %d, Result: 0x%x \n", inst->discoveryRequestCount, cfm->result);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
            for (i = 0; i< cfm->srvcInfoCount; i++)
            {
                /* Check the confirmation is for optional services */
                if (cfm->srvcInfo[i].srvcId == GATT_SD_MICS_SRVC)
                {
                    serviceFound = TRUE;
                    break;
                }
            }

            if (serviceFound) /* In future this condition need to be extended for other optional services */
            {
                capClientMicpHandler(inst, cfm, i);
                return;
            }
#endif

            /* CAS instance is found! Now do Secondary Service Discovery for CSIS
             *  on the remote device if csis data is not found for the cid*/

            if(inst->discoveryRequestCount == 0 && cfm->result == GATT_SD_RESULT_SUCCESS)
            {
                inst->discoveryRequestCount++;
                GattServiceDiscoverFindIncludedServiceStart(gInst->libTask,
                                                         csip->cid,
                                                         GATT_SD_CAS_SRVC);

                CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: included Service Disocvery \n");

            }

            if (cfm->srvcInfoCount && cfm->srvcInfo)
                CsrPmemFree(cfm->srvcInfo);
        }
        break;

        case GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM:
        {
            GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM_T *cfm =
                   (GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM_T*)msg;

            if (cfm->result != GATT_SD_RESULT_INPROGRESS)
                inst->discoveryRequestCount--;

            csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(gInst->csipList, cfm->cid);
            csip->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_SRVC_DSC);

            CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: discoveryRequestCount: %d, Result: 0x%x \n", inst->discoveryRequestCount, cfm->result);


            /* If discovery is complete on all the devices i.e discoveryRequestCount hits zero
               Do Secondary Service Discover */
            if(inst->discoveryRequestCount == 0)
            {
                if (!csip->recentStatus)
                {
                    inst->discoveryRequestCount++;

                    GattServiceDiscoverFindIncludedServiceRange(CSR_BT_CAP_CLIENT_IFACEQUEUE,
                        cfm->cid,
                        GATT_SD_CAS_SRVC,
                        GATT_SD_CSIS_SRVC);

                    CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: GattServiceDiscoverFindIncludedServiceRange \n");

                }
            }
        }
        break;

        case GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM_T *cfm =
                   (GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM_T*)msg;

            csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(gInst->csipList, cfm->cid);

            if(csip == NULL)
            {
                /*TODO: Return NULL instance Error*/
                return;
            }

            inst->discoveryRequestCount--;
            /* If no CSIS instance found, Assume that the group has only one device */
            csip->recentStatus = capClientGetCapClientResult(cfm->result, CAP_CLIENT_SRVC_DSC);
            CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg: srvcInfoCount: %d, Result: 0x%x \n", cfm->srvcInfoCount, cfm->result);


            if(cfm->srvcInfoCount == 0 || cfm->result != GATT_SD_RESULT_SUCCESS )
            {
                capClientUpdateGroupInstance(gInst);
                inst->csipRequestCount = 0;
                inst->discoveryRequestCount = 0;
                inst->addNewDevice = FALSE;
                gInst->currentDeviceCount = inst->deviceCount;

                /* This cannot be Triggered by addnewDevice Method */

                capClientSendInitCfm(inst, CAP_CLIENT_RESULT_SUCCESS);

                return;
            }

            if(csip->recentStatus == CAP_CLIENT_RESULT_SUCCESS)
            {
                CsipInitData data;
                CsipHandles *csipData = NULL;

                if (csip->csipData)
                {
                    csipData = (CsipHandles*)CsrPmemZalloc(sizeof(CsipHandles));
                    CsrMemCpy(&csipData->csisHandle, csip->csipData, sizeof(GattCsisClientDeviceData));
                }
                
                inst->csipRequestCount++;
                data.cid = cfm->cid;

                CsipInitReq(gInst->libTask, &data, csipData);

                CsrPmemFree(csipData);
                csipData = NULL;
            }

            if(cfm->srvcInfoCount && cfm->srvcInfo)
                CsrPmemFree(cfm->srvcInfo);
        }
        break;


        default:
        {
            /* Unrecognized GATT  message */

            CAP_CLIENT_INFO("\n (CAP)handleGattSrvcDiscMsg:  Unrecognized Service Discovery Prim\n");

        }
        break;
    }

    /* If no csip or Service Discovery Requests are pending Send away CFM message*/

    gInst->currentDeviceCount = inst->deviceCount;

    if(!inst->discoveryRequestCount && !inst->csipRequestCount)
    {
        if(inst->addNewDevice)
        {
            inst->addNewDevice =  FALSE;
            capClientSendAddNewDeviceCfm(gInst->groupId,
                                  inst->deviceCount,
                                  CAP_CLIENT_RESULT_SUCCESS,
                                  gInst->appTask,
                                  gInst);

        }
        else
        {
            capClientSendInitCfm(inst, CAP_CLIENT_RESULT_SUCCESS);
        }
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
