/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines and types for TMAP profile sink
    @{
*/

#ifndef TMAP_CLIENT_SINK_PRIVATE_H_
#define TMAP_CLIENT_SINK_PRIVATE_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)

#include <logging.h>
#include <panic.h>
#include "bt_types.h"
#include "pddu_map.h"
#include "device.h"
#include "gatt_connect.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt.h"
#include "gatt_service_discovery_lib.h"
#include "gatt_service_discovery.h"
#include "tmap_client_lib.h"
#include "tmap_client_sink.h"


/*! Number of TMAP clients supported */
#define MAX_TMAP_CLIENT_SUPPORTED   GATT_CONNECT_MAX_REMOTE_DEVICES

/*! Tmap client instance state */
typedef enum
{
    /*! tmap client instance in idle/free state */
    tmap_client_state_idle,

    /*! Discovery in progress state */
    tmap_client_state_discovery,

    /*! tmap client in connected state */
    tmap_client_state_connected
} tmap_client_state_t;

/*! Tmap Client Instance Information */
typedef struct
{
    /*! Connection Identifier for this Tmap client instance */
    gatt_cid_t                  cid;

    /*! TMAP Client Handle */
    TmapClientProfileHandle           tmap_profile_handle;

    /*! Instance present state */
    tmap_client_state_t         state;

    /*! Audio context currently active */
    uint16  audio_context;

    /*! Is a handover in progress? */
    bool     handover_in_progress;

} tmap_client_instance_t;


/*! Tmap client instance state */
typedef enum
{
    /*! Get Call Client instance based on connection identifier */
    tmap_client_compare_by_cid,

    /*! Get Call Client instance based on profile handle */
    tmap_client_compare_by_profile_handle,

    /*! Get Tmap Client instance by state */
    tmap_client_compare_by_state,

    /*! Get Tmap Client instance by bdaddr */
    tmap_client_compare_by_bdaddr,

    /*! Get Tmap Client instance by valid/invalid cid */
    tmap_client_compare_by_valid_invalid_cid,
} tmap_instance_compare_by_type_t;

/*! \brief Tmap client context. */
typedef struct
{
    /*! Tmap profile task */
    TaskData task_data;

    /*! Tmap client instance */
    tmap_client_instance_t tmap_client_instance[MAX_TMAP_CLIENT_SUPPORTED];
} tmap_client_task_data_t;

/*! Tmap client task Data */
extern tmap_client_task_data_t tmap_taskdata;

/*! Returns the tmap client context */
#define TmapClientSink_GetContext()         (&tmap_taskdata)

/*! \brief Get the Tmap client instance based on the compare type */
tmap_client_instance_t * TmapClientSink_GetInstance(tmap_instance_compare_by_type_t type, unsigned cmp_value);

/*! \brief Method used to retrieve discovered TMAS handles data from NVM */
void * TmapClientSink_RetrieveClientHandles(gatt_cid_t cid);

/*! \brief Method used to store discovered TMAS handles data to NVM */
bool TmapClientSink_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size);

/*! \brief Reset the provided tmap client sink instance */
void TmapClientSink_ResetTmapClientInstance(tmap_client_instance_t *tmap_client);

/*! \brief Create the TMAP sink instance */
bool TmapClientSink_CreateInstance(gatt_cid_t cid);

/*! \brief Destroy the TMAP sink instance */
bool TmapClientSink_DestroyInstance(gatt_cid_t cid);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST) */
#endif /* TMAP_CLIENT_SINK_PRIVATE_H_ */

/*! @} */