/****************************************************************************
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
    \version    
    \file       tmap_client_marshal_desc.h
    \addtogroup tmap_profile
    \brief      Creates tables of marshal type descriptors for TMAP client data types
    @{
*/

#ifndef TMAP_CLIENT_MARSHAL_DESC_H
#define TMAP_CLIENT_MARSHAL_DESC_H

#include "marshal_common_desc.h"
#include "gatt_tmas_client.h"

/* Data struture to be marshalled/unmarshalled by tmap client */
typedef struct
{
    uint32                        gatt_cid;
    uint8                         state;
} tmap_client_marshal_data_t;

#define TMAP_CLIENT_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(tmap_client_marshal_data_t) 

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    COMMON_DYN_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    TMAP_CLIENT_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    TMAP_CLIENT_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtd_tmap_client[];

#endif /* TMAP_CLIENT_MARSHAL_DESC_H */


/*! @} */