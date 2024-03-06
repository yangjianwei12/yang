/*!
    \copyright Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
    \version 
    \file 
    \addtogroup media_control_client
    \brief Creates tables of marshal type descriptors for media control client data types
    @{
*/

#ifndef MEDIA_CONTROL_CLIENT_MARSHAL_DESC_H
#define MEDIA_CONTROL_CLIENT_MARSHAL_DESC_H

#include "marshal_common_desc.h"
#include "gatt_mcs_client.h"

/*! \brief Data struture to be marshalled/unmarshalled by media control client */
typedef struct
{
    uint8                         content_id;
    uint8                         state;
    uint8                         server_state;
    uint32                        gatt_cid;
} media_client_marshal_data_t;

#define MEDIA_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(media_client_marshal_data_t) 

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    MEDIA_CONTROL_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    MEDIA_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_media_control_client[];

#endif /* MEDIA_CONTROL_CLIENT_MARSHAL_DESC_H */

/*! @} */