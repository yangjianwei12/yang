/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_dm_sc_ssp_handler.h"
#include "csr_bt_cm_le.h"


static const SignalHandlerType dmProviderHandler[CSR_BT_CM_DM_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtCmSetLocalNameReqHandler,                        /* CSR_BT_CM_SET_LOCAL_NAME_REQ */
    CsrBtCmReadRemoteNameReqHandler,                      /* CSR_BT_CM_READ_REMOTE_NAME_REQ */
    CsrBtCmDmScoConnectReqHandler,                        /* CSR_BT_CM_SCO_CONNECT_REQ */
    CsrBtCmDmScoRenegotiateReqHandler,                    /* CSR_BT_CM_SCO_RENEGOTIATE_REQ */
    CsrBtCmDmScoDisconnectReqHandler,                     /* CSR_BT_CM_SCO_DISCONNECT_REQ */
    CsrBtCmSmDeleteStoreLinkKeyReqHandler,                /* CSR_BT_CM_SM_DELETE_STORE_LINK_KEY_REQ */
    CsrBtCmSmRemoveDeviceReqHandler,                      /* CSR_BT_CM_SM_REMOVE_DEVICE_REQ */
    CsrBtCmSmSetSecModeReqHandler,                        /* CSR_BT_CM_SM_SET_SEC_MODE_REQ */
    CsrBtCmDatabaseReqHandler,                            /* CSR_BT_CM_DATABASE_REQ */
    CsrBtCmSmAuthenticateReqHandler,                      /* CSR_BT_CM_SM_AUTHENTICATE_REQ */
    CsrBtCmSmEncryptionReqHandler,                        /* CSR_BT_CM_SM_ENCRYPTION_REQ */
    CsrBtCmAclCloseReqHandler,                            /* CSR_BT_CM_ACL_CLOSE_REQ */
    CsrBtCmSmSetDefaultSecLevelReqHandler,                /* CSR_BT_CM_SM_SET_DEFAULT_SEC_LEVEL_REQ */
    CsrBtCmSmUnRegisterReqHandler,                        /* CSR_BT_CM_SM_UNREGISTER_REQ */
    CsrBtCmSmRegisterReqHandler,                          /* CSR_BT_CM_SM_REGISTER_REQ */
    CsrBtCmDmRoleDiscoveryReqHandler,                     /* CSR_BT_CM_ROLE_DISCOVERY_REQ */
    CsrBtCmReadBdAddrReqHandler,                          /* CSR_BT_CM_READ_LOCAL_BD_ADDR_REQ */
    CsrBtCmReadLocalNameReqHandler,                       /* CSR_BT_CM_READ_LOCAL_NAME_REQ */
    CsrBtCmDeviceUnderTestReqHandler,                     /* CSR_BT_CM_ENABLE_DUT_MODE_REQ */
    CsrBtCmWriteScanEnableReqHandler,                     /* CSR_BT_CM_WRITE_SCAN_ENABLE_REQ */
    CsrBtCmReadScanEnableReqHandler,                      /* CSR_BT_CM_READ_SCAN_ENABLE_REQ */
    CsrBtCmDmWritePageToReqHandler,                       /* CSR_BT_CM_WRITE_PAGE_TO_REQ */
    CsrBtCmReadTxPowerLevelReqHandler,                    /* CSR_BT_CM_READ_TX_POWER_LEVEL_REQ */
    CsrBtCmGetLinkQualityReqHandler,                      /* CSR_BT_CM_GET_LINK_QUALITY_REQ */
    CsrBtCmReadRssiReqHandler,                            /* CSR_BT_CM_READ_RSSI_REQ */
    CsrBtCmWriteCodReqHandler,                            /* CSR_BT_CM_WRITE_COD_REQ */
    CsrBtCmReadCodReqHandler,                             /* CSR_BT_CM_READ_COD_REQ */
    CsrBtCmReadRemoteExtFeaturesReqHandler,               /* CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ */
    CsrBtCmSetAfhChannelClassReqHandler,                  /* CSR_BT_CM_SET_AFH_CHANNEL_CLASS_REQ */
    CsrBtCmReadAfhChannelAssesModeReqHandler,             /* CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ */
    CsrBtCmWriteAfhChannelAssesModeReqHandler,            /* CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_REQ */
    CsrBtCmReadLocalExtFeaturesReqHandler,                /* CSR_BT_CM_READ_LOCAL_EXT_FEATURES_REQ */
    CsrBtCmReadAfhChannelMapReqHandler,                   /* CSR_BT_CM_READ_AFH_CHANNEL_MAP_REQ */
    CsrBtCmReadClockReqHandler,                           /* CSR_BT_CM_READ_CLOCK_REQ */
    CsrBtCmReadLocalVersionReqHandler,                    /* CSR_BT_CM_READ_LOCAL_VERSION_REQ */
    CsrBtCmSetEventFilterBdaddrReqHandler,                /* CSR_BT_CM_SET_EVENT_FILTER_BDADDR_REQ */
    CsrBtCmSetEventFilterCodReqHandler,                   /* CSR_BT_CM_SET_EVENT_FILTER_COD_REQ */
    CsrBtCmClearEventFilterReqHandler,                    /* CSR_BT_CM_CLEAR_EVENT_FILTER_REQ */
    CsrBtCmReadIacReqHandler,                             /* CSR_BT_CM_READ_IAC_REQ */
    CsrBtCmWriteIacReqHandler,                            /* CSR_BT_CM_WRITE_IAC_REQ */
    CsrBtCmDmWriteCacheParamsReqHandler,                  /* CSR_BT_CM_DM_WRITE_CACHE_PARAMS_REQ */
    CsrBtCmDmUpdateAndClearCachedParamReqHandler,         /* CSR_BT_CM_DM_UPDATE_AND_CLEAR_CACHED_PARAM_REQ */
    CsrBtCmReadEncryptionStatusReqHandler,                /* CSR_BT_CM_READ_ENCRYPTION_STATUS_REQ */
    CsrBtCmWritePageScanSettingsReqHandler,               /* CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_REQ */
    CsrBtCmWritePageScanTypeReqHandler,                   /* CSR_BT_CM_WRITE_PAGESCAN_TYPE_REQ */
    CsrBtCmWriteInquiryScanSettingsReqHandler,            /* CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_REQ */
    CsrBtCmWriteInquiryScanTypeReqHandler,                /* CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_REQ */
    CsrBtCmDmModeSettingsReqHandler,                      /* CSR_BT_CM_DM_MODE_SETTINGS_REQ */
    CsrBtCmDmL2caModeSettingsReqHandler,                  /* CSR_BT_CM_DM_L2CA_MODE_SETTINGS_REQ */
    CsrBtCmDmBnepModeSettingsReqHandler,                  /* CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ */
    CsrBtCmDmCheckSsrReqHandler,                          /* CSR_BT_CM_DM_CHECK_SSR_REQ */
    CsrBtCmSmBondingReqHandler,                           /* CSR_BT_CM_SM_BONDING_REQ */
    CsrBtCmSmSecModeConfigReqHandler,                     /* CSR_BT_CM_SM_SEC_MODE_CONFIG_REQ */
    CsrBtCmSmReadLocalOobDataReqHandler,                  /* CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_REQ */
    NULL,                                                 /* CSR_BT_CM_SM_READ_DEVICE_REQ */
    NULL,                                                 /* CSR_BT_CM_EN_ENABLE_ENHANCEMENTS_REQ */
    CsrBtCmEirUpdateManufacturerReqHandler,               /* CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ */
    CsrBtCmDmWriteAutoFlushTimeoutReqHandler,             /* CSR_BT_CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_REQ */
    CsrBtCmReadFailedContactCounterReqHandler,            /* CSR_BT_CM_READ_FAILED_CONTACT_COUNTER_REQ */
    CsrBtCmReadRemoteFeaturesReqHandler,                  /* CSR_BT_CM_READ_REMOTE_FEATURES_REQ */
    CsrBtCmWriteVoiceSettingsReqHandler,                  /* CSR_BT_CM_WRITE_VOICE_SETTINGS_REQ */
    CsrBtCmSmAccessReqHandler,                            /* CSR_BT_CM_SM_ACCESS_REQ */
    CsrBtCmAlwaysMasterDevicesReqHandler,                 /* CSR_BT_CM_ALWAYS_MASTER_DEVICES_REQ */  
    CsrBtCmDeviceUnderTestDisableReqHandler,              /* CSR_BT_CM_DISABLE_DUT_MODE_REQ */
    CsrBtCmSmLeSecurityReqHandler,                        /* CSR_BT_CM_SM_LE_SECURITY_REQ */
    CsrBtCmSmSetEncryptionKeySizeReqHandler,              /* CSR_BT_CM_SM_SET_ENCRYPTION_KEY_SIZE_REQ */
    CsrBtCmIncomingScoReqHandler,                         /* CSR_BT_CM_INCOMING_SCO_REQ */
    CsrBtCmDmHciQosSetupReqHandler,                      /* CSR_BT_CM_DM_HCI_QOS_SETUP_REQ */
    CsrBtCmDmWriteAuthPayloadTimeoutReqHandler,          /* CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_REQ */
    CsrBtCmLeReadRandomAddressReqHandler,                /* CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ */
    CsrBtCmDmPowerSettingsReqHandler,                   /* CSR_BT_CM_DM_POWER_SETTINGS_REQ */
    CsrBtCmDmIsocRegisterReqHandler,                   /* CSR_BT_CM_ISOC_REGISTER_REQ */
    CsrBtCmDmIsocConfigureCigReqHandler,               /* CSR_BT_CM_ISOC_CONFIGURE_CIG_REQ */
    CsrBtCmDmIsocRemoveCigReqHandler,                  /* CSR_BT_CM_ISOC_REMOVE_CIG_REQ */
    CsrBtCmDmIsocCisConnectReqHandler,                 /* CSR_BT_CM_ISOC_CIS_CONNECT_REQ */
    CsrBtCmDmIsocCisConnectRspHandler,                 /* CSR_BT_CM_ISOC_CIS_CONNECT_RSP */
    CsrBtCmDmIsocCisDisconnectReqHandler,              /* CSR_BT_CM_ISOC_CIS_DISCONNECT_REQ */
    CsrBtCmDmIsocSetupIsoDataPathReqHandler,           /* CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_REQ */
    CsrBtCmDmIsocRemoveIsoDataPathReqHandler,          /* CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_REQ */
    CsrBtCmDmIsocCreateBigReqHandler,                  /* CSR_BT_CM_ISOC_CREATE_BIG_REQ */
    CsrBtCmDmIsocTerminateBigReqHandler,               /* CSR_BT_CM_ISOC_TERMINATE_BIG_REQ */
    CsrBtCmDmIsocBigCreateSyncReqHandler,              /* CSR_BT_CM_ISOC_BIG_CREATE_SYNC_REQ */
    CsrBtCmDmIsocBigTerminateSyncReqHandler,           /* CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_REQ */
    CsrBtCmDmExtAdvRegisterAppAdvSetReqHandler,        /* CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ */
    CsrBtCmDmExtAdvUnregisterAppAdvSetReqHandler,      /* CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ */
    CsrBtCmDmExtAdvSetParamsReqHandler,                /* CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ */
    CsrBtCmDmExtAdvSetDataReqHandler,                  /* CSR_BT_CM_EXT_ADV_SET_DATA_REQ */
    CsrBtCmDmExtAdvSetScanRespDataReqHandler,          /* CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ */
    CsrBtCmDmExtAdvEnableReqHandler,                   /* CSR_BT_CM_EXT_ADV_ENABLE_REQ */
    CsrBtCmDmExtAdvReadMaxAdvDataLenReqHandler,        /* CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    CsrBtCmDmExtScanGetGlobalParamsReqHandler,         /* CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ */
    CsrBtCmDmExtScanSetGlobalParamsReqHandler,         /* CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ */
    CsrBtCmDmExtScanRegisterScannerReqHandler,         /* CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ */
    CsrBtCmDmExtScanUnregisterScannerReqHandler,       /* CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_REQ */
    CsrBtCmDmExtScanConfigureScannerReqHandler,        /* CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ */
    CsrBtCmDmExtScanEnableScannersReqHandler,          /* CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ */
    CsrBtCmDmExtScanGetCtrlScanInfoReqHandler,         /* CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ */
    CsrBtCmDmPeriodicAdvSetParamsReqHandler,           /* CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_REQ */
    CsrBtCmDmPeriodicAdvSetDataReqHandler,             /* CSR_BT_CM_PERIODIC_ADV_SET_DATA_REQ */
    CsrBtCmDmPeriodicAdvReadMaxAdvDataLenReqHandler,   /* CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    CsrBtCmDmPeriodicAdvStartReqHandler,               /* CSR_BT_CM_PERIODIC_ADV_START_REQ */
    CsrBtCmDmPeriodicAdvStopReqHandler,                /* CSR_BT_CM_PERIODIC_ADV_STOP_REQ */
    CsrBtCmDmPeriodicAdvSetTransferReqHandler,         /* CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_REQ */
    CsrBtCmDmPeriodicScanStartFindTrainsReqHandler,    /* CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_REQ */
    CsrBtCmDmPeriodicScanStopFindTrainsReqHandler,     /* CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ */
    CsrBtCmDmPeriodicScanSyncToTrainReqHandler,        /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ */
    CsrBtCmDmPeriodicScanSyncToTrainCancelReqHandler,  /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_REQ */
    CsrBtCmDmPeriodicScanSyncAdvReportEnableReqHandler, /* CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ */
    CsrBtCmDmPeriodicScanSyncTerminateReqHandler,      /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_REQ */
    CsrBtCmDmPeriodicScanSyncLostRspHandler,           /* CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_RSP */
    CsrBtCmDmPeriodicScanSyncTransferReqHandler,       /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_REQ */
    CsrBtCmDmPeriodicScanSyncTransferParamsReqHandler, /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ */
    CsrBtCmDmSetLinkBehaviorReqHandler,                 /* CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_REQ */
    CsrBtCmDmExtAdvSetRandomAddrReqHandler,             /* CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ */
    CsrBtCmLeSirkOperationReqHandler,                   /* CSR_BT_CM_LE_SIRK_OPERATION_REQ */
    CsrBtCmDmGetAdvScanCapabilitiesReqHandler,          /* CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ */
    CsrBtCmDmExtAdvSetsInfoReqHandler,                  /* CSR_BT_CM_EXT_ADV_SETS_INFO_REQ */
    CsrBtCmDmExtAdvMultiEnableReqHandler,               /* CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ */
    CsrBtCmDmCryptoGeneratePublicPrivateKeyReqHandler,  /* CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ */
    CsrBtCmDmCryptoGenerateSharedSecretKeyReqHandler,   /* CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ */
    CsrBtCmDmCryptoEncryptReqHandler,                   /* CSR_BT_CM_CRYPTO_ENCRYPT_REQ */
    CsrBtCmDmCryptoHashReqHandler,                      /* CSR_BT_CM_CRYPTO_HASH_REQ */
    CsrBtCmDmCryptoDecryptReqHandler,                   /* CSR_BT_CM_CRYPTO_DECRYPT_REQ */
    CsrBtCmDmCryptoAesCtrReqHandler,                    /* CSR_BT_CM_CRYPTO_AES_CTR_REQ */
    CsrBtCmDmIsocConfigureCigTestReqHandler,            /* CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_REQ */
    CsrBtCmLeSetDataRelatedAddressChangesReqHandler,    /* CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ */
    CmWriteScHostSupportOverrideReqHandler,             /* CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ */
    CmReadScHostSupportOverrideMaxBdAddrReqHandler,     /* CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ */
    CsrBtCmDmIsocCreateBigTestReqHandler,               /* CSR_BT_CM_ISOC_CREATE_BIG_TEST_REQ */
    CsrBtCmDmPeriodicAdvEnableReqHandler,               /* CSR_BT_CM_PERIODIC_ADV_ENABLE_REQ*/
#if defined(CSR_BT_LE_ENABLE) && defined(CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE)
    CsrBtCmLeSetDefaultSubrateReqHandler,               /* CSR_BT_CM_SET_DEFAULT_SUBRATE_REQ */
    CsrBtCmLeSubrateChangeReqHandler,                   /* CSR_BT_CM_SUBRATE_CHANGE_REQ */
#else
    NULL,
    NULL,
#endif /* End of CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE */
    CsrBtCmSetEirDataReqHandler,                        /* CSR_BT_CM_SET_EIR_DATA_REQ */
    CmDmLeSetPhyReqHandler,                             /* CM_DM_LE_SET_PHY_REQ */
    CmDmLeSetDefaultPhyReqHandler,                      /* CM_DM_LE_SET_DEFAULT_PHY_REQ */
    CmDmChangeConnectionLinkKeyReqHandler,              /* CM_DM_CHANGE_CONNECTION_LINK_KEY_REQ */
    CmDmIsocReadIsoLinkQualityReqHandler,               /* CM_ISOC_READ_ISO_LINK_QUALITY_REQ */
    CmDmReadInquiryModeReqHandler,                      /* CM_DM_READ_INQUIRY_MODE_REQ */
    CmDmReadInquiryTxReqHandler,                        /* CM_DM_READ_INQUIRY_TX_REQ */
    CmDmReadAuthPayloadTimeoutReqHandler,               /* CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_REQ */
    CmDmReadEIRDataReqHandler,                          /* CM_DM_READ_EIR_DATA_REQ */
    CmDmLeReadRemoteTransmitPowerLevelReqHandler,       /* CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ */
    CmDmLeSetTransmitPowerReportingEnableReqHandler,    /* CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_REQ */
    CmDmLeEnhancedReadTransmitPowerLevelReqHandler,     /* CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_REQ */
    CmDmWriteInquiryModeReqHandler,                     /* CM_DM_WRITE_INQUIRY_MODE_REQ */
    CmDmExtAdvSetParamsV2ReqHandler,                    /* CM_DM_EXT_ADV_SET_PARAMS_V2_REQ */
    CmDmExtAdvGetAddressReqHandler,                     /* CM_DM_EXT_ADV_GET_ADDR_REQ */
    CmDuAutomaticProcedureHandler,                      /* CM_DM_DU_AUTO_EVENT_REQ */
    CmDmLeReadPhyReqHandler,                            /* CM_DM_LE_READ_PHY_REQ */
    CmDmRemoveDeviceKeyReqHandler,                      /* CM_DM_REMOVE_DEVICE_KEY_REQ */
    CmDmRemoveDeviceOptionsReqHandler,                  /* CM_DM_REMOVE_DEVICE_OPTIONS_REQ */
    CmDmBnepSwitchRoleReqHandler,                       /* CM_DM_BNEP_SWITCH_ROLE_REQ */
    CmDmWriteLinkPolicyReqHandler,                      /* CM_DM_WRITE_LINK_POLICY_REQ */
    CmDmReadLinkPolicyReqHandler,                       /* CM_DM_READ_LINK_POLICY_REQ */
    CmDmSwitchRoleReqHandler,                           /* CM_DM_SWITCH_ROLE_REQ */
    CmDmExtSetConnParamsReqHandler,                     /* CM_DM_EXT_SET_CONN_PARAMS_REQ */
    CmDmConfigureDataPathReqHandler,                    /* CM_DM_HCI_CONFIGURE_DATA_PATH_REQ */
    CmDmLeReadChannelMapReqHandler,                     /* CM_DM_LE_READ_CHANNEL_MAP_REQ */
    CmDmLeSetPathLossReportingParametersReqHandler,     /* CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ */
    CmDmLeSetPathLossReportingEnableReqHandler,         /* CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_REQ */
    CmDmReadConnAcceptTimeoutReqHandler,                /* CM_DM_READ_CONN_ACCEPT_TIMEOUT_REQ */
    CmDmWriteConnAcceptTimeoutReqHandler,               /* CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_REQ */
    CmDmSniffSubRateReqHandler,                         /* CM_DM_SNIFF_SUB_RATE_REQ */
};

static void CsrBtCmDmSignalHandler(cmInstanceData_t *cmData)
{
    CsrPrim         *primPtr;

    primPtr = (CsrPrim *) cmData->recvMsgP;
    if ((*primPtr - CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST < CSR_BT_CM_DM_PRIM_DOWNSTREAM_COUNT) &&
        dmProviderHandler[*primPtr])
    {
        CsrBtCmDmLockQueue(cmData);
        dmProviderHandler[*primPtr](cmData);
    }

    else
    {
        CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                *primPtr,
                                cmData->globalState,
                                "");
    }
}

void CsrBtCmDmProvider(cmInstanceData_t *cmData)
{
    CsrPrim         *primPtr;

    primPtr = (CsrPrim *) cmData->recvMsgP;

    if (CSR_BT_CM_DM_QUEUE_LOCKED(&cmData->dmVar) &&
        *primPtr != CSR_BT_CM_DISABLE_DUT_MODE_REQ)
    {    /* Need to save signal, because we are waiting for a DM complete signal.
            CSR_BT_CM_DISABLE_DUT_MODE_REQ shall be handled right away */
        CsrMessageQueuePush(&cmData->dmVar.saveQueue, CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = NULL;
    }
    else
    {    /* The DM is ready just proceed */
        CsrBtCmDmSignalHandler(cmData);
    }
}

void CsrBtCmDmLocalQueueHandler(void)
{
    CsrBtCmPrim  *prim;

    prim = CsrPmemAlloc(sizeof(CsrBtCmPrim));
    *prim = CSR_BT_CM_DM_HOUSE_CLEANING;
    CsrBtCmPutMessage(CSR_BT_CM_IFACEQUEUE, prim);
}

void CsrBtCmDmRestoreQueueHandler(cmInstanceData_t *cmData)
{
    CsrUint16          eventClass;
    void *              msg;

    CsrBtCmDmUnlockQueue(cmData);

    if(CsrMessageQueuePop(&cmData->dmVar.saveQueue, &eventClass, &msg))
    {
        SynergyMessageFree(CSR_BT_CM_PRIM, cmData->recvMsgP);
        cmData->recvMsgP = msg;
        CsrBtCmDmSignalHandler(cmData);
    }
}

CsrBool cancelDmMsg(cmInstanceData_t *cmData, CsrBtCmPrim type, CsrSchedQid phandle, CsrBtDeviceAddr bd_addr)
{
    CsrUint16                eventClass;
    void                    *msg;
    CsrBool                  cancelMsg   = FALSE;
    CsrMessageQueueType    *tempQueue  = NULL;

    while(CsrMessageQueuePop(&cmData->dmVar.saveQueue, &eventClass, &msg))
    {
        if (!cancelMsg && eventClass == CSR_BT_CM_PRIM && (type == (*((CsrBtCmPrim *) msg))))
        {
            switch (type)
            {
                case CSR_BT_CM_READ_REMOTE_NAME_REQ:
                    {
                        CsrBtCmReadRemoteNameReq * prim = (CsrBtCmReadRemoteNameReq *) msg;

                        if (phandle == prim->phandle && CsrBtBdAddrEq(&(prim->deviceAddr), &(bd_addr)))
                        {
                            cancelMsg = TRUE;
                            SynergyMessageFree(CSR_BT_CM_PRIM, msg);
                        }
                        else
                        {
                            CsrMessageQueuePush(&tempQueue, eventClass, msg);
                        }
                        break;
                    }
                default:
                    {
                        CsrMessageQueuePush(&tempQueue, eventClass, msg);
                        break;
                    }
            }
        }
        else
        {
            CsrMessageQueuePush(&tempQueue, eventClass, msg);
        }
    }
    cmData->dmVar.saveQueue = tempQueue;
    return (cancelMsg);
}
