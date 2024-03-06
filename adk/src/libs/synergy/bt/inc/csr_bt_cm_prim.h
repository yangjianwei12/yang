#ifndef CSR_BT_CM_PRIM_H__
#define CSR_BT_CM_PRIM_H__

/******************************************************************************
 Copyright (c) 2001-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_profiles.h"
#include "csr_types.h"
#include "csr_bt_util.h"
#include "csr_bt_addr.h"
#include "rfcomm_prim.h"
#if defined(HYDRA) || defined(CAA)
#include "hci.h"
#else
#include "hci_prim.h"
#endif
#include "l2cap_prim.h"
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_bnep_prim.h"
#endif
#include "dm_prim.h"
#include "csr_bt_result.h"
#include "csr_mblk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrBtCmPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

/* ---------- Defines the Connection Managers (CM) CsrBtResultCode ----------*/
#define CSR_BT_RESULT_CODE_CM_SUCCESS                                   ((CsrBtResultCode) (0x0000))
#define CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED                        ((CsrBtResultCode) (0x0001))
#define CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR                            ((CsrBtResultCode) (0x0002))
#define CSR_BT_RESULT_CODE_CM_CANCELLED                                 ((CsrBtResultCode) (0x0003))
#define CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER             ((CsrBtResultCode) (0x0004))
#define CSR_BT_RESULT_CODE_CM_UNSPECIFIED_ERROR                         ((CsrBtResultCode) (0x0005))
#define CSR_BT_RESULT_CODE_CM_ALREADY_CONNECTING                        ((CsrBtResultCode) (0x0006))
#define CSR_BT_RESULT_CODE_CM_UNACCEPTABLE_PARAMETER                    ((CsrBtResultCode) (0x0007))
#define CSR_BT_RESULT_CODE_CM_NUMBER_OF_LM_EVENT_FILTERS_EXCEEDED       ((CsrBtResultCode) (0x0008))
#define CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE                       ((CsrBtResultCode) (0x0009))
#define CSR_BT_RESULT_CODE_CM_REJECTED_DUE_TO_LIMITED_RESOURCES         ((CsrBtResultCode) (0x000A))
#define CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL                         ((CsrBtResultCode) (0x000B))
#define CSR_BT_RESULT_CODE_CM_REBOND_REJECTED_BY_APPLICATION            ((CsrBtResultCode) (0x000C))
#define CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_EXISTS     ((CsrBtResultCode) (0x000D))
#define CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE    ((CsrBtResultCode) (0x000E))
#define CSR_BT_RESULT_CODE_CM_INVALID_PCM_SLOT                          ((CsrBtResultCode) (0x000F))
#define CSR_BT_RESULT_CODE_CM_PCM_SLOT_BLOCKED                          ((CsrBtResultCode) (0x0010))
#define CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED     ((CsrBtResultCode) (0x0011))
#define CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ATTEMPT_FAILED     ((CsrBtResultCode) (0x0012))
#define CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_ACCEPTABLE ((CsrBtResultCode) (0x0013))
#define CSR_BT_RESULT_CODE_CM_MODE_CHANGE_REQUEST_OVERRULED             ((CsrBtResultCode) (0x0014))
#define CSR_BT_RESULT_CODE_CM_FLOW_CONTROL_VIOLATED                     ((CsrBtResultCode) (0x0015))
#define CSR_BT_RESULT_CODE_CM_BNEP_CONNECTION_LIMIT_EXCEEDED            ((CsrBtResultCode) (0x0016))
#define CSR_BT_RESULT_CODE_CM_TIMEOUT                                   ((CsrBtResultCode) (0x0017))
#define CSR_BT_RESULT_CODE_CM_AMP_LINK_LOSS_MOVE                        ((CsrBtResultCode) (0x0018))
#define CSR_BT_RESULT_CODE_CM_SERVER_CHANNEL_ALREADY_USED               ((CsrBtResultCode) (0x0019))
#define CSR_BT_RESULT_CODE_CM_ALREADY_DISCONNECTING                     ((CsrBtResultCode) (0x001A))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_ECC_INVALID_KEY_TYPE               ((CsrBtResultCode) (0x001B))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_ECDH_INVALID_KEY_TYPE              ((CsrBtResultCode) (0x001C))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_ECDH_EMPTY_PUBLIC_KEY              ((CsrBtResultCode) (0x001D))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_ECDH_EMPTY_PRIVATE_KEY             ((CsrBtResultCode) (0x001E))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_DATA_ARRAY               ((CsrBtResultCode) (0x001F))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_KEY_ARRAY                ((CsrBtResultCode) (0x0020))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_SHA_EMPTY_DATA_ARRAY               ((CsrBtResultCode) (0x0021))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_SHA_CONT_EMPTY_DATA_ARRAY          ((CsrBtResultCode) (0x0022))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_AES_INVALID_FLAG                   ((CsrBtResultCode) (0x0023))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_AES_EMPTY_NONCE_ARRAY              ((CsrBtResultCode) (0x0024))
#define CSR_BT_RESULT_CODE_CM_CRYPTO_AES_INVALID_DATA_LEN               ((CsrBtResultCode) (0x0025))
#define RESULT_CODE_CM_POWER_TABLE_UPDATED                              ((CsrBtResultCode) (0x0026))

#define CSR_BT_ACTIVE_MODE               (0x0000)
#define CSR_BT_HOLD_MODE                 (0x0001)
#define CSR_BT_SNIFF_MODE                (0x0002)
#define CSR_BT_PASSIVE_MODE              (0x00FF)

#define CSR_BT_NO_SERVER                 (0xFF)

#define CSR_BT_MICROSEC2SEC              (1000000)

#define CSR_BT_EVENT_FILTER_ALL          (0)
#define CSR_BT_EVENT_FILTER_INQUIRY      (1)
#define CSR_BT_EVENT_FILTER_CONNECTIONS  (2)

#define CSR_BT_CM_CONTEXT_UNUSED         (0x0000)

#define CSR_BT_CM_ACCESS_CODE_GIAC       (0x9e8b33)
#define CSR_BT_CM_ACCESS_CODE_LIAC       (0x9e8b00)
#define CSR_BT_CM_ACCESS_CODE_HIGHEST    (0x9e8b3f)

#define CM_CREATE_RFC_CONN_ID(c)                    ((CsrBtConnId) (CSR_BT_CONN_ID_RFCOMM_TECH_MASK | c))
#define CM_CREATE_L2CA_CONN_ID(c)                   ((CsrBtConnId) (CSR_BT_CONN_ID_L2CAP_TECH_MASK | c))
#define CM_GET_UINT16ID_FROM_BTCONN_ID(c)           ((CsrUint16) (CSR_BT_CONN_ID_GET_MASK & c))

/* A2DP */
/* A2DP bit rate */
#define CSR_BT_A2DP_BIT_RATE_UNKNOWN            ((CsrUint32) 0x00000000)
#define CSR_BT_A2DP_BIT_RATE_STREAM_SUSPENDED   ((CsrUint32) 0xFFFFFFFEu)
#define CSR_BT_A2DP_BIT_RATE_STREAM_CLOSED      ((CsrUint32) 0xFFFFFFFFu)

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
/* These definitions should match with CsrCmeCodecType in csr_cme_prim_8.h */
/* A2DP sampling rate */
#define CSR_BT_A2DP_SAMPLING_FREQ_16000     ((CsrUint8) 0x00)
#define CSR_BT_A2DP_SAMPLING_FREQ_32000     ((CsrUint8) 0x01)
#define CSR_BT_A2DP_SAMPLING_FREQ_44100     ((CsrUint8) 0x02)
#define CSR_BT_A2DP_SAMPLING_FREQ_48000     ((CsrUint8) 0x03)
#define CSR_BT_A2DP_SAMPLING_FREQ_UNKNOWN   ((CsrUint8) 0xFF)

/* A2DP codec location */
#define CSR_BT_A2DP_CODEC_LOCATION_OFF_CHIP ((CsrUint8) 0x00)
#define CSR_BT_A2DP_CODEC_LOCATION_ON_CHIP  ((CsrUint8) 0x01)
#define CSR_BT_A2DP_CODEC_LOCATION_UNKNOWN  ((CsrUint8) 0xFF)

/* A2DP codec type ; This is not based on bluetooth assigned numbers */
#define CSR_BT_A2DP_CODEC_TYPE_SBC          ((CsrUint8) 0x00)
#define CSR_BT_A2DP_CODEC_TYPE_APTX         ((CsrUint8) 0x01)
#define CSR_BT_A2DP_CODEC_TYPE_UNKNOWN      ((CsrUint8) 0xFF)
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

typedef CsrUint32              CsrBtCmEventMask;
/* Defines for event that the application can subscribe for */
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_NONE                         ((CsrBtCmEventMask) 0x00000000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED         ((CsrBtCmEventMask) 0x00000001)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION               ((CsrBtCmEventMask) 0x00000002)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SYNCHRONOUS_CONNECTION       ((CsrBtCmEventMask) 0x00000004)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE                  ((CsrBtCmEventMask) 0x00000008)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_MODE_CHANGE                  ((CsrBtCmEventMask) 0x00000010)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE                  ((CsrBtCmEventMask) 0x00000020)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE                 ((CsrBtCmEventMask) 0x00000040)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_EXT_SYNC_CONNECTION          ((CsrBtCmEventMask) 0x00000080)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES              ((CsrBtCmEventMask) 0x00000100)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_VERSION               ((CsrBtCmEventMask) 0x00000200)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_A2DP_BIT_RATE                ((CsrBtCmEventMask) 0x00000400)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_INQUIRY_PAGE_STATE           ((CsrBtCmEventMask) 0x00000800)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY                   ((CsrBtCmEventMask) 0x00001000) /* only for BT4.0+ devices/stacks */
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE               ((CsrBtCmEventMask) 0x00002000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOCAL_NAME_CHANGE            ((CsrBtCmEventMask) 0x00004000) /* only for BT2.1+ devices/stacks */
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_HIGH_PRIORITY_DATA           ((CsrBtCmEventMask) 0x00008000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_DEINITIALIZED       ((CsrBtCmEventMask) 0x00010000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_OWN_ADDR_TYPE_CHANGE      ((CsrBtCmEventMask) 0x00020000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SIMPLE_PAIRING_COMPLETE      ((CsrBtCmEventMask) 0x00040000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LE_SUBRATE_CHANGE            ((CsrBtCmEventMask) 0x00080000)
#define CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND                ((CsrBtCmEventMask) 0x00100000)
#define CM_EVENT_MASK_SUBSCRIBE_EXT_SCAN_TIMEOUT_IND                ((CsrBtCmEventMask) 0x00200000)

#define CSR_BT_CM_NUM_OF_CM_EVENTS                                  (0x00000016)
#define CSR_BT_CM_EVENT_MASK_RESERVER_VALUES_MASK                   (0x003FFFFF)

/* Backwards compatibility mapping */
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLE_CONNECTION               (CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY)

typedef CsrUint32              CmSetLinkBehaviorMask;
/* Defines for the set link behavior configuration values */
#define CM_SET_LINK_BEHAVIOR_NONE                                   ((CmSetLinkBehaviorMask) 0x00000000)
#define CM_SET_LINK_BEHAVIOR_L2CAP_RETRY_ON                         ((CmSetLinkBehaviorMask) 0x00000001)
#define CM_SET_LINK_BEHAVIOR_APP_HANDLE_PING                        ((CmSetLinkBehaviorMask) 0x00000002)
#define CM_SET_LINK_BEHAVIOR_DONT_ESTABLISH_ACL_ON_L2CAP_CONNECT    ((CmSetLinkBehaviorMask) 0x00000004)

typedef CsrUint16 CsrBtCmEventMaskCond;
/* Condition for the eventmask */
#define CSR_BT_CM_EVENT_MASK_COND_NA                           ((CsrBtCmEventMaskCond) 0x0000)
#define CSR_BT_CM_EVENT_MASK_COND_SUCCESS                      ((CsrBtCmEventMaskCond) 0x0001)
#define CSR_BT_CM_EVENT_MASK_COND_UNKNOWN                      ((CsrBtCmEventMaskCond) 0xEFFF)
#define CSR_BT_CM_EVENT_MASK_COND_ALL                          ((CsrBtCmEventMaskCond) 0xFFFF)

#define CSR_BT_CM_ENC_TYPE_NONE                 DM_SM_ENCR_NONE
#define CSR_BT_CM_ENC_TYPE_LE_AES_CCM           DM_SM_ENCR_ON_LE_AES_CCM
#define CSR_BT_CM_ENC_TYPE_BREDR_E0             DM_SM_ENCR_ON_BREDR_EO
#define CSR_BT_CM_ENC_TYPE_BREDR_AES_CCM        DM_SM_ENCR_ON_BREDR_AES_CCM
#define CSR_BT_CM_ENC_TYPE_KEY_REFRESH          (CSR_BT_CM_ENC_TYPE_BREDR_AES_CCM + 1)

#define CSR_BT_ASSIGN_DYNAMIC_PSM      (0x0000)
#define CSR_BT_ACL_HANDLE_INVALID      HCI_HANDLE_INVALID

#define CSR_BT_CM_PRIORITY_HIGH                 (0x01)
#define CSR_BT_CM_PRIORITY_HIGHEST              (0x00)

#define CSR_BT_CM_PRIORITY_NORMAL               (0x03) /* Must be highest */

#define CSR_BT_CM_ALWAYS_MASTER_DEVICES_CLEAR   DM_LP_WRITE_ALWAYS_MASTER_DEVICES_CLEAR
#define CSR_BT_CM_ALWAYS_MASTER_DEVICES_ADD     DM_LP_WRITE_ALWAYS_MASTER_DEVICES_ADD
#define CSR_BT_CM_ALWAYS_MASTER_DEVICES_DELETE  DM_LP_WRITE_ALWAYS_MASTER_DEVICES_DELETE

#define CSR_BT_CM_HANDLER_TYPE_SD               (0x00) /* handles service discovery type primitives */
#define CSR_BT_CM_HANDLER_TYPE_SC               (0x01) /* handles security type primitives */
#define CSR_BT_CM_HANDLER_TYPE_AMP              (0x02) /* handles AMP type primitives */
#define CSR_BT_CM_HANDLER_TYPE_LE               (0x03) /* handles low energy type primitives */
#define CSR_BT_CM_HANDLER_TYPE_CONN_OFFLOAD     (0x04) /* handles connection context offload messages */

/* UUID types */
#define CSR_BT_CM_SDC_UUID_NONE     0
#define CSR_BT_CM_SDC_UUID32        1
#define CSR_BT_CM_SDC_UUID128       2
#define CSR_BT_CM_SDC_UUID_SET      3

/* Piconet roles */
#define CSR_BT_CM_ROLE_MASTER                   (HCI_MASTER)
#define CSR_BT_CM_ROLE_SLAVE                    (HCI_SLAVE)

/* Low energy advertising types */
#define CSR_BT_CM_LE_ADVTYPE_CONNECTABLE_UNDIRECTED         (HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED)
#define CSR_BT_CM_LE_ADVTYPE_CONNECTABLE_DIRECTED_HIGH_DUTY (HCI_ULP_ADVERT_CONNECTABLE_DIRECTED_HIGH_DUTY)
#define CSR_BT_CM_LE_ADVTYPE_SCANNABLE                      (HCI_ULP_ADVERT_DISCOVERABLE)
#define CSR_BT_CM_LE_ADVTYPE_NONCONNECTABLE                 (HCI_ULP_ADVERT_NON_CONNECTABLE)

/* Low energy scan types */
#define CSR_BT_CM_LE_SCANTYPE_PASSIVE           (HCI_ULP_PASSIVE_SCANNING)
#define CSR_BT_CM_LE_SCANTYPE_ACTIVE            (HCI_ULP_ACTIVE_SCANNING)
#define CSR_BT_CM_LE_SCANTYPE_INITIATING        (HCI_ULP_ACTIVE_SCANNING + 1)

/* Low energy advertising channel */
#define BT_CM_LE_CHANMAP_37                         (HCI_ULP_ADVERT_CHANNEL_37)
#define BT_CM_LE_CHANMAP_38                         (HCI_ULP_ADVERT_CHANNEL_38)
#define BT_CM_LE_CHANMAP_39                         (HCI_ULP_ADVERT_CHANNEL_39)
#define BT_CM_LE_CHANMAP_ALL                        (HCI_ULP_ADVERT_CHANNEL_ALL)

/* Low energy master clock accuracy */
#define CSR_BT_CM_LE_CLOCKACCU_500PPM           (HCI_ULP_EV_CLOCK_ACCURACY_500PPM)
#define CSR_BT_CM_LE_CLOCKACCU_250PPM           (HCI_ULP_EV_CLOCK_ACCURACY_250PPM)
#define CSR_BT_CM_LE_CLOCKACCU_150PPM           (HCI_ULP_EV_CLOCK_ACCURACY_150PPM)
#define CSR_BT_CM_LE_CLOCKACCU_100PPM           (HCI_ULP_EV_CLOCK_ACCURACY_100PPM)
#define CSR_BT_CM_LE_CLOCKACCU_75PPM            (HCI_ULP_EV_CLOCK_ACCURACY_75PPM)
#define CSR_BT_CM_LE_CLOCKACCU_50PPM            (HCI_ULP_EV_CLOCK_ACCURACY_50PPM)
#define CSR_BT_CM_LE_CLOCKACCU_30PPM            (HCI_ULP_EV_CLOCK_ACCURACY_30PPM)
#define CSR_BT_CM_LE_CLOCKACCU_20PPM            (HCI_ULP_EV_CLOCK_ACCURACY_20PPM)

/* Low energy scan/advertising/connection modes */
#define CSR_BT_CM_LE_MODE_OFF                   (0x00) /* turn off */
#define CSR_BT_CM_LE_MODE_ON                    (0x01) /* turn on */
#define CSR_BT_CM_LE_MODE_MODIFY                (0x02) /* turn off, set params, and turn on again */
#define CSR_BT_CM_LE_MODE_CONTINUE              (0x04) /* set params, and continue */
#define CSR_BT_CM_LE_MODE_IGNORE                (0x40) /* internal in CM, do not use */
#define CSR_BT_CM_LE_MODE_COEX_NOTIFY           (0x80) /* GATT-to-CM coex notification. Do not use! */

#define CSR_BT_CM_LE_PARCHG_NONE                (0x00000000) /* do not change */
#define CSR_BT_CM_LE_PARCHG_PAR                 (0x00000001) /* change HCI (advertise) parameters */
#define CSR_BT_CM_LE_PARCHG_DATA_AD             (0x00000002) /* change AD data */
#define CSR_BT_CM_LE_PARCHG_DATA_SR             (0x00000004) /* change SR data */
#define CSR_BT_CM_LE_PARCHG_CONNPAR             (0x00000008) /* change HCI connection parameters */
#define CSR_BT_CM_LE_PARCHG_WHITELIST           (0x00000010) /* set whitelist */
#define CSR_BT_CM_LE_PARCHG_INTERNAL            (0x00000080) /* internal update flag */
#define CSR_BT_CM_LE_PARCHG_DIRECT_ADV          (0x00000100) /* internal update flag */

#define CSR_BT_CM_LE_MAX_REPORT_LENGTH          (31)   /* maximum length of advertise/scan-response data */
#define CM_L2CAP_PING_DATA_LENGTH               (44)   /* maximum ping data length */
#define CSR_BT_CM_LE_WHITELIST_ADD              (0x00)
#define CSR_BT_CM_LE_WHITELIST_CLEAR            (0x01)

#define CSR_BT_CM_LE_ADV_INTERVAL_NOCONN_MIN    (0x00A0) /* min for ADV_SCAN_IND and ADV_NONCONN_IND */
#define CSR_BT_CM_LE_ADV_INTERVAL_MIN           (0x0020) /* min allowed */
#define CSR_BT_CM_LE_ADV_INTERVAL_MAX           (0x4000) /* max allowed */
#define CSR_BT_CM_LE_ADV_INTERVAL_DEFAULT       (0x0800) /* min for ADV_SCAN_IND and ADV_NONCONN_IND */

/* LE Privacy : type defines for privacy mode & device flag */
typedef CsrUint8 CsrBtPrivacyMode;
typedef CsrUint8 CsrBtDeviceFlag;

/* LE Privacy Modes */
#define CSR_BT_NETWORK_PRIVACY_MODE             ((CsrBtPrivacyMode)0x00)
#define CSR_BT_DEVICE_PRIVACY_MODE              ((CsrBtPrivacyMode)0x01)

/* Device flags for local or peer role */
#define CSR_BT_DEVICE_FLAG_LOCAL                ((CsrBtDeviceFlag)DM_SM_READ_ADDRESS_LOCAL)
#define CSR_BT_DEVICE_FLAG_PEER                 ((CsrBtDeviceFlag)DM_SM_READ_ADDRESS_PEER)

#define CSR_BT_CM_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE        (10) /* From HCI spec */
#define CSR_BT_CM_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE_LE     (5)  /* From HCI spec */
#define CSR_BT_CM_AFH_AFH_HOST_CHANNEL_CLASSIFICATION_BAD     (0)  /* From HCI spec */
#define CSR_BT_CM_AFH_AFH_HOST_CHANNEL_CLASSIFICATION_UNKNOWN (1)  /* From HCI spec */

/*ACL FLAGS*/
#define CSR_BT_CM_ACL_OPEN_ULP DM_ACL_FLAG_ULP
#define CSR_BT_CM_ACL_OPEN_CENTRAL_WHITELIST L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_WHITELIST)
typedef CsrPrim CsrBtCmPrim;

/* Options for enabling security features */
typedef CsrUint8 CmSecurityConfigOptions;

/* Default security configuration */
#define CM_SECURITY_CONFIG_OPTION_NONE                      ((CmSecurityConfigOptions)0x00)

/* Enable Secure Connections. */
#define CM_SECURITY_CONFIG_OPTION_SECURE_CONNECTIONS        ((CmSecurityConfigOptions)0x01)

/* Setting this option will disable the Cross Transport Key
 * Derivation. This means connecting on one transport will cause pairing, then
 * a connection to the same device on the other transport will cause
 * pairing again, for that transport.
 */
#define CM_SECURITY_CONFIG_OPTION_DISABLE_CTKD              ((CmSecurityConfigOptions)0x02)

/* Enable Selective Cross Transport Key Derivation (CTKD) per device.
 * When this option is enabled, during pairing process application will get
 * CM_SM_GENERATE_CROSS_TRANS_KEY_IND message to which the application 
 * must respond with API CmGenerateCrossTransKeyRequestResponse to enable
 * or disable CTKD for that peer device.
 *
 * When enabled, pairing with a device on one transport will generate security information
 * of the other transport, otherwise pairing will happen on both transports  
 * as and when the device connects.
 *
 * Note: this feature bit is ignored if CM_SECURITY_CONFIG_OPTION_DISABLE_CTKD is set.
*/
#define CM_SECURITY_CONFIG_OPTION_SELECTIVE_CTKD            ((CmSecurityConfigOptions)0x04)

/* Enable Secure Connections only mode. This needs to be carefully enabled as the compatibilty
 * of the device with the other Non-SC devices may get impacted. */
#define CM_SECURITY_CONFIG_OPTION_SC_ONLY                   ((CmSecurityConfigOptions)0x08)


/* Protocol types */
typedef CsrUint8 CmProtocolType;

#define CM_PROTOCOL_L2CAP    ((CmProtocolType)0x0)
#define CM_PROTOCOL_RFCOMM   ((CmProtocolType)0x1)

/* Application can use the below options to specify from where the device entry should get deleted. */
typedef CsrUint8 CmDeviceRemovalOption;
/* This is a default configuration where device will be deleted from all the places where it is maintained. */
#define CM_REMOVE_DEVICE_OPTION_DEFAULT                 ((CmDeviceRemovalOption) 0)
/* Device entry should will only get deleted from Bluestack SM DB */
#define CM_REMOVE_DEVICE_OPTION_SMDB_ONLY               ((CmDeviceRemovalOption) 1)

/*************************************************************************************
 Private configuration table entries
************************************************************************************/
#define CSR_BT_CM_AUTOPT_FALSE                                ((CsrUint16)0x0000)
#define CSR_BT_CM_AUTOPT_TRUE                                 ((CsrUint16)0x0001)
#define CSR_BT_CM_AUTOPT_AUTO_FCS_OFF_AMP                     ((CsrUint16)0x0081) /* 16 bit, exact, unused value */

/* LE Filter Accept List connection flag */
/* Note: This macro shall be deprecated, instead use CM_ACL_FLAG_LE_CONNECT_ACCEPTOR_LIST */
#define CM_ACL_FLAG_ULP_FILTER_ACCEPT_LIST                    ((CsrUint16) 0x0060) 

/* LE connection flags */
#define CM_ACL_FLAG_LE_CONNECT_AS_MASTER                      ((CsrUint16) DM_ACL_FLAG_ULP_CONNECT_AS_MASTER)
#define CM_ACL_FLAG_LE_CONNECT_ACCEPTOR_LIST                  ((CsrUint16) DM_ACL_FLAG_ULP_CONNECT_ACCEPTOR_LIST)

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/

/* (CsrBtCmPrim) is removed from those prims for which autogen of serializer/dissector is not applicable */

#define CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST                                (0x0000)

#define CSR_BT_CM_SET_LOCAL_NAME_REQ                          ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_NAME_REQ                        ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SCO_CONNECT_REQ                             ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SCO_RENEGOTIATE_REQ                         ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SCO_DISCONNECT_REQ                          ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_DELETE_STORE_LINK_KEY_REQ                ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_REMOVE_DEVICE_REQ                        ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_SET_SEC_MODE_REQ                         ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DATABASE_REQ                                ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_AUTHENTICATE_REQ                         ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_ENCRYPTION_REQ                           ((CsrBtCmPrim) (0x000A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ACL_CLOSE_REQ                               ((CsrBtCmPrim) (0x000B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_SET_DEFAULT_SEC_LEVEL_REQ                ((CsrBtCmPrim) (0x000C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_UNREGISTER_REQ                           ((CsrBtCmPrim) (0x000D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_REGISTER_REQ                             ((CsrBtCmPrim) (0x000E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ROLE_DISCOVERY_REQ                          ((CsrBtCmPrim) (0x000F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_BD_ADDR_REQ                      ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_NAME_REQ                         ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ENABLE_DUT_MODE_REQ                         ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_SCAN_ENABLE_REQ                       ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_SCAN_ENABLE_REQ                        ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGE_TO_REQ                           ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_TX_POWER_LEVEL_REQ                     ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_GET_LINK_QUALITY_REQ                        ((CsrBtCmPrim) (0x0017 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_RSSI_REQ                               ((CsrBtCmPrim) (0x0018 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_COD_REQ                               ((CsrBtCmPrim) (0x0019 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_COD_REQ                                ((CsrBtCmPrim) (0x001A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_EXT_FEATURES_REQ                ((CsrBtCmPrim) (0x001B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_AFH_CHANNEL_CLASS_REQ                   ((CsrBtCmPrim) (0x001C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_REQ        ((CsrBtCmPrim) (0x001D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_REQ       ((CsrBtCmPrim) (0x001E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_EXT_FEATURES_REQ                 ((CsrBtCmPrim) (0x001F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_AFH_CHANNEL_MAP_REQ                    ((CsrBtCmPrim) (0x0020 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_CLOCK_REQ                              ((CsrBtCmPrim) (0x0021 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_VERSION_REQ                      ((CsrBtCmPrim) (0x0022 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_FILTER_BDADDR_REQ                 ((CsrBtCmPrim) (0x0023 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_FILTER_COD_REQ                    ((CsrBtCmPrim) (0x0024 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CLEAR_EVENT_FILTER_REQ                      ((CsrBtCmPrim) (0x0025 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_IAC_REQ                                ((CsrBtCmPrim) (0x0026 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_IAC_REQ                               ((CsrBtCmPrim) (0x0027 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_WRITE_CACHE_PARAMS_REQ                   ((CsrBtCmPrim) (0x0028 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_UPDATE_AND_CLEAR_CACHED_PARAM_REQ        ((CsrBtCmPrim) (0x0029 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_ENCRYPTION_STATUS_REQ                  ((CsrBtCmPrim) (0x002A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_REQ                 ((CsrBtCmPrim) (0x002B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGESCAN_TYPE_REQ                     ((CsrBtCmPrim) (0x002C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_REQ              ((CsrBtCmPrim) (0x002D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_REQ                  ((CsrBtCmPrim) (0x002E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_MODE_SETTINGS_REQ                        ((CsrBtCmPrim) (0x002F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_L2CA_MODE_SETTINGS_REQ                   ((CsrBtCmPrim) (0x0030 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ                   ((CsrBtCmPrim) (0x0031 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_CHECK_SSR_REQ                            ((CsrBtCmPrim) (0x0032 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_BONDING_REQ                              ((CsrBtCmPrim) (0x0033 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_SEC_MODE_CONFIG_REQ                      ((CsrBtCmPrim) (0x0034 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_REQ                  ((CsrBtCmPrim) (0x0035 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_READ_DEVICE_REQ                          ((CsrBtCmPrim) (0x0036 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EN_ENABLE_ENHANCEMENTS_REQ                  ((CsrBtCmPrim) (0x0037 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ            ((CsrBtCmPrim) (0x0038 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_REQ             ((CsrBtCmPrim) (0x0039 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_FAILED_CONTACT_COUNTER_REQ             ((CsrBtCmPrim) (0x003A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_FEATURES_REQ                    ((CsrBtCmPrim) (0x003B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_VOICE_SETTINGS_REQ                    ((CsrBtCmPrim) (0x003C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_ACCESS_REQ                               ((CsrBtCmPrim) (0x003D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ALWAYS_MASTER_DEVICES_REQ                   ((CsrBtCmPrim) (0x003E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DISABLE_DUT_MODE_REQ                        ((CsrBtCmPrim) (0x003F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_LE_SECURITY_REQ                          ((CsrBtCmPrim) (0x0040 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_SET_ENCRYPTION_KEY_SIZE_REQ              ((CsrBtCmPrim) (0x0041 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_INCOMING_SCO_REQ                            ((CsrBtCmPrim) (0x0042 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_HCI_QOS_SETUP_REQ                        ((CsrBtCmPrim) (0x0043 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_REQ              ((CsrBtCmPrim) (0x0044 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ                  ((CsrBtCmPrim) (0x0045 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_POWER_SETTINGS_REQ                       ((CsrBtCmPrim) (0x0046 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REGISTER_REQ                           ((CsrBtCmPrim) (0x0047 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CONFIGURE_CIG_REQ                      ((CsrBtCmPrim) (0x0048 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REMOVE_CIG_REQ                         ((CsrBtCmPrim) (0x0049 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_CONNECT_REQ                        ((CsrBtCmPrim) (0x004A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_CONNECT_RSP                        ((CsrBtCmPrim) (0x004B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_DISCONNECT_REQ                     ((CsrBtCmPrim) (0x004C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_REQ                ((CsrBtCmPrim) (0x004D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_REQ               ((CsrBtCmPrim) (0x004E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CREATE_BIG_REQ                         ((CsrBtCmPrim) (0x004F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_TERMINATE_BIG_REQ                      ((CsrBtCmPrim) (0x0050 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_BIG_CREATE_SYNC_REQ                    ((CsrBtCmPrim) (0x0051 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_REQ                 ((CsrBtCmPrim) (0x0052 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ            ((CsrBtCmPrim) (0x0053 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ          ((CsrBtCmPrim) (0x0054 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ                      ((CsrBtCmPrim) (0x0055 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_DATA_REQ                        ((CsrBtCmPrim) (0x0056 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ              ((CsrBtCmPrim) (0x0057 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_ENABLE_REQ                          ((CsrBtCmPrim) (0x0058 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ           ((CsrBtCmPrim) (0x0059 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ              ((CsrBtCmPrim) (0x005A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ              ((CsrBtCmPrim) (0x005B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ               ((CsrBtCmPrim) (0x005C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_REQ             ((CsrBtCmPrim) (0x005D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ              ((CsrBtCmPrim) (0x005E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ                ((CsrBtCmPrim) (0x005F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ             ((CsrBtCmPrim) (0x0060 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_REQ                 ((CsrBtCmPrim) (0x0061 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_DATA_REQ                   ((CsrBtCmPrim) (0x0062 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ      ((CsrBtCmPrim) (0x0063 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_START_REQ                      ((CsrBtCmPrim) (0x0064 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_STOP_REQ                       ((CsrBtCmPrim) (0x0065 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_REQ               ((CsrBtCmPrim) (0x0066 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_REQ         ((CsrBtCmPrim) (0x0067 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ          ((CsrBtCmPrim) (0x0068 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ             ((CsrBtCmPrim) (0x0069 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_REQ      ((CsrBtCmPrim) (0x006A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ    ((CsrBtCmPrim) (0x006B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_REQ            ((CsrBtCmPrim) (0x006C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_RSP                 ((CsrBtCmPrim) (0x006D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_REQ             ((CsrBtCmPrim) (0x006E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ      ((CsrBtCmPrim) (0x006F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_REQ                   ((CsrBtCmPrim) (0x0070 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ                 ((CsrBtCmPrim) (0x0071 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SIRK_OPERATION_REQ                       ((CsrBtCmPrim) (0x0072 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ               ((CsrBtCmPrim) (0x0073 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SETS_INFO_REQ                       ((CsrBtCmPrim) (0x0074 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ                    ((CsrBtCmPrim) (0x0075 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ      ((CsrBtCmPrim) (0x0076 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ       ((CsrBtCmPrim) (0x0077 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_ENCRYPT_REQ                          ((CsrBtCmPrim) (0x0078 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_HASH_REQ                             ((CsrBtCmPrim) (0x0079 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_DECRYPT_REQ                          ((CsrBtCmPrim) (0x007A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_AES_CTR_REQ                          ((CsrBtCmPrim) (0x007B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_REQ                 ((CsrBtCmPrim) (0x007C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ     ((CsrBtCmPrim) (0x007D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ                 ((CsrBtCmPrim) (0x007E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ      ((CsrBtCmPrim) (0x007F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CREATE_BIG_TEST_REQ                    ((CsrBtCmPrim) (0x0080 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_ENABLE_REQ                     ((CsrBtCmPrim) (0x0081 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_REQ                  ((CsrBtCmPrim) (0x0082 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SUBRATE_CHANGE_REQ                       ((CsrBtCmPrim) (0x0083 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_EIR_DATA_REQ                            ((CsrBtCmPrim) (0x0084 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_SET_PHY_REQ                                  ((CsrBtCmPrim) (0x0085 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_SET_DEFAULT_PHY_REQ                          ((CsrBtCmPrim) (0x0086 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_CHANGE_CONNECTION_LINK_KEY_REQ                  ((CsrBtCmPrim) (0x0087 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_ISOC_READ_ISO_LINK_QUALITY_REQ                     ((CsrBtCmPrim) (0x0088 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_INQUIRY_MODE_REQ                           ((CsrBtCmPrim) (0x0089 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_INQUIRY_TX_REQ                             ((CsrBtCmPrim) (0x008A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_REQ                   ((CsrBtCmPrim) (0x008B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_EIR_DATA_REQ                               ((CsrBtCmPrim) (0x008C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ         ((CsrBtCmPrim) (0x008D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_REQ      ((CsrBtCmPrim) (0x008E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_REQ       ((CsrBtCmPrim) (0x008F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_WRITE_INQUIRY_MODE_REQ                          ((CsrBtCmPrim) (0x0090 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_EXT_ADV_SET_PARAMS_V2_REQ                       ((CsrBtCmPrim) (0x0091 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_EXT_ADV_GET_ADDR_REQ                            ((CsrBtCmPrim) (0x0092 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_DU_AUTO_EVENT_REQ                               ((CsrBtCmPrim) (0x0093 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_READ_PHY_REQ                                 ((CsrBtCmPrim) (0x0094 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_REMOVE_DEVICE_KEY_REQ                           ((CsrBtCmPrim) (0x0095 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_REMOVE_DEVICE_OPTIONS_REQ                       ((CsrBtCmPrim) (0x0096 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_BNEP_SWITCH_ROLE_REQ                            ((CsrBtCmPrim) (0x0097 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_WRITE_LINK_POLICY_REQ                           ((CsrBtCmPrim) (0x0098 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_LINK_POLICY_REQ                            ((CsrBtCmPrim) (0x0099 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_SWITCH_ROLE_REQ                                 ((CsrBtCmPrim) (0x009A + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_EXT_SET_CONN_PARAMS_REQ                         ((CsrBtCmPrim) (0x009B + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_HCI_CONFIGURE_DATA_PATH_REQ                     ((CsrBtCmPrim) (0x009C + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_READ_CHANNEL_MAP_REQ                         ((CsrBtCmPrim) (0x009D + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ       ((CsrBtCmPrim) (0x009E + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_REQ           ((CsrBtCmPrim) (0x009F + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_READ_CONN_ACCEPT_TIMEOUT_REQ                    ((CsrBtCmPrim) (0x00A0 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_REQ                   ((CsrBtCmPrim) (0x00A1 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))
#define CM_DM_SNIFF_SUB_RATE_REQ                              ((CsrBtCmPrim) (0x00A2 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST))

#define CSR_BT_CM_DM_PRIM_DOWNSTREAM_HIGHEST                                 (0x00A2 + CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST                                  (0x0100)

#define CSR_BT_CM_REGISTER_REQ                                ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CONNECT_REQ                                 ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CONNECT_ACCEPT_REQ                          ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_ACCEPT_CONNECT_REQ                   ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DISCONNECT_REQ                              ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ACCEPT_CONNECT_TIMEOUT                      ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_RFC_MODE_CHANGE_REQ                         ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CONNECT_EXT_REQ                             ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_REGISTER_REQ                           ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_REQ                            ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ                     ((CsrBtCmPrim) (0x000A + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_REQ              ((CsrBtCmPrim) (0x000B + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DISCONNECT_REQ                         ((CsrBtCmPrim) (0x000C + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_MODE_CHANGE_REQ                        ((CsrBtCmPrim) (0x000D + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_REGISTER_REQ                           ((CsrBtCmPrim) (0x000E + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_CONNECT_REQ                            ((CsrBtCmPrim) (0x000F + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_CONNECT_ACCEPT_REQ                     ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_REQ              ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_MODE_CHANGE_REQ                        ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_DISCONNECT_REQ                         ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDS_REGISTER_REQ                            ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDS_UNREGISTER_REQ                          ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_MODE_CHANGE_REQ                             ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ                        ((CsrBtCmPrim) (0x0017 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_PHYSICAL_LINK_STATUS_REQ                 ((CsrBtCmPrim) (0x0018 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_TP_CONNECT_REQ                                ((CsrBtCmPrim) (0x0019 + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_REGISTER_FIXED_CID_REQ                        ((CsrBtCmPrim) (0x001A + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST))

#define CSR_BT_CM_SM_PRIM_DOWNSTREAM_HIGHEST                                 (0x001A + CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST                                     (0x0200)

#define CSR_BT_CM_INQUIRY_REQ                                 ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_INQUIRY_REQ                          ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SCO_ACCEPT_CONNECT_REQ                      ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SCO_CANCEL_ACCEPT_CONNECT_REQ               ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_REQ                               ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_AUTHORISE_RES                            ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_PIN_REQUEST_RES                          ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_UNREGISTER_REQ                              ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DATA_REQ                                    ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DATA_RES                                    ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CONTROL_REQ                                 ((CsrBtCmPrim) (0x000A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PORTNEG_RES                                 ((CsrBtCmPrim) (0x000B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_EXTENDED_DATA_REQ                      ((CsrBtCmPrim) (0x000C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ            ((CsrBtCmPrim) (0x000D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_DISCONNECT_RES                         ((CsrBtCmPrim) (0x000E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_CANCEL_SEARCH_REQ                       ((CsrBtCmPrim) (0x000F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_ATTRIBUTE_REQ                           ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_CLOSE_REQ                               ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_CONNECT_REQ                          ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_CANCEL_CONNECT_REQ                       ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_L2CA_CONNECT_REQ                     ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ALWAYS_SUPPORT_MASTER_ROLE_REQ              ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_READ_REMOTE_NAME_REQ                 ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_BONDING_CANCEL_REQ                       ((CsrBtCmPrim) (0x0017 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_RES                ((CsrBtCmPrim) (0x0018 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_NEG_RES            ((CsrBtCmPrim) (0x0019 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_RES            ((CsrBtCmPrim) (0x001A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_NEG_RES        ((CsrBtCmPrim) (0x001B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_PASSKEY_REQUEST_RES                 ((CsrBtCmPrim) (0x001C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_PASSKEY_REQUEST_NEG_RES             ((CsrBtCmPrim) (0x001D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_SEND_KEYPRESS_NOTIFICATION_REQ           ((CsrBtCmPrim) (0x001E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_REPAIR_RES                               ((CsrBtCmPrim) (0x001F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_CANCEL_BNEP_CONNECT_REQ                     ((CsrBtCmPrim) (0x0020 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ROLE_SWITCH_CONFIG_REQ                      ((CsrBtCmPrim) (0x0021 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_MASK_REQ                          ((CsrBtCmPrim) (0x0022 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_MODE_CHANGE_CONFIG_REQ                      ((CsrBtCmPrim) (0x0023 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_UNREGISTER_REQ                         ((CsrBtCmPrim) (0x0024 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_REQ               ((CsrBtCmPrim) (0x0025 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_MOVE_CHANNEL_REQ                            ((CsrBtCmPrim) (0x0026 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_MOVE_CHANNEL_RES                            ((CsrBtCmPrim) (0x0027 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_REQ                ((CsrBtCmPrim) (0x0028 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LOGICAL_CHANNEL_TYPE_REQ                    ((CsrBtCmPrim) (0x0029 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_PORTNEG_REQ                                 ((CsrBtCmPrim) (0x002A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_VERSION_REQ                     ((CsrBtCmPrim) (0x002B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_REGISTER_HANDLER_REQ                        ((CsrBtCmPrim) (0x002C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_RES                               ((CsrBtCmPrim) (0x002D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_ABORT_REQ                         ((CsrBtCmPrim) (0x002E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_A2DP_BIT_RATE_REQ                           ((CsrBtCmPrim) (0x002F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_GET_SECURITY_CONF_RES                       ((CsrBtCmPrim) (0x0030 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_DATA_BUFFER_EMPTY_REQ                       ((CsrBtCmPrim) (0x0031 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SCAN_REQ                                 ((CsrBtCmPrim) (0x0032 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_ADVERTISE_REQ                            ((CsrBtCmPrim) (0x0033 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_WHITELIST_SET_REQ                        ((CsrBtCmPrim) (0x0034 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_CONNPARAM_REQ                            ((CsrBtCmPrim) (0x0035 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_CONNPARAM_UPDATE_REQ                     ((CsrBtCmPrim) (0x0036 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES              ((CsrBtCmPrim) (0x0037 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_EIR_FLAGS_REQ                               ((CsrBtCmPrim) (0x0038 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_ENCRYPTION_KEY_SIZE_REQ                ((CsrBtCmPrim) (0x0039 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ            ((CsrBtCmPrim) (0x003A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_RECEIVER_TEST_REQ                        ((CsrBtCmPrim) (0x003B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_TRANSMITTER_TEST_REQ                     ((CsrBtCmPrim) (0x003C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_TEST_END_REQ                             ((CsrBtCmPrim) (0x003D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_UNLOCK_SM_QUEUE_REQ                      ((CsrBtCmPrim) (0x003E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_GET_CONTROLLER_INFO_REQ                  ((CsrBtCmPrim) (0x003F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_MAP_SCO_PCM_RES                             ((CsrBtCmPrim) (0x0040 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_GET_CHANNEL_INFO_REQ                   ((CsrBtCmPrim) (0x0041 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SET_AV_STREAM_INFO_REQ                      ((CsrBtCmPrim) (0x0042 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_REQ            ((CsrBtCmPrim) (0x0043 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_REQ        ((CsrBtCmPrim) (0x0044 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_REQ             ((CsrBtCmPrim) (0x0045 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_PRIVACY_MODE_REQ                     ((CsrBtCmPrim) (0x0046 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ                 ((CsrBtCmPrim) (0x0047 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ                   ((CsrBtCmPrim) (0x0048 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ                 ((CsrBtCmPrim) (0x0049 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_ADD_DEVICE_REQ                           ((CsrBtCmPrim) (0x004A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_LOCAL_IRK_REQ                       ((CsrBtCmPrim) (0x004B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_UPDATE_INTERNAL_PEER_ADDR_REQ               ((CsrBtCmPrim) (0x004C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_ACL_OPEN_REQ                                ((CsrBtCmPrim) (0x004D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_RFC_DISCONNECT_RSP                          ((CsrBtCmPrim) (0x004E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DISCONNECT_RSP                         ((CsrBtCmPrim) (0x004F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_RFC_CONNECT_ACCEPT_RSP                      ((CsrBtCmPrim) (0x0050 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_ACCEPT_RSP                     ((CsrBtCmPrim) (0x0051 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_TP_CONNECT_ACCEPT_RSP                         ((CsrBtCmPrim) (0x0052 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_ADD_CREDIT_REQ                                ((CsrBtCmPrim) (0x0053 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_SM_REFRESH_ENCRYPTION_KEY_REQ                      ((CsrBtCmPrim) (0x0054 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP            ((CsrBtCmPrim) (0x0055 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_EXT_SCAN_FILTERED_ADV_REPORT_DONE_IND              ((CsrBtCmPrim) (0x0056 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_LE_ADD_DEVICE_TO_WHITE_LIST_REQ                    ((CsrBtCmPrim) (0x0057 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_REQ               ((CsrBtCmPrim) (0x0058 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND             ((CsrBtCmPrim) (0x0059 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_SM_KEY_REQUEST_RSP                                 ((CsrBtCmPrim) (0x005A + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_PING_REQ                                      ((CsrBtCmPrim) (0x005B + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_L2CA_PING_RSP                                      ((CsrBtCmPrim) (0x005C + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_LE_ENHANCED_RECEIVER_TEST_REQ                      ((CsrBtCmPrim) (0x005D + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_LE_ENHANCED_TRANSMITTER_TEST_REQ                   ((CsrBtCmPrim) (0x005E + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CM_SM_CONFIG_REQ                                      ((CsrBtCmPrim) (0x005F + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SM_HOUSE_CLEANING                           ((CsrBtCmPrim) (0x0060 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)) /* must be last */
#define CSR_BT_CM_DM_HOUSE_CLEANING                           ((CsrBtCmPrim) (0x0061 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)) /* must be last */
#define CM_SDC_HOUSE_CLEANING                                 ((CsrBtCmPrim) (0x0062 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)) /* must be last */

#define CSR_BT_CM_PRIM_DOWNSTREAM_HIGHEST                                    (0x0062 + CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)

/* **** */

#define CM_SDC_PRIM_DOWNSTREAM_LOWEST                                        (0x0300)

#define CSR_BT_CM_SDC_SEARCH_REQ                              ((CsrBtCmPrim) (0x0000 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_SERVICE_SEARCH_REQ                      ((CsrBtCmPrim) (0x0001 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_UUID128_SEARCH_REQ                      ((CsrBtCmPrim) (0x0002 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_OPEN_REQ                                ((CsrBtCmPrim) (0x0003 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_RFC_SEARCH_REQ                          ((CsrBtCmPrim) (0x0004 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ                  ((CsrBtCmPrim) (0x0005 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ                 ((CsrBtCmPrim) (0x0006 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_CM_SDC_RELEASE_RESOURCES_REQ                   ((CsrBtCmPrim) (0x0007 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))
#define CM_SDC_SERVICE_SEARCH_ATTR_REQ                        ((CsrBtCmPrim) (0x0008 + CM_SDC_PRIM_DOWNSTREAM_LOWEST))

#define CM_SDC_PRIM_DOWNSTREAM_HIGHEST                                       (0x0008 + CM_SDC_PRIM_DOWNSTREAM_LOWEST)

/*******************************************************************************/

#define CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST                                 (0x0000 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM                   ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CONNECT_CFM                                 ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CONNECT_ACCEPT_CFM                          ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_REGISTER_CFM                                ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DISCONNECT_IND                              ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SCO_CONNECT_CFM                             ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SCO_DISCONNECT_IND                          ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM                      ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DATA_IND                                    ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DATA_CFM                                    ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CONTROL_IND                                 ((CsrBtCmPrim) (0x000A + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_RFC_MODE_CHANGE_IND                         ((CsrBtCmPrim) (0x000B + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PORTNEG_IND                                 ((CsrBtCmPrim) (0x000C + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PORTNEG_CFM                                 ((CsrBtCmPrim) (0x000D + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_RFC_CONNECT_ACCEPT_IND                      ((CsrBtCmPrim) (0x000E + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_RFC_PRIM_UPSTREAM_HIGHEST                                (0x000E + CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST                                 (0x0100 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_SDC_SEARCH_IND                              ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_SEARCH_CFM                              ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_SERVICE_SEARCH_CFM                      ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_OPEN_CFM                                ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_ATTRIBUTE_CFM                           ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_CLOSE_IND                               ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM                   ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDS_REGISTER_CFM                            ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDS_UNREGISTER_CFM                          ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CM_SDC_SERVICE_SEARCH_ATTR_IND                        ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CM_SDC_SERVICE_SEARCH_ATTR_CFM                        ((CsrBtCmPrim) (0x000A + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_SDC_PRIM_UPSTREAM_HIGHEST                                  (0x000A + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_CM_SDS_EXT_REGISTER_CFM                        ((CsrBtCmPrim) (0x000B + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDS_EXT_UNREGISTER_CFM                      ((CsrBtCmPrim) (0x000C + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_SDC_PRIM_EXT_UPSTREAM_HIGHEST                              (0x000C + CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST)


/* **** */

#define CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST                                 (0x0200 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_L2CA_REGISTER_CFM                           ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_CFM                            ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM                     ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CANCEL_CONNECT_ACCEPT_CFM              ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_CFM                               ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_IND                               ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DISCONNECT_IND                         ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_MODE_CHANGE_IND                        ((CsrBtCmPrim) (0x000A + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_DATA_ABORT_CFM                         ((CsrBtCmPrim) (0x000B + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_GET_CHANNEL_INFO_CFM                   ((CsrBtCmPrim) (0x000C + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_AMP_MOVE_IND                           ((CsrBtCmPrim) (0x000D + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND                     ((CsrBtCmPrim) (0x000E + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_TP_CONNECT_CFM                                ((CsrBtCmPrim) (0x000F + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_TP_CONNECT_ACCEPT_IND                         ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_TP_CONNECT_ACCEPT_CFM                         ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_ADD_CREDIT_CFM                                ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_UNREGISTER_CFM                                ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_PING_CFM                                      ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_PING_IND                                      ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))
#define CM_L2CA_REGISTER_FIXED_CID_CFM                        ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_L2CA_PRIM_UPSTREAM_HIGHEST                                 (0x0016 + CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST)


/* **** */

#define CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST                                  (0x0300 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_BNEP_CONNECT_IND                            ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_CONNECT_ACCEPT_CFM                     ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_CANCEL_CONNECT_ACCEPT_CFM              ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_EXTENDED_DATA_IND                      ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_MODE_CHANGE_IND                        ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_SWITCH_ROLE_IND                        ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_DISCONNECT_IND                         ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BNEP_EXTENDED_DATA_CFM                      ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_BNEP_PRIM_UPSTREAM_HIGHEST                                 (0x0007 + CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST                             (0x0400 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_INQUIRY_RESULT_IND                          ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_INQUIRY_CFM                                 ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_NAME_CFM                        ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGE_TO_CFM                           ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_HIGHEST                              (0x0003 + CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST                                    (0x0500 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_SET_LOCAL_NAME_CFM                          ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM                      ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM               ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_NAME_CFM                         ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_TX_POWER_LEVEL_CFM                     ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_GET_LINK_QUALITY_CFM                        ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_RSSI_CFM                               ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_COD_CFM                               ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_COD_CFM                                ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_VERSION_CFM                      ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_VERSION_CFM                     ((CsrBtCmPrim) (0x000A + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_SCAN_ENABLE_CFM                        ((CsrBtCmPrim) (0x000B + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_SCAN_ENABLE_CFM                       ((CsrBtCmPrim) (0x000C + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM                   ((CsrBtCmPrim) (0x000D + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_PHY_SET_CFM                                  ((CsrBtCmPrim) (0x000E + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_DEFAULT_PHY_SET_CFM                          ((CsrBtCmPrim) (0x000F + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_PHY_UPDATE_IND                               ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_CFM                    ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_CHANGE_CONNECTION_LINK_KEY_CFM                  ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_READ_INQUIRY_MODE_CFM                           ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_READ_INQUIRY_TX_CFM                             ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_CFM                   ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_READ_EIR_DATA_CFM                               ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM         ((CsrBtCmPrim) (0x0017 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM      ((CsrBtCmPrim) (0x0018 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_TRANSMIT_POWER_REPORTING_IND                 ((CsrBtCmPrim) (0x0019 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM       ((CsrBtCmPrim) (0x001A + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_WRITE_INQUIRY_MODE_CFM                          ((CsrBtCmPrim) (0x001B + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_PHY_READ_CFM                                 ((CsrBtCmPrim) (0x001C + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_REMOVE_DEVICE_KEY_CFM                           ((CsrBtCmPrim) (0x001D + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_REMOVE_DEVICE_OPTIONS_CFM                       ((CsrBtCmPrim) (0x001E + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_EXT_SET_CONN_PARAMS_CFM                         ((CsrBtCmPrim) (0x001F + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_HCI_CONFIGURE_DATA_PATH_CFM                     ((CsrBtCmPrim) (0x0020 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_READ_CHANNEL_MAP_CFM                         ((CsrBtCmPrim) (0x0021 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_CFM       ((CsrBtCmPrim) (0x0022 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_CFM           ((CsrBtCmPrim) (0x0023 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_LE_PATH_LOSS_THRESHOLD_IND                      ((CsrBtCmPrim) (0x0024 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_EXT_SCAN_TIMEOUT_IND                            ((CsrBtCmPrim) (0x0025 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_READ_CONN_ACCEPT_TIMEOUT_CFM                    ((CsrBtCmPrim) (0x0026 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_CFM                   ((CsrBtCmPrim) (0x0027 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))
#define CM_DM_SNIFF_SUB_RATE_CFM                              ((CsrBtCmPrim) (0x0028 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_DM_PRIM_UPSTREAM_HIGHEST                                   (0x0028 + CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST                                (0x0600 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM                ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SET_AFH_CHANNEL_CLASS_CFM                   ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_AFH_CHANNEL_ASSESSMENT_MODE_CFM        ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_CFM       ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_LOCAL_EXT_FEATURES_CFM                 ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_AFH_CHANNEL_MAP_CFM                    ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_CLOCK_CFM                              ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_HIGHEST                               (0x0006 + CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST)

/* **** */

#define CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST                                  (0x0700 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_ENABLE_DUT_MODE_CFM                         ((CsrBtCmPrim) (0x0000 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_REJECT_RFC_CONNECTION_IND                   ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SDC_UUID128_SEARCH_IND                      ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ROLE_DISCOVERY_CFM                          ((CsrBtCmPrim) (0x0003 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_REMOTE_NAME_IND                        ((CsrBtCmPrim) (0x0004 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_LINK_POLICY_ERROR_IND                 ((CsrBtCmPrim) (0x0005 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_LINK_POLICY_CFM                        ((CsrBtCmPrim) (0x0006 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CONNECTION_REJ_SECURITY_IND                 ((CsrBtCmPrim) (0x0007 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SCO_RENEGOTIATE_CFM                         ((CsrBtCmPrim) (0x0008 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SCO_RENEGOTIATE_IND                         ((CsrBtCmPrim) (0x0009 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_FILTER_BDADDR_CFM                 ((CsrBtCmPrim) (0x000C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_FILTER_COD_CFM                    ((CsrBtCmPrim) (0x000D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CLEAR_EVENT_FILTER_CFM                      ((CsrBtCmPrim) (0x000E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_IND                ((CsrBtCmPrim) (0x000F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_IAC_CFM                                ((CsrBtCmPrim) (0x0010 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_IAC_IND                               ((CsrBtCmPrim) (0x0011 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_ENCRYPTION_STATUS_CFM                  ((CsrBtCmPrim) (0x0012 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM                 ((CsrBtCmPrim) (0x0013 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM                     ((CsrBtCmPrim) (0x0014 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM              ((CsrBtCmPrim) (0x0015 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_INQUIRYSCAN_TYPE_CFM                  ((CsrBtCmPrim) (0x0016 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MAP_SCO_PCM_IND                             ((CsrBtCmPrim) (0x0017 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_CFM            ((CsrBtCmPrim) (0x0019 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_REPAIR_IND                               ((CsrBtCmPrim) (0x001A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DATABASE_CFM                                ((CsrBtCmPrim) (0x001B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_FAILED_CONTACT_COUNTER_CFM             ((CsrBtCmPrim) (0x001C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SET_EVENT_MASK_CFM                          ((CsrBtCmPrim) (0x001D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SYNC_CONNECT_IND                            ((CsrBtCmPrim) (0x001E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SYNC_DISCONNECT_IND                         ((CsrBtCmPrim) (0x001F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SYNC_RENEGOTIATE_IND                        ((CsrBtCmPrim) (0x0020 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ACL_CONNECT_IND                             ((CsrBtCmPrim) (0x0021 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ACL_DISCONNECT_IND                          ((CsrBtCmPrim) (0x0022 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SNIFF_SUB_RATING_IND                        ((CsrBtCmPrim) (0x0023 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MODE_CHANGE_IND                             ((CsrBtCmPrim) (0x0024 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ROLE_CHANGE_IND                             ((CsrBtCmPrim) (0x0025 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LSTO_CHANGE_IND                             ((CsrBtCmPrim) (0x0026 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BLUECORE_INITIALIZED_IND                    ((CsrBtCmPrim) (0x0027 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MODE_CHANGE_CONFIG_CFM                      ((CsrBtCmPrim) (0x0028 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MODE_CHANGE_CFM                             ((CsrBtCmPrim) (0x0029 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SWITCH_ROLE_CFM                             ((CsrBtCmPrim) (0x002A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_VOICE_SETTINGS_CFM                    ((CsrBtCmPrim) (0x002B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MOVE_CHANNEL_CFM                            ((CsrBtCmPrim) (0x002C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MOVE_CHANNEL_IND                            ((CsrBtCmPrim) (0x002D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_MOVE_CHANNEL_CMP_IND                        ((CsrBtCmPrim) (0x002E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_CFM                ((CsrBtCmPrim) (0x002F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LOGICAL_CHANNEL_TYPES_IND                   ((CsrBtCmPrim) (0x0030 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SYNC_CONNECT_IND                        ((CsrBtCmPrim) (0x0031 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_REMOTE_FEATURES_IND                         ((CsrBtCmPrim) (0x0032 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_REMOTE_VERSION_IND                          ((CsrBtCmPrim) (0x0033 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_A2DP_BIT_RATE_IND                           ((CsrBtCmPrim) (0x0034 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_INQUIRY_PAGE_EVENT_IND                      ((CsrBtCmPrim) (0x0035 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_GET_SECURITY_CONF_IND                       ((CsrBtCmPrim) (0x0036 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ENCRYPT_CHANGE_IND                          ((CsrBtCmPrim) (0x0037 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ALWAYS_MASTER_DEVICES_CFM                   ((CsrBtCmPrim) (0x0038 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DATA_BUFFER_EMPTY_CFM                       ((CsrBtCmPrim) (0x0039 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_DISABLE_DUT_MODE_CFM                        ((CsrBtCmPrim) (0x003A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SCAN_CFM                                 ((CsrBtCmPrim) (0x003B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_ADVERTISE_CFM                            ((CsrBtCmPrim) (0x003C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_REPORT_IND                               ((CsrBtCmPrim) (0x003D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LOCAL_NAME_CHANGE_IND                       ((CsrBtCmPrim) (0x003E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_EVENT_ADVERTISING_IND                    ((CsrBtCmPrim) (0x003F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_EVENT_SCAN_IND                           ((CsrBtCmPrim) (0x0040 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_EVENT_CONNECTION_IND                     ((CsrBtCmPrim) (0x0041 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EIR_FLAGS_CFM                               ((CsrBtCmPrim) (0x0042 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_WHITELIST_SET_CFM                        ((CsrBtCmPrim) (0x0043 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_CONNPARAM_CFM                            ((CsrBtCmPrim) (0x0044 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND                 ((CsrBtCmPrim) (0x0045 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_ENCRYPTION_KEY_SIZE_CFM                ((CsrBtCmPrim) (0x0046 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_CFM            ((CsrBtCmPrim) (0x0047 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_RECEIVER_TEST_CFM                        ((CsrBtCmPrim) (0x0048 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_TRANSMITTER_TEST_CFM                     ((CsrBtCmPrim) (0x0049 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_TEST_END_CFM                             ((CsrBtCmPrim) (0x004A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_IND              ((CsrBtCmPrim) (0x004B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_HIGH_PRIORITY_DATA_IND                      ((CsrBtCmPrim) (0x004C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_LOCK_SM_QUEUE_IND                        ((CsrBtCmPrim) (0x004D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_GET_CONTROLLER_INFO_CFM                  ((CsrBtCmPrim) (0x004E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BLUECORE_DEINITIALIZED_IND                  ((CsrBtCmPrim) (0x004F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_CFM            ((CsrBtCmPrim) (0x0050 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM        ((CsrBtCmPrim) (0x0051 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_WRITE_AUTH_PAYLOAD_TIMEOUT_CFM              ((CsrBtCmPrim) (0x0052 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM                  ((CsrBtCmPrim) (0x0053 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_CFM             ((CsrBtCmPrim) (0x0054 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_PRIVACY_MODE_CFM                     ((CsrBtCmPrim) (0x0055 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM                 ((CsrBtCmPrim) (0x0056 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_OWN_ADDRESS_TYPE_CHANGED_IND             ((CsrBtCmPrim) (0x0057 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_STATIC_ADDRESS_CFM                   ((CsrBtCmPrim) (0x0058 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM                 ((CsrBtCmPrim) (0x0059 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_READ_LOCAL_IRK_CFM                       ((CsrBtCmPrim) (0x005A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ACL_OPEN_CFM                                ((CsrBtCmPrim) (0x005B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ACL_CLOSE_CFM                               ((CsrBtCmPrim) (0x005C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REGISTER_CFM                           ((CsrBtCmPrim) (0x005D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CONFIGURE_CIG_CFM                      ((CsrBtCmPrim) (0x005E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REMOVE_CIG_CFM                         ((CsrBtCmPrim) (0x005F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_CONNECT_IND                        ((CsrBtCmPrim) (0x0060 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_CONNECT_CFM                        ((CsrBtCmPrim) (0x0061 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM                     ((CsrBtCmPrim) (0x0062 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CIS_DISCONNECT_IND                     ((CsrBtCmPrim) (0x0063 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM                ((CsrBtCmPrim) (0x0064 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM               ((CsrBtCmPrim) (0x0065 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CREATE_BIG_CFM                         ((CsrBtCmPrim) (0x0066 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_TERMINATE_BIG_CFM                      ((CsrBtCmPrim) (0x0067 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_BIG_CREATE_SYNC_CFM                    ((CsrBtCmPrim) (0x0068 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_IND                 ((CsrBtCmPrim) (0x0069 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM            ((CsrBtCmPrim) (0x006A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM          ((CsrBtCmPrim) (0x006B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM                      ((CsrBtCmPrim) (0x006C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_DATA_CFM                        ((CsrBtCmPrim) (0x006D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM              ((CsrBtCmPrim) (0x006E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_ENABLE_CFM                          ((CsrBtCmPrim) (0x006F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM           ((CsrBtCmPrim) (0x0070 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM              ((CsrBtCmPrim) (0x0071 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM              ((CsrBtCmPrim) (0x0072 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM               ((CsrBtCmPrim) (0x0073 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_CFM             ((CsrBtCmPrim) (0x0074 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM              ((CsrBtCmPrim) (0x0075 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM                ((CsrBtCmPrim) (0x0076 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM             ((CsrBtCmPrim) (0x0077 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_CTRL_SCAN_INFO_IND                 ((CsrBtCmPrim) (0x0078 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND            ((CsrBtCmPrim) (0x0079 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM                 ((CsrBtCmPrim) (0x007A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM                   ((CsrBtCmPrim) (0x007B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM      ((CsrBtCmPrim) (0x007C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_START_CFM                      ((CsrBtCmPrim) (0x007D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_STOP_CFM                       ((CsrBtCmPrim) (0x007E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM               ((CsrBtCmPrim) (0x007F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM         ((CsrBtCmPrim) (0x0080 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM          ((CsrBtCmPrim) (0x0081 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM             ((CsrBtCmPrim) (0x0082 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM      ((CsrBtCmPrim) (0x0083 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM    ((CsrBtCmPrim) (0x0084 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM            ((CsrBtCmPrim) (0x0085 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND           ((CsrBtCmPrim) (0x0086 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND                 ((CsrBtCmPrim) (0x0087 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM             ((CsrBtCmPrim) (0x0088 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND             ((CsrBtCmPrim) (0x0089 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM      ((CsrBtCmPrim) (0x008A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND                  ((CsrBtCmPrim) (0x008B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SET_EIR_DATA_CFM                            ((CsrBtCmPrim) (0x008C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_TERMINATED_IND                      ((CsrBtCmPrim) (0x008D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_SCAN_DURATION_EXPIRED_IND               ((CsrBtCmPrim) (0x008E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM                 ((CsrBtCmPrim) (0x008F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SIRK_OPERATION_CFM                       ((CsrBtCmPrim) (0x0090 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM               ((CsrBtCmPrim) (0x0091 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_SETS_INFO_CFM                       ((CsrBtCmPrim) (0x0092 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM                    ((CsrBtCmPrim) (0x0093 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM      ((CsrBtCmPrim) (0x0094 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM       ((CsrBtCmPrim) (0x0095 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_ENCRYPT_CFM                          ((CsrBtCmPrim) (0x0096 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_HASH_CFM                             ((CsrBtCmPrim) (0x0097 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_DECRYPT_CFM                          ((CsrBtCmPrim) (0x0098 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_CRYPTO_AES_CTR_CFM                          ((CsrBtCmPrim) (0x0099 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_CFM                 ((CsrBtCmPrim) (0x009A + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_CFM     ((CsrBtCmPrim) (0x009B + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND   ((CsrBtCmPrim) (0x009C + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM                 ((CsrBtCmPrim) (0x009D + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM      ((CsrBtCmPrim) (0x009E + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_ISOC_CREATE_BIG_TEST_CFM                    ((CsrBtCmPrim) (0x009F + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PERIODIC_ADV_ENABLE_CFM                     ((CsrBtCmPrim) (0x00A0 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_CFM                  ((CsrBtCmPrim) (0x00A1 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SUBRATE_CHANGE_CFM                       ((CsrBtCmPrim) (0x00A2 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_LE_SUBRATE_CHANGE_IND                       ((CsrBtCmPrim) (0x00A3 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_ISOC_READ_ISO_LINK_QUALITY_CFM                     ((CsrBtCmPrim) (0x00A4 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_LE_ADD_DEVICE_TO_WHITE_LIST_CFM                    ((CsrBtCmPrim) (0x00A5 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM               ((CsrBtCmPrim) (0x00A6 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_DM_EXT_ADV_SET_PARAMS_V2_CFM                       ((CsrBtCmPrim) (0x00A7 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_DM_EXT_ADV_GET_ADDR_CFM                            ((CsrBtCmPrim) (0x00A8 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_ISOC_NOTIFY_CIS_CONNECT_IND                        ((CsrBtCmPrim) (0x00A9 + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_LE_ENHANCED_RECEIVER_TEST_CFM                      ((CsrBtCmPrim) (0x00AA + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_LE_ENHANCED_TRANSMITTER_TEST_CFM                   ((CsrBtCmPrim) (0x00AB + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))
#define CM_SM_CONFIG_CFM                                      ((CsrBtCmPrim) (0x00AC + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_SEND_PRIM_UPSTREAM_HIGHEST                                 (0x00AC + CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST)


#define CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST                                    (0x0800 + CSR_PRIM_UPSTREAM)
/* Serializers/dissectors not to be generated for these prims */
#define CSR_BT_CM_SM_PIN_REQUEST_IND                          ((0x0000 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_IO_CAPABILITY_RESPONSE_IND               ((0x0001 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND                ((0x0002 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND            ((0x0003 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND                 ((0x0004 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_USER_PASSKEY_NOTIFICATION_IND            ((0x0005 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_KEYPRESS_NOTIFICATION_IND                ((0x0006 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_SIMPLE_PAIRING_COMPLETE_IND              ((0x0007 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_AUTHENTICATE_CFM                         ((0x0008 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_SECURITY_IND                             ((0x0009 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_CSRK_COUNTER_CHANGE_IND                  ((0x000A + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_CFM                  ((0x000B + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_LOCAL_KEY_DELETED_IND                    ((0x000C + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_SECURITY_CFM                             ((0x000D + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_KEYS_IND                                 ((0x000E + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_BONDING_CFM                              ((0x000F + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_ENCRYPTION_CHANGE_IND                    ((0x0010 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_ENCRYPT_CFM                              ((0x0011 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_ACCESS_IND                               ((0x0012 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_AUTHORISE_IND                            ((0x0013 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_INIT_CFM                                 ((0x0014 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_REMOVE_DEVICE_CFM                        ((0x0015 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SM_ADD_DEVICE_CFM                           ((0x0016 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CM_SM_GENERATE_CROSS_TRANS_KEY_IND                    ((0x0017 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))
#define CM_SM_KEY_REQUEST_IND                                 ((0x0018 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_SM_PRIM_UPSTREAM_HIGHEST                    (0x0018 + CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST                    (0x0900 + CSR_PRIM_UPSTREAM)

#define CSR_BT_CM_HCI_CREATE_CONNECTION_CANCEL_CFM            ((0x0000 + CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM              ((0x0001 + CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_HCI_REFRESH_ENCRYPTION_KEY_IND              ((0x0002 + CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_CM_HCI_PRIM_UPSTREAM_HIGHEST                   (0x0002 + CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_CM_PRIM_UPSTREAM_HIGHEST                       CSR_BT_CM_HCI_PRIM_UPSTREAM_HIGHEST

#define CSR_BT_CM_DM_PRIM_DOWNSTREAM_COUNT                    (CSR_BT_CM_DM_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_CM_DM_PRIM_DOWNSTREAM_LOWEST)
#define CSR_BT_CM_SM_PRIM_DOWNSTREAM_COUNT                    (CSR_BT_CM_SM_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_CM_SM_PRIM_DOWNSTREAM_LOWEST)
#define CSR_BT_CM_PRIM_DOWNSTREAM_COUNT                       (CSR_BT_CM_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_CM_PRIM_DOWNSTREAM_LOWEST)
#define CM_SDC_PRIM_DOWNSTREAM_COUNT                          (CM_SDC_PRIM_DOWNSTREAM_HIGHEST + 1 - CM_SDC_PRIM_DOWNSTREAM_LOWEST)

#define CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT                     (CSR_BT_CM_RFC_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_SDC_PRIM_UPSTREAM_COUNT                     (CSR_BT_CM_SDC_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_SDC_PRIM_EXT_UPSTREAM_COUNT                 (CSR_BT_CM_SDC_PRIM_EXT_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_L2CA_PRIM_UPSTREAM_COUNT                    (CSR_BT_CM_L2CA_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_L2CA_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_BNEP_PRIM_UPSTREAM_COUNT                    (CSR_BT_CM_BNEP_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_BNEP_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_COUNT                 (CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_INQUIRY_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_DM_PRIM_UPSTREAM_COUNT                      (CSR_BT_CM_DM_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_DM_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_COUNT                  (CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_DM_1P2_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_SEND_PRIM_UPSTREAM_COUNT                    (CSR_BT_CM_SEND_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_SEND_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_SM_PRIM_UPSTREAM_COUNT                      (CSR_BT_CM_SM_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_SM_PRIM_UPSTREAM_LOWEST)
#define CSR_BT_CM_HCI_PRIM_UPSTREAM_COUNT                     (CSR_BT_CM_HCI_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_CM_HCI_PRIM_UPSTREAM_LOWEST)

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/

/*************************************************************************************
 Util. structure typedefs
************************************************************************************/

typedef struct
{
    CsrUint16                maxRemoteLatency;
    CsrUint16                minRemoteTimeout;
    CsrUint16                minLocalTimeout;
} CsrBtSsrSettingsDownstream;

typedef struct
{
    CsrBool                  valid;
    CsrUint16                maxTxLatency;
    CsrUint16                maxRxLatency;
    CsrUint16                minRemoteTimeout;
    CsrUint16                minLocalTimeout;
} CsrBtSsrSettingsUpstream;

/*************************************************************************************
 Primitive typedefs
************************************************************************************/
typedef struct 
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmStdCfm;


#define CSR_BT_CM_INQUIRY_CONFIG_LOW_PRIORITY_DURING_ACL    (1 << 0)
#define CSR_BT_CM_INQUIRY_CONFIG_SCAN_ENABLE                (1 << 1)

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrUint32               configMask;
    CsrUint24               inquiryAccessCode;
    CsrInt8                 inquiryTxPowerLevel;
    CsrUint8                maxResponses;
    CsrUint8                inquiryTimeout;
} CsrBtCmInquiryReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmInquiryCfm;

#define CSR_BT_CM_INQUIRY_STATUS_NONE      (0x00000000)    /* Not specific status information available */
#define CSR_BT_CM_INQUIRY_STATUS_EIR       (0x00000001)    /* The result is from a device supporting extended inquiry */

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtClassOfDevice      classOfDevice;
    CsrInt8                 rssi;
    CsrUint8                eirDataLength;
    CsrUint8                *eirData;
    CsrUint32               status;
    CsrUint16               clockOffset;
    CsrUint8                pageScanRepMode;
    CsrUint8                pageScanMode;
} CsrBtCmInquiryResultInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
} CsrBtCmCancelInquiryReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    CsrUtf8String                *friendlyName;
} CsrBtCmSetLocalNameReq;

typedef struct
{
    CsrBtCmPrim       type;
    CsrBtResultCode   resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSetLocalNameCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrUint8                  levelType;
} CsrBtCmReadTxPowerLevelReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrInt8                   powerLevel;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadTxPowerLevelCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           deviceAddr;
} CsrBtCmGetLinkQualityReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 linkQuality;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmGetLinkQualityCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
} CsrBtCmReadRssiReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrInt8                   rssi;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadRssiCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmReadRemoteNameReq;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrUtf8String                *friendlyName;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmReadRemoteNameCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUtf8String                *friendlyName;
} CsrBtCmReadRemoteNameInd;

typedef struct
{
    CsrBtCmPrim         type;
    CsrSchedQid         appHandle;
    CsrBtDeviceAddr     deviceAddr;
    CsrBtAddressType    addressType;
    CsrBtTransportType  transportType;
} CsrBtCmReadRemoteVersionReq;

typedef struct
{
    CsrBtCmPrim         type;
    CsrBtDeviceAddr     deviceAddr;
    CsrBtAddressType    addressType;
    CsrBtTransportType  transportType;
    CsrUint8            lmpVersion;
    CsrUint16           manufacturerName;
    CsrUint16           lmpSubversion;
    CsrBtResultCode     resultCode;
    CsrBtSupplier       resultSupplier;
} CsrBtCmReadRemoteVersionCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
} CsrBtCmReadLocalBdAddrReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmReadLocalBdAddrCfm;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_DM_READ_INQUIRY_MODE_REQ */
    CsrSchedQid         appHandle;          /* Application handle identifying the calling process */
} CmDmReadInquiryModeReq;

typedef struct
{
    CsrBtCmPrim            type;            /* Identity: CM_DM_READ_INQUIRY_MODE_CFM */
    CsrUint8               mode;            /* Stores the Inquiry Mode of Local Device, 
                                               can take 3 Values:
                                               0x00 - Std Inquiry Result Event Format, 
                                               0x01 - Inquiry result format with RSSI, 
                                               0x02 - Inquiry Result with RSSI or EIR format */
    CsrBtResultCode        resultCode;      /* Result of the Command */
    CsrBtSupplier          resultSupplier;  /* Supplier of the Result Code */
} CmDmReadInquiryModeCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
} CsrBtCmReadLocalNameReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUtf8String                *localName;
} CsrBtCmReadLocalNameCfm;

#define CSR_BT_CM_SERVICE_CLASS_FILTER            (0xFFE000)
#define CSR_BT_CM_MAJOR_DEVICE_CLASS_FILTER       (0x001F00)
#define CSR_BT_CM_MINOR_DEVICE_CLASS_FILTER       (0x0000FC)

typedef CsrUint8 CsrBtCmUpdateFlags;
#define CSR_BT_CM_WRITE_COD_UPDATE_FLAG_SERVICE_CLASS       ((CsrBtCmUpdateFlags) 0x01)
#define CSR_BT_CM_WRITE_COD_UPDATE_FLAG_MAJOR_MINOR_CLASS   ((CsrBtCmUpdateFlags) 0x02)

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtCmUpdateFlags       updateFlags;
    CsrBtClassOfDevice         serviceClassOfDevice;
    CsrBtClassOfDevice         majorClassOfDevice;
    CsrBtClassOfDevice         minorClassOfDevice;
} CsrBtCmWriteCodReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteCodCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
} CsrBtCmReadCodReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint24                classOfDevice;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmReadCodCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    delete_all_flag_t       flag;
} CsrBtCmSmDeleteStoreLinkKeyReq;

typedef struct
{
    CsrBtCmPrim               type;
    dm_security_mode_t      mode;
    CsrUint8                 mode3Enc;
} CsrBtCmSmSetSecModeReq;

typedef struct
{
    CsrBtCmPrim               type;
    dm_security_level_t     seclDefault;
} CsrBtCmSmSetDefaultSecLevelReq;


typedef struct
{
    CsrBtCmPrim               type;
    dm_protocol_id_t        protocolId;
    CsrUint16                channel;
} CsrBtCmSmUnregisterReq;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint8                 pinLength;
    CsrUint8                 pin[CSR_BT_PASSKEY_MAX_LEN];
} CsrBtCmSmPinRequestRes;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    dm_protocol_id_t         protocolId;
    CsrUint16                channel;
    CsrBool                  incoming;
    CsrUint16                authorisation;
    CsrBtAddressType         addressType;
} CsrBtCmSmAuthoriseRes;

typedef struct
{
    CsrBtCmPrim              type;
    dm_protocol_id_t         protocolId;
    CsrUint16                channel;
    CsrBool                  outgoingOk;
    dm_security_level_t      securityLevel;
    CsrUint8                 minEncKeySize;
} CsrBtCmSmRegisterReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmDmCheckSsrReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmRoleDiscoveryReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 role;
} CsrBtCmRoleDiscoveryCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtUuid32                *serviceList;
    CsrUint8                 serviceListSize; /* Number of _items_ in serviceList, _not_ byte size */
    CsrBool                  extendedUuidSearch;
} CsrBtCmSdcSearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtUuid32                *serviceList;
    CsrUint8                 serviceListSize; /* Number of _items_ in serviceList, _not_ byte size */
} CsrBtCmSdcRfcSearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtUuid32                *serviceList;
    CsrUint8                 serviceListSize; /* Number of _items_ in serviceList, _not_ byte size */
} CsrBtCmSdcRfcExtendedSearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtUuid128               *serviceList;
    CsrUint8                 serviceListSize; /* Number of _items_ in serviceList, _not_ byte size */
} CsrBtCmSdcUuid128RfcSearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtUuid128               *serviceList;
    CsrUint8                 serviceListSize; /* Number of _items_ in serviceList, _not_ byte size */
} CsrBtCmSdcUuid128SearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSdcOpenReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSdcOpenCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtUuid32                service;
    CsrBtUuid32                *serviceHandleList;
    CsrUint16                serviceHandleListCount;  /* Number of _items_ in serviceHandleList, _not_ byte size */
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSdcSearchInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtUuid128               service;
    CsrBtUuid32                *serviceHandleList;
    CsrUint16                serviceHandleListCount; /* Number of _items_ in serviceHandleList, _not_ byte size */
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSdcUuid128SearchInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSdcSearchCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint16                uuidSetLength;
    CsrUint8                 *uuidSet;
} CsrBtCmSdcServiceSearchReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                recListLength; /* Number of _items_ in recList, _not_ byte size */
    CsrBtUuid32                *recList;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSdcServiceSearchCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtCmPrim               typeToCancel;
} CsrBtCmSdcCancelSearchReq;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtUuid32              serviceHandle;
    CsrUint16                attributeIdentifier;
    CsrUint16                upperRangeAttributeIdentifier;
    CsrUint16                maxBytesToReturn;
} CsrBtCmSdcAttributeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                 *attributeList;
    CsrUint16                attributeListSize;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSdcAttributeCfm;

typedef struct
{
    CsrUint32     *serviceHandleList;
    CsrUint8      *serverChannelList;
} cmSdcSearchServiceInfo;

typedef struct
{
    CsrUintFast16  noOfAttr;
    CsrUint16     *attrList;
} cmSdcSearchAttrInfo;

typedef struct
{
    cmSdcSearchAttrInfo          *attrInfoList;
    void                         *serviceList;
    CsrUint8                      serviceListSize;   /* Number of _items_ in serviceList, _not_ byte size */
} cmSdcServiceSearchAttrInfo;

typedef struct
{
    CsrBtCmPrim                   type;
    CsrSchedQid                   appHandle;
    CsrBtDeviceAddr               deviceAddr;
    cmSdcServiceSearchAttrInfo   *svcSearchAttrInfoList;
    CsrBool                       extendedUuidSearch;
    CsrUint8                      uuidType;
    CsrUint8                      localServerChannel;
} CmSdcServiceSearchAttrReq;

typedef struct
{
    CsrBtCmPrim                   type;
    CsrBtDeviceAddr               deviceAddr;
    CsrBtUuid32                   serviceHandle;
    CsrUint16                     attributeListSize;
    CsrUint8                     *attributeList;
    CsrUint8                      serviceIndex;
    CsrUint8                      localServerChannel;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CmSdcServiceSearchAttrCfm;

typedef CmSdcServiceSearchAttrCfm CmSdcServiceSearchAttrInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
} CsrBtCmSdcCloseReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmSdcCloseInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8           localServerChannel;
} CsrBtCmSdcReleaseResourcesReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8           localServerChannel;
    CsrBtDeviceAddr         deviceAddr;
} CsrBtCmSdcReleaseResourcesCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
    CsrUint8                *serviceRecord;
    CsrUint16               serviceRecordSize;
    CsrUint16               context;
} CsrBtCmSdsRegisterReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint32               serviceRecHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmSdsRegisterCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint32               serviceRecHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmSdsExtRegisterCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
    CsrUint32               serviceRecHandle;
    CsrUint16               context;
} CsrBtCmSdsUnregisterReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint32               serviceRecHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmSdsUnregisterCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint32               serviceRecHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmSdsExtUnregisterCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrUint16               manufacturerDataSettings;
    CsrUint8                manufacturerDataLength;
    CsrUint8                *manufacturerData;
} CsrBtCmEirUpdateManufacturerDataReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmEirUpdateManufacturerDataCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrBool                 fec;
    CsrUint16               length;
    CsrUint8               *data;
} CsrBtCmSetEirDataReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmSetEirDataCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrUint8                eirFlags;
} CsrBtCmEirFlagsReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmEirFlagsCfm;

typedef struct
{
    CsrBtCmPrim             type; /* CSR_BT_CM_LE_WHITELIST_SET_REQ */
    CsrSchedQid             appHandle;
    CsrUint16               addressCount;
    CsrBtTypedAddr          *addressList;
} CsrBtCmLeWhitelistSetReq;

typedef struct
{
    CsrBtCmPrim             type; /* CSR_BT_CM_LE_WHITELIST_SET_CFM */
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmLeWhitelistSetCfm;

typedef struct
{
    CsrBtCmPrim             type; /* CSR_BT_CM_LE_GET_CONTROLLER_INFO_REQ */
    CsrSchedQid             appHandle;
    CsrUint8                whiteListSize;
} CsrBtCmLeGetControllerInfoReq;

typedef struct
{
    CsrBtCmPrim             type; /* CSR_BT_CM_LE_GET_CONTROLLER_INFO_CFM */
    CsrUint8                whiteListSize;
    CsrUint32               leStatesUpper;
    CsrUint32               leStatesLower;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmLeGetControllerInfoCfm;

typedef struct
{
    CsrBtCmPrim             type;      /* CM_LE_ADD_DEVICE_TO_WHITE_LIST_REQ or CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_REQ */
    CsrSchedQid             appHandle; /* Identity of the calling process to which the confirm would be sent */
    CsrBtDeviceAddr         deviceAddr;/* Address of the device to be added or removed from BLE white list */
    CsrBtAddressType        addrType;  /* Address type */
} CmLeUpdateWhiteListDeviceReq;

typedef struct
{
    CsrBtCmPrim             type; /* CM_LE_ADD_DEVICE_TO_WHITE_LIST_CFM or CM_LE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM */
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CmLeUpdateWhiteListDeviceCfm;

typedef CmLeUpdateWhiteListDeviceReq CmLeAddDeviceToWhiteListReq;
typedef CmLeUpdateWhiteListDeviceCfm CmLeAddDeviceToWhiteListCfm;

typedef CmLeUpdateWhiteListDeviceReq CmLeRemoveDeviceFromWhiteListReq;
typedef CmLeUpdateWhiteListDeviceCfm CmLeRemoveDeviceFromWhiteListCfm;
    
typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
} CsrBtCmEnableDutModeReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8                stepNumber;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmEnableDutModeCfm;


typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
} CsrBtCmDisableDutModeReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmDisableDutModeCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrBool                 disableInquiryScan;
    CsrBool                 disablePageScan;
} CsrBtCmWriteScanEnableReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmWriteScanEnableCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
} CsrBtCmReadScanEnableReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8                scanEnable;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmReadScanEnableCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
} CsrBtCmReadIacReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint24               iac;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmReadIacCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
    CsrUint24               iac;
} CsrBtCmWriteIacReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmWriteIacInd;

#define CSR_BT_HCI_DEFAULT_LSTO                               (0x7D00)

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    CsrUint16                timeout;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmWriteLinkSupervTimeoutReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteLinkSupervTimeoutCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint16                pageTimeout;
} CsrBtCmWritePageToReq;
typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmWritePageToCfm;

typedef struct
{
    CsrBtCmPrim     type;
    CsrBtTypedAddr  typedAddr;  /* Bluetooth address of remote device */
    DM_SM_TRUST_T   trust;      /* Update trust level */
    DM_SM_KEYS_T    *keys;      /* Security keys and requirements */
} CsrBtCmSmAddDeviceReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
} CsrBtCmSmRemoveDeviceReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSmAuthenticateReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtTypedAddr            address;
    CsrUint16                 flags;
    CsrUint8                  reason;
} CsrBtCmAclCloseReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  serverChannel;
    CsrSchedQid                    phandle;
    CsrUint16                 context;
} CsrBtCmCancelAcceptConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  serverChannel;
} CsrBtCmAcceptConnectTimeout;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8                serverChannel;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmCancelAcceptConnectCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    RFC_PORTNEG_VALUES_T    portPar;
    CsrUint16               context;
} CsrBtCmPortnegReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    RFC_PORTNEG_VALUES_T    portPar;
    CsrUint16               context;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmPortnegCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    RFC_PORTNEG_VALUES_T    portPar;
    CsrBool                 request;
    CsrUint16               context;
} CsrBtCmPortnegInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    RFC_PORTNEG_VALUES_T    portPar;
} CsrBtCmPortnegRes;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  appHandle;
    CsrUint8                localServerCh;
    CsrBtUuid32             serviceHandle;
    CsrUint16               profileMaxFrameSize;
    CsrBool                 requestPortPar;
    CsrBool                 validPortPar;
    RFC_PORTNEG_VALUES_T    portPar;
    dm_security_level_t     secLevel;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint16               context;
    CsrUint8                modemStatus;
    CsrUint8                breakSignal;
    CsrUint8                mscTimeout;
    CsrUint8                minEncKeySize;
} CsrBtCmConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                  localServerCh;
    CsrUint8                  remoteServerCh;
    CsrUint16                profileMaxFrameSize;
    CsrBool                  requestPortPar;
    CsrBool                  validPortPar;
    RFC_PORTNEG_VALUES_T    portPar;
    dm_security_level_t     secLevel;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                modemStatus;
    CsrUint8                breakSignal;
    CsrUint8                mscTimeout;
    CsrUint8                minEncKeySize;
} CsrBtCmConnectExtReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                   localServerCh;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtCmPrim               typeToCancel;
} CsrBtCmCancelConnectReq;


#define CSR_BT_CM_SERVER_CHANNEL_DONT_CARE      0

typedef CsrUint8 CsrBtCmRegisterOptions;
#define CM_REGISTER_OPTION_UNUSED                        ((CsrBtCmRegisterOptions)0x00)
#define CM_REGISTER_OPTION_APP_CONNECTION_HANDLING       ((CsrBtCmRegisterOptions)0x01)
#define CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL  ((CsrBtCmRegisterOptions)0x02)

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               phandle;
    CsrUint16                 context;
    CsrUint8                  serverChannel;
    CsrBtCmRegisterOptions    optionsMask;
} CsrBtCmRegisterReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrUint8                serverChannel;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmRegisterCfm;

typedef struct
{    CsrBtCmPrim              type;
    CsrUint8           serverChannel;
} CsrBtCmUnregisterReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16               profileMaxFrameSize;
    CsrBool                 validPortPar;
    RFC_PORTNEG_VALUES_T    portPar;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmConnectCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                serverChannel;
    CsrUint16               profileMaxFrameSize;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim              type;
    CsrUint16                timeout;
    CsrUint16                profileMaxFrameSize;
    CsrUint8                 serverChannel;
    dm_security_level_t      secLevel;
    CsrSchedQid              appHandle;
    CsrUint24                classOfDevice;
    uuid16_t                 profileUuid;
    CsrUint16                context;
    CsrUint8                 modemStatus;
    CsrUint8                 breakSignal;
    CsrUint8                 mscTimeout;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint8                 minEncKeySize;
} CsrBtCmConnectAcceptReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16               context;
} CsrBtCmDisconnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrBool                 status;
    CsrBool                 localTerminated;
    CsrBtReasonCode         reasonCode;
    CsrBtSupplier           reasonSupplier;
    CsrUint16               context;
} CsrBtCmDisconnectInd;

typedef struct
{
    hci_pkt_type_t          audioQuality;
    CsrUint32                txBandwidth;
    CsrUint32                rxBandwidth;
    CsrUint16                maxLatency;
    CsrUint16                voiceSettings;
    CsrUint8                 reTxEffort;
} CsrBtCmScoCommonParms;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                 pcmSlot;
    CsrBool                  pcmReassign;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16                parmsLen;
    CsrBtCmScoCommonParms   *parms;
} CsrBtCmScoConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    hci_pkt_type_t          audioQuality;
    CsrUint32                txBandwidth;
    CsrUint32                rxBandwidth;
    CsrUint16                maxLatency;
    CsrUint16                voiceSettings;
    CsrUint8                 reTxEffort;
} CsrBtCmScoAcceptConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    hci_pkt_type_t          audioQuality;
    CsrUint16                maxLatency;
    CsrUint8                 reTxEffort;
} CsrBtCmScoRenegotiateReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
} CsrBtCmScoCancelAcceptConnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                linkType;
    CsrUint8                txInterval;
    CsrUint8                weSco;
    CsrUint16               rxPacketLength;
    CsrUint16               txPacketLength;
    CsrUint8                airMode;
    hci_connection_handle_t eScoHandle;
    CsrUint8                pcmSlot;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmScoConnectCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                linkType;
    CsrUint8                txInterval;
    CsrUint8                weSco;
    CsrUint16               rxPacketLength;
    CsrUint16               txPacketLength;
    CsrUint8                airMode;
    hci_connection_handle_t eScoHandle;
    CsrUint8                pcmSlot;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmScoAcceptConnectCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId              btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                 linkType;
} CsrBtCmMapScoPcmInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrSchedQid              appHandle;
    CsrBtConnId              btConnId;
    CsrUint8                 linkType;
    dm_protocol_id_t         protocolId;
} CsrBtCmIncomingScoReq;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtConnId              btConnId;                      /* Global Bluetooth connection ID */

    hci_error_t             acceptResponse;
    CsrUint16                parmsLen;
    CsrBtCmScoCommonParms   *parms;
    CsrUint8                 pcmSlot;
    CsrBool                  pcmReassign;
} CsrBtCmMapScoPcmRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    hci_connection_handle_t eScoHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmScoRenegotiateInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    hci_connection_handle_t eScoHandle;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmScoRenegotiateCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                    appHandle;
    CsrBtConnId               btConnId;                      /* Global Bluetooth connection ID */
} CsrBtCmScoDisconnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    hci_connection_handle_t eScoHandle;
    CsrBool                 status;
    CsrBtReasonCode         reasonCode;
    CsrBtSupplier           reasonSupplier;
} CsrBtCmScoDisconnectInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId            btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16              payloadLength;
    CsrUint8              *payload;
} CsrBtCmDataReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId            btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16              payloadLength;
    CsrUint8              *payload;
    CsrUint16               context;
} CsrBtCmDataInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint16               context;
} CsrBtCmDataCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;                      /* Global Bluetooth connection ID */
} CsrBtCmDataRes;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtConnId              btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                 modemstatus;
    CsrUint8                 break_signal;
} CsrBtCmControlReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                modemstatus;
    CsrUint8                break_signal;
    CsrUint16               context;
} CsrBtCmControlInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmRejectRfcConnectionInd;

typedef struct
{
    CsrBtCmPrim       type;
    CsrUint8          mode;
    CsrUint16         length;
    CsrBtConnId       btConnId;                      /* Global Bluetooth connection ID */
    CsrBtResultCode   resultCode;
    CsrBtSupplier     resultSupplier;
    CsrUint16         context;
} CsrBtCmRfcModeChangeInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;                      /* Global Bluetooth connection ID */
    CsrUint8                  requestedMode;
    CsrBool                   forceSniff;
} CsrBtCmRfcModeChangeReq;

typedef LP_POWERSTATE_T CsrBtCmPowerSetting;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         addr;
    CsrUint8                powerTableSize;
    CsrBtCmPowerSetting    *powerTable;
} CsrBtCmDmPowerSettingsReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId             btConnId;                      /* Global Bluetooth connection ID */
    CsrBtSniffSettings     *sniffSettings;
    CsrUint8                 sniffSettingsSize;
    CsrBtSsrSettingsDownstream *ssrSettings;
    CsrUint8                 ssrSettingsSize;
    CsrUint8                 lowPowerPriority;
} CsrBtCmDmModeSettingsReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrBtAddressType        addrType;
    CsrBtDeviceAddr         addr;
    CmSetLinkBehaviorMask   flags;
} CsrBtCmDmSetLinkBehaviourReq;

typedef CsrBtCmDmSetLinkBehaviourReq CsrBtCmDmSetLinkBehaviorReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtAddressType        addrType;
    CsrBtDeviceAddr         addr;
    CsrBtResultCode         status;
} CsrBtCmDmSetLinkBehaviourCfm;

typedef CsrBtCmDmSetLinkBehaviourCfm CsrBtCmDmSetLinkBehaviorCfm;

typedef CsrUint8 CsrBtL2caRegisterOptions;
#define CM_L2CA_REGISTER_OPTION_UNUSED                   ((CsrBtL2caRegisterOptions)0x00)
#define CM_L2CA_REGISTER_OPTION_APP_CONNECTION_HANDLING  ((CsrBtL2caRegisterOptions)0x01)

typedef struct
{
    CsrBtCmPrim                 type;
    CsrSchedQid                 phandle;
    psm_t                       localPsm;
    CsrUint16                   mode_mask;        /*!< Mask of which L2CAP modes are acceptable, see the L2CA_MODE_MASK-defines */
    CsrUint16                   flags;            /*!< Register flags. */
    CsrUint16                   context;
    CsrBtL2caRegisterOptions    optionsMask;      /*!< Options associated with the registration, see CsrBtL2caRegisterOptions */
} CsrBtCmL2caRegisterReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               phandle;
    psm_t                     localPsm;
} CsrBtCmL2caUnregisterReq;

typedef struct
{
    CsrBtCmPrim                 type;
    CsrSchedQid                 phandle;
    CsrUint16                   fixedCid;
    CsrUint16                   context;
    L2CA_CONFIG_T               config;           /*!< Configuration of channel */
} CmL2caRegisterFixedCidReq;

typedef struct
{
    CsrBtCmPrim                 type;
    CsrSchedQid                 phandle;
    CsrUint16                   fixedCid;
    CsrBtResultCode             resultCode;
    CsrBtSupplier               resultSupplier;
    CsrUint16                   context;
} CmL2caRegisterFixedCidCfm;

typedef struct
{
    CsrBtCmPrim               type;
    psm_t                   localPsm;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
    CsrUint16               context;
} CsrBtCmL2caCancelConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim               type;
    psm_t                   localPsm;
    CsrUint16                mode_mask;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
    CsrUint16                 context;
} CsrBtCmL2caRegisterCfm;

typedef struct
{
    CsrBtCmPrim               type;
    psm_t                     connectionlessPsm;
    CsrUint16                 length;
    CsrUint8                 *payload;
    CsrBtDeviceAddr           deviceAddr;
} CsrBtCmL2caConnectionlessDataReq;

typedef struct
{
    CsrBtCmPrim               type;
    psm_t                     connectionlessPsm;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmL2caConnectionlessDataCfm;

typedef struct
{
    CsrBtCmPrim               type;
    psm_t                     connectionlessPsm;
    CsrUint16                 length;
    CsrUint8                 *payload;
    CsrBtDeviceAddr           deviceAddr;
} CsrBtCmL2caConnectionlessDataInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  phandle;
    CsrBtDeviceAddr         addr;
    psm_t                   localPsm;
    psm_t                   remotePsm;
    dm_security_level_t     secLevel;
    CsrUint16               context;
    CsrUint16               conftabCount;
    CsrUint16              *conftab;
    CsrUint8                minEncKeySize;
} CsrBtCmL2caConnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             phandle;
    CsrBtTpdAddrT           tpdAddrT;
    psm_t                   localPsm;
    psm_t                   remotePsm;
    dm_security_level_t     secLevel;
    CsrUint16               context;
    CsrUint16               conftabCount;
    CsrUint16              *conftab;
    CsrUint8                minEncKeySize;
} CmL2caTpConnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  phandle;
    CsrBtDeviceAddr         deviceAddr;
    psm_t                   localPsm;
} CsrBtCmCancelL2caConnectReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid                  phandle;
    psm_t                   localPsm;
    CsrUint24               classOfDevice;
    CsrBool                 primaryAcceptor;
    dm_security_level_t     secLevel;
    uuid16_t                profileUuid;
    CsrUint16               context;
    CsrUint16               conftabCount;
    CsrUint16              *conftab;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                minEncKeySize;
} CsrBtCmL2caConnectAcceptReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;
    psm_t                   localPsm;
    l2ca_mtu_t              mtu;
    l2ca_mtu_t              localMtu;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmL2caConnectCfm;

typedef struct
{
    CsrBtCmPrim             type;                   /* Identity: CM_L2CA_TP_CONNECT_CFM */
    CsrBtConnId             btConnId;               /* Global Bluetooth connection ID for LECOC channel*/
    psm_t                   localPsm;               /* PSM for which the connect confirm is received. */
    l2ca_mtu_t              mtu;                    /* Max Outgoing Payload size */
    l2ca_mtu_t              localMtu;               /* Local device MTU (Max Incoming Payload size)*/
    CsrBtTpdAddrT           tpdAddrT;               /* Remote bluetooth device address with type and transport */
    CsrBtResultCode         resultCode;             /* Result code */
    CsrBtSupplier           resultSupplier;         /* Result supplier */
    CsrUint16               context;                /* Context Value(Passed back from connect request) */
} CsrBtCmL2caTpConnectCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;
    psm_t                   localPsm;
    psm_t                   remotePsm;
    l2ca_mtu_t              mtu;
    l2ca_mtu_t              localMtu;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint16               context;
} CsrBtCmL2caConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim             type;               /* Identity: CM_L2CA_TP_CONNECT_ACCEPT_CFM */
    CsrBtConnId             btConnId;           /* Global Bluetooth connection ID */
    psm_t                   localPsm;           /* Local PSM for which the connect request is received. */
    psm_t                   remotePsm;          /* Peer device's PSM for which the connect request is received. */
    l2ca_mtu_t              mtu;                /* Max Outgoing Payload size */
    l2ca_mtu_t              localMtu;           /* Local device MTU (Max Incoming Payload size)*/
    CsrBtTpdAddrT           tpdAddrT;           /* Remote bluetooth device address with type and transport */
    CsrBtResultCode         resultCode;         /* Result code */
    CsrBtSupplier           resultSupplier;     /* Result supplier */
    CsrUint16               context;            /* Reserved for future use */
} CmL2caTpConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             phandle;
    CsrBtConnId             btConnId;
    CsrUint16               context;
    CsrUint16               credits;
} CmL2caAddCreditReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;
    CsrUint16               context;
    CsrUint16               credits;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CmL2caAddCreditCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;
    CsrUint16               length;
    CsrMblk                 *payload;
    CsrUint16               context;
} CsrBtCmL2caDataReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
} CsrBtCmL2caDataRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
} CsrBtCmL2caDataAbortReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
    CsrUint16                 context;
} CsrBtCmL2caDisconnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
    CsrBool                   localTerminated;
    CsrBtReasonCode           reasonCode;
    CsrBtSupplier             reasonSupplier;
    CsrUint16                 context;
    l2ca_identifier_t         l2caSignalId;
} CsrBtCmL2caDisconnectInd;

typedef struct
{
    CsrBtCmPrim                type;
    CsrBtConnId                btConnId;
    CsrBtAmpController         remoteControl;   /*!< Remote controller ID */
    CsrBtAmpController         localControl;    /*!< Local controller ID */
} CsrBtCmMoveChannelReq;

typedef struct
{
    CsrBtCmPrim                type;
    CsrBtConnId                btConnId;
    CsrBtAmpController         localControl;    /*!< Local controller ID actually used */
    CsrBtDeviceAddr            deviceAddr;      /*!< Peer address */
    CsrBtResultCode            resultCode;
    CsrBtSupplier              resultSupplier;
} CsrBtCmMoveChannelCfm;

typedef CsrBtCmMoveChannelCfm CsrBtCmMoveChannelCmpInd;

typedef struct
{
    CsrBtCmPrim                type;
    CsrBtConnId                btConnId;
    CsrBtAmpController         localControl;    /*!< Local controller ID actually used */
    CsrBtDeviceAddr            deviceAddr;      /*!< Peer address */
} CsrBtCmMoveChannelInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
    CsrBool                   accept;
} CsrBtCmMoveChannelRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId               btConnId;
    CsrUint16                 length;
    CsrUint8                  *payload;
    CsrUint16                 context;
} CsrBtCmL2caDataInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtConnId              btConnId;
    CsrUint16                context;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmL2caDataCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId              btConnId;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
    CsrUint16                context;
} CsrBtCmL2caDataAbortCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtConnId             btConnId;
    CsrUint8                requestedMode;
    CsrBool                 forceSniff;
} CsrBtCmL2caModeChangeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId              btConnId;
    CsrUint8                 mode;
    CsrUint16                length;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
    CsrUint16                context;
} CsrBtCmL2caModeChangeInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId             btConnId;
} CsrBtCmL2caCommonPrim;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtTypedAddr            address;
    CsrUint16                 flags;
} CsrBtCmAclOpenReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtTypedAddr            deviceAddr;
    CsrBool                   status;
} CsrBtCmAclOpenCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBool                  encryptionMode;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSmEncryptionReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    psm_t                   localPsm;
    CsrUint16               context;
} CsrBtCmL2caCancelConnectAcceptReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtConnId              btConnId;
    CsrBtSniffSettings        *sniffSettings;
    CsrUint8                 sniffSettingsSize;
    CsrBtSsrSettingsDownstream *ssrSettings;
    CsrUint8                 ssrSettingsSize;
    CsrUint8                 lowPowerPriority;
} CsrBtCmDmL2caModeSettingsReq;

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
typedef struct
{
    CsrBtCmPrim               type;
    CsrBool                  disableExtended;
    CsrBool                  manualBridge;
    CsrBool                  disableStack;
    CsrSchedQid                     phandle;
    CsrBtDeviceAddr            deviceAddr;
    psm_t                   localPsm;
} CsrBtCmBnepRegisterReq;

typedef struct
{
    CsrBtCmPrim               type;
    BNEP_CONNECT_REQ_FLAGS  flags;
    ETHER_ADDR              rem_addr; /* set msw to ETHER_UNKNOWN for passive */
    CsrUint16                profileMaxFrameSize;
    dm_security_level_t     secLevel;
    CsrUint8                minEncKeySize;
} CsrBtCmBnepConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    ETHER_ADDR              rem_addr;
    CsrUint16                rem_uuid;
    CsrUint16                loc_uuid;
    CsrUint16                profileMaxFrameSize;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmBnepConnectInd;

typedef struct
{
    CsrBtCmPrim               type;
    ETHER_ADDR              rem_addr;
} CsrBtCmCancelBnepConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    BNEP_CONNECT_REQ_FLAGS  flags;
    ETHER_ADDR              rem_addr;
    dm_security_level_t     secLevel;
    CsrUint24                classOfDevice;
    CsrUint16                profileMaxFrameSize;
    CsrUint8                 minEncKeySize;
} CsrBtCmBnepConnectAcceptReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmBnepConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim               type;
} CsrBtCmBnepCancelConnectAcceptReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmBnepCancelConnectAcceptCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                idNot;
    CsrUint16                etherType;
    ETHER_ADDR              dstAddr; /* note may be multicast */
    ETHER_ADDR              srcAddr; /* should be a PANU's address, but who knows? */
    CsrUint16                length;
    CsrUint8                 *payload;
} CsrBtCmBnepExtendedMulticastDataReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrUint16                etherType;
    ETHER_ADDR              dstAddr; /* note may be multicast */
    ETHER_ADDR              srcAddr; /* should be a PANU's address, but who knows? */
    CsrUint16                length;
    CsrUint8                 *payload;
} CsrBtCmBnepExtendedDataInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrUint16                etherType;
    ETHER_ADDR              dstAddr; /* note may be multicast */
    ETHER_ADDR              srcAddr; /* should be a PANU's address, but who knows? */
    CsrUint16                length;
    CsrUint8                 *payload;
} CsrBtCmBnepExtendedDataReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
} CsrBtCmBnepExtendedDataCfm;

typedef struct
{
    CsrUint16                type;
    CsrUint16                flags;
    CsrUint16                id;
} CsrBtCmBnepDisconnectReq;

typedef CsrBtCmBnepDisconnectReq CsrBtCmBnepDisconnectRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrBtReasonCode         reasonCode;
    CsrBtSupplier     reasonSupplier;
} CsrBtCmBnepDisconnectInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrUint8                 requestedMode;
} CsrBtCmBnepModeChangeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrUint8                 mode;
    CsrUint16                length;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmBnepModeChangeInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                id;
    CsrUint8                 role;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmBnepSwitchRoleInd;

typedef struct
{
    CsrBtCmPrim                type;
    CsrUint16                  id;
    CsrBtSniffSettings         *sniffSettings;
    CsrUint8                  sniffSettingsSize;
    CsrBtSsrSettingsDownstream *ssrSettings;
    CsrUint8                 ssrSettingsSize;
    CsrUint8                 lowPowerPriority;
} CsrBtCmDmBnepModeSettingsReq;

#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

typedef struct 
{
    CsrBtCmPrim         type;
    CsrSchedQid         appHandle;
    CsrBtDeviceAddr     deviceAddr;
    CsrBool             useDefaultQos;
} CsrBtCmDmHciQosSetupReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                 pageNum;
    CsrBtDeviceAddr            bd_addr;
} CsrBtCmReadRemoteExtFeaturesReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmReadRemoteFeaturesReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  pageNum;
    CsrUint8                  maxPageNum;
    CsrUint16                 extLmpFeatures[4];
    CsrBtDeviceAddr           bd_addr;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadRemoteExtFeaturesCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrUint8                  map[CSR_BT_CM_AFH_HOST_CHANNEL_CLASSIFICATION_SIZE];
} CsrBtCmSetAfhChannelClassReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmSetAfhChannelClassCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           bd_addr;
} CsrBtCmReadAfhChannelMapReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  mode;
    CsrUint8                  afhMap[10];
    CsrBtDeviceAddr           bd_addr;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadAfhChannelMapCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
} CsrBtCmReadAfhChannelAssessmentModeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  classMode;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadAfhChannelAssessmentModeCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrUint8                  classMode;
} CsrBtCmWriteAfhChannelAssessmentModeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmWriteAfhChannelAssessmentModeCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrUint8                  pageNum;
} CsrBtCmReadLocalExtFeaturesReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  pageNum;
    CsrUint8                  maxPageNum;
    CsrUint8                  extLmpFeatures[8];
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadLocalExtFeaturesCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrUint8                  whichClock;
    CsrBtDeviceAddr           bd_addr;
} CsrBtCmReadClockReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           bd_addr;
    CsrUint32                 clock;
    CsrUint16                 accuracy;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadClockCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
} CsrBtCmReadLocalVersionReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                  lmpVersion;
    hci_version_t             hciVersion;
    CsrUint16                 hciRevision;
    CsrUint16                 manufacturerName;
    CsrUint16                 lmpSubversion;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadLocalVersionCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           deviceAddr;
    CsrBool                   setupLinkPolicySetting;
    link_policy_settings_t    linkPolicySetting;
    CsrUint8                  sniffSettingsCount; /* Number of _items_ in sniffSettings, _not_ byte size */
    CsrBtSniffSettings       *sniffSettings;
} CsrBtCmWriteLinkPolicyReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid               appHandle;
    CsrBtDeviceAddr           deviceAddr;
} CsrBtCmReadLinkPolicyReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrUint8                  actualMode;
    link_policy_settings_t    linkPolicySetting;
    CsrBtSniffSettings        sniffSettings;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmReadLinkPolicyCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteLinkPolicyErrorInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBool                   cancelInitiated;
} CsrBtCmConnectionRejSecurityInd;

typedef struct
{
    CsrBtCmPrim               type;
} CsrBtCmSmHouseCleaning;

typedef struct
{
    CsrBtCmPrim               type;
} CsrBtCmDmHouseCleaning;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBool                  selectInquiryFilter; /* Select inquiry or connection filter */
    CsrUint8                 autoAccept;          /* Auto Accept flag, see hci.h for values */
    CsrBtDeviceAddr            address;
} CsrBtCmSetEventFilterBdaddrReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSetEventFilterBdaddrCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBool                  selectInquiryFilter; /* Select inquiry or connection filter */
    CsrUint8                 autoAccept;          /* Auto Accept flag, see hci.h for values */
    CsrUint24                cod;
    CsrUint24                codMask;
} CsrBtCmSetEventFilterCodReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSetEventFilterCodCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                 filter;
} CsrBtCmClearEventFilterReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmClearEventFilterCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                 activePlayer;
    CsrBtDeviceAddr            devAddr;
    CsrUint16                clockOffset;
    page_scan_mode_t        pageScanMode;
    page_scan_rep_mode_t    pageScanRepMode;
} CsrBtCmDmWriteCacheParamsReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            devAddr;
} CsrBtCmDmUpdateAndClearCachedParamReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSmCancelConnectReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBool                  alwaysSupportMasterRole;
} CsrBtCmAlwaysSupportMasterRoleReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmReadEncryptionStatusReq;

typedef struct
{
    CsrBtCmPrim       type;
    CsrUint16         encrypted;         /* Encrypt type ENCR_NONE, E0 (0x01) or AES-CCM (0x02)*/
    CsrBtResultCode   resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmReadEncryptionStatusCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint16                interval;
    CsrUint16                window;
} CsrBtCmWritePagescanSettingsReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWritePagescanSettingsCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                 scanType;
} CsrBtCmWritePagescanTypeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWritePagescanTypeCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint16                interval;
    CsrUint16                window;
} CsrBtCmWriteInquiryscanSettingsReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteInquiryscanSettingsCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrUint8                 scanType;
} CsrBtCmWriteInquiryscanTypeReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteInquiryscanTypeCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmCancelReadRemoteNameReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSmBondingReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBool                   force;
    CsrBtAddressType          addressType;
} CsrBtCmSmBondingCancelReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint16                writeAuthEnable;
    CsrUint16                config;
} CsrBtCmSmSecModeConfigReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmCommonRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrUint8                  ioCapability;
    CsrUint8                  authenticationRequirements;
    CsrUint8                  oobDataPresent;
    CsrUint8                  *oobHashC;
    CsrUint8                  *oobRandR;
    CsrUint16                 keyDistribution;
} CsrBtCmSmIoCapabilityRequestRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    hci_error_t               reason;
} CsrBtCmSmIoCapabilityRequestNegRes;

typedef CsrBtCmSmIoCapabilityRequestNegRes CsrBtCmSmUserConfirmationRequestRes;
typedef CsrBtCmSmIoCapabilityRequestNegRes CsrBtCmSmUserConfirmationRequestNegRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrUint32                 numericValue;
} CsrBtCmSmUserPasskeyRequestRes;

typedef CsrBtCmSmUserPasskeyRequestRes CsrBtCmSmUserPasskeyRequestNegRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtTransportType        transportType;
} CsrBtCmSmReadLocalOobDataReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtAddressType          addressType;
    CsrBtTransportType        transportType;
    CsrUint8                  notificationType;
} CsrBtCmSmSendKeypressNotificationReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint16               repairId;
    CsrBtAddressType        addressType;
} CsrBtCmSmRepairInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint16                repairId;
    CsrBool                  accept;
    CsrBtAddressType         addressType;
} CsrBtCmSmRepairRes;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
} CsrBtCmSmReadDeviceReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint32                enhancements;
} CsrBtCmEnEnableEnhancementsReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrUint16                 flushTo;
    CsrSchedQid               appHandle;
} CsrBtCmDmWriteAutoFlushTimeoutReq;

#define CSR_BT_CM_ROLE_SWITCH_DEFAULT                  (0x00000000) /* Try and do RS before SCO connections or if more than 1 ACL exists   */
#define CSR_BT_CM_ROLE_SWITCH_ALWAYS                   (0x00000001) /* Always do RS                                                        */
#define CSR_BT_CM_ROLE_SWITCH_NOT_BEFORE_SCO           (0x00000002) /* Don't do RS before SCO setup                                        */
#define CSR_BT_CM_ROLE_SWITCH_BEFORE_RNR               (0x00000003) /* Do RS before RNR                                                    */
#define CSR_BT_CM_ROLE_SWITCH_BEFORE_SCO               (0x00000004) /* Do RS before SCO setup                                              */
#define CSR_BT_CM_ROLE_SWITCH_NOT_BEFORE_RNR           (0x00000005) /* Don't do RS before RNR                                              */
#define CSR_BT_CM_ROLE_SWITCH_ALWAYS_ACL               (0x00000006) /* Do RS every time an ACL is establish                                */
#define CSR_BT_CM_ROLE_SWITCH_MULTIPLE_ACL             (0x00000007) /* Only do RS if more then 1 ACL exitst                                */

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint32                config;
} CsrBtCmRoleSwitchConfigReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrSchedQid                     appHandle;
} CsrBtCmReadFailedContactCounterReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint16                failedContactCount;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmReadFailedContactCounterCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     phandle;
    CsrUint32                eventMask;
    CsrUint32                conditionMask;
} CsrBtCmSetEventMaskReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrUint32                eventMask;
} CsrBtCmSetEventMaskCfm;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint16                lsto;
} CsrBtCmLstoChangeInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 mode;
    CsrUint16                interval;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmModeChangeInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 role;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmRoleChangeInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrBool                  incoming;
    hci_connection_handle_t  syncHandle;
    CsrUint8                 linkType;
    CsrUint8                 txInterval;
    CsrUint8                 weSco;
    CsrUint16                rxPacketLength;
    CsrUint16                txPacketLength;
    CsrUint8                 airMode;
    hci_pkt_type_t           packetType;
    CsrUint32                txBdw;
    CsrUint32                rxBdw;
    CsrUint16                maxLatency;
    CsrUint8                 reTxEffort;
    CsrUint16                voiceSettings;
    CsrBtResultCode          resultCode;
    CsrBtSupplier            resultSupplier;
} CsrBtCmSyncConnectInd;

typedef CsrBtCmSyncConnectInd CsrBtCmSyncRenegotiateInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrBool                  incoming;
    hci_connection_handle_t  syncHandle;
    CsrUint8                 linkType;
    CsrUint8                 txInterval;
    CsrUint8                 weSco;
    CsrUint8                 reservedSlots;
    CsrUint16                rxPacketLength;
    CsrUint16                txPacketLength;
    CsrUint8                 airMode;
    hci_pkt_type_t           packetType;
    CsrUint32                txBdw;
    CsrUint32                rxBdw;
    CsrUint16                maxLatency;
    CsrUint8                 reTxEffort;
    CsrUint16                voiceSettings;
    CsrBtResultCode          resultCode;
    CsrBtSupplier            resultSupplier;
} CsrBtCmExtSyncConnectInd;


typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    hci_connection_handle_t syncHandle;
    hci_reason_t            reason;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSyncDisconnectInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrUint8                  incoming;
    CsrBtClassOfDevice        cod;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmAclConnectInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr           deviceAddr;
    CsrBtReasonCode           reasonCode;
    CsrBtSupplier             reasonSupplier;
} CsrBtCmAclDisconnectInd;

typedef struct
{
    CsrBtCmPrim               type;
} CsrBtCmBluecoreInitializedInd;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtResultCode           resultCode;
    CsrBtSupplier             resultSupplier;
} CsrBtCmBluecoreDeinitializedInd;

typedef struct
{
    CsrBtCmPrim              type;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint16                maxTxLatency;
    CsrUint16                maxRxLatency;
    CsrUint16                minRemoteTimeout;
    CsrUint16                minLocalTimeout;
    CsrBtResultCode          resultCode;
    CsrBtSupplier            resultSupplier;
} CsrBtCmSniffSubRatingInd;

typedef struct
{
    CsrBtCmPrim               type; /* Identity: CSR_BT_CM_LOCAL_NAME_CHANGE_IND */
    CsrUtf8String            *localName;
} CsrBtCmLocalNameChangeInd;

/* Low Energy COEX event. Subscribe via the
 * CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY event mask */
typedef struct
{
    CsrBtCmPrim               type;        /* Identity */
    CsrUint8                  event;       /* State, use CSR_BT_CM_LE_MODE_... */
    CsrUint8                  advType;     /* Type of advertising, use CSR_BT_CM_LE_ADVTYPE_ */
    CsrUint16                 intervalMin; /* Minimum advertising interval (in slots, i.e. x * 0.625ms) */
    CsrUint16                 intervalMax; /* Maximum advertising interval (in slots, i.e. x * 0.625ms) */
    CsrUint8                  channelMap;  /* Advertising channel map, use CSR_BT_CM_LE_CHANMAP_ */
} CsrBtCmLeEventAdvertisingInd;

/* Low Energy COEX event. Subscribe via the
 * CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY event mask */

typedef struct
{
    CsrBtCmPrim               type;      /* Identity */
    CsrUint8                  event;     /* State, use CSR_BT_CM_LE_MODE_... */
    CsrUint8                  scanType;  /* Type of scanning, use CSR_BT_CM_LE_SCANTYPE_ */
    CsrUint16                 interval;  /* Scan interval (in slots, i.e. x * 0.625ms) */
    CsrUint16                 window;    /* Scan window (in slots, i.e. x * 0.625ms) */
} CsrBtCmLeEventScanInd;

typedef CsrUint8 CmConnectStatus;

/* ACL connection is opened with success. */
#define CSR_BT_LE_EVENT_CONNECT_SUCCESS                  ((CmConnectStatus)0x00)
/* ACL connection open has failed. */
#define CSR_BT_LE_EVENT_CONNECT_FAIL                     ((CmConnectStatus)0x01)
/* ACL connection is closed with successfully. */
#define CSR_BT_LE_EVENT_DISCONNECT                       ((CmConnectStatus)0x02)
/* LE Connection update complete is received. */
#define CSR_BT_LE_EVENT_CONNECTION_UPDATE_COMPLETE       ((CmConnectStatus)0x03)
/* ACL connection is closed due to sync timeout. */
#define CSR_BT_LE_EVENT_DISCONNECT_SYNC_TO               ((CmConnectStatus)0x04)

/* Low Energy event. Subscribe via the
 * CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LOW_ENERGY event mask 
 *
 * Note: Use either event or connectStatus to interpret the connections. "event" gives basic
 *       information regarding LE connection modes wheras "connectStatus" gives extra
 *       information like status of an ACL open (success/failure).
 *
 * Param event: Specifies the low energy (LE) mode states.
 *              Following are the meaning of various mode states for connections.
 *               CSR_BT_CM_LE_MODE_ON     --> LE connected.
 *               CSR_BT_CM_LE_MODE_OFF    --> LE not connected.
 *               CSR_BT_CM_LE_MODE_MODIFY --> LE connection updated.
 *
 * Param connectStatus: Specifies the status of LE connection i.e. connected/disconnected/updated.
                         CSR_BT_LE_EVENT_CONNECT_SUCCESS            --> LE connection established successfully.
                         CSR_BT_LE_EVENT_CONNECT_FAIL               --> LE connection failed to establish.
                         CSR_BT_LE_EVENT_DISCONNECT                 --> LE connection is terminated.
                         CSR_BT_LE_EVENT_CONNECTION_UPDATE_COMPLETE --> LE connection updated.
                         CSR_BT_LE_EVENT_DISCONNECT_SYNC_TO         --> LE connection is terminated due to sync timeout.
 */
typedef struct
{
    CsrBtCmPrim               type;          /* Identity */
    CsrUint8                  event;         /* State, use CSR_BT_CM_LE_MODE_... */
    CsrBtTypedAddr            deviceAddr;    /* Peer device address */
    CsrUint8                  role;          /* Role, use CSR_BT_CM_ROLE_... */
    CsrUint16                 interval;      /* Connection interval (in slots, i.e. x * 0.625ms) */
    CsrUint16                 timeout;       /* Supervision timeout (in 10ms units) */
    CsrUint16                 latency;       /* Connection latency (in slots, i.e. x * 0.625ms) */
    CsrUint8                  accuracy;      /* Clock accurary, use CSR_BT_CM_LE_CLOCKACCU_ */
    CmConnectStatus           connectStatus; /* LE Connection Status, refer CmConnectStatus. */
    hci_reason_t              reason;        /* HCI status/error code */
} CsrBtCmLeEventConnectionInd;

#define CSR_BT_CM_MODE_CHANGE_DISABLE  (0) /* Synergy BT is controlling all low power handling. Default setting */
#define CSR_BT_CM_MODE_CHANGE_ENABLE   (1) /* The Application is controlling all low power handling       */

typedef struct
{
    CsrBtCmPrim              type;
    CsrSchedQid              phandle;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint32                config;
} CsrBtCmModeChangeConfigReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmModeChangeConfigCfm;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             phandle;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                mode;
    CsrBool                 forceSniffSettings;
    CsrBtSniffSettings      sniffSettings;
} CsrBtCmModeChangeReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                mode;
    CsrUint16               interval;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmModeChangeCfm;


typedef CsrUint32  CsrBtLogicalChannelType;
#define CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL        ((CsrBtLogicalChannelType)0x00000000)
#define CSR_BT_ACTIVE_DATA_CHANNEL              ((CsrBtLogicalChannelType)0x00000001)
#define CSR_BT_ACTIVE_CONTROL_CHANNEL           ((CsrBtLogicalChannelType)0x00000002)
#define CSR_BT_ACTIVE_STREAM_CHANNEL            ((CsrBtLogicalChannelType)0x00000004)

typedef struct
{
    CsrBtCmPrim     type;
    CsrBtLogicalChannelType     logicalChannelTypeMask;
    CsrBtDeviceAddr deviceAddr;
    CsrBtConnId     btConnId;                      /* Global Bluetooth connection ID */
}CsrBtCmLogicalChannelTypeReq;

typedef struct
{
    CsrBtCmPrim                 type;
    CsrBtDeviceAddr             deviceAddr;
    CsrBtLogicalChannelType     logicalChannelTypeMask;
    CsrUint8                    numberOfGuaranteedLogicalChannels;
} CsrBtCmLogicalChannelTypesInd;


typedef CsrUint32 CsrBtCmRoleType;
#define CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID     ((CsrBtCmRoleType) 0x00)
#define CSR_BT_CM_SWITCH_ROLE_TYPE_ONESHOT     ((CsrBtCmRoleType) 0x01)
#define CSR_BT_CM_SWITCH_ROLE_TYPE_MAX         ((CsrBtCmRoleType) 0x01)

typedef struct
{
    CsrBtCmPrim               type;
    CsrSchedQid                     appHandle;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 role;
    CsrBtCmRoleType          roleType;
    CsrUint32                config;     /*! < RFU */
} CsrBtCmSwitchRoleReq;

typedef struct
{
    CsrBtCmPrim               type;
    CsrBtDeviceAddr            deviceAddr;
    CsrUint8                 role;
    CsrBtCmRoleType          roleType;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmSwitchRoleCfm;

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
typedef struct
{
    CsrBtCmPrim               type;
    CsrUint8                 role;
    CsrUint16                id;
} CsrBtCmBnepSwitchRoleReq;
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

typedef struct
{
    dm_prim_t               type;
    CsrSchedQid                     phandle;
    CsrUint16                voiceSettings;
} CsrBtCmWriteVoiceSettingsReq;

typedef struct
{
    dm_prim_t               type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier     resultSupplier;
} CsrBtCmWriteVoiceSettingsCfm;

typedef struct
{
    dm_prim_t           type;
    CsrSchedQid         phandle;
    CsrBtTpdAddrT       tpAddrt;
    CsrUint16           authPayloadTimeout;
    DM_SM_APT_ROUTE_T   aptRoute;
} CsrBtCmWriteAuthPayloadTimeoutReq;

typedef struct
{
    dm_prim_t        type;
    CsrBtTpdAddrT    tpAddrt;
    CsrBtResultCode  resultCode;
    CsrBtSupplier    resultSupplier;
} CsrBtCmWriteAuthPayloadTimeoutCfm;

typedef struct
{
    dm_prim_t        type;
    CsrBtDeviceAddr  deviceAddr;
} CmDmHciAuthPayloadTimeoutExpiredInd;

typedef struct
{
    CsrBtCmPrim             type;
} CsrBtCmSmAccessReq;

typedef struct
{
    CsrBtCmPrim             type; /* Identity: CSR_BT_CM_REGISTER_HANDLER_REQ */
    CsrUint8                handlerType;
    CsrSchedQid             handle;
    CsrUint32               flags;
} CsrBtCmRegisterHandlerReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                remoteLmpFeatures[8];
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmRemoteFeaturesInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                lmpVersion;
    CsrUint16               manufacturerName;
    CsrUint16               lmpSubversion;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmRemoteVersionInd;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtDeviceAddr         deviceAddr;     /* address of the remote device*/
    CsrUint8                streamIdx;      /* unique Id for the stream in question */
    CsrUint32               bitRate;        /* bit rate used */
} CsrBtCmA2dpBitRateInd;
typedef CsrBtCmA2dpBitRateInd CsrBtCmA2dpBitRateReq;

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
typedef struct
{
    CsrBtCmPrim     type;
    CsrUint16       aclHandle; /* Identifies the ACL Link */
    CsrUint16       l2capConnectionId; /* Identifies the local L2CAP channel ID */
    CsrUint16       bitRate; /* Identifies the bit rate of the codec in kbps */
    CsrUint16       sduSize; /* Identifies the L2CAP MTU negotiated for av */
    CsrUint8        period; /* Identifies the period in ms of codec data being available for transmission */
    CsrUint8        role; /* Identifies the local device role, source or sink */
    CsrUint8        samplingFreq; /* Identifies the sampling frequency of audio codec used */
    CsrUint8        codecType; /* Identifies the codec type e.g. SBC/aptX etc */
    CsrUint8        codecLocation; /* Identifies the location of the codec on/off-chip*/
    CsrUint8        streamIndex; /* unique id for a stream */
    CsrBool         start; /* identifies start/stop of av stream */
} CsrBtCmSetAvStreamInfoReq;
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

typedef CsrUint8 CsrBtCmInquiryEventType;
#define CSR_BT_CM_INQUIRY_TYPE_START            ((CsrBtCmInquiryEventType) 0x00) /* Inquiry ongoing */
#define CSR_BT_CM_INQUIRY_TYPE_STOP             ((CsrBtCmInquiryEventType) 0x01) /* Inquiry not ongoing */

typedef CsrUint8 CsrBtCmPagingEventType;
#define CSR_BT_CM_PAGE_TYPE_START            ((CsrBtCmPagingEventType) 0x00)     /* Paging ongoing */
#define CSR_BT_CM_PAGE_TYPE_STOP             ((CsrBtCmPagingEventType) 0x01)     /* Paging not ongoing */

typedef struct
{
    CsrBtCmPrim                   type;
    CsrBtCmInquiryEventType       inquiry;
    CsrBtCmPagingEventType        paging;
} CsrBtCmInquiryPageEventInd;

typedef struct
{
    CsrBtCmPrim                   type;
    CsrUint8                      lmpVersion;
} CsrBtCmGetSecurityConfInd;

typedef struct
{
    CsrBtCmPrim                   type;
    CsrUint16                     options;
    dm_security_mode_t            securityMode;
    dm_security_level_t           securityLevelDefault;
    CsrUint16                     config;
    CsrUint16                     writeAuthEnable;
    CsrUint8                      mode3enc;
    CsrUint8                      leErCount; /* count either 0 or 8 */
    CsrUint16                    *leEr; /* count either 0 or 8 */
    CsrUint8                      leIrCount; /* count either 0 or 8 */
    CsrUint16                    *leIr; /* count either 0 or 8 */
    CsrUint16                     leSmDivState;
    CsrUint32                     leSmSignCounter;
} CsrBtCmGetSecurityConfRes;

typedef struct
{
    CsrBtCmPrim                   type;
    CsrBtDeviceAddr               deviceAddr;
    CsrBtAddressType              deviceAddrType;
    CsrBtTransportType            transportType;
    CsrUint16                     encryptType;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmEncryptChangeInd;

typedef struct
{
    dm_prim_t               type;
    CsrSchedQid             phandle;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint16               operation;
} CsrBtCmAlwaysMasterDevicesReq;

typedef struct
{
    CsrBtCmPrim         type;
    CsrBtDeviceAddr     deviceAddr;
    CsrBtResultCode     resultCode;
    CsrBtSupplier       resultSupplier;
} CsrBtCmAlwaysMasterDevicesCfm;

typedef struct
{
    dm_prim_t               type; /* CSR_BT_CM_SM_LE_SECURITY_REQ */
    CsrBtTypedAddr          addr;  
    CsrUint16               context;
    CsrUint16               securityRequirements;
    CsrUint16               l2caConFlags;
} CsrBtCmSmLeSecurityReq;

typedef struct
{
    dm_prim_t               type; /* CSR_BT_CM_SM_SET_ENCRYPTION_KEY_SIZE_REQ */
    CsrUint8                minKeySize;
    CsrUint8                maxKeySize;
} CsrBtCmSmSetEncryptionKeySizeReq;

typedef struct
{
    CsrBtCmPrim         type;
    CsrBtConnId         btConnId;                      /* Global Bluetooth connection ID */
} CsrBtCmDataBufferEmptyReq;

typedef struct
{
    CsrBtCmPrim         type;
    CsrUint16           context;
} CsrBtCmDataBufferEmptyCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_SCAN_REQ */
    CsrSchedQid                   appHandle;
    CsrUint16                     context;
    CsrUint8                      mode; /* use CSR_BT_CM_LE_MODE_ */
    CsrUint8                      scanType;
    CsrUint16                     scanInterval;
    CsrUint16                     scanWindow;
    CsrUint8                      scanningFilterPolicy;
    CsrUint8                      filterDuplicates;
    CsrUint16                     addressCount;
    CsrBtTypedAddr                *addressList;
} CsrBtCmLeScanReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_SCAN_CFM */
    CsrUint16                     context;  
    CsrUint8                      scanMode; /* use CSR_BT_CM_LE_MODE_ */
    CsrBool                       whiteListEnable; /* TRUE enable, FALSE disable */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeScanCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_ADVERTISE_REQ */
    CsrSchedQid                   appHandle;
    CsrUint16                     context;
    CsrUint8                      mode; /* use CSR_BT_CM_LE_MODE_ */
    CsrUint32                     paramChange; /* use CSR_BT_CM_LE_PARCHG_ */ 
    CsrUint8                      advertisingDataLength; /* max 31 */
    CsrUint8                     *advertisingData;
    CsrUint8                      scanResponseDataLength; /* max 31 */
    CsrUint8                     *scanResponseData;
    CsrUint16                     advIntervalMin;
    CsrUint16                     advIntervalMax;
    CsrUint8                      advertisingType;
    CsrUint8                      advertisingChannelMap;
    CsrUint8                      advertisingFilterPolicy;
    CsrUint16                     whitelistAddrCount;
    CsrBtTypedAddr               *whitelistAddrList;
    CsrBtTypedAddr                address;
} CsrBtCmLeAdvertiseReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_ADVERTISE_CFM */
    CsrUint16                     context;
    CsrUint8                      advMode; /* use CSR_BT_CM_LE_MODE_ */
    CsrUint8                      advType;
    CsrBool                       whiteListEnable; /* TRUE enable, FALSE disable */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeAdvertiseCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_REPORT_IND */
    CsrUint8                      eventType;
    CsrBtTypedAddr                address; /* current address */
    CsrBtTypedAddr                permanentAddress; /* permanent address (resolved) */
    CsrUint8                      lengthData;
    CsrUint8                      data[CSR_BT_CM_LE_MAX_REPORT_LENGTH];
    CsrInt8                       rssi;
} CsrBtCmLeReportInd;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_CONNPARAM_REQ */
    CsrSchedQid                   appHandle;
    CsrUint16                     scanInterval;            /* These six parameters correspond to */
    CsrUint16                     scanWindow;              /* those in HCI_ULP_CREATE_CONNECTION_T */
    CsrUint16                     connIntervalMin;         /* and are used to fill that primitive */
    CsrUint16                     connIntervalMax;         /* when it is created. If they are not */
    CsrUint16                     connLatency;             /* set explicitly here then the Device */
    CsrUint16                     supervisionTimeout;      /* Manager will choose defaults. */
    CsrUint16                     connLatencyMax;          /* The 3 parameters are used to define the conditions */
    CsrUint16                     supervisionTimeoutMin;   /* to accept a  */
    CsrUint16                     supervisionTimeoutMax;   /* L2CA_CONNECTION_PAR_UPDATE_REQ_T*/
} CsrBtCmLeConnparamReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_CONNPARAM_CFM */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeConnparamCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_CONNPARAM_UPDATE_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                address;
    CsrUint16                     connIntervalMin;
    CsrUint16                     connIntervalMax;
    CsrUint16                     connLatency;
    CsrUint16                     supervisionTimeout;
    CsrUint16                     minimumCeLength;
    CsrUint16                     maximumCeLength;
} CsrBtCmLeConnparamUpdateReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_CONNPARAM_UPDATE_CMP_IND */
    CsrBtTypedAddr                address;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeConnparamUpdateCmpInd;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_IND */
    CsrBtTypedAddr                address; /* Bluetooth device address  of remote device */
    l2ca_identifier_t             l2caSignalId; /* L2CAP Signal ID */
    CsrUint16                     connIntervalMin; /* Minimum allowed connection interval */
    CsrUint16                     connIntervalMax; /* Maximum allowed connection interval */
    CsrUint16                     connLatency; /* connection slave latency */
    CsrUint16                     supervisionTimeout; /* link timeout value */
} CsrBtCmLeAcceptConnparamUpdateInd;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_ACCEPT_CONNPARAM_UPDATE_RES */
    CsrBtTypedAddr                address; /* Bluetooth device address  of remote device */
    l2ca_identifier_t             l2caSignalId; /* L2CAP Signal ID */
    CsrUint16                     connIntervalMin; /* Minimum allowed connection interval */
    CsrUint16                     connIntervalMax; /* Maximum allowed connection interval */
    CsrUint16                     connLatency; /* connection slave latency */
    CsrUint16                     supervisionTimeout; /* link timeout value */
    CsrBool                       accept; /* TRUE - if parameters are acceptable, FALSE - non-acceptable  */ 
} CsrBtCmLeAcceptConnparamUpdateRes;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_READ_ENCRYPTION_KEY_SIZE_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                address;
    CsrUint16                     context;
} CsrBtCmReadEncryptionKeySizeReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_READ_ENCRYPTION_KEY_SIZE_CFM */
    CsrBtTypedAddr                address;
    CsrUint16                     keySize;
    CsrUint16                     context;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmReadEncryptionKeySizeCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_REQ */
    CsrSchedQid                   appHandle;
    CsrUint16                     context;
} CsrBtCmReadAdvertisingChTxPowerReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_READ_ADVERTISING_CH_TX_POWER_CFM */
    CsrInt8                       txPower; /* Tx power in dBm */
    CsrUint16                     context;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmReadAdvertisingChTxPowerCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_RECEIVER_TEST_REQ */
    CsrSchedQid                   appHandle;
    CsrUint8                      rxFrequency; /* n=(f-2402)/2, range 0x00-0x27 */
} CsrBtCmLeReceiverTestReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_RECEIVER_TEST_CFM */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;   
} CsrBtCmLeReceiverTestCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_TRANSMITTER_TEST_REQ */
    CsrSchedQid                   appHandle;
    CsrUint8                      txFrequency; /* n=(f-2402)/2, range 0x00-0x27 */
    CsrUint8                      lengthOfTestData; /* range 0x00-0x25 */
    CsrUint8                      packetPayload; /* Pattern code, see BT4.0 volume 2 part E section 7.8.30 */
} CsrBtCmLeTransmitterTestReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_TRANSMITTER_TEST_CFM */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;   
} CsrBtCmLeTransmitterTestCfm;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_TEST_END_REQ */
    CsrSchedQid                   appHandle;
} CsrBtCmLeTestEndReq;

typedef struct
{
    CsrBtCmPrim                   type; /* Identity: CSR_BT_CM_LE_TEST_END_CFM */
    CsrUint16                     numberOfPackets; /* number of packets Rx'ed, 0 for Tx */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;   
} CsrBtCmLeTestEndCfm;

typedef struct
{
    CsrBtCmPrim                   type;             /* Identity: CM_LE_ENHANCED_RECEIVER_TEST_REQ */
    CsrSchedQid                   appHandle;
    CsrUint8                      rxFrequency;      /* n=(f-2402)/2, range 0x00-0x27 */
    CsrUint8                      phy;               /* Receiver set to use,
                                                       0x01 - LE 1M PHY,
                                                       0x02 - LE 2M PHY,
                                                       0x03 - LE Coded PHY */
    CsrUint8                      modIndex;         /* Assume transmitter will have 
                                                       0x00 - Standard modulation index
                                                       0x01 - Stable modulation index */
} CmLeEnhancedReceiverTestReq;

typedef struct
{
    CsrBtCmPrim                   type;             /* Identity: CM_LE_ENHANCED_RECEIVER_TEST_CFM */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CmLeEnhancedReceiverTestCfm;

typedef struct
{
    CsrBtCmPrim                   type;             /* Identity: CM_LE_ENHANCED_TRANSMITTER_TEST_REQ */
    CsrSchedQid                   appHandle;
    CsrUint8                      txFrequency;      /* n=(f-2402)/2, range 0x00-0x27 */
    CsrUint8                      lengthOfTestData; /* range 0x00-0x25 */
    CsrUint8                      packetPayload;    /* Pattern code, see BT4.0 volume 2 part E section 7.8.30 */
    CsrUint8                      phy;              /* Transmitter set to use,
                                                       0x01 - LE 1M PHY,
                                                       0x02 - LE 2M PHY,
                                                       0x03 - LE Coded PHY with S=8 data coding,
                                                       0x04 - LE Coded PHY with S=2 data coding */
} CmLeEnhancedTransmitterTestReq;

typedef struct
{
    CsrBtCmPrim                   type;             /* Identity: CM_LE_ENHANCED_TRANSMITTER_TEST_CFM */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CmLeEnhancedTransmitterTestCfm;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CSR_BT_CM_L2CA_GET_CHANNEL_INFO_REQ */
    CsrBtConnId                   btConnId;        /* Global Bluetooth connection ID */
    CsrSchedQid                   appHandle;       /* ID of the app to answer to */
} CsrBtCmL2caGetChannelInfoReq;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CSR_BT_CM_L2CA_GET_CHANNEL_INFO_CFM */
    CsrBtConnId                   btConnId;        /* Global Bluetooth connection ID */
    CsrUint16                     aclHandle;       /* ACL connection ID */
    CsrUint16                     remoteCid;       /* remote CID */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;   
} CsrBtCmL2caGetChannelInfoCfm;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CM_L2CA_PING_REQ */
    CsrSchedQid                   appHandle;
    CsrBdAddr                     address;
    CsrUint16                     lengthData;
    CsrUint8                      *data;
    CsrUint16                     context;
    CsrUint32                     flags;
} CmL2caPingReq;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CM_L2CA_PING_CFM */
    CsrSchedQid                   appHandle;
    CsrBdAddr                     address;
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
    CsrUint16                     lengthData;
    CsrUint8                      *data;
    CsrUint16                     context;
    CsrUint32                     flags;
} CmL2caPingCfm;

#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CM_L2CA_PING_IND */
    CsrSchedQid                   appHandle;
    CsrUint8                      identifier;      /* L2CA_PING_REQ Identifier */
    CsrBdAddr                     address;
    CsrUint16                     lengthData;
    CsrUint8                      *data;
} CmL2caPingInd;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CM_L2CA_PING_RSP */
    CsrUint8                      identifier;      /* Should be same as CM_L2CA_PING_IND */
    CsrBdAddr                     address;
    CsrUint16                     lengthData;
    CsrUint8                      *data;
} CmL2caPingRsp;

#endif

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_PHYSICAL_LINK_STATUS_REQ */
    CsrBtTypedAddr                address;  /* Peer address */
    CsrBool                       radioType;/* TRUE = LE, FALSE = BREDR */
    CsrBool                       status;   /* TRUE = connected, FALSE = disconnected */   
} CsrBtCmLePhysicalLinkStatusReq;

typedef struct
{
    CsrBtCmPrim                   type;        /* Identity: CSR_BT_CM_HIGH_PRIORITY_DATA_IND */
    CsrBtDeviceAddr               deviceAddr;  /* Peer address */
    CsrBool                       start;       /* TRUE = start sending high priority data, FALSE = stopped sending high priority data */
} CsrBtCmHighPriorityDataInd;

typedef struct
{
    CsrBtCmPrim                   type;            /* Identity: CSR_BT_CM_L2CA_AMP_MOVE_IND */
    CsrBtConnId                   btConnId;        /* Global Bluetooth connection ID */
    CsrBtAmpController            localControl;    /*!< Local controller ID actually used */  
    CsrUint16                     context;
} CsrBtCmL2caAmpMoveInd;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_LOCK_SM_QUEUE_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                address;  /* Peer address */
} CsrBtCmLeLockSmQueueReq;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_UNLOCK_SM_QUEUE_REQ */
    CsrBtTypedAddr                address;  /* Peer address */
} CsrBtCmLeUnlockSmQueueReq;

typedef CsrBtCmLeUnlockSmQueueReq CsrBtCmLeLockSmQueueInd;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                address;  /* Peer Address */
}CsrBtCmLeReadRemoteUsedFeaturesReq;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_READ_REMOTE_USED_FEATURES_CFM */
    CsrBtTypedAddr                address;  /* Peer Address */
    CsrUint8                      remoteLeFeatures[8];
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
}CsrBtCmLeReadRemoteUsedFeaturesCfm;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_REQ */
    CsrSchedQid                   appHandle;
}CsrBtCmLeReadLocalSupportedFeaturesReq;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_READ_LOCAL_SUPPORTED_FEATURES_CFM */
    CsrUint8                      localLeFeatures[8];
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
}CsrBtCmLeReadLocalSupportedFeaturesCfm;

typedef struct
{
    CsrBtCmPrim                   type;     /* Identity: CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_REQ */
    CsrSchedQid                   appHandle;
} CsrBtCmLeReadResolvingListSizeReq;

typedef struct
{
    CsrBtCmPrim                   type;              /* Identity: CSR_BT_CM_LE_READ_RESOLVING_LIST_SIZE_CFM */
    CsrUint8                      resolvingListSize; /* Size of controller's resolving list */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeReadResolvingListSizeCfm;

typedef struct
{
    CsrBtCmPrim                   type;              /* Identity: CSR_BT_CM_LE_SET_PRIVACY_MODE_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                peerIdAddress;     /* Peer Identity Address info */
    CsrBtPrivacyMode              privacyMode;       /* Privacy mode to be set either Network[0x00] or Device[0x01] */
} CsrBtCmLeSetPrivacyModeReq;

typedef CsrBtCmStdCfm CsrBtCmLeSetPrivacyModeCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_REQ */
    CsrSchedQid                   appHandle;
    CsrBtOwnAddressType           ownAddressType;/* Own address type[0x00, 0x01, 0x02, 0x03] to be used during GAP procedures */
} CsrBtCmLeSetOwnAddressTypeReq;

typedef CsrBtCmStdCfm CsrBtCmLeSetOwnAddressTypeCfm;

typedef struct
{
    CsrBtCmPrim                   type;         /* Identity */
    CsrBtOwnAddressType           addressType;  /* Own address type[0x00, 0x01, 0x02, 0x03] */
} CsrBtCmLeOwnAddressTypeChangedInd;

typedef struct
{
    CsrBtCmPrim                   type;         /* Identity: CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_REQ */
    CsrSchedQid                   appHandle;
    CsrUint16                     timeout;      /* Private address timeout value. Range allowed : 0x0001(1 sec) - 0xA1B8(~11.5 hours) */
} CsrBtCmLeSetPvtAddrTimeoutReq;

typedef CsrBtCmStdCfm CsrBtCmLeSetPvtAddrTimeoutCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_SET_STATIC_ADDRESS_REQ */
    CsrSchedQid                   appHandle;
    CsrBtDeviceAddr               staticAddress; /* Static address to be used locally for the current power cycle */
} CsrBtCmLeSetStaticAddressReq;

typedef CsrBtCmStdCfm CsrBtCmLeSetStaticAddressCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_READ_RANDOM_ADDRESS_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                idAddress;     /* Peer Identity Address info used to retrieve current local/peer RPA */
    CsrBtDeviceFlag               flag;          /* Flag which defines RPA to be retrieved either for local(0x01) or peer(0x02) device */
} CsrBtCmLeReadRandomAddressReq;

typedef struct
{
    CsrBtCmPrim                   type;       /* Identity: CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM */
    CsrBtTypedAddr                idAddress;  /* Peer Identity Address asked for */
    CsrUint8                      flag;       /* RPA is whether for Local or peer device */
    CsrBtDeviceAddr               rpa;        /* Retrieved current local/peer device Resolvable Private address */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeReadRandomAddressCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_READ_LOCAL_IRK_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTypedAddr                remoteAddress; /* Remote device address info used to retrieve local IRK */
} CsrBtCmLeReadLocalIrkReq;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_READ_LOCAL_IRK_CFM */
    CsrBtTypedAddr                remoteAddress; /* Remote device address */
    CsrUint16                     irk[8];        /* Local IRK */
    CsrBtResultCode               resultCode;
    CsrBtSupplier                 resultSupplier;
} CsrBtCmLeReadLocalIrkCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_SIRK_OPERATION_REQ */
    CsrSchedQid                   appHandle;
    CsrBtTpdAddrT                 tpAddrt;
    CsrUint16                     flags;
    CsrUint8                      sirkKey[16];
} CsrBtCmLeSirkOperationReq;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_SIRK_OPERATION_CFM */
    CsrBtTpdAddrT                 tpAddrt;
    CsrBtResultCode               resultCode;
    CsrUint8                      sirkKey[16];
} CsrBtCmLeSirkOperationCfm;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_LE_SET_DATA_RELATED_ADDRESS_CHANGES_REQ */
    CsrSchedQid                   appHandle;
    CsrUint8                      advHandle;
    CsrUint8                      flags;
    CsrUint8                      changeReasons;
} CsrBtCmLeSetDataRelatedAddressChangesReq;

typedef struct 
{
    CsrBtCmPrim             type;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
    CsrUint8                advHandle;
} CsrBtCmLeSetDataRelatedAddressChangesCfm;

typedef struct
{
    CsrBtCmPrim               type;            /* Identity: CSR_BT_CM_ACL_CLOSE_CFM */
    CsrBtTypedAddr            deviceAddr;      /* Remote device address */
    hci_error_t               reason;
    CsrUint16                 flags;
} CsrBtCmAclCloseCfm;

#ifdef CSR_BT_ISOC_ENABLE
#define CM_ISOC_TYPE_UNICAST DM_ISOC_TYPE_UNICAST
#define CM_ISOC_TYPE_BROADCAST DM_ISOC_TYPE_BROADCAST
#else
#define HCI_ULP_BROADCAST_CODE_SIZE 16
#endif
#define CM_MAX_SUPPORTED_CIS          ((CsrUint8)0x08)
#define CM_CODEC_ID_SIZE              ((CsrUint8)0x05) /* Defined in spec */

typedef CsrUint8 PhysicalTransport;
#define TP_ADDR_BREDR_ACL              ((PhysicalTransport)0x00)
#define TP_ADDR_LE_ACL                 ((PhysicalTransport)0x01)
#define TP_ADDR_NO_PHYSICAL_TRANSPORT  ((PhysicalTransport)0xFF)

typedef struct
{
    CsrBtTypedAddr      addrt;
    PhysicalTransport  tp_type;
} CsrBtTransportAddr;

typedef struct
{
    CsrBtCmPrim          type;           /* Identity: CSR_BT_CM_ISOC_REGISTER_REQ */
    CsrSchedQid          appHandle;
    CsrUint16            isoc_type;      /* type of isochronous for registration */
} CsrBtCmIsocRegisterReq;

typedef CsrBtCmIsocRegisterReq CmIsocRegisterReq;

typedef struct
{
    CsrBtCmPrim          type;           /* Identity: CSR_BT_CM_ISOC_REGISTER_CFM */
    CsrBtResultCode      resultCode;
    CsrUint16            isoc_type;      /* type of isochronous registered */
} CsrBtCmIsocRegisterCfm;

typedef CsrBtCmIsocRegisterCfm CmIsocRegisterCfm;

typedef struct
{
    CsrUint8     cis_id;                     /* Unique CIS identifier in given cig_id */
    CsrUint16    max_sdu_m_to_s;             /* Maximum SDU Size from master host */
    CsrUint16    max_sdu_s_to_m;             /* Maximum SDU Size from slave host */
    CsrUint8     phy_m_to_s;                 /* PHY from master */
    CsrUint8     phy_s_to_m;                 /* PHY from slave */
    CsrUint8     rtn_m_to_s;                 /* Retransmission number from master to slave */
    CsrUint8     rtn_s_to_m;                 /* Retransmission number from slave to master */
} CmCisConfig;

typedef struct
{
    CsrBtCmPrim          type;                /* Identity: CSR_BT_CM_ISOC_CONFIGURE_CIG_REQ */
    CsrSchedQid          appHandle;
    CsrUint24            sdu_interval_m_to_s; /* Time interval between the start of consecutive SDUs */
    CsrUint24            sdu_interval_s_to_m; /* Time interval between the start of consecutive SDUs */
    CsrUint16            max_transport_latency_m_to_s;   /* Maximum transport latency from master */
    CsrUint16            max_transport_latency_s_to_m;   /* Maximum transport latency from slave */
    CsrUint8             cig_id;              /* Zero for new configuration, valid for re-configuration */
    CsrUint8             sca;                 /* Sleep clock accuracy */
    CsrUint8             packing;             /* Interleaved, Sequential placement of packets */
    CsrUint8             framing;             /* Indicates the format: framed or unframed */
    CsrUint8             cis_count;           /* Number of CIS under CIG */
    CmCisConfig         *cis_config[CM_MAX_SUPPORTED_CIS];
                                                /*  Array of pointers to cis configuration */
} CsrBtCmIsocConfigureCigReq;

typedef CsrBtCmIsocConfigureCigReq CmIsocConfigureCigReq;

typedef struct
{
    CsrBtCmPrim                type;        /* Identity: CSR_BT_CM_ISOC_CONFIGURE_CIG_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   cig_id;      /* CIG identifier */
    CsrUint8                   cis_count;   /* number of cis configured */
    hci_connection_handle_t    cis_handles[CM_MAX_SUPPORTED_CIS]; /* cis handles for cig_id,
                                                                    * contains cis_count valid elements */
} CsrBtCmIsocConfigureCigCfm;

typedef CsrBtCmIsocConfigureCigCfm CmIsocConfigureCigCfm;

typedef CsrBtCmIsocConfigureCigCfm CsrBtCmIsocConfigureCigTestCfm;

typedef CsrBtCmIsocConfigureCigTestCfm CmIsocConfigureCigTestCfm;

typedef struct
{
    CsrBtCmPrim          type;           /* Identity: CSR_BT_CM_ISOC_REMOVE_CIG_REQ */
    CsrSchedQid          appHandle;
    CsrUint8             cig_id;         /* CIG Identifier to be removed */
} CsrBtCmIsocRemoveCigReq;

typedef CsrBtCmIsocRemoveCigReq CmIsocRemoveCigReq;

typedef struct
{
    CsrBtCmPrim                  type;        /* Identity: CSR_BT_CM_ISOC_REMOVE_CIG_CFM */
    CsrBtResultCode              resultCode;
    CsrUint8                     cig_id;      /* removed CIG Identifier */
} CsrBtCmIsocRemoveCigCfm;

typedef CsrBtCmIsocRemoveCigCfm CmIsocRemoveCigCfm;

typedef struct
{
    hci_connection_handle_t    cis_handle;    /* cis handles for isoc connection */
    CsrBtTransportAddr         tp_addrt;      /* Transport bluetooth device address */
} CmCisConnection;

typedef struct
{
    CsrBtCmPrim          type;              /* Identity: CSR_BT_CM_ISOC_CIS_CONNECT_REQ */
    CsrSchedQid          appHandle;
    context_t            con_context;
    CsrUint8             cis_count;         /* number of CIS connections */
    CmCisConnection      *cis_conn[CM_MAX_SUPPORTED_CIS];
                                               /* list of cis handle and addresses to connect */
} CsrBtCmIsocCisConnectReq;

typedef CsrBtCmIsocCisConnectReq CmIsocCisConnectReq;

typedef struct
{
    CsrBtCmPrim                  type;            /* Identity: CSR_BT_CM_ISOC_CIS_CONNECT_IND */
    CsrBtTransportAddr           tp_addrt;        /* Transport bd address of remote device */
    hci_connection_handle_t      cis_handle;      /* CIS handle */
    CsrUint8                     cig_id;          /* CIG Id received from remote device */
    CsrUint8                     cis_id;          /* CIS Id received from remote device */
} CsrBtCmIsocCisConnectInd;

typedef CsrBtCmIsocCisConnectInd CmIsocCisConnectInd;

typedef struct
{
    CsrBtCmPrim                type;              /* Identity: CSR_BT_CM_ISOC_CIS_CONNECT_RSP */
    CsrSchedQid                appHandle;
    context_t                  con_context;
    hci_connection_handle_t    cis_handle;        /* CIS handle from connect ind */
    hci_return_t               status;            /* hci status */
} CsrBtCmIsocCisConnectRsp;

typedef CsrBtCmIsocCisConnectRsp CmIsocCisConnectRsp;

/* ISOC CIS connection indication notification event. Subscribe via the
 * CM_EVENT_MASK_SUBSCRIBE_ISOC_CIS_CONNECT_IND event mask.
 *
 * Note: This event is for information only.
 *       Application shall not respond to this event with CmIsocCisConnectRsp 
 *       and shall be ignored.
 */
typedef CsrBtCmIsocCisConnectInd CmIsocNotifyCisConnectInd;

typedef struct
{
    CsrUint24 cig_sync_delay;
    CsrUint24 cis_sync_delay;
    CsrUint24 transport_latency_m_to_s;
    CsrUint24 transport_latency_s_to_m;
    CsrUint16 max_pdu_m_to_s;
    CsrUint16 max_pdu_s_to_m;
    CsrUint16 iso_interval;
    CsrUint8  phy_m_to_s;
    CsrUint8  phy_s_to_m;
    CsrUint8  nse;
    CsrUint8  bn_m_to_s;
    CsrUint8  bn_s_to_m;
    CsrUint8  ft_m_to_s;
    CsrUint8  ft_s_to_m;
}CmCisParam;

typedef struct
{
    CsrBtCmPrim                type;       /* Identity: CSR_BT_CM_ISOC_CIS_CONNECT_CFM */
    CsrBtResultCode            resultCode;
    CsrBtTransportAddr         tp_addr;    /* Transport bluetooth device address */
    hci_connection_handle_t    cis_handle; /* CIS handle for cis establishment */
    CmCisParam                 cis_params; /* CIS parameters agreed during cis establishment */
} CsrBtCmIsocCisConnectCfm;

typedef CsrBtCmIsocCisConnectCfm CmIsocCisConnectCfm;

typedef struct
{
    CsrBtCmPrim                type;           /* Identity: CSR_BT_CM_ISOC_CIS_DISCONNECT_REQ */
    CsrSchedQid                appHandle;      /* Destination phandle */
    hci_connection_handle_t    cis_handle;    /* CIS handle for disconnection */
    hci_return_t               reason;        /* Reason for command */
} CsrBtCmIsocCisDisconnectReq;

typedef CsrBtCmIsocCisDisconnectReq CmIsocCisDisconnectReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_ISOC_CIS_DISCONNECT_CFM */
    CsrBtResultCode            resultCode;
    hci_connection_handle_t    cis_handle;    /* CIS handle for disconnection */
} CsrBtCmIsocCisDisconnectCfm;

typedef CsrBtCmIsocCisDisconnectCfm CmIsocCisDisconnectCfm;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_ISOC_CIS_DISCONNECT_IND */
    hci_connection_handle_t    cis_handle;    /* CIS handle for disconnection */
    CsrUint8                   reason;
} CsrBtCmIsocCisDisconnectInd;

typedef CsrBtCmIsocCisDisconnectInd CmIsocCisDisconnectInd;

typedef struct
{
    CsrBtCmPrim                 type;           /* Identity: CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_REQ */
    CsrSchedQid                 appHandle;      /* Destination phandle */
    CsrUint24                   controller_delay;
    hci_connection_handle_t     handle;        /* CIS or BIS handle */
    CsrUint8                    data_path_direction;
    CsrUint8                    data_path_id;
    CsrUint8                    codec_id[5];
    CsrUint8                    codec_config_length;
    CsrUint8                    *codec_config_data;
}  CsrBtCmIsocSetupIsoDataPathReq;

typedef CsrBtCmIsocSetupIsoDataPathReq CmIsocSetupIsoDataPathReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_CFM */
    CsrBtResultCode            resultCode;
    hci_connection_handle_t    handle;        /* CIS handle */
} CsrBtCmIsocSetupIsoDataPathCfm;

typedef CsrBtCmIsocSetupIsoDataPathCfm CmIsocSetupIsoDataPathCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_REQ */
    CsrSchedQid                appHandle;          /* Destination phandle */
    hci_connection_handle_t    handle;              /* CIS handle */
    CsrUint8                   data_path_direction; /* Direction of the path to be removed */
} CsrBtCmIsocRemoveIsoDataPathReq;

typedef CsrBtCmIsocRemoveIsoDataPathReq CmIsocRemoveIsoDataPathReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_ISOC_REMOVE_ISO_DATA_PATH_CFM */
    hci_connection_handle_t    handle;        /* CIS  handle */
    CsrBtResultCode            resultCode;
} CsrBtCmIsocRemoveIsoDataPathCfm;

typedef CsrBtCmIsocRemoveIsoDataPathCfm CmIsocRemoveIsoDataPathCfm;

typedef struct
{
    CsrUint16    max_sdu_m_to_s;             /* Maximum SDU Size from master host */
    CsrUint16    max_sdu_s_to_m;             /* Maximum SDU Size from slave host */
    CsrUint16    max_pdu_m_to_s;             /* Maximum PDU Size from master host */
    CsrUint16    max_pdu_s_to_m;             /* Maximum PDU Size from slave host */
    CsrUint8     cis_id;                     /* Unique CIS identifier in given cig_id */
    CsrUint8     nse;                        /* Max no of sub events for each CIS */
    CsrUint8     phy_m_to_s;                 /* PHY from master */
    CsrUint8     phy_s_to_m;                 /* PHY from slave */
    CsrUint8     bn_m_to_s;                  /* Burst number from master to slave */
    CsrUint8     bn_s_to_m;                  /* Burst number from slave to master */
} CmCisTestConfig;

typedef struct
{
    CsrBtCmPrim          type;                /* Identity: CSR_BT_CM_ISOC_CONFIGURE_CIG_TEST_REQ */
    CsrSchedQid          appHandle;
    CsrUint24            sdu_interval_m_to_s; /* Time interval between the start of consecutive SDUs */
    CsrUint24            sdu_interval_s_to_m; /* Time interval between the start of consecutive SDUs */
    CsrUint16            iso_interval;        /* Time b/w two consecutive CIS anchor points */
    CsrUint8             cig_id;              /* Zero for new configuration, valid for re-configuration */
    CsrUint8             ft_m_to_s;           /* Flush timeout at master side */
    CsrUint8             ft_s_to_m;           /* Flush timeout at slave side */
    CsrUint8             sca;                 /* Sleep clock accuracy */
    CsrUint8             packing;             /* Interleaved, Sequential placement of packets */
    CsrUint8             framing;             /* Indicates the format: framed or unframed */
    CsrUint8             cis_count;           /* Number of CIS under CIG */
    CmCisTestConfig      *cis_test_config[CM_MAX_SUPPORTED_CIS];
                                                /* Array of pointers to cis test configuration */
} CsrBtCmIsocConfigureCigTestReq;

typedef CsrBtCmIsocConfigureCigTestReq CmIsocConfigureCigTestReq;

#define CM_DM_BROADCAST_CODE_SIZE     HCI_ULP_BROADCAST_CODE_SIZE

typedef struct
{
    CsrUint24    sdu_interval;           /* Interval of Periodic SDUs*/
    CsrUint16    max_sdu;                /* Maximum size of an SDU */
    CsrUint16    max_transport_latency;  /* Max transport latency */
    CsrUint8     rtn;                    /* Retransmission number */
    CsrUint8     phy;                    /* Preferred PHY for transmission */
    CsrUint8     packing;                /* Sequential or Interleaved */
    CsrUint8     framing;                /* Framed or Unframed */
} CmBigConfigParam;

typedef struct
{
    CsrBtCmPrim              type;           /* Identity: CSR_BT_CM_ISOC_CREATE_BIG_REQ */
    CsrSchedQid              appHandle;      /* Destination phandle */
    CmBigConfigParam         big_config;     /* Big Params */
    CsrUint8                 big_handle;     /* Host identifier of BIG */
    CsrUint8                 adv_handle;     /* Handle associated with PA */
    CsrUint8                 num_bis;        /* Total number of BISes in BIG */
    CsrUint8                 encryption;    /* Encrypted BIS data if 1 */
    CsrUint8                 broadcast_code[CM_DM_BROADCAST_CODE_SIZE]; /* Broadcast code to enc BIS payloads */
} CsrBtCmIsocCreateBigReq;

typedef CsrBtCmIsocCreateBigReq CmIsocCreateBigReq;

typedef struct
{
    uint24_t                 sdu_interval;
    uint16_t                 iso_interval;
    uint8_t                  nse;
    uint16_t                 max_sdu;
    uint16_t                 max_pdu;
    uint8_t                  phy;
    uint8_t                  packing;
    uint8_t                  framing;
    uint8_t                  bn;
    uint8_t                  irc;
    uint8_t                  pto;
} CmBigTestConfigParam;

typedef struct
{
    CsrBtCmPrim              type;           /* Identity: CSR_BT_CM_ISOC_CREATE_BIG_TEST_REQ */
    CsrSchedQid              appHandle;      /* Destination phandle */
    CmBigTestConfigParam     big_config;     /* Big test Params */
    CsrUint8                 big_handle;     /* Host identifier of BIG */
    CsrUint8                 adv_handle;     /* Handle associated with PA */
    CsrUint8                 num_bis;        /* Total number of BISes in BIG */
    CsrUint8                 encryption;    /* Encrypted BIS data if 1 */
    CsrUint8                 broadcast_code[CM_DM_BROADCAST_CODE_SIZE]; /* Broadcast code to enc BIS payloads */
} CsrBtCmIsocCreateBigTestReq;

typedef CsrBtCmIsocCreateBigTestReq CmIsocCreateBigTestReq;

typedef struct
{
    CsrUint24        transport_latency_big;/* Max time to tx SDUs of all BISes */
    CsrUint16        max_pdu;              /* Maximum size of an PDU */
    CsrUint16        iso_interval;         /* ISO interval */
    CsrUint8         phy;                  /* PHY used */
    CsrUint8         nse;                  /* Number of sub events */
    CsrUint8         bn;                   /* Burst number */
    CsrUint8         pto;                  /* Pre transmission offset */
    CsrUint8         irc;                  /* Repeated count of retransmission */
} CmBigParam;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CSR_BT_CM_ISOC_CREATE_BIG_CFM */
    CsrUint24                big_sync_delay;/* Max time to tx PDUs of all BISes */
    CmBigParam               big_params;    /* Confirmed Big Parameters */
    uint8_t                  big_handle;    /* Host identifier of BIG */
    CsrBtResultCode          resultCode;
    CsrUint8                 num_bis;       /* Number of BISes in BIG */
    hci_connection_handle_t  *bis_handles;  /* Connection handle of BISes */
} CsrBtCmIsocCreateBigCfm;

typedef CsrBtCmIsocCreateBigCfm CmIsocCreateBigCfm;
typedef CsrBtCmIsocCreateBigCfm CmIsocCreateBigTestCfm;

typedef struct
{
    CsrBtCmPrim              type;           /* Identity: CSR_BT_CM_ISOC_TERMINATE_BIG_REQ */
    CsrSchedQid              appHandle;      /* Destination phandle */
    CsrUint8                 big_handle;    /* Host identifier of BIG */
    hci_reason_t             reason;        /* Reason for BIG termination */
} CsrBtCmIsocTerminateBigReq;

typedef CsrBtCmIsocTerminateBigReq CmIsocTerminateBigReq;

typedef struct
{
    CsrBtCmPrim              type;           /* Identity: CSR_BT_CM_ISOC_TERMINATE_BIG_CFM */
    CsrUint8                 big_handle;        /* Host identifier of BIG */
    CsrBtResultCode          resultCode;
} CsrBtCmIsocTerminateBigCfm;

typedef CsrBtCmIsocTerminateBigCfm CmIsocTerminateBigCfm;

typedef struct
{
    CsrBtCmPrim      type;               /* Identity: CSR_BT_CM_ISOC_BIG_CREATE_SYNC_REQ */
    phandle_t        phandle;            /* Destination phandle */
    CsrUint16        sync_handle;        /* Sync handle of the PA */
    CsrUint16        big_sync_timeout;   /* Sync timeout of BIS PDUs */
    CsrUint8         big_handle;         /* Host identifier of BIG */
    CsrUint8         mse;                /* Maximum sub events */
    CsrUint8         encryption;         /* Encryption mode of the BIG */
    CsrUint8         broadcast_code[CM_DM_BROADCAST_CODE_SIZE];/* Broadcast code to encrypt or decrypt BIS payloads*/
    CsrUint8         num_bis;            /* Num of BISes requested */
    CsrUint8         *bis;               /* Indices corresponding to BISes */
} CsrBtCmIsocBigCreateSyncReq;

typedef CsrBtCmIsocBigCreateSyncReq CmIsocBigCreateSyncReq;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CSR_BT_CM_ISOC_BIG_CREATE_SYNC_CFM */
    CsrBtResultCode          resultCode;
    CmBigParam               big_params;    /* Confirmed Big Parameters */
    CsrUint8                 big_handle;    /* Host identifier of BIG */
    CsrUint8                 num_bis;       /* Number of BISes synchronized */
    hci_connection_handle_t  *bis_handles;  /* Connection handle of BISes */
} CsrBtCmIsocBigCreateSyncCfm;

typedef CsrBtCmIsocBigCreateSyncCfm CmIsocBigCreateSyncCfm;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_REQ */
    phandle_t                phandle;       /* Destination phandle */
    CsrUint8                 big_handle;    /* Host identifier of BIG */
} CsrBtCmIsocBigTerminateSyncReq;

typedef CsrBtCmIsocBigTerminateSyncReq CmIsocBigTerminateSyncReq;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CSR_BT_CM_ISOC_BIG_TERMINATE_SYNC_IND */
    CsrUint8                 big_handle;    /* Host identifier of BIG */
    CsrBtResultCode          resultCode;
} CsrBtCmIsocBigTerminateSyncInd;

typedef CsrBtCmIsocBigTerminateSyncInd CmIsocBigTerminateSyncInd;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CM_ISOC_READ_ISO_LINK_QUALITY_REQ */
    CsrSchedQid              appHandle;     /* Destination phandle */
    hci_connection_handle_t  handle;        /* CIS or BIS handle */
} CmIsocReadIsoLinkQualityReq;

typedef struct
{
    CsrBtCmPrim              type;          /* Identity: CM_ISOC_READ_ISO_LINK_QUALITY_CFM */
    hci_connection_handle_t  handle;        /* CIS or BIS handle */
    CsrUint32                tx_unacked_packets;       
    CsrUint32                tx_flushed_packets;
    CsrUint32                tx_last_subevent_packets;
    CsrUint32                retransmitted_packets;
    CsrUint32                crc_error_packets;
    CsrUint32                rx_unreceived_packets;
    CsrUint32                duplicate_packets;
    CsrBtResultCode          resultCode;
    CsrBtSupplier            resultSupplier;    
} CmIsocReadIsoLinkQualityCfm;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
} CsrBtCmGetAdvScanCapabilitiesReq;

typedef CsrBtCmGetAdvScanCapabilitiesReq CmGetAdvScanCapabilitiesReq;

/* The availableApi bit defines */
#define CM_ADV_SCAN_API_LEGACY 1
#define CM_ADV_SCAN_API_EXTENDED 2
#define CM_ADV_SCAN_API_PERIODIC 4
#define CM_ADV_SCAN_API_PAST_SENDER 8
#define CM_ADV_SCAN_API_PERIODIC_RECIPIENT 16
#define CM_ADV_SCAN_API_PERIODIC_ADI 32         /* LE periodic advertising/scanning API
                                                 * supports Periodic Advertising ADI feature */

/* The max length of extended and periodic advertising data */
#define CM_MAX_POTENTIAL_ADV_DATA_LEN 1650

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_GET_ADV_SCAN_CAPABILITIES_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    availableApi;
    CsrUint8                    availableAdvSets;
    CsrUint8                    stackReservedAdvSets;
    CsrUint8                    maxPeriodicSyncListSize;
    CsrUint16                   supportedPhys;
    CsrUint16                   maxPotentialSizeOfTxAdvData;
    CsrUint16                   maxPotentialSizeOfTxPeriodicAdvData;
    CsrUint16                   maxPotentialSizeOfRxAdvData;
    CsrUint16                   maxPotentialSizeOfRxPeriodicAdvData;
} CsrBtCmGetAdvScanCapabilitiesCfm;

typedef CsrBtCmGetAdvScanCapabilitiesCfm CmGetAdvScanCapabilitiesCfm;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint32                   flags;
} CsrBtCmExtAdvRegisterAppAdvSetReq;

typedef CsrBtCmExtAdvRegisterAppAdvSetReq CmExtAdvRegisterAppAdvSetReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvRegisterAppAdvSetCfm;

typedef CsrBtCmExtAdvRegisterAppAdvSetCfm CmExtAdvRegisterAppAdvSetCfm;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} CsrBtCmExtAdvUnregisterAppAdvSetReq;

typedef CsrBtCmExtAdvUnregisterAppAdvSetReq CmExtAdvUnregisterAppAdvSetReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvUnregisterAppAdvSetCfm;

typedef CsrBtCmExtAdvUnregisterAppAdvSetCfm CmExtAdvUnregisterAppAdvSetCfm;

/* Note : CmExtAdUnregisterAppAdvSetCfm prim is deprecated */
typedef CsrBtCmExtAdvUnregisterAppAdvSetCfm CmExtAdUnregisterAppAdvSetCfm;

#define CM_EXT_ADV_SID_INVALID              0x100
#define CM_EXT_ADV_SID_SHARE                0x200
#define CM_EXT_ADV_SID_ASSIGNED_BY_STACK    0x400

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_PARAMS_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint16                   advEventProperties;
    CsrUint32                   primaryAdvIntervalMin;
    CsrUint32                   primaryAdvIntervalMax;
    CsrUint8                    primaryAdvChannelMap;
    CsrUint8                    ownAddrType;
    TYPED_BD_ADDR_T             peerAddr;
    CsrUint8                    advFilterPolicy;
    CsrUint16                   primaryAdvPhy;
    CsrUint8                    secondaryAdvMaxSkip;
    CsrUint16                   secondaryAdvPhy;
    CsrUint16                   advSid;
    CsrUint32                   reserved;
} CsrBtCmExtAdvSetParamsReq;

typedef CsrBtCmExtAdvSetParamsReq CmExtAdvSetParamsReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advSid;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvSetParamsCfm;

typedef CsrBtCmExtAdvSetParamsCfm CmExtAdvSetParamsCfm;

#define CM_EXT_ADV_DATA_LENGTH_MAX       251
#define CM_EXT_ADV_DATA_BLOCK_SIZE       32
#define CM_EXT_ADV_DATA_BYTES_PTRS_MAX   8

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_DATA_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint8                    operation;
    CsrUint8                    fragPreference;
    CsrUint8                    dataLength;
    CsrUint8                    *data[CM_EXT_ADV_DATA_BYTES_PTRS_MAX];
} CsrBtCmExtAdvSetDataReq;

typedef CsrBtCmExtAdvSetDataReq CmExtAdvSetDataReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_DATA_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvSetDataCfm;

typedef CsrBtCmExtAdvSetDataCfm CmExtAdvSetDataCfm;

#define CM_EXT_ADV_SCAN_RESP_DATA_LENGTH_MAX          251
#define CM_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE          32
#define CM_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX      8

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint8                    operation;
    CsrUint8                    fragPreference;
    CsrUint8                    dataLength;
    CsrUint8                    *data[CM_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX];
} CsrBtCmExtAdvSetScanRespDataReq;

typedef CsrBtCmExtAdvSetScanRespDataReq CmExtAdvSetScanRespDataReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvSetScanRespDataCfm;

typedef CsrBtCmExtAdvSetScanRespDataCfm CmExtAdvSetScanRespDataCfm;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_ENABLE_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint8                    enable;
} CsrBtCmExtAdvEnableReq;

typedef CsrBtCmExtAdvEnableReq CmExtAdvEnableReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_ENABLE_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvEnableCfm;

typedef CsrBtCmExtAdvEnableCfm CmExtAdvEnableCfm;


typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CM_DM_EXT_ADV_GET_ADDR_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} CmDmExtAdvGetAddrReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CM_DM_EXT_ADV_GET_ADDR_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
    TYPED_BD_ADDR_T             ownAddr;
} CmDmExtAdvGetAddrCfm;


typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
} CsrBtCmExtAdvReadMaxAdvDataLenReq;

typedef CsrBtCmExtAdvReadMaxAdvDataLenReq CmExtAdvReadMaxAdvDataLenReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_READ_MAX_ADV_DATA_LEN_CFM */
    CsrBtResultCode             resultCode;
    CsrUint16                   maxAdvData;
    CsrUint16                   maxScanRespData;
    CsrUint8                    advHandle;
} CsrBtCmExtAdvReadMaxAdvDataLenCfm;

typedef CsrBtCmExtAdvReadMaxAdvDataLenCfm CmExtAdvReadMaxAdvDataLenCfm;

/* Options for generating or setting a random address for an advertising set.
   Used to set field action. */
#define CM_EXT_ADV_ADDRESS_WRITE_STATIC 0               /* Set static address */
#define CM_EXT_ADV_ADDRESS_GENERATE_STATIC 1            /* Generate and set static address */
#define CM_EXT_ADV_ADDRESS_GENERATE_NON_RESOLVABLE 2    /* Generate and set NRPA */
#define CM_EXT_ADV_ADDRESS_GENERATE_RESOLVABLE 3        /* Generate and set RPA */
#define CM_EXT_ADV_ADDRESS_WRITE_NON_RESOLVABLE 4       /* Set NRPA */
#define CM_EXT_ADV_ADDRESS_WRITE_RESOLVABLE 5           /* Set RPA */

typedef struct
{
    CsrBtCmPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_REQ */
    CsrSchedQid                 appHandle;     /* Destination phandle */
    CsrUint8                    advHandle;     /* Advertising set */
    CsrUint16                   action;        /* How to set random address */
    CsrBtDeviceAddr             randomAddr;    /* Random address to write */
} CsrBtCmExtAdvSetRandomAddrReq;

typedef CsrBtCmExtAdvSetRandomAddrReq CmExtAdvSetRandomAddrReq;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advHandle;
    CsrBtDeviceAddr             randomAddr;
} CsrBtCmExtAdvSetRandomAddrCfm;

typedef CsrBtCmExtAdvSetRandomAddrCfm CmExtAdvSetRandomAddrCfm;

/* Reasons why an advertising set has stopped advertising */
#define CM_EXT_ADV_TERMINATED_CONN          0   /* Connection established on adv set */
#define CM_EXT_ADV_TERMINATED_DURATION      1   /* Duration period has ended */
#define CM_EXT_ADV_TERMINATED_MAX_EVENT     2   /* Max event limit reached */

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CSR_BT_CM_EXT_ADV_TERMINATED_IND */
    CsrUint8                    advHandle;       /* Adv set terminated */
    CsrUint8                    reason;          /* Why it was terminated */
    TYPED_BD_ADDR_T             addrt;           /* Peer device address */
    CsrUint8                    eaEvents;        /* Completed extended advertising events */
    CsrUint8                    maxAdvSets;      /* Number of adv sets below */
    CsrUint32                   advBits;
} CsrBtCmExtAdvTerminatedInd;

typedef CsrBtCmExtAdvTerminatedInd CmExtAdvTerminatedInd;

typedef struct
{
    CsrBtCmPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SETS_INFO_REQ */
    CsrSchedQid                 appHandle;
} CsrBtCmExtAdvSetsInfoReq;

typedef CsrBtCmExtAdvSetsInfoReq CmExtAdvSetsInfoReq;

#define CM_ADV_HANDLE_FOR_LEGACY_API        ((CsrUint8) 0)
#define CM_EXT_ADV_MAX_ADV_HANDLES          ((CsrUint8) (DM_ULP_EXT_ADV_MAX_ADV_HANDLES - 1))
#define CM_EXT_ADV_MAX_REPORTED_ADV_SETS    11
#define CM_EXT_ADVERTISING_ENABLED_FLAG     ((CsrUint16) 0x0001)
#define CM_EXT_ADVERTISING_ENABLED_MASK     ((CsrUint16) 0x0001)

/* Max number of advertising sets allowed to be used by application. */
#define CM_EXT_ADV_HANDLE_INVALID                         ((CsrUint8) DM_ULP_EXT_ADV_HANDLE_INVALID)

/* Advertising set register flag */
/* Stack to assign availale adv_handle if application provided adv_handle is not available or its set to CM_EXT_ADV_HANDLE_INVALID */
#define CM_EXT_ADV_REG_SET_ASSIGNED_BY_STACK              ((CsrUint32) DM_ULP_EXT_ADV_REG_SET_ASSIGNED_BY_STACK)


typedef struct
{
    CsrUint8                    registered;
    CsrUint8                    advertising;
    CsrUint16                   info;
} CmExtAdvSetInfo;

typedef struct
{
    CsrBtCmPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_SETS_INFO_CFM */
    CsrUint16                   flags;
    CsrUint8                    numAdvSets;
    CmExtAdvSetInfo             advSets[CM_EXT_ADV_MAX_REPORTED_ADV_SETS];
} CsrBtCmExtAdvSetsInfoCfm;

typedef CsrBtCmExtAdvSetsInfoCfm CmExtAdvSetsInfoCfm;

#define CM_EXT_ADV_MAX_NUM_ENABLE   4
#define CM_EXT_ADV_N0_DURATION      0
#define CM_EXT_ADV_NO_MAX_EVENTS    0

typedef struct
{
    CsrUint8                    advHandle;
    CsrUint8                    maxEaEvents;
    CsrUint16                   duration;
} CmEnableConfig;

typedef struct
{
    CsrBtCmPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_MULTI_ENABLE_REQ */
    CsrSchedQid                 appHandle;
    CsrUint8                    enable;        /* Start/stop advertising */
    CsrUint8                    numSets;       /* How many advertising set configs in prim */
    CmEnableConfig              config[CM_EXT_ADV_MAX_NUM_ENABLE];
} CsrBtCmExtAdvMultiEnableReq;

typedef CsrBtCmExtAdvMultiEnableReq CmExtAdvMultiEnableReq;

typedef struct
{
    CsrBtCmPrim                 type;          /* Identity: CSR_BT_CM_EXT_ADV_MULTI_ENABLE_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    maxAdvSets;
    CsrUint32                   advBits;
} CsrBtCmExtAdvMultiEnableCfm;

typedef CsrBtCmExtAdvMultiEnableCfm CmExtAdvMultiEnableCfm;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_REQ */
    CsrSchedQid                appHandle;
} CsrBtCmExtScanGetGlobalParamsReq;

typedef CsrBtCmExtScanGetGlobalParamsReq CmExtScanGetGlobalParamsReq;

typedef struct
{
    CsrUint8                   scan_type;
    CsrUint16                  scan_interval;
    CsrUint16                  scan_window;
} CmScanningPhy;

#define CM_EXT_SCAN_MAX_SCANNING_PHYS 2
#define CM_EXT_SCAN_LE_1M_PHY_BIT         0x00
#define CM_EXT_SCAN_LE_2M_PHY_BIT         0x01
#define CM_EXT_SCAN_LE_CODED_PHY_BIT      0x02

#define CM_EXT_SCAN_LE_1M_PHY_BIT_MASK    0x01
#define CM_EXT_SCAN_LE_2M_PHY_BIT_MASK    0x02
#define CM_EXT_SCAN_LE_CODED_PHY_BIT_MASK 0x04

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_GET_GLOBAL_PARAMS_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   flags;
    CsrUint8                   own_address_type;
    CsrUint8                   scanning_filter_policy;
    CsrUint8                   filter_duplicates;
    CsrUint16                  scanning_phys;
    CmScanningPhy              phys[CM_EXT_SCAN_MAX_SCANNING_PHYS];
} CsrBtCmExtScanGetGlobalParamsCfm;

typedef CsrBtCmExtScanGetGlobalParamsCfm CmExtScanGetGlobalParamsCfm;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   flags;
    CsrUint8                   own_address_type;
    CsrUint8                   scanning_filter_policy;
    CsrUint8                   filter_duplicates;
    CsrUint16                  scanning_phy;
    CmScanningPhy              phys[CM_EXT_SCAN_MAX_SCANNING_PHYS];
} CsrBtCmExtScanSetGlobalParamsReq;

typedef CsrBtCmExtScanSetGlobalParamsReq CmExtScanSetGlobalParamsReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_SET_GLOBAL_PARAMS_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmExtScanSetGlobalParamsCfm;

typedef CsrBtCmExtScanSetGlobalParamsCfm CmExtScanSetGlobalParamsCfm;

#define CM_EXT_SCAN_MAX_REG_AD_TYPES 10

/* Valid values for adv_filter field */
#define CM_EXT_SCAN_ADV_FILTER_NONE 0
#define CM_EXT_SCAN_ADV_FILTER_BLOCK_ALL 1
#define CM_EXT_SCAN_ADV_FILTER_LEGACY 2
#define CM_EXT_SCAN_ADV_FILTER_EXTENDED 3
#define CM_EXT_SCAN_ADV_FILTER_ASSOCIATED_PERIODIC 4

/* Valid values for ad_structure_filter field */
#define CM_EXT_SCAN_AD_STRUCT_FILTER_NONE 0

/* Sub fields setting for adv_filter or ad_structure_filter when not used */
#define CM_EXT_SCAN_SUB_FIELD_INVALID 0


typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   flags;
    CsrUint16                  adv_filter;
    CsrUint16                  adv_filter_sub_field1;
    CsrUint32                  adv_filter_sub_field2;
    CsrUint16                  ad_structure_filter;
    CsrUint16                  ad_structure_filter_sub_field1;
    CsrUint32                  ad_structure_filter_sub_field2;
    CsrUint8                   num_reg_ad_types;
    CsrUint8                   reg_ad_types[CM_EXT_SCAN_MAX_REG_AD_TYPES];
} CsrBtCmExtScanRegisterScannerReq;

typedef CsrBtCmExtScanRegisterScannerReq CmExtScanRegisterScannerReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_REGISTER_SCANNER_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   scan_handle;
} CsrBtCmExtScanRegisterScannerCfm;

typedef CsrBtCmExtScanRegisterScannerCfm CmExtScanRegisterScannerCfm;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   scan_handle;
} CsrBtCmExtScanUnregisterScannerReq;

typedef CsrBtCmExtScanUnregisterScannerReq CmExtScanUnregisterScannerReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_UNREGISTER_SCANNER_CFM */
    CsrSchedQid                appHandle;
    CsrBtResultCode            resultCode;
} CsrBtCmExtScanUnregisterScannerCfm;

typedef CsrBtCmExtScanUnregisterScannerCfm CmExtScanUnregisterScannerCfm;

typedef struct
{
    CsrUint8                   use_scan_defaults;
    CsrUint8                   scan_type;
    CsrUint16                  scan_interval;
    CsrUint8                   scan_window;
} CmScanPhy;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   scan_handle;
    CsrUint8                   use_only_global_params;
    CsrUint16                  scanning_phys;
    CmScanPhy                  phys[CM_EXT_SCAN_MAX_SCANNING_PHYS];
} CsrBtCmExtScanConfigureScannerReq;

typedef CsrBtCmExtScanConfigureScannerReq CmExtScanConfigureScannerReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_CONFIGURE_SCANNER_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmExtScanConfigureScannerCfm;

typedef CsrBtCmExtScanConfigureScannerCfm CmExtScanConfigureScannerCfm;

typedef struct
{
    CsrUint8         scan_handle;
    CsrUint16        duration;
} CmScanners;

#define CM_EXT_SCAN_MAX_SCANNERS 5
#define CM_EXT_SCAN_FOREVER 0


typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   enable;
    CsrUint8                   num_of_scanners;
    CmScanners                 scanners[CM_EXT_SCAN_MAX_SCANNERS];
} CsrBtCmExtScanEnableScannersReq;

typedef CsrBtCmExtScanEnableScannersReq CmExtScanEnableScannersReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_ENABLE_SCANNERS_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmExtScanEnableScannersCfm;

typedef CsrBtCmExtScanEnableScannersCfm CmExtScanEnableScannersCfm;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_REQ */
    CsrSchedQid                appHandle;
} CsrBtCmExtScanGetCtrlScanInfoReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CM_DM_EXT_SCAN_TIMEOUT_IND */
} CmDmExtScanTimeoutInd;

typedef CsrBtCmExtScanGetCtrlScanInfoReq CmExtScanGetCtrlScanInfoReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_GET_CTRL_SCAN_INFO_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   num_of_enabled_scanners;
    CsrUint16                  duration;
    CsrUint16                  scanning_phys;
    CmScanningPhy              phys[CM_EXT_SCAN_MAX_SCANNING_PHYS];
} CsrBtCmExtScanGetCtrlScanInfoCfm;

typedef CsrBtCmExtScanGetCtrlScanInfoCfm CmExtScanGetCtrlScanInfoCfm;

/* Reason for Controller Scan Info Indication */
#define CM_EXT_SCAN_REASON_GLOBALS_CHANGED   0
#define CM_EXT_SCAN_REASON_SCANNERS_ENABLED  1
#define CM_EXT_SCAN_REASON_SCANNERS_DISABLED 2

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_CTRL_SCAN_INFO_IND */
    CsrUint8                   reason;
    CsrUint8                   controller_updated;
    CsrUint8                   num_of_enabled_scanners;
    CsrUint8                   legacy_scanner_enabled;
    CsrUint16                  duration;
    CsrUint16                  scanning_phys;
    CmScanningPhy              phys[CM_EXT_SCAN_MAX_SCANNING_PHYS];
} CsrBtCmExtScanCtrlScanInfoInd;

typedef CsrBtCmExtScanCtrlScanInfoInd CmExtScanCtrlScanInfoInd;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND */
    CsrUint16                  eventType;    
    CsrUint16                  primaryPhy;
    CsrUint16                  secondaryPhy;
    CsrUint8                   advSid;
    TYPED_BD_ADDR_T            currentAddrt;
    TYPED_BD_ADDR_T            permanentAddrt;
    TYPED_BD_ADDR_T            directAddrt;
    CsrInt8                    txPower;
    CsrInt8                    rssi;
    CsrUint16                  periodicAdvInterval;
    CsrUint8                   advDataInfo;
    CsrUint8                   adFlags;
    CsrUint16                  dataLength;
    CsrUint8                   *data;
} CsrBtCmExtScanFilteredAdvReportInd;

typedef CsrBtCmExtScanFilteredAdvReportInd CmExtScanFilteredAdvReportInd;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CM_EXT_SCAN_FILTERED_ADV_REPORT_DONE_IND */
    CsrUint16                  dataLength;
    void                      *data;
} CmExtScanFilteredAdvReportDoneInd;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CSR_BT_CM_EXT_SCAN_DURATION_EXPIRED_IND */
    CsrUint8                   scan_handle;    
    CsrUint8                   scan_handle_unregistered;
} CsrBtCmExtScanDurationExpiredInd;

typedef CsrBtCmExtScanDurationExpiredInd CmExtScanDurationExpiredInd;

#ifdef INSTALL_CM_EXT_SET_CONN_PARAM
#define CM_EXT_MAX_INITIATING_PHYS DM_EXT_CONNECTION_NUM_INITIATING_PHYS

typedef DM_ULP_ES_INITIATING_PHY_T CmInitiatingPhy;

typedef struct
{
    CsrBtCmPrim                type;                    /* Identity: CM_DM_EXT_SET_CONN_PARAMS_REQ */
    CsrSchedQid                appHandle;               /* Destination phandle */
    CsrUint8                   advHandle;               /* Reserved for future use. Shall be set to 0xFF for now */
    CsrUint8                   subevent;                /* Reserved for future use. Shall be set to 0xFF for now */
    CsrUint16                  connAttemptTimeout;      /* Connection timeout */
    CsrUint8                   ownAddressType;          /* Own Address Type used in LE connection */
    CsrUint8                   phyCount;                /* Number of entries in initPhys[]. Maximum CM_EXT_MAX_INITIATING_PHYS 
                                                           entries (LE 1M, 2M, LE Coded). Minimum = 1 */
    CmInitiatingPhy            *initPhys[CM_EXT_MAX_INITIATING_PHYS]; /* Initiating Phy parameters */
} CmDmExtSetConnParamsReq;

typedef struct
{
    CsrBtCmPrim                type;          /* Identity: CM_DM_EXT_SET_CONN_PARAMS_CFM */
    CsrSchedQid                appHandle;
    CsrBtResultCode            resultCode;
} CmDmExtSetConnParamsCfm;
#endif /* End of INSTALL_CM_EXT_SET_CONN_PARAM */

#ifdef INSTALL_CM_DM_CONFIGURE_DATA_PATH
#define CM_DM_CONFIGURE_DATA_PATH_MAX_INDEX     HCI_CONFIGURE_DATA_PATH_PTRS

typedef struct
{
    CsrBtCmPrim                type;                    /* Identity: CM_DM_HCI_CONFIGURE_DATA_PATH_REQ */
    CsrSchedQid                appHandle;               /* Destination phandle */
    CsrUint8                   dataPathDirection;       /* Data Path direction */
    CsrUint8                   dataPathId;              /* Data Path ID */
    CsrUint8                   vendorSpecificConfigLen; /* Total length of vendorSpecificConfig[] in octets */
    CsrUint8                   *vendorSpecificConfig[CM_DM_CONFIGURE_DATA_PATH_MAX_INDEX]; /* Vendor Specific Configuration data.
                                                           Each index can hold maximum HCI_CONFIGURE_DATA_PATH_PER_PTR octets */
} CmDmConfigureDataPathReq;

typedef struct
{
    CsrBtCmPrim                type;                    /* Identity: CM_DM_HCI_CONFIGURE_DATA_PATH_CFM */
    CsrBtResultCode            resultCode;
} CmDmConfigureDataPathCfm;
#endif /* End of INSTALL_CM_DM_CONFIGURE_DATA_PATH */

#ifdef INSTALL_CM_DM_LE_READ_CHANNEL_MAP
#define CM_DM_LE_CHANNEL_MAP_LEN             5
typedef struct
{
    CsrBtCmPrim                type;         /* Identity: CM_DM_LE_READ_CHANNEL_MAP_REQ */
    CsrSchedQid                appHandle;    /* Destination phandle */
    TYPED_BD_ADDR_T            peerAddr;     /* Remote address */
} CmDmLeReadChannelMapReq;

typedef struct
{
    CsrBtCmPrim                type;         /* Identity: CM_DM_LE_READ_CHANNEL_MAP_CFM */
    CsrBtResultCode            resultCode;
    CsrBtSupplier              resultSupplier;
    TYPED_BD_ADDR_T            peerAddr;     /* Remote address */
    CsrUint8                   channelMap[CM_DM_LE_CHANNEL_MAP_LEN]; /* Channel map bit field.
                                             37 bits mapped to 0 to 36 Link Layer channel index.
                                             Bits 37 to 39 most significant bits are reserved for future use */
} CmDmLeReadChannelMapCfm;
#endif /* End of INSTALL_CM_DM_LE_READ_CHANNEL_MAP */

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   advHandle;
    CsrUint32                  flags;
    CsrUint16                  periodicAdvIntervalMin;
    CsrUint16                  periodicAdvIntervalMax;
    CsrUint16                  periodicAdvProperties;
} CsrBtCmPeriodicAdvSetParamsReq;

typedef CsrBtCmPeriodicAdvSetParamsReq CmPeriodicAdvSetParamsReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvSetParamsCfm;

typedef CsrBtCmPeriodicAdvSetParamsCfm CmPeriodicAdvSetParamsCfm;

#define CM_PERIODIC_ADV_DATA_LENGTH_MAX          252
#define CM_PERIODIC_ADV_DATA_BLOCK_SIZE          32
#define CM_PERIODIC_ADV_DATA_BYTES_PTRS_MAX      8

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_DATA_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   advHandle;
    CsrUint8                   operation;
    CsrUint8                   dataLength;
    CsrUint8                   *data[CM_PERIODIC_ADV_DATA_BYTES_PTRS_MAX];
} CsrBtCmPeriodicAdvSetDataReq;

typedef CsrBtCmPeriodicAdvSetDataReq CmPeriodicAdvSetDataReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvSetDataCfm;

typedef CsrBtCmPeriodicAdvSetDataCfm CmPeriodicAdvSetDataCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvReadMaxAdvDataLenReq;

typedef CsrBtCmPeriodicAdvReadMaxAdvDataLenReq CmPeriodicAdvReadMaxAdvDataLenReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_READ_MAX_ADV_DATA_LEN_CFM */
    CsrBtResultCode            resultCode;
    CsrUint16                  maxAdvData;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvReadMaxAdvDataLenCfm;

typedef CsrBtCmPeriodicAdvReadMaxAdvDataLenCfm CmPeriodicAdvReadMaxAdvDataLenCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_START_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvStartReq;

typedef CsrBtCmPeriodicAdvStartReq CmPeriodicAdvStartReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_START_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvStartCfm;

typedef CsrBtCmPeriodicAdvStartCfm CmPeriodicAdvStartCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_STOP_REQ */
    CsrSchedQid                appHandle;
    CsrUint8                   advHandle;
    CsrUint8                   stopAdvertising;
} CsrBtCmPeriodicAdvStopReq;

typedef CsrBtCmPeriodicAdvStopReq CmPeriodicAdvStopReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_STOP_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvStopCfm;

typedef CsrBtCmPeriodicAdvStopCfm CmPeriodicAdvStopCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_REQ */
    phandle_t                  phandle;            /* Destination phandle */
    TYPED_BD_ADDR_T            addrt;              /* Address of peer device */
    uint16_t                   serviceData;        /* Service data for peer's Host */
    uint8_t                    advHandle;          /* Identifies the adv. set to local Controller */
} CsrBtCmPeriodicAdvSetTransferReq;

typedef CsrBtCmPeriodicAdvSetTransferReq CmPeriodicAdvSetTransferReq;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_SET_TRANSFER_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvSetTransferCfm;

typedef CsrBtCmPeriodicAdvSetTransferCfm CmPeriodicAdvSetTransferCfm;

typedef struct
{
    CsrBtCmPrim                type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_ENABLE_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   advHandle;
} CsrBtCmPeriodicAdvEnableCfm;

typedef CsrBtCmPeriodicAdvEnableCfm CmPeriodicAdvEnableCfm;
#define CM_PERIODIC_SCAN_AD_STRUCT_INFO_LENGTH  255

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_REQ */
    phandle_t                  phandle;         /* Where to send confirm and indications for this scanner. */
    CsrUint32                  flags;
    CsrUint16                  scanForXSeconds;
    CsrUint16                  adStructureFilter;
    CsrUint16                  adStructureFilterSubField1;
    CsrUint32                  adStructureFilterSubField2;
    CsrUint8                   adStructureInfoLen;
    CsrUint8                  *adStructureInfo;
} CsrBtCmPeriodicScanStartFindTrainsReq;

typedef CsrBtCmPeriodicScanStartFindTrainsReq CmPeriodicScanStartFindTrainsReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_START_FIND_TRAINS_CFM */
    CsrBtResultCode            resultCode;
    CsrUint8                   scanHandle;
} CsrBtCmPeriodicScanStartFindTrainsCfm;

typedef CsrBtCmPeriodicScanStartFindTrainsCfm CmPeriodicScanStartFindTrainsCfm;


typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_REQ */
    phandle_t                  phandle;         /* Where to send confirm and indications for this scanner. */
    CsrUint8                   scanHandle;
} CsrBtCmPeriodicScanStopFindTrainsReq;

typedef CsrBtCmPeriodicScanStopFindTrainsReq CmPeriodicScanStopFindTrainsReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_STOP_FIND_TRAINS_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmPeriodicScanStopFindTrainsCfm;

typedef CsrBtCmPeriodicScanStopFindTrainsCfm CmPeriodicScanStopFindTrainsCfm;

#define CM_MAX_PERIODIC_TRAIN_LIST_SIZE 3

typedef struct
{
    CsrUint8                   advSid;
    TYPED_BD_ADDR_T            addrt;        /* Bluetooth address and type */
} CmPeriodicScanTrains;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ */
    phandle_t                  phandle;         /* Where to send confirm and indications for this scanner. */
    CsrUint8                   reportPeriodic;
    CsrUint16                  skip;
    CsrUint16                  syncTimeout;
    CsrUint8                   syncCteType;
    CsrUint16                  attemptSyncForXSeconds;
    CsrUint8                   numberOfPeriodicTrains;
    CmPeriodicScanTrains       periodicTrains[CM_MAX_PERIODIC_TRAIN_LIST_SIZE];
} CsrBtCmPeriodicScanSyncToTrainReq;

typedef CsrBtCmPeriodicScanSyncToTrainReq CmPeriodicScanSyncToTrainReq;

#define CM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_PENDING 0xFFFF

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CFM */
    CsrBtResultCode            resultCode;      /* Success or failure or pending */
    CsrUint16                  syncHandle;      /* The handle for the synced train */
    CsrUint8                   advSid;
    TYPED_BD_ADDR_T            addrt;
    CsrUint8                   advPhy;
    CsrUint16                  periodicAdvInterval;
    CsrUint8                   advClockAccuracy;
} CsrBtCmPeriodicScanSyncToTrainCfm;

typedef CsrBtCmPeriodicScanSyncToTrainCfm CmPeriodicScanSyncToTrainCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_REQ */
    phandle_t                  phandle;         /* Where to send confirm and indications for this scanner. */
} CsrBtCmPeriodicScanSyncToTrainCancelReq;

typedef CsrBtCmPeriodicScanSyncToTrainCancelReq CmPeriodicScanSyncToTrainCancelReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TO_TRAIN_CANCEL_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmPeriodicScanSyncToTrainCancelCfm;

typedef CsrBtCmPeriodicScanSyncToTrainCancelCfm CmPeriodicScanSyncToTrainCancelCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_REQ */
    phandle_t                  phandle;         /* Where to send confirm */
    CsrUint16                  syncHandle;      /* The train to enable/disable adv reporting */
    CsrUint8                   enable;          /* TRUE or FALSE */
} CsrBtCmPeriodicScanSyncAdvReportEnableReq;

typedef CsrBtCmPeriodicScanSyncAdvReportEnableReq CmPeriodicScanSyncAdvReportEnableReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_ENABLE_CFM */
    CsrBtResultCode            resultCode;
} CsrBtCmPeriodicScanSyncAdvReportEnableCfm;

typedef CsrBtCmPeriodicScanSyncAdvReportEnableCfm CmPeriodicScanSyncAdvReportEnableCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_REQ */
    phandle_t                  phandle;         /* Where to send confirm */
    CsrUint16                  syncHandle;
} CsrBtCmPeriodicScanSyncTerminateReq;

typedef CsrBtCmPeriodicScanSyncTerminateReq CmPeriodicScanSyncTerminateReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TERMINATE_CFM */
    CsrBtResultCode            resultCode;
    CsrUint16                  syncHandle;
} CsrBtCmPeriodicScanSyncTerminateCfm;

typedef CsrBtCmPeriodicScanSyncTerminateCfm CmPeriodicScanSyncTerminateCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND */
    CsrUint16                  syncHandle;
    CsrUint8                   txPower;
    CsrUint8                   rssi;
    CsrUint8                   cteType;
    CsrUint16                  dataLength;
    CsrUint8                   *data;
} CsrBtCmPeriodicScanSyncAdvReportInd;

typedef CsrBtCmPeriodicScanSyncAdvReportInd CmPeriodicScanSyncAdvReportInd;

typedef struct
{
    CsrBtCmPrim                type;            /* CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND */
    CsrUint16                  dataLength;
    CsrUint8                   *data;
} CmPeriodicScanSyncAdvReportDoneInd;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_IND */
    CsrUint16                  syncHandle;
} CsrBtCmPeriodicScanSyncLostInd;

typedef CsrBtCmPeriodicScanSyncLostInd CmPeriodicScanSyncLostInd;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_LOST_RSP */
    CsrUint16                  syncHandle;
} CsrBtCmPeriodicScanSyncLostRsp;

typedef CsrBtCmPeriodicScanSyncLostRsp CmPeriodicScanSyncLostRsp;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_REQ */
    phandle_t                  phandle;         /* Where to send confirm */
    TYPED_BD_ADDR_T            addrt;           /* Address of peer device */
    CsrUint16                  serviceData;     /* Service data for peer's Host */
    CsrUint16                  syncHandle;      /* Identifies sync train to local Controller */
} CsrBtCmPeriodicScanSyncTransferReq;

typedef CsrBtCmPeriodicScanSyncTransferReq CmPeriodicScanSyncTransferReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_CFM */
    CsrBtResultCode            resultCode;
    CsrUint16                  syncHandle;
} CsrBtCmPeriodicScanSyncTransferCfm;

typedef CsrBtCmPeriodicScanSyncTransferCfm CmPeriodicScanSyncTransferCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_IND */
    CsrBtResultCode            resultCode;
    CsrUint8                   advSid;
    CsrUint16                  syncHandle;
    CsrUint16                  serviceData;
    TYPED_BD_ADDR_T            addrt;
} CsrBtCmPeriodicScanSyncTransferInd;

typedef CsrBtCmPeriodicScanSyncTransferInd CmPeriodicScanSyncTransferInd;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_REQ */
    phandle_t                  phandle;         /* Where to send confirm */
    TYPED_BD_ADDR_T            addrt;           /* Address of peer device */
    CsrUint16                  skip;            /* Number of ads the Controller may skip */
    CsrUint16                  syncTimeout;     /* Max time between successful ads received */
    CsrUint8                   mode;            /* What to do when SyncInfo is received. */
    CsrUint8                   cteType;         /* Specify types of Constant Tone Extension. */
} CsrBtCmPeriodicScanSyncTransferParamsReq;

typedef CsrBtCmPeriodicScanSyncTransferParamsReq CmPeriodicScanSyncTransferParamsReq;

typedef struct
{
    CsrBtCmPrim                type;            /* CSR_BT_CM_PERIODIC_SCAN_SYNC_TRANSFER_PARAMS_CFM */
    CsrBtResultCode            resultCode;
    TYPED_BD_ADDR_T            addrt;              /* Address of peer device */
} CsrBtCmPeriodicScanSyncTransferParamsCfm;

typedef CsrBtCmPeriodicScanSyncTransferParamsCfm CmPeriodicScanSyncTransferParamsCfm;

typedef struct
{
    CsrBtCmPrim                type;            /* Identity: CSR_BT_CM_BLE_BIGINFO_ADV_REPORT_IND */
    CmBigParam                 bigParams;       /* Big params from peer s*/
    CsrUint24                  sduInterval;     /* Interval of Periodic SDUs*/
    CsrUint16                  syncHandle;      /* Sync handle of the PA */
    CsrUint16                  maxSdu;          /* Maximum size of an SDU */
    CsrUint8                   numBis;          /* Number of BISes in BIG */
    CsrUint8                   framing;         /* Framed or unframed data */
    CsrUint8                   encryption;      /* Data encryption status */
} CsrBtCmBleBiginfoAdvReportInd;

typedef CsrBtCmBleBiginfoAdvReportInd CmBleBigInfoAdvReportInd;

typedef struct
{
    CsrBtCmPrim                   type;          /* Identity: CSR_BT_CM_UPDATE_INTERNAL_PEER_ADDR_REQ */
    CsrBtDeviceAddr               newPeerAddr;   /* New Peer Earbud Address */
    CsrBtDeviceAddr               oldPeerAddr;   /* Old Peer Earbud Address */
} CsrBtCmUpdateInternalPeerAddrReq;

typedef CsrBtCmUpdateInternalPeerAddrReq CsrBtCmUpdateInternalPeerAddressReq;

typedef struct
{
    CsrBtCmPrim             type;       /* CSR_BT_CM_RFC_DISCONNECT_RSP */
    CsrBtConnId             btConnId;   /* Global Bluetooth connection ID */
} CsrBtCmRfcDisconnectRsp;

typedef struct
{
    CsrBtCmPrim             type;       /* CSR_BT_CM_L2CA_DISCONNECT_RSP */
    l2ca_identifier_t       identifier; /* l2cap identifier for which the disconnection is received */
    CsrBtConnId             btConnId;   /* Global Bluetooth connection ID */
} CsrBtCmL2caDisconnectRsp;

/* Defines for Crypto Functionalities Support */
#define CSR_BT_CM_CRYPTO_LOCAL_PVT_KEY_LEN         16
#define CSR_BT_CM_CRYPTO_REMOTE_PUB_KEY_LEN        32
#define CSR_BT_CM_CRYPTO_AES_DATA_LEN              8
#define CSR_BT_CM_CRYPTO_AES_KEY_LEN               8
#define CSR_BT_CM_CRYPTO_AES_NONCE_LEN             8
#define CSR_BT_CM_CRYPTO_PUBLIC_KEY_LEN            32
#define CSR_BT_CM_CRYPTO_PRIVATE_KEY_LEN           16
#define CSR_BT_CM_CRYPTO_SECRET_KEY_LEN            16
#define CSR_BT_CM_CRYPTO_SHA_HASH_LEN              16

/*  This specific define is the most vulnerable to change from the firmware; if
 *  the firmware's maximum block size changes, this will need to be changed
 *  accordingly. */
#define CSR_BT_CM_CRYPTO_SHA_DATA_LEN              16
#define CSR_BT_CM_CRYPTO_AES_CTR_MAX_DATA_LEN      32

typedef CsrUint8 CsrBtCmCryptoEccType;
#define CSR_BT_CM_CRYPTO_ECC_P192                    ((CsrBtCmCryptoEccType) 0x01)
#define CSR_BT_CM_CRYPTO_ECC_P256                    ((CsrBtCmCryptoEccType) 0x02)
#define CSR_BT_CM_CRYPTO_ECC_UNKNOWN_TYPE            ((CsrBtCmCryptoEccType) 0XFF)

typedef CsrUint8 CsrBtCmCryptoHashOperation;
#define CSR_BT_CM_CRYPTO_SINGLE_BLOCK                ((CsrBtCmCryptoHashOperation) 0x00)
#define CSR_BT_CM_CRYPTO_DATA_START                  ((CsrBtCmCryptoHashOperation) 0x01)
#define CSR_BT_CM_CRYPTO_DATA_CONTINUE               ((CsrBtCmCryptoHashOperation) 0x02)
#define CSR_BT_CM_CRYPTO_DATA_END                    ((CsrBtCmCryptoHashOperation) 0x03)
#define CSR_BT_CM_CRYPTO_UNKNOWN_OPERATION           ((CsrBtCmCryptoHashOperation) 0xFF)

typedef CsrUint16 CsrBtCmCryptoAesCtrFlags;
#define CSR_BT_CM_AES_CTR_NONE                       ((CsrBtCmCryptoAesCtrFlags) 0x0000)
/* Concatenate the counter value to the upper bits of the initial vector. */
#define CSR_BT_CM_AES_CTR_BIG_ENDIAN                 ((CsrBtCmCryptoAesCtrFlags) 0x0001)

typedef struct
{
    CsrBtCmPrim                type;                                             /* Identity: CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_REQ */
    CsrSchedQid                appHandle;                                        /* Destination phandle */
    CsrBtCmCryptoEccType       keyType;                                          /* CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256 */
} CsrBtCmCryptoGeneratePublicPrivateKeyReq;

typedef struct
{
    CsrBtCmPrim                type;                                             /* Identity: CSR_BT_CM_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM */
    CsrBtCmCryptoEccType       keyType;                                          /* CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256 */
    CsrUint16                  publicKey[CSR_BT_CM_CRYPTO_PUBLIC_KEY_LEN];       /* Public key, zero if status is failure */
    CsrUint16                  privateKey[CSR_BT_CM_CRYPTO_PRIVATE_KEY_LEN];     /* Private key, zero if status is failure */
    CsrBtResultCode            resultCode;                                       /* Generation success or failed */
    CsrBtSupplier              resultSupplier;                                   /* Supplier of result code */
} CsrBtCmCryptoGeneratePublicPrivateKeyCfm;

typedef struct
{
    CsrBtCmPrim                type;                                             /* Identity: CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_REQ */
    CsrSchedQid                appHandle;                                        /* Destination phandle */
    CsrBtCmCryptoEccType       keyType;                                          /* CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256 */
    CsrUint16                  privateKey[CSR_BT_CM_CRYPTO_LOCAL_PVT_KEY_LEN];   /* Private key */
    CsrUint16                  publicKey[CSR_BT_CM_CRYPTO_REMOTE_PUB_KEY_LEN];   /* Public key */
} CsrBtCmCryptoGenerateSharedSecretKeyReq;

typedef struct
{
    CsrBtCmPrim                type;                                             /* Identity: CSR_BT_CM_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM */
    CsrBtCmCryptoEccType       keyType;                                          /* CSR_BT_CM_CRYPTO_ECC_P192/CSR_BT_CM_CRYPTO_ECC_P256 */
    CsrUint16                  sharedSecretKey[CSR_BT_CM_CRYPTO_SECRET_KEY_LEN]; /* Shared secret key(dhkey), zero if status is not success */
    CsrBtResultCode            resultCode;                                       /* Generation success or failed */
    CsrBtSupplier              resultSupplier;                                   /* Supplier of result code */
} CsrBtCmCryptoGenerateSharedSecretKeyCfm;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_ENCRYPT_REQ */
    CsrSchedQid                 appHandle;                                       /* Destination phandle */
    CsrUint16                   dataArray[CSR_BT_CM_CRYPTO_AES_DATA_LEN];        /* Data to be encrypted */
    CsrUint16                   keyArray[CSR_BT_CM_CRYPTO_AES_KEY_LEN];          /* Encryption Key */
    CsrUint8                    flags;                                           /* Reserved */
} CsrBtCmCryptoEncryptReq;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_ENCRYPT_CFM */
    CsrUint8                    flags;                                           /* Reserved */
    CsrUint16                   encryptedData[CSR_BT_CM_CRYPTO_AES_DATA_LEN];    /* Encrypted Data */
    CsrBtResultCode             resultCode;                                      /* Generation success or failed */
    CsrBtSupplier               resultSupplier;                                  /* Supplier of result code */
} CsrBtCmCryptoEncryptCfm;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_HASH_REQ */
    CsrSchedQid                 appHandle;                                       /* Destination phandle */
    CsrUint16                  *dataArray;                                       /* Data to be hashed */
    CsrUint16                   arraySize;                                       /* Size of the data array */
    CsrUint16                   currentIndex;                                    /* Index pointing to data to be hashed*/
    CsrUint8                    flags;                                           /* Reserved */
} CsrBtCmCryptoHashReq;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_HASH_CFM */
    CsrBtCmCryptoHashOperation  operation;                                       /* Operation type from request */
    CsrUint8                    flags;                                           /* Reserved */
    CsrUint16                   hash[CSR_BT_CM_CRYPTO_SHA_HASH_LEN];             /* Hash of the supplied data */
    CsrBtResultCode             resultCode;                                      /* Generation success or failed */
    CsrBtSupplier               resultSupplier;                                  /* Supplier of result code */
} CsrBtCmCryptoHashCfm;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_DECRYPT_REQ */
    CsrSchedQid                 appHandle;                                       /* Destination phandle */
    CsrUint16                   dataArray[CSR_BT_CM_CRYPTO_AES_DATA_LEN];        /* Data to be encrypted */
    CsrUint16                   keyArray[CSR_BT_CM_CRYPTO_AES_KEY_LEN];          /* Encryption Key */
    CsrUint8                    flags;                                           /* Reserved */
} CsrBtCmCryptoDecryptReq;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_DECRYPT_CFM */
    CsrUint8                    flags;                                           /* Reserved */
    CsrUint16                   decryptedData[CSR_BT_CM_CRYPTO_AES_DATA_LEN];    /* Decrypted Data */
    CsrBtResultCode             resultCode;                                      /* Generation success or failed */
    CsrBtSupplier               resultSupplier;                                  /* Supplier of result code */
} CsrBtCmCryptoDecryptCfm;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_AES_CTR_REQ */
    CsrSchedQid                 appHandle;                                       /* Destination phandle */
    CsrUint32                   counter;                                         /* Initial counter value */
    CsrBtCmCryptoAesCtrFlags    flags;                                           /* Set Crypto flag */
    CsrUint16                   secretKey[CSR_BT_CM_CRYPTO_AES_KEY_LEN];         /* Secret Key */
    CsrUint16                   nonce[CSR_BT_CM_CRYPTO_AES_NONCE_LEN];           /* Nonce */
    CsrUint16                   dataLen;                                         /* Length of data */
    CsrUint16                  *data;                                            /* Data */
} CsrBtCmCryptoAesCtrReq;

typedef struct
{
    CsrBtCmPrim                 type;                                            /* Identity: CSR_BT_CM_CRYPTO_AES_CTR_CFM */
    CsrBtCmCryptoAesCtrFlags    flags;                                           /* Set Crypto flag */
    CsrUint16                   dataLen;                                         /* Length of output data */
    CsrUint16                  *data;                                            /* Output data */
    CsrBtResultCode             resultCode;                                      /* Generation success or failed */
    CsrBtSupplier               resultSupplier;                                  /* Supplier of result code */
} CsrBtCmCryptoAesCtrCfm;

typedef struct
{
    CsrBtCmPrim                 type;                   /* Identity: CSR_BT_CM_RFC_CONNECT_ACCEPT_IND */
    CsrBtConnId                 btConnId;               /* Global Bluetooth connection ID */
    CsrBtDeviceAddr             deviceAddr;             /* Bluetooth address of the remote device which wants to connect. */
    uint8                       localServerChannel;     /* Local server channel for which the connection is requested */
} CsrBtCmRfcConnectAcceptInd;

typedef struct
{
    CsrBtCmPrim              type;                  /* Identity: CSR_BT_CM_RFC_CONNECT_ACCEPT_RSP */
    CsrSchedQid              appHandle;             /* application phandle */
    CsrBtConnId              btConnId;              /* Global Bluetooth connection ID */
    CsrBtDeviceAddr          deviceAddr;            /* Bluetooth Device Address */
    CsrBool                  accept;                /* If RFC the connection is accepted or rejected */
    CsrUint8                 serverChannel;         /* Local Server channel for which the connection is requested */
    CsrUint8                 modemStatus;           /* RFC modem Status */
    CsrUint8                 breakSignal;           /* Break Signal */
    CsrUint8                 mscTimeout;            /* RFC Modem Status Timeout in milliseconds */
} CsrBtCmRfcConnectAcceptRsp;

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND */
    CsrBtConnId                 btConnId;           /* Global Bluetooth connection ID */
    l2ca_identifier_t           identifier;         /* l2cap identifier for which the connection is received */
    psm_t                       localPsm;           /* PSM for which the connect request is received. */
    CsrBtDeviceAddr             deviceAddr;         /* Remote bluetooth device address. */
} CsrBtCmL2caConnectAcceptInd;

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CM_L2CA_TP_CONNECT_ACCEPT_IND */
    CsrBtConnId                 btConnId;           /* Global Bluetooth connection ID */
    CsrUint16                   context;            /* Context */
    l2ca_identifier_t           identifier;         /* l2cap identifier for which the connection is received */
    psm_t                       localPsm;           /* PSM for which the connect request is received. */
    CsrBtTpdAddrT               tpdAddrT;           /* Remote bluetooth device address with type and transport */
    l2ca_controller_t           localControl;       /* Local controller ID */
    l2ca_conflags_t             flags;              /* Special connection flags */
    l2ca_mtu_t                  mtu;                /* Reserved for future use. */
    l2ca_mtu_t                  localMtu;           /* Reserved for future use. */
    CsrUint16                   credits;            /* Reserved for future use. */
    CsrUint8                    dataPriority;       /* Reserved for future use. */
} CmL2caTpConnectAcceptInd;

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CSR_BT_CM_L2CA_CONNECT_ACCEPT_RSP */
    CsrSchedQid                 phandle;            /* application phandle. */
    CsrBool                     accept;             /* Accept or reject the connection. */
    CsrBtConnId                 btConnId;           /* Global Bluetooth connection ID */
    psm_t                       localPsm;           /* Local PSM. */
    l2ca_identifier_t           identifier;         /* l2cap identifier for which the connection is received */
    CsrUint16                   conftabCount;       /* Configuration tab count */
    CsrUint16                  *conftab;            /* Configuration pointer */
    CsrBtDeviceAddr             deviceAddr;         /* Bluetooth device address */
    CsrUint8                    minEncKeySize;      /* Minimum Encryption Key Size */
} CsrBtCmL2caConnectAcceptRsp;

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CM_L2CA_TP_CONNECT_ACCEPT_RSP */
    CsrSchedQid                 phandle;            /* application phandle. */
    CsrBool                     accept;             /* Accept or reject the connection. */
    CsrBtConnId                 btConnId;           /* Global Bluetooth connection ID */
    psm_t                       localPsm;           /* Local PSM. */
    l2ca_identifier_t           identifier;         /* l2cap identifier for which the connection is received */
    CsrUint16                   conftabCount;       /* Configuration tab count */
    CsrUint16                  *conftab;            /* Configuration pointer */
    CsrBtTpdAddrT               tpdAddrT;           /* Bluetooth device address with type and transport */
    CsrUint8                    minEncKeySize;      /* Minimum Encryption Key Size */
} CmL2caTpConnectAcceptRsp;

typedef CsrUint8 CmScOverrideAction;
#define CM_SC_OVERRIDE_ACTION_DISABLE   ((CmScOverrideAction)0x00)        /* Enable SC support if disabled */
#define CM_SC_OVERRIDE_ACTION_ENABLE    ((CmScOverrideAction)0x01)        /* Disable SC support if enabled */
#define CM_SC_OVERRIDE_ACTION_DELETE    ((CmScOverrideAction)0xFF)       /* Delete the BD address from the list */

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ */
    CsrSchedQid                 appHandle;          /* Application handle */
    CsrBtDeviceAddr             deviceAddr;         /* Global bluetooth device address */
    CmScOverrideAction          overrideAction;     /* Override action for enabling/disabling/deleting SC support */
} CmWriteScHostSupportOverrideReq;

typedef struct
{
    CsrBtCmPrim                 type;               /* Identity: CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ */
    CsrSchedQid                 appHandle;          /* Application handle */
} CmReadScHostSupportOverrideMaxBdAddrReq;

typedef struct
{
    CsrBtCmPrim             type;                   /* Type: CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM */
    hci_return_t            status;                 /* status of set secure connections override */
    CsrBtDeviceAddr         deviceAddr;             /* Global bluetooth address of the device. */
    CsrUint8                hostSupportOverride;    /* Override value */
} CmWriteScHostSupportOverrideCfm;

typedef struct
{
    CsrBtCmPrim             type;                   /* Type: CM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM */
    hci_return_t            status;                 /* Status of set secure connections override */
    CsrUint8                maxOverrideBdAddr;      /* Max number of BD Addresses that can be supported by local controller */
} CmReadScHostSupportOverrideMaxBdAddrCfm;


typedef struct
{
    CsrBtCmPrim             type;               /* Identity: CSR_BT_CM_PERIODIC_ADV_ENABLE_REQ */
    CsrSchedQid             phandle;            /* Destination phandle */
    uint8_t                 adv_handle;

    /* flags bit field value.
     * Bit 0: If set enable extended advertising while enabling periodic advertising. 
     *        (This bit does nothing if extended advertising is already enabled).
     * Bit 1: If set disable extended advertising while disabling periodic advertising.
     *        (This bit does nothing if extended advertising is already disabled) */
    uint16_t                flags;

    /* enable bit field value.
     * Bit 0: If set enable periodic advertising, else disable.
     * Bit 1: If set add ADI field to AUX_SYNC_IND PDU when enabling periodic advertising. */
    uint8_t                 enable;
} CsrBtCmPeriodicAdvEnableReq;

typedef CsrBtCmPeriodicAdvEnableReq CmPeriodicAdvEnableReq;

typedef struct
{
    CsrBtCmPrim              type;                                               /* Identity: CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_REQ */
    CsrSchedQid              appHandle;                                          /* Application handle */
    CsrUint16                subrate_min;                                        /* Minimum subrate factor allowed in requests by a Peripheral */
    CsrUint16                subrate_max;                                        /* Maximum subrate factor allowed in requests by a Peripheral */
    CsrUint16                max_latency;                                        /* Maximum Peripheral latency allowed in requests by a Peripheral */
    CsrUint16                continuation_num;                                   /* Minimum number of underlying connection events to remain active */
    CsrUint16                supervision_timeout;                                /* Maximum supervision timeout allowed in requests by a Peripheral */
} CsrBtCmLeSetDefaultSubrateReq;

typedef struct
{
    CsrBtCmPrim              type;                                               /* Identity: CSR_BT_CM_LE_SET_DEFAULT_SUBRATE_CFM */
    CsrBtResultCode          resultCode;                                         /* Generation success or failed */
} CsrBtCmLeSetDefaultSubrateCfm;

typedef struct
{
    CsrBtCmPrim              type;                                               /* Identity: CSR_BT_CM_LE_SUBRATE_CHANGE_REQ */
    CsrSchedQid              appHandle;                                          /* Application handle */
    CsrBtTypedAddr           address;                                            /* Peer Address */
    CsrUint16                subrate_min;                                        /* Minimum subrate factor to be applied to the underlying connection interval */
    CsrUint16                subrate_max;                                        /* Maximum subrate factor to be applied to the underlying connection interval */
    CsrUint16                max_latency;                                        /* Maximum Peripheral latency for the connection in units of subrated connection intervals */
    CsrUint16                continuation_num;                                   /* Minimum number of underlying connection events to remain active */
    CsrUint16                supervision_timeout;                                /* Supervision timeout for this connection */
} CsrBtCmLeSubrateChangeReq;

typedef struct
{
    CsrBtCmPrim              type;                                               /* Identity: CSR_BT_CM_LE_SUBRATE_CHANGE_CFM */
    CsrBtResultCode          resultCode;                                         /* Generation success or failed */
    CsrBtTypedAddr           address;                                            /* Peer Address */
} CsrBtCmLeSubrateChangeCfm;

typedef struct
{
    CsrBtCmPrim              type;                                               /* Identity: CSR_BT_CM_LE_SUBRATE_CHANGE_IND */
    CsrBtResultCode          status;                                             /* status of the configuration */
    TYPED_BD_ADDR_T          addrt;                                              /* peer device address */
    CsrUint16                subrate_factor;                                     /* New subrate factor applied */
    CsrUint16                peripheral_latency;                                 /* New peripheral latency in number of subrated connection events */
    CsrUint16                continuation_num;                                   /* Number of underlying connection events to remain active */
    CsrUint16                supervision_timeout;                                /* New supervision timeout for the connection */
} CsrBtCmLeSubrateChangeInd;

typedef struct
{
    CsrBtCmPrim             type;           /* type: CM_SM_REFRESH_ENCRYPTION_KEY_REQ */
    CsrBtDeviceAddr         deviceAddr;     /* Bluetooth Device Address */
} CmSmRefreshEncryptionKeyReq;

typedef struct
{
    CsrUint8            minTxRate;      /* Minimum preferred tx rate */
    CsrUint8            maxTxRate;      /* Maximum preferred tx rate */
    CsrUint8            minRxRate;      /* Minimum preferred rx rate */
    CsrUint8            maxRxRate;      /* Maximum preferred rx rate */
    CsrUint8            flags;          /* Flags for additional preference,
                                           reserved for now, shall be set to zero. */
} CmDmLePhyInfo;

#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
typedef struct
{
    CsrBtCmPrim         type;           /* Type: CM_DM_LE_READ_PHY_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling process */
    CsrBtTpdAddrT       tpAddr;         /* Bluetooth device address with transport type and address type */
} CmDmLeReadPhyReq;
#endif

typedef struct
{
    CsrBtCmPrim         type;           /* Type: CM_DM_LE_SET_PHY_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling process */
    CsrBtTpdAddrT       tpAddr;         /* Bluetooth device address with transport type and address type */
    CmDmLePhyInfo       phyInfo;        /* Information regarding the phy rates, see CmDmLePhyInfo */
} CmDmLeSetPhyReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Type: CM_DM_LE_SET_DEFAULT_PHY_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling process */
    CmDmLePhyInfo       phyInfo;        /* Information regarding the phy rates, see CmDmLePhyInfo */
} CmDmLeSetDefaultPhyReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Type: CM_DM_LE_PHY_SET_CFM */
    CsrBtTpdAddrT       tpAddr;         /* Bluetooth device address with transport type and address type */
    CsrUint8            txPhyType;      /* current tx PHY type of the connection */
    CsrUint8            rxPhyType;      /* current rx PHY type of the connection */
    hci_return_t        status;         /* Status of the command, anything other than zero is failure */
} CmDmLeSetPhyCfm;

typedef struct
{
    CsrBtCmPrim         type;           /* Type: CM_DM_LE_PHY_UPDATE_IND */
    CsrBtTpdAddrT       tpAddr;         /* Bluetooth device address with transport type and address type */
    CsrUint8            txPhyType;      /* current tx PHY type of the connection */
    CsrUint8            rxPhyType;      /* current rx PHY type of the connection */
} CmDmLePhyUpdateInd;

typedef struct
{
    CsrBtCmPrim         type;               /* Type: CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_REQ */
    CsrSchedQid         appHandle;          /* Application handle identifying the calling process */
    CsrUint16           connAcceptTimeout;  /* connection accept timeout */
} CmDmWriteConnAcceptTimeoutReq;

typedef struct
{
    CsrBtCmPrim         type;               /* Type: CM_DM_READ_CONN_ACCEPT_TIMEOUT_REQ */
    CsrSchedQid         appHandle;          /* Application handle identifying the calling process */
} CmDmReadConnAcceptTimeoutReq;

typedef struct
{
    CsrBtCmPrim         type;               /* Type: CM_DM_WRITE_CONN_ACCEPT_TIMEOUT_CFM */
    hci_return_t        status;             /* status */
} CmDmWriteConnAcceptTimeoutCfm;

typedef struct
{
    CsrBtCmPrim         type;               /* Type: CM_DM_READ_CONN_ACCEPT_TIMEOUT_CFM */
    hci_return_t        status;             /* status */
    CsrUint16           connAcceptTimeout;  /* connection accept timeout */
} CmDmReadConnAcceptTimeoutCfm;


typedef DM_ULP_SET_DEFAULT_PHY_CFM_T CmDmLeSetDefaultPhyCfm;
#ifdef INSTALL_CM_LE_PHY_UPDATE_FEATURE
typedef DM_ULP_READ_PHY_CFM_T CmDmReadPhyCfm;
#endif
typedef CsrUint16 CmSmSelectiveCTKDFlag;

#define CM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_DISABLE     ((CmSmSelectiveCTKDFlag)DM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_DISABLE)
#define CM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_ENABLE      ((CmSmSelectiveCTKDFlag)DM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_ENABLE)

typedef struct
{
    CsrBtCmPrim                 type;           /* Identity: CM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP */
    CsrBtTpdAddrT               tpAddr;         /* Remote device's bluetooth address. */
    CsrUint8                    identifier;     /* Same value as in CM_SM_GENERATE_CROSS_TRANS_KEY_IND */
    CmSmSelectiveCTKDFlag       flags;          /* Flag indicating selective CTKD to enable or disable,
                                                 * Refer to CmSmSelectiveCTKDFlag above.
                                                 */
} CmSmGenerateCrossTransKeyRequestRsp;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_L2CA_UNREGISTER_CFM */
    CsrUint16           localPsm;           /* PSM value which got unregistered. */
    CsrBtResultCode     resultCode;         /* Result of the unregister request API */
    CsrBtSupplier       resultSupplier;     /* Supplier of result code */
} CmL2caUnregisterCfm;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_WRITE_AUTO_FLUSH_TIMEOUT_CFM */
    CsrBtResultCode     resultCode;     /* result of the command */
    CsrBtSupplier       resultSupplier; /* supplier of the result code */
} CmDmWriteAutoFlushTimeoutCfm;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_CHANGE_CONNECTION_LINK_KEY_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling process */
    CsrBtDeviceAddr     deviceAddr;     /* Address of bluetooth device for which the connection link key needs to change. */
} CmDmChangeConnectionLinkKeyReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_CHANGE_CONNECTION_LINK_KEY_CFM */
    CsrBtDeviceAddr     deviceAddr;     /* Address of bluetooth device for which the connection link key needs to change. */
    CsrBtResultCode     resultCode;     /* result of the command */
    CsrBtSupplier       resultSupplier; /* supplier of the result code */
} CmDmChangeConnectionLinkKeyCfm;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_DM_READ_INQUIRY_TX_REQ */
    CsrSchedQid         appHandle;          /* Application handle identifying the calling process */
} CmDmReadInquiryTxReq;

typedef struct
{
    CsrBtCmPrim            type;            /* Identity: CM_DM_READ_INQUIRY_TX_CFM */
    CsrInt8                txPower;         /* Stores the Power level used to transmit the FHS and EIR Data packets
                                               Range => [-70,20] dBm */
    CsrBtResultCode        resultCode;      /* Result of the Command */
    CsrBtSupplier          resultSupplier;  /* Supplier of the Result Code */
} CmDmReadInquiryTxCfm;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_REQ */
    CsrSchedQid         appHandle;          /* Application handle identifying the calling process */
    CsrBtTpdAddrT       tpAddrt;            /* Stores Bluetooth device address and transport */
} CmDmReadAuthPayloadTimeoutReq;

typedef struct
{
    CsrBtCmPrim         type;                   /* Identity: CM_DM_READ_AUTH_PAYLOAD_TIMEOUT_CFM */
    CsrUint16           authPayloadTimeout;     /* Stores the maximum amount of time specified 
                                                   between packets authenticated by a MIC (Message Integrity Check) 
                                                   Time = authPayloadTimeout * 10ms
                                                   Default = 0x0BB8 (30s)
                                                   Range : 0x0001 to 0xFFFF (10 ms to 655,350 ms) */
    CsrBtTpdAddrT       tpAddrt;                /* Stores Bluetooth Device Address with Transport and Address type */
    CsrBtResultCode     resultCode;             /* Result of the Command */
    CsrBtSupplier       resultSupplier;         /* Supplier of the Result code */
} CmDmReadAuthPayloadTimeoutCfm;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_SM_KEY_REQUEST_RSP */
    CsrUint16           secRequirements;    /* Security Requirements */
    CsrBtTypedAddr      tpAddrt;            /* Stores Bluetooth Device Address and its type */
    DM_SM_KEY_TYPE_T    keyType;            /* Key Type */
    CsrBool             keyAvailable;       /* Key is present or not */
    DM_SM_UKEY_T        key;                /* Union of pointers to key structures */
} CmSmKeyRequestRsp;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_READ_EIR_DATA_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling process */
} CmDmReadEIRDataReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_READ_EIR_DATA_CFM */
    CsrUint8            fecRequired;    /* States if Forward Error Correction is required, 
                                           0x00 - FEC Not Required,
                                           0x01 - FEC Required */
    CsrUint8            eirDataLength;  /* Length of Parsed EIR Data in bytes */
    CsrUint8            *eirData;       /* Parsed EIR Data */
    CsrBtResultCode     resultCode;     /* Result of the Command */
    CsrBtSupplier       resultSupplier; /* Supplier of the result code */
} CmDmReadEIRDataCfm;

typedef struct
{
    CsrBtCmPrim     type;                           /* Identity: CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_REQ */
    CsrSchedQid     appHandle;                      /* Application handle identifying the calling Process */
    CsrBtTpdAddrT   tpAddrt;                        /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8        phy;                            /* Indicates whether the link is on 1M, 2M or Coded PHY */
} CmDmLeReadRemoteTransmitPowerLevelReq;

typedef struct
{
    CsrBtCmPrim         type;                       /* Identity: CM_DM_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL_CFM */
    CsrBtTpdAddrT       tpAddrt;                    /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8            phy;                        /* Indicates whether the link is on 1M, 2M or Coded PHY */
    CsrUint8            reason;                     /* Reason for Power Change */
    CsrInt8             txPowerLevel;               /* Stores the Transmission Power Level of the Remote Controller */
    CsrUint8            txPowerLevelFlag;           /* Transmit Power Level at Min/Max */
    CsrInt8             delta;                      /* Change in transmit Power Level (Zero means unchanged) */
    CsrBtResultCode     resultCode;                 /* Result of the Command */
    CsrBtSupplier       resultSupplier;             /* Supplier of the Result Code */
} CmDmLeReadRemoteTransmitPowerLevelCfm;

typedef struct
{
    CsrBtCmPrim     type;                                   /* Identity: CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_REQ */
    CsrSchedQid     appHandle;                              /* Application handle identifying the calling process */
    CsrBtTpdAddrT   tpAddrt;                                /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8        localEnable;                            /* Stores 0/1 => Disable/enable local transmit power reports respectively */
    CsrUint8        remoteEnable;                           /* Stores 0/1 => Disable/enable remote transmit power reports respectively */
} CmDmLeSetTransmitPowerReportingEnableReq;

typedef struct
{
    CsrBtCmPrim         type;                           /* Identity: CM_DM_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE_CFM */
    CsrBtTpdAddrT       tpAddrt;                        /* Stores Bluetooth Device Address with Transport and Address type */
    CsrBtResultCode     resultCode;                     /* Result of the Command */
    CsrBtSupplier       resultSupplier;                 /* Supplier of the Result code */
} CmDmLeSetTransmitPowerReportingEnableCfm;

/* Transmit Power Reporting Reason types */
typedef CsrUint8 CmTxPowerReportingReason;

/* Reported due to change in local tx Power */
#define CM_TX_POWER_REPORTING_REASON_LOCAL_TX_POW_CHANGED     ((CmTxPowerReportingReason)0x00)
/* Reported due to change in remote tx Power */
#define CM_TX_POWER_REPORTING_REASON_REMOTE_TX_POW_CHANGED    ((CmTxPowerReportingReason)0x01)

typedef struct
{
    CsrBtCmPrim                 type;                       /* Identity: CM_DM_LE_TRANSMIT_POWER_REPORTING_IND */
    CsrBtTpdAddrT               tpAddrt;                    /* Stores Bluetooth Device Address with Transport and Address type */
    CmTxPowerReportingReason    reason;                     /* Reason for Power Change */
    CsrUint8                    phy;                        /* Indicates whether the link is on 1M, 2M or Coded PHY */
    CsrInt8                     txPowerLevel;               /* Transmit Power Level */
    CsrUint8                    txPowerLevelFlag;           /* Transmit Power Level at Min/Max */
    CsrInt8                     delta;                      /* Change in transmit Power Level (Zero means unchanged) */
} CmDmLeTransmitPowerReportingInd;

typedef struct
{
    CsrBtCmPrim     type;           /* Identity: CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_REQ */
    CsrSchedQid     appHandle;      /* Application handle identifying the calling Process */
    CsrBtTpdAddrT   tpAddrt;        /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8        phy;            /* Indicates whether the link is on 1M, 2M or Coded PHY */
} CmDmLeEnhancedReadTransmitPowerLevelReq;

typedef struct
{
    CsrBtCmPrim         type;               /* Identity: CM_DM_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL_CFM */
    CsrBtTpdAddrT       tpAddrt;            /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8            phy;                /* Indicates whether the link is on 1M, 2M or Coded PHY */
    CsrInt8             curTxPowerLevel;    /* Stores the Current Transmission Power Level of the Local Controller */
    CsrInt8             maxTxPowerLevel;    /* Stores the Maximum Transmission Power Level of the Local Controller */
    CsrBtResultCode     resultCode;         /* Result of the Command */
    CsrBtSupplier       resultSupplier;     /* Supplier of the Result Code */
} CmDmLeEnhancedReadTransmitPowerLevelCfm;


#ifdef INSTALL_CM_LE_PATH_LOSS_REPORTING
typedef struct
{
    CsrBtCmPrim     type;               /* Identity: CM_DM_LE_SET_PATH_LOSS_REPORTING_PARAMETERS_REQ */
    CsrSchedQid     appHandle;          /* Application handle identifying the calling process */
    CsrBtTpdAddrT   tpAddrt;            /* Stores Bluetooth Device Address with Transport and Address type */
    CsrInt8         highThreshold;      /* High threshold for path loss in dB */
    CsrInt8         highHysteresis;     /* Hysteresis value for the high threshold in dB */
    CsrInt8         lowThreshold;       /* Low threshold for path loss in dB */
    CsrInt8         lowHysteresis;      /* Hysteresis value for the low threshold in dB */
    CsrUint16       minTimeSpent;       /* Minimum time spent in number of connection events to be observed
                                        once the path crosses the threshold before an event is generated */
} CmDmLeSetPathLossReportingParametersReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_LE_PATH_LOSS_POWER_REPORTING_PARAMETERS_CFM */
    CsrBtTpdAddrT       tpAddrt;        /* Stores Bluetooth Device Address with Transport and Address type */
    CsrBtResultCode     resultCode;     /* Result of the Command */
    CsrBtSupplier       resultSupplier; /* Supplier of the Result code */
} CmDmLeSetPathLossReportingParametersCfm;

typedef struct
{
    CsrBtCmPrim     type;               /* Identity: CM_DM_LE_SET_PATH_LOSS_REPORTING_ENABLE_REQ */
    CsrSchedQid     appHandle;          /* Application handle identifying the calling process */
    CsrBtTpdAddrT   tpAddrt;            /* Stores Bluetooth Device Address with Transport and Address type */
    CsrUint8        enable;             /* Stores 0/1 => Disable/enable LE Power Loss Reporting */
} CmDmLeSetPathLossReportingEnableReq;

/* Identity: CM_DM_LE_PATH_LOSS_POWER_REPORTING_ENABLE_CFM */
typedef CmDmLeSetPathLossReportingParametersCfm CmDmLeSetPathLossReportingEnableCfm;

typedef struct
{
    CsrBtCmPrim     type;               /* Identity: CM_DM_LE_PATH_LOSS_THRESHOLD_IND */
    CsrBtTpdAddrT   tpAddrt;            /* Bluetooth Device Address with Transport and Address type */
    CsrUint8        currPathLoss;       /* Indicates the current path loss value as calculated by the controller. */
    CsrUint8        zoneEntered;        /* Which zone was entered */
} CmDmLePathLossThresholdInd;
#endif /* INSTALL_CM_LE_PATH_LOSS_REPORTING */

typedef struct
{
    CsrBtCmPrim     type;           /* Identity: CM_DM_WRITE_INQUIRY_MODE_REQ */
    CsrSchedQid     appHandle;      /* Application handle identifying the calling Process */
    CsrUint8        mode;           /* Sets the Inquiry Mode of Local Device,
                                       can take 3 Values:
                                       0x00 - Std Inquiry Result Event Format,
                                       0x01 - Inquiry result format with RSSI,
                                       0x02 - Inquiry Result with RSSI or EIR format */
} CmDmWriteInquiryModeReq;

typedef struct
{
    CsrBtCmPrim            type;            /* Identity: CM_DM_WRITE_INQUIRY_MODE_CFM */
    CsrBtResultCode        resultCode;      /* Result of the Command */
    CsrBtSupplier          resultSupplier;  /* Supplier of the Result Code */
} CmDmWriteInquiryModeCfm;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CM_DM_EXT_ADV_SET_PARAMS_V2_REQ */
    CsrSchedQid                 appHandle;       /* Destination phandle */
    CsrUint8                    advHandle;
    CsrUint16                   advEventProperties;
    CsrUint32                   primaryAdvIntervalMin;
    CsrUint32                   primaryAdvIntervalMax;
    CsrUint8                    primaryAdvChannelMap;
    CsrUint8                    ownAddrType;
    TYPED_BD_ADDR_T             peerAddr;
    CsrUint8                    advFilterPolicy;
    CsrUint16                   primaryAdvPhy;
    CsrUint8                    secondaryAdvMaxSkip;
    CsrUint16                   secondaryAdvPhy;
    CsrUint16                   advSid;
    CsrInt8                     advTxPower;
    CsrUint8                    scanReqNotifyEnable;         /* Reserved for future use */
    CsrUint8                    primaryAdvPhyOptions;        /* Reserved for future use */
    CsrUint8                    secondaryAdvPhyOptions;      /* Reserved for future use */
} CmDmExtAdvSetParamsV2Req;

typedef struct
{
    CsrBtCmPrim                 type;            /* Identity: CM_DM_EXT_ADV_SET_PARAMS_V2_CFM */
    CsrBtResultCode             resultCode;
    CsrUint8                    advSid;
    CsrUint8                    advHandle;
    CsrInt8                     selectedTxPower;
} CmDmExtAdvSetParamsV2Cfm;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_REMOVE_DEVICE_KEY_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling Process */
    CsrBtDeviceAddr     deviceAddr;     /* Bluetooth device address */
    CsrBtAddressType    addressType;    /* Type of the bluetooth device address (public/random) */
    CsrUint8            keyType;        /* Type of the key information to be removed (0-BREDR, 1-LE) */
} CmDmRemoveDeviceKeyReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_REMOVE_DEVICE_KEY_CFM */
    CsrBtDeviceAddr     deviceAddr;     /* Bluetooth device address */
    CsrBtAddressType    addressType;    /* Type of the bluetooth device address (public/random) */
    CsrBtResultCode     resultCode;     /* Result of the API */
    CsrBtSupplier       resultSupplier; /* Supplier of the result refer to CsrBtSupplier */
} CmDmRemoveDeviceKeyCfm;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_REMOVE_DEVICE_OPTIONS_REQ */
    CsrSchedQid         appHandle;      /* Application handle identifying the calling Process */
    CsrBtDeviceAddr     deviceAddr;     /* Bluetooth device address */
    CsrBtAddressType    addressType;    /* Type of the bluetooth device address (public/random) */
    CsrUint8            option;         /* Remove device option. Refer CmDeviceRemovalOption for more details.*/
} CmDmRemoveDeviceOptionsReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_REMOVE_DEVICE_OPTIONS_CFM */
    CsrBtDeviceAddr     deviceAddr;     /* Bluetooth device address */
    CsrBtAddressType    addressType;    /* Type of the bluetooth device address (public/random) */
    CsrBtResultCode     resultCode;     /* Result of the API */
    CsrBtSupplier       resultSupplier; /* Supplier of the result refer to CsrBtSupplier */
} CmDmRemoveDeviceOptionsCfm;

typedef struct
{
    CsrBtCmPrim                 type;        /* Indentity: CM_DM_SNIFF_SUB_RATE_REQ */
    CsrSchedQid                 appHandle;   /* Application handle identifying the calling process */
    CsrBtDeviceAddr             deviceAddr;  /* Bluetooth device address */
    CsrBtSsrSettingsDownstream  ssrSettings; /* Sniff subrate settings */
} CmDmSniffSubRateReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_DM_SNIFF_SUB_RATE_CFM */
    CsrBtDeviceAddr     deviceAddr;     /* Bluetooth device address */
    CsrBtResultCode     resultCode;     /* Result of the API */
    CsrBtSupplier       resultSupplier; /* Supplier of the result refer to CsrBtSupplier */
} CmDmSniffSubRateCfm;

#ifdef INSTALL_CM_SM_CONFIG
typedef CsrUint16 CmSmConfigMask;

#define CM_SM_CONFIG_MASK_INVALID               DM_SM_CONFIG_MASK_INVALID
/* This flag allows the application to provide it's own LTK for an encrypted connection.
 * Application is consulted each time when encryption is required.*/
#define CM_SM_CONFIG_MASK_APP_LTK_QUERY_ENABLE  DM_SM_CONFIG_MASK_APP_LTK_QUERY_ENABLE

typedef struct
{
    CsrBtCmPrim     type;           /* Identity: CM_SM_CONFIG_REQ */
    CsrSchedQid     appHandle;      /* Application handle identifying the calling process */
    CmSmConfigMask  configMask;     /* Configuration mask for the security manager, see CmSmConfigMask */
    CsrUint8        length;         /* Shall not be used, reserved for future use */
    void            *params;        /* Shall not be used, reserved for future use */
} CmSmConfigReq;

typedef struct
{
    CsrBtCmPrim         type;           /* Identity: CM_SM_CONFIG_CFM */
    CsrBtResultCode     resultCode;     /* Result of the API */
    CsrBtSupplier       resultSupplier; /* Supplier of the result refer to CsrBtSupplier */
} CmSmConfigCfm;
#endif /* INSTALL_CM_SM_CONFIG */

typedef DM_SM_PIN_REQUEST_IND_T CsrBtCmSmPinRequestInd;
typedef DM_SM_IO_CAPABILITY_RESPONSE_IND_T CsrBtCmSmIoCapabilityResponseInd;
typedef DM_SM_IO_CAPABILITY_REQUEST_IND_T CsrBtCmSmIoCapabilityRequestInd;
typedef DM_SM_USER_CONFIRMATION_REQUEST_IND_T CsrBtCmSmUserConfirmationRequestInd;
typedef DM_SM_USER_PASSKEY_REQUEST_IND_T CsrBtCmSmUserPasskeyRequestInd;
typedef DM_SM_USER_PASSKEY_NOTIFICATION_IND_T CsrBtCmSmUserPasskeyNotificationInd;
typedef DM_SM_KEYPRESS_NOTIFICATION_IND_T CsrBtCmSmKeypressNotificationInd;
typedef DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T CsrBtCmSmSimplePairingCompleteInd;
typedef DM_SM_AUTHENTICATE_CFM_T CsrBtCmSmAuthenticateCfm;
typedef DM_SM_SECURITY_IND_T CsrBtCmSmSecurityInd;
typedef DM_SM_CSRK_COUNTER_CHANGE_IND_T CsrBtCmSmCsrkCounterChangeInd;
typedef DM_SM_READ_LOCAL_OOB_DATA_CFM_T CsrBtCmSmReadLocalOobDataCfm;
typedef DM_SM_LOCAL_KEY_DELETED_IND_T CsrBtCmSmLocalKeyDeletedInd;
typedef DM_SM_ENCRYPTION_CHANGE_IND_T CsrBtCmSmEncryptionChangeInd;
typedef DM_SM_ENCRYPT_CFM_T CsrBtCmSmEncryptCfm;
typedef DM_SM_ACCESS_IND_T CsrBtCmSmAccessInd;
typedef DM_SM_AUTHORISE_IND_T CsrBtCmSmAuthoriseInd;
typedef DM_SM_INIT_CFM_T CsrBtCmSmInitCfm;
typedef DM_SM_REMOVE_DEVICE_CFM_T CsrBtCmSmRemoveDeviceCfm;
typedef DM_SM_ADD_DEVICE_CFM_T CsrBtCmSmAddDeviceCfm;
typedef DM_SM_SECURITY_CFM_T CsrBtCmSmSecurityCfm;
typedef DM_SM_KEYS_IND_T CsrBtCmSmKeysInd;
typedef DM_SM_BONDING_CFM_T CsrBtCmSmBondingCfm;
typedef DM_SM_KEY_REQUEST_IND_T CmSmKeyRequestInd;

typedef DM_HCI_CREATE_CONNECTION_CANCEL_CFM_T CsrBtCmHciCreateConnectionCancelCfm;
typedef DM_HCI_DELETE_STORED_LINK_KEY_CFM_T CsrBtCmHciDeleteStoredLinkKeyCfm;
typedef DM_HCI_REFRESH_ENCRYPTION_KEY_IND_T CsrBtCmHciRefreshEncryptionKeyInd;

typedef DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND_T CmSmGenerateCrossTransKeyRequestInd;

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CM_PRIM_H */
