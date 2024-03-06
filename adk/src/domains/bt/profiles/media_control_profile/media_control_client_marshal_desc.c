/****************************************************************************
    \copyright Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
    \version 
    \file
    \ingroup media_control_client
    \brief Creates tables of marshal type descriptors for media control client data types
*/

#include "media_control_client_marshal_desc.h"

static const marshal_member_descriptor_t mmd_media_client_data[] =
{
    MAKE_MARSHAL_MEMBER(media_client_marshal_data_t, uint8, content_id),
    MAKE_MARSHAL_MEMBER(media_client_marshal_data_t, uint8, state),
    MAKE_MARSHAL_MEMBER(media_client_marshal_data_t, uint8, server_state),
    MAKE_MARSHAL_MEMBER(media_client_marshal_data_t, uint32, gatt_cid),
};

static const marshal_type_descriptor_t mtd_media_client_marshal_data_t =
        MAKE_MARSHAL_TYPE_DEFINITION(media_client_marshal_data_t, mmd_media_client_data);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtd_media_control_client[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MEDIA_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

