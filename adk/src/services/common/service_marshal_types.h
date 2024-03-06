/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       service_marshal_types.h
    \defgroup   common_services Common
    @{
        \ingroup    services
        \brief      Types used by all services and all its clients.
*/
#ifndef SERVICE_MARSHAL_TYPES_H
#define SERVICE_MARSHAL_TYPES_H

#include "marshal_common.h"
#include "domain_marshal_types.h"
#include "state_proxy_handover.h"
#include "media_player_marshal_typedef.h"
#include <hydra_macros.h>



/*! \brief Use xmacro to expand type table as enumeration of marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum
{
    LAST_DOMAIN_MARSHAL_TYPE = NUMBER_OF_DOMAIN_MARSHAL_OBJECT_TYPES-1, /* Subtracting 1 to keep the marshal types contiguous */
    STATE_PROXY_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    MEDIA_PLAYER_MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_SERVICE_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION


#endif /* SERVICE_MARSHAL_TYPES_H */

/*! @} */