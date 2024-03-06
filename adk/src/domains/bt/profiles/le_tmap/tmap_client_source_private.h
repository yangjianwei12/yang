/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines and types for TMAP source
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_PRIVATE_H
#define TMAP_CLIENT_SOURCE_PRIVATE_H

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include <logging.h>
#include <panic.h>
#include "bt_types.h"
#include "pddu_map.h"
#include "device.h"
#include "gatt_connect.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt.h"
#include "tmap_client_lib.h"
#include "tmap_client_source.h"
#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include "tmap_client_source_broadcast_private.h"
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "tmap_client_source_unicast_private.h"
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! Number of devices supported within a group */
#define MAX_TMAP_DEVICES_SUPPORTED      (2)

/*! Tmap client instance state */
typedef enum
{
    /*! tmap client instance in idle/free state */
    tmap_client_source_state_idle,

    /*! Discovery in progress state */
    tmap_client_source_state_discovery,

    /*! tmap client in connected state */
    tmap_client_source_state_connected,

    /*! tmap client in disconnecting state */
    tmap_client_source_state_disconnecting,
} tmap_client_source_state_t;

/*! Tmap Client Instance Information */
typedef struct
{
    /*! Connection Identifier for this Tmap client instance */
    gatt_cid_t                  cid;

    /*! Instance present state */
    tmap_client_source_state_t  state;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    /*! TMAP Media Config */
    tmap_media_config_t         spkr_audio_path;

    /*! TMAP Mic Config */
    tmap_microphone_config_t    mic_audio_path;
#endif
} tmap_client_source_instance_t;

/*! Tmap client instance state */
typedef enum
{
    /*! Get Call Client instance based on connection identifier */
    tmap_client_source_compare_by_cid,

    /*! Get Call Client instance based on profile handle */
    tmap_client_source_compare_by_profile_handle,

    /*! Get Tmap Client instance by state */
    tmap_client_source_compare_by_state,

    /*! Get Tmap Client instance by valid/invalid cid */
    tmap_client_source_compare_by_valid_invalid_cid,

    /*! Get Tmap Client instance by group ID */
    tmap_client_source_compare_by_group_id
} tmap_source_instance_compare_by_type_t;

/*! Tmap Client group instance information */
typedef struct
{
    /*! Group Handle */
    ServiceHandle                     cap_group_handle;

    /*! TMAP Client Handle */
    TmapClientProfileHandle           tmap_profile_handle;

    /*! Audio context currently active */
    uint16                            audio_context;

    /*! Tmap client instances */
    tmap_client_source_instance_t     tmap_client_instance[MAX_TMAP_DEVICES_SUPPORTED];
} tmap_client_source_group_instance_t;

/*! \brief Tmap client context. */
typedef struct
{
    /*! Tmap profile task */
    TaskData                              task_data;

    /*! TMAP Client profile callback handler */
    tmap_client_source_callback_handler_t callback_handler;

    /*! Tmap client instance */
    tmap_client_source_group_instance_t   group_instance;

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
    /* Unicast related data */
    tmap_src_unicast_task_data_t          unicast_data;
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    /* Broadcast related data */
    tmap_src_bcast_task_data_t            broadcast_data;
#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
} tmap_client_source_task_data_t;

/*! \brief TMAP client source task data. */
extern tmap_client_source_task_data_t tmap_client_source_taskdata;

/* Invalid CAP group ID */
#define TMAP_CLIENT_SOURCE_INVALID_GROUP_ID    ((ServiceHandle) (0x0000))

/*! Returns the tmap client context */
#define TmapClientSource_GetContext()          (&tmap_client_source_taskdata)

/*! Returns the tmap client context */
#define TmapClientSource_GetGroupInstance()    (&tmap_client_source_taskdata.group_instance)

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! Sets iso handle for speaker audio path */
#define tmapClientSource_SetSpeakerIsoHandle(instance, handle, handle_right) \
{\
    instance->spkr_audio_path.source_iso_handle = handle; \
    instance->spkr_audio_path.source_iso_handle_right = handle_right; \
}

/*! Sets iso handle for mic audio path */
#define tmapClientSource_SetMicIsoHandle(instance, handle, handle_right) \
{\
    instance->mic_audio_path.source_iso_handle = handle; \
    instance->mic_audio_path.source_iso_handle_right = handle_right; \
}
#else
#define tmapClientSource_SetSpeakerIsoHandle(instance, handle, handle_right) (UNUSED(instance))
#define tmapClientSource_SetMicIsoHandle(instance, handle, handle_right) (UNUSED(instance))
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

/*! \brief Handler that process the generic tmap messages */
void tmapClientSource_HandleTmapProfileMessage(Message message);

/*! \brief Get the Tmap client instance based on the compare type */
tmap_client_source_instance_t * TmapClientSource_GetInstance(tmap_source_instance_compare_by_type_t type, unsigned cmp_value);

/*! \brief Method used to retrieve discovered TMAS handles data from NVM */
void * TmapClientSource_RetrieveClientHandles(gatt_cid_t cid);

/*! \brief Method used to store discovered TMAS handles data to NVM */
bool TmapClientSource_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size);

/*! \brief Reset the provided tmap client instance */
void TmapClientSource_ResetTmapClientInstance(tmap_client_source_instance_t *tmap_client);

/*! \brief Reset the provided tmap group instance */
void TmapClient_ResetTmapGroupInstance(tmap_client_source_group_instance_t *tmap_group_instance);

/*! \brief Return the tmap profile handle */
TmapClientProfileHandle TmapClientSource_GetProfileHandle(void);

/*! \brief Common handler that receives all message from TMAP library */
void tmapClientSourceMessageHandler_HandleMessage(Task task, MessageId id, Message message);

/*! \brief Returns TRUE if PTS Mode is enabled */
bool TmapClientSource_IsInPtsMode(void);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */
#endif /* TMAP_CLIENT_SOURCE_PRIVATE_H */
/*! @} */