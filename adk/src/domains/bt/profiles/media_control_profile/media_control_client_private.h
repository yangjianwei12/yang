/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup media_control_client
    \brief      Private defines and types for media control profile.
    @{
*/

#ifndef MEDIA_CONTROL_CLIENT_PRIVATE_H
#define MEDIA_CONTROL_CLIENT_PRIVATE_H

#include "mcp.h"
#include "bt_types.h"
#include "gatt_connect.h"
#include "media_control_client.h"

/*! Number of Media Control Servers supported */
#define MAX_MEDIA_SERVER_SUPPORTED GATT_CONNECT_MAX_REMOTE_DEVICES

/*! Media client instance state */
typedef enum
{
    /*! Media client instance in idle/free state */
    media_client_state_idle,

    /*! Discovery in progress state */
    media_client_state_discovery,

    /*! Media client in connected state */
    media_client_state_connected
} media_client_state_t;

/*! Media control Server - Media State in remote server */
typedef enum
{
    /*! Remote Media state inactive*/
    media_server_state_inactive,

    /*! Media is playing in the remote server */
    media_server_state_playing,

    /*! Media is paused in the remote server */
    media_server_state_paused,

    /*! Media is being seeked(fast forward/backward) in remote server */
    media_server_state_seeked
} media_server_state_t;

/*! Media control Client Instance Information */
typedef struct
{
    /*! Connection Identifier for this Media client instance */
    gatt_cid_t            cid;

    /*! Media Control Profile Handle */
    McpProfileHandle      mcp_profile_handle;

    /*! Media Control Service Handle */
    ServiceHandle         mcs_service_handle;

    /* Content control Id */
    uint8 content_control_id;

    /*! Instance present state */
    media_client_state_t  state;

    /*! Media server state */
    media_server_state_t  server_state;

    /*! Is a handover in progress? */
    bool                  handover_in_progress;
} media_control_client_instance_t;

/*! Media client instance state */
typedef enum
{
    /*! Get Media Client instance based on connection identifier */
    media_client_compare_by_cid,

    /*! Get Media Client instance based on profile handle */
    media_client_compare_by_profile,

    /*! Get Media Client instance by state */
    media_client_compare_by_state,

    /*! Get Media Client instance by bdaddr */
    media_client_compare_by_bdaddr
} media_instance_compare_by_type_t;

/*! Linked list for clients of MCP state change indications */
typedef struct _media_control_mcp_state_client_t
{
    media_control_client_callback_if callback_if;
    struct _media_control_mcp_state_client_t * next;

}media_control_mcp_state_client_t;

/*! \brief Media control client context. */
typedef struct
{
    /*! Media control profile task */
    TaskData task_data;

    /*! Media control client instance */
    media_control_client_instance_t media_client_instance[MAX_MEDIA_SERVER_SUPPORTED];

    /*! List of clients registered to receive MCP state change indictaions */
    media_control_mcp_state_client_t * clients;
} media_control_client_task_data_t;

/*! \brief Media control client opcode. */
typedef struct
{
    /*! Media control opcode */
    GattMcsOpcode op;

    /*! value associated with opcode */
    int32 val;
} media_control_set_t;

/*! \brief Media control client - Media Player Attribute */
typedef struct
{
    /*! Media player attribute */
    MediaPlayerAttribute attrib;

    /*! Length associated with attribute */
    uint16 len;

    /*! value associated with attribute */
    uint8 *val;
} media_control_attrib_t;

/*! \brief Media control client notification register request */
typedef struct
{
    /*! Media player attributeBitmask of MCS characteristics  */
    MediaPlayerAttribute attrib;

    /*! Bitmask to enable/disable respective characteristics CCCD */
    uint32 notif_value;

} media_control_notification_req_t;

/*! \brief Media control client task Data */
extern media_control_client_task_data_t media_control_taskdata;

/*! \brief Returns the media control client context */
#define MediaControlClient_GetContext()         (&media_control_taskdata)

/*! \brief Method used to store discovered GMCS handle data to NVM */
bool MediaControlClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size);

/*! \brief Method used to retrieve GMCS handle data from NVM */
void * MediaControlClient_RetrieveClientHandles(gatt_cid_t cid);

/*! \brief Get the Media client instance based on the compare type */
media_control_client_instance_t * MediaControlClient_GetInstance(media_instance_compare_by_type_t type, unsigned cmp_value);

/*! \brief Reset the provided media client instance */
void MediaControlClient_ResetMediaClientInstance(media_control_client_instance_t *media_client);

#endif /* MEDIA_CONTROL_CLIENT_PRIVATE_H */

/*! @} */
