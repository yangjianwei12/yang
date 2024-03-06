/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       handover_profile_private.h
    \addtogroup handover_profile
    \brief      Handover Profile private declarations
    @{  
*/

#ifndef HANDOVER_PROFILE_PRIVATE_H_
#define HANDOVER_PROFILE_PRIVATE_H_

#ifdef INCLUDE_MIRRORING

#include <task_list.h>
#include <sink.h>
#include <source.h>
#include <pmalloc.h>
#include <acl.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <stream.h>
#include "connection_abstraction.h"
#include "handover_profile.h"
#include "handover_profile_device.h"
#include "handover_protocol.h"
#include "handover_profile_primary.h"
#include "handover_profile_secondary.h"
#include "handover_profile_apps_p1.h"
#include "power_manager.h"
#include "peer_signalling.h"
#include "connection_manager.h"
#include "link_policy_config.h"
#include "timestamp_event.h"
#include "mirror_profile_protected.h"
#include "kymera.h"
#include <handover_if.h>

/*! The maximum number of milliseconds to wait for a A2DP media packet to arrive
    from the handset before proceeding with handover. If set to zero, the software
    will not wait for a A2DP media packet before starting handover.
*/
#define HANDOVER_PROFILE_A2DP_HANDOVER_WAIT_FOR_PACKET_TIMEOUT_MS 50

/*! Maximum time to wait for the AclReceiveEnable trap call to complete */
#define HANDOVER_PROFILE_ACL_RECEIVE_ENABLE_TIMEOUT_USEC                    (750000)
/*! Maximum time to wait for AclReceiveDataProcessed trap call to complete */
#define HANDOVER_PROFILE_ACL_RECEIVED_DATA_PROCESSED_TIMEOUT_USEC           (500000)
/* Allowing enough time to send the P0 data is critical, we must be sure that
   either the data has been sent, or the link has been lost. Therefore setting
   the timeout to the link supervision timeout */
#define HANDOVER_PROFILE_P0_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC              appConfigEarbudLinkSupervisionTimeout()
/*! Maximum time to wait for ACL data recieved from the handset to clear during
    the preparation for handover */
#define HANDOVER_PROFILE_ACL_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC             (500)
/*! Maximum time to wait for protocol messages for the peer earbud during the handover procedure */
#define HANDOVER_PROFILE_PROTOCOL_MSG_TIMEOUT_MSEC                          (2000)
/*! Maximum time to wait to exit sniff mode with the peer earbud during the handover procedure */
#define HANDOVER_PROFILE_EXIT_SNIFF_TIMEOUT_MSEC                            (500)
/*! Maximum time to wait for the handset ACL to be prepared for handover by the
    BTSS when the link to the handset is in active mode */
#define HANDOVER_PROFILE_ACL_HANDOVER_PREPARE_TIMEOUT_MSEC                  (20)
/*! Maximum time to wait (as a multiple of the handset sniff interval) for the
    handset ACL to be prepared for handover by the BTSS when the link to the
    handset is in sniff mode */
#define HANDOVER_PROFILE_NO_OF_TIMES_SNIFF_INTERVAL                         (2)
/*! Maximum time to wait to enter sniff mode with the peer earbud during the handover procedure */
#define HANDOVER_PROFILE_REENTER_SNIFF_TIMEOUT_MSEC                         (200)

/*! Maximum number of times to try the SDP search for the peer_signalling attributes.
    After this many attempts the connection request will be failed. */
#define HandoverProfile_GetSdpSearchTryLimit()                              (3)

/*! Maximum time to wait in old secondary to send acknowledgment for BT stack marshalled data received from old primary */
#define HANDOVER_PROFILE_STACK_MARSHAL_DATA_ACK_TIMEOUT_MSEC        (80)

/*! With QHS, the maximum packet size is 1019 octets.The L2CAP inbound/outbound MTU size is computed after
    deducting the L2CAP payload header size of 4 octets from 1019 octets.
*/
#define HANDOVER_PROFILE_L2CAP_MTU_SIZE        (1015)

/*! The size of the source buffer to create to contain the appsP1 marshal data.This pipe size is computed after 
    deducting the P1 Marshal header size of two octets from HANDOVER_PROFILE_L2CAP_MTU_SIZE
*/
#define HANDOVER_PROFILE_MARSHAL_PIPE_BUFFER_SIZE     (HANDOVER_PROFILE_L2CAP_MTU_SIZE - 2)

/*! Enable toggling on PIO18 and PIO19 during handover procedure. 
    This is useful for determining the time taken in the different
    parts of the handover procedure.

    The PIOs need to be setup in pydbg as outputs controlled by P1:
    mask = (1<<18 | 1<<19)
    apps1.fw.call.PioSetMapPins32Bank(0, mask, mask)
    apps1.fw.call.PioSetDir32Bank(0, mask, mask)
*/
//#define HANDOVER_PIO_TOGGLE
#ifdef HANDOVER_PIO_TOGGLE
#include "pio.h"
#define HANDOVER_PIO_MASK (1<<21)
#define HandoverPioSet() PioSet32Bank(0, HANDOVER_PIO_MASK, HANDOVER_PIO_MASK)
#define HandoverPioClr() PioSet32Bank(0, HANDOVER_PIO_MASK, 0)
#else
#define HandoverPioSet()
#define HandoverPioClr()
#endif

/*! Special handle to be used with AclHandoverRoleCommit when there is no BR/EDR mirrored device */
#define HANDOVER_PROFILE_INVALID_HANDLE 0xFF

/*!
@startuml
[*] -d-> INITIALISING : Module init
    INITIALISING : Register SDP record for L2CAP
    INITIALISING -d-> DISCONNECTED : CL_SDP_REGISTER_CFM

    DISCONNECTED : No peer connection
    DISCONNECTED --> CONNECTING_SDP_SEARCH : Startup request (ACL connected)
    DISCONNECTED --> DISCONNECTED : BD-Addr Not valid or ACL not connected
    DISCONNECTED --> CONNECTING_REMOTE : Remote L2CAP connect indication

    CONNECTING_SDP_SEARCH : Performing SDP search for Handover profile service
    CONNECTING_SDP_SEARCH --> CONNECTING_LOCAL : SDP success
    CONNECTING_SDP_SEARCH --> CONNECTING_SDP_SEARCH : SDP retry
    CONNECTING_SDP_SEARCH --> DISCONNECTED : Shutdown request(Cancel SDP)
    CONNECTING_SDP_SEARCH --> DISCONNECTED : SDP error

    CONNECTING_LOCAL : Local initiated connection
    CONNECTING_LOCAL --> CONNECTED : L2CAP connect cfm (success)
    CONNECTING_LOCAL --> DISCONNECTED : L2CAP connect cfm (fail)
    CONNECTING_LOCAL --> DISCONNECTED : Remote L2CAP disconnect ind
    CONNECTING_LOCAL --> DISCONNECTING : Shutdown request

    CONNECTING_REMOTE : Remote initiated connection
    CONNECTING_REMOTE --> CONNECTED : L2CAP connect (success)
    CONNECTING_REMOTE --> DISCONNECTING : Shutdown request
    CONNECTING_REMOTE --> DISCONNECTED : L2CAP connect (fail)
    CONNECTING_REMOTE --> DISCONNECTED : Remote L2CAP disconnect ind

    CONNECTED : Handover profile active
    CONNECTED --> DISCONNECTING : Shutdown request
    CONNECTED --> DISCONNECTED : Remote L2CAP disconnect ind

    DISCONNECTING : Waiting for disconnect result
    DISCONNECTING --> DISCONNECTED : L2CAP disconnect cfm
@enduml
*/

/*! Handover profile state machine states for connecting the peer earbud */
typedef enum
{
    /*! Handover Profile not initialised */
    HANDOVER_PROFILE_STATE_NONE = 0,

    /*!< Handover Profile is initialised */
    HANDOVER_PROFILE_STATE_INITIALISING,

    /*! No connection */
    HANDOVER_PROFILE_STATE_DISCONNECTED,

    /*! Searching for Peer Signalling service */
    HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH,

    /*! Locally initiated connection in progress */
    HANDOVER_PROFILE_STATE_CONNECTING_LOCAL,

    /*! Remotely initiated connection is progress */
    HANDOVER_PROFILE_STATE_CONNECTING_REMOTE,

    /*! Connnected */
    HANDOVER_PROFILE_STATE_CONNECTED,

    /*! Disconnection in progress */
    HANDOVER_PROFILE_STATE_DISCONNECTING
}handover_profile_state_t;

/*! Firmware match/mismatch type */
typedef enum
{
    /*! Earbud does not know the peer earbud's firmware versions */
    HANDOVER_PROFILE_PEER_FIRMWARE_UNKNOWN,

    /*! Primary earbud and secondary earbud have mismatched firmware versions */
    HANDOVER_PROFILE_PEER_FIRMWARE_MISMATCHED,

    /*! Primary earbud and secondary earbud have matched firmware versions */
    HANDOVER_PROFILE_PEER_FIRMWARE_MATCHED,

} handover_profile_peer_firmware_t;

/*! Handover profile module state. */
typedef struct
{
    /*!< Handover Profile task */
    TaskData task;
    /*!< TRUE if role is Primary */
    bool is_primary;
    /*< List of devices that will be handed over */
    handover_device_t *device_list;
    /*!< L2CAP PSM registered */
    uint16 local_psm;
    /*!< L2CAP PSM registered by peer device */
    uint16 remote_psm;
    /*!< The sink of the L2CAP link */
    Sink link_sink;
    /*!< The source of the L2CAP link */
    Source link_source;
    /*!< Bluetooth address of the peer we are signalling */
    bdaddr peer_addr;
    /*!< Current connection state of the handover profile */
    handover_profile_state_t state;
    /*!< Store the Task which requested a connect. */
    Task connect_task;
    /*!< Store the Task which requested a disconnect. */
    Task disconnect_task;
    /*!< List of tasks registered for notifications from handover profile */
    task_list_t handover_client_tasks;
    /*!< Count of failed SDP searches */
    uint16 sdp_search_attempts;
#ifdef USE_SYNERGY
    void *sdp_search_data;
    bool connect_req_sent;
#endif
    /*!< Handover protocol session identifier */
    uint8 session_id;
    /*!< BT ROM firmware versions */
    uint32 btss_rom_version;
    /*!< BT patch firmware versions */
    uint32 btss_patch_version;
    /*!< Records the status of the peer earbud's firmware verions. Handover
         is allowed if the firmware versions match. Handover will be vetoed
         until the peer earbud's firmware version has been received. */
    handover_profile_peer_firmware_t peer_firmware;
    /*! Type of handover. */
    handover_type_t handover_type;
    /*! Pointer to application callback handling handover events on acceptor (Secondary) role */
    const handover_profile_acceptor_cb_t *handover_acceptor_cb;
} handover_profile_task_data_t;

extern handover_profile_task_data_t ho_profile;

/* Returns the handover profile state */
#define HandoverProfile_GetState(ho_inst) (ho_inst->state)

/*! Get pointer to the handover profile task structure */
#define Handover_GetTaskData() (&ho_profile)

/*! Macro to iterate through handover devices conditionally */
#define FOR_EACH_HANDOVER_DEVICE_CONDITIONAL(device, conditional) for (handover_device_t *device = ho_profile.device_list; (conditional) && device != NULL; device=device->next)

/*! Macro to iterate through handover devices */
#define FOR_EACH_HANDOVER_DEVICE(device) FOR_EACH_HANDOVER_DEVICE_CONDITIONAL(device, TRUE)

/*! \brief Internal messages used by handover profile. */
typedef enum
{
    /*! Message to bring up link to peer */
    HANDOVER_PROFILE_INTERNAL_STARTUP_REQ = INTERNAL_MESSAGE_BASE,

    /*! Message to shut down link to peer */
    HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ,

    /*! Message to release resources allocated for SDP search */
    HANDOVER_PROFILE_INTERNAL_CLOSE_SDP_REQ,

    /*! This must be the final message */
    HANDOVER_PROFILE_INTERNAL_MESSAGE_END
} handover_profile_internal_msgs_t;
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(HANDOVER_PROFILE_INTERNAL_MESSAGE_END)

/*! Internal message sent to start initiate handover profile connection 
    to a peer */
typedef struct
{
    bdaddr peer_addr;           /*!< Address of peer */
} HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T;

/*! \brief Set handover profile to new state.

    \param[in] state      Refer \ref handover_profile_state_t, new state.

*/
void HandoverProfile_SetState(handover_profile_state_t state);

/*! \brief Handle result of L2CAP PSM registration request.

    Handles registration of handover-profile with the L2CAP and register the handover 
    profile with the SDP.
    
    \param[in] cfm      Refer \ref CL_L2CAP_REGISTER_CFM_T, pointer to L2CAP register 
                        confirmation message.
*/
#ifdef USE_SYNERGY
void HandoverProfile_HandleCmL2caRegisterCfm(const CsrBtCmL2caRegisterCfm *cfm);

/*! \brief Handle result of the SDP service record registration request.

    Handles confirmation received for registration of handover-profile service record with SDP 
    and move to HANDOVER_PROFILE_STATE_DISCONNECTED state.
    
    \param[in] cfm      Refer \ref CL_SDP_REGISTER_CFM_T, pointer to SDP register 
                        confirmation message.

*/
void HandoverProfile_HandleCmSdsRegisterCfm(const CsrBtCmSdsRegisterCfm *cfm);

/*! \brief Handle the incoming l2cap connection request from remote device.

    This is called only for remote initiated connection.

    \param[in] ind      Refer \ref CsrBtCmL2caConnectAcceptInd, pointer to Connect accept
                        indication message.

*/
void HandoverProfile_HandleCmL2caConnectAcceptInd(const CsrBtCmL2caConnectAcceptInd *ind);

/*! \brief Handle the L2CAP connect confirmation for remote requests.

    \param[in] cfm      Refer \ref CsrBtCmL2caConnectAcceptCfm, pointer to Connect
                        accept confirmation message.

*/
void HandoverProfile_HandleCmL2caConnectAcceptCfm(const CsrBtCmL2caConnectAcceptCfm *cfm);

/*! \brief Handle the L2CAP connect confirmation for local initiated connection.

    \param[in] cfm      Refer \ref CsrBtCmL2caConnectCfm, pointer to Connect
                        confirmation message.

*/
void HandoverProfile_HandleCmL2caConnectCfm(const CsrBtCmL2caConnectCfm *cfm);

/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void HandoverProfile_HandleCmL2caDisconnectInd(const CsrBtCmL2caDisconnectInd *ind);

void HandoverProfile_HandleInternalCloseSdpReq(void);
#else

void HandoverProfile_HandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm);

/*! \brief Handle result of the SDP service record registration request.

    Handles confirmation received for registration of handover-profile service record with SDP 
    and move to HANDOVER_PROFILE_STATE_DISCONNECTED state.
    
    \param[in] cfm      Refer \ref CL_SDP_REGISTER_CFM_T, pointer to SDP register 
                        confirmation message.

*/
void HandoverProfile_HandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm);


/*! \brief Handle the result of a SDP service attribute search.

    The returned attributes are checked to make sure they match the expected format of a 
    handover profile service record.
    
    \param[in] cfm      Refer \ref CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T, pointer to SDP searched 
                        attribute results.

*/
void HandoverProfile_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm);

/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device.

    \param[in] ind      Refer \ref CL_L2CAP_CONNECT_IND_T, pointer to L2CAP connection 
                        indication message.

*/
void HandoverProfile_HandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind);

/*! \brief Handle the result of a L2CAP connection request.

    This is called for both local and remote initiated L2CAP requests.
    
    \param[in] cfm      Refer \ref CL_L2CAP_CONNECT_CFM_T, pointer to L2CAP connect 
                        confirmation.

*/
void HandoverProfile_HandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm);

/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void HandoverProfile_HandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind);
#endif
/*! \brief Handle a L2CAP disconnect confirmation.

    This is called for both local and remote initiated disconnects.
    
    \param[in] cfm      Refer \ref CL_L2CAP_DISCONNECT_CFM_T, pointer to L2CAP disconnect 
                        confirmation.

*/
void HandoverProfile_HandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm);

/*! \brief Handles internal startup request of handover profile.

    Handles internal startup request by intiating connection to peer device based on 
    the current state machine.
    
    \param[in] req      Refer \ref HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T, pointer to 
                        internal startup request message.

*/
void HandoverProfile_HandleInternalStartupRequest(HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T *req);

/*! \brief Handles internal shut-down request of handover-profile.

    Handles internal shutdown request by intiating disconnection to peer device based on 
    the current state machine state.
*/
void HandoverProfile_HandleInternalShutdownReq(void);

/*! \brief Shutdown (or disconnect) the Handover profile connection.

    \param[in] task   Client task
*/
void HandoverProfile_Shutdown(Task task);

/*! \brief Start Handover Signalling channel

    Start handover profile signalling channel by establishing the L2CAP connection.

    \param[in] task         Client task requesting the Handover profile connection.
    \param[in] peer_addr    Address of the peer device
*/
void HandoverProfile_Startup(Task task, const bdaddr *peer_addr);

#endif /* INCLUDE_MIRRORING */
#endif /*HANDOVER_PROFILE_PRIVATE_H_*/
/*! @} */