/*! 
    \copyright Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
    \version 
    \file 
    \addtogroup le_unicast_manager
    \brief Creates tables of marshal type descriptors for Unicast manager data types
    @{
*/

#ifndef LE_UNICAST_MANAGER_MARSHAL_DESC_H
#define LE_UNICAST_MANAGER_MARSHAL_DESC_H

#if defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING)

#include "types.h"
#include "app/marshal/marshal_if.h"
#include "le_unicast_manager_private.h"
#include <marshal_common.h>

#define NUMBER_OF_UNICAST_MAANGER_COMMON_MARSHAL_OBJECT_TYPES   (3)

/* Data struture to be marshalled/unmarshalled by unicast manager */
typedef struct
{
    uint8               ase_id[le_um_audio_location_max];
    uint16              audio_context;

    uint32              cid;
} unicast_manager_marshal_data_t;

#define UNICAST_MANAGER_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(unicast_manager_marshal_data_t)

#define UNICAST_MANAGER_COMMON_MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(uint8) \
    ENTRY(uint16) \
    ENTRY(uint32)

/* Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    /* common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_UNICAST_MAANGER_COMMON_MARSHAL_OBJECT_TYPES-1,
    UNICAST_MANAGER_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    UNICAST_MANAGER_MARSHAL_OBJ_TYPE_COUNT
};
#undef EXPAND_AS_ENUMERATION

extern const marshal_type_descriptor_t * const  mtdesc_unicast_mgr[];

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */

#endif /* LE_UNICAST_MANAGER_MARSHAL_DESC_H */

/*! @} */