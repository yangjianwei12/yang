/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       media_control_client_handover.c
    \ingroup    media_control_client
    \brief      Media control client Handover functions are defined
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING)

#include <panic.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include "marshal.h"
#include "handover_if.h"
#include "logging.h"
#include "media_control_client_marshal_desc.h"
#include "gatt.h"
#include "mcp_handover.h"
#include "media_control_client_private.h"
#include "gatt_connect.h"

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool unmarshalled = FALSE;

/*! \brief Check if the media client control is ready for handover */
static bool mediaControlClientHandover_Veto(void)
{
    media_control_client_task_data_t *media_ctx = MediaControlClient_GetContext();
    uint8 idx;

    /* Iterate through the media client instances and check if it needs to veto */
    for (idx = 0; idx < MAX_MEDIA_SERVER_SUPPORTED; idx++)
    {
        if (media_ctx->media_client_instance[idx].state == media_client_state_discovery)
        {
            return TRUE;
        }

        if (media_ctx->media_client_instance[idx].state == media_client_state_connected)
        {
            if (McpHandoverVeto(media_ctx->media_client_instance[idx].mcp_profile_handle))
            {
                return TRUE;
            }
        }
    }

    /* Check message queue status */
    if (MessagesPendingForTask(&media_ctx->task_data, NULL) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*! \brief Marshal media control profile data */
static bool mediaControlClientHandover_Marshal(const tp_bdaddr *tp_bd_addr,
                                               uint8 *buf,
                                               uint16 length,
                                               uint16 *written)
{
    bool marshalled;
    media_client_marshal_data_t obj;
    media_control_client_instance_t *instance = NULL;
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    DEBUG_LOG("mediaControlClientHandover_Marshal: Marashalling for addr[0x%06x], gatt_cid: 0x%x", tp_bd_addr->taddr.addr.lap, gatt_cid);

    instance = MediaControlClient_GetInstance(media_client_compare_by_cid, (unsigned)gatt_cid);

    if (instance != NULL)
    {
        obj.gatt_cid = instance->cid;
        obj.state = instance->state;
        obj.server_state = instance->server_state;
        obj.content_id = instance->content_control_id;

        marshaller_t marshaller = MarshalInit(mtd_media_control_client, MEDIA_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
        MarshalSetBuffer(marshaller, (void*)buf, length);
        marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(media_client_marshal_data_t));
        *written = marshalled? MarshalProduced(marshaller) : 0;
        MarshalDestroy(marshaller, FALSE);
        return TRUE;
    }

    *written = 0;
    return TRUE;
}

/*! \brief Unmarshal Media control profile data */
static bool mediaControlClientHandover_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                                 const uint8 *buf,
                                                 uint16 length,
                                                 uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    marshal_type_t unmarshalled_type;
    media_client_marshal_data_t *data = NULL;
    media_control_client_instance_t *instance = NULL;

    DEBUG_LOG("mediaControlClientHandover_Unmarshal for addr %04x %02x %06x",
              tp_bd_addr->taddr.addr.nap,
              tp_bd_addr->taddr.addr.uap,
              tp_bd_addr->taddr.addr.lap);

    unmarshaller_t unmarshaller = UnmarshalInit(mtd_media_control_client, MEDIA_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *)buf, length);

    if (Unmarshal(unmarshaller, (void **)&data, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(media_client_marshal_data_t));
        PanicNull(data);

        instance = MediaControlClient_GetInstance(media_client_compare_by_state, media_client_state_idle);
        DEBUG_LOG("mediaControlClientHandover_Unmarshal instance %p gatt_cid 0x%x", instance, data->gatt_cid);

        PanicNull(instance);

        instance->cid = data->gatt_cid;
        instance->state = data->state;
        instance->server_state = data->server_state;
        instance->content_control_id = data->content_id;

        unmarshalled = TRUE;
        *consumed = UnmarshalConsumed(unmarshaller);
        UnmarshalDestroy(unmarshaller, TRUE);
        return TRUE;
    }
    else
    {
        *consumed = 0;
        DEBUG_LOG("mediaControlClientHandover_Unmarshal: failed unmarshal");
        UnmarshalDestroy(unmarshaller, TRUE);
        return FALSE;
    }
}

/*! \brief Initialize MCP for the new primary */
static void mediaControlClientHandover_HandleCommitForPrimary(unsigned gatt_cid)
{
    media_control_client_instance_t *instance;
    McpInitData client_init_params;
    McpHandles mcp_handle_data;

    DEBUG_LOG("mediaControlClientHandover_HandleCommitForPrimary");

    /* Initialize the MCP */
    instance = MediaControlClient_GetInstance(media_client_compare_by_cid, (unsigned)gatt_cid);

    if (instance != NULL)
    {
        instance->state = media_client_state_discovery;
        instance->handover_in_progress = TRUE;
        memset(&mcp_handle_data, 0, sizeof(McpHandles));
        client_init_params.cid = gatt_cid;
        mcp_handle_data.mcsInstCount = 1;

        mcp_handle_data.mcsHandle = (GattMcsClientDeviceData*) MediaControlClient_RetrieveClientHandles(gatt_cid);
        PanicFalse(mcp_handle_data.mcsHandle != NULL);

        McpInitReq(TrapToOxygenTask((Task)&media_control_taskdata.task_data),
                   &client_init_params,
                   &mcp_handle_data);
    }
}

/*! \brief Handle commit request on new secondary */
static void mediaControlClientHandover_HandleCommitForSecondary(unsigned gatt_cid)
{
    UNUSED(gatt_cid);
    DEBUG_LOG("mediaControlClientHandover_HandleCommitForSecondary");

    /* Dont do anything here. When GATT Disconnects(Link transferred), MCP will send a destroy
     * request. Terminate confirmation will come and will clean up the instance.
     */
}

/*! \brief Handle commit for Media control client */
static void mediaControlClientHandover_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (is_primary)
    {
        mediaControlClientHandover_HandleCommitForPrimary(gatt_cid);
    }
    else
    {
        mediaControlClientHandover_HandleCommitForSecondary(gatt_cid);
    }
}

/*! \brief Handle handover complete for media control client */
static void mediaControlClientHandover_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
    /* mark complete of unmarshalled data */
    unmarshalled = FALSE;
}

/*! \brief On abort, reset the media control client context in the secondary */
static void mediaControlClientHandover_HandoverAbort(void)
{
    uint8 idx;
    media_control_client_task_data_t *media_ctx = MediaControlClient_GetContext();

    DEBUG_LOG("mediaControlClientHandover_HandoverAbort");

    if (unmarshalled)
    {
        unmarshalled = FALSE;

        /* Iterate through the media client instances and reset */
        for (idx = 0; idx < MAX_MEDIA_SERVER_SUPPORTED; idx++)
        {
            MediaControlClient_ResetMediaClientInstance(&media_ctx->media_client_instance[idx]);
        }
    }
}

/*! \brief On abort, Media control client handover interfaces */
const handover_interface media_control_client_handover_if =
        MAKE_BLE_HANDOVER_IF(&mediaControlClientHandover_Veto,
                             &mediaControlClientHandover_Marshal,
                             &mediaControlClientHandover_Unmarshal,
                             &mediaControlClientHandover_HandoverCommit,
                             &mediaControlClientHandover_HandoverComplete,
                             &mediaControlClientHandover_HandoverAbort);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */
