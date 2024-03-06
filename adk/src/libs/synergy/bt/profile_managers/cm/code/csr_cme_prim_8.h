#ifndef CME_PRIM_H__
#define CME_PRIM_H__
/******************************************************************************
 Copyright (c) 2013-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/
/* 
 * NOTE: Primitives definitions and macros used here must match with
 * Host CME interface from mainline; cme_prim_8.h#28
 */

#include <csr_types.h>


#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************

  NAME
    CME_A2DP_ROLE

  DESCRIPTION
    A2DP Device role for A2DP profile.

 VALUES
    CME_A2DP_SOURCE -
    CME_A2DP_SINK   -

*******************************************************************************/
typedef unsigned int CsrCmeA2dpRole;
#define CME_A2DP_SOURCE ((CsrCmeA2dpRole) 0)
#define CME_A2DP_SINK ((CsrCmeA2dpRole) 1)

/*******************************************************************************

  NAME
    CME_ACTIVITY_TYPE

  DESCRIPTION
    Index into table specifying the CDL priorities to be used for a set of BT
    activities.

 VALUES
    CME_ACTIVITY_TYPE_PAGE
                   - Page
    CME_ACTIVITY_TYPE_PAGE_SCAN
                   - Page scan
    CME_ACTIVITY_TYPE_SNIFF
                   - Sniff
    CME_ACTIVITY_TYPE_ACL
                   - Bulk ACL
    CME_ACTIVITY_TYPE_BROADCAST
                   - Broadcast
    CME_ACTIVITY_TYPE_PARK
                   - Park
    CME_ACTIVITY_TYPE_BAND_SCAN
                   - Band scan
    CME_ACTIVITY_TYPE_CONDITIONAL_SCAN
                   - Conditional scan
    CME_ACTIVITY_TYPE_RADIO_TRIM
                   - Radio trim
    CME_ACTIVITY_TYPE_BLE_ADV_NC
                   - Non-connectable Advertising
    CME_ACTIVITY_TYPE_BLE_ADV_DU
                   - Discoverable Advertising
    CME_ACTIVITY_TYPE_BLE_ADV_CU
                   - Connectable Undirected Advertising
    CME_ACTIVITY_TYPE_INQUIRY
                   - Inquiry
    CME_ACTIVITY_TYPE_BLE_ADV_CD
                   - Connectable Directed Advertising
    CME_ACTIVITY_TYPE_BLE_ADV_SCAN_RSP
                   - Advertising - Scan Response
    CME_ACTIVITY_TYPE_BLE_SCAN_PASSIVE
                   - Passive scanning
    CME_ACTIVITY_TYPE_BLE_SCAN_ACTIVE
                   - Active scanning
    CME_ACTIVITY_TYPE_BLE_SCAN_SCAN_RSP
                   - Active Scanning - Scan Response
    CME_ACTIVITY_TYPE_BLE_INITIATOR
                   - Initiator
    CME_ACTIVITY_TYPE_BLE_CONN_EST_MASTER
                   - Connection Establishment (Master)
    CME_ACTIVITY_TYPE_BLE_CONN_EST_SLAVE
                   - Connection Establishment (Slave)
    CME_ACTIVITY_TYPE_BLE_ANCHOR_MASTER
                   - Anchor Point (Master)
    CME_ACTIVITY_TYPE_BLE_ANCHOR_SLAVE
                   - Anchor Point (Slave)
    CME_ACTIVITY_TYPE_INQUIRY_SCAN
                   - Inquiry scan
    CME_ACTIVITY_TYPE_BLE_MORE_DATA_MASTER
                   - Data (Master)
    CME_ACTIVITY_TYPE_BLE_MORE_DATA_SLAVE
                   - Data (Slave)
    CME_ACTIVITY_TYPE_SIZE
                   -
    CME_ACTIVITY_TYPE_ROLE_SWITCH
                   - Role switch
    CME_ACTIVITY_TYPE_LMP_TO_MASTER
                   - LMP to master
    CME_ACTIVITY_TYPE_LMP_FROM_MASTER
                   - LMP from master
    CME_ACTIVITY_TYPE_SCO_ESCO
                   - (e)SCO
    CME_ACTIVITY_TYPE_ESCO_RETRANS
                   - eSCO retransmission
    CME_ACTIVITY_TYPE_POLLING
                   - Poll

*******************************************************************************/
typedef CsrUint8 CsrCmeActivityType;
#define CME_ACTIVITY_TYPE_PAGE ((CsrCmeActivityType) 0)
#define CME_ACTIVITY_TYPE_PAGE_SCAN ((CsrCmeActivityType) 1)
#define CME_ACTIVITY_TYPE_INQUIRY ((CsrCmeActivityType) 2)
#define CME_ACTIVITY_TYPE_INQUIRY_SCAN ((CsrCmeActivityType) 3)
#define CME_ACTIVITY_TYPE_ROLE_SWITCH ((CsrCmeActivityType) 4)
#define CME_ACTIVITY_TYPE_LMP_TO_MASTER ((CsrCmeActivityType) 5)
#define CME_ACTIVITY_TYPE_LMP_FROM_MASTER ((CsrCmeActivityType) 6)
#define CME_ACTIVITY_TYPE_SCO_ESCO ((CsrCmeActivityType) 7)
#define CME_ACTIVITY_TYPE_ESCO_RETRANS ((CsrCmeActivityType) 8)
#define CME_ACTIVITY_TYPE_POLLING ((CsrCmeActivityType) 9)
#define CME_ACTIVITY_TYPE_SNIFF ((CsrCmeActivityType) 10)
#define CME_ACTIVITY_TYPE_ACL ((CsrCmeActivityType) 11)
#define CME_ACTIVITY_TYPE_BROADCAST ((CsrCmeActivityType) 12)
#define CME_ACTIVITY_TYPE_PARK ((CsrCmeActivityType) 13)
#define CME_ACTIVITY_TYPE_BAND_SCAN ((CsrCmeActivityType) 14)
#define CME_ACTIVITY_TYPE_CONDITIONAL_SCAN ((CsrCmeActivityType) 15)
#define CME_ACTIVITY_TYPE_RADIO_TRIM ((CsrCmeActivityType) 16)
#define CME_ACTIVITY_TYPE_BLE_ADV_NC ((CsrCmeActivityType) 17)
#define CME_ACTIVITY_TYPE_BLE_ADV_DU ((CsrCmeActivityType) 18)
#define CME_ACTIVITY_TYPE_BLE_ADV_CU ((CsrCmeActivityType) 19)
#define CME_ACTIVITY_TYPE_BLE_ADV_CD ((CsrCmeActivityType) 20)
#define CME_ACTIVITY_TYPE_BLE_ADV_SCAN_RSP ((CsrCmeActivityType) 21)
#define CME_ACTIVITY_TYPE_BLE_SCAN_PASSIVE ((CsrCmeActivityType) 22)
#define CME_ACTIVITY_TYPE_BLE_SCAN_ACTIVE ((CsrCmeActivityType) 23)
#define CME_ACTIVITY_TYPE_BLE_SCAN_SCAN_RSP ((CsrCmeActivityType) 24)
#define CME_ACTIVITY_TYPE_BLE_INITIATOR ((CsrCmeActivityType) 25)
#define CME_ACTIVITY_TYPE_BLE_CONN_EST_MASTER ((CsrCmeActivityType) 26)
#define CME_ACTIVITY_TYPE_BLE_CONN_EST_SLAVE ((CsrCmeActivityType) 27)
#define CME_ACTIVITY_TYPE_BLE_ANCHOR_MASTER ((CsrCmeActivityType) 28)
#define CME_ACTIVITY_TYPE_BLE_ANCHOR_SLAVE ((CsrCmeActivityType) 29)
#define CME_ACTIVITY_TYPE_BLE_MORE_DATA_MASTER ((CsrCmeActivityType) 30)
#define CME_ACTIVITY_TYPE_BLE_MORE_DATA_SLAVE ((CsrCmeActivityType) 31)
#define CME_ACTIVITY_TYPE_SIZE ((CsrCmeActivityType) 32)

/*******************************************************************************

  NAME
    CME_BT_SCHEDULER_RESTRICTION

  DESCRIPTION
    Temporary restriction applied to the LC scheduling algorithm to
    facilitate WLAN operation.

 VALUES
    CME_BT_SCHEDULER_RESTRICTION_ALL_ALLOWED
                   - No restrictions - All BT activities allowed. Time values
                     are not relevant
    CME_BT_SCHEDULER_RESTRICTION_NO_LOW_PRIORITY
                   - (e)SCO and polling (master) / sniff (slave) allowed between
                     stop_bt_time and start_bt_time
    CME_BT_SCHEDULER_RESTRICTION_NO_LOW_OR_HIGH_PRIORITY
                   - only (e)SCO (master) allowed between stop_bt_time and
                     start_bt_time
    CME_BT_SCHEDULER_RESTRICTION_NO_ACTIVITIES_ALLOWED
                   - No BT activities allowed between stop_bt_time and
                     start_bt_time

*******************************************************************************/
typedef CsrUint16 CsrCmeBtSchedulerRestriction;
#define CME_BT_SCHEDULER_RESTRICTION_ALL_ALLOWED ((CsrCmeBtSchedulerRestriction) 0)
#define CME_BT_SCHEDULER_RESTRICTION_NO_LOW_PRIORITY ((CsrCmeBtSchedulerRestriction) 1)
#define CME_BT_SCHEDULER_RESTRICTION_NO_LOW_OR_HIGH_PRIORITY ((CsrCmeBtSchedulerRestriction) 2)
#define CME_BT_SCHEDULER_RESTRICTION_NO_ACTIVITIES_ALLOWED ((CsrCmeBtSchedulerRestriction) 3)

/*******************************************************************************

  NAME
    CME_CDL_PRIORITY

  DESCRIPTION
    CDL priority values (4 bits). Priority can be set to any value between
    CME_CDL_PRIORITY_MIN and CME_CDL_PRIORITY_MAX.

 VALUES
    CME_CDL_PRIORITY_MIN -
    CME_CDL_PRIORITY_MAX -

*******************************************************************************/
typedef unsigned int CsrCmeCdlPriority;
#define CME_CDL_PRIORITY_MIN ((CsrCmeCdlPriority) 0)
#define CME_CDL_PRIORITY_MAX ((CsrCmeCdlPriority) 15)

/*******************************************************************************

  NAME
    CME_CODEC_LOCATION

  DESCRIPTION
    Codec locations for A2DP profile.

 VALUES
    CME_CODEC_LOCATION_OFF_CHIP -
    CME_CODEC_LOCATION_ON_CHIP  -

*******************************************************************************/
typedef unsigned int CsrCmeCodecLocation;
#define CME_CODEC_LOCATION_OFF_CHIP ((CsrCmeCodecLocation) 0)
#define CME_CODEC_LOCATION_ON_CHIP ((CsrCmeCodecLocation) 1)

/*******************************************************************************

  NAME
    CME_CODEC_SAMPLING_FREQ

  DESCRIPTION
    Sampling frequency of the A2DP audio stream.

 VALUES
    CME_CODEC_16000_SAMPLING_FREQ -
    CME_CODEC_32000_SAMPLING_FREQ -
    CME_CODEC_44100_SAMPLING_FREQ -
    CME_CODEC_48000_SAMPLING_FREQ -

*******************************************************************************/
typedef unsigned int CsrCmeCodecSamplingFreq;
#define CME_CODEC_16000_SAMPLING_FREQ ((CsrCmeCodecSamplingFreq) 0)
#define CME_CODEC_32000_SAMPLING_FREQ ((CsrCmeCodecSamplingFreq) 1)
#define CME_CODEC_44100_SAMPLING_FREQ ((CsrCmeCodecSamplingFreq) 2)
#define CME_CODEC_48000_SAMPLING_FREQ ((CsrCmeCodecSamplingFreq) 3)

/*******************************************************************************

  NAME
    CME_CODEC_TYPE

  DESCRIPTION
    Codec types used for A2DP profile.

 VALUES
    CME_CODEC_TYPE_SBC  -
    CME_CODEC_TYPE_APTX -

*******************************************************************************/
typedef unsigned int CsrCmeCodecType;
#define CME_CODEC_TYPE_SBC ((CsrCmeCodecType) 0)
#define CME_CODEC_TYPE_APTX ((CsrCmeCodecType) 1)

/*******************************************************************************

  NAME
    CME_Signal_Id

  DESCRIPTION

 VALUES
    coex_service_active_ind      -
    sync_req                     -
    acl_role_switch_req          -
    acl_role_switch_cfm          -
    esco_connect_ind             -
    esco_disconnect_ind          -
    sniff_mode_ind               -
    sniff_mode_off_ind           -
    sniff_event_end_ind          -
    inquiry_start_ind            -
    inquiry_end_ind              -
    page_start_ind               -
    sync_cfm                     -
    page_end_ind                 -
    inquiry_scan_start_ind       -
    inquiry_scan_end_ind         -
    page_scan_start_ind          -
    page_scan_end_ind            -
    acl_data_start_ind           -
    replay_req                   -
    acl_data_end_ind             -
    ble_connect_ind              -
    ble_disconnect_req           -
    ble_disconnect_cfm           -
    ble_scan_enabled_ind         -
    ble_scan_disabled_ind        -
    wlan_channel_map_ind         -
    profile_a2dp_start_ind       -
    profile_a2dp_stop_ind        -
    acl_flow_control_ind         -
    replay_cfm                   -
    acl_threshold_ind            -
    bt_cal_start_req             -
    bt_cal_start_cfm             -
    bt_cal_end_ind               -
    wlan_channel_map_lo_hi_ind   -
    coex_stop_ind                -
    ble_connection_event_end_ind -
    host_replay_req              -
    host_replay_cfm              -
    acl_connect_ind              -
    acl_disconnect_req           -
    acl_disconnect_cfm           -

*******************************************************************************/
typedef CsrUint8 CsrCmeSignalId;
#define CSR_CME_SIGNAL_ID_COEX_SERVICE_ACTIVE_IND ((CsrCmeSignalId) 0)
#define CSR_CME_SIGNAL_ID_SYNC_REQ ((CsrCmeSignalId) 1)
#define CSR_CME_SIGNAL_ID_SYNC_CFM ((CsrCmeSignalId) 2)
#define CSR_CME_SIGNAL_ID_REPLAY_REQ ((CsrCmeSignalId) 3)
#define CSR_CME_SIGNAL_ID_REPLAY_CFM ((CsrCmeSignalId) 4)
#define CSR_CME_SIGNAL_ID_HOST_REPLAY_REQ ((CsrCmeSignalId) 5)
#define CSR_CME_SIGNAL_ID_HOST_REPLAY_CFM ((CsrCmeSignalId) 6)
#define CSR_CME_SIGNAL_ID_ACL_CONNECT_IND ((CsrCmeSignalId) 7)
#define CSR_CME_SIGNAL_ID_ACL_DISCONNECT_REQ ((CsrCmeSignalId) 8)
#define CSR_CME_SIGNAL_ID_ACL_DISCONNECT_CFM ((CsrCmeSignalId) 9)
#define CSR_CME_SIGNAL_ID_ACL_ROLE_SWITCH_REQ ((CsrCmeSignalId) 10)
#define CSR_CME_SIGNAL_ID_ACL_ROLE_SWITCH_CFM ((CsrCmeSignalId) 11)
#define CSR_CME_SIGNAL_ID_ESCO_CONNECT_IND ((CsrCmeSignalId) 12)
#define CSR_CME_SIGNAL_ID_ESCO_DISCONNECT_IND ((CsrCmeSignalId) 13)
#define CSR_CME_SIGNAL_ID_SNIFF_MODE_IND ((CsrCmeSignalId) 14)
#define CSR_CME_SIGNAL_ID_SNIFF_MODE_OFF_IND ((CsrCmeSignalId) 15)
#define CSR_CME_SIGNAL_ID_SNIFF_EVENT_END_IND ((CsrCmeSignalId) 16)
#define CSR_CME_SIGNAL_ID_INQUIRY_START_IND ((CsrCmeSignalId) 17)
#define CSR_CME_SIGNAL_ID_INQUIRY_END_IND ((CsrCmeSignalId) 18)
#define CSR_CME_SIGNAL_ID_PAGE_START_IND ((CsrCmeSignalId) 19)
#define CSR_CME_SIGNAL_ID_PAGE_END_IND ((CsrCmeSignalId) 20)
#define CSR_CME_SIGNAL_ID_INQUIRY_SCAN_START_IND ((CsrCmeSignalId) 23)
#define CSR_CME_SIGNAL_ID_INQUIRY_SCAN_END_IND ((CsrCmeSignalId) 24)
#define CSR_CME_SIGNAL_ID_PAGE_SCAN_START_IND ((CsrCmeSignalId) 27)
#define CSR_CME_SIGNAL_ID_PAGE_SCAN_END_IND ((CsrCmeSignalId) 28)
#define CSR_CME_SIGNAL_ID_ACL_DATA_START_IND ((CsrCmeSignalId) 29)
#define CSR_CME_SIGNAL_ID_ACL_DATA_END_IND ((CsrCmeSignalId) 30)
#define CSR_CME_SIGNAL_ID_BLE_CONNECT_IND ((CsrCmeSignalId) 31)
#define CSR_CME_SIGNAL_ID_BLE_DISCONNECT_REQ ((CsrCmeSignalId) 32)
#define CSR_CME_SIGNAL_ID_BLE_DISCONNECT_CFM ((CsrCmeSignalId) 33)
#define CSR_CME_SIGNAL_ID_BLE_SCAN_ENABLED_IND ((CsrCmeSignalId) 34)
#define CSR_CME_SIGNAL_ID_BLE_SCAN_DISABLED_IND ((CsrCmeSignalId) 35)
#define CSR_CME_SIGNAL_ID_WLAN_CHANNEL_MAP_IND ((CsrCmeSignalId) 36)
#define CSR_CME_SIGNAL_ID_PROFILE_A2DP_START_IND ((CsrCmeSignalId) 37)
#define CSR_CME_SIGNAL_ID_PROFILE_A2DP_STOP_IND ((CsrCmeSignalId) 38)
#define CSR_CME_SIGNAL_ID_ACL_FLOW_CONTROL_IND ((CsrCmeSignalId) 39)
#define CSR_CME_SIGNAL_ID_ACL_THRESHOLD_IND ((CsrCmeSignalId) 40)
#define CSR_CME_SIGNAL_ID_BT_CAL_START_REQ ((CsrCmeSignalId) 41)
#define CSR_CME_SIGNAL_ID_BT_CAL_START_CFM ((CsrCmeSignalId) 42)
#define CSR_CME_SIGNAL_ID_BT_CAL_END_IND ((CsrCmeSignalId) 43)
#define CSR_CME_SIGNAL_ID_WLAN_CHANNEL_MAP_LO_HI_IND ((CsrCmeSignalId) 44)
#define CSR_CME_SIGNAL_ID_COEX_STOP_IND ((CsrCmeSignalId) 45)
#define CSR_CME_SIGNAL_ID_BLE_CONNECTION_EVENT_END_IND ((CsrCmeSignalId) 46)


#define CME_PRIM_8_ANY_SIZE 1

/*******************************************************************************

  NAME
    CME_BT_CDL_Priority_Table

  DESCRIPTION
    Table specifying the CDL priorities to be used for a set of BT
    activities. The priority values are packed 4 per 16-bit word.

  MEMBERS
    page                      - CDL priority for page
    page_scan                 - CDL priority for page scan
    inquiry                   - CDL priority for inquiry
    inquiry_scan              - CDL priority for inquiry scan
    role_switch               - CDL priority for role_switch
    lmp_to_master             - CDL priority for LMP to master
    lmp_from_master           - CDL priority for LMP from master
    esco                      - CDL priority for SCO/eSCO
    esco_retrans              - CDL priority for eSCO retransmissions
    polling                   - CDL priority for polling
    sniff                     - CDL priority for start of sniff
    bulk_acl                  - CDL priority for bulk ACL
    broadcast                 - CDL priority for broadcast transmissions
    park                      - CDL priority for park
    band_scan                 - CDL priority for band scan
    cond_scan                 - CDL priority for conditional scan
    trim                      - CDL priority for radio trim
    ble_nonconnectable_advert - CDL priority for BLE non-connectable advertising
    ble_discoverable_advert   - CDL priority for BLE discoverable advertising
    ble_undirected_advert     - CDL priority for BLE connectable undirected
                                advertising
    ble_directed_advert       - CDL priority for BLE connectable directed
                                advertising
    ble_advert_scan_response  - CDL priority for BLE advertising scan response
    ble_passive_scan          - CDL priority for BLE passive scanning
    ble_active_scan           - CDL priority for BLE active scanning
    ble_active_scan_response  - CDL priority for BLE active scanning scan
                                response
    ble_initiator             - CDL priority for BLE initiator
    ble_master_establishment  - CDL priority for BLE connection establishment
                                (master)
    ble_slave_establishment   - CDL priority for BLE connection establishment
                                (slave)
    ble_master_anchor         - CDL priority for BLE anchor point (master)
    ble_slave_anchor          - CDL priority for BLE anchor point (slave)
    ble_master_data           - CDL priority for BLE data (master)
    ble_slave_data            - CDL priority for BLE data (slave)

*******************************************************************************/
typedef struct
{
    CsrUint8 _data[16];
} CsrCmeBtCdlPriorityTable;

/* The following macros take CME_BT_CDL_PRIORITY_TABLE *cme_bt_cdl_priority_table_ptr */
#define CME_BT_CDL_PRIORITY_TABLE_PAGE_BYTE_OFFSET (0)
#define CME_BT_CDL_PRIORITY_TABLE_PAGE_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[0] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_PAGE_SET(cme_bt_cdl_priority_table_ptr, page) ((cme_bt_cdl_priority_table_ptr)->_data[0] =  \
                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[0] & ~0xf) | (((page)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_PAGE_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[0] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_PAGE_SCAN_SET(cme_bt_cdl_priority_table_ptr, page_scan) ((cme_bt_cdl_priority_table_ptr)->_data[0] =  \
                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[0] & ~0xf0) | (((page_scan) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_INQUIRY_BYTE_OFFSET (1)
#define CME_BT_CDL_PRIORITY_TABLE_INQUIRY_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[1] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_INQUIRY_SET(cme_bt_cdl_priority_table_ptr, inquiry) ((cme_bt_cdl_priority_table_ptr)->_data[1] =  \
                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[1] & ~0xf) | (((inquiry)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_INQUIRY_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[1] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_INQUIRY_SCAN_SET(cme_bt_cdl_priority_table_ptr, inquiry_scan) ((cme_bt_cdl_priority_table_ptr)->_data[1] =  \
                                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[1] & ~0xf0) | (((inquiry_scan) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_ROLE_SWITCH_BYTE_OFFSET (2)
#define CME_BT_CDL_PRIORITY_TABLE_ROLE_SWITCH_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[2] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_ROLE_SWITCH_SET(cme_bt_cdl_priority_table_ptr, role_switch) ((cme_bt_cdl_priority_table_ptr)->_data[2] =  \
                                                                                                   (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[2] & ~0xf) | (((role_switch)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_LMP_TO_MASTER_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[2] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_LMP_TO_MASTER_SET(cme_bt_cdl_priority_table_ptr, lmp_to_master) ((cme_bt_cdl_priority_table_ptr)->_data[2] =  \
                                                                                                       (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[2] & ~0xf0) | (((lmp_to_master) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_LMP_FROM_MASTER_BYTE_OFFSET (3)
#define CME_BT_CDL_PRIORITY_TABLE_LMP_FROM_MASTER_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[3] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_LMP_FROM_MASTER_SET(cme_bt_cdl_priority_table_ptr, lmp_from_master) ((cme_bt_cdl_priority_table_ptr)->_data[3] =  \
                                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[3] & ~0xf) | (((lmp_from_master)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_ESCO_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[3] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_ESCO_SET(cme_bt_cdl_priority_table_ptr, esco) ((cme_bt_cdl_priority_table_ptr)->_data[3] =  \
                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[3] & ~0xf0) | (((esco) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_ESCO_RETRANS_BYTE_OFFSET (4)
#define CME_BT_CDL_PRIORITY_TABLE_ESCO_RETRANS_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[4] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_ESCO_RETRANS_SET(cme_bt_cdl_priority_table_ptr, esco_retrans) ((cme_bt_cdl_priority_table_ptr)->_data[4] =  \
                                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[4] & ~0xf) | (((esco_retrans)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_POLLING_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[4] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_POLLING_SET(cme_bt_cdl_priority_table_ptr, polling) ((cme_bt_cdl_priority_table_ptr)->_data[4] =  \
                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[4] & ~0xf0) | (((polling) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_SNIFF_BYTE_OFFSET (5)
#define CME_BT_CDL_PRIORITY_TABLE_SNIFF_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[5] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_SNIFF_SET(cme_bt_cdl_priority_table_ptr, sniff) ((cme_bt_cdl_priority_table_ptr)->_data[5] =  \
                                                                                       (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[5] & ~0xf) | (((sniff)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BULK_ACL_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[5] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BULK_ACL_SET(cme_bt_cdl_priority_table_ptr, bulk_acl) ((cme_bt_cdl_priority_table_ptr)->_data[5] =  \
                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[5] & ~0xf0) | (((bulk_acl) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BROADCAST_BYTE_OFFSET (6)
#define CME_BT_CDL_PRIORITY_TABLE_BROADCAST_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[6] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BROADCAST_SET(cme_bt_cdl_priority_table_ptr, broadcast) ((cme_bt_cdl_priority_table_ptr)->_data[6] =  \
                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[6] & ~0xf) | (((broadcast)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_PARK_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[6] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_PARK_SET(cme_bt_cdl_priority_table_ptr, park) ((cme_bt_cdl_priority_table_ptr)->_data[6] =  \
                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[6] & ~0xf0) | (((park) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BAND_SCAN_BYTE_OFFSET (7)
#define CME_BT_CDL_PRIORITY_TABLE_BAND_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[7] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BAND_SCAN_SET(cme_bt_cdl_priority_table_ptr, band_scan) ((cme_bt_cdl_priority_table_ptr)->_data[7] =  \
                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[7] & ~0xf) | (((band_scan)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_COND_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[7] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_COND_SCAN_SET(cme_bt_cdl_priority_table_ptr, cond_scan) ((cme_bt_cdl_priority_table_ptr)->_data[7] =  \
                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[7] & ~0xf0) | (((cond_scan) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_TRIM_BYTE_OFFSET (8)
#define CME_BT_CDL_PRIORITY_TABLE_TRIM_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[8] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_TRIM_SET(cme_bt_cdl_priority_table_ptr, trim) ((cme_bt_cdl_priority_table_ptr)->_data[8] =  \
                                                                                     (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[8] & ~0xf) | (((trim)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_NONCONNECTABLE_ADVERT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[8] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_NONCONNECTABLE_ADVERT_SET(cme_bt_cdl_priority_table_ptr, ble_nonconnectable_advert) ((cme_bt_cdl_priority_table_ptr)->_data[8] =  \
                                                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[8] & ~0xf0) | (((ble_nonconnectable_advert) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DISCOVERABLE_ADVERT_BYTE_OFFSET (9)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DISCOVERABLE_ADVERT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[9] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DISCOVERABLE_ADVERT_SET(cme_bt_cdl_priority_table_ptr, ble_discoverable_advert) ((cme_bt_cdl_priority_table_ptr)->_data[9] =  \
                                                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[9] & ~0xf) | (((ble_discoverable_advert)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_UNDIRECTED_ADVERT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[9] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_UNDIRECTED_ADVERT_SET(cme_bt_cdl_priority_table_ptr, ble_undirected_advert) ((cme_bt_cdl_priority_table_ptr)->_data[9] =  \
                                                                                                                       (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[9] & ~0xf0) | (((ble_undirected_advert) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DIRECTED_ADVERT_BYTE_OFFSET (10)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DIRECTED_ADVERT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[10] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_DIRECTED_ADVERT_SET(cme_bt_cdl_priority_table_ptr, ble_directed_advert) ((cme_bt_cdl_priority_table_ptr)->_data[10] =  \
                                                                                                                   (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[10] & ~0xf) | (((ble_directed_advert)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ADVERT_SCAN_RESPONSE_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[10] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ADVERT_SCAN_RESPONSE_SET(cme_bt_cdl_priority_table_ptr, ble_advert_scan_response) ((cme_bt_cdl_priority_table_ptr)->_data[10] =  \
                                                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[10] & ~0xf0) | (((ble_advert_scan_response) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_PASSIVE_SCAN_BYTE_OFFSET (11)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_PASSIVE_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[11] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_PASSIVE_SCAN_SET(cme_bt_cdl_priority_table_ptr, ble_passive_scan) ((cme_bt_cdl_priority_table_ptr)->_data[11] =  \
                                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[11] & ~0xf) | (((ble_passive_scan)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ACTIVE_SCAN_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[11] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ACTIVE_SCAN_SET(cme_bt_cdl_priority_table_ptr, ble_active_scan) ((cme_bt_cdl_priority_table_ptr)->_data[11] =  \
                                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[11] & ~0xf0) | (((ble_active_scan) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ACTIVE_SCAN_RESPONSE_BYTE_OFFSET (12)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ACTIVE_SCAN_RESPONSE_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[12] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_ACTIVE_SCAN_RESPONSE_SET(cme_bt_cdl_priority_table_ptr, ble_active_scan_response) ((cme_bt_cdl_priority_table_ptr)->_data[12] =  \
                                                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[12] & ~0xf) | (((ble_active_scan_response)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_INITIATOR_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[12] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_INITIATOR_SET(cme_bt_cdl_priority_table_ptr, ble_initiator) ((cme_bt_cdl_priority_table_ptr)->_data[12] =  \
                                                                                                       (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[12] & ~0xf0) | (((ble_initiator) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ESTABLISHMENT_BYTE_OFFSET (13)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ESTABLISHMENT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[13] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ESTABLISHMENT_SET(cme_bt_cdl_priority_table_ptr, ble_master_establishment) ((cme_bt_cdl_priority_table_ptr)->_data[13] =  \
                                                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[13] & ~0xf) | (((ble_master_establishment)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_ESTABLISHMENT_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[13] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_ESTABLISHMENT_SET(cme_bt_cdl_priority_table_ptr, ble_slave_establishment) ((cme_bt_cdl_priority_table_ptr)->_data[13] =  \
                                                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[13] & ~0xf0) | (((ble_slave_establishment) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ANCHOR_BYTE_OFFSET (14)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ANCHOR_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[14] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_ANCHOR_SET(cme_bt_cdl_priority_table_ptr, ble_master_anchor) ((cme_bt_cdl_priority_table_ptr)->_data[14] =  \
                                                                                                               (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[14] & ~0xf) | (((ble_master_anchor)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_ANCHOR_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[14] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_ANCHOR_SET(cme_bt_cdl_priority_table_ptr, ble_slave_anchor) ((cme_bt_cdl_priority_table_ptr)->_data[14] =  \
                                                                                                             (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[14] & ~0xf0) | (((ble_slave_anchor) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_DATA_BYTE_OFFSET (15)
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_DATA_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[15] & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_MASTER_DATA_SET(cme_bt_cdl_priority_table_ptr, ble_master_data) ((cme_bt_cdl_priority_table_ptr)->_data[15] =  \
                                                                                                           (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[15] & ~0xf) | (((ble_master_data)) & 0xf)))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_DATA_GET(cme_bt_cdl_priority_table_ptr) ((CsrCmeCdlPriority)(((cme_bt_cdl_priority_table_ptr)->_data[15] & 0xf0) >> 4))
#define CME_BT_CDL_PRIORITY_TABLE_BLE_SLAVE_DATA_SET(cme_bt_cdl_priority_table_ptr, ble_slave_data) ((cme_bt_cdl_priority_table_ptr)->_data[15] =  \
                                                                                                         (CsrUint8)(((cme_bt_cdl_priority_table_ptr)->_data[15] & ~0xf0) | (((ble_slave_data) << 4) & 0xf0)))
#define CME_BT_CDL_PRIORITY_TABLE_BYTE_SIZE (16)
/*lint -e(773) allow unparenthesized*/
#define CME_BT_CDL_PRIORITY_TABLE_CREATE(page, page_scan, inquiry, inquiry_scan, role_switch, lmp_to_master, lmp_from_master, esco, esco_retrans, polling, sniff, bulk_acl, broadcast, park, band_scan, cond_scan, trim, ble_nonconnectable_advert, ble_discoverable_advert, ble_undirected_advert, ble_directed_advert, ble_advert_scan_response, ble_passive_scan, ble_active_scan, ble_active_scan_response, ble_initiator, ble_master_establishment, ble_slave_establishment, ble_master_anchor, ble_slave_anchor, ble_master_data, ble_slave_data) \
    (CsrUint8)(((page)) & 0xf) | \
    (CsrUint8)(((page_scan) << 4) & 0xf0), \
    (CsrUint8)(((inquiry)) & 0xf) | \
    (CsrUint8)(((inquiry_scan) << 4) & 0xf0), \
    (CsrUint8)(((role_switch)) & 0xf) | \
    (CsrUint8)(((lmp_to_master) << 4) & 0xf0), \
    (CsrUint8)(((lmp_from_master)) & 0xf) | \
    (CsrUint8)(((esco) << 4) & 0xf0), \
    (CsrUint8)(((esco_retrans)) & 0xf) | \
    (CsrUint8)(((polling) << 4) & 0xf0), \
    (CsrUint8)(((sniff)) & 0xf) | \
    (CsrUint8)(((bulk_acl) << 4) & 0xf0), \
    (CsrUint8)(((broadcast)) & 0xf) | \
    (CsrUint8)(((park) << 4) & 0xf0), \
    (CsrUint8)(((band_scan)) & 0xf) | \
    (CsrUint8)(((cond_scan) << 4) & 0xf0), \
    (CsrUint8)(((trim)) & 0xf) | \
    (CsrUint8)(((ble_nonconnectable_advert) << 4) & 0xf0), \
    (CsrUint8)(((ble_discoverable_advert)) & 0xf) | \
    (CsrUint8)(((ble_undirected_advert) << 4) & 0xf0), \
    (CsrUint8)(((ble_directed_advert)) & 0xf) | \
    (CsrUint8)(((ble_advert_scan_response) << 4) & 0xf0), \
    (CsrUint8)(((ble_passive_scan)) & 0xf) | \
    (CsrUint8)(((ble_active_scan) << 4) & 0xf0), \
    (CsrUint8)(((ble_active_scan_response)) & 0xf) | \
    (CsrUint8)(((ble_initiator) << 4) & 0xf0), \
    (CsrUint8)(((ble_master_establishment)) & 0xf) | \
    (CsrUint8)(((ble_slave_establishment) << 4) & 0xf0), \
    (CsrUint8)(((ble_master_anchor)) & 0xf) | \
    (CsrUint8)(((ble_slave_anchor) << 4) & 0xf0), \
    (CsrUint8)(((ble_master_data)) & 0xf) | \
    (CsrUint8)(((ble_slave_data) << 4) & 0xf0)
#define CME_BT_CDL_PRIORITY_TABLE_PACK(cme_bt_cdl_priority_table_ptr, page, page_scan, inquiry, inquiry_scan, role_switch, lmp_to_master, lmp_from_master, esco, esco_retrans, polling, sniff, bulk_acl, broadcast, park, band_scan, cond_scan, trim, ble_nonconnectable_advert, ble_discoverable_advert, ble_undirected_advert, ble_directed_advert, ble_advert_scan_response, ble_passive_scan, ble_active_scan, ble_active_scan_response, ble_initiator, ble_master_establishment, ble_slave_establishment, ble_master_anchor, ble_slave_anchor, ble_master_data, ble_slave_data) \
    do { \
        (cme_bt_cdl_priority_table_ptr)->_data[0] = (CsrUint8)(((page)) & 0xf) | \
                                                    (CsrUint8)(((page_scan) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[1] = (CsrUint8)(((inquiry)) & 0xf) | \
                                                    (CsrUint8)(((inquiry_scan) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[2] = (CsrUint8)(((role_switch)) & 0xf) | \
                                                    (CsrUint8)(((lmp_to_master) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[3] = (CsrUint8)(((lmp_from_master)) & 0xf) | \
                                                    (CsrUint8)(((esco) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[4] = (CsrUint8)(((esco_retrans)) & 0xf) | \
                                                    (CsrUint8)(((polling) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[5] = (CsrUint8)(((sniff)) & 0xf) | \
                                                    (CsrUint8)(((bulk_acl) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[6] = (CsrUint8)(((broadcast)) & 0xf) | \
                                                    (CsrUint8)(((park) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[7] = (CsrUint8)(((band_scan)) & 0xf) | \
                                                    (CsrUint8)(((cond_scan) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[8] = (CsrUint8)(((trim)) & 0xf) | \
                                                    (CsrUint8)(((ble_nonconnectable_advert) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[9] = (CsrUint8)(((ble_discoverable_advert)) & 0xf) | \
                                                    (CsrUint8)(((ble_undirected_advert) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[10] = (CsrUint8)(((ble_directed_advert)) & 0xf) | \
                                                     (CsrUint8)(((ble_advert_scan_response) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[11] = (CsrUint8)(((ble_passive_scan)) & 0xf) | \
                                                     (CsrUint8)(((ble_active_scan) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[12] = (CsrUint8)(((ble_active_scan_response)) & 0xf) | \
                                                     (CsrUint8)(((ble_initiator) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[13] = (CsrUint8)(((ble_master_establishment)) & 0xf) | \
                                                     (CsrUint8)(((ble_slave_establishment) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[14] = (CsrUint8)(((ble_master_anchor)) & 0xf) | \
                                                     (CsrUint8)(((ble_slave_anchor) << 4) & 0xf0); \
        (cme_bt_cdl_priority_table_ptr)->_data[15] = (CsrUint8)(((ble_master_data)) & 0xf) | \
                                                     (CsrUint8)(((ble_slave_data) << 4) & 0xf0); \
    } while (0)


#define CME_BT_CDL_PRIORITY_TABLE_INDEXED_GET(cme_bt_cdl_priority_table_ptr, index) ((CME_CDL_PRIORITY)(((cme_bt_cdl_priority_table_ptr)->_data[index >> 2] >> ((index & 3) << 2)) & 0x000f))

#define CME_BT_CDL_PRIORITY_TABLE_INDEXED_SET(cme_bt_cdl_priority_table_ptr, index, value) ((cme_bt_cdl_priority_table_ptr)->_data[index >> 2] = (uint16)(((cme_bt_cdl_priority_table_ptr)->_data[index >> 2] & ~(0xf << ((index & 3) << 2))) | ((((uint16)(value)) & 0x0f) << ((index & 3) << 2))))


/*******************************************************************************

  NAME
    CME_BT_Shared_Data

  DESCRIPTION
    Shared data structure in BT memory which WLAN can read/write via remote
    buffer mechanism.

  MEMBERS
    bt_defer_scheduling_restriction   - Defines a temporary restriction applied
                                        to the LC scheduling algorithm to
                                        facilitate WLAN operation.
    bt_defer_scheduling_tear_counter  - The counter is incremented when updates
                                        to the scheduling start/stop times are
                                        started and incremented again when
                                        updates are completed. Thus whenever the
                                        times are not being updated bit 0 of the
                                        counter is 0. The counter should be read
                                        before and after reading the times to
                                        check they have not been updated while
                                        being read.
    bt_defer_scheduling_stop_bt_time  - No restricted activities shall be
                                        scheduled between
                                        bt_defer_scheduling_stop_bt_time and
                                        bt_defer_scheduling_start_bt_time.
                                        Specified as Microsecond time i.e. units
                                        of 1us, range up to 2047s in the future
    bt_defer_scheduling_start_bt_time - No restricted activities shall be
                                        scheduled between
                                        bt_defer_scheduling_stop_bt_time and
                                        bt_defer_scheduling_start_bt_time.
                                        Specified as Microsecond time i.e. units
                                        of 1us, range up to 2047s in the future
    bt_cdl_priority_table_index       - Index of active CDL priority table
    bt_cdl_priority_table_0           - 1st CDL priority table
    bt_cdl_priority_table_1           - 2nd CDL priority table

*******************************************************************************/
typedef struct
{
    CsrCmeBtSchedulerRestriction bt_defer_scheduling_restriction;
    CsrUint16                    bt_defer_scheduling_tear_counter;
    CsrUint32                    bt_defer_scheduling_stop_bt_time;
    CsrUint32                    bt_defer_scheduling_start_bt_time;
    CsrUint16                    bt_cdl_priority_table_index;
    CsrCmeBtCdlPriorityTable     bt_cdl_priority_table_0;
    CsrCmeBtCdlPriorityTable     bt_cdl_priority_table_1;
} CsrCmeBtSharedData;

/* The following macros take CME_BT_SHARED_DATA *cme_bt_shared_data_ptr or CsrUint8 *addr */
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_RESTRICTION_BYTE_OFFSET (0)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_RESTRICTION_GET(addr)  \
    ((CsrCmeBtSchedulerRestriction)((CsrUint16)(*(((const CsrUint8 *)(addr)))) | \
                                    ((CsrUint16)(*(((const CsrUint8 *)(addr)) + 1)) << 8)))
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_RESTRICTION_SET(addr, bt_defer_scheduling_restriction) do { \
        *(((CsrUint8 *)(addr))) = (CsrUint8)((bt_defer_scheduling_restriction) & 0xff); \
        *(((CsrUint8 *)(addr)) + 1) = (CsrUint8)((bt_defer_scheduling_restriction) >> 8); } while (0)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_TEAR_COUNTER_BYTE_OFFSET (2)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_TEAR_COUNTER_GET(addr)  \
    (((CsrUint16)(*(((const CsrUint8 *)(addr)) + 2)) | \
      ((CsrUint16)(*(((const CsrUint8 *)(addr)) + 3)) << 8)))
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_TEAR_COUNTER_SET(addr, bt_defer_scheduling_tear_counter) do { \
        *(((CsrUint8 *)(addr)) + 2) = (CsrUint8)((bt_defer_scheduling_tear_counter) & 0xff); \
        *(((CsrUint8 *)(addr)) + 3) = (CsrUint8)((bt_defer_scheduling_tear_counter) >> 8); } while (0)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_STOP_BT_TIME_BYTE_OFFSET (4)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_STOP_BT_TIME_GET(addr)  \
    (((CsrUint32)(*(((const CsrUint8 *)(addr)) + 4)) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 5)) << 8) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 4 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 4 + 3)) << (3 * 8))))
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_STOP_BT_TIME_SET(addr, bt_defer_scheduling_stop_bt_time) do { \
        *(((CsrUint8 *)(addr)) + 4) = (CsrUint8)((bt_defer_scheduling_stop_bt_time) & 0xff); \
        *(((CsrUint8 *)(addr)) + 5) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> 8) & 0xff); \
        *(((CsrUint8 *)(addr)) + 4 + 2) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> (2 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 4 + 3) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> (3 * 8)) & 0xff); } while (0)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_START_BT_TIME_BYTE_OFFSET (8)
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_START_BT_TIME_GET(addr)  \
    (((CsrUint32)(*(((const CsrUint8 *)(addr)) + 8)) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 8 + 1)) << 8) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 8 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*(((const CsrUint8 *)(addr)) + 8 + 3)) << (3 * 8))))
#define CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_START_BT_TIME_SET(addr, bt_defer_scheduling_start_bt_time) do { \
        *(((CsrUint8 *)(addr)) + 8) = (CsrUint8)((bt_defer_scheduling_start_bt_time) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 1) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> 8) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 2) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> (2 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 3) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> (3 * 8)) & 0xff); } while (0)
#define CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_INDEX_BYTE_OFFSET (12)
#define CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_INDEX_GET(addr)  \
    (((CsrUint16)(*(((const CsrUint8 *)(addr)) + 12)) | \
      ((CsrUint16)(*(((const CsrUint8 *)(addr)) + 13)) << 8)))
#define CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_INDEX_SET(addr, bt_cdl_priority_table_index) do { \
        *(((CsrUint8 *)(addr)) + 12) = (CsrUint8)((bt_cdl_priority_table_index) & 0xff); \
        *(((CsrUint8 *)(addr)) + 13) = (CsrUint8)((bt_cdl_priority_table_index) >> 8); } while (0)
#define CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_0_BYTE_OFFSET (14)
#define CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_1_BYTE_OFFSET (30)
#define CME_BT_SHARED_DATA_BYTE_SIZE (46)
/*lint -e(773) allow unparenthesized*/
#define CME_BT_SHARED_DATA_CREATE(bt_defer_scheduling_restriction, bt_defer_scheduling_tear_counter, bt_defer_scheduling_stop_bt_time, bt_defer_scheduling_start_bt_time, bt_cdl_priority_table_index) \
    (CsrUint8)((bt_defer_scheduling_restriction) & 0xff), \
    (CsrUint8)((bt_defer_scheduling_restriction) >> 8), \
    (CsrUint8)((bt_defer_scheduling_tear_counter) & 0xff), \
    (CsrUint8)((bt_defer_scheduling_tear_counter) >> 8), \
    (CsrUint8)((bt_defer_scheduling_stop_bt_time) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> 8) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> 2 * 8) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> 3 * 8) & 0xff), \
    (CsrUint8)((bt_defer_scheduling_start_bt_time) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> 8) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> 2 * 8) & 0xff), \
    (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> 3 * 8) & 0xff), \
    (CsrUint8)((bt_cdl_priority_table_index) & 0xff), \
    (CsrUint8)((bt_cdl_priority_table_index) >> 8)
#define CME_BT_SHARED_DATA_PACK(addr, bt_defer_scheduling_restriction, bt_defer_scheduling_tear_counter, bt_defer_scheduling_stop_bt_time, bt_defer_scheduling_start_bt_time, bt_cdl_priority_table_index) \
    do { \
        *(((CsrUint8 *)(addr))) = (CsrUint8)((bt_defer_scheduling_restriction) & 0xff); \
        *(((CsrUint8 *)(addr)) + 1) = (CsrUint8)((bt_defer_scheduling_restriction) >> 8); \
        *(((CsrUint8 *)(addr)) + 2) = (CsrUint8)((bt_defer_scheduling_tear_counter) & 0xff); \
        *(((CsrUint8 *)(addr)) + 3) = (CsrUint8)((bt_defer_scheduling_tear_counter) >> 8); \
        *(((CsrUint8 *)(addr)) + 4) = (CsrUint8)((bt_defer_scheduling_stop_bt_time) & 0xff); \
        *(((CsrUint8 *)(addr)) + 5) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> 8) & 0xff); \
        *(((CsrUint8 *)(addr)) + 4 + 2) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> (2 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 4 + 3) = (CsrUint8)(((bt_defer_scheduling_stop_bt_time) >> (3 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8) = (CsrUint8)((bt_defer_scheduling_start_bt_time) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 1) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> 8) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 2) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> (2 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 8 + 3) = (CsrUint8)(((bt_defer_scheduling_start_bt_time) >> (3 * 8)) & 0xff); \
        *(((CsrUint8 *)(addr)) + 12) = (CsrUint8)((bt_cdl_priority_table_index) & 0xff); \
        *(((CsrUint8 *)(addr)) + 13) = (CsrUint8)((bt_cdl_priority_table_index) >> 8); \
    } while (0)

#define CME_BT_SHARED_DATA_MARSHALL(addr, cme_bt_shared_data_ptr) \
    do { \
        *((addr)) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_restriction) & 0xff); \
        *((addr) + 1) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_restriction) >> 8); \
        *((addr) + 2) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_tear_counter) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_tear_counter) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_stop_bt_time) & 0xff); \
        *((addr) + 5) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_stop_bt_time) >> 8) & 0xff); \
        *((addr) + 4 + 2) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_stop_bt_time) >> (2 * 8)) & 0xff); \
        *((addr) + 4 + 3) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_stop_bt_time) >> (3 * 8)) & 0xff); \
        *((addr) + 8) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_defer_scheduling_start_bt_time) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_start_bt_time) >> 8) & 0xff); \
        *((addr) + 8 + 2) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_start_bt_time) >> (2 * 8)) & 0xff); \
        *((addr) + 8 + 3) = (CsrUint8)((((cme_bt_shared_data_ptr)->bt_defer_scheduling_start_bt_time) >> (3 * 8)) & 0xff); \
        *((addr) + 12) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_cdl_priority_table_index) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((cme_bt_shared_data_ptr)->bt_cdl_priority_table_index) >> 8); \
    } while (0)

#define CME_BT_SHARED_DATA_UNMARSHALL(addr, cme_bt_shared_data_ptr) \
    do { \
        (cme_bt_shared_data_ptr)->bt_defer_scheduling_restriction = CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_RESTRICTION_GET(addr); \
        (cme_bt_shared_data_ptr)->bt_defer_scheduling_tear_counter = CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_TEAR_COUNTER_GET(addr); \
        (cme_bt_shared_data_ptr)->bt_defer_scheduling_stop_bt_time = CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_STOP_BT_TIME_GET(addr); \
        (cme_bt_shared_data_ptr)->bt_defer_scheduling_start_bt_time = CME_BT_SHARED_DATA_BT_DEFER_SCHEDULING_START_BT_TIME_GET(addr); \
        (cme_bt_shared_data_ptr)->bt_cdl_priority_table_index = CME_BT_SHARED_DATA_BT_CDL_PRIORITY_TABLE_INDEX_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    Cme_Header

  DESCRIPTION

  MEMBERS
    Signal_Id -
    Length    - The length of the whole message in 16-bit words.

*******************************************************************************/
typedef struct
{
    CsrCmeSignalId Signal_Id;
    CsrUint8       Length;
} CsrCmeHeader;

/* The following macros take CME_HEADER *cme_header_ptr or CsrUint8 *addr */
#define CME_HEADER_SIGNAL_ID_BYTE_OFFSET (0)
#define CME_HEADER_SIGNAL_ID_GET(addr) ((CsrCmeSignalId) * (((const CsrUint8 *)(addr))))
#define CME_HEADER_SIGNAL_ID_SET(addr, signal_id) (*(((CsrUint8 *)(addr))) = (CsrUint8)(signal_id))
#define CME_HEADER_LENGTH_BYTE_OFFSET (1)
#define CME_HEADER_LENGTH_GET(addr) (*(((const CsrUint8 *)(addr)) + 1))
#define CME_HEADER_LENGTH_SET(addr, length) (*(((CsrUint8 *)(addr)) + 1) = (CsrUint8)(length))
#define CME_HEADER_BYTE_SIZE (2)
/*lint -e(773) allow unparenthesized*/
#define CME_HEADER_CREATE(Signal_Id, Length) \
    (CsrUint8)(Signal_Id), \
    (CsrUint8)(Length)
#define CME_HEADER_PACK(addr, Signal_Id, Length) \
    do { \
        *(((CsrUint8 *)(addr))) = (CsrUint8)(Signal_Id); \
        *(((CsrUint8 *)(addr)) + 1) = (CsrUint8)(Length); \
    } while (0)

#define CME_HEADER_MARSHALL(addr, cme_header_ptr) \
    do { \
        *((addr)) = (CsrUint8)((cme_header_ptr)->Signal_Id); \
        *((addr) + 1) = (CsrUint8)((cme_header_ptr)->Length); \
    } while (0)

#define CME_HEADER_UNMARSHALL(addr, cme_header_ptr) \
    do { \
        (cme_header_ptr)->Signal_Id = CME_HEADER_SIGNAL_ID_GET(addr); \
        (cme_header_ptr)->Length = CME_HEADER_LENGTH_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_COEX_SERVICE_ACTIVE_IND

  DESCRIPTION
    Message from CME to peer CME to indicate that coex service setup complete
    and the coex service is now active.

  MEMBERS
    buffer_handle      - The fully qualified buffer handle for the buffer used
                         to access the shared memory data structure.
    shared_data_offset - An offset in octets into the buffer at which the shared
                         data structure can be found.
    shared_data_length - The length in octets of the shared data structure.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    buffer_handle;
    CsrUint16    shared_data_offset;
    CsrUint16    shared_data_length;
} CsrCmeCoexServiceActiveInd;

/* The following macros take CME_COEX_SERVICE_ACTIVE_IND *cme_coex_service_active_ind_ptr or CsrUint8 *addr */
#define CME_COEX_SERVICE_ACTIVE_IND_BUFFER_HANDLE_BYTE_OFFSET (2)
#define CME_COEX_SERVICE_ACTIVE_IND_BUFFER_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_COEX_SERVICE_ACTIVE_IND_BUFFER_HANDLE_SET(addr, buffer_handle) do { \
        *((addr) + 2) = (CsrUint8)((buffer_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((buffer_handle) >> 8); } while (0)
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_OFFSET_BYTE_OFFSET (4)
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_OFFSET_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_OFFSET_SET(addr, shared_data_offset) do { \
        *((addr) + 4) = (CsrUint8)((shared_data_offset) & 0xff); \
        *((addr) + 5) = (CsrUint8)((shared_data_offset) >> 8); } while (0)
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_LENGTH_BYTE_OFFSET (6)
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_LENGTH_GET(addr)  \
    (((CsrUint16)(*((addr) + 6)) | \
      ((CsrUint16)(*((addr) + 7)) << 8)))
#define CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_LENGTH_SET(addr, shared_data_length) do { \
        *((addr) + 6) = (CsrUint8)((shared_data_length) & 0xff); \
        *((addr) + 7) = (CsrUint8)((shared_data_length) >> 8); } while (0)
#define CME_COEX_SERVICE_ACTIVE_IND_BYTE_SIZE (8)
#define CME_COEX_SERVICE_ACTIVE_IND_PACK(addr, buffer_handle, shared_data_offset, shared_data_length) \
    do { \
        *((addr) + 2) = (CsrUint8)((buffer_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((buffer_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((shared_data_offset) & 0xff); \
        *((addr) + 5) = (CsrUint8)((shared_data_offset) >> 8); \
        *((addr) + 6) = (CsrUint8)((shared_data_length) & 0xff); \
        *((addr) + 7) = (CsrUint8)((shared_data_length) >> 8); \
    } while (0)

#define CME_COEX_SERVICE_ACTIVE_IND_MARSHALL(addr, cme_coex_service_active_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_coex_service_active_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->buffer_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->buffer_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->shared_data_offset) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->shared_data_offset) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->shared_data_length) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((cme_coex_service_active_ind_ptr)->shared_data_length) >> 8); \
    } while (0)

#define CME_COEX_SERVICE_ACTIVE_IND_UNMARSHALL(addr, cme_coex_service_active_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_coex_service_active_ind_ptr)->header)); \
        (cme_coex_service_active_ind_ptr)->buffer_handle = CME_COEX_SERVICE_ACTIVE_IND_BUFFER_HANDLE_GET(addr); \
        (cme_coex_service_active_ind_ptr)->shared_data_offset = CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_OFFSET_GET(addr); \
        (cme_coex_service_active_ind_ptr)->shared_data_length = CME_COEX_SERVICE_ACTIVE_IND_SHARED_DATA_LENGTH_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_SYNC_REQ

  DESCRIPTION
    Message from CME to peer CME to check messaging between peers is
    operational. Peer responds with CME_SYNC_CFM.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeSyncReq;

/* The following macros take CME_SYNC_REQ *cme_sync_req_ptr or CsrUint8 *addr */
#define CME_SYNC_REQ_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_SYNC_CFM

  DESCRIPTION
    Message from CME to peer CME to check messaging between peers is
    operational. Triggered by CME_SYNC_REQ.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeSyncCfm;

/* The following macros take CME_SYNC_CFM *cme_sync_cfm_ptr or CsrUint8 *addr */
#define CME_SYNC_CFM_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_REPLAY_REQ

  DESCRIPTION
    Message from CME to peer CME to request it to replay event notifications.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeReplayReq;

/* The following macros take CME_REPLAY_REQ *cme_replay_req_ptr or CsrUint8 *addr */
#define CME_REPLAY_REQ_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_REPLAY_CFM

  DESCRIPTION
    Message from CME to peer CME to indicate replay of event notifications is
    complete.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeReplayCfm;

/* The following macros take CME_REPLAY_CFM *cme_replay_cfm_ptr or CsrUint8 *addr */
#define CME_REPLAY_CFM_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_HOST_REPLAY_REQ

  DESCRIPTION
    Message from on-chip CME_WLAN to host CME to request it to replay profile
    indications.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeHostReplayReq;

/* The following macros take CME_HOST_REPLAY_REQ *cme_host_replay_req_ptr or CsrUint8 *addr */
#define CME_HOST_REPLAY_REQ_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_HOST_REPLAY_CFM

  DESCRIPTION
    Message from host CME to on-chip CME_WLAN to confirm that replay of
    profile indications is complete.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeHostReplayCfm;

/* The following macros take CME_HOST_REPLAY_CFM *cme_host_replay_cfm_ptr or CsrUint8 *addr */
#define CME_HOST_REPLAY_CFM_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_ACL_CONNECT_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that an ACL connection has
    been established between the Bluetooth device and a remote device, or
    that an existing connection has updated parameters.

  MEMBERS
    handle                   - Identifies a particular ACL Link in the firmware
                               and host. Some characteristics of the ACL link
                               (e.g. clock referencing) will apply to the
                               SCO/eSCO link.
    is_master                - Indicates if the Bluetooth device is the Master
                               (0x0001) or Slave (0x0000) of the ACL link.
    is_edr                   - Indicates if at least one Enhanced Data Rate
                               packet type is supported over the ACL link.
    spare                    - Spare.
    poll_interval            - Indicates the maximum time in Bluetooth Slots
                               between transmissions from the Master to the
                               Slave (Tpoll).
    link_supervision_timeout - Indicates the maximum time in Bluetooth Slots the
                               Master will wait for communication from the Slave
                               before considering that the connection has been
                               dropped.
    wallclock_buffer_handle  - The fully qualified buffer handle for the buffer
                               used to read the wallclock.
    wallclock_offset         - An offset in octets into the buffer at which the
                               wallclock shared memory structure can be found.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
    CsrUint8     is_master; /* Only 1 bits used */
    CsrUint8     is_edr;    /* Only 1 bits used */
    CsrUint8     poll_interval;
    CsrUint16    link_supervision_timeout;
    CsrUint16    wallclock_buffer_handle;
    CsrUint16    wallclock_offset;
} CsrCmeAclConnectInd;

/* The following macros take CME_ACL_CONNECT_IND *cme_acl_connect_ind_ptr or CsrUint8 *addr */
#define CME_ACL_CONNECT_IND_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_CONNECT_IND_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_CONNECT_IND_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_ACL_CONNECT_IND_IS_MASTER_BYTE_OFFSET (4)
#define CME_ACL_CONNECT_IND_IS_MASTER_GET(addr) (((*((addr) + 4) & 0x1)))
#define CME_ACL_CONNECT_IND_IS_MASTER_SET(addr, is_master) (*((addr) + 4) =  \
                                                                (CsrUint8)((*((addr) + 4) & ~0x1) | (((is_master)) & 0x1)))
#define CME_ACL_CONNECT_IND_IS_EDR_GET(addr) (((*((addr) + 4) & 0x2) >> 1))
#define CME_ACL_CONNECT_IND_IS_EDR_SET(addr, is_edr) (*((addr) + 4) =  \
                                                          (CsrUint8)((*((addr) + 4) & ~0x2) | (((is_edr) << 1) & 0x2)))
#define CME_ACL_CONNECT_IND_POLL_INTERVAL_BYTE_OFFSET (5)
#define CME_ACL_CONNECT_IND_POLL_INTERVAL_GET(addr) (*((addr) + 5))
#define CME_ACL_CONNECT_IND_POLL_INTERVAL_SET(addr, poll_interval) (*((addr) + 5) = (CsrUint8)(poll_interval))
#define CME_ACL_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_BYTE_OFFSET (6)
#define CME_ACL_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_GET(addr)  \
    (((CsrUint16)(*((addr) + 6)) | \
      ((CsrUint16)(*((addr) + 7)) << 8)))
#define CME_ACL_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_SET(addr, link_supervision_timeout) do { \
        *((addr) + 6) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)((link_supervision_timeout) >> 8); } while (0)
#define CME_ACL_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_BYTE_OFFSET (8)
#define CME_ACL_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 8)) | \
      ((CsrUint16)(*((addr) + 8 + 1)) << 8)))
#define CME_ACL_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_SET(addr, wallclock_buffer_handle) do { \
        *((addr) + 8) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((wallclock_buffer_handle) >> 8); } while (0)
#define CME_ACL_CONNECT_IND_WALLCLOCK_OFFSET_BYTE_OFFSET (10)
#define CME_ACL_CONNECT_IND_WALLCLOCK_OFFSET_GET(addr)  \
    (((CsrUint16)(*((addr) + 10)) | \
      ((CsrUint16)(*((addr) + 11)) << 8)))
#define CME_ACL_CONNECT_IND_WALLCLOCK_OFFSET_SET(addr, wallclock_offset) do { \
        *((addr) + 10) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)((wallclock_offset) >> 8); } while (0)
#define CME_ACL_CONNECT_IND_BYTE_SIZE (12)
#define CME_ACL_CONNECT_IND_PACK(addr, handle, is_master, is_edr, poll_interval, link_supervision_timeout, wallclock_buffer_handle, wallclock_offset) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((is_master)) & 0x1) | \
                        (CsrUint8)(((is_edr) << 1) & 0x2); \
        *((addr) + 5) = (CsrUint8)(poll_interval); \
        *((addr) + 6) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)((link_supervision_timeout) >> 8); \
        *((addr) + 8) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((wallclock_buffer_handle) >> 8); \
        *((addr) + 10) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)((wallclock_offset) >> 8); \
    } while (0)

#define CME_ACL_CONNECT_IND_MARSHALL(addr, cme_acl_connect_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_connect_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_connect_ind_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_connect_ind_ptr)->handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((((cme_acl_connect_ind_ptr)->is_master)) & 0x1) | \
                        (CsrUint8)((((cme_acl_connect_ind_ptr)->is_edr) << 1) & 0x2); \
        *((addr) + 5) = (CsrUint8)((cme_acl_connect_ind_ptr)->poll_interval); \
        *((addr) + 6) = (CsrUint8)(((cme_acl_connect_ind_ptr)->link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((cme_acl_connect_ind_ptr)->link_supervision_timeout) >> 8); \
        *((addr) + 8) = (CsrUint8)(((cme_acl_connect_ind_ptr)->wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)(((cme_acl_connect_ind_ptr)->wallclock_buffer_handle) >> 8); \
        *((addr) + 10) = (CsrUint8)(((cme_acl_connect_ind_ptr)->wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((cme_acl_connect_ind_ptr)->wallclock_offset) >> 8); \
    } while (0)

#define CME_ACL_CONNECT_IND_UNMARSHALL(addr, cme_acl_connect_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_connect_ind_ptr)->header)); \
        (cme_acl_connect_ind_ptr)->handle = CME_ACL_CONNECT_IND_HANDLE_GET(addr); \
        (cme_acl_connect_ind_ptr)->is_master = CME_ACL_CONNECT_IND_IS_MASTER_GET(addr); \
        (cme_acl_connect_ind_ptr)->is_edr = CME_ACL_CONNECT_IND_IS_EDR_GET(addr); \
        (cme_acl_connect_ind_ptr)->poll_interval = CME_ACL_CONNECT_IND_POLL_INTERVAL_GET(addr); \
        (cme_acl_connect_ind_ptr)->link_supervision_timeout = CME_ACL_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_GET(addr); \
        (cme_acl_connect_ind_ptr)->wallclock_buffer_handle = CME_ACL_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_GET(addr); \
        (cme_acl_connect_ind_ptr)->wallclock_offset = CME_ACL_CONNECT_IND_WALLCLOCK_OFFSET_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_DISCONNECT_REQ

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that an ACL connection has
    been removed. This should be acknowledged with a CME_ACL_DISCONNECT_CFM
    so the wallclock shared memory can be released.

  MEMBERS
    acl_handle   - Identifies the particular ACL Link.
    wallclock_id - Identifies the wallclock associated with the ACL link. This
                   should be echoed back in the CFM.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    wallclock_id;
} CsrCmeAclDisconnectReq;

/* The following macros take CME_ACL_DISCONNECT_REQ *cme_acl_disconnect_req_ptr or CsrUint8 *addr */
#define CME_ACL_DISCONNECT_REQ_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_DISCONNECT_REQ_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_DISCONNECT_REQ_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_DISCONNECT_REQ_WALLCLOCK_ID_BYTE_OFFSET (4)
#define CME_ACL_DISCONNECT_REQ_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_DISCONNECT_REQ_WALLCLOCK_ID_SET(addr, wallclock_id) do { \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); } while (0)
#define CME_ACL_DISCONNECT_REQ_BYTE_SIZE (6)
#define CME_ACL_DISCONNECT_REQ_PACK(addr, acl_handle, wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_DISCONNECT_REQ_MARSHALL(addr, cme_acl_disconnect_req_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_disconnect_req_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_disconnect_req_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_disconnect_req_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_disconnect_req_ptr)->wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_disconnect_req_ptr)->wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_DISCONNECT_REQ_UNMARSHALL(addr, cme_acl_disconnect_req_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_disconnect_req_ptr)->header)); \
        (cme_acl_disconnect_req_ptr)->acl_handle = CME_ACL_DISCONNECT_REQ_ACL_HANDLE_GET(addr); \
        (cme_acl_disconnect_req_ptr)->wallclock_id = CME_ACL_DISCONNECT_REQ_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_DISCONNECT_CFM

  DESCRIPTION
    Message from CME_WLAN to CME_BT to acknowledge that an ACL connection has
    been removed

  MEMBERS
    acl_handle   - Identifies the particular ACL Link.
    wallclock_id - Identifies the wallclock associated with the ACL link. This
                   should be taken from the REQ.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    wallclock_id;
} CsrCmeAclDisconnectCfm;

/* The following macros take CME_ACL_DISCONNECT_CFM *cme_acl_disconnect_cfm_ptr or CsrUint8 *addr */
#define CME_ACL_DISCONNECT_CFM_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_DISCONNECT_CFM_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_DISCONNECT_CFM_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_DISCONNECT_CFM_WALLCLOCK_ID_BYTE_OFFSET (4)
#define CME_ACL_DISCONNECT_CFM_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_DISCONNECT_CFM_WALLCLOCK_ID_SET(addr, wallclock_id) do { \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); } while (0)
#define CME_ACL_DISCONNECT_CFM_BYTE_SIZE (6)
#define CME_ACL_DISCONNECT_CFM_PACK(addr, acl_handle, wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_DISCONNECT_CFM_MARSHALL(addr, cme_acl_disconnect_cfm_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_disconnect_cfm_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_disconnect_cfm_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_disconnect_cfm_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_disconnect_cfm_ptr)->wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_disconnect_cfm_ptr)->wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_DISCONNECT_CFM_UNMARSHALL(addr, cme_acl_disconnect_cfm_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_disconnect_cfm_ptr)->header)); \
        (cme_acl_disconnect_cfm_ptr)->acl_handle = CME_ACL_DISCONNECT_CFM_ACL_HANDLE_GET(addr); \
        (cme_acl_disconnect_cfm_ptr)->wallclock_id = CME_ACL_DISCONNECT_CFM_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_ROLE_SWITCH_REQ

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that an ACL role switch has
    occurred. This should be acknowledged with a CME_ACL_ROLE_SWITCH_CFM so
    the original wallclock shared memory can be released.

  MEMBERS
    acl_handle               - Identifies the particular ACL Link.
    is_master                - Indicates if the Bluetooth device is the Master
                               (0x0001) or Slave (0x0000) of the ACL link.
    spare                    - Spare.
    poll_interval            - Indicates the maximum time in Bluetooth Slots
                               between transmissions from the Master to the
                               Slave (Tpoll).
    link_supervision_timeout - Indicates the maximum time in Bluetooth Slots the
                               Master will wait for communication from the Slave
                               before considering that the connection has been
                               dropped.
    wallclock_buffer_handle  - The fully qualified buffer handle for the buffer
                               used to read the wallclock.
    wallclock_offset         - An offset in octets into the buffer at which the
                               wallclock shared memory structure can be found.
    old_wallclock_id         - Identifies the old wallclock associated with the
                               ACL link.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint8     is_master; /* Only 1 bits used */
    CsrUint8     poll_interval;
    CsrUint16    link_supervision_timeout;
    CsrUint16    wallclock_buffer_handle;
    CsrUint16    wallclock_offset;
    CsrUint16    old_wallclock_id;
} CsrCmeAclRoleSwitchReq;

/* The following macros take CME_ACL_ROLE_SWITCH_REQ *cme_acl_role_switch_req_ptr or CsrUint8 *addr */
#define CME_ACL_ROLE_SWITCH_REQ_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_ROLE_SWITCH_REQ_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_ROLE_SWITCH_REQ_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_REQ_IS_MASTER_BYTE_OFFSET (4)
#define CME_ACL_ROLE_SWITCH_REQ_IS_MASTER_GET(addr) (((*((addr) + 4) & 0x1)))
#define CME_ACL_ROLE_SWITCH_REQ_IS_MASTER_SET(addr, is_master) (*((addr) + 4) =  \
                                                                    (CsrUint8)((*((addr) + 4) & ~0x1) | (((is_master)) & 0x1)))
#define CME_ACL_ROLE_SWITCH_REQ_POLL_INTERVAL_BYTE_OFFSET (5)
#define CME_ACL_ROLE_SWITCH_REQ_POLL_INTERVAL_GET(addr) (*((addr) + 5))
#define CME_ACL_ROLE_SWITCH_REQ_POLL_INTERVAL_SET(addr, poll_interval) (*((addr) + 5) = (CsrUint8)(poll_interval))
#define CME_ACL_ROLE_SWITCH_REQ_LINK_SUPERVISION_TIMEOUT_BYTE_OFFSET (6)
#define CME_ACL_ROLE_SWITCH_REQ_LINK_SUPERVISION_TIMEOUT_GET(addr)  \
    (((CsrUint16)(*((addr) + 6)) | \
      ((CsrUint16)(*((addr) + 7)) << 8)))
#define CME_ACL_ROLE_SWITCH_REQ_LINK_SUPERVISION_TIMEOUT_SET(addr, link_supervision_timeout) do { \
        *((addr) + 6) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)((link_supervision_timeout) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_BUFFER_HANDLE_BYTE_OFFSET (8)
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_BUFFER_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 8)) | \
      ((CsrUint16)(*((addr) + 8 + 1)) << 8)))
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_BUFFER_HANDLE_SET(addr, wallclock_buffer_handle) do { \
        *((addr) + 8) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((wallclock_buffer_handle) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_OFFSET_BYTE_OFFSET (10)
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_OFFSET_GET(addr)  \
    (((CsrUint16)(*((addr) + 10)) | \
      ((CsrUint16)(*((addr) + 11)) << 8)))
#define CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_OFFSET_SET(addr, wallclock_offset) do { \
        *((addr) + 10) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)((wallclock_offset) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_REQ_OLD_WALLCLOCK_ID_BYTE_OFFSET (12)
#define CME_ACL_ROLE_SWITCH_REQ_OLD_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 12)) | \
      ((CsrUint16)(*((addr) + 13)) << 8)))
#define CME_ACL_ROLE_SWITCH_REQ_OLD_WALLCLOCK_ID_SET(addr, old_wallclock_id) do { \
        *((addr) + 12) = (CsrUint8)((old_wallclock_id) & 0xff); \
        *((addr) + 13) = (CsrUint8)((old_wallclock_id) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_REQ_BYTE_SIZE (14)
#define CME_ACL_ROLE_SWITCH_REQ_PACK(addr, acl_handle, is_master, poll_interval, link_supervision_timeout, wallclock_buffer_handle, wallclock_offset, old_wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((is_master)) & 0x1); \
        *((addr) + 5) = (CsrUint8)(poll_interval); \
        *((addr) + 6) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)((link_supervision_timeout) >> 8); \
        *((addr) + 8) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((wallclock_buffer_handle) >> 8); \
        *((addr) + 10) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)((wallclock_offset) >> 8); \
        *((addr) + 12) = (CsrUint8)((old_wallclock_id) & 0xff); \
        *((addr) + 13) = (CsrUint8)((old_wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_ROLE_SWITCH_REQ_MARSHALL(addr, cme_acl_role_switch_req_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_role_switch_req_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((((cme_acl_role_switch_req_ptr)->is_master)) & 0x1); \
        *((addr) + 5) = (CsrUint8)((cme_acl_role_switch_req_ptr)->poll_interval); \
        *((addr) + 6) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->link_supervision_timeout) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->link_supervision_timeout) >> 8); \
        *((addr) + 8) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->wallclock_buffer_handle) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->wallclock_buffer_handle) >> 8); \
        *((addr) + 10) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->wallclock_offset) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->wallclock_offset) >> 8); \
        *((addr) + 12) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->old_wallclock_id) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((cme_acl_role_switch_req_ptr)->old_wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_ROLE_SWITCH_REQ_UNMARSHALL(addr, cme_acl_role_switch_req_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_role_switch_req_ptr)->header)); \
        (cme_acl_role_switch_req_ptr)->acl_handle = CME_ACL_ROLE_SWITCH_REQ_ACL_HANDLE_GET(addr); \
        (cme_acl_role_switch_req_ptr)->is_master = CME_ACL_ROLE_SWITCH_REQ_IS_MASTER_GET(addr); \
        (cme_acl_role_switch_req_ptr)->poll_interval = CME_ACL_ROLE_SWITCH_REQ_POLL_INTERVAL_GET(addr); \
        (cme_acl_role_switch_req_ptr)->link_supervision_timeout = CME_ACL_ROLE_SWITCH_REQ_LINK_SUPERVISION_TIMEOUT_GET(addr); \
        (cme_acl_role_switch_req_ptr)->wallclock_buffer_handle = CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_BUFFER_HANDLE_GET(addr); \
        (cme_acl_role_switch_req_ptr)->wallclock_offset = CME_ACL_ROLE_SWITCH_REQ_WALLCLOCK_OFFSET_GET(addr); \
        (cme_acl_role_switch_req_ptr)->old_wallclock_id = CME_ACL_ROLE_SWITCH_REQ_OLD_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_ROLE_SWITCH_CFM

  DESCRIPTION
    Message from CME_WLAN to CME_BT to acknowledge that an ACL role switch
    has occurred. This should be sent when a CME_ACL_ROLE_SWITCH_REQ is
    received so the original wallclock shared memory can be released.

  MEMBERS
    acl_handle       - Identifies the particular ACL Link.
    old_wallclock_id - Identifies the old wallclock associated with the ACL
                       link. This should be taken from the REQ.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    old_wallclock_id;
} CsrCmeAclRoleSwitchCfm;

/* The following macros take CME_ACL_ROLE_SWITCH_CFM *cme_acl_role_switch_cfm_ptr or CsrUint8 *addr */
#define CME_ACL_ROLE_SWITCH_CFM_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_ROLE_SWITCH_CFM_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_ROLE_SWITCH_CFM_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_CFM_OLD_WALLCLOCK_ID_BYTE_OFFSET (4)
#define CME_ACL_ROLE_SWITCH_CFM_OLD_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_ROLE_SWITCH_CFM_OLD_WALLCLOCK_ID_SET(addr, old_wallclock_id) do { \
        *((addr) + 4) = (CsrUint8)((old_wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((old_wallclock_id) >> 8); } while (0)
#define CME_ACL_ROLE_SWITCH_CFM_BYTE_SIZE (6)
#define CME_ACL_ROLE_SWITCH_CFM_PACK(addr, acl_handle, old_wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((old_wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((old_wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_ROLE_SWITCH_CFM_MARSHALL(addr, cme_acl_role_switch_cfm_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_role_switch_cfm_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_role_switch_cfm_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_role_switch_cfm_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_role_switch_cfm_ptr)->old_wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_role_switch_cfm_ptr)->old_wallclock_id) >> 8); \
    } while (0)

#define CME_ACL_ROLE_SWITCH_CFM_UNMARSHALL(addr, cme_acl_role_switch_cfm_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_role_switch_cfm_ptr)->header)); \
        (cme_acl_role_switch_cfm_ptr)->acl_handle = CME_ACL_ROLE_SWITCH_CFM_ACL_HANDLE_GET(addr); \
        (cme_acl_role_switch_cfm_ptr)->old_wallclock_id = CME_ACL_ROLE_SWITCH_CFM_OLD_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ESCO_CONNECT_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicates that a SCO/eSCO connection
    has been established between the Bluetooth device and a remote device, or
    that an existing connection has updated parameters.

  MEMBERS
    handle                - Identifies a particular SCO/eSCO Link in the
                            firmware and host. The SCO/eSCO connection inherits
                            some characteristics of the associated ACL
                            connection.
    acl_handle            - Identifies the ACL Link associated with the SCO/eSCO
                            link. Some characteristics of the ACL link (e.g.
                            clock referencing) will apply to the SCO/eSCO link.
    reserved_slot         - Indicates the alignment of the SCO/eSCO channel in
                            Bluetooth slots relative to the master clock. When
                            the message is sent, BT sets reserved_slot to the
                            Bluetooth time/2 of the start of the next reserved
                            slots (range = 0 to 2^27-1). This can be propagated
                            forward by adding multiples of interval and
                            converted to a microsecond time using the wallclock.
                            Around the time the master clock wraps, it should be
                            evident whether the reserved_slot refers to the
                            pre-wrap time or post-wrap time by examining bit 26
                            (1 => pre-wrap).
    interval              - Indicates the periodicity of the SCO/eSCO channel in
                            Bluetooth slots. This value corresponds to Tsco or
                            Tesco.
    window                - Indicates the number of reserved slots used for each
                            SCO/eSCO exchange. This parameter represents the sum
                            of both the transmission and reception slots.
    retransmission_window - Indicates the number of retransmission slots
                            allocated for each eSCO exchange. This parameter
                            represents the sum of both the transmission and
                            reception slots. It should be set to zero for SCO
                            channels.
    spare                 - Spare.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
    CsrUint16    acl_handle;
    CsrUint32    reserved_slot;
    CsrUint8     interval;
    CsrUint8     window;
    CsrUint8     retransmission_window;
} CsrCmeEscoConnectInd;

/* The following macros take CME_ESCO_CONNECT_IND *cme_esco_connect_ind_ptr or CsrUint8 *addr */
#define CME_ESCO_CONNECT_IND_HANDLE_BYTE_OFFSET (2)
#define CME_ESCO_CONNECT_IND_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ESCO_CONNECT_IND_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_ESCO_CONNECT_IND_ACL_HANDLE_BYTE_OFFSET (4)
#define CME_ESCO_CONNECT_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ESCO_CONNECT_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 4) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 5) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ESCO_CONNECT_IND_RESERVED_SLOT_BYTE_OFFSET (6)
#define CME_ESCO_CONNECT_IND_RESERVED_SLOT_GET(addr)  \
    (((CsrUint32)(*((addr) + 6)) | \
      ((CsrUint32)(*((addr) + 7)) << 8) | \
      ((CsrUint32)(*((addr) + 6 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 6 + 3)) << (3 * 8))))
#define CME_ESCO_CONNECT_IND_RESERVED_SLOT_SET(addr, reserved_slot) do { \
        *((addr) + 6) = (CsrUint8)((reserved_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((reserved_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((reserved_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((reserved_slot) >> (3 * 8)) & 0xff); } while (0)
#define CME_ESCO_CONNECT_IND_INTERVAL_BYTE_OFFSET (10)
#define CME_ESCO_CONNECT_IND_INTERVAL_GET(addr) (*((addr) + 10))
#define CME_ESCO_CONNECT_IND_INTERVAL_SET(addr, interval) (*((addr) + 10) = (CsrUint8)(interval))
#define CME_ESCO_CONNECT_IND_WINDOW_BYTE_OFFSET (11)
#define CME_ESCO_CONNECT_IND_WINDOW_GET(addr) (*((addr) + 11))
#define CME_ESCO_CONNECT_IND_WINDOW_SET(addr, window) (*((addr) + 11) = (CsrUint8)(window))
#define CME_ESCO_CONNECT_IND_RETRANSMISSION_WINDOW_BYTE_OFFSET (12)
#define CME_ESCO_CONNECT_IND_RETRANSMISSION_WINDOW_GET(addr) (*((addr) + 12))
#define CME_ESCO_CONNECT_IND_RETRANSMISSION_WINDOW_SET(addr, retransmission_window) (*((addr) + 12) = (CsrUint8)(retransmission_window))
#define CME_ESCO_CONNECT_IND_BYTE_SIZE (14)
#define CME_ESCO_CONNECT_IND_PACK(addr, handle, acl_handle, reserved_slot, interval, window, retransmission_window) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 5) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 6) = (CsrUint8)((reserved_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((reserved_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((reserved_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((reserved_slot) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)(interval); \
        *((addr) + 11) = (CsrUint8)(window); \
        *((addr) + 12) = (CsrUint8)(retransmission_window); \
        *((addr) + 13) = 0; \
    } while (0)

#define CME_ESCO_CONNECT_IND_MARSHALL(addr, cme_esco_connect_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_esco_connect_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_esco_connect_ind_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_esco_connect_ind_ptr)->handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_esco_connect_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_esco_connect_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_esco_connect_ind_ptr)->reserved_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)((((cme_esco_connect_ind_ptr)->reserved_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)((((cme_esco_connect_ind_ptr)->reserved_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)((((cme_esco_connect_ind_ptr)->reserved_slot) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)((cme_esco_connect_ind_ptr)->interval); \
        *((addr) + 11) = (CsrUint8)((cme_esco_connect_ind_ptr)->window); \
        *((addr) + 12) = (CsrUint8)((cme_esco_connect_ind_ptr)->retransmission_window); \
        *((addr) + 13) = 0; \
    } while (0)

#define CME_ESCO_CONNECT_IND_UNMARSHALL(addr, cme_esco_connect_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_esco_connect_ind_ptr)->header)); \
        (cme_esco_connect_ind_ptr)->handle = CME_ESCO_CONNECT_IND_HANDLE_GET(addr); \
        (cme_esco_connect_ind_ptr)->acl_handle = CME_ESCO_CONNECT_IND_ACL_HANDLE_GET(addr); \
        (cme_esco_connect_ind_ptr)->reserved_slot = CME_ESCO_CONNECT_IND_RESERVED_SLOT_GET(addr); \
        (cme_esco_connect_ind_ptr)->interval = CME_ESCO_CONNECT_IND_INTERVAL_GET(addr); \
        (cme_esco_connect_ind_ptr)->window = CME_ESCO_CONNECT_IND_WINDOW_GET(addr); \
        (cme_esco_connect_ind_ptr)->retransmission_window = CME_ESCO_CONNECT_IND_RETRANSMISSION_WINDOW_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ESCO_DISCONNECT_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that a SCO/eSCO connection
    has been removed

  MEMBERS
    sco_handle - Identifies the particular SCO/eSCO Link.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    sco_handle;
} CsrCmeEscoDisconnectInd;

/* The following macros take CME_ESCO_DISCONNECT_IND *cme_esco_disconnect_ind_ptr or CsrUint8 *addr */
#define CME_ESCO_DISCONNECT_IND_SCO_HANDLE_BYTE_OFFSET (2)
#define CME_ESCO_DISCONNECT_IND_SCO_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ESCO_DISCONNECT_IND_SCO_HANDLE_SET(addr, sco_handle) do { \
        *((addr) + 2) = (CsrUint8)((sco_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((sco_handle) >> 8); } while (0)
#define CME_ESCO_DISCONNECT_IND_BYTE_SIZE (4)
#define CME_ESCO_DISCONNECT_IND_PACK(addr, sco_handle) \
    do { \
        *((addr) + 2) = (CsrUint8)((sco_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((sco_handle) >> 8); \
    } while (0)

#define CME_ESCO_DISCONNECT_IND_MARSHALL(addr, cme_esco_disconnect_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_esco_disconnect_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_esco_disconnect_ind_ptr)->sco_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_esco_disconnect_ind_ptr)->sco_handle) >> 8); \
    } while (0)

#define CME_ESCO_DISCONNECT_IND_UNMARSHALL(addr, cme_esco_disconnect_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_esco_disconnect_ind_ptr)->header)); \
        (cme_esco_disconnect_ind_ptr)->sco_handle = CME_ESCO_DISCONNECT_IND_SCO_HANDLE_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_SNIFF_MODE_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that corresponding ACL link
    has gone into Sniff Mode, or that an existing connection has updated
    parameters.

  MEMBERS
    acl_handle  - Identifies the ACL Link.
    interval    - Indicates the periodicity of the Sniff anchor points in
                  Bluetooth slots. This value corresponds to Tsniff.
    anchor_slot - Indicates the alignment of the Sniff anchor points in
                  Bluetooth slots relative to the master clock. When the message
                  is sent, BT sets reserved_slot to the Bluetooth time/2 of the
                  start of the next anchor point (range = 0 to 2^27-1). This can
                  be propagated forward by adding multiples of interval and
                  converted to a microsecond time using the wallclock. Around
                  the time the master clock wraps, it should be evident whether
                  the anchor_slot refers to the pre-wrap time or post-wrap time
                  by examining bit 26 (1 => pre-wrap).
    window      - Indicates the number of Bluetooth slots used for sniff
                  activities.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    interval;
    CsrUint32    anchor_slot;
    CsrUint16    window;
} CsrCmeSniffModeInd;

/* The following macros take CME_SNIFF_MODE_IND *cme_sniff_mode_ind_ptr or CsrUint8 *addr */
#define CME_SNIFF_MODE_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_SNIFF_MODE_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_SNIFF_MODE_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_SNIFF_MODE_IND_INTERVAL_BYTE_OFFSET (4)
#define CME_SNIFF_MODE_IND_INTERVAL_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_SNIFF_MODE_IND_INTERVAL_SET(addr, interval) do { \
        *((addr) + 4) = (CsrUint8)((interval) & 0xff); \
        *((addr) + 5) = (CsrUint8)((interval) >> 8); } while (0)
#define CME_SNIFF_MODE_IND_ANCHOR_SLOT_BYTE_OFFSET (6)
#define CME_SNIFF_MODE_IND_ANCHOR_SLOT_GET(addr)  \
    (((CsrUint32)(*((addr) + 6)) | \
      ((CsrUint32)(*((addr) + 7)) << 8) | \
      ((CsrUint32)(*((addr) + 6 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 6 + 3)) << (3 * 8))))
#define CME_SNIFF_MODE_IND_ANCHOR_SLOT_SET(addr, anchor_slot) do { \
        *((addr) + 6) = (CsrUint8)((anchor_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((anchor_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((anchor_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((anchor_slot) >> (3 * 8)) & 0xff); } while (0)
#define CME_SNIFF_MODE_IND_WINDOW_BYTE_OFFSET (10)
#define CME_SNIFF_MODE_IND_WINDOW_GET(addr)  \
    (((CsrUint16)(*((addr) + 10)) | \
      ((CsrUint16)(*((addr) + 11)) << 8)))
#define CME_SNIFF_MODE_IND_WINDOW_SET(addr, window) do { \
        *((addr) + 10) = (CsrUint8)((window) & 0xff); \
        *((addr) + 11) = (CsrUint8)((window) >> 8); } while (0)
#define CME_SNIFF_MODE_IND_BYTE_SIZE (12)
#define CME_SNIFF_MODE_IND_PACK(addr, acl_handle, interval, anchor_slot, window) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((interval) & 0xff); \
        *((addr) + 5) = (CsrUint8)((interval) >> 8); \
        *((addr) + 6) = (CsrUint8)((anchor_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((anchor_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((anchor_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((anchor_slot) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)((window) & 0xff); \
        *((addr) + 11) = (CsrUint8)((window) >> 8); \
    } while (0)

#define CME_SNIFF_MODE_IND_MARSHALL(addr, cme_sniff_mode_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_sniff_mode_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->interval) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->interval) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->anchor_slot) & 0xff); \
        *((addr) + 7) = (CsrUint8)((((cme_sniff_mode_ind_ptr)->anchor_slot) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)((((cme_sniff_mode_ind_ptr)->anchor_slot) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)((((cme_sniff_mode_ind_ptr)->anchor_slot) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->window) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((cme_sniff_mode_ind_ptr)->window) >> 8); \
    } while (0)

#define CME_SNIFF_MODE_IND_UNMARSHALL(addr, cme_sniff_mode_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_sniff_mode_ind_ptr)->header)); \
        (cme_sniff_mode_ind_ptr)->acl_handle = CME_SNIFF_MODE_IND_ACL_HANDLE_GET(addr); \
        (cme_sniff_mode_ind_ptr)->interval = CME_SNIFF_MODE_IND_INTERVAL_GET(addr); \
        (cme_sniff_mode_ind_ptr)->anchor_slot = CME_SNIFF_MODE_IND_ANCHOR_SLOT_GET(addr); \
        (cme_sniff_mode_ind_ptr)->window = CME_SNIFF_MODE_IND_WINDOW_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_SNIFF_MODE_OFF_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that corresponding ACL link
    has exited Sniff Mode.

  MEMBERS
    acl_handle - Identifies the ACL Link.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
} CsrCmeSniffModeOffInd;

/* The following macros take CME_SNIFF_MODE_OFF_IND *cme_sniff_mode_off_ind_ptr or CsrUint8 *addr */
#define CME_SNIFF_MODE_OFF_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_SNIFF_MODE_OFF_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_SNIFF_MODE_OFF_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_SNIFF_MODE_OFF_IND_BYTE_SIZE (4)
#define CME_SNIFF_MODE_OFF_IND_PACK(addr, acl_handle) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
    } while (0)

#define CME_SNIFF_MODE_OFF_IND_MARSHALL(addr, cme_sniff_mode_off_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_sniff_mode_off_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_sniff_mode_off_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_sniff_mode_off_ind_ptr)->acl_handle) >> 8); \
    } while (0)

#define CME_SNIFF_MODE_OFF_IND_UNMARSHALL(addr, cme_sniff_mode_off_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_sniff_mode_off_ind_ptr)->header)); \
        (cme_sniff_mode_off_ind_ptr)->acl_handle = CME_SNIFF_MODE_OFF_IND_ACL_HANDLE_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_SNIFF_EVENT_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of a specific Sniff
    window. Sniff transactions start at each anchor point as defined by the
    CME_SNIFF_MODE_IND message. The Sniff Event End message indicates when
    the actual activity has been terminated, however note that since this is
    sent from the background it will be sent some time after the event.

  MEMBERS
    acl_handle - Identifies the ACL Link. Note that the handle is not
                 implemented initially due to the difficulty of signalling this
                 between the BT FG and BG.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
} CsrCmeSniffEventEndInd;

/* The following macros take CME_SNIFF_EVENT_END_IND *cme_sniff_event_end_ind_ptr or CsrUint8 *addr */
#define CME_SNIFF_EVENT_END_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_SNIFF_EVENT_END_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_SNIFF_EVENT_END_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_SNIFF_EVENT_END_IND_BYTE_SIZE (4)
#define CME_SNIFF_EVENT_END_IND_PACK(addr, acl_handle) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
    } while (0)

#define CME_SNIFF_EVENT_END_IND_MARSHALL(addr, cme_sniff_event_end_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_sniff_event_end_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_sniff_event_end_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_sniff_event_end_ind_ptr)->acl_handle) >> 8); \
    } while (0)

#define CME_SNIFF_EVENT_END_IND_UNMARSHALL(addr, cme_sniff_event_end_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_sniff_event_end_ind_ptr)->header)); \
        (cme_sniff_event_end_ind_ptr)->acl_handle = CME_SNIFF_EVENT_END_IND_ACL_HANDLE_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_INQUIRY_START_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the start of an inquiry
    operation.

  MEMBERS
    duration - Indicates the maximum time the Inquiry will take in units of 1.28
               seconds.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    duration;
} CsrCmeInquiryStartInd;

/* The following macros take CME_INQUIRY_START_IND *cme_inquiry_start_ind_ptr or CsrUint8 *addr */
#define CME_INQUIRY_START_IND_DURATION_BYTE_OFFSET (2)
#define CME_INQUIRY_START_IND_DURATION_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_INQUIRY_START_IND_DURATION_SET(addr, duration) do { \
        *((addr) + 2) = (CsrUint8)((duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)((duration) >> 8); } while (0)
#define CME_INQUIRY_START_IND_BYTE_SIZE (4)
#define CME_INQUIRY_START_IND_PACK(addr, duration) \
    do { \
        *((addr) + 2) = (CsrUint8)((duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)((duration) >> 8); \
    } while (0)

#define CME_INQUIRY_START_IND_MARSHALL(addr, cme_inquiry_start_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_inquiry_start_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_inquiry_start_ind_ptr)->duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_inquiry_start_ind_ptr)->duration) >> 8); \
    } while (0)

#define CME_INQUIRY_START_IND_UNMARSHALL(addr, cme_inquiry_start_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_inquiry_start_ind_ptr)->header)); \
        (cme_inquiry_start_ind_ptr)->duration = CME_INQUIRY_START_IND_DURATION_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_INQUIRY_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of an inquiry
    operation. The end of an Inquiry period may be different from the
    predicted end indicated by the Duration in the Inquiry Start
    Notification.  The Inquiry may be asynchronously stopped by the host or
    may end after the reporting of a certain number of devices.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeInquiryEndInd;

/* The following macros take CME_INQUIRY_END_IND *cme_inquiry_end_ind_ptr or CsrUint8 *addr */
#define CME_INQUIRY_END_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_PAGE_START_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the start of an page
    operation.

  MEMBERS
    duration - Indicates the maximum time the Page will take in units of
               Bluetooth slots.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    duration;
} CsrCmePageStartInd;

/* The following macros take CME_PAGE_START_IND *cme_page_start_ind_ptr or CsrUint8 *addr */
#define CME_PAGE_START_IND_DURATION_BYTE_OFFSET (2)
#define CME_PAGE_START_IND_DURATION_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_PAGE_START_IND_DURATION_SET(addr, duration) do { \
        *((addr) + 2) = (CsrUint8)((duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)((duration) >> 8); } while (0)
#define CME_PAGE_START_IND_BYTE_SIZE (4)
#define CME_PAGE_START_IND_PACK(addr, duration) \
    do { \
        *((addr) + 2) = (CsrUint8)((duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)((duration) >> 8); \
    } while (0)

#define CME_PAGE_START_IND_MARSHALL(addr, cme_page_start_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_page_start_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_page_start_ind_ptr)->duration) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_page_start_ind_ptr)->duration) >> 8); \
    } while (0)

#define CME_PAGE_START_IND_UNMARSHALL(addr, cme_page_start_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_page_start_ind_ptr)->header)); \
        (cme_page_start_ind_ptr)->duration = CME_PAGE_START_IND_DURATION_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_PAGE_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of an page operation.
    The Page procedure ends in three different cases: 1) an ACL link is
    established; 2) an error occurs during link set-up and 3) the Page
    timeout is reached without the establishment of a connection.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmePageEndInd;

/* The following macros take CME_PAGE_END_IND *cme_page_end_ind_ptr or CsrUint8 *addr */
#define CME_PAGE_END_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_INQUIRY_SCAN_START_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the start of a single inquiry
    scan operation. Note that since this is sent from the background it will
    be sent some time after the event.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeInquiryScanStartInd;

/* The following macros take CME_INQUIRY_SCAN_START_IND *cme_inquiry_scan_start_ind_ptr or CsrUint8 *addr */
#define CME_INQUIRY_SCAN_START_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_INQUIRY_SCAN_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of a single inquiry
    scan operation. Note that since this is sent from the background it will
    be sent some time after the event.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeInquiryScanEndInd;

/* The following macros take CME_INQUIRY_SCAN_END_IND *cme_inquiry_scan_end_ind_ptr or CsrUint8 *addr */
#define CME_INQUIRY_SCAN_END_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_PAGE_SCAN_START_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the start of a single page
    scan operation. Note that since this is sent from the background it will
    be sent some time after the event.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmePageScanStartInd;

/* The following macros take CME_PAGE_SCAN_START_IND *cme_page_scan_start_ind_ptr or CsrUint8 *addr */
#define CME_PAGE_SCAN_START_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_PAGE_SCAN_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of a single page scan
    operation. Note that since this is sent from the background it will be
    sent some time after the event.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmePageScanEndInd;

/* The following macros take CME_PAGE_SCAN_END_IND *cme_page_scan_end_ind_ptr or CsrUint8 *addr */
#define CME_PAGE_SCAN_END_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_ACL_DATA_START_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the avaiability of bulk ACL
    data traffic originating from the host. This message is sent when the ACL
    data buffer level exceeds a threshold.

  MEMBERS
    acl_handle         - Identifies the ACL buffer being monitored.
    buffer_level       - Indicates the buffer level in octets for the ACL data
                         buffer.
    timestamp          - Indicates the microsecond timer time when the buffer
                         level was detected to be above the high threshold.
    first_tx_timestamp - Indicates the microsecond timer time when the first
                         packet was transmitted after the last time this message
                         was sent. Conditional on is_packet_timestamp_enabled in
                         CME_ACL_THRESHOLD_IND.
    last_tx_timestamp  - Indicates the microsecond timer time when the last
                         packet was transmitted prior to this message.
                         Conditional on is_packet_timestamp_enabled in
                         CME_ACL_THRESHOLD_IND.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    buffer_level;
    CsrUint32    timestamp;
    CsrUint32    first_tx_timestamp;
    CsrUint32    last_tx_timestamp;
} CsrCmeAclDataStartInd;

/* The following macros take CME_ACL_DATA_START_IND *cme_acl_data_start_ind_ptr or CsrUint8 *addr */
#define CME_ACL_DATA_START_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_DATA_START_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_DATA_START_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_DATA_START_IND_BUFFER_LEVEL_BYTE_OFFSET (4)
#define CME_ACL_DATA_START_IND_BUFFER_LEVEL_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_DATA_START_IND_BUFFER_LEVEL_SET(addr, buffer_level) do { \
        *((addr) + 4) = (CsrUint8)((buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_level) >> 8); } while (0)
#define CME_ACL_DATA_START_IND_TIMESTAMP_BYTE_OFFSET (6)
#define CME_ACL_DATA_START_IND_TIMESTAMP_GET(addr)  \
    (((CsrUint32)(*((addr) + 6)) | \
      ((CsrUint32)(*((addr) + 7)) << 8) | \
      ((CsrUint32)(*((addr) + 6 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 6 + 3)) << (3 * 8))))
#define CME_ACL_DATA_START_IND_TIMESTAMP_SET(addr, timestamp) do { \
        *((addr) + 6) = (CsrUint8)((timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((timestamp) >> (3 * 8)) & 0xff); } while (0)
#define CME_ACL_DATA_START_IND_FIRST_TX_TIMESTAMP_BYTE_OFFSET (10)
#define CME_ACL_DATA_START_IND_FIRST_TX_TIMESTAMP_GET(addr)  \
    (((CsrUint32)(*((addr) + 10)) | \
      ((CsrUint32)(*((addr) + 11)) << 8) | \
      ((CsrUint32)(*((addr) + 12)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 13)) << (3 * 8))))
#define CME_ACL_DATA_START_IND_FIRST_TX_TIMESTAMP_SET(addr, first_tx_timestamp) do { \
        *((addr) + 10) = (CsrUint8)((first_tx_timestamp) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((first_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)(((first_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((first_tx_timestamp) >> (3 * 8)) & 0xff); } while (0)
#define CME_ACL_DATA_START_IND_LAST_TX_TIMESTAMP_BYTE_OFFSET (14)
#define CME_ACL_DATA_START_IND_LAST_TX_TIMESTAMP_GET(addr)  \
    (((CsrUint32)(*((addr) + 14)) | \
      ((CsrUint32)(*((addr) + 15)) << 8) | \
      ((CsrUint32)(*((addr) + 14 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 14 + 3)) << (3 * 8))))
#define CME_ACL_DATA_START_IND_LAST_TX_TIMESTAMP_SET(addr, last_tx_timestamp) do { \
        *((addr) + 14) = (CsrUint8)((last_tx_timestamp) & 0xff); \
        *((addr) + 15) = (CsrUint8)(((last_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 14 + 2) = (CsrUint8)(((last_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 14 + 3) = (CsrUint8)(((last_tx_timestamp) >> (3 * 8)) & 0xff); } while (0)
#define CME_ACL_DATA_START_IND_BYTE_SIZE (18)
#define CME_ACL_DATA_START_IND_PACK(addr, acl_handle, buffer_level, timestamp, first_tx_timestamp, last_tx_timestamp) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_level) >> 8); \
        *((addr) + 6) = (CsrUint8)((timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)((first_tx_timestamp) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((first_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)(((first_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((first_tx_timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 14) = (CsrUint8)((last_tx_timestamp) & 0xff); \
        *((addr) + 15) = (CsrUint8)(((last_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 14 + 2) = (CsrUint8)(((last_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 14 + 3) = (CsrUint8)(((last_tx_timestamp) >> (3 * 8)) & 0xff); \
    } while (0)

#define CME_ACL_DATA_START_IND_MARSHALL(addr, cme_acl_data_start_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_data_start_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->buffer_level) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->first_tx_timestamp) & 0xff); \
        *((addr) + 11) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->first_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->first_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->first_tx_timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 14) = (CsrUint8)(((cme_acl_data_start_ind_ptr)->last_tx_timestamp) & 0xff); \
        *((addr) + 15) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->last_tx_timestamp) >> 8) & 0xff); \
        *((addr) + 14 + 2) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->last_tx_timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 14 + 3) = (CsrUint8)((((cme_acl_data_start_ind_ptr)->last_tx_timestamp) >> (3 * 8)) & 0xff); \
    } while (0)

#define CME_ACL_DATA_START_IND_UNMARSHALL(addr, cme_acl_data_start_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_data_start_ind_ptr)->header)); \
        (cme_acl_data_start_ind_ptr)->acl_handle = CME_ACL_DATA_START_IND_ACL_HANDLE_GET(addr); \
        (cme_acl_data_start_ind_ptr)->buffer_level = CME_ACL_DATA_START_IND_BUFFER_LEVEL_GET(addr); \
        (cme_acl_data_start_ind_ptr)->timestamp = CME_ACL_DATA_START_IND_TIMESTAMP_GET(addr); \
        (cme_acl_data_start_ind_ptr)->first_tx_timestamp = CME_ACL_DATA_START_IND_FIRST_TX_TIMESTAMP_GET(addr); \
        (cme_acl_data_start_ind_ptr)->last_tx_timestamp = CME_ACL_DATA_START_IND_LAST_TX_TIMESTAMP_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_DATA_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that no more ACL data can be
    sent on an ACL link due to either the end of a burst of ACL data traffic
    originating from the host or flow control stop being asserted by the
    peer. This message is sent when an acknowledgement is received from the
    peer and there is no more ACL data to transmit or flow control off is
    requested.

  MEMBERS
    acl_handle   - Identifies the ACL buffer being monitored.
    buffer_level - Indicates the buffer level in octets for the ACL data buffer.
                   Note that this is only valid when flow_control = 0.
    timestamp    - Indicates the microsecond timer time when the buffer level
                   was detected to be below the low threshold.
    flow_control - Indicates flow control state. 1 => Data flow stopped.
    spare        - Spare.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    buffer_level;
    CsrUint32    timestamp;
    CsrUint8     flow_control; /* Only 1 bits used */
} CsrCmeAclDataEndInd;

/* The following macros take CME_ACL_DATA_END_IND *cme_acl_data_end_ind_ptr or CsrUint8 *addr */
#define CME_ACL_DATA_END_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_DATA_END_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_DATA_END_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_DATA_END_IND_BUFFER_LEVEL_BYTE_OFFSET (4)
#define CME_ACL_DATA_END_IND_BUFFER_LEVEL_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_DATA_END_IND_BUFFER_LEVEL_SET(addr, buffer_level) do { \
        *((addr) + 4) = (CsrUint8)((buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_level) >> 8); } while (0)
#define CME_ACL_DATA_END_IND_TIMESTAMP_BYTE_OFFSET (6)
#define CME_ACL_DATA_END_IND_TIMESTAMP_GET(addr)  \
    (((CsrUint32)(*((addr) + 6)) | \
      ((CsrUint32)(*((addr) + 7)) << 8) | \
      ((CsrUint32)(*((addr) + 6 + 2)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 6 + 3)) << (3 * 8))))
#define CME_ACL_DATA_END_IND_TIMESTAMP_SET(addr, timestamp) do { \
        *((addr) + 6) = (CsrUint8)((timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((timestamp) >> (3 * 8)) & 0xff); } while (0)
#define CME_ACL_DATA_END_IND_FLOW_CONTROL_BYTE_OFFSET (10)
#define CME_ACL_DATA_END_IND_FLOW_CONTROL_GET(addr) (((*((addr) + 10) & 0x1)))
#define CME_ACL_DATA_END_IND_FLOW_CONTROL_SET(addr, flow_control) (*((addr) + 10) =  \
                                                                       (CsrUint8)((*((addr) + 10) & ~0x1) | (((flow_control)) & 0x1)))
#define CME_ACL_DATA_END_IND_BYTE_SIZE (12)
#define CME_ACL_DATA_END_IND_PACK(addr, acl_handle, buffer_level, timestamp, flow_control) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_level) >> 8); \
        *((addr) + 6) = (CsrUint8)((timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)(((timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)(((timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)(((flow_control)) & 0x1); \
        *((addr) + 11) = 0; \
    } while (0)

#define CME_ACL_DATA_END_IND_MARSHALL(addr, cme_acl_data_end_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_data_end_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_data_end_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_data_end_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_data_end_ind_ptr)->buffer_level) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_data_end_ind_ptr)->buffer_level) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_acl_data_end_ind_ptr)->timestamp) & 0xff); \
        *((addr) + 7) = (CsrUint8)((((cme_acl_data_end_ind_ptr)->timestamp) >> 8) & 0xff); \
        *((addr) + 6 + 2) = (CsrUint8)((((cme_acl_data_end_ind_ptr)->timestamp) >> (2 * 8)) & 0xff); \
        *((addr) + 6 + 3) = (CsrUint8)((((cme_acl_data_end_ind_ptr)->timestamp) >> (3 * 8)) & 0xff); \
        *((addr) + 10) = (CsrUint8)((((cme_acl_data_end_ind_ptr)->flow_control)) & 0x1); \
        *((addr) + 11) = 0; \
    } while (0)

#define CME_ACL_DATA_END_IND_UNMARSHALL(addr, cme_acl_data_end_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_data_end_ind_ptr)->header)); \
        (cme_acl_data_end_ind_ptr)->acl_handle = CME_ACL_DATA_END_IND_ACL_HANDLE_GET(addr); \
        (cme_acl_data_end_ind_ptr)->buffer_level = CME_ACL_DATA_END_IND_BUFFER_LEVEL_GET(addr); \
        (cme_acl_data_end_ind_ptr)->timestamp = CME_ACL_DATA_END_IND_TIMESTAMP_GET(addr); \
        (cme_acl_data_end_ind_ptr)->flow_control = CME_ACL_DATA_END_IND_FLOW_CONTROL_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BLE_CONNECT_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that a BLE connection has
    been established between the Bluetooth device and a remote device, or
    that an existing connection has updated parameters.

  MEMBERS
    handle                   - Identifies a particular BLE Link in the firmware
                               and host.
    interval                 - Indicates the time between two anchor points in
                               the BLE connection. This parameter is expressed
                               in Bluetooth Slots; it varies from 7.5ms to 4
                               seconds.
    spare                    - Spare.
    is_master                - Indicates if the Bluetooth device is the Master
                               (0x0001) or Slave (0x0000) of the BLE link.
    latency                  - Indicates the period that the Slave device can
                               delay listening to the Master. The Slave is not
                               required to listen to the Master at each anchor
                               point. This parameter is expressed in Bluetooth
                               Slots; it varies from 7.5ms to 4 seconds.
    window                   - Indicates the expected duration in Bluetooth
                               slots of a connection event (data transfer).
    start_time               - Indicates the microsecond timer time when the BLE
                               connection started. This is used as the reference
                               time for the anchor points.
    link_supervision_timeout - Indicates the maximum time in Bluetooth Slots the
                               Master will wait for communication from the Slave
                               before considering that the connection has been
                               dropped. It varies from 100ms to 32 seconds.
    wallclock_buffer_handle  - The fully qualified buffer handle for the buffer
                               used to read the wallclock.
    wallclock_offset         - An offset in octets into the buffer at which the
                               wallclock shared memory structure can be found.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
    CsrUint16    interval;  /* Only 13 bits used */
    CsrUint8     is_master; /* Only 1 bits used */
    CsrUint16    latency;
    CsrUint16    window;
    CsrUint32    start_time;
    CsrUint16    link_supervision_timeout;
    CsrUint16    wallclock_buffer_handle;
    CsrUint16    wallclock_offset;
} CsrCmeBleConnectInd;

/* The following macros take CME_BLE_CONNECT_IND *cme_ble_connect_ind_ptr or CsrUint8 *addr */
#define CME_BLE_CONNECT_IND_HANDLE_BYTE_OFFSET (2)
#define CME_BLE_CONNECT_IND_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_BLE_CONNECT_IND_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_INTERVAL_BYTE_OFFSET (4)
#define CME_BLE_CONNECT_IND_INTERVAL_GET(addr) ((((*((addr) + 4) & 0xff)) |  \
                                                 ((*((addr) + 5) & 0x1f) << (8 - 0))))
#define CME_BLE_CONNECT_IND_INTERVAL_SET(addr, interval) do { \
        *((addr) + 4) = (CsrUint8)((*((addr) + 4) & ~0xff) | (((interval)) & 0xff)); \
        *((addr) + 5) = (CsrUint8)((*((addr) + 5) & ~0x1f) | (((interval) >> (8 - 0)) & 0x1f)); } while (0)
#define CME_BLE_CONNECT_IND_IS_MASTER_GET(addr) (((*((addr) + 5) & 0x80) >> 7))
#define CME_BLE_CONNECT_IND_IS_MASTER_SET(addr, is_master) (*((addr) + 5) =  \
                                                                (CsrUint8)((*((addr) + 5) & ~0x80) | (((is_master) << 7) & 0x80)))
#define CME_BLE_CONNECT_IND_LATENCY_BYTE_OFFSET (6)
#define CME_BLE_CONNECT_IND_LATENCY_GET(addr)  \
    (((CsrUint16)(*((addr) + 6)) | \
      ((CsrUint16)(*((addr) + 7)) << 8)))
#define CME_BLE_CONNECT_IND_LATENCY_SET(addr, latency) do { \
        *((addr) + 6) = (CsrUint8)((latency) & 0xff); \
        *((addr) + 7) = (CsrUint8)((latency) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_WINDOW_BYTE_OFFSET (8)
#define CME_BLE_CONNECT_IND_WINDOW_GET(addr)  \
    (((CsrUint16)(*((addr) + 8)) | \
      ((CsrUint16)(*((addr) + 8 + 1)) << 8)))
#define CME_BLE_CONNECT_IND_WINDOW_SET(addr, window) do { \
        *((addr) + 8) = (CsrUint8)((window) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((window) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_START_TIME_BYTE_OFFSET (10)
#define CME_BLE_CONNECT_IND_START_TIME_GET(addr)  \
    (((CsrUint32)(*((addr) + 10)) | \
      ((CsrUint32)(*((addr) + 11)) << 8) | \
      ((CsrUint32)(*((addr) + 12)) << (2 * 8)) | \
      ((CsrUint32)(*((addr) + 13)) << (3 * 8))))
#define CME_BLE_CONNECT_IND_START_TIME_SET(addr, start_time) do { \
        *((addr) + 10) = (CsrUint8)((start_time) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((start_time) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)(((start_time) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((start_time) >> (3 * 8)) & 0xff); } while (0)
#define CME_BLE_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_BYTE_OFFSET (14)
#define CME_BLE_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_GET(addr)  \
    (((CsrUint16)(*((addr) + 14)) | \
      ((CsrUint16)(*((addr) + 15)) << 8)))
#define CME_BLE_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_SET(addr, link_supervision_timeout) do { \
        *((addr) + 14) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 15) = (CsrUint8)((link_supervision_timeout) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_BYTE_OFFSET (16)
#define CME_BLE_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 16)) | \
      ((CsrUint16)(*((addr) + 17)) << 8)))
#define CME_BLE_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_SET(addr, wallclock_buffer_handle) do { \
        *((addr) + 16) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 17) = (CsrUint8)((wallclock_buffer_handle) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_WALLCLOCK_OFFSET_BYTE_OFFSET (18)
#define CME_BLE_CONNECT_IND_WALLCLOCK_OFFSET_GET(addr)  \
    (((CsrUint16)(*((addr) + 18)) | \
      ((CsrUint16)(*((addr) + 18 + 1)) << 8)))
#define CME_BLE_CONNECT_IND_WALLCLOCK_OFFSET_SET(addr, wallclock_offset) do { \
        *((addr) + 18) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 18 + 1) = (CsrUint8)((wallclock_offset) >> 8); } while (0)
#define CME_BLE_CONNECT_IND_BYTE_SIZE (20)
#define CME_BLE_CONNECT_IND_PACK(addr, handle, interval, is_master, latency, window, start_time, link_supervision_timeout, wallclock_buffer_handle, wallclock_offset) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((interval)) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((interval) >> (8 - 0)) & 0x1f) | \
                        (CsrUint8)(((is_master) << 7) & 0x80); \
        *((addr) + 6) = (CsrUint8)((latency) & 0xff); \
        *((addr) + 7) = (CsrUint8)((latency) >> 8); \
        *((addr) + 8) = (CsrUint8)((window) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((window) >> 8); \
        *((addr) + 10) = (CsrUint8)((start_time) & 0xff); \
        *((addr) + 11) = (CsrUint8)(((start_time) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)(((start_time) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)(((start_time) >> (3 * 8)) & 0xff); \
        *((addr) + 14) = (CsrUint8)((link_supervision_timeout) & 0xff); \
        *((addr) + 15) = (CsrUint8)((link_supervision_timeout) >> 8); \
        *((addr) + 16) = (CsrUint8)((wallclock_buffer_handle) & 0xff); \
        *((addr) + 17) = (CsrUint8)((wallclock_buffer_handle) >> 8); \
        *((addr) + 18) = (CsrUint8)((wallclock_offset) & 0xff); \
        *((addr) + 18 + 1) = (CsrUint8)((wallclock_offset) >> 8); \
    } while (0)

#define CME_BLE_CONNECT_IND_MARSHALL(addr, cme_ble_connect_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_ble_connect_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_ble_connect_ind_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_ble_connect_ind_ptr)->handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((((cme_ble_connect_ind_ptr)->interval)) & 0xff); \
        *((addr) + 5) = (CsrUint8)((((cme_ble_connect_ind_ptr)->interval) >> (8 - 0)) & 0x1f) | \
                        (CsrUint8)((((cme_ble_connect_ind_ptr)->is_master) << 7) & 0x80); \
        *((addr) + 6) = (CsrUint8)(((cme_ble_connect_ind_ptr)->latency) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((cme_ble_connect_ind_ptr)->latency) >> 8); \
        *((addr) + 8) = (CsrUint8)(((cme_ble_connect_ind_ptr)->window) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)(((cme_ble_connect_ind_ptr)->window) >> 8); \
        *((addr) + 10) = (CsrUint8)(((cme_ble_connect_ind_ptr)->start_time) & 0xff); \
        *((addr) + 11) = (CsrUint8)((((cme_ble_connect_ind_ptr)->start_time) >> 8) & 0xff); \
        *((addr) + 12) = (CsrUint8)((((cme_ble_connect_ind_ptr)->start_time) >> (2 * 8)) & 0xff); \
        *((addr) + 13) = (CsrUint8)((((cme_ble_connect_ind_ptr)->start_time) >> (3 * 8)) & 0xff); \
        *((addr) + 14) = (CsrUint8)(((cme_ble_connect_ind_ptr)->link_supervision_timeout) & 0xff); \
        *((addr) + 15) = (CsrUint8)(((cme_ble_connect_ind_ptr)->link_supervision_timeout) >> 8); \
        *((addr) + 16) = (CsrUint8)(((cme_ble_connect_ind_ptr)->wallclock_buffer_handle) & 0xff); \
        *((addr) + 17) = (CsrUint8)(((cme_ble_connect_ind_ptr)->wallclock_buffer_handle) >> 8); \
        *((addr) + 18) = (CsrUint8)(((cme_ble_connect_ind_ptr)->wallclock_offset) & 0xff); \
        *((addr) + 18 + 1) = (CsrUint8)(((cme_ble_connect_ind_ptr)->wallclock_offset) >> 8); \
    } while (0)

#define CME_BLE_CONNECT_IND_UNMARSHALL(addr, cme_ble_connect_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_ble_connect_ind_ptr)->header)); \
        (cme_ble_connect_ind_ptr)->handle = CME_BLE_CONNECT_IND_HANDLE_GET(addr); \
        (cme_ble_connect_ind_ptr)->interval = CME_BLE_CONNECT_IND_INTERVAL_GET(addr); \
        (cme_ble_connect_ind_ptr)->is_master = CME_BLE_CONNECT_IND_IS_MASTER_GET(addr); \
        (cme_ble_connect_ind_ptr)->latency = CME_BLE_CONNECT_IND_LATENCY_GET(addr); \
        (cme_ble_connect_ind_ptr)->window = CME_BLE_CONNECT_IND_WINDOW_GET(addr); \
        (cme_ble_connect_ind_ptr)->start_time = CME_BLE_CONNECT_IND_START_TIME_GET(addr); \
        (cme_ble_connect_ind_ptr)->link_supervision_timeout = CME_BLE_CONNECT_IND_LINK_SUPERVISION_TIMEOUT_GET(addr); \
        (cme_ble_connect_ind_ptr)->wallclock_buffer_handle = CME_BLE_CONNECT_IND_WALLCLOCK_BUFFER_HANDLE_GET(addr); \
        (cme_ble_connect_ind_ptr)->wallclock_offset = CME_BLE_CONNECT_IND_WALLCLOCK_OFFSET_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BLE_DISCONNECT_REQ

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate that an BLE connection has
    been removed. This should be acknowledged with a CME_BLE_DISCONNECT_CFM
    so the wallclock shared memory can be released.

  MEMBERS
    handle       - Identifies the particular BLE Link.
    wallclock_id - Identifies the wallclock associated with the BLE link. This
                   should be echoed back in the CFM.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
    CsrUint16    wallclock_id;
} CsrCmeBleDisconnectReq;

/* The following macros take CME_BLE_DISCONNECT_REQ *cme_ble_disconnect_req_ptr or CsrUint8 *addr */
#define CME_BLE_DISCONNECT_REQ_HANDLE_BYTE_OFFSET (2)
#define CME_BLE_DISCONNECT_REQ_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_BLE_DISCONNECT_REQ_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_BLE_DISCONNECT_REQ_WALLCLOCK_ID_BYTE_OFFSET (4)
#define CME_BLE_DISCONNECT_REQ_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_BLE_DISCONNECT_REQ_WALLCLOCK_ID_SET(addr, wallclock_id) do { \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); } while (0)
#define CME_BLE_DISCONNECT_REQ_BYTE_SIZE (6)
#define CME_BLE_DISCONNECT_REQ_PACK(addr, handle, wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); \
    } while (0)

#define CME_BLE_DISCONNECT_REQ_MARSHALL(addr, cme_ble_disconnect_req_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_ble_disconnect_req_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_ble_disconnect_req_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_ble_disconnect_req_ptr)->handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_ble_disconnect_req_ptr)->wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_ble_disconnect_req_ptr)->wallclock_id) >> 8); \
    } while (0)

#define CME_BLE_DISCONNECT_REQ_UNMARSHALL(addr, cme_ble_disconnect_req_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_ble_disconnect_req_ptr)->header)); \
        (cme_ble_disconnect_req_ptr)->handle = CME_BLE_DISCONNECT_REQ_HANDLE_GET(addr); \
        (cme_ble_disconnect_req_ptr)->wallclock_id = CME_BLE_DISCONNECT_REQ_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BLE_DISCONNECT_CFM

  DESCRIPTION
    Message from CME_BT to CME_WLAN to acknowledge that an BLE connection has
    been removed

  MEMBERS
    handle       - Identifies the particular BLE Link.
    wallclock_id - Identifies the wallclock associated with the BLE link. This
                   should be taken from the REQ.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
    CsrUint16    wallclock_id;
} CsrCmeBleDisconnectCfm;

/* The following macros take CME_BLE_DISCONNECT_CFM *cme_ble_disconnect_cfm_ptr or CsrUint8 *addr */
#define CME_BLE_DISCONNECT_CFM_HANDLE_BYTE_OFFSET (2)
#define CME_BLE_DISCONNECT_CFM_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_BLE_DISCONNECT_CFM_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_BLE_DISCONNECT_CFM_WALLCLOCK_ID_BYTE_OFFSET (4)
#define CME_BLE_DISCONNECT_CFM_WALLCLOCK_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_BLE_DISCONNECT_CFM_WALLCLOCK_ID_SET(addr, wallclock_id) do { \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); } while (0)
#define CME_BLE_DISCONNECT_CFM_BYTE_SIZE (6)
#define CME_BLE_DISCONNECT_CFM_PACK(addr, handle, wallclock_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((wallclock_id) >> 8); \
    } while (0)

#define CME_BLE_DISCONNECT_CFM_MARSHALL(addr, cme_ble_disconnect_cfm_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_ble_disconnect_cfm_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_ble_disconnect_cfm_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_ble_disconnect_cfm_ptr)->handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_ble_disconnect_cfm_ptr)->wallclock_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_ble_disconnect_cfm_ptr)->wallclock_id) >> 8); \
    } while (0)

#define CME_BLE_DISCONNECT_CFM_UNMARSHALL(addr, cme_ble_disconnect_cfm_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_ble_disconnect_cfm_ptr)->header)); \
        (cme_ble_disconnect_cfm_ptr)->handle = CME_BLE_DISCONNECT_CFM_HANDLE_GET(addr); \
        (cme_ble_disconnect_cfm_ptr)->wallclock_id = CME_BLE_DISCONNECT_CFM_WALLCLOCK_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BLE_SCAN_ENABLED_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the enabling of BLE scan
    operations.

  MEMBERS
    interval  - Indicates the periodicity of the BLE scan in Bluetooth slots.
                This is the period that the link layer switches between
                advertising channels. It varies from 2.5ms to 10.24 seconds.
    window    - Indicates the number of Bluetooth slots used for BLE scan
                activities on each advertising channel. It varies from 20ms to
                10.24 seconds. If the window is equal to the interval, the link
                layer scans continuously.
    is_active - Indicates if the scan is an active scan (0x0001) or a passive
                scan (0x0000).

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    interval;
    CsrUint16    window;    /* Only 15 bits used */
    CsrUint8     is_active; /* Only 1 bits used */
} CsrCmeBleScanEnabledInd;

/* The following macros take CME_BLE_SCAN_ENABLED_IND *cme_ble_scan_enabled_ind_ptr or CsrUint8 *addr */
#define CME_BLE_SCAN_ENABLED_IND_INTERVAL_BYTE_OFFSET (2)
#define CME_BLE_SCAN_ENABLED_IND_INTERVAL_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_BLE_SCAN_ENABLED_IND_INTERVAL_SET(addr, interval) do { \
        *((addr) + 2) = (CsrUint8)((interval) & 0xff); \
        *((addr) + 3) = (CsrUint8)((interval) >> 8); } while (0)
#define CME_BLE_SCAN_ENABLED_IND_WINDOW_BYTE_OFFSET (4)
#define CME_BLE_SCAN_ENABLED_IND_WINDOW_GET(addr) ((((*((addr) + 4) & 0xff)) |  \
                                                    ((*((addr) + 5) & 0x7f) << (8 - 0))))
#define CME_BLE_SCAN_ENABLED_IND_WINDOW_SET(addr, window) do { \
        *((addr) + 4) = (CsrUint8)((*((addr) + 4) & ~0xff) | (((window)) & 0xff)); \
        *((addr) + 5) = (CsrUint8)((*((addr) + 5) & ~0x7f) | (((window) >> (8 - 0)) & 0x7f)); } while (0)
#define CME_BLE_SCAN_ENABLED_IND_IS_ACTIVE_GET(addr) (((*((addr) + 5) & 0x80) >> 7))
#define CME_BLE_SCAN_ENABLED_IND_IS_ACTIVE_SET(addr, is_active) (*((addr) + 5) =  \
                                                                     (CsrUint8)((*((addr) + 5) & ~0x80) | (((is_active) << 7) & 0x80)))
#define CME_BLE_SCAN_ENABLED_IND_BYTE_SIZE (6)
#define CME_BLE_SCAN_ENABLED_IND_PACK(addr, interval, window, is_active) \
    do { \
        *((addr) + 2) = (CsrUint8)((interval) & 0xff); \
        *((addr) + 3) = (CsrUint8)((interval) >> 8); \
        *((addr) + 4) = (CsrUint8)(((window)) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((window) >> (8 - 0)) & 0x7f) | \
                        (CsrUint8)(((is_active) << 7) & 0x80); \
    } while (0)

#define CME_BLE_SCAN_ENABLED_IND_MARSHALL(addr, cme_ble_scan_enabled_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_ble_scan_enabled_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_ble_scan_enabled_ind_ptr)->interval) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_ble_scan_enabled_ind_ptr)->interval) >> 8); \
        *((addr) + 4) = (CsrUint8)((((cme_ble_scan_enabled_ind_ptr)->window)) & 0xff); \
        *((addr) + 5) = (CsrUint8)((((cme_ble_scan_enabled_ind_ptr)->window) >> (8 - 0)) & 0x7f) | \
                        (CsrUint8)((((cme_ble_scan_enabled_ind_ptr)->is_active) << 7) & 0x80); \
    } while (0)

#define CME_BLE_SCAN_ENABLED_IND_UNMARSHALL(addr, cme_ble_scan_enabled_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_ble_scan_enabled_ind_ptr)->header)); \
        (cme_ble_scan_enabled_ind_ptr)->interval = CME_BLE_SCAN_ENABLED_IND_INTERVAL_GET(addr); \
        (cme_ble_scan_enabled_ind_ptr)->window = CME_BLE_SCAN_ENABLED_IND_WINDOW_GET(addr); \
        (cme_ble_scan_enabled_ind_ptr)->is_active = CME_BLE_SCAN_ENABLED_IND_IS_ACTIVE_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BLE_SCAN_DISABLED_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the disabling of passive scan
    operations.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeBleScanDisabledInd;

/* The following macros take CME_BLE_SCAN_DISABLED_IND *cme_ble_scan_disabled_ind_ptr or CsrUint8 *addr */
#define CME_BLE_SCAN_DISABLED_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_WLAN_CHANNEL_MAP_IND

  DESCRIPTION
    Message from CME_WLAN to CME_BT to indicate which Bluetooth channels
    should be used to avoid conflict with WLAN. 1 => OK to use, 0 => Not OK
    to use.

  MEMBERS
    channel_map - Bit map with each bit corresponding to a Bluetooth channel.
                  The bit map is compatible with Bluetooth HCI command syntax.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    channel_map[5];
} CsrCmeWlanChannelMapInd;

/* The following macros take CME_WLAN_CHANNEL_MAP_IND *cme_wlan_channel_map_ind_ptr or CsrUint8 *addr */
#define CME_WLAN_CHANNEL_MAP_IND_CHANNEL_MAP_BYTE_OFFSET (2)
#define CME_WLAN_CHANNEL_MAP_IND_BYTE_SIZE (12)


/*******************************************************************************

  NAME
    CME_PROFILE_A2DP_START_IND

  DESCRIPTION
    Message from CME_BH to CME_WLAN to indicate that an A2DP profile has
    started a connection or that an existing connection has resumed with
    updated parameters.

  MEMBERS
    acl_handle          - Identifies the ACL Link used for the profile
                          connection.
    l2cap_connection_id - Identifies the local L2CAP connection ID.
    bit_rate            - Identifies the bit rate of the codec in kbps.
    codec_type          - Identifies the codec type (CME_CODEC_TYPE enum).
    codec_location      - Identifies the location of the codec
                          (CME_CODEC_LOCATION enum).
    sdu_size            - Identifies the maximum size of the A2DP SDU (MTU
                          negotiated for the L2CAP link) in octets.
    period              - Identifies the period in ms of codec data being
                          available for transmission.
    role                - Identifies the local device role, source or sink.
    sampling_freq       - Identifies the sampling frequency of the A2DP audio
                          stream.
    spare               - Spare.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader            header;
    CsrUint16               acl_handle;
    CsrUint16               l2cap_connection_id;
    CsrUint16               bit_rate; /* Only 12 bits used */
    CsrCmeCodecType         codec_type;
    CsrCmeCodecLocation     codec_location;
    CsrUint16               sdu_size;
    CsrUint8                period;
    CsrCmeA2dpRole          role;
    CsrCmeCodecSamplingFreq sampling_freq;
} CsrCmeProfileA2dpStartInd;

/* The following macros take CME_PROFILE_A2DP_START_IND *cme_profile_a2dp_start_ind_ptr or CsrUint8 *addr */
#define CME_PROFILE_A2DP_START_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_PROFILE_A2DP_START_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_PROFILE_A2DP_START_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_PROFILE_A2DP_START_IND_L2CAP_CONNECTION_ID_BYTE_OFFSET (4)
#define CME_PROFILE_A2DP_START_IND_L2CAP_CONNECTION_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_PROFILE_A2DP_START_IND_L2CAP_CONNECTION_ID_SET(addr, l2cap_connection_id) do { \
        *((addr) + 4) = (CsrUint8)((l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((l2cap_connection_id) >> 8); } while (0)
#define CME_PROFILE_A2DP_START_IND_BIT_RATE_BYTE_OFFSET (6)
#define CME_PROFILE_A2DP_START_IND_BIT_RATE_GET(addr) ((((*((addr) + 6) & 0xff)) |  \
                                                        ((*((addr) + 7) & 0xf) << (8 - 0))))
#define CME_PROFILE_A2DP_START_IND_BIT_RATE_SET(addr, bit_rate) do { \
        *((addr) + 6) = (CsrUint8)((*((addr) + 6) & ~0xff) | (((bit_rate)) & 0xff)); \
        *((addr) + 7) = (CsrUint8)((*((addr) + 7) & ~0xf) | (((bit_rate) >> (8 - 0)) & 0xf)); } while (0)
#define CME_PROFILE_A2DP_START_IND_CODEC_TYPE_GET(addr) ((CsrCmeCodecType)((*((addr) + 7) & 0x70) >> 4))
#define CME_PROFILE_A2DP_START_IND_CODEC_TYPE_SET(addr, codec_type) (*((addr) + 7) =  \
                                                                         (CsrUint8)((*((addr) + 7) & ~0x70) | (((codec_type) << 4) & 0x70)))
#define CME_PROFILE_A2DP_START_IND_CODEC_LOCATION_GET(addr) ((CsrCmeCodecLocation)((*((addr) + 7) & 0x80) >> 7))
#define CME_PROFILE_A2DP_START_IND_CODEC_LOCATION_SET(addr, codec_location) (*((addr) + 7) =  \
                                                                                 (CsrUint8)((*((addr) + 7) & ~0x80) | (((codec_location) << 7) & 0x80)))
#define CME_PROFILE_A2DP_START_IND_SDU_SIZE_BYTE_OFFSET (8)
#define CME_PROFILE_A2DP_START_IND_SDU_SIZE_GET(addr)  \
    (((CsrUint16)(*((addr) + 8)) | \
      ((CsrUint16)(*((addr) + 8 + 1)) << 8)))
#define CME_PROFILE_A2DP_START_IND_SDU_SIZE_SET(addr, sdu_size) do { \
        *((addr) + 8) = (CsrUint8)((sdu_size) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((sdu_size) >> 8); } while (0)
#define CME_PROFILE_A2DP_START_IND_PERIOD_BYTE_OFFSET (10)
#define CME_PROFILE_A2DP_START_IND_PERIOD_GET(addr) (*((addr) + 10))
#define CME_PROFILE_A2DP_START_IND_PERIOD_SET(addr, period) (*((addr) + 10) = (CsrUint8)(period))
#define CME_PROFILE_A2DP_START_IND_ROLE_BYTE_OFFSET (11)
#define CME_PROFILE_A2DP_START_IND_ROLE_GET(addr) ((CsrCmeA2dpRole)((*((addr) + 11) & 0x1)))
#define CME_PROFILE_A2DP_START_IND_ROLE_SET(addr, role) (*((addr) + 11) =  \
                                                             (CsrUint8)((*((addr) + 11) & ~0x1) | (((role)) & 0x1)))
#define CME_PROFILE_A2DP_START_IND_SAMPLING_FREQ_GET(addr) ((CsrCmeCodecSamplingFreq)((*((addr) + 11) & 0x6) >> 1))
#define CME_PROFILE_A2DP_START_IND_SAMPLING_FREQ_SET(addr, sampling_freq) (*((addr) + 11) =  \
                                                                               (CsrUint8)((*((addr) + 11) & ~0x6) | (((sampling_freq) << 1) & 0x6)))
#define CME_PROFILE_A2DP_START_IND_BYTE_SIZE (12)
#define CME_PROFILE_A2DP_START_IND_PACK(addr, acl_handle, l2cap_connection_id, bit_rate, codec_type, codec_location, sdu_size, period, role, sampling_freq) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((l2cap_connection_id) >> 8); \
        *((addr) + 6) = (CsrUint8)(((bit_rate)) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((bit_rate) >> (8 - 0)) & 0xf) | \
                        (CsrUint8)(((codec_type) << 4) & 0x70) | \
                        (CsrUint8)(((codec_location) << 7) & 0x80); \
        *((addr) + 8) = (CsrUint8)((sdu_size) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)((sdu_size) >> 8); \
        *((addr) + 10) = (CsrUint8)(period); \
        *((addr) + 11) = (CsrUint8)(((role)) & 0x1) | \
                         (CsrUint8)(((sampling_freq) << 1) & 0x6); \
    } while (0)

#define CME_PROFILE_A2DP_START_IND_MARSHALL(addr, cme_profile_a2dp_start_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_profile_a2dp_start_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->l2cap_connection_id) >> 8); \
        *((addr) + 6) = (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->bit_rate)) & 0xff); \
        *((addr) + 7) = (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->bit_rate) >> (8 - 0)) & 0xf) | \
                        (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->codec_type) << 4) & 0x70) | \
                        (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->codec_location) << 7) & 0x80); \
        *((addr) + 8) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->sdu_size) & 0xff); \
        *((addr) + 8 + 1) = (CsrUint8)(((cme_profile_a2dp_start_ind_ptr)->sdu_size) >> 8); \
        *((addr) + 10) = (CsrUint8)((cme_profile_a2dp_start_ind_ptr)->period); \
        *((addr) + 11) = (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->role)) & 0x1) | \
                         (CsrUint8)((((cme_profile_a2dp_start_ind_ptr)->sampling_freq) << 1) & 0x6); \
    } while (0)

#define CME_PROFILE_A2DP_START_IND_UNMARSHALL(addr, cme_profile_a2dp_start_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_profile_a2dp_start_ind_ptr)->header)); \
        (cme_profile_a2dp_start_ind_ptr)->acl_handle = CME_PROFILE_A2DP_START_IND_ACL_HANDLE_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->l2cap_connection_id = CME_PROFILE_A2DP_START_IND_L2CAP_CONNECTION_ID_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->bit_rate = CME_PROFILE_A2DP_START_IND_BIT_RATE_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->codec_type = CME_PROFILE_A2DP_START_IND_CODEC_TYPE_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->codec_location = CME_PROFILE_A2DP_START_IND_CODEC_LOCATION_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->sdu_size = CME_PROFILE_A2DP_START_IND_SDU_SIZE_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->period = CME_PROFILE_A2DP_START_IND_PERIOD_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->role = CME_PROFILE_A2DP_START_IND_ROLE_GET(addr); \
        (cme_profile_a2dp_start_ind_ptr)->sampling_freq = CME_PROFILE_A2DP_START_IND_SAMPLING_FREQ_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_PROFILE_A2DP_STOP_IND

  DESCRIPTION
    Message from CME_BH to CME_WLAN to indicate that an A2DP profile has
    stopped or paused.

  MEMBERS
    acl_handle          - Identifies the ACL Link used for the profile
                          connection.
    l2cap_connection_id - Identifies the local L2CAP connection ID.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    l2cap_connection_id;
} CsrCmeProfileA2dpStopInd;

/* The following macros take CME_PROFILE_A2DP_STOP_IND *cme_profile_a2dp_stop_ind_ptr or CsrUint8 *addr */
#define CME_PROFILE_A2DP_STOP_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_PROFILE_A2DP_STOP_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_PROFILE_A2DP_STOP_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_PROFILE_A2DP_STOP_IND_L2CAP_CONNECTION_ID_BYTE_OFFSET (4)
#define CME_PROFILE_A2DP_STOP_IND_L2CAP_CONNECTION_ID_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_PROFILE_A2DP_STOP_IND_L2CAP_CONNECTION_ID_SET(addr, l2cap_connection_id) do { \
        *((addr) + 4) = (CsrUint8)((l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((l2cap_connection_id) >> 8); } while (0)
#define CME_PROFILE_A2DP_STOP_IND_BYTE_SIZE (6)
#define CME_PROFILE_A2DP_STOP_IND_PACK(addr, acl_handle, l2cap_connection_id) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)((l2cap_connection_id) >> 8); \
    } while (0)

#define CME_PROFILE_A2DP_STOP_IND_MARSHALL(addr, cme_profile_a2dp_stop_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_profile_a2dp_stop_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_profile_a2dp_stop_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_profile_a2dp_stop_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_profile_a2dp_stop_ind_ptr)->l2cap_connection_id) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_profile_a2dp_stop_ind_ptr)->l2cap_connection_id) >> 8); \
    } while (0)

#define CME_PROFILE_A2DP_STOP_IND_UNMARSHALL(addr, cme_profile_a2dp_stop_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_profile_a2dp_stop_ind_ptr)->header)); \
        (cme_profile_a2dp_stop_ind_ptr)->acl_handle = CME_PROFILE_A2DP_STOP_IND_ACL_HANDLE_GET(addr); \
        (cme_profile_a2dp_stop_ind_ptr)->l2cap_connection_id = CME_PROFILE_A2DP_STOP_IND_L2CAP_CONNECTION_ID_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_FLOW_CONTROL_IND

  DESCRIPTION
    Message from CME_WLAN to CME_BH to request flow control of non-A2DP data.
    The BT host will only allow max_non_a2dp_sdus non-A2DP SDUs to be written
    in every time_period_ms milliseconds. The flow control mechanism can be
    disabled by setting time_period_ms = 0.

  MEMBERS
    max_non_a2dp_sdus - Identifies the number of non-A2DP SDUs that can be
                        written in the time period time_period_ms.
    time_period_ms    - Identifies the time period during which max_non_a2dp
                        non-A2DP SDUs can be written.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint8     max_non_a2dp_sdus;
    CsrUint8     time_period_ms;
} CsrCmeAclFlowControlInd;

/* The following macros take CME_ACL_FLOW_CONTROL_IND *cme_acl_flow_control_ind_ptr or CsrUint8 *addr */
#define CME_ACL_FLOW_CONTROL_IND_MAX_NON_A2DP_SDUS_BYTE_OFFSET (2)
#define CME_ACL_FLOW_CONTROL_IND_MAX_NON_A2DP_SDUS_GET(addr) (*((addr) + 2))
#define CME_ACL_FLOW_CONTROL_IND_MAX_NON_A2DP_SDUS_SET(addr, max_non_a2dp_sdus) (*((addr) + 2) = (CsrUint8)(max_non_a2dp_sdus))
#define CME_ACL_FLOW_CONTROL_IND_TIME_PERIOD_MS_BYTE_OFFSET (3)
#define CME_ACL_FLOW_CONTROL_IND_TIME_PERIOD_MS_GET(addr) (*((addr) + 3))
#define CME_ACL_FLOW_CONTROL_IND_TIME_PERIOD_MS_SET(addr, time_period_ms) (*((addr) + 3) = (CsrUint8)(time_period_ms))
#define CME_ACL_FLOW_CONTROL_IND_BYTE_SIZE (4)
#define CME_ACL_FLOW_CONTROL_IND_PACK(addr, max_non_a2dp_sdus, time_period_ms) \
    do { \
        *((addr) + 2) = (CsrUint8)(max_non_a2dp_sdus); \
        *((addr) + 3) = (CsrUint8)(time_period_ms); \
    } while (0)

#define CME_ACL_FLOW_CONTROL_IND_MARSHALL(addr, cme_acl_flow_control_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_flow_control_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)((cme_acl_flow_control_ind_ptr)->max_non_a2dp_sdus); \
        *((addr) + 3) = (CsrUint8)((cme_acl_flow_control_ind_ptr)->time_period_ms); \
    } while (0)

#define CME_ACL_FLOW_CONTROL_IND_UNMARSHALL(addr, cme_acl_flow_control_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_flow_control_ind_ptr)->header)); \
        (cme_acl_flow_control_ind_ptr)->max_non_a2dp_sdus = CME_ACL_FLOW_CONTROL_IND_MAX_NON_A2DP_SDUS_GET(addr); \
        (cme_acl_flow_control_ind_ptr)->time_period_ms = CME_ACL_FLOW_CONTROL_IND_TIME_PERIOD_MS_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_ACL_THRESHOLD_IND

  DESCRIPTION
    Message from CME_WLAN to CME_BT to request reporting of buffer levels on
    for an ACL buffer.

  MEMBERS
    acl_handle                        - Identifies the ACL buffer to be
                                        monitored.
    buffer_high_threshold             - Identifies the level in octets of the
                                        ACL buffer above which a
                                        CME_ACL_DATA_START_IND message should be
                                        sent.
    buffer_low_threshold              - Identifies the level in octets of the
                                        ACL buffer below which a
                                        CME_ACL_DATA_END_IND message should be
                                        sent.
    is_high_threshold_enabled         - Indicates if exceeding the
                                        buffer_high_threshold should trigger
                                        sending a CME_ACL_DATA_START_IND
                                        message. Mutually exclusive with
                                        is_debug_high_enabled bit.
    is_low_threshold_enabled          - Indicates if going below the
                                        buffer_low_threshold should trigger
                                        sending a CME_ACL_DATA_START_IND message.
    is_debug_high_enabled             - Indicates if going up (ignoring high
                                        threshold) should trigger sending a
                                        CME_ACL_DATA_START_IND message. Mutually
                                        exclusive with is_high_threshold_enabled
                                        bit.
    is_packet_timestamp_enabled       - Indicates if first/last packet
                                        timestamps should be included in the
                                        CME_ACL_DATA_START_IND message.
    is_flow_control_reporting_enabled - Indicates if the flow_control state
                                        changing to stopped should trigger the
                                        CME_ACL_DATA_END_IND message.
    spare                             - Spare.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    acl_handle;
    CsrUint16    buffer_high_threshold;
    CsrUint16    buffer_low_threshold;
    CsrUint8     is_high_threshold_enabled;         /* Only 1 bits used */
    CsrUint8     is_low_threshold_enabled;          /* Only 1 bits used */
    CsrUint8     is_debug_high_enabled;             /* Only 1 bits used */
    CsrUint8     is_packet_timestamp_enabled;       /* Only 1 bits used */
    CsrUint8     is_flow_control_reporting_enabled; /* Only 1 bits used */
} CsrCmeAclThresholdInd;

/* The following macros take CME_ACL_THRESHOLD_IND *cme_acl_threshold_ind_ptr or CsrUint8 *addr */
#define CME_ACL_THRESHOLD_IND_ACL_HANDLE_BYTE_OFFSET (2)
#define CME_ACL_THRESHOLD_IND_ACL_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_ACL_THRESHOLD_IND_ACL_HANDLE_SET(addr, acl_handle) do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); } while (0)
#define CME_ACL_THRESHOLD_IND_BUFFER_HIGH_THRESHOLD_BYTE_OFFSET (4)
#define CME_ACL_THRESHOLD_IND_BUFFER_HIGH_THRESHOLD_GET(addr)  \
    (((CsrUint16)(*((addr) + 4)) | \
      ((CsrUint16)(*((addr) + 5)) << 8)))
#define CME_ACL_THRESHOLD_IND_BUFFER_HIGH_THRESHOLD_SET(addr, buffer_high_threshold) do { \
        *((addr) + 4) = (CsrUint8)((buffer_high_threshold) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_high_threshold) >> 8); } while (0)
#define CME_ACL_THRESHOLD_IND_BUFFER_LOW_THRESHOLD_BYTE_OFFSET (6)
#define CME_ACL_THRESHOLD_IND_BUFFER_LOW_THRESHOLD_GET(addr)  \
    (((CsrUint16)(*((addr) + 6)) | \
      ((CsrUint16)(*((addr) + 7)) << 8)))
#define CME_ACL_THRESHOLD_IND_BUFFER_LOW_THRESHOLD_SET(addr, buffer_low_threshold) do { \
        *((addr) + 6) = (CsrUint8)((buffer_low_threshold) & 0xff); \
        *((addr) + 7) = (CsrUint8)((buffer_low_threshold) >> 8); } while (0)
#define CME_ACL_THRESHOLD_IND_IS_HIGH_THRESHOLD_ENABLED_BYTE_OFFSET (8)
#define CME_ACL_THRESHOLD_IND_IS_HIGH_THRESHOLD_ENABLED_GET(addr) (((*((addr) + 8) & 0x1)))
#define CME_ACL_THRESHOLD_IND_IS_HIGH_THRESHOLD_ENABLED_SET(addr, is_high_threshold_enabled) (*((addr) + 8) =  \
                                                                                                  (CsrUint8)((*((addr) + 8) & ~0x1) | (((is_high_threshold_enabled)) & 0x1)))
#define CME_ACL_THRESHOLD_IND_IS_LOW_THRESHOLD_ENABLED_GET(addr) (((*((addr) + 8) & 0x2) >> 1))
#define CME_ACL_THRESHOLD_IND_IS_LOW_THRESHOLD_ENABLED_SET(addr, is_low_threshold_enabled) (*((addr) + 8) =  \
                                                                                                (CsrUint8)((*((addr) + 8) & ~0x2) | (((is_low_threshold_enabled) << 1) & 0x2)))
#define CME_ACL_THRESHOLD_IND_IS_DEBUG_HIGH_ENABLED_GET(addr) (((*((addr) + 8) & 0x4) >> 2))
#define CME_ACL_THRESHOLD_IND_IS_DEBUG_HIGH_ENABLED_SET(addr, is_debug_high_enabled) (*((addr) + 8) =  \
                                                                                          (CsrUint8)((*((addr) + 8) & ~0x4) | (((is_debug_high_enabled) << 2) & 0x4)))
#define CME_ACL_THRESHOLD_IND_IS_PACKET_TIMESTAMP_ENABLED_GET(addr) (((*((addr) + 8) & 0x8) >> 3))
#define CME_ACL_THRESHOLD_IND_IS_PACKET_TIMESTAMP_ENABLED_SET(addr, is_packet_timestamp_enabled) (*((addr) + 8) =  \
                                                                                                      (CsrUint8)((*((addr) + 8) & ~0x8) | (((is_packet_timestamp_enabled) << 3) & 0x8)))
#define CME_ACL_THRESHOLD_IND_IS_FLOW_CONTROL_REPORTING_ENABLED_GET(addr) (((*((addr) + 8) & 0x10) >> 4))
#define CME_ACL_THRESHOLD_IND_IS_FLOW_CONTROL_REPORTING_ENABLED_SET(addr, is_flow_control_reporting_enabled) (*((addr) + 8) =  \
                                                                                                                  (CsrUint8)((*((addr) + 8) & ~0x10) | (((is_flow_control_reporting_enabled) << 4) & 0x10)))
#define CME_ACL_THRESHOLD_IND_BYTE_SIZE (10)
#define CME_ACL_THRESHOLD_IND_PACK(addr, acl_handle, buffer_high_threshold, buffer_low_threshold, is_high_threshold_enabled, is_low_threshold_enabled, is_debug_high_enabled, is_packet_timestamp_enabled, is_flow_control_reporting_enabled) \
    do { \
        *((addr) + 2) = (CsrUint8)((acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)((buffer_high_threshold) & 0xff); \
        *((addr) + 5) = (CsrUint8)((buffer_high_threshold) >> 8); \
        *((addr) + 6) = (CsrUint8)((buffer_low_threshold) & 0xff); \
        *((addr) + 7) = (CsrUint8)((buffer_low_threshold) >> 8); \
        *((addr) + 8) = (CsrUint8)(((is_high_threshold_enabled)) & 0x1) | \
                        (CsrUint8)(((is_low_threshold_enabled) << 1) & 0x2) | \
                        (CsrUint8)(((is_debug_high_enabled) << 2) & 0x4) | \
                        (CsrUint8)(((is_packet_timestamp_enabled) << 3) & 0x8) | \
                        (CsrUint8)(((is_flow_control_reporting_enabled) << 4) & 0x10); \
        *((addr) + 8 + 1) = 0; \
    } while (0)

#define CME_ACL_THRESHOLD_IND_MARSHALL(addr, cme_acl_threshold_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_acl_threshold_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->acl_handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->acl_handle) >> 8); \
        *((addr) + 4) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->buffer_high_threshold) & 0xff); \
        *((addr) + 5) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->buffer_high_threshold) >> 8); \
        *((addr) + 6) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->buffer_low_threshold) & 0xff); \
        *((addr) + 7) = (CsrUint8)(((cme_acl_threshold_ind_ptr)->buffer_low_threshold) >> 8); \
        *((addr) + 8) = (CsrUint8)((((cme_acl_threshold_ind_ptr)->is_high_threshold_enabled)) & 0x1) | \
                        (CsrUint8)((((cme_acl_threshold_ind_ptr)->is_low_threshold_enabled) << 1) & 0x2) | \
                        (CsrUint8)((((cme_acl_threshold_ind_ptr)->is_debug_high_enabled) << 2) & 0x4) | \
                        (CsrUint8)((((cme_acl_threshold_ind_ptr)->is_packet_timestamp_enabled) << 3) & 0x8) | \
                        (CsrUint8)((((cme_acl_threshold_ind_ptr)->is_flow_control_reporting_enabled) << 4) & 0x10); \
        *((addr) + 8 + 1) = 0; \
    } while (0)

#define CME_ACL_THRESHOLD_IND_UNMARSHALL(addr, cme_acl_threshold_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_acl_threshold_ind_ptr)->header)); \
        (cme_acl_threshold_ind_ptr)->acl_handle = CME_ACL_THRESHOLD_IND_ACL_HANDLE_GET(addr); \
        (cme_acl_threshold_ind_ptr)->buffer_high_threshold = CME_ACL_THRESHOLD_IND_BUFFER_HIGH_THRESHOLD_GET(addr); \
        (cme_acl_threshold_ind_ptr)->buffer_low_threshold = CME_ACL_THRESHOLD_IND_BUFFER_LOW_THRESHOLD_GET(addr); \
        (cme_acl_threshold_ind_ptr)->is_high_threshold_enabled = CME_ACL_THRESHOLD_IND_IS_HIGH_THRESHOLD_ENABLED_GET(addr); \
        (cme_acl_threshold_ind_ptr)->is_low_threshold_enabled = CME_ACL_THRESHOLD_IND_IS_LOW_THRESHOLD_ENABLED_GET(addr); \
        (cme_acl_threshold_ind_ptr)->is_debug_high_enabled = CME_ACL_THRESHOLD_IND_IS_DEBUG_HIGH_ENABLED_GET(addr); \
        (cme_acl_threshold_ind_ptr)->is_packet_timestamp_enabled = CME_ACL_THRESHOLD_IND_IS_PACKET_TIMESTAMP_ENABLED_GET(addr); \
        (cme_acl_threshold_ind_ptr)->is_flow_control_reporting_enabled = CME_ACL_THRESHOLD_IND_IS_FLOW_CONTROL_REPORTING_ENABLED_GET(addr); \
    } while (0)


/*******************************************************************************

  NAME
    CME_BT_CAL_START_REQ

  DESCRIPTION
    Message from CME_BT to CME_WLAN to request WLAN to allow BT to start
    calibration operations. Requests WLAN subsystem to enter keep-alive mode
    and to allow BT periods where it can perform calibration during the gaps
    in the keep-alive operations. These periods will be variable length and
    BT may chose not to use a period if it is too short. WLAN confirms with a
    CME_BT_CAL_START_CFM, which indicates that the bt_defer_scheduling
    mechanism has been initialised, and controls access to the antenna using
    the bt_defer_scheduling mechanism.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeBtCalStartReq;

/* The following macros take CME_BT_CAL_START_REQ *cme_bt_cal_start_req_ptr or CsrUint8 *addr */
#define CME_BT_CAL_START_REQ_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_BT_CAL_START_CFM

  DESCRIPTION
    Message from CME_WLAN to CME_BT to indicate that the bt_defer_scheduling
    mechanism in shared memory has been configured and should be used to
    control the calibration operations.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeBtCalStartCfm;

/* The following macros take CME_BT_CAL_START_CFM *cme_bt_cal_start_cfm_ptr or CsrUint8 *addr */
#define CME_BT_CAL_START_CFM_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_BT_CAL_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of calibration
    operations. Allows WLAN subsystem to exit keep-alive mode.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeBtCalEndInd;

/* The following macros take CME_BT_CAL_END_IND *cme_bt_cal_end_ind_ptr or CsrUint8 *addr */
#define CME_BT_CAL_END_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_WLAN_CHANNEL_MAP_LO_HI_IND

  DESCRIPTION
    Message from CME_WLAN to CME_BT to indicate which Bluetooth channels
    should use high Local Oscillator (above wanted) and which should use low
    Local Oscillator (below wanted) to avoid interference to WLAN 5GHz
    receiver. 1 => High LO, 0 => Low LO.

  MEMBERS
    channel_map - Bit map with each bit corresponding to a Bluetooth channel.
                  The bit map is compatible with Bluetooth HCI command syntax.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    channel_map[5];
} CsrCmeWlanChannelMapLoHiInd;

/* The following macros take CME_WLAN_CHANNEL_MAP_LO_HI_IND *cme_wlan_channel_map_lo_hi_ind_ptr or CsrUint8 *addr */
#define CME_WLAN_CHANNEL_MAP_LO_HI_IND_CHANNEL_MAP_BYTE_OFFSET (2)
#define CME_WLAN_CHANNEL_MAP_LO_HI_IND_BYTE_SIZE (12)


/*******************************************************************************

  NAME
    CME_COEX_STOP_IND

  DESCRIPTION
    Message from CME_WLAN to CME_BH to indicate that the coex service is
    stopping. CME_BH responds with CME_COEX_STOP_RSP.

  MEMBERS

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
} CsrCmeCoexStopInd;

/* The following macros take CME_COEX_STOP_IND *cme_coex_stop_ind_ptr or CsrUint8 *addr */
#define CME_COEX_STOP_IND_BYTE_SIZE (2)


/*******************************************************************************

  NAME
    CME_BLE_CONNECTION_EVENT_END_IND

  DESCRIPTION
    Message from CME_BT to CME_WLAN to indicate the end of a specific BLE
    connection event. BLE connection events start at each anchor point as
    defined by the CME_BLE_CONNECT_IND message. The BLE Connection Event End
    message indicates when the actual activity has been terminated, however
    note that since this is sent from the background it will be sent some
    time after the event.

  MEMBERS
    handle - Identifies the BLE Link. Note that the handle is not implemented
             initially due to the difficulty of signalling this between the BT
             FG and BG.

*******************************************************************************/
typedef struct
{
    CsrCmeHeader header;
    CsrUint16    handle;
} CsrCmeBleConnectionEventEndInd;

/* The following macros take CME_BLE_CONNECTION_EVENT_END_IND *cme_ble_connection_event_end_ind_ptr or CsrUint8 *addr */
#define CME_BLE_CONNECTION_EVENT_END_IND_HANDLE_BYTE_OFFSET (2)
#define CME_BLE_CONNECTION_EVENT_END_IND_HANDLE_GET(addr)  \
    (((CsrUint16)(*((addr) + 2)) | \
      ((CsrUint16)(*((addr) + 3)) << 8)))
#define CME_BLE_CONNECTION_EVENT_END_IND_HANDLE_SET(addr, handle) do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); } while (0)
#define CME_BLE_CONNECTION_EVENT_END_IND_BYTE_SIZE (4)
#define CME_BLE_CONNECTION_EVENT_END_IND_PACK(addr, handle) \
    do { \
        *((addr) + 2) = (CsrUint8)((handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)((handle) >> 8); \
    } while (0)

#define CME_BLE_CONNECTION_EVENT_END_IND_MARSHALL(addr, cme_ble_connection_event_end_ind_ptr) \
    do { \
        CME_HEADER_MARSHALL((addr), &((cme_ble_connection_event_end_ind_ptr)->header)); \
        *((addr) + 2) = (CsrUint8)(((cme_ble_connection_event_end_ind_ptr)->handle) & 0xff); \
        *((addr) + 3) = (CsrUint8)(((cme_ble_connection_event_end_ind_ptr)->handle) >> 8); \
    } while (0)

#define CME_BLE_CONNECTION_EVENT_END_IND_UNMARSHALL(addr, cme_ble_connection_event_end_ind_ptr) \
    do { \
        CME_HEADER_UNMARSHALL((addr), &((cme_ble_connection_event_end_ind_ptr)->header)); \
        (cme_ble_connection_event_end_ind_ptr)->handle = CME_BLE_CONNECTION_EVENT_END_IND_HANDLE_GET(addr); \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif /* CME_PRIM_H__ */

