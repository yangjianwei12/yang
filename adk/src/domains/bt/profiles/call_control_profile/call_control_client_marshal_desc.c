/****************************************************************************
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
    \version    
    \file
    \ingroup    call_control_client
    \brief      Creates tables of marshal type descriptors for call control client data types
*/

#include "call_control_client_marshal_desc.h"

static const marshal_type_descriptor_t mtd_gtbs_status_feature_info_t =
        MAKE_MARSHAL_TYPE_DEFINITION_BASIC(gtbs_status_feature_info_t);

static const marshal_type_descriptor_t mtd_ccp_call_info_t =
        MAKE_MARSHAL_TYPE_DEFINITION_BASIC(ccp_call_info_t);

static const marshal_member_descriptor_t mmd_call_client_data[] =
{
    MAKE_MARSHAL_MEMBER(call_client_marshal_data_t, uint16, content_id),
    MAKE_MARSHAL_MEMBER(call_client_marshal_data_t, uint32, gatt_cid),
    MAKE_MARSHAL_MEMBER(call_client_marshal_data_t, gtbs_status_feature_info_t, feature_info),
    MAKE_MARSHAL_MEMBER(call_client_marshal_data_t, uint8, state),
    MAKE_MARSHAL_MEMBER_ARRAY(call_client_marshal_data_t, ccp_call_info_t, call_state, MAX_ACTIVE_CALLS_SUPPORTED)
};

static const marshal_type_descriptor_t mtd_call_client_marshal_data_t =
        MAKE_MARSHAL_TYPE_DEFINITION(call_client_marshal_data_t, mmd_call_client_data);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtd_call_control_client[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    CALL_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION
