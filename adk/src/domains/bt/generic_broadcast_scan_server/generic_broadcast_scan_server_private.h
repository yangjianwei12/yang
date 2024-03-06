/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \addtogroup generic_broadcast_scan_server
    \brief      Header file for the Gaia Broadcast Scan Server module.
    @{

*/
#ifndef GENERIC_GAIA_BROADCAST_SCAN_SERVER_PRIVATE_H_
#define GENERIC_GAIA_BROADCAST_SCAN_SERVER_PRIVATE_H_

#ifdef INCLUDE_GBSS
#include "generic_broadcast_scan_server.h"

#include "gatt_connect.h"
#include "gatt_handler_db.h"
#include "csr_bt_gatt_prim.h"

#include "logging.h"
#include "bt_types.h"

/* The code under this flag is intended only for RDP demo */
#define ENABLE_RDP_DEMO

#define MAX_CONNECTIONS (2)
#define INVALID_CID_INDEX (0xFFu)

#define GBSS_CHARACTERISTICS (2)

#define START_SCANNING_OPCODE_SIZE_MIN (6)
#define STOP_SCANNING_OPCODE_SIZE (1)
#define ADD_SOURCE_OPCODE_SIZE_MIN (20)
#define MODIFY_SOURCE_OPCODE_SIZE_MIN (10)
#define SET_BROADCAST_CODE_OPCODE_SIZE (17)
#define REMOVE_SOURCE_OPCODE_SIZE (1)

#define GENERIC_BROADCAST_SCAN_SERVICE_CCC_VALUE_SIZE (2)

/* BASS always gives fixed source ids for the characteristics */
#define SOURCE_ID_TO_RECEIVER_STATE_IDX(source_id)   (source_id - 1)
#define RECEIVER_STATE_IDX_TO_SOURCE_ID(idx)         (idx + 1)

enum GBSS_RECEIVER_STATE_IDX
{
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_1)
    GBSS_RECEIVER_STATE_IDX_1    = 0,
#endif
#if defined(HANDLE_GENERIC_BROADCAST_RECEIVE_STATE_2)
    GBSS_RECEIVER_STATE_IDX_2    = 1,
#endif
    GBSS_RECEIVER_STATE_IDX_MAX
};

enum GBSS_OPCODE
{
    START_SCANNING_OPCODE,
    STOP_SCANNING_OPCODE,
    ADD_SOURCE_OPCODE,
    MODIFY_SOURCE_OPCODE,
    SET_BROADCAST_CODE_OPCODE,
    REMOVE_SOURCE_OPCODE,
    RESET_OPCODE
};

enum GBSS_VOLUME_OPCODE
{
    RELATIVE_VOLUME_DOWN_OPCODE,
    RELATIVE_VOLUME_UP_OPCODE,
    SET_ABSOLUTE_VOLUME_OPCODE = 0x04
};

typedef enum {
    GBSS_SCAN_INACTIVE = 0,
    GBSS_SCAN_ACTIVE,
    GBSS_SCAN_IND
} gbss_scan_active_t;

typedef enum {
    SCANNING_SOURCE_FOUND = 0,
    /*This error codes match le_broadcast_manager_self_scan_status_t*/
    SCANNING_FAILED,
    SCANNING_BAD_PARAMETERS,
    SCANNING_TIMEOUT,
    SCANNING_IN_PROGRESS,
    SCANNING_STOPPED,

    SCANNING_NO_SOURCE =0xff
} gbss_self_scan_status_t;


typedef enum
{
    GBSS_BASS_STATUS_SUCCESS,
    GBSS_BASS_STATUS_IN_PROGRESS,
    GBSS_BASS_STATUS_INVALID_PARAMETER,
    GBSS_BASS_STATUS_NOT_ALLOWED,
    GBSS_BASS_STATUS_FAILED,
    GBSS_BASS_STATUS_BC_SOURCE_IN_SYNC,
    GBSS_BASS_STATUS_INVALID_SOURCE_ID,
} gbss_bass_status_t;

typedef struct
{
    uint8 source_id;
    uint32 broadcast_id; 
}gbss_scan_control_point_response_params_t;

typedef struct
{
    uint8 opcode;
    uint16 status_code;
    uint8 param_len;
    gbss_scan_control_point_response_params_t params;
} gbss_scan_control_point_response_t;

typedef struct
{
    connection_id_t cid;
    gbss_config_t client_cfg;
    gbss_scan_control_point_response_t scan_cp_response;
} gbss_client_data_t;

typedef struct
{
    uint8 metadata_length;
    uint8 *metadata;
} gbss_subgroups_t;

/*! Structure holding information for the application handling of GENERIC Broadcast Scan Server */
typedef struct
{
    gbss_scan_active_t scan_active;
    gbss_self_scan_status_t scan_result;
    uint8 encryption_required;
    int8 rssi;
    uint8 broadcast_name_len;
    uint8 *broadcast_name;
    typed_bdaddr source_address;
    uint8 source_adv_sid;
    uint32 broadcast_id;
    uint16 pa_interval;
    uint8 num_subgroups;
    gbss_subgroups_t * subgroups;
} gbss_scan_report_t;

typedef struct
{
    uint8 volume_setting;
    uint8 change_counter;
    uint8 mute;
    uint8 step_size;
} gbss_volume_data_t;

/*! Structure holding information for the application handling of GENERIC Broadcast Scan Server */
typedef struct
{
    /*! Task for handling GENERIC Broadcast Scan Server related messages */
    TaskData gbss_task;

    /*! Unique GATT id provided by GATT lib */
    CsrBtGattId gattId;

    /*! client ID for broadcast manager BASS receiver state notify interface */
    uint32 client_id;

    /*! If MAX_CONNECTIONS is significantly increased, adding num of active 
    connections can save us time. For the current size it is not meaningful */
    gbss_client_data_t connected_clients[MAX_CONNECTIONS];

    gbss_scan_report_t gbss_report;

    gbss_volume_data_t gbss_volume_data;

#ifdef ENABLE_RDP_DEMO
    uint8 src_state_ntf_counter;
#endif
} gbss_srv_data_t;

gbss_client_data_t *genericBroadcastScanServer_AddConnection(connection_id_t cid);
gbss_client_data_t *genericBroadcastScanServer_FindConnection(connection_id_t cid);
uint8 *genericBroadcastScanServer_PrepareBroadcastScanControlPointValue(gbss_scan_control_point_response_t *scan_cp_response, uint8 *len);
uint8 *genericBroadcastScanServer_PrepareBroadcastScanReportValue(gbss_scan_report_t *gbss_report, uint8 *len);
uint8 *genericBroadcastScanServer_PrepareBroadcastReceiverStateValue(uint8 *len, uint8 idx);
gbss_srv_data_t *genericBroadcastScanServer_GetInstance(void); 

#endif /* INCLUDE_GBSS */
#endif /* GENERIC_GAIA_BROADCAST_SCAN_SERVER_PRIVATE_H_ */

/*! @} */