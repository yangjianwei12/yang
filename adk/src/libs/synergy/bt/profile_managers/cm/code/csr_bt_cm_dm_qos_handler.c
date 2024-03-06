/******************************************************************************
 Copyright (c) 2013-2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP

#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_l2cap.h"

#include "csr_log_text_2.h"

#define CSR_BT_CM_DM_SEARCH_TYPE_DEVICE_ROLE   0x00
#define CSR_BT_CM_DM_SEARCH_TYPE_QOS_LATENCY   0x01

static CsrBool csrBtCmDmFindL2caPriorityDataChannelFromACLConnections(
                                                        cmInstanceData_t *cmData,
                                                        aclTable **aclConnectionElem,
                                                        void* pContext,
                                                        CsrUint8 searchType)
{
    /* Find acl with high priority l2cap channel and matches search type*/
    CsrUintFast8 index = 0;
    CsrBool result = FALSE;
    aclTable *aclConnectionElement = NULL;

    for ( ; index < NUM_OF_ACL_CONNECTION; index++)
    {
        /* iterate through the entire acl table for a valid search type */
        CsrBtDeviceAddr deviceAddr;

        aclConnectionElement = &(cmData->roleVar.aclVar[index]);
        deviceAddr = aclConnectionElement->deviceAddr;
        if (!CsrBtBdAddrEqZero(&deviceAddr))
        {
            cmL2caConnElement *theElement = 
                    CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caPriorityDataChannel, 
                                         &(deviceAddr));
            if (theElement)
            {
                if (searchType == CSR_BT_CM_DM_SEARCH_TYPE_DEVICE_ROLE)
                {
                    CsrUint8 role = *(CsrUint8*)pContext;
                    if (role == aclConnectionElement->role)
                    {
                        result = TRUE;
                        break;
                    }
                }
                else if (searchType == CSR_BT_CM_DM_SEARCH_TYPE_QOS_LATENCY)
                {
                    CsrUint32 serviceLatency = *(CsrUint32*)pContext;
                    if (serviceLatency == aclConnectionElement->latency)
                    {
                        result = TRUE;
                        break;
                    }
                }
            }
        }
    }

    *aclConnectionElem = aclConnectionElement;

    return result;
}

static CsrBool csrBtCmDmSetHciQosSettings(aclTable *aclConnectionElement,
                                                   CsrBool useDefaultQos)
{
    CsrBool result = FALSE;

    if (useDefaultQos)
    {
        if (aclConnectionElement->latency != CSR_BT_CM_DEFAULT_QOS_LATENCY)
        {
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "hci qos setup req - service latency = 25ms"));

            result = TRUE;

            dm_hci_qos_setup_req(&(aclConnectionElement->deviceAddr),
                                 0,
                                 HCI_QOS_BEST_EFFORT,
                                 CSR_BT_CM_DEFAULT_QOS_TOKEN_RATE,
                                 CSR_BT_CM_DEFAULT_QOS_PEAK_BANDWIDTH,
                                 CSR_BT_CM_DEFAULT_QOS_LATENCY,
                                 CSR_BT_CM_DEFAULT_QOS_DELAY_VARIATION,
                                 NULL);
        }
    }
    else
    {
        if (aclConnectionElement->latency != CSR_BT_AV_QOS_LATENCY)
        {
            /* high priority data channel service latency */
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "hci qos setup req - service latency = 11.25ms"));

            result = TRUE;

            dm_hci_qos_setup_req(&(aclConnectionElement->deviceAddr),
                                 0,
                                 CSR_BT_AV_QOS_SERVICE_TYPE,
                                 CSR_BT_AV_QOS_TOKEN_RATE,
                                 CSR_BT_AV_QOS_PEAK_BANDWIDTH,
                                 CSR_BT_AV_QOS_LATENCY,
                                 CSR_BT_AV_QOS_DELAY_VARIATION,
                                 NULL);
        }
    }

    return result;
}

CsrBool CsrBtCmDmHciQosSetupDirect(cmInstanceData_t *cmData,
                                             CsrSchedQid appHandle,
                                             CsrBtDeviceAddr* addr,
                                             CsrBool setDefaultQos)
{
    CsrUintFast8 numACLConnections = returnNumOfAclConnection(cmData);
    CsrBool result = FALSE;

    if (numACLConnections)
    {
        aclTable *aclConnectionElement = NULL;

        /* Following scenarios covered:
              * Master of an A2DP link and RNR (paging involved) OR SLC that requires 
              * paging.
              * We as a master of high priority l2cap data and then doing RNR/paging
              * should use HCI_QOS_SETUP to set the link with AV latency to 12ms 
              * that would cause the Tpoll to be 12ms (11.25msec/18 slots to be accurate) 
              * in an attempt to recover some of the bandwidth on the A2DP link when you 
              * are master, and set AV QOS latency back to to 24ms on page complete. */

        result = TRUE;

        /* This could be the address of the remote device to which a page might be 
              * initiated or page is completed, if valid or could be no address at all, say if
              * hci qos setup is initiated from the application as a part of RNR based inquiry */
        if (!CsrBtBdAddrEqZero(addr))
        {
            returnAclConnectionElement(cmData, *addr, &aclConnectionElement);
            if ((1 == numACLConnections) && (NULL != aclConnectionElement))
            {
                /* this is the only connection, so do not have to set qos */
                result = FALSE;
            }
        }

        if (result)
        {
            if (TRUE == setDefaultQos)
            {
                CsrUint32 latency = CSR_BT_AV_QOS_LATENCY;
                /* set to default QOS if an ACL link with qos set to 12ms*/
                result = 
                csrBtCmDmFindL2caPriorityDataChannelFromACLConnections(cmData,
                                     &aclConnectionElement, (void*)&latency,
                                     CSR_BT_CM_DM_SEARCH_TYPE_QOS_LATENCY);
            }
            else
            {
                CsrUint8 role = CSR_BT_MASTER_ROLE;
                /* set to non-default QOS if we are the master of a high priority channel */
                result = 
                csrBtCmDmFindL2caPriorityDataChannelFromACLConnections(cmData, 
                                        &aclConnectionElement, (void*)&role, 
                                        CSR_BT_CM_DM_SEARCH_TYPE_DEVICE_ROLE);
            }

            if (result)
            {
                /* change the qos settings for the acl connection */
                result = 
                csrBtCmDmSetHciQosSettings(aclConnectionElement, setDefaultQos);
                if (result && (CSR_SCHED_QID_INVALID != appHandle))
                {
                    /* qos set up initiated for a high priority acl */
                    aclConnectionElement->aclQosSetupAppHandle = appHandle;
                }
            }
        }
    }

    return result;
}

void CsrBtCmDmHciQosSetupReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmDmHciQosSetupReq* cmPrim = (CsrBtCmDmHciQosSetupReq *)cmData->recvMsgP;

    CsrBool result =
        CsrBtCmDmHciQosSetupDirect(cmData,
                        cmPrim->appHandle,
                        &(cmPrim->deviceAddr),
                        cmPrim->useDefaultQos);

    if (!result)
    {
        /* hci qos setup not done, start house cleaning */
        CsrBtCmDmLocalQueueHandler();
    }
}

void CsrBtCmDmHciQosSetupCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_QOS_SETUP_CFM_T *dmPrim = (DM_HCI_QOS_SETUP_CFM_T *) cmData->recvMsgP;
    aclTable *aclConnectionElement = NULL;

    returnAclConnectionElement(cmData, dmPrim->bd_addr, &aclConnectionElement);
    if (NULL != aclConnectionElement)
    {
        /* update the acl table with the current qos settings */
        aclConnectionElement->serviceType = dmPrim->service_type;
        aclConnectionElement->tokenRate = dmPrim->token_rate;
        aclConnectionElement->peakBandwidth = dmPrim->peak_bandwidth;
        aclConnectionElement->latency = dmPrim->latency;
        aclConnectionElement->delayVariation = dmPrim->delay_variation;
 
        if (dmPrim->latency < 8750 &&
            dmPrim->status == HCI_SUCCESS)
        {
            /* Only if the HCI QoS request asks for a latency (Tpoll) of less
                     * than 14 slots (8.75 milliseconds) do we try to adjust the QoS
                     * to 40 slots. This is consistent with B-4441. Also ensure that
                     * we only attempt the QoS-adjustment only once for these
                     * devices */
            if (!aclConnectionElement->unsolicitedQosSetup)
            {
                /* Try once and only once to correct QoS */
                aclConnectionElement->unsolicitedQosSetup = TRUE;
                dm_hci_qos_setup_req(&(dmPrim->bd_addr),
                                     0,
                                     HCI_QOS_BEST_EFFORT,
                                     CSR_BT_CM_DEFAULT_QOS_TOKEN_RATE,
                                     CSR_BT_CM_DEFAULT_QOS_PEAK_BANDWIDTH,
                                     CSR_BT_CM_DEFAULT_QOS_LATENCY,
                                     CSR_BT_CM_DEFAULT_QOS_DELAY_VARIATION,
                                     NULL);
            }
            return;
        }        

        if (aclConnectionElement->aclQosSetupAppHandle != CSR_SCHED_QID_INVALID)
        {
            /* application initiated hci qos set up for acl link */
            aclConnectionElement->aclQosSetupAppHandle = CSR_SCHED_QID_INVALID;
            /* start house cleaning */
            CsrBtCmDmLocalQueueHandler();
        }
    }
}

#endif /* CSR_BT_EXCLUDE_HCI_QOS_SETUP */
