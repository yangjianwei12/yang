/****************************************************************************
    \copyright Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
    \version 
    \file
    \addtogroup call_control_client
    \brief Creates tables of marshal type descriptors for call control client data types
    @{
*/

#ifndef CALL_CONTROL_CLIENT_MARSHAL_DESC_H
#define CALL_CONTROL_CLIENT_MARSHAL_DESC_H

#include "marshal_common_desc.h"
#include "app/marshal/marshal_if.h"
#include "call_control_client_private.h"
#include "gatt_telephone_bearer_client.h"

/* Data struture to be marshalled/unmarshalled by call control client */
typedef struct
{
    uint16                        content_id;
    uint32                        gatt_cid;
    gtbs_status_feature_info_t    feature_info;
    call_client_state_t           state;
    ccp_call_info_t               call_state[MAX_ACTIVE_CALLS_SUPPORTED];
} call_client_marshal_data_t;

#define CALL_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(call_client_marshal_data_t) \
    ENTRY(gtbs_status_feature_info_t) \
    ENTRY(ccp_call_info_t)

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    CALL_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    CALL_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_call_control_client[];

#endif /* CALL_CONTROL_CLIENT_MARSHAL_DESC_H */

/*! @} */