/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
    \version    %version
    \file
    \ingroup    generic_broadcast_scan_server
    \brief      Creates tables of marshal type descriptors generic_broadcast_scan_server data types
*/
#ifdef INCLUDE_GBSS
#include "generic_broadcast_scan_server_marshal_desc.h"

static const marshal_member_descriptor_t mmd_generic_broadcast_scan_server_data[] =
{
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint16, gbss_scan_ccc),
    MAKE_MARSHAL_MEMBER_ARRAY(generic_broadcast_scan_server_marshal_data_t, uint16, gbss_rcv_state_ccc, NUM_GBSS_BRS),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint32, cid),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint16, gbss_scan_cp_ccc),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_scan_cp_rsp_opcode),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint16, gbss_scan_cp_rsp_status),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_scan_cp_rsp_param_len),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_scan_cp_rsp_params_source_id),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint32, gbss_scan_cp_rsp_params_broadcast_id),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint16, gbss_volume_state_ccc),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_volume_setting),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_volume_change_counter),
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_volume_mute),
#ifdef ENABLE_RDP_DEMO
    MAKE_MARSHAL_MEMBER(generic_broadcast_scan_server_marshal_data_t, uint8, gbss_src_state_ntf_counter),
#endif
};

static const marshal_type_descriptor_t marshal_type_descriptor_generic_broadcast_scan_server_marshal_data_t =
        MAKE_MARSHAL_TYPE_DEFINITION(generic_broadcast_scan_server_marshal_data_t, mmd_generic_broadcast_scan_server_data);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const  mtdesc_generic_broadcast_scan_server_mgr[GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_OBJ_TYPE_COUNT] =
{
    GENERIC_BROADCAST_SCAN_SERVER_COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION
#endif /* INCLUDE_GBSS */
