/*!
    \copyright  Copyright (c) 2017 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       dfu.c
    \ingroup    dfu
    \brief      Device firmware upgrade management.

    Over the air upgrade is managed from this file.
*/

#ifdef INCLUDE_DFU

#include "dfu.h"

#include "system_state.h"
#include "adk_log.h"
#include "phy_state.h"
#include "bt_device.h"
#include "device_properties.h"
#include "device_db_serialiser.h"

#include <charger_monitor.h>
#include <system_state.h>

#include <vmal.h>
#include <panic.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <upgrade.h>
#include <ps.h>
#include <gatt_connect.h>
#include <gatt_handler.h>
#include <gatt_server_gatt.h>
#include <connection_manager.h>
#include <device_list.h>
#include <connection_manager_list.h>
#include "dfu_rules.h"

#ifdef INCLUDE_DFU_PEER
#include "dfu_peer.h"
#include "bt_device.h"
#include <app/message/system_message.h>
#include <peer_signalling.h>
#include <tws_topology_config.h>
#include <mirror_profile.h>
#include <handover_profile.h>
#include <power_manager.h>
#endif

#ifdef INCLUDE_DFU_CASE
#include "dfu_case.h"
#endif

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(DFU, DFU_MESSAGE_END)

#endif

/*!< Task information for UPGRADE support */
dfu_task_data_t app_dfu;

/*! Identifiers for messages used internally by the DFU module */
typedef enum dfu_internal_messages_t
{
    DFU_INTERNAL_BASE = INTERNAL_MESSAGE_BASE ,

    DFU_INTERNAL_CONTINUE_HASH_CHECK_REQUEST,

#ifdef INCLUDE_DFU_PEER
    DFU_INTERNAL_UPGRADE_APPLY_RES_ON_PEER_PROFILES_CONNECTED,
#endif

    /*! This must be the final message */
    DFU_INTERNAL_MESSAGE_END
};
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(DFU_INTERNAL_MESSAGE_END)

LOGGING_PRESERVE_MESSAGE_ENUM(dfu_internal_messages_t)
LOGGING_PRESERVE_MESSAGE_TYPE(dfu_messages_t)

/* The upgrade libraries use of partitions is not relevant to the
   partitions as used on devices targetted by this application.

   As it is not possible to pass 0 partitions in the Init function
   use a simple entry */
static const UPGRADE_UPGRADABLE_PARTITION_T logicalPartitions[]
                    = {UPGRADE_PARTITION_SINGLE(0x1000,DFU)
                      };

/*! Maximum size of buffer used to hold the variant string
    supplied by the application. 6 chars, plus NULL terminator */
#define VARIANT_BUFFER_SIZE (7)

static void dfu_MessageHandler(Task task, MessageId id, Message message);
static void dfu_GattConnect(gatt_cid_t cid);
static void dfu_GattDisconnect(gatt_cid_t cid);
static void dfu_GetVersionInfo(dfu_VersionInfo *ver_info);
static void dfu_SetGattServiceUpdateFlags(void);
#ifdef INCLUDE_DFU_PEER
static void dfu_PeerSetContextTx(upgrade_context_t context);
static void dfu_PeerSyncVldtnCompleteTx(void);
#endif

static const gatt_connect_observer_callback_t dfu_gatt_connect_callback =
{
    .OnConnection = dfu_GattConnect,
    .OnDisconnection = dfu_GattDisconnect
};

static void dfu_NotifyStartedNeedConfirm(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_REQUESTED_TO_CONFIRM);
}


static void dfu_NotifyStartedWithInProgress(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_REQUESTED_IN_PROGRESS);
}


static void dfu_NotifyActivity(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_ACTIVITY);
}


static void dfu_NotifyStart(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_STARTED);
}

static void dfu_NotifyCompleted(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_COMPLETED);
}

static void dfu_NotifyAbort(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_CLEANUP_ON_ABORT);
}

static void dfu_NotifyAborted(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_ABORTED);
}

static void dfu_NotifyReadyforSilentCommit(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_READY_FOR_SILENT_COMMIT);
}

static void dfu_NotifyReadyToReboot(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_READY_TO_REBOOT);
}

/*************************************************************************
    Provide the logical partition map.

    For earbuds this is initially hard coded, but may come from other
    storage in time.
*/
static void dfu_GetLogicalPartitions(const UPGRADE_UPGRADABLE_PARTITION_T **partitions, uint16 *count)
{
    uint16 num_partitions = sizeof(logicalPartitions)/sizeof(logicalPartitions[0]);
    *partitions = logicalPartitions;
    *count = num_partitions;
}

/*************************************************************************
    Get the variant Id from the firmware and convert it into a variant
    string that can be passed to UpgradeInit.

    This function allocates a buffer for the string which must be freed
    after the call to UpgradeInit.
*/
static void dfu_GetVariant(char *variant, size_t length)
{
    int i = 0;
    char chr;
    uint32 product_id;

    PanicFalse(length >= VARIANT_BUFFER_SIZE);

    product_id = VmalVmReadProductId();
    if (product_id == 0)
    {
        variant[0] = '\0';
        return;
    }

    /* The product Id is encoded as two ascii chars + 4 integers in BCD format. */

    /* The ascii chars may be undefined or invalid (e.g. '\0').
       If so, don't include them in the variant string. */
    chr = (char)((product_id >> 8) & 0xFF);
    if (isalnum(chr))
        variant[i++] = chr;

    chr = (char)(product_id & 0xFF);
    if (isalnum(chr))
        variant[i++] = chr;

    sprintf(&variant[i], "%04X", ((uint16)((product_id >> 16) & 0xFFFF)));
}


/********************  PUBLIC FUNCTIONS  **************************/


bool Dfu_EarlyInit(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("Dfu_EarlyInit");

#if (THE_DFU_CLIENT_LIST_INIT_CAPACITY == 1)
    TaskList_Initialise((task_list_t *)Dfu_GetClientList());
#else
    TaskList_InitialiseWithCapacity(Dfu_GetClientList(), THE_DFU_CLIENT_LIST_INIT_CAPACITY);
#endif

    return TRUE;
}

/*! Initialisation point for the over the air support in the upgrade library.
 *
 */
bool Dfu_Init(Task init_task)
{
    dfu_task_data_t *the_dfu=Dfu_GetTaskData();
    uint16 num_partitions;
    const UPGRADE_UPGRADABLE_PARTITION_T *logical_partitions;
    char variant[VARIANT_BUFFER_SIZE];
    dfu_VersionInfo ver_info;
    UNUSED(init_task);

    dfu_GetVersionInfo(&ver_info);

    GattConnect_RegisterObserver(&dfu_gatt_connect_callback);

    the_dfu->dfu_task.handler = dfu_MessageHandler;
    the_dfu->reboot_permission_required = FALSE;

#ifdef INCLUDE_DFU_PEER
    /*
     * Register to use marshalled message channel with DFU domain for Peer DFU
     * messages.
     */
    appPeerSigMarshalledMsgChannelTaskRegister(Dfu_GetTask(),
        PEER_SIG_MSG_CHANNEL_DFU,
        dfu_peer_sig_marshal_type_descriptors,
        NUMBER_OF_DFU_PEER_SIG_MARSHAL_TYPES);

    /* Register for peer signaling notifications */
    appPeerSigClientRegister(Dfu_GetTask());

    /* Register for connect / disconnect events from mirror profile */
    MirrorProfile_ClientRegister(Dfu_GetTask());

    HandoverProfile_ClientRegister(Dfu_GetTask());

    /* Register for power management during DFU process */
    appPowerClientRegister(Dfu_GetTask());
    appPowerClientAllowSleep(Dfu_GetTask());
#endif

    ConManagerRegisterConnectionsClient(Dfu_GetTask());

    dfu_GetVariant(variant, sizeof(variant));

    dfu_GetLogicalPartitions(&logical_partitions, &num_partitions);

    /* Initiate the DFU rules engine */
    DfuRules_Init(Dfu_GetTask());

    /* Allow storage of info at end of (SINK_UPGRADE_CONTEXT_KEY) */
    UpgradeInit(Dfu_GetTask(), UPGRADE_CONTEXT_KEY, UPGRADE_LIBRARY_CONTEXT_OFFSET,
            logical_partitions,
            num_partitions,
            UPGRADE_INIT_POWER_MANAGEMENT,
            variant,
            upgrade_perm_always_ask,
            &ver_info.upgrade_ver,
            ver_info.config_ver);

    return TRUE;
}


bool Dfu_HandleSystemMessages(MessageId id, Message message, bool already_handled)
{
    switch (id)
    {
        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
        case MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS:
        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
        {
            Task upg = Dfu_GetTask();

            upg->handler(upg, id, message);
            return TRUE;
        }
    }
    return already_handled;
}

static void dfu_ForwardInitCfm(const UPGRADE_INIT_CFM_T *cfm)
{
    UPGRADE_INIT_CFM_T *copy = PanicUnlessNew(UPGRADE_INIT_CFM_T);
    *copy = *cfm;

    MessageSend(SystemState_GetTransitionTask(), UPGRADE_INIT_CFM, copy);
}

static void dfu_HandleRestartedInd(const UPGRADE_RESTARTED_IND_T *restart)
{
    /* This needs to base its handling on the reason in the message,
       for instance upgrade_reconnect_not_required is a hint that errr,
       reconnect isn't a priority. */

    DEBUG_LOG("dfu_HandleRestartedInd 0x%x", restart->reason);
    switch (restart->reason)
    {
        case upgrade_reconnect_not_required:
            /* No need to reconnect, not even sure why we got this */
            break;

        case upgrade_reconnect_required_for_confirm:
            Dfu_SetRebootReason(REBOOT_REASON_DFU_RESET);
            dfu_NotifyStartedNeedConfirm();
#ifndef INCLUDE_DFU_PEER
            /* If peer is NOT supported, commit new image here for silent commit option*/
            if(UpgradeIsSilentCommitEnabled())
            {
                DEBUG_LOG("dfu_HandleRestartedInd: UpgradeCommitConfirmForSilentCommit");
                UpgradeCommitConfirmForSilentCommit();
            }
#endif

#ifdef INCLUDE_DFU_PEER
            /* Prohibit sleep during commit phase */
            DEBUG_LOG_VERBOSE("dfu_HandleRestartedInd Prohibit earbud to sleep");
            appPowerClientProhibitSleep(Dfu_GetTask());
#endif
            break;

        case upgrade_reconnect_recommended_as_completed:
        case upgrade_reconnect_recommended_in_progress:
            /*
            * Remember the reset reason, in order to progress an DFU 
            * if abruptly reset.
            */
            Dfu_SetRebootReason(REBOOT_REASON_ABRUPT_RESET);
            dfu_NotifyStartedWithInProgress();

#ifdef INCLUDE_DFU_PEER
            /* Prohibit sleep during abrupt reset phase */
            DEBUG_LOG_VERBOSE("dfu_HandleRestartedInd Prohibit earbud to sleep");
            appPowerClientProhibitSleep(Dfu_GetTask());
#endif

            break;
    }
}


static void dfu_HandleUpgradeStatusInd(const UPGRADE_STATUS_IND_T *sts)
{
    dfu_NotifyActivity();

    switch (sts->state)
    {
        case upgrade_state_idle:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. idle(%d)",sts->state);
            break;

        case upgrade_state_downloading:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. downloading(%d)",sts->state);
            break;

        case upgrade_state_download_completed:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. download completed(%d)",sts->state);
            DfuRules_SetEvent(DFU_EVENT_DATA_TRANSFER_COMPLETE);
            break;

        case upgrade_state_validation_completed:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. validation completed(%d)",sts->state);
#ifdef INCLUDE_DFU_PEER
            if(UPGRADE_PEER_IS_CONNECTED)
            {
                /* Send the resume point info to peer device */
                dfu_PeerSyncVldtnCompleteTx();
            }
#endif
            /* Event to handle the validaiton complete scenario */
            DfuRules_SetEvent(DFU_EVENT_VALIDATION_COMPLETE);

            break;
        case upgrade_state_commiting:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. commiting(%d)",sts->state);
            break;

        case upgrade_state_done:
            DEBUG_LOG("dfu_HandleUpgradeStatusInd. done(%d)",sts->state);
            dfu_NotifyCompleted();

            /* To Do: remove when merging GAA resume changes because context gets cleared
               as a part of upgrade pskey */
            Upgrade_SetContext(UPGRADE_CONTEXT_UNUSED);

            DfuRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);

#ifdef INCLUDE_DFU_PEER
            DEBUG_LOG_VERBOSE("dfu_HandleUpgradeStatusInd Allow earbud to sleep");
            /* DFU is completed. Allow device to sleep */
            appPowerClientAllowSleep(Dfu_GetTask());
#endif
            break;

        default:
            DEBUG_LOG_ERROR("dfu_HandleUpgradeStatusInd. Unexpected state %d",sts->state);
            Panic();
            break;
    }
}

static void dfu_HandleUpgradeOperationInd(const UPGRADE_OPERATION_IND_T *operation)
{
    DEBUG_LOG_INFO("dfu_HandleUpgradeOperationInd. Ops enum:upgrade_ops_t:%d", operation->ops);
    switch (operation->ops)
    {
        case upgrade_ops_trnsfr_complt_res_send_to_peer:
        {
#ifdef INCLUDE_DFU_PEER
            /* Send UPGRADE_PEER_TRANSFER_COMPLETE_RES message to 
             * upgrade_peer library of both the devices
             * since the action set with this message will be required
             * during the dynamic role commit phase.
             */
            if(UPGRADE_PEER_IS_SUPPORTED)
            {
                uint8 is_silent_commit = operation->action;
                upgrade_action_status_t upgrade_action = 
                             is_silent_commit? UPGRADE_SILENT_COMMIT : UPGRADE_CONTINUE;

                DfuPeer_ProcessHostMsg(UPGRADE_PEER_TRANSFER_COMPLETE_RES, (uint8)upgrade_action);
            }
#endif
        }
            break;
        case upgrade_ops_send_silent_commit_ind_to_host:
        {
#ifdef INCLUDE_DFU_PEER
            if(!UPGRADE_PEER_IS_PRIMARY && UPGRADE_PEER_IS_CONNECTED)
            {
                UpgradeSendReadyForSilentCommitInd();
            }
#else
            UpgradeSendReadyForSilentCommitInd();
#endif
        }
            break;
        case upgrade_ops_check_peer_during_commit:
        {
#ifdef INCLUDE_DFU_PEER
            /* Verify that peer is connected otherwise, we should just abort rather than waiting. */

            if(UPGRADE_PEER_IS_SUPPORTED && !UPGRADE_PEER_IS_CONNECTED)
            {
                DEBUG_LOG("dfu_HandleUpgradeOperationInd Fatal error, UPGRADE_PEER_IS_CONNECTED %d",UPGRADE_PEER_IS_CONNECTED);
                UpgradeFatalErrorAppNotReady();
            }

            if(UPGRADE_PEER_IS_PRIMARY && UPGRADE_PEER_IS_STARTED)
            {
                DfuPeer_ProcessHostMsg(UPGRADE_PEER_COMMIT_CFM, operation->action);
            }
#endif
        }
            break;
        case upgrade_ops_init_peer_context:
        {
#ifdef INCLUDE_DFU_PEER
            if(!UPGRADE_PEER_IS_PRIMARY)
            {
                DfuPeer_CtxInit();
            }
#endif
        }
            break;
        case upgrade_ops_delay_prim_commit:
        {
            /* For Earbuds Case*/
            /* Primary EB should wait for the peer to first complete the commit
             * so any error occured on secondary EB can be handled. eg., if SEB
             * has been reset before receiving the commit confirm, then it will
             * be in the sync state so, we should just abort. On success, SEB
             * will send the Upgrade complete indication. After this, PEB can commit. */
#ifdef INCLUDE_DFU_PEER
            if(!UPGRADE_PEER_IS_PRIMARY)
#endif
                UpgradeSmCommitConfirmYes();
        }
            break;

        case upgrade_ops_send_host_in_progress_ind:
        {
#ifdef INCLUDE_DFU_PEER
            if(UPGRADE_PEER_IS_PRIMARY && UPGRADE_PEER_IS_STARTED)
            {
                if(UPGRADE_PEER_IS_COMMIT_CONTINUE && UpgradeSmIsStateCommitHostContinue())
                {
                    UpgradeSmSendHostInProgressInd(TRUE, TRUE);
                }
                else
                {
                    UpgradeSmSendHostInProgressInd(TRUE, FALSE);
                }
            }
            else
#endif
            {
                UpgradeSmSendHostInProgressInd(FALSE, FALSE);
            }
        }
            break;

        case upgrade_ops_check_peer_commit:
        {
#ifdef INCLUDE_DFU_PEER
            /* If Peer upgrade is running then we will wait for
             * UPGRADE_PEER_COMMIT_REQ to proceed, else proceed further.
             */
            if(!(UPGRADE_PEER_IS_PRIMARY && !UPGRADE_PEER_IS_COMMITED))
            {
                UpgradeSmHandleCommitVerifyProceed();
            }
#else
            UpgradeSmHandleCommitVerifyProceed();
#endif
        }
            break;

        case upgrade_ops_relay_peer_proceed_to_commit:
        {
#ifdef INCLUDE_DFU_PEER
            /*
             * In the post reboot DFU commit phase, now main role
             * (Primary/Secondary) are no longer fixed rather dynamically
             * selected by Topology using role selection. So if a role swap
             * occurs in the post reboot DFU commit phase.
             * (e.g. Primary on post reboot DFU commit phase becomes Secondary
             * In this scenario, the peer DFU L2CAP channel is established
             * by Old Primary and as a result UPGRADE_PEER_IS_STARTED won't be
             * satisfied on the New Primary.)
             *
             * Since the DFU domain communicates the main role on peer
             * signalling channel establishment which is established earlier
             * then handset connection, the necessary and sufficient pre-
             * conditions to relay UPGRADE_PEER_PROCEED_TO_COMMIT are as follows:
             * - firstly on the role as Primary and
             * - secondly on the peer DFU channel being setup. If peer DFU
             * channel isn't setup then defer relaying UPGRADE_PEER_PROCEED_TO_COMMIT
             * to the peer. Peer DFU channel is established post peer signalling
             * channel establishment.
             */
            if(UPGRADE_PEER_IS_PRIMARY)
            {
                if(UPGRADE_PEER_IS_CONNECTED)
                {
                    DfuPeer_ProcessHostMsg(UPGRADE_PEER_PROCEED_TO_COMMIT, operation->action);
                    UpgradeSmHandleProceedToCommit(FALSE, ZERO_DURATION, operation->action);
                }
                else
                {
                    UpgradeSmHandleProceedToCommit(TRUE, UPGRADE_PEER_POLL_INTERVAL, operation->action);
                }
            }
            else
#endif
            {
                UpgradeSmHandleProceedToCommit(TRUE, ZERO_DURATION, operation->action);
            }
        }
            break;
        case upgrade_ops_handle_notify_host_of_commit:
        {
#ifdef INCLUDE_DFU_PEER
            /*
             * Notify the Host of commit and upgrade completion only
             * when peer is done with its commit and upgrade completion.
             *
             * Poll for peer upgrade completion at fixed intervals
             * (less frequently) before notify the Host of its commit and
             * upgrade completion.
             */
            if(UPGRADE_PEER_IS_PRIMARY && !UPGRADE_PEER_IS_ENDED)
            {
                UpgradeSmHandleNotifyHostOfCommit(TRUE, UPGRADE_PEER_POLL_INTERVAL);
            }
            else
#endif
            {
                UpgradeSmHandleNotifyHostOfCommit(FALSE, ZERO_DURATION);
            }
        }
            break;


        case upgrade_ops_notify_host_of_upgrade_complete:
        {
            bool is_silent_commit = UpgradeIsSilentCommitEnabled();
            bool is_primary = FALSE;
            /*
             * Notify the Host of commit and upgrade completion only
             * when peer is done with its commit and upgrade completion.
             *
             * Poll for peer upgrade completion at fixed intervals
             * (less frequently) before notify the Host of its commit and
             * upgrade completion.
             */
            /*
             * For silent commit, no notification to host is required so 
             * proceed with completion in else part
             */
#ifdef INCLUDE_DFU_PEER
            is_primary = UPGRADE_PEER_IS_PRIMARY;
            if (!is_silent_commit && UPGRADE_PEER_IS_PRIMARY && !UPGRADE_PEER_IS_ENDED)
            {
                UpgradeSmHandleNotifyHostOfComplete(is_silent_commit, UPGRADE_PEER_POLL_INTERVAL, is_primary);
            }
            else
#endif
            {
                UpgradeSmHandleNotifyHostOfComplete(is_silent_commit, ZERO_DURATION, is_primary);
            }

        }
            break;

        case upgrade_ops_abort_post_transfer_complete:
        {
#ifdef INCLUDE_DFU_PEER
            if(UPGRADE_PEER_IS_PRIMARY && UPGRADE_PEER_IS_STARTED)
            {
                /* Host has aborted th DFU, inform peer device as well */
                DfuPeer_ProcessHostMsg(UPGRADE_PEER_TRANSFER_COMPLETE_RES, (uint8)UPGRADE_ABORT);
            }
            else
#endif
            {
                UpgradeSMHandleAbort();
            }
        }
            break;

        case upgrade_ops_handle_abort:
        {
#ifdef INCLUDE_DFU_PEER
            /* If peer upgrade is supported then inform UpgradePeer lib as well */
            if(UPGRADE_PEER_IS_PRIMARY)
            {
                DfuPeer_ProcessHostMsg(UPGRADE_PEER_ABORT_REQ, (uint8)UPGRADE_ABORT);
            }
#endif
        }
            break;

        case upgrade_ops_reset_peer_current_state:
        {
#ifdef INCLUDE_DFU_PEER
            /* Set the currentState of UpgradePSKeys and SmCtx to default value
             * after role switch since the new secondary don't need this
             * information at this stage. Moreover, if it is not set, then
             * during subsequent DFU, the currentState incorrect value can
             * lead to not starting the peer DFU.
             */
            UpgradePeerResetCurState();
#endif
        }
            break;

            case upgrade_ops_clear_peer_pskeys:
            {
                /* Clear the  upgrade peer pskey */
#ifdef INCLUDE_DFU_PEER
                UpgradePeerClearPSKeys();
#endif
            }
                break;

            case upgrade_ops_clean_up_on_abort:
            {
#ifdef INCLUDE_DFU_PEER
                if(UpgradePeerIsSecondary())
                {
                    DEBUG_LOG("dfu_HandleUpgradeOperationInd UPGRADE_HOST_ERRORWARN_RES received from Peer");
                    UpgradeSMAbort();
                    UpgradeCleanupOnAbort();
                }
#endif
            }
                break;

        default:
            DEBUG_LOG_ERROR("dfu_HandleUpgradeOperationInd. Unexpected state %d",operation->ops);
            break;
    }
}

static void dfu_HandleUpgradeTransportNotification(const UPGRADE_NOTIFY_TRANSPORT_STATUS_T *notification)
{
    DEBUG_LOG_INFO("dfu_HandleUpgradeTransportNotification. Status: %d", notification->status);
    switch (notification->status)
    {
        case upgrade_notify_transport_connect:
        {
#ifdef INCLUDE_DFU_PEER
            if(BtDevice_IsMyAddressPrimary())
#endif
            {
                /* Update QoS value during DFU data transfer phase */
                if(UpgradeIsResumePointStart())
                {
                    /* Set QOS to low latency over BLE connection even if it is set previously.
                     */
                    Dfu_RequestQOS();
                }
                /* Inform upgrade protocol connect to the application */
                TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_TRANSPORT_CONNECTED);
            }
        }
            break;

        case upgrade_notify_transport_disconnect:
        {
#ifdef INCLUDE_DFU_PEER
            if(BtDevice_IsMyAddressPrimary())
#endif
            {
                DEBUG_LOG_INFO("dfu_HandleUpgradeTransportNotification exit");
                /* Release QOS which was earlier requested over BLE connection,
                 * and set using Dfu_RequestQOS() call.
                 */
                Dfu_ReleaseQOS();
                /* Inform upgrade protocol disconnect to the application */
                TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), DFU_TRANSPORT_DISCONNECTED);
            }

#ifdef INCLUDE_DFU_CASE
            /* Inform the dfu_case about transport disconnection if case DFU is in progress. */
            if(UpgradeIsCaseDfuInProgress())
            {
                DfuCase_HandleTransportDisconnect();
            }
#endif
        }
            break;

        default:
            DEBUG_LOG_ERROR("dfu_HandleUpgradeTransportNotification. Unexpected state %d",notification->status);
            break;
    }
}

static void dfu_SwapImage(void)
{
    dfu_SetGattServiceUpdateFlags();
    device_t upgrade_device = BtDevice_GetUpgradeDevice();
    /* SILENT_COMMIT : During DFU, if USER chooses option to install update 
       "LATER" in AG means SILENT commit is enabled.
       In other words, earbud may not need to remain active for handset to 
       complete the commit phase. 
       Therefore , if earbuds goes into case before COMMIT triggered by AG
       then earbuds independent of AG and applies the new DFU image and 
       won't need AG to complete the COMMIT. 
       INTERACTIVE_COMMIT : During DFU, if USER chooses option to install update
       "NOW" in AG means INTERACTIVE commit is enabled. 

       Setting the MRU flag for upgrade device only when SILENT_COMMIT is not 
       enabled so after DFU reboot,upgrade handset is tried for re-connection first. */
    if ((upgrade_device != NULL) && !Dfu_IsSilentCommitEnabled())
    {
        bdaddr handset_addr = DeviceProperties_GetBdAddr(upgrade_device);
        DEBUG_LOG("dfu_SwapImage upgrade_device 0x%p [%04x,%02x,%06lx]",
                  upgrade_device, 
                  handset_addr.nap,
                  handset_addr.uap,
                  handset_addr.lap);
        appDeviceUpdateMruDevice(&handset_addr);

        /* Store device data in ps */
        DeviceDbSerialiser_Serialise();
    }

    /* Tear down all the ACL connections before rebooting the device */
    ConManagerTerminateAllAcls(Dfu_GetTask());

    /* Set the reboot_required_on_acl_close flag as we need to check this flag before reboot */
    Dfu_GetTaskData()->dfu_reboot_pending_on_acl_close = TRUE;
}

static void dfu_HandleUpgradeShutAudio(void)
{
    DEBUG_LOG("dfu_HandleUpgradeShutAudio");
    dfu_SwapImage();
}


static void dfu_HandleUpgradeCopyAudioImageOrSwap(void)
{
    DEBUG_LOG("dfu_HandleUpgradeCopyAudioImageOrSwap");
    dfu_SwapImage();
}

#ifdef INCLUDE_DFU_PEER
static void dfu_PeerDeviceNotInUseTx(void)
{
    bool is_primary = BtDevice_IsMyAddressPrimary();

    DEBUG_LOG("dfu_PeerDeviceNotInUseTx is_primary:%d", is_primary);

    if (is_primary)
    {
        dfu_peer_device_not_in_use_t *ind = PanicUnlessMalloc(sizeof(dfu_peer_device_not_in_use_t));
        memset(ind, 0, sizeof(dfu_peer_device_not_in_use_t));

        /* send device_not_in_use indication to secondary device */
        appPeerSigMarshalledMsgChannelTx(Dfu_GetTask(),
                                        PEER_SIG_MSG_CHANNEL_DFU,
                                        ind, MARSHAL_TYPE(dfu_peer_device_not_in_use_t));
    }
}

static void dfu_PeerDeviceNotInUseRx(void)
{
    DEBUG_LOG("dfu_PeerDeviceNotInUseRx");
    Dfu_HandleDeviceNotInUse();
}

static void dfu_PeerSetContextTx(upgrade_context_t context)
{
    DEBUG_LOG("dfu_PeerSetContextTx context %d", context);

    dfu_peer_set_context_t *ind = PanicUnlessMalloc(sizeof(dfu_peer_set_context_t));
    memset(ind, 0, sizeof(dfu_peer_set_context_t));
    ind->context = (uint16) context;

    /* send dfu_peer_set_context_t indication to secondary device */
    appPeerSigMarshalledMsgChannelTx(Dfu_GetTask(),
                                    PEER_SIG_MSG_CHANNEL_DFU,
                                    ind, MARSHAL_TYPE(dfu_peer_set_context_t));
}

static void dfu_PeerSetContextRx(dfu_peer_set_context_t *msg)
{
    DEBUG_LOG("dfu_PeerSetContextRx context %d", msg->context);
    Upgrade_SetContext((upgrade_context_t) msg->context);
}

static void dfu_PeerSyncRevertRebootTx(void)
{
    DEBUG_LOG("dfu_PeerSyncRevertRebootTx");

    dfu_peer_sync_revert_reboot_t *ind = PanicUnlessMalloc(sizeof(dfu_peer_sync_revert_reboot_t));
    memset(ind, 0, sizeof(dfu_peer_sync_revert_reboot_t));

    /* Sync revert reboot request sent*/
    appPeerSigMarshalledMsgChannelTx(Dfu_GetTask(), PEER_SIG_MSG_CHANNEL_DFU, 
                                     ind, MARSHAL_TYPE(dfu_peer_sync_revert_reboot_t));
}

static void dfu_PeerSyncRevertRebootRx(void)
{
    DEBUG_LOG("dfu_PeerSyncRevertRebootRx");
    Upgrade_RevertReboot(TRUE);
}

static void dfu_PeerSyncResumePointTx(UpdateResumePoint point, uint8 is_validation_complete)
{
    DEBUG_LOG("dfu_PeerSyncResumePointTx");

    dfu_peer_sync_resume_point_t *ind = PanicUnlessMalloc(sizeof(dfu_peer_sync_resume_point_t));
    memset(ind, 0, sizeof(dfu_peer_sync_resume_point_t));
    ind->point = (uint16) point;
    ind->is_validation_complete = (uint16) is_validation_complete;

    /* Sync resume point request sent*/
    appPeerSigMarshalledMsgChannelTx(Dfu_GetTask(), PEER_SIG_MSG_CHANNEL_DFU, 
                                     ind, MARSHAL_TYPE(dfu_peer_sync_resume_point_t));
}

static void dfu_PeerSyncResumePointRx(dfu_peer_sync_resume_point_t *msg)
{
    DEBUG_LOG("dfu_PeerSyncResumePointRx");
    UpgradePeerSetResumePoint((upgrade_peer_resume_point_t)msg->point);
    UpgradePeerSetVldtnCompleteStatus((upgrade_peer_resume_point_t)msg->is_validation_complete);
    /* During link-loss or any kind of peer disconnection scenarios, we need to trigger the below event so
     * as to sync the upgrade states in both the devices during DFU
     */
    DEBUG_LOG("dfu_PeerSyncResumePointRx DFU_EVENT_RESUME_POINT_SYNC_COMPLETED");
    DfuRules_SetEvent(DFU_EVENT_RESUME_POINT_SYNC_COMPLETED);
}

static void dfu_PeerSyncVldtnCompleteTx(void)
{
    DEBUG_LOG("dfu_PeerSyncVldtnCompleteTx");

    dfu_peer_sync_vldtn_complete_t *ind = PanicUnlessMalloc(sizeof(dfu_peer_sync_vldtn_complete_t));
    memset(ind, 0, sizeof(dfu_peer_sync_vldtn_complete_t));

    /* Sync resume point request sent*/
    appPeerSigMarshalledMsgChannelTx(Dfu_GetTask(), PEER_SIG_MSG_CHANNEL_DFU, 
                                     ind, MARSHAL_TYPE(dfu_peer_sync_vldtn_complete_t));
}

static void dfu_PeerSyncVldtnCompleteRx(void)
{
    DEBUG_LOG("dfu_PeerSyncVldtnCompleteRx");
    UpgradePeerSetVldtnCompleteStatus(TRUE);

    /* Event to handle the peer validation complete scenario */
    DfuRules_SetEvent(DFU_EVENT_UPGRADE_PEER_VLDTN_COMPLETE);
}

static void dfu_HandlePeerSigMarshalledMsgChannelRxInd(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind)
{
    DEBUG_LOG("dfu_HandlePeerSigMarshalledMsgChannelRxInd. Channel 0x%x, type %d", ind->channel, ind->type);

    switch (ind->type)
    {
        case MARSHAL_TYPE_dfu_peer_device_not_in_use_t:
            {
                /* device_not_in_use indication received */
                dfu_PeerDeviceNotInUseRx();
            }
            break;

        case MARSHAL_TYPE_dfu_peer_set_context_t:
            {
                dfu_peer_set_context_t *msg = (dfu_peer_set_context_t *)ind->msg;
                /* dfu_peer_set_context indication received */
                dfu_PeerSetContextRx(msg);
            }
            break;

        case MARSHAL_TYPE_dfu_peer_sync_revert_reboot_t:
            {
                /* dfu_peer_sync_revert_reboot indication received */
                dfu_PeerSyncRevertRebootRx();
            }
            break;

        case MARSHAL_TYPE_dfu_peer_sync_resume_point_t:
            {
                dfu_peer_sync_resume_point_t *msg = (dfu_peer_sync_resume_point_t *)ind->msg;
                /* dfu_peer_sync_resume_point indication received */
                dfu_PeerSyncResumePointRx(msg);
            }
            break;

        case MARSHAL_TYPE_dfu_peer_sync_vldtn_complete_t:
            {
                 /* dfu_peer_sync_vldtn_complete indication received */
                dfu_PeerSyncVldtnCompleteRx();
            }
            break;

        default:
            break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

static void dfu_HandlePeerSigMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm)
{
    peerSigStatus status = cfm->status;

    if (peerSigStatusSuccess != status)
    {
        DEBUG_LOG("dfu_HandlePeerSigMarshalledMsgChannelTxCfm reports failure code 0x%x(%d)", status, status);
    }

}

dfu_peer_sig_l2cap_status_t Dfu_GetPeerSigL2capStatus(void)
{
	dfu_task_data_t *the_dfu = Dfu_GetTaskData();
	return the_dfu->peer_sig_l2cap_status;
}
static void dfu_SetPeerSigL2capStatus(dfu_peer_sig_l2cap_status_t status)
{
    DEBUG_LOG("dfu_SetPeerSigL2capStatus status:%d", status);
    dfu_task_data_t *the_dfu = Dfu_GetTaskData();
    the_dfu->peer_sig_l2cap_status = status;
}

static void dfu_HandlePeerSigConnectInd(const PEER_SIG_CONNECTION_IND_T *ind)
{
    DEBUG_LOG("dfu_HandlePeerSigConnectInd, status %u", ind->status);

    /*
     * Make DFU domain aware of the current device role (Primary/Secondary)
     */
    if (ind->status == peerSigStatusConnected)
    {
        dfu_task_data_t *the_dfu = Dfu_GetTaskData();
        bool is_primary = BtDevice_IsMyAddressPrimary();

        if (is_primary)
        {
            the_dfu->peerProfilesToConnect &= ~DEVICE_PROFILE_PEERSIG;
            DEBUG_LOG("dfu_HandlePeerSigConnectInd (profiles:x%x) pending to connect", the_dfu->peerProfilesToConnect);
        }
        else
        {
            /*Cancel pending UPGRADE_PEER_CONNECT_REQ, if any*/
            UpgradePeerCancelDFU();
        }

        DfuPeer_SetRole(is_primary);
		
        /* Reset the peer signaling L2CAP disconnection reason to 0 (connected).
         */
        dfu_SetPeerSigL2capStatus(dfu_peer_sig_l2cap_connected);

        /* If the reboot reason is a defined reset as part of DFU process, then
         * start the peer connection once again, and continue with commit phase
         */
        if(Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET)
        {
            DEBUG_LOG("dfu_HandlePeerSigConnectInd: UpgradePeerApplicationReconnect()");
            /* Device is  restarted in upgrade process, send connect request again */
            UpgradePeerApplicationReconnect();
        }
        else
        {
            DfuRules_SetEvent(DFU_EVENT_PEER_SIG_CONNECT_IND);
        }

    }
    /* In Panic situation, the peer device gets disconneted and peerSigStatusDisconnected is sent by peer_signalling which needs to be handled */
    else if (ind->status == peerSigStatusLinkLoss || ind->status == peerSigStatusDisconnected)
    {
        /*
         * In the post reboot DFU commit phase, now main role (Primary/Secondary)
         * are no longer fixed rather dynamically selected by Topology using role
         * selection. This process may take time so its recommendable to reset
         * this reconnection timer in linkloss scenarios (if any) in the post
         * reboot DFU commit phase.
         */
        UpgradeRestartReconnectionTimer();

        if(ind->status == peerSigStatusLinkLoss)
    	{
    		/* Reset the peer signaling L2CAP disconnection reason to 1 (link loss).
		     */
		    dfu_SetPeerSigL2capStatus(dfu_peer_sig_l2cap_link_loss);
    	}
    }
}

static void dfu_HandleConManagerConnectionInd(const CON_MANAGER_CONNECTION_IND_T* ind)
{
    bool is_upgrade_in_progress = Dfu_IsUpgradeInProgress();
    bool is_primary = BtDevice_IsMyAddressPrimary();

    DEBUG_LOG("dfu_HandleConManagerConnectionInd Conn:%u BLE:%u %04x,%02x,%06lx", ind->connected,
                                                                                          ind->ble,
                                                                                          ind->bd_addr.nap,
                                                                                          ind->bd_addr.uap,
                                                                                          ind->bd_addr.lap);
    if(!ind->ble && appDeviceIsPeer(&ind->bd_addr) && ind->connected && is_upgrade_in_progress && is_primary)
    {
        dfu_task_data_t *the_dfu = Dfu_GetTaskData();
        the_dfu->peerProfilesToConnect = appPhyStateGetState() == PHY_STATE_IN_CASE ?
                        DEVICE_PROFILE_PEERSIG : TwsTopologyConfig_PeerProfiles();
        DEBUG_LOG("dfu_HandleConManagerConnectionInd PEER BREDR Connected (profiles:x%x) to connect", the_dfu->peerProfilesToConnect);
    }
}
#endif

static void dfu_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("dfu_MessageHandler. MESSAGE:dfu_internal_messages_t:0x%X", id);

    switch (id)
    {
#ifdef INCLUDE_DFU_PEER
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            dfu_HandlePeerSigMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *)message);
            break;

        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            dfu_HandlePeerSigMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *)message);
            break;

        case CON_MANAGER_CONNECTION_IND:
            dfu_HandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

        case PEER_SIG_CONNECTION_IND:
            dfu_HandlePeerSigConnectInd((const PEER_SIG_CONNECTION_IND_T *)message);
            break;

        /* MIRROR PROFILE MESSAGES */
        case MIRROR_PROFILE_CONNECT_IND:
            {
                dfu_task_data_t *the_dfu = Dfu_GetTaskData();
                bool is_primary = BtDevice_IsMyAddressPrimary();

                if (is_primary)
                {
                    the_dfu->peerProfilesToConnect &= ~DEVICE_PROFILE_MIRROR;
                    DEBUG_LOG("dfu_MessageHandler (profiles:x%x) pending to connect", the_dfu->peerProfilesToConnect);
                }
            }
            break;

        case HANDOVER_PROFILE_CONNECTION_IND:
            {
                dfu_task_data_t *the_dfu = Dfu_GetTaskData();
                bool is_primary = BtDevice_IsMyAddressPrimary();

                if (is_primary)
                {
                    the_dfu->peerProfilesToConnect &= ~DEVICE_PROFILE_HANDOVER;
                    DEBUG_LOG("dfu_MessageHandler (profiles:x%x) pending to connect", the_dfu->peerProfilesToConnect);
                }
            }
            break;
#endif

        case CON_MANAGER_CLOSE_ALL_CFM:
            {
                if(Dfu_GetTaskData()->dfu_reboot_pending_on_acl_close)
                {
                    DEBUG_LOG_INFO("dfu_MessageHandler. CON_MANAGER_CLOSE_ALL_CFM Call UpgradeImageSwap()");
                    Dfu_GetTaskData()->dfu_reboot_pending_on_acl_close = FALSE;
                    UpgradeImageSwap();
                }
            }
            break;

            /* Message sent in response to UpgradeInit().
             * In this case we need to forward to the app to unblock initialisation.
             */
        case UPGRADE_INIT_CFM:
            {
                const UPGRADE_INIT_CFM_T *init_cfm = (const UPGRADE_INIT_CFM_T *)message;

                DEBUG_LOG("dfu_MessageHandler. UPGRADE_INIT_CFM %d (sts)",init_cfm->status);

                dfu_ForwardInitCfm(init_cfm);
            }
            break;

            /* Message sent during initialisation of the upgrade library
                to let the VM application know that a restart has occurred
                and reconnection to a host may be required. */
        case UPGRADE_RESTARTED_IND:
            dfu_HandleRestartedInd((UPGRADE_RESTARTED_IND_T*)message);
            break;

            /* Message sent to application to request applying a downloaded upgrade.
                Note this may include a warm reboot of the device.
                Application must respond with UpgradeApplyResponse() */
        case UPGRADE_APPLY_IND:
            {
#ifdef INCLUDE_DFU_PEER
                bool isPrimary = BtDevice_IsMyAddressPrimary();
                dfu_task_data_t *the_dfu = Dfu_GetTaskData();

                DEBUG_LOG("dfu_MessageHandler UPGRADE_APPLY_IND, isPrimary:%d", isPrimary);
#ifndef HOSTED_TEST_ENVIRONMENT
                if (isPrimary)
                {
                    /*
                     * As per the legacy scheme, Primary reboots post Secondary
                     * having rebooted. And as part of Secondary reboot, the
                     * peer links (including DFU L2CAP channel) are
                     * re-established. Wait for the connections of these other
                     * peer profiles to complete before Primary reboot, in order
                     * to avoid undefined behavior on the Secondary
                     * (such as panic on asserts) owing to invalid connection
                     * handles while handling disconnection sequence because
                     * of linkloss to Primary if the Primary didn't await for
                     * the peer profile connections to complete.
                     * Since no direct means to cancel the peer connection from
                     * DFU domain except Topology which can cancel through
                     * cancellable goals, for now its better to wait for the
                     * peer profile connections to be done before Primary
                     * reboot for a deterministic behavior and avoid the problem
                     * as described above.
                     *
                     * (Note: The invalid connection handle problem was seen
                     *        with Mirror Profile.)
                     *
                     */
                    MessageSendConditionally(Dfu_GetTask(),
                                            DFU_INTERNAL_UPGRADE_APPLY_RES_ON_PEER_PROFILES_CONNECTED, NULL,
                                            (uint16 *)&the_dfu->peerProfilesToConnect);
                }
                else
#endif
#endif
                {
                    DEBUG_LOG("dfu_MessageHandler. UPGRADE_APPLY_IND saying now !");
                    dfu_NotifyActivity();
                    if (Dfu_GetTaskData()->reboot_permission_required)
                    {
                        dfu_NotifyReadyToReboot();
                    }
                    else
                    {
                        UpgradeApplyResponse(0);
                    }
                }
            }
            break;

            /* Message sent to application to request blocking the system for an extended
                period of time to erase serial flash partitions.
                Application must respond with UpgradeBlockingResponse() */
        case UPGRADE_BLOCKING_IND:
            DEBUG_LOG("dfu_MessageHandler. UPGRADE_BLOCKING_IND");
            dfu_NotifyActivity();
            UpgradeBlockingResponse(0);
            break;

            /* Message sent to application to indicate that blocking operation is finished */
        case UPGRADE_BLOCKING_IS_DONE_IND:
            DEBUG_LOG("dfu_MessageHandler. UPGRADE_BLOCKING_IS_DONE_IND");
            dfu_NotifyActivity();
            break;

            /* Message sent to application to inform of the current status of an upgrade. */
        case UPGRADE_STATUS_IND:
            dfu_HandleUpgradeStatusInd((const UPGRADE_STATUS_IND_T *)message);
            break;

            /* Message recieved from upgrade library to handle upgrade operations */
        case UPGRADE_OPERATION_IND:
            dfu_HandleUpgradeOperationInd((const UPGRADE_OPERATION_IND_T *)message);
            break;

            /* Message received from upgrade library to get transport connection status */
        case UPGRADE_NOTIFY_TRANSPORT_STATUS:
            dfu_HandleUpgradeTransportNotification((const UPGRADE_NOTIFY_TRANSPORT_STATUS_T *)message);
            break;

#ifdef INCLUDE_DFU_CASE
            /* Message received from upgrade library to inform dfu_case of a case related operation. */
        case UPGRADE_CASE_DFU_CASE_IMAGE_IND:
        case UPGRADE_CASE_DFU_EARBUD_IMAGE_IND:
        case UPGRADE_CASE_DFU_RESUME_IND:
        case UPGRADE_CASE_DFU_CASE_ABORT:
            DfuCase_HandleLibOperation(id, message);
            break;
#endif

            /* Message sent to application to request any audio to get shut */
        case UPGRADE_SHUT_AUDIO:
            dfu_HandleUpgradeShutAudio();
            break;
            
            /* Message sent to application to inform that upgrade is ready for the silent commit. */
        case UPGRADE_READY_FOR_SILENT_COMMIT:
            dfu_NotifyReadyforSilentCommit();
            break;

            /* Message sent to application set the audio busy flag and copy audio image */
        case UPRGADE_COPY_AUDIO_IMAGE_OR_SWAP:
            dfu_HandleUpgradeCopyAudioImageOrSwap();
            break;

            /* Message sent to application to reset the audio busy flag */
        case UPGRADE_AUDIO_COPY_FAILURE:
            DEBUG_LOG("dfu_MessageHandler. UPGRADE_AUDIO_COPY_FAILURE (not handled)");
            break;

            /* Message sent to application to inform that the actual upgrade has started */
        case UPGRADE_START_DATA_IND:
            {    
#ifdef INCLUDE_DFU_PEER
                bool is_primary = BtDevice_IsMyAddressPrimary();

                uint32 md5_checksum = UpgradeGetMD5Checksum();

                DEBUG_LOG("dfu_MessageHandler UPGRADE_START_DATA_IND, is_primary:%d", is_primary);

                UpgradePeerStoreMd5(md5_checksum);

                /* Don't let earbuds go in sleep during the data transfer for earbud
                 * and case dfu
                 */
                DEBUG_LOG_VERBOSE("dfu_HandleUpgradeOperationInd Prohibit earbud to sleep");
                appPowerClientProhibitSleep(Dfu_GetTask());

                /* Send this event to start the peer DFU */
                DfuRules_SetEvent(DFU_EVENT_UPGRADE_START_DATA_IND);
#endif
                dfu_NotifyStart();
            }
            break;

            /* Message sent to application to inform that the actual upgrade has ended */
        case UPGRADE_END_DATA_IND:
            {
                UPGRADE_END_DATA_IND_T *end_data_ind = (UPGRADE_END_DATA_IND_T *)message;
                DEBUG_LOG("dfu_MessageHandler. UPGRADE_END_DATA_IND %d (handled for abort indication)", end_data_ind->state);

                /* Notify application that upgrade has ended owing to abort. */
                if (end_data_ind->state == upgrade_end_state_abort)
                {
                    dfu_NotifyAborted();
                    /* To Do: remove when merging GAA resume changes because context gets cleared
                       as a part of upgrade pskey */
                    Upgrade_SetContext(UPGRADE_CONTEXT_UNUSED);
#ifdef INCLUDE_DFU_PEER

                    /* If DFU is aborted due to SYNC ID mismatch, then prohibit
                     * device to sleep for handling the subsequent SYNC request
                     */
                    if(!UpgradeIsAbortDfuDueToSyncIdMismatch())
                    {
                        /* DFU is aborted. Allow device to sleep */
                        DEBUG_LOG_VERBOSE("dfu_MessageHandler Allow earbud to sleep");
                        appPowerClientAllowSleep(Dfu_GetTask());
                    }
#endif
                }
                DfuRules_ResetEvent(RULE_EVENT_ALL_EVENTS_MASK);
            }
            break;

        case UPGRADE_RESUME_IND:
            DfuRules_SetEvent(DFU_EVENT_UPGRADE_RESUME);
        break;

            /* Message sent to application to inform for cleaning up DFU state variables on Abort */
        case UPGRADE_CLEANUP_ON_ABORT:
            DEBUG_LOG("dfu_MessageHandler. UPGRADE_CLEANUP_ON_ABORT");
            dfu_NotifyAbort();
            break;

#ifdef INCLUDE_DFU_PEER
            case DFU_INTERNAL_UPGRADE_APPLY_RES_ON_PEER_PROFILES_CONNECTED:
            {
                DEBUG_LOG("dfu_MessageHandler. DFU_INTERNAL_UPGRADE_APPLY_RES_ON_PEER_PROFILES_CONNECTED, Respond to UPGRADE_APPLY_IND now!");
                dfu_NotifyActivity();
                UpgradeApplyResponse(0);
            }
            break;
#endif

            /* Set appropriate reboot reason if a commit is reverted or 
             * unexpected reset of device encountered during post reboot phase.
             */
        case UPGRADE_REVERT_RESET:
            DEBUG_LOG_DEBUG("dfu_MessageHandler. UPGRADE_REVERT_RESET");
#ifndef INCLUDE_DFU_PEER
            /* Only the headset app is using this reason. It needs to be cleared after the usage. */
            Dfu_SetRebootReason(REBOOT_REASON_REVERT_RESET);
#endif
            break;

        case UPGRADE_SYNC_REVERT_REBOOT:
#ifdef INCLUDE_DFU_PEER
            DEBUG_LOG_DEBUG("dfu_MessageHandler. UPGRADE_SYNC_REVERT_REBOOT");
            /*Signal the peer about revert*/
            dfu_PeerSyncRevertRebootTx();
#endif
            break;

        case UPGRADE_REBOOT_IND:
            DfuRules_SetEvent(DFU_EVENT_UPGRADE_TRANSFER_COMPLETE_RES);
            break;

        case DFU_INTERNAL_CONTINUE_HASH_CHECK_REQUEST:
            {
                DEBUG_LOG_INFO("dfu_MessageHandler. DFU_INTERNAL_CONTINUE Hash Checking");
                UpgradeStartValidation();
            }
            break;

        case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
            DEBUG_LOG("dfu_MessageHandler. MESSAGE_IMAGE_UPGRADE_ERASE_STATUS");

            dfu_NotifyActivity();

            UpgradeHandleSystemMessges(id, message);
            break;

        case MESSAGE_IMAGE_UPGRADE_COPY_STATUS:
            DEBUG_LOG("dfu_MessageHandler. MESSAGE_IMAGE_UPGRADE_COPY_STATUS");

            dfu_NotifyActivity();
            UpgradeHandleSystemMessges(id, message);
            break;

        case MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS:
            DEBUG_LOG("dfu_MessageHandler. MESSAGE_IMAGE_UPGRADE_HASH_ALL_SECTIONS_UPDATE_STATUS");
            UpgradeHandleSystemMessges(id, message);
            break;

#ifdef INCLUDE_DFU_PEER
        case APP_POWER_SLEEP_PREPARE_IND:
            DEBUG_LOG_INFO("dfu_MessageHandler APP_POWER_SLEEP_PREPARE_IND");
            appPowerSleepPrepareResponse(Dfu_GetTask());
            break;
        
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            DEBUG_LOG_INFO("dfu_MessageHandler APP_POWER_SHUTDOWN_PREPARE_IND");
            appPowerShutdownPrepareResponse(Dfu_GetTask());
            break;
#endif
        /* Handle DFU rules */
        case DFU_RULES_START_VALIDATION:
            DEBUG_LOG_INFO("dfu_MessageHandler DFU_RULES_START_VALIDATION");
            UpgradeStartValidation();
            DfuRules_SetRuleComplete(DFU_RULES_START_VALIDATION);
            break;

        case DFU_RULES_RELAY_RESUME_POINT_INFO:
#ifdef INCLUDE_DFU_PEER
            /* Send the resume point info to peer device */
            dfu_PeerSyncResumePointTx(UpgradeGetResumePoint(), UpgradeIsValidationComplete());
#endif
            DfuRules_SetRuleComplete(DFU_RULES_RELAY_RESUME_POINT_INFO);
            break;

         case DFU_RULES_REVERT_UPGRADE:
            /* Reboot the device */
            Upgrade_RevertReboot(FALSE);
            break;

        case DFU_RULES_RELAY_TRANSFER_COMPLETE:
#ifdef INCLUDE_DFU_PEER
            /* Send the resume point info to peer device */
            dfu_PeerSyncResumePointTx(UpgradeGetResumePoint(), UpgradeIsValidationComplete());
#endif
            DfuRules_SetRuleComplete(DFU_RULES_RELAY_TRANSFER_COMPLETE);
            break;

        case DFU_RULES_VALIDATION_COMPLETE:
            UpgradeHandleValidationComplete();
            DfuRules_SetRuleComplete(DFU_RULES_VALIDATION_COMPLETE);
            break;

        case DFU_RULES_START_PEER_DFU:
#ifdef INCLUDE_DFU_PEER
            UpgradePeerStartDfu();
#endif
            DfuRules_SetRuleComplete(DFU_RULES_START_PEER_DFU);
            break;

        case DFU_RULES_START_REBOOT:
            DEBUG_LOG_INFO("dfu_MessageHandler DFU_RULES_START_REBOOT");
            UpgradeSMHandleRebootAction();
            DfuRules_SetRuleComplete(DFU_RULES_START_REBOOT);
            break;

            /* Catch-all panic for unexpected messages */
        default:
            if (UPGRADE_UPSTREAM_MESSAGE_BASE <= id && id <  UPGRADE_UPSTREAM_MESSAGE_TOP)
            {
                DEBUG_LOG_ERROR("dfu_MessageHandler. Unexpected upgrade library message MESSAGE:0x%x", id);
            }
#ifdef INCLUDE_DFU_PEER
            else if (PEER_SIG_INIT_CFM <= id && id <= PEER_SIG_LINK_LOSS_IND)
            {
                DEBUG_LOG("dfu_MessageHandler. Unhandled peer sig message MESSAGE:0x%x", id);
            }
#endif
            else
            {
                DEBUG_LOG_ERROR("dfu_MessageHandler. Unexpected message MESSAGE:dfu_internal_messages_t:0x%X", id);
            }
            break;
    }

}

static void dfu_SetGattServiceUpdateFlagForHandset(device_t device, void *data)
{
    UNUSED(data);
    if(BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
    {
        if(!BtDevice_IsFirstConnectAfterDFU(device))
        {
            appDeviceSetFirstConnectAfterDFU(device, TRUE);
            DeviceDbSerialiser_SerialiseDevice(device);
        }
    }
}

static void dfu_SetGattServiceUpdateFlags(void)
{
    DeviceList_Iterate(dfu_SetGattServiceUpdateFlagForHandset, NULL);
}

static void dfu_GattConnect(gatt_cid_t cid)
{
    DEBUG_LOG("dfu_GattConnect. cid:0x%X", cid);
    device_t device = NULL;

    device = GattConnect_GetBtDevice(cid);
    if(device)
    {
        DEBUG_LOG("dfu_GattConnect retrieving property device=0x%p", device);
        if(BtDevice_IsFirstConnectAfterDFU(device))
        {
            GattServerGatt_SetServerServicesChanged(cid);
            appDeviceSetFirstConnectAfterDFU(device, FALSE);
        }
    }
}

static void dfu_GattDisconnect(gatt_cid_t cid)
{
    DEBUG_LOG("dfu_GattDisconnect. cid:0x%X", cid);

    /* We choose not to do anything when GATT is disconnect */
}

static void dfu_GetVersionInfo(dfu_VersionInfo *ver_info)
{
    *ver_info = Dfu_GetTaskData()->verInfo;
}

bool Dfu_AllowUpgrades(bool allow)
{
    upgrade_status_t sts = (upgrade_status_t)-1;
    bool successful = FALSE;

    /* The Upgrade library API can panic very easily if UpgradeInit had
       not been called previously */
    if (SystemState_GetState() > system_state_initialised)
    {
        upgrade_permission_t permission = upgrade_perm_no;

        if (allow && Dfu_GetTaskData()->reboot_permission_required)
        {
            permission = upgrade_perm_always_ask;
        }
        else if (allow)
        {
            permission = upgrade_perm_assume_yes;
        }

         sts = UpgradePermit(permission);
         successful = (sts == upgrade_status_success);
    }

    DEBUG_LOG("Dfu_AllowUpgrades(%d) - success:%d (sts:%d)", allow, successful, sts);

    return successful;
}

void Dfu_RequireRebootPermission(bool permission_required)
{
    DEBUG_LOG("Dfu_RequireRebootPermission %u", permission_required);
    Dfu_GetTaskData()->reboot_permission_required = permission_required;
}

void Dfu_RebootConfirm(void)
{
    DEBUG_LOG("Dfu_RebootConfirm rebooting now");
    UpgradeApplyResponse(0);
}

void Dfu_ClientRegister(Task tsk)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(Dfu_GetClientList()), tsk);
}

/* Since primary device is the one connected to Host, Dfu_SetContext will be 
 * called only on primary device and not for secondary device.
 */
void Dfu_SetContext(upgrade_context_t context)
{
    /* Upgrade_SetContext sets the context of the upgrade module in a PS Key
     * whereas Upgrade_SetHostType is used to set a variable in upgrade 
     * context which is used to decide the resume methodology for Primary 
     * EB to Secondary EB resume. (As of now PEB to SEB transfer is via GAIA).
     */
    Upgrade_SetContext(context);

#ifdef INCLUDE_DFU_PEER
    /* Synchronize the upgrade context with peer.*/
    dfu_PeerSetContextTx(context);
#endif /* INCLUDE_DFU_PEER */
}

upgrade_context_t Dfu_GetContext(void)
{
    return Upgrade_GetContext();
}

/*! \brief Return REBOOT_REASON_DFU_RESET for defined reboot phase of upgrade
           else REBOOT_REASON_ABRUPT_RESET for abrupt reset.
 */
dfu_reboot_reason_t Dfu_GetRebootReason(void)
{
    return Dfu_GetTaskData()->dfu_reboot_reason;
}

/*! \brief Set to REBOOT_REASON_DFU_RESET for defined reboot phase of upgrade
           else REBOOT_REASON_ABRUPT_RESET for abrupt reset.
 */
void Dfu_SetRebootReason(dfu_reboot_reason_t val)
{
    Dfu_GetTaskData()->dfu_reboot_reason = val;
}

extern bool UpgradePSClearStore(void);
/*! \brief Clear upgrade related PSKeys.
 */
bool Dfu_ClearPsStore(void)
{
    /* Clear out any in progress DFU status */
    return UpgradePSClearStore();
}

void Dfu_HandleDeviceNotInUse(void)
{
    DEBUG_LOG_INFO("Dfu_HandleDeviceNotInUse: Initiate DFU reboot");

#ifdef INCLUDE_DFU_PEER
    /* Inform the peer about device not in use. */
    dfu_PeerDeviceNotInUseTx();
#endif /* INCLUDE_DFU_PEER */

    UpgradeRebootForSilentCommit();
}

bool Dfu_IsSilentCommitEnabled(void)
{
    return UpgradeIsSilentCommitEnabled();
}

bool Dfu_IsUpgradeInProgress(void)
{
    return UpgradeIsInitialised() && (UpgradeInProgressId() != 0);
}

void Dfu_SetVersionInfo(uint16 uv_major, uint16 uv_minor, uint16 cfg_ver)
{
    Dfu_GetTaskData()->verInfo.upgrade_ver.major =  uv_major;
    Dfu_GetTaskData()->verInfo.upgrade_ver.minor = uv_minor;
    Dfu_GetTaskData()->verInfo.config_ver = cfg_ver;
}

void Dfu_SetSilentCommitSupported(uint8 is_silent_commit_supported)
{
    DEBUG_LOG_INFO("Dfu_SetSilentCommitSupported: is_silent_commit_supported %d",
                    is_silent_commit_supported);
    UpgradeSetSilentCommitSupported(is_silent_commit_supported);
}

void Dfu_RequestQOS(void)
{
    tp_bdaddr tpaddr ;
    device_t upgrade_device = BtDevice_GetUpgradeDevice();
    if (upgrade_device != NULL)
    {
        bdaddr handset_addr = DeviceProperties_GetBdAddr(upgrade_device);

        memset(&tpaddr, 0, sizeof(tp_bdaddr));
        tpaddr.transport  = TRANSPORT_BLE_ACL;
        tpaddr.taddr.type = TYPED_BDADDR_PUBLIC;
        tpaddr.taddr.addr = handset_addr;

        DEBUG_LOG_INFO("Dfu_RequestQos: for BLE transport set QOS to low latency during DFU");
        ConManagerRequestDeviceQos(&tpaddr, cm_qos_low_latency);
        Dfu_GetTaskData()->is_qos_update_requested_during_dfu = TRUE;
    }
}

void Dfu_ReleaseQOS(void)
{
    tp_bdaddr tpaddr ;
    device_t upgrade_device = BtDevice_GetUpgradeDevice();

    /* QOS release from cm_qos_low_latency if the current QOS is
     * set as cm_qos_low_latency due to Dfu_RequestQOS() call
     */
    if ((upgrade_device != NULL) && (Dfu_GetTaskData()->is_qos_update_requested_during_dfu))
    {
        bdaddr handset_addr = DeviceProperties_GetBdAddr(upgrade_device);

        memset(&tpaddr, 0, sizeof(tp_bdaddr));
        tpaddr.transport  = TRANSPORT_BLE_ACL;
        tpaddr.taddr.type = TYPED_BDADDR_PUBLIC;
        tpaddr.taddr.addr = handset_addr;

        DEBUG_LOG_INFO("Dfu_ReleaseQos: for BLE transport release QOS from low latency");
        ConManagerReleaseDeviceQos(&tpaddr, cm_qos_low_latency);
    }
    /* Set this to FALSE while release QoS call, as older values might exist due to role
     * switch/handover sometimes for Earbuds. Also, there is no harm in setting
     * it to FALSE irrespective of whether release QoS took place or not, once
     * the DFU is over.
     */
    Dfu_GetTaskData()->is_qos_update_requested_during_dfu = FALSE;
}

uint32 Dfu_GetFileOffset(void)
{
    return UpgradeGetDfuFileOffset();
}

#ifdef INCLUDE_DFU_PEER

void Dfu_UpgradeHostRspSwap(bool is_primary)
{
    if(!is_primary)
    {
        UpgradeSetFPtr(UpgradePeerGetFPtr());
    }
    else
    {
        UpgradeSetFPtr(UpgradeGetFPtr());
    }
}

void Dfu_ProhibitSleep(void)
{
    /* Don't let earbuds go in sleep during the data transfer */
    appPowerClientProhibitSleep(Dfu_GetTask());
}

void Dfu_HandlePeerChannelConnection(void)
{
    DEBUG_LOG("Dfu_HandlePeerChannelConnection RebootReason %u, is_primary %u, silent_commit %u", 
    Dfu_GetRebootReason(), BtDevice_IsMyAddressPrimary(), Dfu_IsSilentCommitEnabled());

    if(Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET &&
       Dfu_IsSilentCommitEnabled())
    {
        if(!BtDevice_IsMyAddressPrimary())
        {
            /* During the post reboot state, secondary EB should start the silent commit
             * process by sending the commit_req. */
            UpgradeSmHandleCommitVerifyProceed();
        }
    }
}

void Dfu_HandleCommitReqFromPeer(void)
{
    DEBUG_LOG("Dfu_HandleCommitReqFromPeer RebootReason %u, is_primary %u, silent_commit %u", 
    Dfu_GetRebootReason(), BtDevice_IsMyAddressPrimary(), Dfu_IsSilentCommitEnabled());
    if(Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET &&
       BtDevice_IsMyAddressPrimary())
    {
        if(Dfu_IsSilentCommitEnabled())
        {
            /* Primary EB is in correct state for silent commit so,
             * inform peer device to procceed.  */
            UpgradePeerProcessHostMsg(UPGRADE_PEER_COMMIT_CFM, UPGRADE_CONTINUE);
        }
        else
        {
            /* Just inform the upgrade lib about the commit_req for interective commit. */
            UpgradeCommitMsgFromUpgradePeer();
        }
    }
}

void Dfu_HandleCompleteIndFromPeer(uint8 status)
{
    DEBUG_LOG("Dfu_HandleCompleteIndFromPeer RebootReason %u, is_primary %u, silent_commit %u, commit status %u", 
    Dfu_GetRebootReason(), BtDevice_IsMyAddressPrimary(), Dfu_IsSilentCommitEnabled(), status);

    if(Dfu_GetRebootReason() == REBOOT_REASON_DFU_RESET &&
       BtDevice_IsMyAddressPrimary())
    {
        if(Dfu_IsSilentCommitEnabled())
        {
            /* Peer device has completed the silent commit. Ask the upgrade lib to commit.
             * Also start the peer channel disconnection as the DFU is over. */
            UpgradeCommitConfirmForSilentCommit();
            DfuPeer_InitiatePeerLinkDisconnection();
        }
        else
        {
            /* Just inform the upgrade lib about the complete_ind for interective commit. */
            UpgradeCompleteMsgFromUpgradePeer(status);
        }
    }
}

void Dfu_SyncResumePointWithPeer(void)
{
    DEBUG_LOG("Dfu_SyncResumePointWithPeer");
    DfuRules_SetEvent(DFU_EVENT_RESUME_POINT_SYNC);
}

void Dfu_HandleProcessCompleteFromPeer(void)
{
    DEBUG_LOG("Dfu_HandleProcessCompleteFromPeer");
    DfuRules_SetEvent(DFU_EVENT_UPGRADE_PEER_PROCESS_COMPLETE);
}

#endif /* INCLUDE_DFU_PEER */

#endif /* INCLUDE_DFU */
