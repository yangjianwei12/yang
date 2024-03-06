/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file

\brief      Table containing the availalbe list implementations
*/

#include "list_interface.h"

extern const list_interface_t list_linked_single_if;
extern const list_interface_t list_array_if;

#include <logging.h>

const list_interface_t* ListGetInterface(const list_config_t *config)
{
    const list_interface_t* interface = NULL;

    switch(config->type)
    {
        case list_type_linked_single:
        {
            interface = &list_linked_single_if;
        }
        break;

        case list_type_array:
        {
            if ((config->element_store.size <= 0) && (config->element_store.type != list_store_reference))
            {
                DEBUG_LOG_PANIC("%s: Unsupported storage type for array lists enum:list_store_t:%d", __func__, config->element_store.type);
            }
            interface = &list_array_if;
        }
        break;
        default:
            DEBUG_LOG_PANIC("%s: Unsupported list type enum:list_type_t:%d", __func__, config->type);
    }
    return interface;
}
