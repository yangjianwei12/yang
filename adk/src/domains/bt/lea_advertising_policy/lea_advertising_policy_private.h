/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lea_advertising_policy_private.h
\brief      Header file for LEA Advertising Policy Private definitions
*/

#include "lea_advertising_policy.h"
#include "le_advertising_manager.h"

/*! LEA adv policy client observer list, add new client befor lea_adv_client_observer_max */
typedef enum
{
    lea_adv_client_observer_csip = 0,
    lea_adv_client_observer_cap,
    lea_adv_client_observer_bap,
    lea_adv_client_observer_tmap,
    lea_adv_client_observer_bass,
    lea_adv_client_observer_vcs,
    lea_adv_client_observer_reserved,
    lea_adv_client_observer_max
} lea_adv_client_observer_t;

#define MAX_NUMBER_LEA_CLIENT_OBSERVERS     (lea_adv_client_observer_max)

/*! Registry to maintain all the registered LEA GATT servers */
typedef struct
{
    const lea_adv_policy_clients_callback_t *callbacks[MAX_NUMBER_LEA_CLIENT_OBSERVERS];
    unsigned registered_client_count;
} lea_adv_policy_client_registry_t;

/*! Type that stores configuration information about an advertising mode */
typedef struct
{
    /*! Announcement type associated with this config */
    lea_adv_policy_announcement_type_t announcement_type;

    /*! Minimum Advertising Interval */
    uint16 advertising_interval_min;

    /*! Maximum Advertising Interval */
    uint16 advertising_interval_max;

    /*! Holds the advertising data */
    le_adv_item_data_t adv_data;

    /*! Registered handle for adverts */
    le_adv_item_handle registered_adv_handle;

    /*! Audio context masks for Sink and Source available context */
    uint32 audio_context;
} lea_adv_policy_config_data_t;

/*! LEA Advertising policy taskdata */
typedef struct
{
    /*! Holds the undirected advertising configuration information */
    lea_adv_policy_config_data_t    undirected_advert_config;

    /*! Holds the directed advertising configuration information */
    lea_adv_policy_config_data_t    directed_advert_config;

    /*! Peer Address(used for directed advertising) */
    typed_bdaddr peer_bd_addr;

    /*! Registry to maintain all the registered LEA GATT servers */
    lea_adv_policy_client_registry_t lea_client_registry;
} lea_adv_policy_task_data_t;

/*! LEA Advertising Policy taskdata */
extern lea_adv_policy_task_data_t lea_advert_policy_taskdata;

#define leaAdvertisingPolicy_SetAudioContext(sink, source)             ((source << 16) & 0xFFFF0000) | (sink & 0x0000FFFF)

#define leaAdvertisingPolicy_GetDirectedAudioContext()                (lea_advert_policy_taskdata.directed_advert_config.audio_context)
#define leaAdvertisingPolicy_SetDirectedAudioContext(sink, source)    (lea_advert_policy_taskdata.directed_advert_config.audio_context = leaAdvertisingPolicy_SetAudioContext(sink, source))

#define leaAdvertisingPolicy_GetUndirectedAudioContext()              (lea_advert_policy_taskdata.undirected_advert_config.audio_context)
#define leaAdvertisingPolicy_SetUndirectedAudioContext(sink, source)  (lea_advert_policy_taskdata.undirected_advert_config.audio_context = leaAdvertisingPolicy_SetAudioContext(sink, source))


#define leaAdvertisingPolicy_GetMinAdvIntervalForDirected()      (lea_advert_policy_taskdata.directed_advert_config.advertising_interval_min)
#define leaAdvertisingPolicy_GetMaxAdvIntervalForDirected()      (lea_advert_policy_taskdata.directed_advert_config.advertising_interval_max)
#define leaAdvertisingPolicy_SetMinAdvIntervalForDirected(val)   (lea_advert_policy_taskdata.directed_advert_config.advertising_interval_min = val)
#define leaAdvertisingPolicy_SetMaxAdvIntervalForDirected(val)   (lea_advert_policy_taskdata.directed_advert_config.advertising_interval_max = val)

#define leaAdvertisingPolicy_GetMinAdvIntervalForUndirected()      (lea_advert_policy_taskdata.undirected_advert_config.advertising_interval_min)
#define leaAdvertisingPolicy_GetMaxAdvIntervalForUndirected()      (lea_advert_policy_taskdata.undirected_advert_config.advertising_interval_max)
#define leaAdvertisingPolicy_SetMinAdvIntervalForUndirected(val)   (lea_advert_policy_taskdata.undirected_advert_config.advertising_interval_min = val)
#define leaAdvertisingPolicy_SetMaxAdvIntervalForUndirected(val)   (lea_advert_policy_taskdata.undirected_advert_config.advertising_interval_max = val)

#define leaAdvertisingPolicy_SetUndirectedAnnouncementType(type)   (lea_advert_policy_taskdata.undirected_advert_config.announcement_type = type)
#define leaAdvertisingPolicy_GetUndirectedAnnouncementType()       (lea_advert_policy_taskdata.undirected_advert_config.announcement_type)

#define leaAdvertisingPolicy_SetDirectedAnnouncementType(type)     (lea_advert_policy_taskdata.directed_advert_config.announcement_type = type)
#define leaAdvertisingPolicy_GetDirectedAnnouncementType()         (lea_advert_policy_taskdata.directed_advert_config.announcement_type)

