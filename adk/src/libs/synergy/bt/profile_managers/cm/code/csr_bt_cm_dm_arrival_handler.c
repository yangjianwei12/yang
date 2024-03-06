/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_events_handler.h"
#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_callback_q.h"
#include "dm_prim.h"
#include "csr_log_text_2.h"

#ifdef CSR_BT_LE_ENABLE
#include "csr_bt_cm_le.h"

#if defined(CSR_BT_INSTALL_EXTENDED_SCANNING) ||                                \
    defined(CSR_BT_INSTALL_EXTENDED_ADVERTISING) ||                             \
    defined(CSR_BT_INSTALL_PERIODIC_SCANNING) ||                                \
    defined(CSR_BT_INSTALL_PERIODIC_ADVERTISING)
#define CSR_BT_INSTALL_CM_ADV_EXT
#endif

#endif

/* Handles private DM primitives */
static void csrBtCmDmPrivatePrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_AM_REGISTER_CFM:
        {
            if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
            {
                CmInitSequenceHandler(cmData,
                                      CM_INIT_SEQ_AM_REGISTER_CFM,
                                      CSR_BT_RESULT_CODE_CM_SUCCESS,
                                      CSR_BT_SUPPLIER_CM);
            }
            break;
        }

        case DM_SET_BT_VERSION_CFM:
        {
            if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
            {
                DM_SET_BT_VERSION_CFM_T *cfm = (DM_SET_BT_VERSION_CFM_T *)cmData->recvMsgP;

                /* We are currently in CM initialization phase, continue with the sequence. */
                CmInitSequenceHandler(cmData,
                                      CM_INIT_SEQ_SET_BT_VERSION_CFM,
                                      cfm->status,
                                      CSR_BT_SUPPLIER_HCI);
            }
            break;
        }

        case DM_LP_WRITE_ROLESWITCH_POLICY_CFM:
        {
            if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
            {
                DM_LP_WRITE_ROLESWITCH_POLICY_CFM_T *cfm = (DM_LP_WRITE_ROLESWITCH_POLICY_CFM_T *)cmData->recvMsgP;

                /* We are currently in CM initialization phase, continue with the sequence. */
                CmInitSequenceHandler(cmData,
                                      CM_INIT_SEQ_LP_WRITE_ROLESWITCH_POLICY_CFM,
                                      cfm->status,
                                      CSR_BT_SUPPLIER_HCI);
            }
            break;
        }

#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS
        case DM_WRITE_CACHED_PAGE_MODE_CFM:
                if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_CACHED_PAGE_MODE))
                {
                    CsrBtCmDmWriteCachedPageModeCfmHandler(cmData);
                }
                break;

        case DM_WRITE_CACHED_CLOCK_OFFSET_CFM:
                if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_CACHED_CLOCK_OFFSET))
                {
                    CsrBtCmDmWriteCachedClockOffsetCfmHandler(cmData);
                }
                break;

        case DM_CLEAR_PARAM_CACHE_CFM:
                if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_CLEAR_PARAM_CACHE))
                {
                    CsrBtCmDmLocalQueueHandler();
                }
                break;
#endif /* CSR_BT_INSTALL_CM_CACHE_PARAMS */

        case DM_ACL_OPEN_CFM:
            CsrBtCmDmAclOpenCfmHandler(cmData);
            break;

        case DM_ACL_OPENED_IND:
            CsrBtCmDmAclOpenedIndHandler(cmData);
            break;

        case DM_ACL_CONN_START_IND:
            CsrBtCmDmAclConnStartIndHandler(cmData);
            break;

        case DM_ULP_ADV_PARAM_UPDATE_IND:
            /* Indication handling is pending */
            break;

        case DM_ACL_CLOSED_IND:
            CsrBtCmDmAclCloseIndHandler(cmData);
            break;

        case DM_ACL_CLOSE_CFM:
            CsrBtCmDmAclCloseCfmHandler(cmData);
            break;

        case DM_HCI_SWITCH_ROLE_CFM:
            CsrBtCmDmHciSwitchRoleCompleteHandler(cmData);
            break;

        case DM_HCI_REMOTE_NAME_CFM:
            CsrBtCmDmHciRemoteNameCompleteHandler(cmData);
            break;

#ifdef CSR_BT_LE_ENABLE
        case DM_SET_BLE_CONNECTION_PARAMETERS_CFM:
            CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
            break;

        case DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM:
            CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
            break;

        case DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND:
            CsrBtCmLeAcceptConnparamUpdateIndHandler(cmData);
            break;

#ifdef DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_REQ
        case DM_ULP_SET_DATA_RELATED_ADDRESS_CHANGES_CFM:
            CsrBtCmLeSetDataRelatedAddressChangesCompleteHandler(cmData);
            break;
#endif

#endif

        case DM_BAD_MESSAGE_IND:
            /* Ignore */
#ifdef CSR_TARGET_PRODUCT_VM
/* ULCONV_TODO Need a design change in BlueStack, we should receive a
   disconnect for stream channel.*/
           CsrBtCmDmBadMessageIndHandler(cmData);
#endif
            break;

#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
        case DM_LP_WRITE_ALWAYS_MASTER_DEVICES_CFM:
            CsrBtCmAlwaysMasterDevicesCfmHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CHANGE_ACL_PACKET_TYPE
        case DM_HCI_CHANGE_ACL_CONN_PKT_TYPE_CFM:
            /* Ignore */
            break;
#endif

        case DM_LP_WRITE_POWERSTATES_CFM:
            CsrBtCmDmPowerSettingsCfmHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
        case DM_SET_LINK_BEHAVIOR_CFM:
            CsrBtCmDmSetLinkBehaviorCfmHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CRYPTO_SUPPORT
        case DM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM:
            CsrBtCmDmCryptoGeneratePublicPrivateKeyCfmHandler(cmData);
            break;

        case DM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM:
            CsrBtCmDmCryptoGenerateSharedSecretKeyCfmHandler(cmData);
            break;

        case DM_CRYPTO_ENCRYPT_CFM:
            CsrBtCmDmCryptoEncryptCfmHandler(cmData);
            break;

        case DM_CRYPTO_HASH_CFM:
            CsrBtCmDmCryptoHashCfmHandler(cmData);
            break;

        case DM_CRYPTO_DECRYPT_CFM:
            CsrBtCmDmCryptoDecryptCfmHandler(cmData);
            break;

        case DM_CRYPTO_AES_CTR_CFM:
            CsrBtCmDmCryptoAesCtrCfmHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_CRYPTO_SUPPORT */

        case DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM:
            CmDmWriteScHostSupportOverrideCfmHandler(cmData);
            break;

        case DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM:
            CmDmReadScHostSupportOverrideMaxBdAddrCfmHandler(cmData);
            break;

#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
        case DM_ULP_SET_PHY_CFM:
            CmDmLeSetPhyCfmHandler(cmData);
            break;

        case DM_ULP_SET_DEFAULT_PHY_CFM:
            CmDmLeSetDefaultPhyCfmHandler(cmData);
            break;

        case DM_ULP_PHY_UPDATE_IND:
            CmDmLeSetDefaultPhyIndHandler(cmData);
            break;

        case DM_ULP_READ_PHY_CFM:
            CmDmLeReadPhyCfmHandler(cmData);
            break;

#endif /* INSTALL_CM_LE_PHY_UPDATE_FEATURE */

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE
        case DM_HCI_ULP_SET_DEFAULT_SUBRATE_CFM:
            CsrBtCmLeSetDefaultSubrateCfmHandler(cmData);
            break;

        case DM_HCI_ULP_SUBRATE_CHANGE_CFM:
            CsrBtCmLeSubrateChangeCfmHandler(cmData);
            break;

        case DM_HCI_ULP_SUBRATE_CHANGE_IND:
            CsrBtCmLeSubrateChangeIndHandler(cmData);
            break;
#endif

#ifdef INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL
        case DM_ULP_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM:
            CmDmLeReadRemoteTransmitPowerLevelCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING
        case DM_ULP_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM:
            CmDmLeSetTransmitPowerReportingEnableCfmHandler(cmData);
            break;

        case DM_ULP_TRANSMIT_POWER_REPORTING_IND:
            CmDmLeTransmitPowerReportingIndHandler(cmData);
            break;
#endif /* INSTALL_CM_LE_SET_TRANSMIT_POWER_REPORTING */

#ifdef INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL
        case DM_ULP_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM:
            CmDmLeEnhancedReadTransmitPowerLevelCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL */

#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
        case DM_ULP_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM:
            CmDmLeSetPathLossReportingParametersCfmHandler(cmData);
            break;

        case DM_ULP_SET_PATH_LOSS_REPORTING_ENABLE_CFM:
            CmDmLeSetPathLossReportingEnableCfmHandler(cmData);
            break;

        case DM_ULP_PATH_LOSS_THRESHOLD_IND:
            CmDmLePathLossThresholdIndHandler(cmData);
            break;
#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmSyncPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
        case DM_SYNC_REGISTER_CFM:
        {
            if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
            {
                CmInitSequenceHandler(cmData,
                                      CM_INIT_SYNC_REGISTER_CFM,
                                      CSR_BT_RESULT_CODE_CM_SUCCESS,
                                      CSR_BT_SUPPLIER_CM);
            }
            else
            {
                CsrBtCmDmSyncRegisterCfmHandler(cmData);
            }
            break;
        }

        case DM_SYNC_CONNECT_CFM:
            CsrBtCmDmSyncConnectCfmHandler(cmData);
            break;

        case DM_SYNC_CONNECT_IND:
            CsrBtCmDmSyncConnectIndHandler(cmData);
            break;

        case DM_SYNC_DISCONNECT_CFM:
            CsrBtCmDmSyncDisconnectCfmHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
        case DM_SYNC_RENEGOTIATE_CFM:
            CsrBtCmDmSyncRenegotiateCfmHandler(cmData);
            break;
#endif

        case DM_SYNC_CONNECT_COMPLETE_IND:
            CsrBtCmDmSyncConnectCompleteIndHandler(cmData);
            break;

        case DM_SYNC_DISCONNECT_IND:
            CsrBtCmDmSyncDisconnectIndHandler(cmData);
            break;

        case DM_SYNC_RENEGOTIATE_IND:
            CsrBtCmDmSyncRenegotiateIndHandler(cmData);
            break;
#endif /* ifndef EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_REGISTER_CFM:
            CsrBtCmDmIsocRegisterCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CONFIGURE_CIG_CFM:
            CsrBtCmDmIsocConfigureCigCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CONFIGURE_CIG_TEST_CFM:
            CsrBtCmDmIsocConfigureCigTestCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_REMOVE_CIG_CFM:
            CsrBtCmDmIsocRemoveCigCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CIS_CONNECT_IND:
            CsrBtCmDmIsocCisConnectIndHandler(cmData, dmPrim);

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND
            CsrBtCmPropgateEvent(cmData,
                                 CmPropgateIsocCisNotifyConnectIndEvent,
                                 CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND,
                                 HCI_SUCCESS,
                                 dmPrim,
                                 NULL);
#endif /* INSTALL_CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND */
            break;

        case DM_ISOC_CIS_CONNECT_CFM:
            CsrBtCmDmIsocCisConnectCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CIS_DISCONNECT_CFM:
            CsrBtCmDmIsocCisDisconnectCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CIS_DISCONNECT_IND:
            CsrBtCmDmIsocCisDisconnectIndHandler(cmData, dmPrim);
            break;

        case DM_ISOC_SETUP_ISO_DATA_PATH_CFM:
            CsrBtCmDmIsocSetupIsoDataPathCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
            CsrBtCmDmIsocRemoveIsoDataPathCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CREATE_BIG_CFM:
            CsrBtCmDmIsocCreateBigCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_CREATE_BIG_TEST_CFM:
            CsrBtCmDmIsocCreateBigTestCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_TERMINATE_BIG_CFM:
            CsrBtCmDmIsocTerminateBigCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_BIG_CREATE_SYNC_CFM:
            CsrBtCmDmIsocBigCreateSyncCfmHandler(cmData, dmPrim);
            break;

        case DM_ISOC_BIG_TERMINATE_SYNC_IND:
            CsrBtCmDmIsocBigTerminateSyncIndHandler(cmData, dmPrim);
            break;

        case DM_ISOC_READ_ISO_LINK_QUALITY_CFM:
            CmDmIsocReadIsoLinkQualityCfmHandler(cmData, dmPrim);
            break;
#endif /* INSTALL_ISOC_SUPPORT */

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmSmPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_SM_INIT_CFM:
            CsrBtCmDmSmInitCfmHandler(cmData);
            break;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        case DM_SM_ACCESS_CFM:
            CsrBtCmDmSmAccessCfmHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
        case DM_SM_ENCRYPT_CFM:
            CsrBtCmDmSmEncryptCfmHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_SC_ENCRYPTION */

        case DM_SM_AUTHORISE_IND:
            CsrBtCmDmAuthoriseIndHandler(cmData);
            break;

        case DM_SM_PIN_REQUEST_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_PIN_REQUEST_IND);
            break;

        case DM_SM_ACCESS_IND:
            CsrBtCmDmSmAccessIndHandler(cmData);
            break;

        case DM_SM_IO_CAPABILITY_RESPONSE_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_IO_CAPABILITY_RESPONSE_IND);
            break;

        case DM_SM_IO_CAPABILITY_REQUEST_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND);
            break;

        case DM_SM_USER_CONFIRMATION_REQUEST_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND);
            break;

        case DM_SM_USER_PASSKEY_REQUEST_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND);
            break;

        case DM_SM_USER_PASSKEY_NOTIFICATION_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_USER_PASSKEY_NOTIFICATION_IND);
            break;

        case DM_SM_KEYPRESS_NOTIFICATION_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_KEYPRESS_NOTIFICATION_IND);
            break;

        case DM_SM_SIMPLE_PAIRING_COMPLETE_IND:
            CsrBtCmDmSmSimplePairingCompleteHandler(cmData);
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND);
            break;

        case DM_SM_BONDING_CFM:
            CsrBtCmDmSmBondingScStateHandler(cmData);
            break;

        case DM_SM_ADD_DEVICE_CFM:
            CsrBtCmDmSmAddDeviceCfmHandler(cmData);
            break;

        case DM_SM_REMOVE_DEVICE_CFM:
            CsrBtCmDmSmRemoveDeviceCfmHandler(cmData);
            break;

        case DM_SM_AUTHENTICATE_CFM:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_AUTHENTICATE_CFM);
            CsrBtCmDmLocalQueueHandler();
            break;

        case DM_SM_ENCRYPTION_CHANGE_IND:
            CsrBtCmDmSmEncryptionChangeHandler(cmData);
            break;

        case DM_SM_GENERATE_LOCAL_KEY_IND:      /* Fall through */
        case DM_SM_REGISTER_CFM:                /* Fall through */
        case DM_SM_REGISTER_OUTGOING_CFM:       /* Fall through */
        case DM_SM_UNREGISTER_CFM:              /* Fall through */
        case DM_SM_UNREGISTER_OUTGOING_CFM:
            /* Ignore and consume these. Then exit completely. */
            break;

        case DM_SM_KEYS_IND:
            CsrBtCmDmSmKeysIndHandler(cmData);
            break;

        case DM_SM_SECURITY_CFM:
            CsrBtCmDmSmSecurityCfmHandler(cmData);
            break;

        case DM_SM_KEY_REQUEST_IND:
            CsrBtDmSmKeyRequestIndHandler(cmData);
            break;

        case DM_SM_SECURITY_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_SECURITY_IND);
            break;

        case DM_SM_CSRK_COUNTER_CHANGE_IND:
#if defined(CSR_BT_LE_SIGNING_ENABLE) && defined(EXCLUDE_CSR_BT_SC_MODULE)
            CmDmSmCsrkCounterChangeIndHandler(cmData);
#endif
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_CSRK_COUNTER_CHANGE_IND);
            break;

#ifdef CSR_BT_INSTALL_CM_OOB
        case DM_SM_READ_LOCAL_OOB_DATA_CFM:
        {
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_CFM);
#if defined( CSR_TARGET_PRODUCT_VM ) || defined( CSR_TARGET_PRODUCT_WEARABLE ) 
            {
                DM_SM_READ_LOCAL_OOB_DATA_CFM_T *cfm = (DM_SM_READ_LOCAL_OOB_DATA_CFM_T*) (cmData->recvMsgP);
                cfm->oob_hash_c = NULL;
                cfm->oob_rand_r = NULL;
            }
#endif
            CsrBtCmDmLocalQueueHandler();
        }
        break;
#endif

        case DM_SM_LOCAL_KEY_DELETED_IND:
            CsrBtCmScMessagePut(cmData, CSR_BT_CM_SM_LOCAL_KEY_DELETED_IND);
            break;

        case DM_SM_WRITE_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM:
            CsrBtCmDmWriteAuthPayloadTimeoutCompleteHandler(cmData);
            break;

#ifdef CSR_BT_LE_ENABLE
        case DM_SM_AUTO_CONFIGURE_LOCAL_ADDRESS_CFM:
            CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
            break;

        case DM_SM_READ_RANDOM_ADDRESS_CFM:
            CsrBtCmLeReadRandomAddressCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
        case DM_SM_READ_LOCAL_IRK_CFM:
            CsrBtCmLeReadLocalIrkCompleteHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

#ifndef EXCLUDE_DM_SM_SIRK_OPERATION_REQ
        case DM_SM_SIRK_OPERATION_CFM:
            CsrBtCmLeSirkOperationCompleteHandler(cmData);
            break;
#endif

#endif /* CSR_BT_LE_ENABLE */

#ifdef INSTALL_CM_SM_CONFIG
        case DM_SM_CONFIG_CFM:
            CmSmConfigCompleteHandler(cmData);
            break;
#endif /* INSTALL_CM_SM_CONFIG */

        case DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND:
            CsrBtCmScMessagePut(cmData, CM_SM_GENERATE_CROSS_TRANS_KEY_IND);
            break;

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

#ifdef CSR_BT_INSTALL_CM_ADV_EXT
static void csrBtCmDmAdvExtPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
        case DM_ULP_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
            CsrBtCmDmExtAdvRegisterAppAdvSetCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
            CsrBtCmDmExtAdvUnregisterAppAdvSetCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_SET_PARAMS_CFM:
            CsrBtCmDmExtAdvSetParamsCfmHandler(cmData, dmPrim);
            break;

#ifdef INSTALL_CM_EXT_ADV_SET_PARAM_V2
		case DM_ULP_EXT_ADV_SET_PARAMS_V2_CFM:
			CmDmExtAdvSetParamsV2CfmHandler(cmData, dmPrim);
			break;
#endif

        case DM_ULP_EXT_ADV_ENABLE_CFM:
            CsrBtCmDmExtAdvEnableCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM:
            CsrBtCmDmExtAdvReadMaxAdvDataLenCfmHandler(cmData, dmPrim);
            break;
        
        case DM_ULP_EXT_ADV_TERMINATED_IND:
            CsrBtCmDmExtAdvTerminatedIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_SET_RANDOM_ADDR_CFM:
            CsrBtCmDmExtAdvSetRandomAddrCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_SETS_INFO_CFM:
            CsrBtCmDmExtAdvSetsInfoCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_ADV_MULTI_ENABLE_CFM:
            CsrBtCmDmExtAdvMultiEnableCfmHandler(cmData, dmPrim);
            break;
            
        case DM_ULP_EXT_ADV_GET_ADDR_CFM:
            CmDmExtAdvGetAddressCfmHandler(cmData, dmPrim);
            break;

#endif /* CSR_BT_INSTALL_EXTENDED_ADVERTISING */

#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
        case DM_ULP_EXT_SCAN_GET_GLOBAL_PARAMS_CFM:
            CsrBtCmDmExtScanGetGlobalParamsCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_SET_GLOBAL_PARAMS_CFM:
            CsrBtCmDmExtScanSetGlobalParamsCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_REGISTER_SCANNER_CFM:
            CsrBtCmDmExtScanRegisterScannerCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_UNREGISTER_SCANNER_CFM:
            CsrBtCmDmExtScanUnregisterScannerCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_CONFIGURE_SCANNER_CFM:
            CsrBtCmDmExtScanConfigureScannerCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_ENABLE_SCANNERS_CFM:
            CsrBtCmDmExtScanEnableScannersCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM:
            CsrBtCmDmExtScanGetCtrlScanInfoCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_CTRL_SCAN_INFO_IND:
            CsrBtCmDmExtScanCtrlScanInfoIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND:
            CsrBtCmDmExtScanFilteredAdvReportIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_EXT_SCAN_DURATION_EXPIRED_IND:
            CsrBtCmDmExtScanDurationExpiredIndHandler(cmData, dmPrim);
            break;

#ifdef INSTALL_CM_EXT_SET_CONN_PARAM
        case DM_ULP_EXT_SET_CONNECTION_PARAMS_CFM:
            CmDmExtSetConnParamsCfmHandler(cmData, dmPrim);
            break;
#endif /* INSTALL_CM_EXT_SET_CONN_PARAM */

#ifdef INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND
        case DM_ULP_EXT_SCAN_TIMEOUT_IND:
            CsrBtCmPropgateEvent(cmData,
                                 CmPropgateExtScanTimeoutIndEvent,
                                 CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND,
                                 HCI_SUCCESS,
                                 dmPrim,
                                 NULL);
            break;
#endif /* INSTALL_CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND */

#endif /* CSR_BT_INSTALL_EXTENDED_SCANNING */

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING
        case DM_ULP_PERIODIC_ADV_SET_PARAMS_CFM:
            CsrBtCmDmPeriodicAdvSetParamsCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM:
            CsrBtCmDmPeriodicAdvReadMaxAdvDataLenCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_ADV_START_CFM:
            CsrBtCmDmPeriodicAdvStartCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_ADV_STOP_CFM:
            CsrBtCmDmPeriodicAdvStopCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_ADV_SET_TRANSFER_CFM:
            CsrBtCmDmPeriodicAdvSetTransferCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_ADV_ENABLE_CFM:
            CsrBtCmDmPeriodicAdvEnableCfmHandler(cmData, dmPrim);
            break;

#endif /* CSR_BT_INSTALL_PERIODIC_ADVERTISING */

#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
        case DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_CFM:
            CsrBtCmDmPeriodicScanStartFindTrainsCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM:
            CsrBtCmDmPeriodicScanStopFindTrainsCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM:
            CsrBtCmDmPeriodicScanSyncToTrainCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM:
            CsrBtCmDmPeriodicScanSyncToTrainCancelCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM:
            CsrBtCmDmPeriodicScanSyncAdvReportEnableCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TERMINATE_CFM:
            CsrBtCmDmPeriodicScanSyncTerminateCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            CsrBtCmDmPeriodicScanSyncAdvReportIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_LOST_IND:
            CsrBtCmDmPeriodicScanSyncLostIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_CFM:
            CsrBtCmDmPeriodicScanSyncTransferCfmHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_IND:
            CsrBtCmDmPeriodicScanSyncTransferIndHandler(cmData, dmPrim);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM:
            CsrBtCmDmPeriodicScanSyncTransferParamsCfmHandler(cmData, dmPrim);
            break;
#endif /* CSR_BT_INSTALL_PERIODIC_SCANNING */

        case DM_ULP_GET_ADV_SCAN_CAPABILITIES_CFM:
        {
            CsrBtCmDmGetAdvScanCapabilitiesCfmHandler(cmData, dmPrim);
            break;
        }

        default:
            CsrBtCmGeneralException(DM_PRIM,
                            dmPrim->type,
                            (CsrUint16) cmData->globalState,
                            "");
            break;
    }
}
#endif /* CSR_BT_INSTALL_CM_ADV_EXT */

static void csrBtCmDmHciLcPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_HCI_INQUIRY_CFM:
            CsrBtCmDmHciInquiryCompleteHandler(cmData);
            break;

        case DM_HCI_INQUIRY_CANCEL_CFM:
            CsrBtCmDmHciInquiryCancelCompleteHandler(cmData);
            break;

        case DM_HCI_CREATE_CONNECTION_CANCEL_CFM:
            CsrBtCmHciMessagePut(cmData, CSR_BT_CM_HCI_CREATE_CONNECTION_CANCEL_CFM);
            break;

        case DM_HCI_REMOTE_NAME_REQ_CANCEL_CFM:
            /* This is not handled. Wait for DM_HCI_REMOTE_NAME_CFM instead */
            break;

        case DM_HCI_READ_REMOTE_VER_INFO_CFM:
            CsrBtCmDmHciReadRemoteVersionCompleteHandler(cmData);
            break;

        case DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM:
            CsrBtCmDmHciReadRemoteFeaturesCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_CACHE_PARAMS
        case DM_HCI_READ_CLOCK_OFFSET_CFM:
            CsrBtCmDmHciReadClockOffsetCompleteHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_CM_CACHE_PARAMS */

        case DM_HCI_READ_REMOTE_EXT_FEATURES_CFM:
            CsrBtCmDmHciReadRemoteExtFeaturesCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_INQUIRY_TRANSMIT_POWER_LEVEL_CFM:
            CsrBtCmDmHciWriteInquiryTransmitPowerLevelCompleteHandler(cmData);
            break;

        case DM_HCI_CHANGE_CONN_LINK_KEY_CFM:
            CmDmChangeConnectionLinkKeyCfmHandler(cmData);
            break;

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmHciLpPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_HCI_WRITE_LINK_POLICY_SETTINGS_CFM:
        {
            if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_LINK_POLICY))
            {
                CsrBtCmDmHciWriteLpSettingsCompleteHandler(cmData);
            }
            break;
        }

        case DM_HCI_ROLE_DISCOVERY_CFM:
            CsrBtCmDmHciRoleDiscoveryCompleteHandler(cmData);
            break;

#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
        case DM_HCI_QOS_SETUP_CFM:
            CsrBtCmDmHciQosSetupCompleteHandler(cmData);
            break;
#endif
        case DM_HCI_SNIFF_SUB_RATE_CFM:
            CsrBtCmDmHciSniffSubRateCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM:
            CsrBtCmDmHciWriteDefaultLinkPolicySettingsCompleteHandler(cmData);
            break;

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmHciBbPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
#ifndef  CSR_BT_EXCLUDE_HCI_READ_LOCAL_NAME
        case DM_HCI_READ_LOCAL_NAME_CFM:
            CsrBtCmDmHciReadLocalNameCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_CHANGE_LOCAL_NAME_CFM:
            CsrBtCmDmHciChangeLocalNameCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_CLASS_OF_DEVICE_CFM:
            CsrBtCmDmHciWriteClassOfDeviceCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_READ_COD
        case DM_HCI_READ_CLASS_OF_DEVICE_CFM:
            CsrBtCmDmHciReadClassOfDeviceCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_WRITE_PAGESCAN_ACTIVITY_CFM:
            CsrBtCmDmHciWritePageScanActivityCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_INQUIRYSCAN_ACTIVITY_CFM:
            CsrBtCmDmHciWriteInquiryScanActivityCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_SCAN_ENABLE_CFM:
            CsrBtCmDmHciWriteScanEnableCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_CFM:
            CsrBtCmDmHciWriteAutoFlushTimeoutCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_READ_SCAN_EANBLE
        case DM_HCI_READ_SCAN_ENABLE_CFM:
            CsrBtCmDmHciReadScanEnableCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_DELETE_STORED_LINK_KEY_CFM:
            CsrBtCmHciMessagePut(cmData, CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM);

            /* In addition to CsrBtCmSmDeleteStoreLinkKeyReqHandler(), 
             * dm_hci_delete_stored_link_key() is also called internally from CM
             * eg. from csrBtCmDbRemoveKeys(). These calls shall not unlock DM Lock
             */
            if (cmData->dmVar.lockMsg == CSR_BT_CM_SM_DELETE_STORE_LINK_KEY_REQ)
            {
                CsrBtCmDmLocalQueueHandler();
            }
            break;

        case DM_HCI_WRITE_VOICE_SETTING_CFM:
            CsrBtCmDmHciWriteVoiceSettingCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_CFM:
            CsrBtCmDmHciWriteLinkSuperVisionTimeoutCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_PAGE_TIMEOUT_CFM:
            CsrBtCmDmHciWritePageToCompleteHandler(cmData);
            break;

        case DM_HCI_SET_EVENT_FILTER_CFM:
            CsrBtCmSetEventFilterCommonCfmHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_READ_TX_POWER_LEVEL
        case DM_HCI_READ_TX_POWER_LEVEL_CFM:
            CsrBtCmDmHciReadTxPowerLevelCompleteHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CM_PRI_IAC
#ifdef CSR_BT_INSTALL_CM_PRI_IAC_READ
        case DM_HCI_READ_CURRENT_IAC_LAP_CFM:
            CsrBtCmDmHciReadIacCompleteHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_CM_PRI_IAC_READ */

        case DM_HCI_WRITE_CURRENT_IAC_LAP_CFM:
            CsrBtCmDmHciWriteIacCompleteHandler(cmData);
            break;
#endif /* CSR_BT_INSTALL_CM_PRI_IAC */

#ifdef CSR_BT_INSTALL_CM_AFH
        case DM_HCI_SET_AFH_CHANNEL_CLASS_CFM:
            CsrBtCmDmHciSetAfhChannelClassCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_WRITE_INQUIRY_SCAN_TYPE_CFM:
            CsrBtCmDmHciWriteInquiryScanTypeCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_INQUIRY_MODE_CFM:
            CsrBtCmDmHciWriteInquiryModeCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_PAGE_SCAN_TYPE_CFM:
            CsrBtCmDmHciWritePageScanTypeCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_AFH
        case DM_HCI_READ_AFH_CHANNEL_CLASS_M_CFM:
            CsrBtCmDmHciReadAfhChannelAssesModeCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_AFH_CHANNEL_CLASS_M_CFM:
            CsrBtCmDmHciWriteAfhChannelAssesModeCompleteHandler(cmData);
            break;
#endif

#if (CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1)
        case DM_HCI_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_CFM:
            CsrBtCmDmHciWriteExtendedInquiryResponseDataCompleteHandler(cmData);
            break;

        case DM_HCI_WRITE_INQUIRY_TRANSMIT_POWER_LEVEL_CFM:
            CsrBtCmDmHciWriteInquiryTransmitPowerLevelCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_REFRESH_ENCRYPTION_KEY_IND:
#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE
            CsrBtCmPropgateEvent(cmData,
                                 CsrBtCmPropagateEncryptionRefreshIndStatusEvents,
                                 CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE,
                                 HCI_SUCCESS,
                                 dmPrim,
                                 NULL);
#endif
            CsrBtCmHciMessagePut(cmData, CSR_BT_CM_HCI_REFRESH_ENCRYPTION_KEY_IND);
            break;

#ifdef INSTALL_CM_READ_INQUIRY_MODE
        case DM_HCI_READ_INQUIRY_MODE_CFM:
            CmDmReadInquiryModeCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_READ_INQUIRY_MODE */

#ifdef INSTALL_CM_READ_INQUIRY_TX
        case DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_CFM:
            CmDmReadInquiryTxCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_READ_INQUIRY_TX */

#ifdef INSTALL_CM_READ_APT
        case DM_HCI_READ_AUTHENTICATED_PAYLOAD_TIMEOUT_CFM:
            CmDmReadAuthPayloadTimeoutCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_READ_APT */

#ifdef INSTALL_CM_READ_EIR_DATA
        case DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM:
            CmDmReadEIRDataCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_READ_EIR_DATA */

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmHciInfPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_HCI_READ_BD_ADDR_CFM:
            CsrBtCmDmHciReadBdAddrCompleteHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_READ_LOCAL_EXT_FEATURES
        case DM_HCI_READ_LOCAL_EXT_FEATURES_CFM:
            CsrBtCmDmHciReadLocalExtFeaturesCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_READ_LOCAL_SUPP_FEATURES_CFM:
            CsrBtCmDmHciReadSuppFeaturesCfmHandler(cmData);
            break;

        case DM_HCI_READ_LOCAL_VER_INFO_CFM:
            CsrBtCmDmHciReadLocalVersionCompleteHandler(cmData);
            break;

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmHciStatusPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
#ifdef CSR_BT_INSTALL_CM_READ_RSSI
        case DM_HCI_READ_RSSI_CFM:
            CsrBtCmDmHciReadRssiCompleteHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CM_GET_LINK_QUALITY
        case DM_HCI_GET_LINK_QUALITY_CFM:
            CsrBtCmDmHciGetLinkQualityCompleteHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CM_AFH
        case DM_HCI_READ_AFH_CHANNEL_MAP_CFM:
            CsrBtCmReadAfhChannelMapCfmHandler(cmData);
            break;
#endif

#ifdef CSR_BT_INSTALL_CM_READ_CLOCK
        case DM_HCI_READ_CLOCK_CFM:
            CsrBtCmDmHciReadClockCompleteHandler(cmData);
            break;
#endif

        case DM_HCI_READ_ENCRYPTION_KEY_SIZE_CFM:
            CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
            break;

#ifdef CSR_BT_INSTALL_CM_READ_FAILED_CONTACT_COUNTER
        case DM_HCI_READ_FAILED_CONTACT_COUNT_CFM:
            CsrBtCmDmHciReadFailedContactCounterCompleteHandler(cmData);
            break;
#endif

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}

#ifdef CSR_BT_INSTALL_CM_DUT_MODE
static void csrBtCmDmHciTestPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    switch (dmPrim->type)
    {
        case DM_HCI_ENABLE_DUT_MODE_CFM:
            CsrBtCmDmHciDeviceUnderTestCompleteHandler(cmData);
            break;

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;
    }
}
#endif /* CSR_BT_INSTALL_CM_DUT_MODE */

/* Remaining HCI events use the standard grouping */
static void csrBtCmDmHciGroupedPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;
    dm_prim_t dmEventType = (CsrUint16) (dmPrim->type & DM_OGF_MASK);

    switch (dmEventType)
    {
        case DM_LC_PRIM | DM_HCI_WITH_HANDLE:       /* Fall through */
        case DM_LC_PRIM:
            csrBtCmDmHciLcPrimHandler(cmData);
            break;

        case DM_LP_PRIM | DM_HCI_WITH_HANDLE:       /* Fall through */
        case DM_LP_PRIM:
            csrBtCmDmHciLpPrimHandler(cmData);
            break;

        case DM_BB_PRIM | DM_HCI_WITH_HANDLE:       /* Fall through */
        case DM_BB_PRIM:
            csrBtCmDmHciBbPrimHandler(cmData);
            break;

        case DM_INF_PRIM | DM_HCI_WITH_HANDLE:      /* Fall through */
        case DM_INF_PRIM:
            csrBtCmDmHciInfPrimHandler(cmData);
            break;

        case DM_STATUS_PRIM | DM_HCI_WITH_HANDLE:   /* Fall through */
        case DM_STATUS_PRIM:
            csrBtCmDmHciStatusPrimHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_DUT_MODE
        case DM_TEST_PRIM | DM_HCI_WITH_HANDLE:     /* Fall through */
        case DM_TEST_PRIM:
            csrBtCmDmHciTestPrimHandler(cmData);
            break;
#endif

#ifdef CSR_BT_LE_ENABLE
        case DM_ULP_PRIM | DM_HCI_WITH_HANDLE:      /* Fall through */
        case DM_ULP_PRIM:
            switch(dmPrim->type)
            {
#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
                case DM_HCI_ULP_EXT_ADV_SET_DATA_CFM:
                    CsrBtCmDmExtAdvSetDataCfmHandler(cmData, dmPrim);
                    break;

                case DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_CFM:
                    CsrBtCmDmExtAdvSetScanRespDataCfmHandler(cmData, dmPrim);
                    break;
#endif /* CSR_BT_INSTALL_EXTENDED_ADVERTISING */

#ifdef CSR_BT_INSTALL_PERIODIC_ADVERTISING
                case DM_HCI_ULP_PERIODIC_ADV_SET_DATA_CFM:
                    CsrBtCmDmPeriodicAdvSetDataCfmHandler(cmData, dmPrim);
                    break;
#endif /* CSR_BT_INSTALL_PERIODIC_ADVERTISING */

                case DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM:
                    if (cmData->globalState == CSR_BT_CM_STATE_NOT_READY)
                    {
                        DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM_T *cfm =
                                    (DM_HCI_ULP_READ_LOCAL_SUPPORTED_FEATURES_CFM_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
                        /* Keep the result stored into CM instance LE record */
                        if (CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(cfm ->feature_set,
                                                              CSR_BT_LE_FEATURE_LL_PRIVACY))
                        {
                            cmData->leVar.llFeaturePrivacy = TRUE;
                        }
                        else
#endif
                        {
                            cmData->leVar.llFeaturePrivacy = FALSE;
                        }

                        /* CFM received during CM initialization. It means CM itself called DM lib.
                         * Just handle it and continue the initialization procedure */
                        CmInitSequenceHandler(cmData,
                                              CM_INIT_SEQ_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM, 
                                              cfm->status,
                                              CSR_BT_SUPPLIER_HCI);
                    }
                    else
                    {
                        /* CFM received after CM initialization. It means REQ was initiated
                         * by different module through CM provider handler. 
                         * Just call the registered handler's callback */
                        CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
                    }
                    break;

                default:
                    CsrBtCmCallbackDispatchSimple(cmData, dmPrim);
                    break;
            }
            break;
#endif

        default:
            CsrBtCmGeneralException(DM_PRIM,
                                    (CsrUint16)dmEventType,
                                    cmData->globalState,
                                    "");
            break;
    }
}

static void csrBtCmDmHciPrimHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T*) cmData->recvMsgP;

    /* These are special HCI events that doesn't use the normal grouping system.
     * Grouped messages are handled in default case */
    switch (dmPrim->type)
    {
        case DM_HCI_INQUIRY_RESULT_IND:
            CsrBtCmDmHciInquiryResultHandler(cmData);
            break;

        case DM_HCI_MODE_CHANGE_EVENT_IND:
            CsrBtCmDmHciModeChangeEventHandler(cmData);
            break;

        case DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND:
            CsrBtCmDmHciInquiryResultWithRssiHandler(cmData);
            break;

#if (CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1)
        case DM_HCI_SNIFF_SUB_RATING_IND:
            CsrBtCmDmHciSniffSubRatingIndHandler(cmData);
            break;

        case DM_HCI_EXTENDED_INQUIRY_RESULT_IND:
            CsrBtCmDmHciExtendedInquiryResultIndHandler(cmData);
            break;
#endif /* CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 */

        case DM_HCI_LINK_SUPERV_TIMEOUT_IND:
            CsrBtCmDmHciLinkSupervisionTimeoutIndHandler(cmData);
            break;

        case DM_HCI_REM_HOST_SUPPORTED_FEATURES_IND:
            /* Ignore this one for now */
            break;

        case DM_HCI_CONN_PACKET_TYPE_CHANGED_IND: /* Fall through */
            CsrBtCmGeneralException(DM_PRIM,
                                    dmPrim->type,
                                    cmData->globalState,
                                    "");
            break;

#ifdef CSR_BT_LE_ENABLE
        case DM_HCI_ULP_ADVERTISING_REPORT_IND:
            CsrBtCmLeReportIndHandler(cmData,
                                      (DM_HCI_ULP_ADVERTISING_REPORT_IND_T *) dmPrim);
            break;

        case DM_HCI_ULP_CONNECTION_UPDATE_COMPLETE_IND:
            CsrBtCmLeConnectionUpdateCmpIndHandler(cmData,
                                                   (DM_HCI_ULP_CONNECTION_UPDATE_COMPLETE_IND_T *) dmPrim);
            break;

#ifdef CSR_BT_ISOC_ENABLE
        case DM_HCI_ULP_BIGINFO_ADV_REPORT_IND:
            CsrBtCmDmBleBigInfoAdvReportIndHandler(cmData,
                                                   (DM_HCI_ULP_BIGINFO_ADV_REPORT_IND_T *) dmPrim);
            break;
#endif /* CSR_BT_ISOC_ENABLE */

        case DM_HCI_ULP_CHANNEL_SELECTION_ALGORITHM_IND:
            /* Just ignore */
            break;
#endif /* CSR_BT_LE_ENABLE */

        case DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND:
            CmDmHciAuthPayloadTimeoutExpiredIndHandler(cmData);
            break;

#ifndef CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT
        case DM_HCI_READ_CONN_ACCEPT_TIMEOUT_CFM:
            CmDmReadConnAcceptTimeoutCfmHandler(cmData);
            break;
#endif /* !CSR_BT_EXCLUDE_HCI_READ_CONN_ACCEPT_TIMEOUT */

#ifndef CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT
        case DM_HCI_WRITE_CONN_ACCEPT_TIMEOUT_CFM:
            CmDmWriteConnAcceptTimeoutCfmHandler(cmData);
            break;
#endif /* !CSR_BT_EXCLUDE_HCI_WRITE_CONN_ACCEPT_TIMEOUT */

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
        case DM_HCI_CONFIGURE_DATA_PATH_CFM:
            CmDmConfigureDataPathCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
        case DM_HCI_ULP_READ_CHANNEL_MAP_CFM:
            CmDmLeReadChannelMapCfmHandler(cmData);
            break;
#endif /* INSTALL_CM_DM_LE_READ_CHANNEL_MAP */

        default: /* Must be one of the grouped HCI message */
            csrBtCmDmHciGroupedPrimHandler(cmData);
            break;
    }
}

/*************************************************************************************
  CsrBtCmDmArrivalHandler:
  Handles incoming events from the DM layer
************************************************************************************/
void CsrBtCmDmArrivalHandler(cmInstanceData_t *cmData)
{
    DM_UPRIM_T *dmPrim = (DM_UPRIM_T *) cmData->recvMsgP;
    dm_prim_t dmEventType = (CsrUint16) (dmPrim->type & DM_GROUP_MASK);

#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmArrivalHandler cat:0x%x MESSAGE:dm_prim_tag:0x%x", dmEventType, dmPrim->type));
#else
    CSR_LOG_TEXT_INFO((CsrBtCmLto, 0, "CsrBtCmDmArrivalHandler cat:0x%x Msg:0x%x",
                dmEventType, dmPrim->type));
#endif

    switch (dmEventType)
    {
        case DM_PRIV_UP_PRIM:
            csrBtCmDmPrivatePrimHandler(cmData);
            break;

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
        case DM_SYNC_UP_PRIM:
            csrBtCmDmSyncPrimHandler(cmData);
            break;
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

        case DM_SM_UP_PRIM:
            csrBtCmDmSmPrimHandler(cmData);
            break;

#ifdef CSR_BT_INSTALL_CM_ADV_EXT
        case DM_ADV_EXT_UP_PRIM:
            csrBtCmDmAdvExtPrimHandler(cmData);
            break;
#endif

        default: /* This must be an HCI event then... */
            csrBtCmDmHciPrimHandler(cmData);
            break;
    }

    dm_free_upstream_primitive(cmData->recvMsgP);
    cmData->recvMsgP = NULL;
}
