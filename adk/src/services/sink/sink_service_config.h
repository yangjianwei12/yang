/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup sink_service
    \brief      Sink service config header
    @{
*/

#ifndef SINK_SERVICE_CONFIG_H
#define SINK_SERVICE_CONFIG_H

#include "rssi_pairing.h"
#include "sink_service.h"

/*! \brief Get the sink service configuration.

    \return const pointer to the config defined in the application
*/
static inline const sink_service_config_t * SinkServiceConfig_GetConfig(void)
{
    return &sink_service_config;
}

/*! \brief Maximum permitted number of BR/EDR ACL connections

    \return max number of configured bredr connections.
*/
static inline uint8 sinkService_BredrAclMaxConnections(void)
{
    return sink_service_config.max_bredr_connections;
}

#ifdef ENABLE_LE_SINK_SERVICE
/*! \brief Maximum permitted number of BR/EDR ACL connections

    \return max number of configured bredr connections.
*/
static inline uint8 sinkService_LeAclMaxConnections(void)
{
    return sink_service_config.max_le_connections;
}
#endif /* ENABLE_LE_SINK_SERVICE */

#endif /* SINK_SERVICE_CONFIG_H */

/*! @} */