/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       
    \ingroup    call_control_client
    \addtogroup call_control_client
    \brief      Callcontrol client Handover functions are defined
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
#include "call_control_client_marshal_desc.h"
#include "gatt.h"
#include "ccp_handover.h"
#include "call_control_client_private.h"
#include "gatt_telephone_bearer_client.h"
#include "gatt_connect.h"

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool unmarshalled = FALSE;

/*! \brief Check if the call client control is ready for handover */
static bool callControlClientHandover_Veto(void)
{
    call_control_client_task_data_t *call_ctx = CallControlClient_GetContext();
    uint8 instance_idx;
    uint8 call_idx;
    ccp_call_state_t call_state;

    /* Iterate through the call client instances and check if it needs to veto */
    for (instance_idx = 0; instance_idx < MAX_CALL_SERVER_SUPPORTED; instance_idx++)
    {
        if (call_ctx->call_client_instance[instance_idx].state == call_client_state_discovery)
        {
            return TRUE;
        }

        if (call_ctx->call_client_instance[instance_idx].state == call_client_state_connected)
        {
            if (CcpHandoverVeto(call_ctx->call_client_instance[instance_idx].ccp_profile_handle))
            {
                return TRUE;
            }
        }

        /* Veto if any of the call state is in 'incoming' OR 'outgoing' state */
        for (call_idx = 0; call_idx < MAX_ACTIVE_CALLS_SUPPORTED; call_idx++) 
        {
            call_state = call_ctx->call_client_instance[instance_idx].call_state[call_idx].state;

            if (call_state == CCP_CALL_STATE_INCOMING ||
                call_state == CCP_CALL_STATE_OUTGOING_ALERTING ||
                call_state == CCP_CALL_STATE_OUTGOING_DIALING)
            {
                return TRUE;
            }
        }
    }

    /* Check message queue status */
    if (MessagesPendingForTask(CallControlClient_GetTask(), NULL) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*! \brief Marshal call control profile data */
static bool callControlClientHandover_Marshal(const tp_bdaddr *tp_bd_addr,
                                              uint8 *buf,
                                              uint16 length,
                                              uint16 *written)
{
    bool marshalled;
    call_client_marshal_data_t obj;
    call_control_client_instance_t *instance = NULL;
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)gatt_cid);

    DEBUG_LOG("callControlClientHandover_Marshal: Marshalling for addr[0x%06x], instance: %p, gatt_cid: 0x%x", tp_bd_addr->taddr.addr.lap, instance, gatt_cid);

    if (instance != NULL)
    {
        obj.gatt_cid = instance->cid;
        obj.feature_info = instance->tbs_status_info;
        obj.content_id = instance->content_control_id;
        obj.state = instance->state;
        memcpy(&obj.call_state, &instance->call_state, sizeof(ccp_call_info_t) * MAX_ACTIVE_CALLS_SUPPORTED);

        marshaller_t marshaller = MarshalInit(mtd_call_control_client, CALL_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
        MarshalSetBuffer(marshaller, (void*)buf, length);
        marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(call_client_marshal_data_t));
        DEBUG_LOG("callControlClientHandover_Marshal marshalled %d", marshalled);
        *written = marshalled? MarshalProduced(marshaller) : 0;
        MarshalDestroy(marshaller, FALSE);
        return TRUE;
    }

    *written = 0;
    return TRUE;
}

/*! \brief Unmarshal call control profile data */
static bool callControlClientHandover_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                                const uint8 *buf,
                                                uint16 length,
                                                uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    marshal_type_t unmarshalled_type;
    call_client_marshal_data_t *data = NULL;
    call_control_client_instance_t *instance = NULL;

    DEBUG_LOG("callControlClientHandover_Unmarshal for addr %04x %02x %06x",
              tp_bd_addr->taddr.addr.nap,
              tp_bd_addr->taddr.addr.uap,
              tp_bd_addr->taddr.addr.lap);

    unmarshaller_t unmarshaller = UnmarshalInit(mtd_call_control_client, CALL_CONTROL_CLIENT_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *)buf, length);

    if (Unmarshal(unmarshaller, (void **)&data, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(call_client_marshal_data_t));
        PanicNull(data);

        instance = CallControlClient_GetInstance(call_client_compare_by_state, call_client_state_idle);
        DEBUG_LOG("callControlClientHandover_Unmarshal instance %p gatt_cid 0x%x", instance, data->gatt_cid);

        PanicNull(instance);

        instance->cid = data->gatt_cid;
        instance->tbs_status_info = data->feature_info;
        instance->content_control_id = data->content_id;
        instance->state = data->state;
        memcpy(&instance->call_state,
               &data->call_state,
               sizeof(ccp_call_info_t) * MAX_ACTIVE_CALLS_SUPPORTED);

        unmarshalled = TRUE;
        *consumed = UnmarshalConsumed(unmarshaller);
        UnmarshalDestroy(unmarshaller, TRUE);
        return TRUE;
    }
    else
    {
        *consumed = 0;
        DEBUG_LOG("callControlClientHandover_Unmarshal: failed unmarshal");
        UnmarshalDestroy(unmarshaller, TRUE);
        return FALSE;
    }
}

/*! \brief Initialize CCP for the new primary */
static void callControlClientHandover_HandleCommitForPrimary(unsigned gatt_cid)
{
    call_control_client_instance_t *instance;
    CcpInitData client_init_params;
    CcpHandles ccp_handle_data = {0};

    DEBUG_LOG("callControlClientHandover_HandleCommitForPrimary");
    /* Initialize the CCP */
    instance = CallControlClient_GetInstance(call_client_compare_by_cid, (unsigned)gatt_cid);

    if (instance != NULL)
    {
        GattTelephoneBearerClientDeviceData *tbs_client_handle;

        instance->state = call_client_state_discovery;
        instance->handover_in_progress = TRUE;
        memset(&ccp_handle_data, 0, sizeof(CcpHandles));
        client_init_params.cid = gatt_cid;
        tbs_client_handle = (GattTelephoneBearerClientDeviceData*) CallControlClient_RetrieveClientHandles(gatt_cid);

        if (tbs_client_handle != NULL)
        {
            ccp_handle_data.tbsHandle = *tbs_client_handle;
        }
        CcpInitReq(TrapToOxygenTask((Task)&call_control_taskdata.task_data),
                   &client_init_params,
                   tbs_client_handle == NULL ? NULL : &ccp_handle_data,
                   FALSE);
    }
}

/*! \brief Handle commit request on new secondary */
static void callControlClientHandover_HandleCommitForSecondary(unsigned gatt_cid)
{
    UNUSED(gatt_cid);
    DEBUG_LOG("callControlClientHandover_HandleCommitForSecondary");

    /* Dont do anything here. When GATT Disconnects(Link transferred), CCP will send a destroy
     * request. Terminate confirmation will come and will clean up the instance.
     */
}

/*! \brief Handle commit for call control client */
static void callControlClientHandover_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    unsigned gatt_cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (is_primary)
    {
        callControlClientHandover_HandleCommitForPrimary(gatt_cid);
    }
    else
    {
        callControlClientHandover_HandleCommitForSecondary(gatt_cid);
    }
}

/*! \brief Handle handover complete for call control client */
static void callControlClientHandover_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
    /* mark complete of unmarshalled data */
    unmarshalled = FALSE;
}

/*! \brief On abort, reset the call control client context in the secondary */
static void callControlClientHandover_HandoverAbort(void)
{
    uint8 idx;
    call_control_client_task_data_t *call_ctx = CallControlClient_GetContext();

    DEBUG_LOG("callControlClientHandover_HandoverAbort");

    if (unmarshalled)
    {
        unmarshalled = FALSE;

        /* Iterate through the call client instances and reset */
        for (idx = 0; idx < MAX_CALL_SERVER_SUPPORTED; idx++)
        {
            CallControlClient_ResetCallClientInstance(&call_ctx->call_client_instance[idx]);
        }
    }
}

/*! \brief Call control client handover interfaces */
const handover_interface call_control_client_handover_if =
        MAKE_BLE_HANDOVER_IF(&callControlClientHandover_Veto,
                             &callControlClientHandover_Marshal,
                             &callControlClientHandover_Unmarshal,
                             &callControlClientHandover_HandoverCommit,
                             &callControlClientHandover_HandoverComplete,
                             &callControlClientHandover_HandoverAbort);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */
