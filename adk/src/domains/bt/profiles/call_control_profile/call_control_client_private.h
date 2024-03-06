/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup call_control_client
    \brief      Private defines and types for media control profile.
    @{
*/

#ifndef CALL_CONTROL_CLIENT_PRIVATE_H_
#define CALL_CONTROL_CLIENT_PRIVATE_H_

#include <logging.h>
#include <panic.h>
#include "bt_types.h"
#include "ccp.h"
#include "pddu_map.h"
#include "device.h"
#include "gatt_connect.h"
#include "device_properties.h"
#include "device_db_serialiser.h"
#include "gatt.h"
#include "gatt_service_discovery_lib.h"
#include "synergy.h"
#include "telephony_messages.h"
#include "ui.h"
#include <message.h>
#include <focus_voice_source.h>
#include "gatt_service_discovery.h"
#include "call_control_client.h"

/*! Number of Call Control Servers supported */
#define MAX_CALL_SERVER_SUPPORTED GATT_CONNECT_MAX_REMOTE_DEVICES

/*!@todo Remove this macro, once CCP Profile Prim is defined in synergy */
#define CCP_PROFILE_PRIM             (0x505C)

#define MAX_ACTIVE_CALLS_SUPPORTED      (2)

/* Call originate command prefix size */
#define CCP_SIZE_OF_PREFIX                 4

/* Call originate command suffix size */
#define CCP_SIZE_OF_SUFFIX                 1

/* Call originate command Fixed number size */
#define CCP_SIZE_OF_FIXED_NUMBER           3

/* Call originate command Fixed number */
#define CCP_FIXED_NUMBER                (uint8 *)("123")

/*! \brief Internal message IDs */
enum ccp_profile_internal_messages
{
    CCP_INTERNAL_OUT_OF_BAND_RINGTONE_REQ = INTERNAL_MESSAGE_BASE,      /*!< Internal message to request out of band ringtone indication */

    /*! This must be the final message */
    CCP_INTERNAL_MESSAGE_END
};

/*! Call client instance state */
typedef enum
{
    /*! Call client instance in idle/free state */
    call_client_state_idle,

    /*! Discovery in progress state */
    call_client_state_discovery,

    /*! Call client in connected state */
    call_client_state_connected
} call_client_state_t;

/*! CCP task bitfields data structure */
typedef struct
{
    /*! AG supports in-band ringing tone */
    uint8 in_band_ring:1;

    /*! Server is in silent mode */
    uint8 silent_mode:1;
} gtbs_status_feature_info_t;

/*! Call control Client Instance Information */
typedef struct
{
    /*! Connection Identifier for this Call client instance */
    gatt_cid_t                  cid;
 
    /*! Call Control Profile Handle */
    CcpProfileHandle            ccp_profile_handle;
 
    /* Content control Id */
    uint16 content_control_id;

    /*! Instance present state */
    call_client_state_t         state;

    /*! GTBS status and feature info */
    gtbs_status_feature_info_t  tbs_status_info;

    /*! Call state and index data for Max supported calls */
    ccp_call_info_t             call_state[MAX_ACTIVE_CALLS_SUPPORTED];

    /* Is a handover in progress */
    bool handover_in_progress;
} call_control_client_instance_t;


/*! Call client instance state */
typedef enum
{
    /*! Get Call Client instance based on connection identifier */
    call_client_compare_by_cid,

    /*! Get Call Client instance based on profile handle */
    call_client_compare_by_profile_handle,

    /*! Get Call Client instance by state */
    call_client_compare_by_state,

    /*! Get Call Client instance by bdaddr */
    call_client_compare_by_bdaddr,

    /*! Get Call Client instance by voice source */
    call_client_compare_by_voice_source,

    /*! Get Call Client instance by valid and inavlid cid */
    call_client_compare_by_valid_invalid_cid
} call_instance_compare_by_type_t;

/*! \brief Call control client context. */
typedef struct
{
    /*! Call control profile task */
    TaskData task_data;

    /*! Call control client instance */
    call_control_client_instance_t call_client_instance[MAX_CALL_SERVER_SUPPORTED];
} call_control_client_task_data_t;

/*! \brief Call control client opcode. */
typedef struct
{
    /* Opcode */
    GattTbsOpcode op;

    /*! value associated with opcode */
    int32 val;
} call_control_set_t;

typedef struct
{
    void (*enter) (ccp_call_info_t *, const TbsCallState*, voice_source_t source);
    void (*exit) (ccp_call_info_t *, const TbsCallState*, voice_source_t source);
} ccp_call_state_handler_t;


/*! \brief Get the voice source for associated Cid
    \param cid - The connection id of the instance
    \return voice_source_t - The voice source associated with the cid
 */
voice_source_t CallControlClient_GetVoiceSourceForCid(gatt_cid_t cid);

/*! \brief Set the call state
    \param call_info - The call info from the call control client instance, maintaining the call id and state.
    \param tbs_call_state - The TBS call state info from the server.
 */
void CcpSm_SetCallState(gatt_cid_t cid, ccp_call_info_t *call_info, const TbsCallState *tbs_call_state);

/*! \brief Get the call state
    \brief cid - Connection Id of the instance.
    \param instance - Call control Client Instance.
    \param index - index of the call control client instance for which the state is requested.

    \return The call state of the matching tbs_call_id if match is successful,
            else returns CCP_CALL_STATE_IDLE.
 */
ccp_call_state_t CcpSm_GetCallStateForMatchingCallId(call_control_client_instance_t *instance, uint8 call_id);

/*! \brief Get the call state
    \param instance - Call control Client Instance.
    \param index - index of the call control client instance for which the state is requested.

    \return The call state of the requested call index.
 */
#define CcpSm_GetCallState(instance, index)      (instance->call_state[index].state)

#define IsInbandRingingEnabled(flags) ((flags) & (1<<(TBS_CALL_FLAG_INBAND_RING)))

#define IsServerInSilentMode(flags) ((flags) & (1<<(TBS_CALL_FLAG_SERVER_SILENT)))

/*! Returns TRUE if call state is active */
#define CallControlClient_IsCallOngoing(state)    (state == CCP_CALL_STATE_ACTIVE ||\
                                                   state == CCP_CALL_STATE_OUTGOING_DIALING ||\
                                                   state == CCP_CALL_STATE_OUTGOING_ALERTING)

/*! Returns TRUE if call state is held */
#define CallControlClient_IsCallHeld(state)       (state == CCP_CALL_STATE_LOCALLY_HELD ||\
                                                   state == CCP_CALL_STATE_REMOTELY_HELD ||\
                                                   state == CCP_CALL_STATE_LOCALLY_REMOTELY_HELD)

/*! Call control client task Data */
extern call_control_client_task_data_t call_control_taskdata;

/*! Returns the call control client context */
#define CallControlClient_GetContext()         (&call_control_taskdata)

/*! Returns the call control client task */
#define CallControlClient_GetTask()            (&call_control_taskdata.task_data)

/*! \brief Get the Call client instance based on the compare type */
call_control_client_instance_t * CallControlClient_GetInstance(call_instance_compare_by_type_t type, unsigned cmp_value);

/*! \brief Retrieve the GTBS handles from NVM */
void * CallControlClient_RetrieveClientHandles(gatt_cid_t cid);

/*! \brief Store the GTBS handles in NVM */
bool CallControlClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size);

/*! \brief Reset the call control client instance */
void CallControlClient_ResetCallClientInstance(call_control_client_instance_t *call_client);

#endif /* CALL_CONTROL_CLIENT_PRIVATE_H_ */
/*! @} */