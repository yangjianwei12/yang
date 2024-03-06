/*!
   \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup    gatt_service_discovery
   \brief         Gatt Service Discovery implementation.
   @{
*/

#ifndef GATT_SERVICE_DISCOVERY_PRIVATE_H
#define GATT_SERVICE_DISCOVERY_PRIVATE_H

#ifdef USE_SYNERGY
#include "gatt_service_discovery_lib.h"
#endif

#ifdef USE_SYNERGY
void gattServiceDiscovery_InitLegacyClientsList(const client_id_t* gatt_client_prioritised_id[],
                                                const GattSdSrvcId service_id[],
                                                uint8 num_elements);
#else
void gattServiceDiscovery_Init(const client_id_t* gatt_client_prioritised_id[], uint8 num_elements);
#endif

#endif // GATT_SERVICE_DISCOVERY_PRIVATE_H
/*! @} */