/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to connect profiles between Primary and Secondary.
*/

#include "stereo_topology_procedure_pri_connect_peer_profiles.h"
#include "stereo_topology_private.h"
#include "stereo_topology_config.h"
#include "stereo_topology_procedure_send_topology_message.h"
#include "stereo_topology.h"


#include <bt_device.h>
#include <peer_signalling.h>
#include <mirror_profile.h>
#include <power_manager.h>

#include <connection_manager.h>

#include <logging.h>
#include <message.h>

/*! Parameter definition for connecting peer profiles */
const PEER_PROFILE_PARAMS_T stereo_topology_procedure_peer_prof_connect = { .connect = TRUE };
/*! Parameter definition for disconnecting peer profiles */
const PEER_PROFILE_PARAMS_T stereo_topology_procedure_peer_prof_disconnect = { .connect = FALSE };


STEREOTOP_PROC_SEND_MESSAGE_PARAMS_T stereo_peer_profile_conn_finished = {PROC_SEND_STEREO_TOPOLOGY_MESSAGE_PEER_PROFILE_CONN_FINISHED, (Message)NULL, 0};


static void stereoTopology_ProcPriConnectPeerProfilesHandleMessage(Task task, MessageId id, Message message);
void StereoTopology_ProcedurePriConnectPeerProfilesStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void StereoTopology_ProcedurePriConnectPeerProfilesCancel(procedure_cancel_cfm_func_t proc_cancel_fn);

const procedure_fns_t proc_pri_connect_peer_profiles_fns = {
    StereoTopology_ProcedurePriConnectPeerProfilesStart,
    StereoTopology_ProcedurePriConnectPeerProfilesCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    uint32 profiles_status;
    bool active;
} stereoTopProcPriConnectPeerProfilesTaskData;

stereoTopProcPriConnectPeerProfilesTaskData stereotop_proc_pri_connect_peer_profiles = {stereoTopology_ProcPriConnectPeerProfilesHandleMessage};

#define StereoTopProcPriConnectPeerProfilesGetTaskData()     (&stereotop_proc_pri_connect_peer_profiles)
#define StereoTopProcPriConnectPeerProfilesGetTask()         (&stereotop_proc_pri_connect_peer_profiles.task)

static void stereoTopology_ProcedurePriConnectPeerProfilesReset(void)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();
    bdaddr secondary_addr;

    /* release the ACL, now held open by L2CAP */
    appDeviceGetSecondaryBdAddr(&secondary_addr);
    ConManagerReleaseAcl(&secondary_addr);
    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, StereoTopProcPriConnectPeerProfilesGetTask());

    td->profiles_status = 0;
    td->active = FALSE;
}

static void stereoTopology_ProcedurePriConnectPeerProfilesConnectProfile(void)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();
    bdaddr secondary_addr;

    appDeviceGetSecondaryBdAddr(&secondary_addr);

    /* Connect the requested profiles in sequence. */
#ifdef INCLUDE_MIRRORING
    if (td->profiles_status & DEVICE_PROFILE_MIRROR)
    {
        DEBUG_LOG("stereoTopology_ProcedurePriConnectPeerProfilesConnectProfile MIRROR");
        MirrorProfile_Connect(StereoTopProcPriConnectPeerProfilesGetTask(), &secondary_addr);
        return;
    }
#endif

    /* When peer signalling connects, many application messages are sent between
       buds (e.g. syncing state). These messages would delay the connection of other
       profiles if they were not already connected. So peer sig is connected last
       to allow this procedure to complete as quickly as possible */
    if (td->profiles_status & DEVICE_PROFILE_PEERSIG)
    {
        DEBUG_LOG("stereoTopology_ProcedurePriConnectPeerProfilesConnectProfile PEERSIG");
        appPeerSigConnect(StereoTopProcPriConnectPeerProfilesGetTask(), &secondary_addr);
    }
}

static void stereoTopology_ProcedurePriPeerProfilesDisconnectProfile(void)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();
    /* Connect the requested profiles in sequence. */
#ifdef INCLUDE_MIRRORING
    if (td->profiles_status & DEVICE_PROFILE_MIRROR)
    {
        DEBUG_LOG("stereoTopology_ProcedurePriPeerProfilesDisconnectProfile MIRROR");
        MirrorProfile_Disconnect(StereoTopProcPriConnectPeerProfilesGetTask());
        return;
    }
#endif
    if (td->profiles_status & DEVICE_PROFILE_PEERSIG)
    {
        DEBUG_LOG("stereoTopology_ProcedurePriPeerProfilesDisconnectProfile PEERSIG");
        appPeerSigDisconnect(StereoTopProcPriConnectPeerProfilesGetTask());
    }
}


void StereoTopology_ProcedurePriConnectPeerProfilesStart(Task result_task,
                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                   procedure_complete_func_t proc_complete_fn,
                                                   Message goal_data)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();
    PEER_PROFILE_PARAMS_T* params = (PEER_PROFILE_PARAMS_T*)goal_data;

    UNUSED(result_task);

    td->profiles_status = StereoTopologyConfig_PeerProfiles();
    DEBUG_LOG("StereoTopology_ProcedurePriConnectPeerProfilesStart (profiles:0x%x)", td->profiles_status);

    /* remember completion function */
    td->complete_fn = proc_complete_fn;
    /* mark procedure active so if cleanup() requested this procedure can ignore
     * any CFM messages that arrive afterwards */
    td->active = TRUE;
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, StereoTopProcPriConnectPeerProfilesGetTask());

    /* need to connect to peer profiles */
    if(params->connect)
    {
        appPowerPerformanceProfileRequest();
        stereoTopology_ProcedurePriConnectPeerProfilesConnectProfile();
    }
    else
    {
        stereoTopology_ProcedurePriPeerProfilesDisconnectProfile();
    }

    /* start is synchronous, use the callback to confirm now */
    proc_start_cfm_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_success);
}

void StereoTopology_ProcedurePriConnectPeerProfilesCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("StereoTopology_ProcedurePriConnectPeerProfilesCancel");

    stereoTopology_ProcedurePriConnectPeerProfilesReset();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_success);
}

static void stereoTopology_ProcPriConnectPeerProfilesStatus(uint32 profile, bool succeeded)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();

    /* remove the profile from the list being handled */
    td->profiles_status &= ~profile;

    /* if one of the profiles failed to connect, then reset this procedure and report
     * failure */
    if (!succeeded)
    {
        DEBUG_LOG("stereoTopology_ProcPriConnectPeerProfilesStatus failed");
        stereoTopology_ProcedurePriConnectPeerProfilesReset();
        td->complete_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_failed);
    }
    else if (!td->profiles_status)
    {
        /* reset procedure and report start complete if all profiles connected */
        stereoTopology_ProcedurePriConnectPeerProfilesReset();
        td->complete_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_success);
    }
    else
    {
        stereoTopology_ProcedurePriConnectPeerProfilesConnectProfile();
    }
    
    appPowerPerformanceProfileRelinquish();
}

static void stereoTopology_ProcPriDisconnectPeerProfilesStatus(uint32 profile, bool succeeded)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();

    /* remove the profile from the list being handled */
    td->profiles_status &= ~profile;

    /* if one of the profiles failed to connect, then reset this procedure and report
     * failure */
    if (!succeeded)
    {
        DEBUG_LOG("stereoTopology_ProcPriDisconnectPeerProfilesStatus failed");
        stereoTopology_ProcedurePriConnectPeerProfilesReset();
        td->complete_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_failed);
    }
    else if (!td->profiles_status)
    {
        /* reset procedure and report start complete if all profiles connected */
        stereoTopology_ProcedurePriConnectPeerProfilesReset();
        td->complete_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_success);
    }
    else
    {
        stereoTopology_ProcedurePriPeerProfilesDisconnectProfile();
    }
}


static void stereoTopology_ProcPriConnectPeerProfileHandleDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();
    bdaddr secondary_addr;
    appDeviceGetSecondaryBdAddr(&secondary_addr);

    if (BdaddrIsSame(&ind->tpaddr.taddr.addr, &secondary_addr))
    {
        DEBUG_LOG("stereoTopology_ProcPriConnectPeerProfileHandleDisconnectInd secondary disconnected");
        td->complete_fn(stereo_topology_procedure_pri_connect_peer_profiles, procedure_result_failed);
        stereoTopology_ProcedurePriConnectPeerProfilesReset();
    }
}

static void stereoTopology_ProcPriConnectPeerProfilesHandleMessage(Task task, MessageId id, Message message)
{
    stereoTopProcPriConnectPeerProfilesTaskData* td = StereoTopProcPriConnectPeerProfilesGetTaskData();

    UNUSED(task);

    /* if no longer active then ignore any CFM messages,
     * they'll be connect_cfm(cancelled) */
    if (!td->active)
    {
        return;
    }

    switch (id)
    {
        case CON_MANAGER_TP_DISCONNECT_IND:
            stereoTopology_ProcPriConnectPeerProfileHandleDisconnectInd(message);
        break;

        case PEER_SIG_CONNECT_CFM:
        {
            PEER_SIG_CONNECT_CFM_T* cfm = (PEER_SIG_CONNECT_CFM_T*)message;
            DEBUG_LOG("stereoTopology_ProcPriConnectPeerProfilesStatus PEERSIG status %d", cfm->status);
            stereoTopology_ProcPriConnectPeerProfilesStatus(DEVICE_PROFILE_PEERSIG, cfm->status == peerSigStatusSuccess);
        }
        break;

        case PEER_SIG_DISCONNECT_CFM:
        {
            PEER_SIG_DISCONNECT_CFM_T* cfm = (PEER_SIG_DISCONNECT_CFM_T*)message;
            DEBUG_LOG("stereoTopology_ProcPriDisconnectPeerProfilesStatus PEERSIG status %d", cfm->status);
            stereoTopology_ProcPriDisconnectPeerProfilesStatus(DEVICE_PROFILE_PEERSIG, cfm->status == peerSigStatusSuccess);
        }
        break;

#ifdef INCLUDE_MIRRORING
        case MIRROR_PROFILE_CONNECT_CFM:
        {
            MIRROR_PROFILE_CONNECT_CFM_T* cfm = (MIRROR_PROFILE_CONNECT_CFM_T*)message;
            DEBUG_LOG("stereoTopology_ProcPriConnectPeerProfilesStatus MIRROR status %d", cfm->status);
            stereoTopology_ProcPriConnectPeerProfilesStatus(DEVICE_PROFILE_MIRROR, cfm->status == mirror_profile_status_peer_connected);
        }
        break;

        case MIRROR_PROFILE_DISCONNECT_CFM:
        {
            MIRROR_PROFILE_DISCONNECT_CFM_T* cfm = (MIRROR_PROFILE_DISCONNECT_CFM_T*)message;
            DEBUG_LOG("stereoTopology_ProcPriDisconnectPeerProfilesStatus MIRROR status %d", cfm->status);
            stereoTopology_ProcPriDisconnectPeerProfilesStatus(DEVICE_PROFILE_MIRROR, cfm->status == mirror_profile_status_peer_disconnected);
            
        }
        break;
#endif

        default:
        break;
    }
}


