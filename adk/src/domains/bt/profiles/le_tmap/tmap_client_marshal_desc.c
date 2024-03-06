/****************************************************************************
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
    \version    
    \file       tmap_client_marshal_desc.c
    \ingroup    tmap_profile
    \brief      Creates tables of marshal type descriptors for TMAP client data types
*/

#include "tmap_client_marshal_desc.h"

static const marshal_member_descriptor_t mmd_tmap_client_data[] =
{
    MAKE_MARSHAL_MEMBER(tmap_client_marshal_data_t, uint32, gatt_cid),
    MAKE_MARSHAL_MEMBER(tmap_client_marshal_data_t, uint8, state)
};

static const marshal_type_descriptor_t mtd_tmap_client_marshal_data_t =
        MAKE_MARSHAL_TYPE_DEFINITION(tmap_client_marshal_data_t, mmd_tmap_client_data);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &mtd_##type,
const marshal_type_descriptor_t * const  mtd_tmap_client[] =
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    TMAP_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION


