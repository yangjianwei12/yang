/*!
    \copyright Copyright (c) 2023 Qualcomm Technologies International, Ltd.
    \version %version
    \file 
    \addtogroup generic_broadcast_scan_server
    \brief Creates tables of marshal type descriptors for generic_broadcast_scan_server data types
    @{
*/
#ifdef INCLUDE_GBSS
#ifndef GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_DESC_H
#define GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_DESC_H

#include "app/marshal/marshal_if.h"
#include <marshal_common.h>
#include "generic_broadcast_scan_server.h"
#include "generic_broadcast_scan_server_private.h"

#define NUMBER_OF_GENERIC_BROADCAST_SCAN_SERVER_COMMON_OBJECT_TYPES   (3)

/* Data struture to be marshalled/unmarshalled by GATT server */
typedef struct
{
    uint16 gbss_scan_ccc;
    uint16 gbss_rcv_state_ccc[NUM_GBSS_BRS];
    uint32 cid;
    uint16 gbss_scan_cp_ccc;
    uint8 gbss_scan_cp_rsp_opcode;
    uint16 gbss_scan_cp_rsp_status;
    uint8 gbss_scan_cp_rsp_param_len;
    uint8 gbss_scan_cp_rsp_params_source_id;
    uint32 gbss_scan_cp_rsp_params_broadcast_id;
    uint16 gbss_volume_state_ccc;
    uint8 gbss_volume_setting;
    uint8 gbss_volume_change_counter;
    uint8 gbss_volume_mute;
#ifdef ENABLE_RDP_DEMO
    uint8 gbss_src_state_ntf_counter;
#endif
}generic_broadcast_scan_server_marshal_data_t;

#define GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(generic_broadcast_scan_server_marshal_data_t)

#define GENERIC_BROADCAST_SCAN_SERVER_COMMON_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(uint8) \
    ENTRY(uint16) \
    ENTRY(uint32)

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    /*! common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_GENERIC_BROADCAST_SCAN_SERVER_COMMON_OBJECT_TYPES-1,
    GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtdesc_generic_broadcast_scan_server_mgr[];

#endif /*GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_DESC_H */
#endif /* INCLUDE_GBSS */

/*! @} */