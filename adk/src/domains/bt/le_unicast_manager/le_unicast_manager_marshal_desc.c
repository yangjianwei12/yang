/*!
    \copyright Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
    \version 
    \file
    \ingroup    le_unicast_manager
    \brief Creates tables of marshal type descriptors for unicast manager data types
*/

#include "le_unicast_manager_marshal_desc.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING)

static const marshal_member_descriptor_t mmd_unicast_manager_data[] =
{
    MAKE_MARSHAL_MEMBER_ARRAY(unicast_manager_marshal_data_t, uint8, ase_id, le_um_audio_location_max),
    MAKE_MARSHAL_MEMBER(unicast_manager_marshal_data_t, uint16, audio_context),
    MAKE_MARSHAL_MEMBER(unicast_manager_marshal_data_t, uint32, cid),
};

static const marshal_type_descriptor_t marshal_type_descriptor_unicast_manager_marshal_data_t =
        MAKE_MARSHAL_TYPE_DEFINITION(unicast_manager_marshal_data_t, mmd_unicast_manager_data);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *) &marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const  mtdesc_unicast_mgr[UNICAST_MANAGER_MARSHAL_OBJ_TYPE_COUNT] =
{
    UNICAST_MANAGER_COMMON_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    UNICAST_MANAGER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */

