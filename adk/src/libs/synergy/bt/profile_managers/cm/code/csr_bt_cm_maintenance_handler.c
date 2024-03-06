/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_callback_q.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_prim.h"
#endif

#include "csr_bt_cm_util.h"
#include "csr_bt_cm_common_amp.h"
#include "csr_log_text_2.h"

#ifdef CSR_BT_LE_ENABLE
#include "csr_bt_cm_le.h"
#endif

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
#include "csr_bt_cm_cme.h"
#endif

#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_panic.h"
#endif

#ifdef INSTALL_CONTEXT_TRANSFER
#include "context_common.h"
#endif /* ifdef INSTALL_CONTEXT_TRANSFER */

void CsrBtCmPutMessage(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_CM_PRIM, msg);
}

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static void csrBtCmCbInitRfcConnElement(CsrCmnListElm_t *elem)
{
    cmRfcConnInstType *connInst   = (cmRfcConnInstType *) CsrPmemZalloc(sizeof(cmRfcConnInstType));
    cmRfcConnElement  *theElement = (cmRfcConnElement *) elem;

    connInst->appHandle                       = CSR_SCHED_QID_INVALID;
    connInst->btConnId                        = CSR_BT_CONN_ID_INVALID;
    connInst->remoteServerChan                = CSR_BT_NO_SERVER;
    connInst->serverChannel                   = CSR_BT_NO_SERVER;

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    connInst->ssrSettings.maxRemoteLatency    = CM_SSR_DISABLED;
    connInst->ssrSettings.minLocalTimeout     = CM_SSR_DISABLED;
    connInst->ssrSettings.minRemoteTimeout    = CM_SSR_DISABLED;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

    connInst->state                           = CSR_BT_CM_RFC_STATE_IDLE;
#ifdef CSR_AMP_ENABLE
    connInst->controller                      = CSR_BT_AMP_CONTROLLER_BREDR;
    connInst->ampProcessState                 = CSR_BT_CM_AMP_PROCESS_IDLE;
#endif

#ifndef CSR_STREAMS_ENABLE
    connInst->dataControl.dataResReceived     = TRUE;
#endif

    connInst->pending                         = FALSE;
    connInst->modemStatusState                = CSR_BT_CM_MODEM_STATUS_IDLE;

    connInst->dmSmAccessRetry                 = 0;

    theElement->cmRfcConnInst                 = connInst;
}

static void csrBtCmCbFreeRfcConnElementPointers(CsrCmnListElm_t *elem)
{
    cmRfcConnElement * theElement = (cmRfcConnElement *) elem;
    cleanUpConnectionTable(&(theElement->cmRfcConnInst));
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static void csrBtCmCbInitL2caConnElement(CsrCmnListElm_t *elem)
{
    cmL2caConnInstType *connInst   = (cmL2caConnInstType *) CsrPmemZalloc(sizeof(cmL2caConnInstType));
    cmL2caConnElement  *theElement = (cmL2caConnElement *) elem;

#ifndef CSR_STREAMS_ENABLE
    connInst->rxQueueMax                    = MAX_L2CAP_DATA_QUEUE_LENGTH;;
    connInst->txPendingContext              = 0xFFFF;
#endif
    connInst->authorised                    = FALSE;
    connInst->btConnId                      = BTCONN_ID_EMPTY;
    connInst->outgoingFlush                 = L2CA_FLUSH_TO_DEFAULT;
    connInst->state                         = CSR_BT_CM_L2CAP_STATE_IDLE;
    connInst->psm                           = NO_LOCAL_PSM;
    connInst->remotePsm                     = NO_REMOTE_PSM;
#ifdef CSR_AMP_ENABLE
    connInst->controller                    = CSR_BT_AMP_CONTROLLER_BREDR;
    connInst->ampProcessState               = CSR_BT_CM_AMP_PROCESS_IDLE;
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    connInst->ssrSettings.maxRemoteLatency  = CM_SSR_DISABLED;
    connInst->ssrSettings.minLocalTimeout   = CM_SSR_DISABLED;
    connInst->ssrSettings.minRemoteTimeout  = CM_SSR_DISABLED;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS */

#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
    connInst->secLevel                      = SECL_NONE;
#endif

    connInst->dataPriority                  = CSR_BT_CM_PRIORITY_NORMAL;
    connInst->addressType                   = TBDADDR_PUBLIC;
    connInst->transportType                 = BREDR_ACL;

    theElement->cmL2caConnInst = connInst;
    theElement->useTpPrim = FALSE;
}

static void csrBtCmCbFreeL2caConnElementPointers(CsrCmnListElm_t *elem)
{
    cmL2caConnElement * theElement = (cmL2caConnElement *) elem;
    CsrBtCleanUpCmL2caConnInst(&(theElement->cmL2caConnInst));
}
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

void CsrBtCmInitInstData(cmInstanceData_t *cmData)
{
    /* Initial the CM instance data */
    CsrUintFast16 i;
    cmData->globalState                     = CSR_BT_CM_STATE_NOT_READY;

    /* Special upper-layer handlers */
#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    cmData->sdHandle                        = CSR_SCHED_QID_INVALID;
    cmData->scHandle                        = CSR_SCHED_QID_INVALID;
#endif

#ifdef CSR_AMP_ENABLE
    cmData->ampmHandle                      = CSR_SCHED_QID_INVALID;
#endif

#ifdef CSR_BT_LE_ENABLE
#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
    cmData->leHandle                        = CSR_SCHED_QID_INVALID;
#endif

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    cmData->leVar.ownAddressType            = CSR_BT_ADDR_TYPE_RANDOM;
    /* set default static address */
    cmData->leVar.localStaticAddr           = CSR_BT_LE_DEFAULT_STATIC_ADDRESS;
    cmData->leVar.staticAddrSet             = FALSE;
#else /* CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC */
    cmData->leVar.ownAddressType            = CSR_BT_ADDR_TYPE_PUBLIC;
    /* set default RPA timeout */
    cmData->leVar.pvtAddrTimeout            = CSR_BT_LE_DEFAULT_PVT_ADDR_TIMEOUT;
#endif /* !CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC */
#endif /* CSR_BT_LE_ENABLE */

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    cmData->roleVar.doMssBeforeScoSetup     = TRUE;
#endif

    for ( i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
        /* Default is no QHS, this will be set by application. */
        cmData->roleVar.aclVar[i].qhsPhyConnected = FALSE;
#endif
#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
        cmData->roleVar.aclVar[i].swbDisabled = FALSE;
#endif
        cmData->roleVar.aclVar[i].l2capExtendedFeatures = CM_INVALID_L2CAP_EXT_FEAT;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
        cmData->roleVar.aclVar[i].logicalChannelTypeMask = CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL;
#endif
    }

    cmData->dmVar.state                                 = CSR_BT_CM_DM_STATE_NULL;
    cmData->dmVar.lockMsg                               = CSR_BT_CM_DM_QUEUE_UNLOCK - 1; /* Locked the queue for CM initialization. */
    cmData->dmVar.rfcConnIndex                          = CM_ERROR;
    cmData->dmVar.majorCod                              = CSR_BT_MAJOR_MINOR_DEVICE_CLASS & CSR_BT_CM_MAJOR_DEVICE_CLASS_FILTER;
    cmData->dmVar.minorCod                              = CSR_BT_MAJOR_MINOR_DEVICE_CLASS & CSR_BT_CM_MINOR_DEVICE_CLASS_FILTER;
    cmData->dmVar.codWrittenToChip                      = 0xffffff;
    cmData->dmVar.pendingCod                            = 0xffffff;

    /* Link policy */
    cmData->dmVar.defaultLinkPolicySettings          = (link_policy_settings_t) CSR_BT_DEFAULT_LOW_POWER_MODES;

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(INSTALL_CM_INTERNAL_LPM)
    cmData->dmVar.defaultSniffSettings.max_interval  = CSR_BT_SNIFF_MAX_TIME_INTERVAL;
    cmData->dmVar.defaultSniffSettings.min_interval  = CSR_BT_SNIFF_MIN_TIME_INTERVAL;
    cmData->dmVar.defaultSniffSettings.attempt       = CSR_BT_SNIFF_ATTEMPT;
    cmData->dmVar.defaultSniffSettings.timeout       = CSR_BT_SNIFF_TIMEOUT;
#endif /* INSTALL_CM_DEVICE_UTILITY && INSTALL_CM_INTERNAL_LPM */

    cmData->dmVar.lowInquiryPriorityLevel            = CSR_BT_CM_DEFAULT_INQUIRY_LEVEL;
    CsrBtCmDmSmClearRebondData(cmData);

    for (i = 0; i < MAX_PCM_SLOTS; i++)
    {
        cmData->dmVar.pcmAllocationTable[i] = NO_SCO;
    }

    /* Inquiry */
    cmData->dmVar.inquiryTxPowerLevel       = CSR_BT_CM_INQUIRY_TX_POWER_LEVEL_DEFAULT;
    cmData->dmVar.inquiryAccessCode         = CSR_BT_CM_ACCESS_CODE_GIAC;
    cmData->dmVar.maxResponses              = CSR_BT_UNLIMITED;
    cmData->dmVar.inquiryTimeout            = HCI_INQUIRY_LENGTH_MAX;

    /* Set default page and inquiry scan settings and type */
    cmData->dmVar.pagescanInterval          = CSR_BT_PAGE_SCAN_INTERVAL_DEFAULT;   /* Should be 0x0800 according to spec 1.2 */
    cmData->dmVar.pagescanWindow            = CSR_BT_PAGE_SCAN_WINDOW_DEFAULT;     /* Should be 0x0012 according to spec 1.2 */
    cmData->dmVar.pagescanType              = CSR_BT_PAGE_SCAN_TYPE_DEFAULT;
    cmData->dmVar.inquiryscanInterval       = HCI_INQUIRYSCAN_INTERVAL_DEFAULT;   /* Should be 0x1000 according to spec 1.2 */
    cmData->dmVar.inquiryscanWindow         = HCI_INQUIRYSCAN_WINDOW_DEFAULT;     /* Should be 0x0012 according to spec 1.2 */
    cmData->dmVar.inquiryscanType           = CSR_BT_INQUIRY_SCAN_TYPE_DEFAULT;

    /* Assume that all features are supported, but set version
     * conservatively */
    CsrMemSet(cmData->dmVar.lmpSuppFeatures, 0xFF, sizeof(cmData->dmVar.lmpSuppFeatures));

    cmData->dmVar.scanTime                  = SCAN_TIMER_DEFAULT;

#ifdef CSR_BT_LE_ENABLE
    cmData->dmVar.connParams.scanInterval          = CSR_BT_LE_DEFAULT_SCAN_INTERVAL;
    cmData->dmVar.connParams.scanWindow            = CSR_BT_LE_DEFAULT_SCAN_WINDOW;
    cmData->dmVar.connParams.connIntervalMin       = CSR_BT_LE_DEFAULT_CONN_INTERVAL_MIN;
    cmData->dmVar.connParams.connIntervalMax       = CSR_BT_LE_DEFAULT_CONN_INTERVAL_MAX;
    cmData->dmVar.connParams.connLatency           = CSR_BT_LE_DEFAULT_CONN_LATENCY;
    cmData->dmVar.connParams.supervisionTimeout    = CSR_BT_LE_DEFAULT_CONN_SUPERVISION_TIMEOUT;
    cmData->dmVar.connParams.connLatencyMax        = CSR_BT_LE_DEFAULT_CONN_LATENCY_MAX;
    cmData->dmVar.connParams.supervisionTimeoutMin = CSR_BT_LE_DEFAULT_CONN_SUPERVISION_TIMEOUT_MIN;
    cmData->dmVar.connParams.supervisionTimeoutMax = CSR_BT_LE_DEFAULT_CONN_SUPERVISION_TIMEOUT_MAX;
#endif

    cmData->smVar.smInProgress              = TRUE;
    cmData->smVar.arg.reg.registeringSrvChannel     = CSR_BT_CM_SERVER_CHANNEL_DONT_CARE;
    cmData->sdcVar.lockMsg = CM_SDC_QUEUE_UNLOCK - 1; /* Locked the queue for CM initialization. */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmData->rfcVar.activeElemId = CM_ERROR;
    CsrCmnListInit(&(cmData->rfcVar.connList), 0, csrBtCmCbInitRfcConnElement, csrBtCmCbFreeRfcConnElementPointers);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    cmData->l2caVar.activeElemId = CM_ERROR;
    CsrCmnListInit(&(cmData->l2caVar.connList), 0, csrBtCmCbInitL2caConnElement, csrBtCmCbFreeL2caConnElementPointers);
#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
    CsrBtCmL2caConnlessInit(cmData);
#endif
    CsrBtCmScCleanupVar(cmData);
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    cmData->bnepVar.roleSwitchReqIndex          = CM_ERROR;

    for ( i = 0; i < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS; i++)
    {
        cmData->bnepVar.connectVar[i].id                = ID_EMPTY;
        cmData->bnepVar.connectVar[i].logicalChannelTypeMask  = CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL;
    }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
    cmData->cmeServiceRunning = FALSE;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
#ifdef CSR_STREAMS_ENABLE
    cmData->extScanSource = 0;
    cmData->extScanState = CSR_BT_EA_PA_NONE;
#endif /* CSR_STREAMS_ENABLE */

    for ( i = 0; i < MAX_EXT_SCAN_APP; i++)
    {
        cmData->extScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
        cmData->extScanHandles[i].scanHandle = CSR_BT_EXT_SCAN_HANDLE_INVALID;
        cmData->extScanHandles[i].pending = FALSE;
    }
#endif

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
    cmData->pastAppHandle = CSR_SCHED_QID_INVALID;

    for ( i = 0; i < MAX_PERIODIC_SCAN_APP; i++)
    {
        cmData->periodicScanHandles[i].pHandle = CSR_SCHED_QID_INVALID;
        cmData->periodicScanHandles[i].syncHandle = CSR_BT_PERIODIC_SCAN_HANDLE_INVALID;
#ifdef CSR_STREAMS_ENABLE
        cmData->periodicScanHandles[i].source = 0;
        cmData->periodicScanHandles[i].paSyncState = CSR_BT_EA_PA_NONE;
#endif /* CSR_STREAMS_ENABLE */
        cmData->periodicScanHandles[i].pending = FALSE;
    }
#endif

#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
    for ( i = 0; i < MAX_EXT_ADV_APP; i++)
    {
        cmData->extAdvAppHandle[i] = CSR_SCHED_QID_INVALID;
    }
#endif

#ifdef CSR_BT_SC_ONLY_MODE_ENABLE
    /* SC only flag is enabled by default. */
    cmData->options |= CM_SECURITY_CONFIG_OPTION_SC_ONLY;
#endif
}

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
static CsrUint8 numOfConnectionToPeerDeviceRfc(cmInstanceData_t *cmData, CsrBtDeviceAddr *theAddr)
{
    cmRfcConnElement *currentElem;
    CsrUint8 count   = 0;

    for (currentElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the RFC connection table */
        if (currentElem->cmRfcConnInst)
        {
            if (currentElem->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED)
            {
                if (CsrBtBdAddrEq(theAddr, &(currentElem->cmRfcConnInst->deviceAddr)))
                {
                    count++;
                }
            }
        }
    }
    return count;
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
static CsrUint8 numOfConnectionToPeerDeviceL2cap(cmInstanceData_t *cmData, CsrBtDeviceAddr *theAddr)
{
    cmL2caConnElement *currentElem;
    CsrUint8           count = 0;

    for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList); currentElem; currentElem = currentElem->next)
    { /* Search through the L2CAP connection list */
        if (currentElem->cmL2caConnInst)
        {
            if (currentElem->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
            {
                if (CsrBtBdAddrEq(theAddr, &(currentElem->cmL2caConnInst->deviceAddr)))
                {
                    count++;
                }
            }
        }
    }
    return count;
}
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
static CsrUint8 numOfConnectionToPeerDeviceBnep(cmInstanceData_t *cmData, CsrBtDeviceAddr *theAddr)
{
    CsrUintFast8 i     = 0;
    CsrUint8 count = 0;

    for (i = 0; i < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS; i++)
    {
        if (cmData->bnepVar.connectVar[i].state == CSR_BT_CM_BNEP_STATE_CONNECTED)
        {
            if (CsrBtBdAddrEq(theAddr, &(cmData->bnepVar.connectVar[i].deviceAddr)))
            {
                count++;
            }
        }
    }
    return count;
}
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */


CsrUint8 CsrBtCmReturnNumOfConnectionsToPeerDevice(cmInstanceData_t *cmData,
                                                   CsrBtDeviceAddr theAddr)
{
    CsrUint8 count = 0;
    aclTable *acl = NULL;

    if (returnAclConnectionElement(cmData, theAddr, &acl) != CM_ERROR)
    {
#ifdef CSR_BT_LE_ENABLE
        if (acl->gattConnectionActive)
        {
            count += 1;
        }
#endif
        /* consider app's requested ACL entry also a stakeholder */
        if (acl->aclRequestedByApp)
        {
            count += 1;
        }
    }

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    count += numOfConnectionToPeerDeviceRfc(cmData, &theAddr);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    count += numOfConnectionToPeerDeviceL2cap(cmData, &theAddr);
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    count += numOfConnectionToPeerDeviceBnep(cmData, &theAddr);
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

    return count;
}

CsrUint8 CmReturnNumOfL2CRFCToPeerDevice(cmInstanceData_t *cmData,
                                                   CsrBtDeviceAddr theAddr)
{
    CsrUint8 count = 0;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    count += numOfConnectionToPeerDeviceRfc(cmData, &theAddr);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    count += numOfConnectionToPeerDeviceL2cap(cmData, &theAddr);
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

    return count;
}

CsrBool CsrBtCmElementCounterIncrement(cmInstanceData_t *cmData)
{
    CsrUintFast8 i;

    for (i = 0; i < CM_ERROR; i++)
    {
        cmData->elementCounter++;

        if (cmData->elementCounter == CM_ERROR)
        {
            cmData->elementCounter = 1;
        }

        if (CM_FIND_L2CA_ELEMENT(CsrBtCmL2caConnElementIndexCheck,
                                 &cmData->elementCounter))
        {
            /* Skip */
        }
        else if (CM_FIND_RFC_ELEMENT(CsrBtCmRfcConnElementIndexCheck,
                                     &cmData->elementCounter))
        {
            /* Skip */
        }
        else
        {
            return (TRUE);
        }
    }

    return FALSE;
}

#ifdef CSR_AMP_ENABLE
CsrUint8 CsrBtCmBtControllerActive(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr)
{
    CsrUint8 result;
    result = CTRL_ACTIVE_INACTIVE;

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    if (numOfConnectionToPeerDeviceBnep(cmData, &deviceAddr) > 0)
    {
        /* At least one bnep connection is connected. BNEP can not use
         * AMP at the moment, so this means BR/EDR is active */
        return CTRL_ACTIVE_BREDR_ACTIVE;
    }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */


#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    {
        cmRfcConnElement *rfcElm;
        for(rfcElm = CM_RFC_GET_FIRST(cmData->rfcVar.connList);
            rfcElm;
            rfcElm = rfcElm->next)
        {
            if(rfcElm->cmRfcConnInst)
            {
                cmRfcConnInstType *conn;
                conn = rfcElm->cmRfcConnInst;
                if(CsrBtBdAddrEq(&conn->deviceAddr, &deviceAddr))
                {
                    if (conn->state == CSR_BT_CM_RFC_STATE_CONNECTED)
                    {
                        if(conn->controller == CSR_BT_AMP_CONTROLLER_BREDR)
                        {
                            /* At least one RFCOMM connection using the BT controller */
                            return CTRL_ACTIVE_BREDR_ACTIVE;
                        }
                        else
                        {
                            /* We should not really have to continue
                             * scanning as RFCOMM uses just one L2CAP
                             * channel. Oh well... */
                            result = CTRL_ACTIVE_AMP_ONLY;
                        }
                    }
                }
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    {
        cmL2caConnElement *currentElem;
        for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
             currentElem;
             currentElem = currentElem->next)
        {
            /* Search through the L2CAP connection list */
            if (currentElem->cmL2caConnInst)
            {
                cmL2caConnInstType *conn = currentElem->cmL2caConnInst;
                
                if(CsrBtBdAddrEq(&(deviceAddr), &(conn->deviceAddr)))
                {
                    if (conn->state == CSR_BT_CM_L2CAP_STATE_CONNECTED)
                    {
                        if (conn->controller == CSR_BT_AMP_CONTROLLER_BREDR)
                        {
                            /* At least one L2CAP connection using the BT controller */
                            return CTRL_ACTIVE_BREDR_ACTIVE;
                        }
                        else
                        {
                            /* AMP is active, but keep scanning list
                             * to see if there is any BR/EDR
                             * activity */
                            result = CTRL_ACTIVE_AMP_ONLY;
                        }
                    }
                }
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

    return result;
}
#endif /* CSR_AMP_ENABLE */

CsrUint24 CsrBtCmReturnClassOfdevice(cmInstanceData_t *cmData)
{ /* This function return the Class of device */
    CsrUint24    classOfDevice = 0x000000;

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    cmL2caConnElement *currentL2caElem;
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    cmRfcConnElement *currentRfcElem;

    for (currentRfcElem = CM_RFC_GET_FIRST(cmData->rfcVar.connList);
         currentRfcElem; 
         currentRfcElem = currentRfcElem->next)
    { /* Goes through RFC connection table to find all CSR_BT_CONN_ID_INVALID and adds COD for them */
        if (currentRfcElem->cmRfcConnInst)
        {
            if (currentRfcElem->cmRfcConnInst->btConnId == CSR_BT_CONN_ID_INVALID ||
                currentRfcElem->cmRfcConnInst->pending)
            {
                classOfDevice |= currentRfcElem->cmRfcConnInst->classOfDevice;
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE
    for (currentL2caElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList); 
         currentL2caElem; 
         currentL2caElem = currentL2caElem->next)
    { /* Search through the L2CAP connection list to find all BTCONN_ID_RESERVED and
         adds COD for them */
        if (currentL2caElem->cmL2caConnInst)
        {
            if(currentL2caElem->cmL2caConnInst->btConnId == BTCONN_ID_RESERVED)
            {
                classOfDevice |= currentL2caElem->cmL2caConnInst->classOfDevice;
            }
        }
    }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    classOfDevice |= cmData->bnepVar.classOfDevice;
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
    classOfDevice |= cmData->dmVar.serviceCod;
    classOfDevice  = (classOfDevice & CSR_BT_CM_SERVICE_CLASS_FILTER);
    classOfDevice  = (classOfDevice | (cmData->dmVar.majorCod & CSR_BT_CM_MAJOR_DEVICE_CLASS_FILTER));
    classOfDevice  = (classOfDevice | (cmData->dmVar.minorCod & CSR_BT_CM_MINOR_DEVICE_CLASS_FILTER));
    return classOfDevice;
}

void CsrBtCmDmLockQueue(cmInstanceData_t *cmData)
{
    CsrPrim         *primPtr;

    primPtr = (CsrPrim *) cmData->recvMsgP;

    cmData->dmVar.lockMsg = *primPtr;

#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_DEBUG((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "[DM Queue] Locked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->dmVar.lockMsg));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "Locked Msg Id 0x%04x",cmData->dmVar.lockMsg));
#endif
}

void CsrBtCmDmUnlockQueue(cmInstanceData_t *cmData)
{
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_DEBUG((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "[DM Queue] Unlocked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->dmVar.lockMsg));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_DM_QUEUE, "Unlocked Msg Id 0x%04x",cmData->dmVar.lockMsg));
#endif

    cmData->dmVar.lockMsg = CSR_BT_CM_DM_QUEUE_UNLOCK;
}

void CsrBtCmSmLockQueue(cmInstanceData_t *cmData)
{
    CsrPrim         *primPtr;

    primPtr = (CsrPrim *) cmData->recvMsgP;

    cmData->smVar.smInProgress        = TRUE;
    cmData->smVar.smMsgTypeInProgress = *primPtr;

#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_DEBUG((CsrBtCmLto, CSR_BT_CM_LTSO_SM_QUEUE, "[SM Queue] Locked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->smVar.smMsgTypeInProgress));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_SM_QUEUE, "Locked Msg Id 0x%04x",cmData->smVar.smMsgTypeInProgress));
#endif
}

void CsrBtCmSmUnlockQueue(cmInstanceData_t *cmData)
{
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_DEBUG((CsrBtCmLto, CSR_BT_CM_LTSO_SM_QUEUE, "[SM Queue] Unlocked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->smVar.smMsgTypeInProgress));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_SM_QUEUE, "Unlocked Msg Id 0x%04x",cmData->smVar.smMsgTypeInProgress));
#endif

    cmData->smVar.smInProgress = FALSE;
}

void CmSdcLockQueue(cmInstanceData_t *cmData)
{
    CsrPrim         *primPtr;

    primPtr = (CsrPrim *) cmData->recvMsgP;
    cmData->sdcVar.lockMsg = *primPtr;

#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CM_LTSO_SDC_QUEUE, "[SDC Queue]Locked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->sdcVar.lockMsg));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CM_LTSO_SDC_QUEUE, "[SDC Queue]Locked Msg Id 0x%04x",cmData->sdcVar.lockMsg));
#endif
}

void CmSdcUnlockQueue(cmInstanceData_t *cmData)
{
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CM_LTSO_SDC_QUEUE, "[SDC Queue]Unlocked Msg Id MESSAGE:CsrBtCmPrim:0x%04x",cmData->sdcVar.lockMsg));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, CM_LTSO_SDC_QUEUE, "[SDC Queue]Unlocked Msg Id 0x%04x",cmData->sdcVar.lockMsg));
#endif

    cmData->sdcVar.lockMsg = CM_SDC_QUEUE_UNLOCK;
}

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
CsrBool updateLogicalChannelTypeMaskAndNumber(cmInstanceData_t *cmData, CsrBtDeviceAddr *theAddr)
{
    CsrUint8    count = 0;
    CsrUintFast8    i;
    aclTable    *aclConnectionElement = NULL;
    CsrBtLogicalChannelType    logicalChannelTypeMaskSum = CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL;
    CsrBool     needUpdate = FALSE;
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE) || !defined(EXCLUDE_CSR_BT_BNEP_MODULE)
    CsrBtLogicalChannelType    checkMask = CSR_BT_ACTIVE_CONTROL_CHANNEL | CSR_BT_ACTIVE_DATA_CHANNEL;
    CsrBool     stopSearch = FALSE;
#endif

    /* Start by finding the proper ACL link table */
    for ( i = 0; ((i < NUM_OF_ACL_CONNECTION)  && (aclConnectionElement == NULL)); i++)
    {
        if( CsrBtBdAddrEq(theAddr, &(cmData->roleVar.aclVar[i].deviceAddr)))
        {
            /* Found */
            aclConnectionElement = &(cmData->roleVar.aclVar[i]);
        }
    }

    if (aclConnectionElement)
    {
#ifndef  EXCLUDE_CSR_BT_L2CA_MODULE
        /* Now check all the L2CAP connections to the given bd address */
        cmL2caConnElement *currentElem;

        for (currentElem = CM_L2CA_GET_FIRST(cmData->l2caVar.connList);
             currentElem; 
             currentElem = currentElem->next)
        { /* Search through the L2CAP connection list */
            if (currentElem->cmL2caConnInst)
            {
                if ((currentElem->cmL2caConnInst->state == CSR_BT_CM_L2CAP_STATE_CONNECTED) &&
                    (CsrBtBdAddrEq(theAddr, &(currentElem->cmL2caConnInst->deviceAddr))) )
                {
                    if (currentElem->cmL2caConnInst->logicalChannelData)
                    {
                        logicalChannelTypeMaskSum |= CSR_BT_ACTIVE_DATA_CHANNEL;
                    }

                    if (currentElem->cmL2caConnInst->logicalChannelControl)
                    {
                        logicalChannelTypeMaskSum |= CSR_BT_ACTIVE_CONTROL_CHANNEL;
                    }

                    if (currentElem->cmL2caConnInst->logicalChannelStream)
                    {
                        logicalChannelTypeMaskSum |= CSR_BT_ACTIVE_STREAM_CHANNEL;

                        /* A2DP stream channels are only possible in L2CAP connections */
                        count++;
                    }
                }
            }
        }
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

        {
            /* Check all the RFCOMM connections to the given bd address unless control and data channels already found */
            cmRfcConnElement *currentElement;
            stopSearch = ((logicalChannelTypeMaskSum & checkMask) == checkMask);

            for (currentElement = CM_RFC_GET_FIRST(cmData->rfcVar.connList);
                 (currentElement && !stopSearch);
                 currentElement = currentElement->next)
            {
                if (currentElement->cmRfcConnInst)
                {
                    if (CSR_BT_CM_RFC_STATE_CONNECTED == currentElement->cmRfcConnInst->state &&
                        CsrBtBdAddrEq(theAddr, &(currentElement->cmRfcConnInst->deviceAddr)))
                    {
                        if (currentElement->cmRfcConnInst->logicalChannelData)
                        {
                            logicalChannelTypeMaskSum |= CSR_BT_ACTIVE_DATA_CHANNEL;
                        }

                        if (currentElement->cmRfcConnInst->logicalChannelControl)
                        {
                            logicalChannelTypeMaskSum |= CSR_BT_ACTIVE_CONTROL_CHANNEL;
                        }

                        stopSearch = ((logicalChannelTypeMaskSum & checkMask) == checkMask);
                    }
                }
            }
        }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE

        {/* If data channel type already active, don't bother running through the BNEP table... */
            bnepTable *bnepEntry;
            stopSearch = ((logicalChannelTypeMaskSum & checkMask) == CSR_BT_ACTIVE_DATA_CHANNEL);
            for ( i = 0; ((i < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS) && !stopSearch); i++)
            {
                bnepEntry = &cmData->bnepVar.connectVar[i];
                if (bnepEntry->state == CSR_BT_CM_BNEP_STATE_CONNECTED &&
                     CsrBtBdAddrEq(theAddr, &(bnepEntry->deviceAddr)))
                {
                    logicalChannelTypeMaskSum |= bnepEntry->logicalChannelTypeMask;
                    stopSearch = ((logicalChannelTypeMaskSum & checkMask) == CSR_BT_ACTIVE_DATA_CHANNEL);
                }
            }

        }
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */

        if ((aclConnectionElement->logicalChannelTypeMask != logicalChannelTypeMaskSum) ||
            (aclConnectionElement->noOfGuaranteedLogicalChannels != count))
        {
            /* Need to update table and send event */
            aclConnectionElement->logicalChannelTypeMask        = logicalChannelTypeMaskSum;
            aclConnectionElement->noOfGuaranteedLogicalChannels = count;
            needUpdate = TRUE;
        }
    }
    /* else..... not possible at all; there must be an ACL connection
     * to the device address if there is an L2CAP or RFCOMM
     * connection! */

    return needUpdate;
}
#endif /* CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE */

#ifndef CSR_STREAMS_ENABLE
void CsrBtCmDataBufferEmptyCfmSend(CsrSchedQid appHandle, CsrUint16 context)
{
    CsrBtCmDataBufferEmptyCfm *prim = (CsrBtCmDataBufferEmptyCfm *)
                                       CsrPmemAlloc(sizeof(CsrBtCmDataBufferEmptyCfm));


    prim->type      = CSR_BT_CM_DATA_BUFFER_EMPTY_CFM;
    prim->context   = context;
    CsrBtCmPutMessage(appHandle, prim);
}

void CsrBtCmDataBufferEmptyReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmDataBufferEmptyReq *prim = (CsrBtCmDataBufferEmptyReq *) cmData->recvMsgP;

    if (CSR_BT_CONN_ID_IS_L2CA(prim->btConnId))
    {
#ifndef  EXCLUDE_CSR_BT_L2CA_MODULE
        cmL2caConnElement *connElem = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId, &prim->btConnId);

        if (connElem)
        {
            cmL2caConnInstType *l2capConn = connElem->cmL2caConnInst;

            if (l2capConn->txCount > 0)
            { /* There are still data elements pending in the L2CAP buffer */
                l2capConn->pendingBufferStatus = TRUE;
            }
            else
            { /* There are no data elements in the L2CAP buffer */
                CsrBtCmDataBufferEmptyCfmSend(l2capConn->appHandle, l2capConn->context);
            }
        }
#else
        /* Could not identify the btConnId, just ignore */
        ;
#endif
    }
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    else if (CSR_BT_CONN_ID_IS_RFC(prim->btConnId))
    {
        cmRfcConnElement *connElem  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId, &(prim->btConnId));
        if (connElem)
        {
            cmRfcConnInstType *rfcConn = connElem->cmRfcConnInst;

            if (rfcConn->dataControl.txCount > 0)
            { /* There are still data elements pending in the L2CAP buffer */
                rfcConn->dataControl.pendingBufferStatus = TRUE;
            }
            else
            { /* There are no data elements in the L2CAP buffer */
                CsrBtCmDataBufferEmptyCfmSend(rfcConn->appHandle, rfcConn->context);
            }
        }
    }
#endif
    else
    { /* Could not identify the btConnId, just ignore */
        ;
    }
}
#endif /* !CSR_STREAMS_ENABLE */

/* Register special upper-layer handler */
#ifdef CSR_BT_INSTALL_CM_HANDLE_REGISTER
void CsrBtCmRegisterHandlerReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmRegisterHandlerReq *prim;
    prim = (CsrBtCmRegisterHandlerReq*)cmData->recvMsgP;
    
    switch(prim->handlerType)
    {
        case CSR_BT_CM_HANDLER_TYPE_SD:
            cmData->sdHandle = prim->handle;
            break;

        case CSR_BT_CM_HANDLER_TYPE_SC:
            cmData->scHandle = prim->handle;
            break;

#ifdef CSR_AMP_ENABLE
        case CSR_BT_CM_HANDLER_TYPE_AMP:
            cmData->ampmHandle = prim->handle;
            break;
#endif

#ifdef CSR_BT_LE_ENABLE
        case CSR_BT_CM_HANDLER_TYPE_LE:
            cmData->leHandle = prim->handle;
            break;
#endif

#ifdef INSTALL_CONTEXT_TRANSFER
        case CSR_BT_CM_HANDLER_TYPE_CONN_OFFLOAD:
            cmData->offloadHandle = prim->handle;
            rfcomm_register_conn_cb(CmRfcConnectCb);
            break;
#endif

        default:
            /* invalid handler type - ignore */
            break;
    }
}
#endif

void CsrBtCmInitCompleted(cmInstanceData_t *cmData)
{
    /* CM init sequence completed */
    CSR_BT_CM_STATE_CHANGE(cmData->globalState, CSR_BT_CM_STATE_IDLE);

    /* Open up callback queue system flood gates */
    CsrBtCmCallbackUnblock(cmData);

    if ((cmData->dmVar.inquiryAppState == CM_INQUIRY_APP_STATE_RESTARTING) &&
        (cmData->dmVar.inquiryDmState == CM_INQUIRY_DM_STATE_IDLE))
    {
        /* Start inquiry if requested from app before CM was ready */
        CsrBtCmStartInquiry(cmData);
    }

#ifndef CSR_TARGET_PRODUCT_VM
    if (!cmData->rfcBuild)
#endif
    {
#if CSR_BT_SDP_MTU
        sds_config_req(CSR_BT_CM_IFACEQUEUE,
                       CSR_BT_SDP_MTU,
                       0); /* flags */
        sdc_config_req(CSR_BT_CM_IFACEQUEUE,
                       CSR_BT_SDP_MTU,
                       0); /* flags */
#endif
    }

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateBlueCoreInitializedEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED,
                         HCI_SUCCESS,
                         NULL,
                         NULL);
#endif
    CsrBtCmDmLocalQueueHandler();
    CsrBtCmServiceManagerLocalQueueHandler(cmData);
    CmSdcManagerLocalQueueHandler(cmData);
}

/* This function is used to handle pending primitive using callback function */
void CmHandlePendingPrim(cmInstanceData_t *cmData, 
                         CsrUint16 eventClass, 
                         CsrBtCmPrim event,
                         void* pContext,
                         CsrBool (*cmPendingMsgHandler)(cmInstanceData_t *cmData, void* msg, void* pContext),
                         void (*cmPendingMsgFree)(void* msg))
{
    if (cmPendingMsgHandler && cmPendingMsgFree)
    {
        CsrUint16            savedEventClass;
        void                *savedMsg;
        CsrMessageQueueType *tempQueue  = NULL;

        while(CsrMessageQueuePop(&cmData->smVar.saveQueue, &savedEventClass, &savedMsg))
        {
            if (savedEventClass == eventClass && ((*((CsrBtCmPrim*)savedMsg)) == event))
            {
                if (cmPendingMsgHandler(cmData, savedMsg, pContext))
                {
                    /* Message handled, Free the message */
                    cmPendingMsgFree(savedMsg);
                }
                else
                {
                    CsrMessageQueuePush(&tempQueue, savedEventClass, savedMsg);
                }
            }
            else
            {
                CsrMessageQueuePush(&tempQueue, savedEventClass, savedMsg);
            }
        }
        cmData->smVar.saveQueue = tempQueue;
    }
    else
    {
        CsrBtCmGeneralException(eventClass,
                                event,
                                cmData->globalState,
                                "CmHandlePendingPrim, callback handler Invalid");
    }
}

/* This function retries certain initialization sequence events which got failed in the first attempt.
 *
 * RETURN
 * TRUE  - The step is retried, and further steps of initialization shall be done after this retry.
 * FALSE - retry is not required, continue with the initialization sequence.
*/
static CsrBool cmInitRetryRequest(cmInstanceData_t *cmData,
                                  CmInitSeqEvent    event,
                                  CsrBtResultCode   resultCode,
                                  CsrBtSupplier     resultSupplier)
{
    CsrBool retried = FALSE;

    switch (event)
    {
#ifdef CM_INIT_WRITE_PAGESCAN_ACTIVITY
        case CM_INIT_SEQ_WRITE_PAGESCAN_ACTIVITY_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI &&
                resultCode != HCI_SUCCESS &&
                !cmData->dmVar.fallbackPerformed)
            {
                cmData->dmVar.fallbackPerformed = TRUE;
                dm_hci_write_pagescan_activity(HCI_PAGESCAN_INTERVAL_DEFAULT,
                                               HCI_PAGESCAN_WINDOW_DEFAULT,
                                               NULL);
                retried = TRUE;
            }
            break;
        };
#endif /* CM_INIT_WRITE_PAGESCAN_ACTIVITY */

#ifdef CM_INIT_WRITE_INQUIRY_SCAN_TYPE
        case CM_INIT_SEQ_WRITE_INQUIRY_SCAN_TYPE_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI && resultCode != HCI_SUCCESS)
            {
                if (!cmData->dmVar.fallbackPerformed)
                {
                    cmData->dmVar.fallbackPerformed = TRUE;
                    dm_hci_write_inquiry_scan_type(HCI_SCAN_TYPE_LEGACY,
                                                   NULL);
                    retried = TRUE;
                }
            }
            break;
        }
#endif /* CM_INIT_WRITE_INQUIRY_SCAN_TYPE */

#ifdef CM_INIT_WRITE_INQUIRY_MODE
        case CM_INIT_SEQ_WRITE_INQUIRY_MODE_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI && resultCode != HCI_SUCCESS)
            {
                if (!cmData->dmVar.fallbackPerformed)
                {
                    cmData->dmVar.fallbackPerformed = TRUE;
                    dm_hci_write_inquiry_mode(HCI_INQUIRY_MODE_STANDARD,
                                              NULL);
                    retried = TRUE;
                }
            }
            break;
        }
#endif /* CM_INIT_WRITE_INQUIRY_MODE */

#ifdef CM_INIT_WRITE_PAGE_SCAN_TYPE
        case CM_INIT_SEQ_WRITE_PAGE_SCAN_TYPE_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI && resultCode != HCI_SUCCESS)
            {
                if (!cmData->dmVar.fallbackPerformed)
                {
                    cmData->dmVar.fallbackPerformed = TRUE;
                    dm_hci_write_page_scan_type(HCI_SCAN_TYPE_LEGACY,
                                                NULL);
                    retried = TRUE;
                }
            }
            break;
        }
#endif /* CM_INIT_WRITE_PAGE_SCAN_TYPE */


#ifdef CM_INIT_WRITE_INQUIRYSCAN_ACTIVITY
        case CM_INIT_SEQ_WRITE_INQUIRYSCAN_ACTIVITY_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI &&
                resultCode != HCI_SUCCESS &&
                !cmData->dmVar.fallbackPerformed)
            {
                cmData->dmVar.fallbackPerformed = TRUE;
                dm_hci_write_inquiryscan_activity(HCI_INQUIRYSCAN_INTERVAL_DEFAULT,
                                                  HCI_INQUIRYSCAN_WINDOW_DEFAULT,
                                                  NULL);
                retried = TRUE;
            }
            break;
        }
#endif /* CM_INIT_WRITE_INQUIRYSCAN_ACTIVITY */

#ifdef CM_INIT_WRITE_VOICE_SETTING
        case CM_INIT_SEQ_WRITE_VOICE_SETTING_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI && resultCode != HCI_SUCCESS)
            {
                dm_hci_write_voice_setting(CSR_BT_VOICE_SETTING, NULL);
                retried = TRUE;
            }
            break;
        }
#endif /* CM_INIT_WRITE_VOICE_SETTING */

#ifdef CM_INIT_SCAN
        case CM_INIT_SEQ_WRITE_SCAN_ENABLE_CFM:
        {
            if (resultSupplier == CSR_BT_SUPPLIER_HCI && resultCode != HCI_SUCCESS)
            {
                CsrUint8 mode = returnConnectAbleParameters(cmData);
                dm_hci_write_scan_enable(mode, NULL);
                retried = TRUE;
            }
            break;
        }
#endif /* CM_INIT_SCAN */

        default:
            break;
    }

    return retried;
}

/* CmInitSequenceHandler
 *
 * This function takes care of the initialization sequence, based on the init event passed to it by the caller.
 * Order of initialization sequenece is defined by CmInitSeqEvent. Flag based configuration in order to enable/disable
 * individual initialization step is present inside csr_bt_config_global.h under CM_INIT_X.
 *
 * Through this, a configuration based initialization sequence can be created based on the platform requirements.
 *
 * Algorithm:
 *
 * CmInitSequenceHandler(EVENT)
 * IF EVENT MATCHES:
 *     IF NEXT_STEP_ALLOWED:
 *         PERFORM_NEXT_STEP
 *         RETURN
 *     ELSE
 *         SKIP_THIS_STEP
 *         CONTINUE_WITH_NEXT_EVENT_INLINE // this is as defined by CmInitSeqEvent
 * 
*/
void CmInitSequenceHandler(cmInstanceData_t *cmData,
                           CmInitSeqEvent    event,
                           CsrBtResultCode   resultCode,
                           CsrBtSupplier     resultSupplier)
{
    if (cmData->globalState != CSR_BT_CM_STATE_NOT_READY)
    {
        /* Raise exception, if we are not under CM initialization state and ignore the call. */
        CsrBtCmGeneralException(DM_PRIM,
                                *((dm_prim_t *)cmData->recvMsgP),
                                CSR_BT_CM_STATE_IDLE,
                                "CmInitSequenceHandler");
        return;
    }

    if (cmInitRetryRequest(cmData, event, resultCode, resultSupplier))
    {
        /* Fallback is required, this means the DM command has failed and needs to be retried. */
        CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CmInit: Fallback required %d resultCode %d resultSupplier %d",
                          event, resultCode, resultSupplier));
        return;
    }

#ifdef CM_INIT_BLUECORE_TRANSPORT
    if (event < CM_INIT_SEQ_AM_REGISTER_CFM)
    {
        dm_am_register_req(CSR_BT_CM_IFACEQUEUE);
        return;
    }
#endif /* CM_INIT_BLUECORE_TRANSPORT */

#ifdef CM_INIT_READ_LOCAL_VERSION
    if (event < CM_INIT_SEQ_READ_LOCAL_VER_INFO_CFM)
    {
        dm_sm_service_register_req(CSR_BT_CM_IFACEQUEUE,
                                   0, /* context */
                                   SEC_PROTOCOL_L2CAP,
                                   0x0001,
                                   TRUE,
                                   SECL_NONE,
                                   0,
                                   NULL);
        dm_hci_read_local_version(NULL);
        return;
    }
#endif /* CM_INIT_READ_LOCAL_VERSION */

#ifdef CM_INIT_READ_LOCAL_FEATURES
    if (event < CM_INIT_SEQ_READ_LOCAL_SUPP_FEATURES_CFM)
    {
         dm_hci_read_local_features(NULL);
         return;
    }
#endif /* CM_INIT_READ_LOCAL_FEATURES */

#ifdef CM_INIT_WRITE_PAGE_TO
    if (event < CM_INIT_SEQ_WRITE_PAGE_TIMEOUT_CFM)
    {
        dm_hci_write_page_to(CSR_BT_PAGE_TIMEOUT_DEFAULT, NULL);
        return;
    }
#endif /* CM_INIT_WRITE_PAGE_TO */

#ifdef CM_INIT_LE_READ_LOCAL_SUPP_FEATURES
    if (event < CM_INIT_SEQ_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM)
    {
#if defined(EXCLUDE_CSR_BT_SC_MODULE) && defined(CSR_BT_LE_ENABLE)
        dm_hci_ulp_read_local_supported_features_req(NULL);
        return;
#endif /* EXCLUDE_CSR_BT_SC_MODULE && CSR_BT_LE_ENABLE */
    }
#endif /* CM_INIT_LE_READ_LOCAL_SUPP_FEATURES */

#ifdef CM_INIT_WRITE_PAGESCAN_ACTIVITY
    if (event < CM_INIT_SEQ_WRITE_PAGESCAN_ACTIVITY_CFM)
    {
        cmData->dmVar.fallbackPerformed = FALSE;
        dm_hci_write_pagescan_activity(CSR_BT_PAGE_SCAN_INTERVAL_DEFAULT, CSR_BT_PAGE_SCAN_WINDOW_DEFAULT, NULL);
        return;
    }
#endif /* CM_INIT_WRITE_PAGESCAN_ACTIVITY */

#ifdef CM_INIT_WRITE_INQUIRY_SCAN_TYPE
    if (event < CM_INIT_SEQ_WRITE_INQUIRY_SCAN_TYPE_CFM)
    {
        cmData->dmVar.fallbackPerformed = FALSE;
        dm_hci_write_inquiry_scan_type(cmData->dmVar.inquiryscanType,
                                       NULL);
        return;
    }
#endif

#ifdef CM_INIT_WRITE_INQUIRY_MODE
    if (event < CM_INIT_SEQ_WRITE_INQUIRY_MODE_CFM)
    {
        cmData->dmVar.fallbackPerformed = FALSE;
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
        if (cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            dm_hci_write_inquiry_mode(HCI_INQUIRY_MODE_WITH_EIR,
                                      NULL);
        }
        else
#endif
        {
            dm_hci_write_inquiry_mode(HCI_INQUIRY_MODE_WITH_RSSI,
                                      NULL);
        }
        return;
    }
#endif /* CM_INIT_WRITE_INQUIRY_MODE */

#ifdef CM_INIT_SET_BT_VERSION
    if (event < CM_INIT_SEQ_SET_BT_VERSION_CFM)
    {
        if (cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            dm_set_bt_version(DM_BT_VERSION, NULL);
            return;
        }
    }
#endif

#ifdef CM_INIT_LP_WRITE_ROLESWITCH_POLICY_REQ
    if (event < CM_INIT_SEQ_LP_WRITE_ROLESWITCH_POLICY_CFM)
    {
        if (cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            CsrUint16 *tab = CsrPmemZalloc(sizeof(CsrUint16));
            dm_lp_write_roleswitch_policy_req(0,        /* version */
                                              1,        /* length */
                                              tab,      /* table */
                                              NULL);    /* pp_prim */
        }
        return;
    }
#endif /* CM_INIT_LP_WRITE_ROLESWITCH_POLICY_REQ */

#ifdef CM_INIT_SET_DEFAULT_LINK_POLICY_REQ
    if (event < CM_INIT_SEQ_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM)
    {
        if (cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            /* This sets both DM and HCI default policy */
            dm_set_default_link_policy_req((CSR_BT_DEFAULT_LOW_POWER_MODES | ENABLE_MS_SWITCH),
                                           (CSR_BT_DEFAULT_LOW_POWER_MODES | ENABLE_MS_SWITCH),
                                           NULL);
            return;
        }
    }
#endif /* CM_INIT_SET_DEFAULT_LINK_POLICY_REQ */

#ifdef CM_INIT_WRITE_PAGE_SCAN_TYPE
    if (event < CM_INIT_SEQ_WRITE_PAGE_SCAN_TYPE_CFM)
    {
        cmData->dmVar.fallbackPerformed = FALSE;
        dm_hci_write_page_scan_type(cmData->dmVar.pagescanType,
                                    NULL);
        return;
    }
#endif /* CM_INIT_WRITE_PAGE_SCAN_TYPE */

#ifdef CM_INIT_SM_INIT_REQ
    if (event < CM_INIT_SEQ_SM_INIT_CFM)
    {
        CsrBtCmGetSecurityConfIndSend(cmData,
                                      cmData->dmVar.lmpVersion);
        return;
    }
#endif /* CM_INIT_SM_INIT_REQ */

#ifdef CSR_BT_INSTALL_CM_AUTO_EIR
#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
    if (event < CM_INIT_SEQ_READ_LOCAL_NAME_CFM)
    {
        if (cmData->dmVar.lmpVersion >= CSR_BT_BLUETOOTH_VERSION_2P1)
        {
            /* LMP version is 2.1 or newer - read local name for EIR */
            dm_hci_read_local_name(NULL);
            return;
        }
    }

    if (event < CM_INIT_SEQ_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_CFM)
    {
        if (cmData->dmVar.localEirData &&
            !cmData->dmVar.localEirData->requestedName &&
            resultSupplier == CSR_BT_SUPPLIER_HCI &&
            resultCode == HCI_SUCCESS)
        {
            CsrBtCmEirUpdateName(cmData);
            return;
        }
    }
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 */
#endif /* CSR_BT_INSTALL_CM_AUTO_EIR */

#ifdef CM_INIT_WRITE_INQUIRYSCAN_ACTIVITY
    if (event < CM_INIT_SEQ_WRITE_INQUIRYSCAN_ACTIVITY_CFM)
    {
        dm_hci_write_inquiryscan_activity(cmData->dmVar.inquiryscanInterval,
                                          cmData->dmVar.inquiryscanWindow,
                                          NULL);
        return;
    }
#endif

#ifdef CM_INIT_WRITE_VOICE_SETTING
    if (event < CM_INIT_SEQ_WRITE_VOICE_SETTING_CFM)
    {
        /* Setup the voice setting */
        dm_hci_write_voice_setting(CSR_BT_VOICE_SETTING,
                                   NULL);
        return;
    }
#endif

#ifdef CM_INIT_CLASS_OF_DEVICE
    if (event < CM_INIT_SEQ_WRITE_COD_CFM)
    {
        CsrUint24 classOfDevice = CsrBtCmReturnClassOfdevice(cmData);
        if (classOfDevice != cmData->dmVar.codWrittenToChip)
        {
            cmData->dmVar.pendingCod = classOfDevice;
            dm_hci_write_class_of_device(classOfDevice, NULL);
            return;
        }
    }
#endif

#ifdef CM_INIT_SCAN
    if (event < CM_INIT_SEQ_WRITE_SCAN_ENABLE_CFM)
    {
        CsrUint8 mode = returnConnectAbleParameters(cmData);

        if (mode != cmData->dmVar.currentChipScanMode)
        {
            cmData->dmVar.pendingChipScanMode = mode;
            dm_hci_write_scan_enable(mode, NULL);
            return;
        }
    }
#endif

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
    if (event < CM_INIT_RFC_INIT_CFM)
    {
        RfcInitReqSend(CSR_BT_CM_IFACEQUEUE);
        return;
    }
#endif

#if !defined(EXCLUDE_CSR_BT_SCO_MODULE) && !defined(EXCLUDE_CSR_BT_RFC_MODULE)
    if (event < CM_INIT_SYNC_REGISTER_CFM)
    {
        dm_sync_register_req(CSR_BT_CM_IFACEQUEUE, 0);
        return;
    }
#endif

#ifdef EXCLUDE_CSR_BT_SC_MODULE
    if (event < CM_INIT_SM_ADD_DEVICE_CFM)
    {
        /* Always require unauthenticated link key on RFCOMM PSM */
        dm_sm_service_register_req(CSR_BT_CM_IFACEQUEUE,
                                   0, /* context */
                                   SEC_PROTOCOL_L2CAP,
                                   RFCOMM_PSM,
                                   TRUE,
                                   SECL4_IN_SSP | SECL4_OUT_SSP,
                                   CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                   NULL);

        /* Start adding device records from persistent store to Bluestack */
        CsrBtCmSmDbAddDeviceIndex(cmData, 0);
        return;
    }
#endif

    if (event < CM_INIT_COMPLETED)
    {
        CsrBtCmInitCompleted(cmData);
    }
}

CsrBool CmSearchPendingListByType(CsrCmnListElm_t *elem, void *data)
{
    cmPendingMsg_t *pendingMsg = (cmPendingMsg_t*) elem;
    CsrUint8 *type = (CsrUint8 *)data;

    if (pendingMsg->type == (*type))
    {
        return TRUE;
    }
    return FALSE;
}

