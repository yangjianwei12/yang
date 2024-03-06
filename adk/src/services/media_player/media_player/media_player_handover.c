/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Media Player handover interface
*/

#ifdef INCLUDE_MIRRORING

#include "media_player_private.h"
#include <service_marshal_types.h>
#include <app_handover_if.h>
#include <bt_device.h>
#include <device_properties.h>
#include <audio_sources.h>
#include <logging.h>
#include <stdlib.h>
#include <panic.h>

/* To minimise the size of marshalled data, due-time for pending messages is stored in uint8 type.
 * The time is quantized in 256-ms steps i.e. 0x1 step means 256 ms, 0xff steps means 65280 ms.
 * All due times are capped at the max of 65280 ms. */
#define DELAY_STEP_SIZE_MILLISECONDS        256
#define MAX_DELAY_MILLISECONDS              (DELAY_STEP_SIZE_MILLISECONDS * 0xFF)
#define MILLISECONDS_TO_STEPS(ms)           (ms / DELAY_STEP_SIZE_MILLISECONDS)
#define STEPS_TO_MILLISECONDS(steps)        (steps * DELAY_STEP_SIZE_MILLISECONDS)

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool mediaPlayer_Veto(void);
static bool mediaPlayer_Marshal(const bdaddr *bd_addr,
                               marshal_type_t type,
                               void **marshal_obj);
static bool mediaPlayer_MarshalLe(const typed_bdaddr *taddr,
                               marshal_type_t type,
                               void **marshal_obj);
static app_unmarshal_status_t mediaPlayer_Unmarshal(const bdaddr *bd_addr,
                                 marshal_type_t type,
                                 void *unmarshal_obj);
static app_unmarshal_status_t mediaPlayer_UnmarshalLe(const typed_bdaddr *taddr,
                                 marshal_type_t type,
                                 void *unmarshal_obj);
static void mediaPlayer_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
const marshal_type_info_t media_player_marshal_types[] = {
    MARSHAL_TYPE_INFO(media_player_task_data_t, MARSHAL_TYPE_CATEGORY_GENERIC)
};
const marshal_type_list_t media_player_marshal_types_list = {media_player_marshal_types, ARRAY_DIM(media_player_marshal_types)};
REGISTER_HANDOVER_INTERFACE(MEDIA_PLAYER, &media_player_marshal_types_list, mediaPlayer_Veto, mediaPlayer_Marshal,
                                                                        mediaPlayer_Unmarshal, mediaPlayer_Commit);
REGISTER_HANDOVER_INTERFACE_LE(MEDIA_PLAYER, &media_player_marshal_types_list, mediaPlayer_Veto, mediaPlayer_MarshalLe,
                               mediaPlayer_UnmarshalLe, mediaPlayer_Commit);
                               
/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*!
    \brief Handle Veto during handover.
    \return TRUE to veto handover, FALSE otherwise.
*/
static bool mediaPlayer_Veto(void)
{
    DEBUG_LOG("mediaPlayer_Veto");
    bool veto = FALSE;
    
    for (uint8 group = 0; group < MEDIA_PLAYER_INTERNAL_MESSAGE_GROUPS_COUNT; group++)
    {
        for (audio_source_t source = audio_source_none; source < max_audio_sources; source++)
        {
            MessageId id_base = mediaPlayerConvertInternalMessageGroupToIdBase(group);
            MessageId id = mediaPlayerConvertAudioSourceToInternalMessageId(source, id_base);
            mediaPlayer_MediaTaskData()->outstanding_timeouts[id] = 0;
            int32 first_due = 0;
            
            if (MessagePendingFirst(mediaPlayer_MediaTask(), id, &first_due))
            {
                uint8 timeout = MAX(1, MILLISECONDS_TO_STEPS(MIN(first_due, MAX_DELAY_MILLISECONDS)));
                mediaPlayer_MediaTaskData()->outstanding_timeouts[id] = timeout;
            }
        }
    }
    
    return veto;
}

/*!
    \brief The function shall set marshal_obj to the address of the object to
           be marshalled.
    \param[in] bd_addr      Bluetooth address of the link to be marshalled.
    \param[in] type         Type of the data to be marshalled.
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.
*/
static bool mediaPlayer_Marshal(const bdaddr *bd_addr,
                               marshal_type_t type,
                               void **marshal_obj)
{
    DEBUG_LOG("mediaPlayer_Marshal");
    UNUSED(bd_addr);
    *marshal_obj = NULL;
    
    switch (type)
    {
        case MARSHAL_TYPE(media_player_task_data_t):
            *marshal_obj = mediaPlayer_MediaTaskData();
            return TRUE;
            
        default:
            break;
    }
    
    return FALSE;
}

static bool mediaPlayer_MarshalLe(const typed_bdaddr *taddr,
                               marshal_type_t type,
                               void **marshal_obj)
{
    return mediaPlayer_Marshal(&taddr->addr, type, marshal_obj);
}

/*!
    \brief The function shall copy the unmarshal_obj associated to specific
            marshal type
    \param[in] bd_addr      Bluetooth address of the link to be unmarshalled.
    \param[in] type         Type of the unmarshalled data.
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return unmarshalling result. Based on this, caller decides whether to free
            the marshalling object or not.
*/
static app_unmarshal_status_t mediaPlayer_Unmarshal(const bdaddr *bd_addr,
                                 marshal_type_t type,
                                 void *unmarshal_obj)
{
    DEBUG_LOG("mediaPlayer_Unmarshal");
    
    UNUSED(bd_addr);
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;
    
    switch (type)
    {
        case MARSHAL_TYPE(media_player_task_data_t):
        {
            media_player_task_data_t *unmarshal_td = (media_player_task_data_t*)unmarshal_obj;
            memcpy(mediaPlayer_MediaTaskData()->outstanding_timeouts, unmarshal_td->outstanding_timeouts,
                                                sizeof(mediaPlayer_MediaTaskData()->outstanding_timeouts));
            result = UNMARSHAL_SUCCESS_FREE_OBJECT;
        }
        break;
        
        default:
            /* Do nothing */
            break;
    }
    
    return result;
}

static app_unmarshal_status_t mediaPlayer_UnmarshalLe(const typed_bdaddr *taddr,
                               marshal_type_t type,
                               void *unmarshal_obj)
{
    return mediaPlayer_Unmarshal(&taddr->addr, type, unmarshal_obj);
}

/*!
    \brief Component commits to the specified role
    The component should take any actions necessary to commit to the
    new role.
    \param[in] is_primary   TRUE if new role is primary, else secondary
*/
static void mediaPlayer_Commit(bool is_primary)
{
    DEBUG_LOG_VERBOSE("mediaPlayer_Commit");
    
    if (is_primary)
    {
        /* Resume all pending timers on the new Primary.
         * Depending on the nature of the timer, this resumption may be meaningful for only "shared" audio sources
         * such as A2DP and LEA -- which are typically the only sources supported by usecases requiring handover. */
        for (uint8 group = 0; group < MEDIA_PLAYER_INTERNAL_MESSAGE_GROUPS_COUNT; group++)
        {
            for (audio_source_t source = audio_source_none; source < max_audio_sources; source++)
            {
                MessageId id_base = mediaPlayerConvertInternalMessageGroupToIdBase(group);
                MessageId id = mediaPlayerConvertAudioSourceToInternalMessageId(source, id_base);
                uint16 timeout = mediaPlayer_MediaTaskData()->outstanding_timeouts[id];
                
                if (timeout)
                {
                    timeout = STEPS_TO_MILLISECONDS(timeout);
                    MessageCancelAll(mediaPlayer_MediaTask(), id);
                    MessageSendLater(mediaPlayer_MediaTask(), id, NULL, timeout);
                    DEBUG_LOG_INFO("mediaPlayer_Commit, enum:media_player_internal_messages:%u for source=enum:audio_source_t:%u in %u ms",
                                                                                        id_base, source, timeout);
                    mediaPlayer_MediaTaskData()->outstanding_timeouts[source] = 0;
                }
            }
        }
    }
    else
    {
        /* Cancel all pending timers on the new Secondary */
        for (uint8 group = 0; group < MEDIA_PLAYER_INTERNAL_MESSAGE_GROUPS_COUNT; group++)
        {
            for (audio_source_t source = audio_source_none; source < max_audio_sources; source++)
            {
                MessageId id_base = mediaPlayerConvertInternalMessageGroupToIdBase(group);
                MessageId id = mediaPlayerConvertAudioSourceToInternalMessageId(source, id_base);
                MessageCancelAll(mediaPlayer_MediaTask(), id);
                mediaPlayer_MediaTaskData()->outstanding_timeouts[id] = 0;
            }
        }
    }
}

#endif /* INCLUDE_MIRRORING */
