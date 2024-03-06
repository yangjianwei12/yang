/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

#include "csr_bt_cm_sdc.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_common_amp.h"
#include "csr_bt_cm_le.h"


#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_panic.h"
#endif

#ifdef CSR_STREAMS_ENABLE
/* This function is used only when streams are supported */
static void csrBtCmCommonDataPrimHandler(cmInstanceData_t *cmData)
{
    CSR_UNUSED(cmData);
    CsrPanic(CSR_TECH_BT,
             CSR_BT_PANIC_UNSUPPORTED_FEATURE,
             "Data write request");
}
#endif /* CSR_STREAMS_ENABLE */

#ifdef CSR_TARGET_PRODUCT_VM
static void CsrBtCmUpdateInternalPeerAddressReqHandler(cmInstanceData_t *cmData);

static void CsrBtCmUpdateInternalPeerAddressReqHandler(cmInstanceData_t *cmData)
{
    CsrUint8 index;
    CsrBtCmUpdateInternalPeerAddressReq *req = (CsrBtCmUpdateInternalPeerAddressReq *) cmData->recvMsgP;

    for (index = 0; index < NUM_OF_ACL_CONNECTION ; index++)
    {
        /* Check if ACL instance is already prsent */
        if (CsrBtBdAddrEq(&cmData->roleVar.aclVar[index].deviceAddr, &req->oldPeerAddr))
        {
            cmData->roleVar.aclVar[index].deviceAddr = req->newPeerAddr;
            break;
        }
    }
}

static void cmL2caConnectReqLockAclforPeer(cmInstanceData_t *cmData)
{
    CsrBtCmL2caConnectReq  *prim = (CsrBtCmL2caConnectReq *) cmData->recvMsgP;
    aclTable *aclConnectionElement;
    returnAclConnectionElement(cmData, prim->addr, &aclConnectionElement);
    if (aclConnectionElement)
    {
        if ((prim->remotePsm == 0xfeff /*Peer Signalling Profile*/ ||
             prim->remotePsm == 0xfefd /*Handover Profile*/ ||
             prim->remotePsm == 0xfefb /*Mirror Profile*/) &&
             aclConnectionElement->aclLockedForPeer == FALSE)
        {
            CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "cmL2caConnectReqLockAclforPeer, REMOTEPSM:%02x, LOCK STATE:%d",
                               prim->remotePsm, aclConnectionElement->aclLockedForPeer));

            TYPED_BD_ADDR_T ad;
            ad.addr = prim->addr;
            ad.type = CSR_BT_ADDR_PUBLIC;
            dm_acl_open_req(&ad, 0, NULL);
            aclConnectionElement->aclLockedForPeer = TRUE;
        }
    }
}
#else
#define CsrBtCmUpdateInternalPeerAddressReqHandler NULL
#endif

/***********************************************************************************
 * CM profile provider jump table
 ***********************************************************************************/
static const SignalHandlerType cmProviderHandlers[CSR_BT_CM_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtCmInquiryReqHandler,                         /* CSR_BT_CM_INQUIRY_REQ */
    CsrBtCmCancelInquiryReqHandler,                   /* CSR_BT_CM_CANCEL_INQUIRY_REQ */
    CsrBtCmDmScoAcceptConnectReqHandler,              /* CSR_BT_CM_SCO_ACCEPT_CONNECT_REQ */
    CsrBtCmDmScoCancelAcceptConnectReqHandler,        /* CSR_BT_CM_SCO_CANCEL_ACCEPT_CONNECT_REQ */
    CsrBtCmL2caDataWriteReqHandler,                   /* CSR_BT_CM_L2CA_DATA_REQ */
    CsrBtCmSmAuthoriseResHandler,                     /* CSR_BT_CM_SM_AUTHORISE_RES */
    CsrBtCmSmPinRequestResHandler,                    /* CSR_BT_CM_SM_PIN_REQUEST_RES */
    CsrBtCmRfcUnRegisterReqMsgSend,                   /* CSR_BT_CM_UNREGISTER_REQ */
    CsrBtCmRfcDataReqHandler,                         /* CSR_BT_CM_DATA_REQ */
    CsrBtCmRfcDataResHandler,                         /* CSR_BT_CM_DATA_RES */
    CsrBtCmRfcControlReqMsgSend,                      /* CSR_BT_CM_CONTROL_REQ */
    CsrBtCmRfcPortNegResHandler,                      /* CSR_BT_CM_PORTNEG_RES */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    CsrBtCmBnepExtendedDataReqHandler,                /* CSR_BT_CM_BNEP_EXTENDED_DATA_REQ */
    CsrBtCmBnepExtendedMulticastDataReqHandler,       /* CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ */
    CsrBtCmBnepDisconnectResHandler,                  /* CSR_BT_CM_BNEP_DISCONNECT_RES */
#else
    NULL,                                             /* CSR_BT_CM_BNEP_EXTENDED_DATA_REQ */
    NULL,                                             /* CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ */
    NULL,                                             /* CSR_BT_CM_BNEP_DISCONNECT_RES */
#endif /* EXCLUDE_CSR_BT_BNEP_MODULE */
    CsrBtCmSdcCancelSearchReqHandler,                 /* CSR_BT_CM_SDC_CANCEL_SEARCH_REQ */
    CsrBtCmSdcAttrReqHandle,                          /* CSR_BT_CM_SDC_ATTRIBUTE_REQ */
    CsrBtCmSdcCloseReqHandler,                        /* CSR_BT_CM_SDC_CLOSE_REQ */
    CsrBtCmCancelConnectReqHandler,                   /* CSR_BT_CM_CANCEL_CONNECT_REQ */
    CsrBtCmSmCancelConnectReqHandler,                 /* CSR_BT_CM_SM_CANCEL_CONNECT_REQ */
    CsrBtCml2caCancelConnectReqHandler,               /* CSR_BT_CM_CANCEL_L2CA_CONNECT_REQ */
    CsrBtCmAlwaysSupportMasterRoleReqHandler,         /* CSR_BT_CM_ALWAYS_SUPPORT_MASTER_ROLE_REQ */
    CsrBtCmCancelReadRemoteNameReqHandler,            /* CSR_BT_CM_CANCEL_READ_REMOTE_NAME_REQ */
    CsrBtCmSmBondingCancelReqHandler,                 /* CSR_BT_CM_SM_BONDING_CANCEL_REQ */
    CsrBtCmSmIoCapabilityRequestResHandler,           /* CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES */
    CsrBtCmSmIoCapabilityRequestNegResHandler,        /* CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_NEG_RES */
    CsrBtCmSmUserConfirmationRequestResHandler,       /* CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_RES */
    CsrBtCmSmUserConfirmationRequestNegResHandler,    /* CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_NEG_RES */
    CsrBtCmSmUserPasskeyRequestResHandler,            /* CSR_BT_CM_SM_USER_PASSKEY_REQUEST_RES */
    CsrBtCmSmUserPasskeyRequestNegResHandler,         /* CSR_BT_CM_SM_USER_PASSKEY_REQUEST_NEG_RES */
    CsrBtCmSmSendKeypressNotificationReqHandler,      /* CSR_BT_CM_SM_SEND_KEYPRESS_NOTIFICATION_REQ */
    CsrBtCmSmRepairResHandler,                        /* CSR_BT_CM_SM_REPAIR_RES */
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    CsrBtCmBnepCancelConnectReqHandler,               /* CSR_BT_CM_CANCEL_BNEP_CONNECT_REQ */
#else
    NULL,                                             /* CSR_BT_CM_CANCEL_BNEP_CONNECT_REQ */
#endif
    CsrBtCmRoleSwitchConfigReqHandler,                /* CSR_BT_CM_ROLE_SWITCH_CONFIG_REQ */
    CsrBtCmSetEventMaskReqHandler,                    /* CSR_BT_CM_SET_EVENT_MASK_REQ */
    CsrBtCmModeChangeConfigReqHandler,                /* CSR_BT_CM_MODE_CHANGE_CONFIG_REQ */
    CsrBtCmL2caUnRegisterReqHandler,                  /* CSR_BT_CM_L2CA_UNREGISTER_REQ */
    CsrBtCmWriteLinkSuperVTimeoutReqHandler,          /* CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_REQ */
    CsrBtCmAmpMoveChannelReqHandler,                  /* CSR_BT_CM_MOVE_CHANNEL_REQ */
    CsrBtCmAmpMoveChannelResHandler,                  /* CSR_BT_CM_MOVE_CHANNEL_RES */
    CsrBtCmL2caConnlessDataReqHandler,                /* CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_REQ */
    CsrBtCmLogicalChannelTypeHandler,                 /* CSR_BT_CM_LOGICAL_CHANNEL_TYPE_REQ */
    CsrBtCmRfcPortnegReqHandler,                      /* CSR_BT_CM_PORTNEG_REQ */
    CsrBtCmReadRemoteVersionReqHandler,               /* CSR_BT_CM_READ_REMOTE_VERSION_REQ */
    CsrBtCmRegisterHandlerReqHandler,                 /* CSR_BT_CM_REGISTER_HANDLER_REQ */
    CsrBtCmL2caDataResHandler,                        /* CSR_BT_CM_L2CA_DATA_RES */
    CsrBtCmL2caDataAbortReqHandler,                   /* CSR_BT_CM_L2CA_DATA_ABORT_REQ */
    CsrBtCmA2DPBitrateHandler,                        /* CSR_BT_CM_A2DP_BIT_RATE_REQ */
    CsrBtCmGetSecurityConfResHandler,                 /* CSR_BT_CM_GET_SECURITY_CONF_RES */  
    CsrBtCmDataBufferEmptyReqHandler,                 /* CSR_BT_CM_DATA_BUFFER_EMPTY_REQ */
    CsrBtCmLeScanReqHandler,                          /* CSR_BT_CM_LE_SCAN_REQ */
    CsrBtCmLeAdvertiseReqHandler,                     /* CSR_BT_CM_LE_ADVERTISE_REQ */
    CsrBtCmLeWhitelistSetReqHandler,                  /* CSR_BT_CM_LE_WHITELIST_SET_REQ */
    CsrBtCmLeConnparamReqHandler,                     /* CSR_BT_CM_LE_CONNPARAM_REQ */
    CsrBtCmLeConnparamUpdateReqHandler,               /* CSR_BT_CM_LE_CONNPARAM_UPDATE_REQ */
    CsrBtCmLeAcceptConnparamUpdateResHandler,         /* CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES */
    CsrBtCmEirFlagsReqHandler,                        /* CSR_BT_CM_EIR_FLAGS_REQ */
    CsrBtCmReadEncryptionKeySizeReqHandler,           /* CSR_BT_CM_READ_ENCRYPTION_KEY_SIZE_REQ */
    CsrBtCmReadAdvertisingChTxPowerReqHandler,        /* CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ */
    CsrBtCmLeReceiverTestReqHandler,                  /* CSR_BT_CM_LE_RECEIVER_TEST_REQ */
    CsrBtCmLeTransmitterTestReqHandler,               /* CSR_BT_CM_LE_TRANSMITTER_TEST_REQ */
    CsrBtCmLeTestEndReqHandler,                       /* CSR_BT_CM_LE_TEST_END_REQ */
    CsrBtCmLeUnLockSmQueueHandler,                    /* CSR_BT_CM_LE_UNLOCK_SM_QUEUE_REQ */
    CsrBtCmLeGetControllerInfoReqHandler,             /* CSR_BT_CM_LE_GET_CONTROLLER_INFO_REQ */
    CsrBtCmMapScoPcmResHandler,                       /* CSR_BT_CM_MAP_SCO_PCM_RES */
    CsrBtCmL2caGetChannelInfoReqHandler,              /* CSR_BT_CM_L2CA_GET_CHANNEL_INFO_REQ */
    CsrBtCmSetAvStreamInfoReqHandler,                 /* CSR_BT_CM_SET_AV_STREAM_INFO_REQ */
    CsrBtCmLeReadRemoteUsedFeaturesReqHandler,        /* CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_REQ */
    CsrBtCmLeReadLocalSupportedFeaturesReqHandler,    /* CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_REQ */
    CsrBtCmLeReadResolvingListSizeReqHandler,         /* CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_REQ */
    CsrBtCmLeSetPrivacyModeReqHandler,                /* CSR_BT_CM_LE_SET_PRIVACY_MODE_REQ */
    CsrBtCmLeSetOwnAddressTypeReqHandler,             /* CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ */
    CsrBtCmLeSetStaticAddressReqHandler,              /* CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ */
    CsrBtCmLeSetPvtAddrTimeoutReqHandler,             /* CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ */
    CsrBtCmSmAddDeviceReqHandler,                     /* CSR_BT_CM_SM_ADD_DEVICE_REQ */
    CsrBtCmLeReadLocalIrkReqHandler,                  /* CSR_BT_CM_LE_READ_LOCAL_IRK_REQ */
    CsrBtCmUpdateInternalPeerAddressReqHandler,       /* CSR_BT_CM_UPDATE_INTERNAL_PEER_ADDR_REQ */
    CsrBtCmAclOpenReqHandler,                         /* CSR_BT_CM_ACL_OPEN_REQ */
    CsrBtCmRfcDisconnectRspHandler,                   /* CSR_BT_CM_RFC_DISCONNECT_RSP */
    CsrBtCmL2caDisconnectRspHandler,                  /* CSR_BT_CM_L2CA_DISCONNECT_RSP */
    CsrBtCmRfcConnectAcceptRspHandler,                /* CSR_BT_CM_RFC_CONNECT_ACCEPT_RSP */
    CsrBtCmL2caConnectAcceptRspHandler,               /* CSR_BT_CM_L2CA_CONNECT_ACCEPT_RSP */
    CmL2caTpConnectAcceptRspHandler,                  /* CM_L2CA_TP_CONNECT_ACCEPT_RSP */
    CmL2caAddCreditReqHandler,                        /* CM_L2CA_ADD_CREDIT_REQ */
    CmSmRefreshEncryptionKeyReqHandler,               /* CM_SM_REFRESH_ENCRYPTION_KEY_REQ */
    CmSmGenerateCrossTransKeyRequestRspHandler,       /* CM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP */
    CmExtScanFilteredAdvReportDoneIndHandler,         /* CM_EXT_SCAN_FILTERED_ADV_REPORT_DONE_IND */
    CmLeAddDeviceToWhiteListReqHandler,               /* CM_LE_ADD_DEVICE_TO_WHITE_LIST_REQ */
    CmLeRemoveDeviceFromWhiteListReqHandler,          /* CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_REQ */
    CmPeriodicScanSyncAdvReportDoneIndHandler,        /* CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND */  
    CmSmKeyRequestRspHandler,                         /* CM_SM_KEY_REQUEST_RSP */
    CmL2caPingReqHandler,                             /* CM_L2CA_PING_REQ */
    CmL2caPingRspHandler,                             /* CM_L2CA_PING_RSP */
    CmLeEnhancedReceiverTestReqHandler,               /* CM_LE_ENHANCED_TRANSMITTER_TEST_REQ */
    CmLeEnhancedTransmitterTestReqHandler,            /* CM_LE_ENHANCED_RECEIVER_TEST_REQ */
    CmSmConfigReqHandler,                             /* CM_SM_CONFIG_REQ */
    /* These three must ALWAYS be at the end */
    CsrBtCmRestoreServiceManagerQueue,                /* CSR_BT_CM_SM_HOUSE_CLEANING */
    CsrBtCmDmRestoreQueueHandler,                     /* CSR_BT_CM_DM_HOUSE_CLEANING */
    CmSdcRestoreSdcQueueHandler                       /* CM_SDC_HOUSE_CLEANING */
};

/*************************************************************************************
  CsrBtCmProfileProvider:
************************************************************************************/
void CsrBtCmProfileProvider(cmInstanceData_t *cmData)
{
    CsrUint16 id = *(CsrPrim*)cmData->recvMsgP;
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_DEBUG((CsrBtCmLto, 0, "CsrBtCmProfileProvider MESSAGE:CsrBtCmPrim:0x%x",id));
#endif

    if (id >= CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST &&
        id <= CSR_BT_CM_PRIM_DOWNSTREAM_HIGHEST &&
        cmProviderHandlers[(CsrUint16)(id - CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)] != NULL)
    {
        /* Use jump table */
        cmProviderHandlers[(CsrUint16) (id - CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)](cmData);
    }
    else if (id <= CSR_BT_CM_DM_PRIM_DOWNSTREAM_HIGHEST &&
             id != CSR_BT_CM_EN_ENABLE_ENHANCEMENTS_REQ)
    {
        /* Defer to DM provider */
        CsrBtCmDmProvider(cmData);
    }
    else if (id == CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ)
    {
        /* Defer to SM provider, but avoid locking */
        CsrBtCmServiceManagerL2caConnectAcceptProvider(cmData);
    }
    else if (id == CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_REQ)
    {
        /* Defer to SM provider, but avoid locking */
        CsrBtCmServiceManagerL2caCancelConnectAcceptProvider(cmData);
    }
    else if (id == CSR_BT_CM_CANCEL_ACCEPT_CONNECT_REQ)
    {
        /* Defer to SM provider, but avoid locking */
        CsrBtCmServiceManagerCancelAcceptConnectProvider(cmData);
    }
    else if (id >= CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST &&
             id <= CSR_BT_CM_SM_PRIM_DOWNSTREAM_HIGHEST)
    {
#ifdef CSR_TARGET_PRODUCT_VM
            if (id == CSR_BT_CM_L2CA_CONNECT_REQ)
            {
                /* Defer to SM provider, but lock ACL if req is for peer device */
                cmL2caConnectReqLockAclforPeer(cmData);
            }
#endif
            /* Defer to SM provider */
            CsrBtCmServiceManagerProvider(cmData);
    }
    else if (id >= CM_SDC_PRIM_DOWNSTREAM_LOWEST &&
             id <= CM_SDC_PRIM_DOWNSTREAM_HIGHEST)
    {
        /* Defer to SDC provider */
        CmSdcManagerProvider(cmData);
    }
    else
    {
        /* Event error */
        CsrBtCmGeneralException(CSR_BT_CM_PRIM, id, cmData->globalState, "");
    }

    if (cmData->recvMsgP)
    {
        if (id >= CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)
        {
            CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM,
                                               cmData->recvMsgP);
        }
        else
        {
            CsrBtCmFreeDownstreamMessageContents(CSR_BT_CM_PRIM,
                                                 cmData->recvMsgP);
        }
    }
}

