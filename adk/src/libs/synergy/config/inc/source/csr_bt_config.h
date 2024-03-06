#ifndef _CSR_BT_FEATURE_DEFAULT_H
#define _CSR_BT_FEATURE_DEFAULT_H
/******************************************************************************
 Copyright (c) 2012-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


/*--------------------------------------------------------------------------
 * Version info
 *--------------------------------------------------------------------------*/
/* #undef CSR_BT_RELEASE_TYPE_TEST */
#define CSR_BT_VERSION_MAJOR    21
#define CSR_BT_VERSION_MINOR    0
#define CSR_BT_VERSION_FIXLEVEL 0
#define CSR_BT_VERSION_BUILD    0
#define CSR_BT_RELEASE_TYPE_ENG
#ifdef CSR_BT_RELEASE_TYPE_ENG
#define CSR_BT_RELEASE_VERSION  "21.0.0.0"
#else
#define CSR_BT_RELEASE_VERSION  "21.0.0"
#endif

/****************************************************************************
 Csr Bt Component Versions
 ****************************************************************************/
#define CSR_BT_BT_VERSION CSR_BT_BLUETOOTH_VERSION_5P4

/****************************************************************************
 Csr Board Types
 ****************************************************************************/

#define CSR_BOARD_M2107_A05 1
#define CSR_BOARD_M2107_B07 2
#define CSR_BOARD_M2399_A10 3
#define CSR_BOARD_M2501_A08 4
#define CSR_BOARD_M2501_A10 5
#define CSR_BOARD_M2501_A11 6

/****************************************************************************
 application defines
 ****************************************************************************/
/* #undef CSR_BT_APP_AMP_UWB */
/* #undef CSR_BT_APP_AMP_WIFI */
/* #undef CSR_BT_INSTALL_INTERNAL_APP_DEPENDENCIES */

/****************************************************************************
conversion from global flags to bt flags
 ****************************************************************************/
#ifdef CSR_CHIP_MANAGER_TEST
    #define CSR_CHIP_MANAGER_TEST_ENABLE
#endif

#ifdef CSR_BUILD_DEBUG
    #define DM_ACL_DEBUG
    #define INSTALL_L2CAP_DEBUG
#endif

/****************************************************************************
 Random address Types
 ****************************************************************************/
#ifdef CSR_BT_LE_ENABLE
#define RPA    1
#define NRPA   2
#define STATIC 3
#endif

/* Needed to convert to defines used by BT stack */
#ifndef WIFI_MAJOR_VERSION
#define WIFI_MAJOR_VERSION    
#endif
#ifndef WIFI_MINOR_VERSION
#define WIFI_MINOR_VERSION    
#endif
#ifndef WIFI_FIXLEVEL_VERSION
#define WIFI_FIXLEVEL_VERSION 
#endif
#ifndef WIFI_BUILD_VERSION
#define WIFI_BUILD_VERSION    
#endif

/*Reading root directory path to resolve paths for psr files to open*/
#define CSR_BT_TOPDIR          C:/Users/stomar/st11_jenkins_btvmmerge_2/bt/partial_qbl_syn


/* #undef EXCLUDE_CSR_BT_AVRCP_MODULE */
/* #undef EXCLUDE_CSR_BT_AVRCP_CT_MODULE */
/* #undef EXCLUDE_CSR_BT_AVRCP_TG_MODULE */
/* #undef EXCLUDE_CSR_BT_AVRCP_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_AVRCP_IMAGING_MODULE
#define EXCLUDE_CSR_BT_AT_MODULE
/* #undef EXCLUDE_CSR_BT_AV_MODULE */
#define EXCLUDE_CSR_BT_AV_OPTIONAL
#define EXCLUDE_CSR_BT_BPPS_MODULE
/* #undef EXCLUDE_CSR_BT_BPPS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_BPPC_MODULE
/* #undef EXCLUDE_CSR_BT_BPPC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_BIPC_MODULE
#define EXCLUDE_CSR_BT_BIPS_MODULE
#define EXCLUDE_CSR_BT_BNEP_MODULE
#define EXCLUDE_CSR_BT_BSL_MODULE
#define EXCLUDE_CSR_BT_CME_BH_FEATURE
#define EXCLUDE_CSR_BT_DG_MODULE
/* #undef EXCLUDE_CSR_BT_DHCP_MODULE */
#define EXCLUDE_CSR_BT_DUNC_MODULE
#define EXCLUDE_CSR_BT_FTC_MODULE
/* #undef EXCLUDE_CSR_BT_FTC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_FTS_MODULE
/* #undef EXCLUDE_CSR_BT_FTS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE
/* #undef EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_GNSS_SERVER_MODULE
/* #undef EXCLUDE_CSR_BT_GNSS_SERVER_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_HCRP_MODULE
#define EXCLUDE_CSR_BT_HDP_MODULE
/* #undef EXCLUDE_CSR_BT_HF_MODULE */
#define EXCLUDE_CSR_BT_HF_MODULE_OPTIONAL
/* #undef EXCLUDE_CSR_BT_HFG_MODULE */
/* #undef EXCLUDE_CSR_BT_HFG_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_HID_PARSER_MODULE
/* #undef EXCLUDE_CSR_BT_HIDD_MODULE */
#define EXCLUDE_CSR_BT_HIDD_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_HIDH_MODULE
#define EXCLUDE_CSR_BT_HOGH_MODULE
/* #undef EXCLUDE_CSR_BT_ICMP_MODULE */
/* #undef EXCLUDE_CSR_BT_IP_MODULE */
#define EXCLUDE_CSR_BT_IWU_MODULE
#define EXCLUDE_CSR_BT_JSR82_MODULE
#define EXCLUDE_CSR_BT_MAPC_MODULE
#define EXCLUDE_CSR_BT_MAPC_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_MAPS_MODULE
#define EXCLUDE_CSR_BT_MCAP_MODULE
#define EXCLUDE_CSR_BT_OPC_MODULE
/* #undef EXCLUDE_CSR_BT_OPC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_OPS_MODULE
/* #undef EXCLUDE_CSR_BT_OPS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_PAC_MODULE
#define EXCLUDE_CSR_BT_PAC_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_PAS_MODULE
#define EXCLUDE_CSR_BT_PHDC_AG_MODULE
#define EXCLUDE_CSR_BT_PHDC_MGR_MODULE
#define EXCLUDE_CSR_BT_SAPC_MODULE
#define EXCLUDE_CSR_BT_SAPS_MODULE
/* #undef EXCLUDE_CSR_BT_SCO_MODULE */
#define EXCLUDE_CSR_BT_SD_SERVICE_RECORD_MODULE
#define EXCLUDE_CSR_BT_SMLC_MODULE
/* #undef EXCLUDE_CSR_BT_SMLC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SMLS_MODULE
/* #undef EXCLUDE_CSR_BT_SMLS_MODULE_OPTIONAL */
/* #undef EXCLUDE_CSR_BT_SPP_MODULE */
#define EXCLUDE_CSR_BT_SPP_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_SYNCC_MODULE
/* #undef EXCLUDE_CSR_BT_SYNCC_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_SYNCS_MODULE
/* #undef EXCLUDE_CSR_BT_SYNCS_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_PXPM_MODULE
#define EXCLUDE_CSR_BT_PROX_SRV_MODULE
#define EXCLUDE_CSR_BT_THERM_SRV_MODULE
#define EXCLUDE_CSR_BT_TPM_MODULE
#define EXCLUDE_CSR_BT_SC_MODULE
#define EXCLUDE_CSR_BT_SC_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_SD_MODULE
#define EXCLUDE_CSR_BT_SD_MODULE_OPTIONAL
/* #undef EXCLUDE_CSR_BT_CM_MODULE */
#define EXCLUDE_CSR_BT_CM_MODULE_OPTIONAL
#define EXCLUDE_CSR_BT_OPTIONAL_UTILS
#define EXCLUDE_CSR_BT_TPT_MODULE
/* #undef EXCLUDE_CSR_BT_UDP_MODULE */
#define EXCLUDE_CSR_BT_MDER_MODULE
#define EXCLUDE_CSR_BT_GOEP_20_MODULE
#define EXCLUDE_CSR_BT_SBC_MODULE
#define EXCLUDE_CSR_BT_PPP_MODULE
/* #undef EXCLUDE_CSR_BT_PAN_MODULE */
/* #undef EXCLUDE_CSR_BT_GATT_MODULE_OPTIONAL */
/* #undef EXCLUDE_CSR_BT_GATT_MODULE_OPTIONAL2 */
/* #undef CSR_BT_LE_SIGNING_ENABLE */
/* #undef EXCLUDE_CSR_BT_L2CA_MODULE */
/* #undef EXCLUDE_CSR_BT_RFC_MODULE */
#define EXCLUDE_CSR_BT_VCARD_MODULE
/* #undef INSTALL_L2CAP_RAW_SUPPORT */
/* #undef CSR_DSPM_ENABLE */
#define EXCLUDE_CSR_BT_BSL_FLOW_CONTROL_FEATURE
#define EXCLUDE_CSR_BT_ASM_MODULE
/* #undef EXCLUDE_CSR_BT_ASM_MODULE_OPTIONAL */
#define EXCLUDE_CSR_BT_LE_SRV_MODULE
#define EXCLUDE_CSR_BT_LE_SVC_MODULE
#define EXCLUDE_CSR_BT_LPM_MODULE
#define EXCLUDE_CSR_BT_GGPROXY_MODULE
#define EXCLUDE_CSR_BT_AVDTS_SERVER_MODULE
/* #undef EXCLUDE_CSR_BT_BAS_SERVER_MODULE */
/* #undef EXCLUDE_CSR_BT_DIS_SERVER_MODULE */
/* #undef INSTALL_GATT_SECURITY_LEVELS */
/* #undef EXCLUDE_GATT_QSS_SERVER_MODULE */
/* #undef EXCLUDE_CSR_BT_GATT_SRVC_DISC_MODULE */

/*--------------------------------------------------------------------------
 * LE Audio Configurations
 *--------------------------------------------------------------------------*/
#define LE_AUDIO_ENABLE

/* #undef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE */
/* #undef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_PBP_MODULE */
/* #undef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE */
#define EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
#define EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
#define EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE

#define EXCLUDE_CSR_BT_BAP_SERVER_MODULE
#define EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
#define EXCLUDE_CSR_BT_PACS_SERVER_MODULE
#define EXCLUDE_CSR_BT_BASS_SERVER_MODULE
#define EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
#define EXCLUDE_CSR_BT_VCS_SERVER_MODULE
#define EXCLUDE_CSR_BT_MICS_SERVER_MODULE
#define EXCLUDE_CSR_BT_MCP_MODULE
#define EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
#define EXCLUDE_CSR_BT_CCP_MODULE
#define EXCLUDE_CSR_BT_TBS_CLIENT_MODULE

/* #undef EXCLUDE_CSR_BT_BAP_MODULE */
/* #undef EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE */
/* #undef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_CSIP_MODULE */
/* #undef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_VCP_MODULE */
/* #undef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_MICP_MODULE */
/* #undef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE */
/* #undef EXCLUDE_CSR_BT_MCS_SERVER_MODULE */
/* #undef EXCLUDE_CSR_BT_TBS_SERVER_MODULE */
#define EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
#define EXCLUDE_CSR_BT_AICS_CLIENT_MODULE

#define EXCLUDE_CSR_BT_AUTO_BROADCAST_DEMO
/*--------------------------------------------------------------------------*/

/* #undef CSR_DSPM_DISABLE_SCO_RATEMATCHING */
/* #undef CSR_CVC_ENABLE */
/* #undef CSR_CVC_NB_FE_ENABLE */
/* #undef CSR_BT_APP_OUTPUT_A2DP_TO_I2S */
/* #undef CSR_DSPM_SRC_ENABLE */
#define CSR_BT_CONFIG_CARKIT

/* #undef CSR_BT_BLUE_STACK_DEBUG */
/* #undef CSR_BT_CONFIG_L2CAP_FCS */
/* #undef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT */
/* #undef CSR_BT_INSTALL_L2CAP_UCD_SUPPORT */

/* #undef CSR_BT_SC_ONLY_MODE_ENABLE */
#define CSR_BT_INSTALL_LESC_SUPPORT
#define CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT

/* #undef CSR_BT_DISABLE_BREDR */

#define CSR_BT_GATT_INSTALL_FLAT_DB
/* #undef EXCLUDE_CSR_BT_DUT_MODULE */
/* #undef CSR_BT_INSTALL_AVRCP_BROWSING */
#define CSR_BT_GATT_CACHING
#define CSR_BT_GATT_INSTALL_EATT
/* #undef GATT_CACHING_CLIENT_ROLE */
/* #undef GATT_DATA_LOGGER */
/* #undef CSR_BT_WEAR_CONTEXT_TO_FILE */
/* #undef CSR_BT_SLATE_FULL_CONFIG */

#define CSR_BT_ISOC_ENABLE

#define CSR_BT_INSTALL_EXTENDED_ADVERTISING
#define CSR_BT_INSTALL_EXTENDED_SCANNING
#define CSR_BT_INSTALL_PERIODIC_ADVERTISING
#define CSR_BT_INSTALL_PERIODIC_SCANNING

/* #undef CSR_BT_HFG_INCLUDE_SWB_SUPPORT */
#define EXCLUDE_CSR_BT_TPS_SERVER_MODULE
#define EXCLUDE_CSR_BT_TDS_SERVER_MODULE
#define EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
#define EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
#define CSR_BT_INSTALL_CRYPTO_SUPPORT

/* #undef CSR_BT_INSTALL_QUAL_TESTER_SUPPORT */

#ifdef CSR_BT_LE_ENABLE
/*-------------------------------------------------------------------------------------------------------*
 * CTKD shall not be enabled when local random address is configured as NRPA or STATIC(static address).  *
 * 1. In case of NRPA: Synergy does not allow bonding, so keys on other transport cannot be generated.   *
 * 2. In case of Static address: Synergy distributes the same static address as its Identity address     *
 *    while pairing, not the public address, so again correct keys can't be generated.                   *
 *-------------------------------------------------------------------------------------------------------*/
#if (RPA == RPA)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_RPA
#define CSR_BT_INSTALL_CTKD_SUPPORT
#elif (RPA == NRPA)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_NRPA
#elif (RPA == STATIC)
#define CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
#endif /* CSR_BT_LE_RANDOM_ADDRESS_TYPE */

#undef RPA
#undef NRPA
#undef STATIC

#define CSR_BT_INSTALL_DLE_SUPPORT

#endif /* CSR_BT_LE_ENABLE */

#ifndef DATAPATH_ID
#define DATAPATH_ID 0
#endif

/* Conversion of application instances */
#ifndef EXCLUDE_CSR_BT_AV_MODULE
#define NUM_AV_INST 1
#else
#define NUM_AV_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE
#define NUM_AVRCP_INST 1
#else
#define NUM_AVRCP_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_DUNC_MODULE
#define NUM_DUNC_INST 0
#else
#define NUM_DUNC_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_DG_MODULE
#define NUM_DG_INST 0
#else
#define NUM_DG_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HF_MODULE
#define NUM_HF_INST 1
#else
#define NUM_HF_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HFG_MODULE
#define NUM_HFG_INST 1
#else
#define NUM_HFG_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HCRP_MODULE
#define NUM_HCRPS_INST 0
#else
#define NUM_HCRPS_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HDP_MODULE
#define NUM_HDP_INST 0
#else
#define NUM_HDP_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
#define NUM_HIDD_INST 0
#else
#define NUM_HIDD_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_HIDH_MODULE
#define NUM_HIDH_INST 0
#else
#define NUM_HIDH_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_GNSS_CLIENT_MODULE
#define NUM_GNSS_CLIENT_INST 0
#else
#define NUM_GNSS_CLIENT_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_GNSS_SERVER_MODULE
#define NUM_GNSS_SERVER_INST 0
#else
#define NUM_GNSS_SERVER_INST    0
#endif


#ifndef EXCLUDE_CSR_BT_BIPC_MODULE
#define NUM_BIPC_INST 0
#else
#define NUM_BIPC_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_BIPS_MODULE
#define NUM_BIPS_INST 0
#else
#define NUM_BIPS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_BPPC_MODULE
#define NUM_BPPC_INST 0
#else
#define NUM_BPPC_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_BPPS_MODULE
#define NUM_BPPS_INST 0
#else
#define NUM_BPPS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_FTC_MODULE
#define NUM_FTC_INST 0
#else
#define NUM_FTC_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_FTS_MODULE
#define NUM_FTS_INST 0
#else
#define NUM_FTS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
/* Set this value to 1 when CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT is not enabled */
#define NUM_MAPC_INST 2
/* NUM_MAPC_NOTI_INST must be 1 unless more num of notification instances are added in MAPC module */
#define NUM_MAPC_NOTI_INST 1
#else
#define NUM_MAPC_INST      0
#define NUM_MAPC_NOTI_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_MAPS_MODULE
#define NUM_MAPS_INST 0
#else
#define NUM_MAPS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_PAC_MODULE
#define NUM_PAC_INST 1
#else
#define NUM_PAC_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_PAS_MODULE
#define NUM_PAS_INST 0
#else
#define NUM_PAS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_OPC_MODULE
#define NUM_OPC_INST 0
#else
#define NUM_OPC_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_OPS_MODULE
#define NUM_OPS_INST 0
#else
#define NUM_OPS_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_SYNCC_MODULE
#define NUM_SYNCC_INST 0
#else
#define NUM_SYNCC_INST    0
#endif

/* Max number of obex instances possible.
 * Currently PAC & MAP are the only modules (for VM) which use obex instances.
 * Update this value if any new module initializes the OBEX instance.*/
#define OBEX_MAX_NUM_INSTANCES    (PAC_MAX_NUM_INSTANCES + (NUM_MAPC_INST + NUM_MAPC_NOTI_INST))

#ifdef EXCLUDE_CSR_BT_BNEP_MODULE
#define EXCLUDE_CSR_BT_BSL_MODULE
#define EXCLUDE_CSR_BT_BSL_FLOW_CONTROL_FEATURE
#define EXCLUDE_CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_CFM
#define EXCLUDE_CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_CONNECT_ACCEPT_CFM
#define EXCLUDE_CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_CONNECT_IND
#define EXCLUDE_CSR_BT_CM_BNEP_CONNECT_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_DISCONNECT_IND
#define EXCLUDE_CSR_BT_CM_BNEP_DISCONNECT_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_DISCONNECT_RES
#define EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_CFM
#define EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_IND
#define EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_MODE_CHANGE_IND
#define EXCLUDE_CSR_BT_CM_BNEP_MODE_CHANGE_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_REGISTER_REQ
#define EXCLUDE_CSR_BT_CM_BNEP_SWITCH_ROLE_IND
#define EXCLUDE_CM_DM_BNEP_SWITCH_ROLE_REQ
#define EXCLUDE_CSR_BT_CM_CANCEL_BNEP_CONNECT_REQ
#define EXCLUDE_CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ
#endif

#ifndef EXCLUDE_CSR_BT_BSL_MODULE
#define NUM_PAN_INST 0
#else
#define NUM_PAN_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#define NUM_SPP_INST 0
#else
#define NUM_SPP_INST    0
#endif

#ifdef CSR_BT_LE_ENABLE
#define NUM_GATT_GENERIC_SERVER_INST 0
#define NUM_GATT_LE_BROWSER_INST 0
#define NUM_GATT_THERMC_INST 0
#define NUM_GATT_THERMS_INST 0
#define NUM_GATT_HOGD_INST 0
#define NUM_GATT_LE_AUDIO_INST 0

#ifndef EXCLUDE_CSR_BT_HOGH_MODULE
#define NUM_GATT_HOGH_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_PXPM_MODULE
#define NUM_GATT_PXPM_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_LE_SRV_MODULE
#define NUM_GATT_FMPS_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_LPM_MODULE
#define NUM_GATT_YH_INST 0
#endif

#define NUM_GATT_PROXS_INST 0
#define NUM_GATT_RSCC_INST 0
#define NUM_GATT_RSCS_INST 0
#endif

#ifdef NUM_GATT_HOGH_INST
#define CSR_BT_HOGH_ENABLE_INITIAL_REPORTS
#endif

#ifndef EXCLUDE_CSR_BT_TPM_MODULE
#define NUM_GATT_TPM_INST 0
#define NUM_GATT_TPMS_INST 0
#endif

#ifndef EXCLUDE_CSR_BT_PHDC_AG_MODULE
#define NUM_PHDC_AG_INST 0
#else
#define NUM_PHDC_AG_INST    0
#endif

#ifndef EXCLUDE_CSR_BT_PHDC_MGR_MODULE
#define NUM_PHDC_MGR_INST 0
#else
#define NUM_PHDC_MGR_INST    0
#endif

#ifdef CSR_AMP_ENABLE
#define NUM_AMPWIFI_INST 0
#endif

#ifdef CSR_DSPM_ENABLE
#if ((NUM_AV_INST) || (NUM_HF_INST) || (NUM_HFG_INST))
#define CSR_USE_DSPM
#define EXCLUDE_CSR_BT_CHIP_SUPPORT_MAP_SCO_PCM
#endif

#ifdef CSR_DSPM_DISABLE_SCO_RATEMATCHING
#define CSR_DISABLE_SCO_RATEMATCHING
#endif

#ifdef CSR_CVC_ENABLE
#define CSR_USE_CVC_1MIC
#ifdef CSR_CVC_NB_FE_ENABLE
#define CSR_FE_ENABLE
#endif
#endif

#ifdef CSR_DSPM_SRC_ENABLE
#define CSR_BT_AV_USE_RESAMPLER
#endif

#ifdef CSR_BT_APP_OUTPUT_A2DP_TO_I2S
#define CSR_A2DP_OVER_I2S
#endif

#ifndef EXCLUDE_CSR_BT_ASM_MODULE
#define CSR_BT_APP_MPAA_ENABLE
#endif

#endif /* CSR_DSPM_ENABLE */

#if defined(CSR_USE_QCA_CHIP) || defined(EXCLUDE_CSR_BCCMD_MODULE)
#define EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#define EXCLUDE_CSR_BT_CHIP_SUPPORT_MAP_SCO_PCM
#endif

#ifdef CSR_AMP_ENABLE
#if (CSR_BOARD_M2501_A10 == CSR_BOARD_M2107_A05)
    #define CSR_WIFI_DESIGN_M2107_R02
#elif (CSR_BOARD_M2501_A10 == CSR_BOARD_M2107_B07)
    #define CSR_WIFI_DESIGN_M2107_R03
#elif (CSR_BOARD_M2501_A10 == CSR_BOARD_M2399_A10)
    #define CSR_WIFI_DESIGN_M2399_R03
#elif (CSR_BOARD_M2501_A10 == CSR_BOARD_M2501_A08)
    #define CSR_WIFI_DESIGN_M2501_R03
#elif (CSR_BOARD_M2501_A10 == CSR_BOARD_M2501_A10)
    #define CSR_WIFI_DESIGN_M2501_R03
#endif /*CSR_BOARD */
#endif /* CSR_AMP_ENABLE */

#ifdef EXCLUDE_CSR_BT_CME_BH_FEATURE
#define EXCLUDE_CSR_BT_CM_SET_AV_STREAM_INFO_REQ /* exclude serialization code */
#endif

#ifndef CSR_BT_PAGE_TIMEOUT
#define CSR_BT_PAGE_TIMEOUT 24000
#endif

#ifndef TD_DB_PATH
#define TD_DB_PATH ""
#endif

#ifndef TD_DB_FILE_NAME
#define TD_DB_FILE_NAME "Synergy_DataBaseStorage.db"
#endif
/****************************************************************************
 Csr Bt Component Versions
 ****************************************************************************/
#if defined (CSR_TARGET_PRODUCT_VM)
#define SYNERGY_VM_SOURCE_APP_ENABLED
#endif
#if defined (CSR_TARGET_PRODUCT_WEARABLE)
#define DEFINE_QAPI_STATUS
#endif

/*--------------------------------------------------------------------------
 * Include csr_bt_devel_config.h if the development Configuration is available
 *--------------------------------------------------------------------------*/
/* #undef CSR_BT_DEVEL_CONFIG_ENABLE */
#ifdef CSR_BT_DEVEL_CONFIG_ENABLE
#include "csr_bt_devel_config.h"
#endif

#include "csr_bt_config_global.h"

#endif /* _CSR_BT_FEATURE_DEFAULT_H */
