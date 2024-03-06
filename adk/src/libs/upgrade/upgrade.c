/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade.c

DESCRIPTION
    Upgrade library API implementation.
*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>
DEBUG_LOG_DEFINE_LEVEL_VAR

#include <string.h>
#include <stdlib.h>
#include <boot.h>
#include <message.h>
#include <byte_utils.h>
#include <panic.h>

#include "upgrade_ctx.h"
#include "upgrade_private.h"
#include "upgrade_sm.h"
#include "upgrade_host_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_msg_vm.h"
#include "upgrade_msg_internal.h"
#include "upgrade_host_if_data.h"

#include "upgrade_partition_data.h"
#include "upgrade_partition_offset_calculator.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(upgrade_application_message)
LOGGING_PRESERVE_MESSAGE_TYPE(upgrade_transport_message)
LOGGING_PRESERVE_MESSAGE_TYPE(UpgradeMsgHost)
LOGGING_PRESERVE_MESSAGE_TYPE(UpgradeMsgInternal)
LOGGING_PRESERVE_MESSAGE_TYPE(UpgradePartitionDataState)

static void SMHandler(Task task, MessageId id, Message message);
static void SendUpgradeInitCfm(Task task, upgrade_status_t status);
static void RequestApplicationReconnectIfNeeded(void);
static bool isPsKeyStartValid(uint16 dataPskeyStart);
static void UpgradeHandleCommitRevert(uint16 dataPskey, uint16 dataPskeyStart);
static void UpgradeAbortCaseDfuOnReboot(void);

const upgrade_response_functions_t Upgrade_fptr = {
    .SendSyncCfm = UpgradeHostIFDataSendSyncCfm,
    .SendShortMsg = UpgradeHostIFDataSendShortMsg,
    .SendStartCfm = UpgradeHostIFDataSendStartCfm,
    .SendBytesReq = UpgradeHostIFDataSendBytesReq,
    .SendErrorInd = UpgradeHostIFDataSendErrorInd,
    .SendIsCsrValidDoneCfm = UpgradeHostIFDataSendIsCsrValidDoneCfm,
    .SendVersionCfm = UpgradeHostIFDataSendVersionCfm,
    .SendVariantCfm = UpgradeHostIFDataSendVariantCfm,
    .SendSilentCommitSupportedCfm = UpgradeHostIFDataSendSilentCommitSupportedCfm,
    .SendCommitStatusCfm = UpgradeHostIFDataSendCommitStatus,
};

/****************************************************************************
NAME
    UpgradeGetFPtr

DESCRIPTION
    To get the Upgrade librabry fptr to be set in UpgradeCtxGet()->funcs.
*/
const upgrade_response_functions_t *UpgradeGetFPtr(void)
{
    return &Upgrade_fptr;
}

/****************************************************************************
NAME
    UpgradeInit

DESCRIPTION
    Perform initialisation for the upgrade library. This consists of fixed
    initialisation as well as taking account of the information provided
    by the application.
*/

void UpgradeInit(Task appTask,uint16 dataPskey,uint16 dataPskeyStart,
    const UPGRADE_UPGRADABLE_PARTITION_T * logicalPartitions,
    uint16 numPartitions,
    upgrade_power_management_t power_mode,
    const char * dev_variant,
    upgrade_permission_t init_perm,
    const upgrade_version *init_version,
    uint16 init_config_version)
{
    UpgradeCtx *upgradeCtx;

    DEBUG_LOG("UpgradeInit");

    upgradeCtx = PanicUnlessMalloc(sizeof(*upgradeCtx));
    memset(upgradeCtx, 0, sizeof(*upgradeCtx));
    upgradeCtx->mainTask = appTask;
    upgradeCtx->smTaskData.handler = SMHandler;

    UpgradeCtxSet(upgradeCtx);

    /* handle permission initialisation, must be an "enabled" state */
    if (   (init_perm != upgrade_perm_assume_yes)
        && (init_perm != upgrade_perm_always_ask))
    {
        Panic();
    }
    UpgradeCtxGet()->perms = init_perm;

    /* Set functions for Upgrade */
    UpgradeCtxGet()->funcs = &Upgrade_fptr;

    /* set the initial power management mode */
    UpgradeCtxGet()->power_mode = power_mode;

    /* set the initial state to battery ok, expecting sink powermanagement to soon update the state */
    UpgradeCtxGet()->power_state = upgrade_battery_ok;

    UpgradeCtxGet()->waitForPeerAbort = FALSE;

    /* set default case as success */
    UpgradeCtxGet()->peercommitStatus = IMAGE_UPGRADE_COMMIT_AND_SECURITY_UPDATE_SUCCESS;

    /* dfu_case module can set this value if it is supported. */
    UpgradeCtxGet()->isCaseDfuSupported = FALSE;
    UpgradeCtxGet()->caseDfuStatus = UPGRADE_CASE_DFU_NOT_STARTED;

    /* store the device variant */
    if(dev_variant != NULL)
    {
        strncpy(UpgradeCtxGet()->dev_variant, dev_variant, UPGRADE_HOST_VARIANT_CFM_BYTE_SIZE );
    }

    if (!isPsKeyStartValid(dataPskeyStart)
        || !UpgradePartitionsSetMappingTable(logicalPartitions,numPartitions))
    {
        SendUpgradeInitCfm(appTask, upgrade_status_unexpected_error);
        free(upgradeCtx);
        UpgradeCtxSet(NULL);
        return;
    }

    /* By default, we do not need to reboot on abort because we are running from boot bank only.
       In case if we abort at commit time(after warm reboot), we need to reboot and revert back
       to boot bank. at that time, this flag will be set to TRUE */
    UpgradeCtxGet()->isImageRevertNeededOnAbort = FALSE;

    UpgradeHandleCommitRevert(dataPskey, dataPskeyStart);
    UpgradeLoadPSStore(dataPskey,dataPskeyStart);
    UpgradeAbortCaseDfuOnReboot();

    /* @todo Need to deal with two things here
     * Being called when the PSKEY has already been set-up
     * being called for the first time. should we/can we verify partition
     * mapping
     */
    DEBUG_LOG_VERBOSE("UpgradeInit : upgrade_version major = %d, upgrade_version minor = %d and init config version = %d", init_version->major, init_version->minor, init_config_version);
    /* Initial version setting */
    if (UpgradeCtxGetPSKeys()->version.major == 0
        && UpgradeCtxGetPSKeys()->version.minor == 0)
    {
        UpgradeCtxGetPSKeys()->version = *init_version;
    }

    if (UpgradeCtxGetPSKeys()->config_version == 0)
    {
        UpgradeCtxGetPSKeys()->config_version = init_config_version;
    }

    /* Make this call before initialising the state machine so that the
       SM cannot cause the initial state to change */
    RequestApplicationReconnectIfNeeded();

    /* initialise the state machine and pass in the event that enables upgrades
     * @todo this UPGRADE_VM_PERMIT_UPGRADE event can be removed if we're always
     * starting in an upgrade enabled state, just need to initialise the state
     * machine in the correct state. */
    UpgradeSMInit();
    UpgradeSMHandleMsg(UPGRADE_VM_PERMIT_UPGRADE, 0);
    UpgradeHostIFClientConnect(&upgradeCtx->smTaskData);
    if(UpgradeInProgressId() && UpgradeCtxGetPSKeys()->upgrade_in_progress_key <= UPGRADE_RESUME_POINT_PRE_REBOOT)
    {
        UpgradePartitionOffsetCalculatorCalculateDfuOffset();
    }

    SendUpgradeInitCfm(appTask, upgrade_status_success);
}

void UpgradeSetPartitionDataBlockSize(uint32 size)
{
    UpgradeCtxGet()->partitionDataBlockSize = size;
}

/****************************************************************************
NAME
    UpgradePowerManagementSetState

DESCRIPTION
    Receives the current state of the power management from the Sink App

RETURNS

*/
upgrade_status_t UpgradePowerManagementSetState(upgrade_power_state_t state)
{
    DEBUG_LOG("UpgradePowerManagementSetState, state %u", state);

    /* if initially the power management was set to disabled, don't accept any change */
    /* we need to make sure this is called AFTER UpgradeInit is called */
    if(UpgradeCtxGet()->power_mode == upgrade_power_management_disabled)
    {
        return upgrade_status_invalid_power_state;
    }

    UpgradeCtxGet()->power_state = state;

    if(UpgradeCtxGet()->power_state == upgrade_battery_low)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_BATTERY_LOW, NULL);
    }

    return upgrade_status_success;
}

/****************************************************************************
NAME
    UpgradeGetPartitionInUse

DESCRIPTION
    Find out current physical partition for a logical partition.

RETURNS
    uint16 representing the partition that is active.
    UPGRADE_PARTITION_NONE_MAPPED is returned for an invalid partition.
*/
uint16 UpgradeGetPartitionInUse(uint16 logicalPartition)
{
    return (uint16)UpgradePartitionsPhysicalPartition(logicalPartition,UpgradePartitionActive);
}

/****************************************************************************
NAME
    UpgradeGetAppTask

DESCRIPTION
    Returns the VM application task registered with the library at
    initialisation in #UpgradeInit

RETURNS
    Task VM application task
*/
Task UpgradeGetAppTask(void)
{
    return UpgradeCtxGet()->mainTask;
}

/****************************************************************************
NAME
    UpgradeGetUpgradeTask

DESCRIPTION
    Returns the upgrade library main task.

RETURNS
    Task Upgrade library task.
*/
Task UpgradeGetUpgradeTask(void)
{
    return &UpgradeCtxGet()->smTaskData;
}

/****************************************************************************
NAME
    UpgradeHandleMsg

DESCRIPTION
    Main message handler for messages to the upgrade library from VM
    applications.
*/
void UpgradeHandleMsg(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UpgradeSMHandleMsg(id, message);
}

/****************************************************************************
NAME
    UpgradePermit

DESCRIPTION
    Control the permission the upgrade has for upgrade operations.

RETURNS
    upgrade_status_t Success or failure of requested permission type.
*/
upgrade_status_t UpgradePermit(upgrade_permission_t perm)
{
    DEBUG_LOG("UpgradePermit, perm %u", perm);
    switch (perm)
    {
        case upgrade_perm_no:
            /* if we already have an upgrade in progress, return an
             * error and do not modify our permissions */
            if (UpgradeSMUpgradeInProgress())
            {
                return upgrade_status_in_progress;
            }
            break;

        case upgrade_perm_assume_yes:
            /* fall-thru - both cases are permitting an upgrade */
        case upgrade_perm_always_ask:
            UpgradeSMHandleMsg(UPGRADE_VM_PERMIT_UPGRADE, 0);
            break;

        default:
            return upgrade_status_unexpected_error;
    }

    /* remember the permission setting */
    UpgradeCtxGet()->perms = perm;

    return upgrade_status_success;
}

/****************************************************************************
NAME
    UpgradeTransportConnectRequest

DESCRIPTION
    When a client wants to initiate an upgrade, the transport
    must first connect to the upgrade library so that it knows
    which Task to use to send messages to a client.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_CONNECT_CFM to transportTask.

*/
void UpgradeTransportConnectRequest(Task transportTask, upgrade_data_cfm_type_t cfm_type, uint32 max_request_size)
{
    DEBUG_LOG("UpgradeTransportConnectRequest, transportTask 0x%p, cfm_type %u, max_request_size %lu",
        (void *)transportTask, cfm_type, max_request_size);
    UpgradeHostIFTransportConnect(transportTask, cfm_type, max_request_size);
}

/****************************************************************************
NAME
    UpgradeProcessDataRequest

DESCRIPTION
    All data packets from a client should be sent to the Upgrade library
    via this function. Data packets must be in order but do not need
    to contain a whole upgrade message.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_DATA_CFM to the Task set in
    UpgradeTransportConnectRequest().

*/
void UpgradeProcessDataRequest(uint16 size_data, uint8 *data)
{
    DEBUG_LOG("UpgradeProcessDataRequest, size_data %u", size_data);
    (void)UpgradeHostIFProcessDataRequest(data, size_data);
}

/****************************************************************************
NAME
    UpgradeFlowControlProcessDataRequest

DESCRIPTION
    Similar to UpgradeProcessDataRequest but an appropriate wrapper to be used
    as public API.
*/

bool UpgradeFlowControlProcessDataRequest(uint8 *data, uint16 size_data)
{
    DEBUG_LOG("UpgradeFlowControlProcessDataRequest, size_data %u", size_data);
    return UpgradeHostIFProcessDataRequest(data, size_data);
}

/****************************************************************************
NAME
    UpgradeTransportDisconnectRequest

DESCRIPTION
    When a transport no longer needs to use the Upgrade
    library it must disconnect.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_DISCONNECT_CFM to the Task set in
    UpgradeTransportConnectRequest().

*/
void UpgradeTransportDisconnectRequest(void)
{
    DEBUG_LOG("UpgradeTransportDisconnectRequest");
    UpgradeHostIFTransportDisconnect();
}

/****************************************************************************
NAME
    UpgradeTransportInUse

DESCRIPTION
    Indicates whether the upgrade library currently has a transport connected.

*/
bool UpgradeTransportInUse(void)
{
    bool inUse = UpgradeHostIFTransportInUse();
    DEBUG_LOG("UpgradeTransportInUse, in_use %u", inUse);
    return inUse;
}

/****************************************************************************
NAME
    UpgradeTransportDataCfm

DESCRIPTION
    Reseting the delayDataCfm will send all messages that are conditioned on this variable.
*/
void UpgradeTransportDataCfm(void)
{
    UpgradeCtxGet()->delayDataCfm = FALSE;
}

/****************************************************************************
NAME
    UpgradeTransportScheduleDataCfm

DESCRIPTION
    Send a data confirmation message to the transport task conditionally. delayDataCfm is the condition variable.
*/
void UpgradeTransportScheduleDataCfm(void* cfm, Task transportTask)
{
    DEBUG_LOG("UpgradeTransportScheduleDataCfm confirmation message %d", cfm);

    if(cfm)
    {
        UpgradeCtxGet()->delayDataCfm = TRUE;
#ifndef HOSTED_TEST_ENVIRONMENT
        MessageSendConditionally(transportTask, UPGRADE_TRANSPORT_DATA_CFM, cfm,
                                        (uint16 *)&UpgradeCtxGet()->delayDataCfm);
#else
        MessageSend(transportTask, UPGRADE_TRANSPORT_DATA_CFM, cfm);
#endif
    }
}

void SMHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UpgradeSMHandleMsg(id, message);
}

/****************************************************************************
NAME
    UpgradeHandleSystemMessges

DESCRIPTION
    Inform the Upgrade library of a system message sent on completing an asynchronous trap.
    This function passes that message to UpgradePartitionData FSM. If it doesn't
    handle that message then passes it to Upgrade FSM.

RETURNS
    n/a
*/
void UpgradeHandleSystemMessges(MessageId id, Message message)
{
    if(!UpgradePartitionDataHandleMessage(id, message))
    {
        UpgradeSMHandleMsg(id, message);
    }
}

/****************************************************************************
NAME
    UpgradeCopyAudioStatus

DESCRIPTION
    Inform the Upgrade library of the result of an attempt to copy the Audio SQIF.

RETURNS
    n/a
*/
void UpgradeCopyAudioStatus(Message message)
{
    DEBUG_LOG_DEBUG("UpgradeCopyAudioStatus(%p)", message);
#ifdef MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS
    UpgradeSMCopyAudioStatus(message);
#endif
}

/****************************************************************************
NAME
    UpgradeApplyResponse

DESCRIPTION
    Handle application decision on applying (reboot) an upgrade.

    If the application wishes to postpone the reboot, resend the message to
    the application after the requested delay. Otherwise, push a reboot
    event into the state machine.

    @todo do we want protection against these being called by a bad application
     at the wrong time? The state machine *should* cover this.

RETURNS
    n/a
*/
void UpgradeApplyResponse(uint32 postpone)
{
    if (!postpone)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REBOOT, NULL);
    }
    else
    {
        MessageSendLater(UpgradeCtxGet()->mainTask, UPGRADE_APPLY_IND, 0, postpone);
    }
}

/****************************************************************************
NAME
    UpgradeApplyAudioShutDown

DESCRIPTION
    Sends a message to sink upgrade's main handler in order to call into sink
    audio and shut down any voice or audio streams

RETURNS
    n/a
*/
void UpgradeApplyAudioShutDown(void)
{
    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_SHUT_AUDIO, NULL);
}

/****************************************************************************
NAME
    UpgradeApplyAudioCopyFailed

DESCRIPTION
    Sends a message to sink upgrade's main handler in order to clear the audio
    busy flag should the copy of the audio image fail

RETURNS
    n/a
*/
void UpgradeApplyAudioCopyFailed(void)
{
    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_AUDIO_COPY_FAILURE, NULL);
}

/****************************************************************************
NAME
    UpgradeCopyAudioImage

DESCRIPTION
    Calls into the main state machine to invoke the trap call for the audio
    image copy

RETURNS
    n/a
*/
void UpgradeCopyAudioImage(void)
{
    UpgradeSMHandleAudioDFU();
}

/****************************************************************************
NAME
    UpgradeBlockingResponse

DESCRIPTION
    Handle application decision on blocking the system (erase).

    If the application wishes to postpone the blocking erase, resend the
    message to the application after the requested delay. Otherwise, push an
    erase event into the state machine.

    @todo do we want protection against these being called by a bad application
     at the wrong time? The state machine *should* cover this.

RETURNS
    n/a
*/
void UpgradeBlockingResponse(uint32 postpone)
{
    if (!postpone)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_ERASE, NULL);
    }
    else
    {
        MessageSendLater(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IND, 0, postpone);
    }
}

/****************************************************************************
NAME
    UpgradeRunningNewApplication

DESCRIPTION
    Query the upgrade library to see if we are part way through an upgrade.

    This is used by the application during early boot to check if the
    running application is the upgrade one but it hasn't been committed yet.

    Note: This should only to be called during the early init phase, before
          UpgradeInit has been called.

RETURNS
    TRUE if the upgraded application is running but hasn't been
    committed yet. FALSE otherwise, or in the case of an error.
*/
bool UpgradeRunningNewApplication(uint16 dataPskey, uint16 dataPskeyStart)
{
    if (UpgradeIsInitialised() || !isPsKeyStartValid(dataPskeyStart))
        return FALSE;

    if (UpgradePsRunningNewApplication(dataPskey, dataPskeyStart))
    {
        return TRUE;
    }

    return FALSE;
}

/************************************************************************************
NAME
    UpgradeSendEndUpgradeDataInd

DESCRIPTION
    To inform vm app that downloading of upgrade data from host app has ended.

RETURNS
    None
*/

void UpgradeSendEndUpgradeDataInd(upgrade_end_state_t state, uint32 message_delay)
{
    UPGRADE_END_DATA_IND_T *upgradeEndDataInd = (UPGRADE_END_DATA_IND_T *)PanicUnlessMalloc(sizeof(UPGRADE_END_DATA_IND_T));

    upgradeEndDataInd->state = state;

    DEBUG_LOG_DEBUG("UpgradeSendEndUpgradeDataInd: state enum:upgrade_end_state_t:%d, message_delay:%d", state, message_delay);

    if(message_delay)
        MessageSendLater(UpgradeCtxGet()->mainTask, UPGRADE_END_DATA_IND, upgradeEndDataInd, message_delay);
    else
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_END_DATA_IND, upgradeEndDataInd);
}

/************************************************************************************
NAME
    UpgradeSendReadyForSilentCommitInd

DESCRIPTION
    To inform vm app that silent commit command has been received from the host.

RETURNS
    None
*/

void UpgradeSendReadyForSilentCommitInd(void)
{
    DEBUG_LOG_DEBUG("UpgradeSendReadyForSilentCommitInd");
    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_READY_FOR_SILENT_COMMIT, NULL);
}

/****************************************************************************
NAME
    SendUpgradeInitCfm

DESCRIPTION
    Build and send an UPGRADE_INIT_CFM message and send to the specified task.

RETURNS
    n/a
*/
static void SendUpgradeInitCfm(Task task, upgrade_status_t status)
{
    MESSAGE_MAKE(upgradeInitCfm, UPGRADE_INIT_CFM_T);
    upgradeInitCfm->status = status;
    MessageSend(task, UPGRADE_INIT_CFM, upgradeInitCfm);
}

/****************************************************************************
NAME
    RequestApplicationReconnectIfNeeded

DESCRIPTION
    Check the upgrade status and decide if the application needs to consider
    restarting communication / UI so that it can connect to a host.

    If needed, builds and send an UPGRADE_RESTARTED_IND_T message and sends to
    the application task.

NOTE
    Considered implementing this as part of UpgradeSMInit() which also looks at
    the resume point information, but it is not really related to the SM.
*/
static void RequestApplicationReconnectIfNeeded(void)
{
    upgrade_reconnect_recommendation_t reconnect = upgrade_reconnect_not_required;

    DEBUG_LOG_INFO("RequestApplicationReconnectIfNeeded(): upgrade_in_progress_key %d, dfu_partition_num %d",
                   UpgradeCtxGetPSKeys()->upgrade_in_progress_key,
                   UpgradeCtxGetPSKeys()->dfu_partition_num);

    switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
        /* Resume from the beginning, includes download phase. */
    case UPGRADE_RESUME_POINT_START:
    case UPGRADE_RESUME_POINT_ERROR:
        {
            if (UpgradeCtxGetPSKeys()->id_in_progress)
            {
                /* Not in a critical operation, but there is an upgrade in progress,
                   either on this device or on peer device (primary device)
                   (in the case of _ERROR) presumably was an upgrade in progress.
                   So the application may want to restart operations to allow it to
                   resume */
                reconnect = upgrade_reconnect_recommended_in_progress;
            }
        }
        break;

    case UPGRADE_RESUME_POINT_ERASE:
            /* Not in a critical operation, but there is an upgrade in progress.
               Separated from the two cases above as there is a lesser argument
               for the reconnect - so may change in future. */
        reconnect = upgrade_reconnect_recommended_in_progress;
        break;

    case UPGRADE_RESUME_POINT_PRE_VALIDATE:
    case UPGRADE_RESUME_POINT_PRE_REBOOT:
    {
        /* There is an upgrade in progress so, the application should
         * restart operations to allow it to resume.
         */
        reconnect = upgrade_reconnect_recommended_in_progress;
        break;
     }

    case UPGRADE_RESUME_POINT_POST_REBOOT:
    case UPGRADE_RESUME_POINT_COMMIT:
        if (UpgradeCtxGetPSKeys()->dfu_partition_num == 0)
        {
            /* We are in the middle of an upgrade that requires the host/app to
               confirm its success. */
            reconnect = upgrade_reconnect_required_for_confirm;
        }
        else
        {
            /* There is a DFU to be finished off. No host interaction is
               needed but won't hurt. */
            reconnect = upgrade_reconnect_recommended_as_completed;
        }
        break;
    }

    if (reconnect != upgrade_reconnect_not_required)
    {
        UPGRADE_RESTARTED_IND_T *restarted = (UPGRADE_RESTARTED_IND_T*)
                                                PanicUnlessMalloc(sizeof(*restarted));
        restarted->reason = reconnect;
        UpgradeCtxGet()->reconnect_reason = reconnect;
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_RESTARTED_IND, restarted);
    }

}

/****************************************************************************
NAME
    isPsKeyStartValid

DESCRIPTION
    Verify that the upgrade PS key start offset is within valid limits.

RETURNS
    TRUE if offset is ok, FALSE otherwise.
*/
static bool isPsKeyStartValid(uint16 dataPskeyStart)
{
    uint16 available_space = PSKEY_MAX_STORAGE_LENGTH - dataPskeyStart;

    if ((dataPskeyStart >= PSKEY_MAX_STORAGE_LENGTH)
        || (available_space < UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS))
        return FALSE;
    else
        return TRUE;
}

/****************************************************************************
NAME
    UpgradeHandleCommitRevert

DESCRIPTION
    Check to detect reverted commit or unexpected reset of device during post reboot 
    phase and clear the pskeys if detected.
*/
static void UpgradeHandleCommitRevert(uint16 dataPskey, uint16 dataPskeyStart)
{
    /* ImageUpgradeSwapTryStatus will return false if we are running from 
     * the boot bank and true if we are running from the alternate bank */
    bool result = ImageUpgradeSwapTryStatus();
    uint16 resumePoint = UpgradePsGetResumePoint(dataPskey, dataPskeyStart);
    DEBUG_LOG_INFO("UpgradeHandleCommitRevert ImageUpgradeSwapTryStatus() returns %d and resume point is %d",result, resumePoint);

    /* If user resets the device in the post reboot phase or aborts at the commit 
     * screen then, device will reboot from the boot bank but still, the resume point
     * will be post reboot in the PSStore. If the syncID is cleared and 
     * resume point is UPGRADE_RESUME_POINT_ERROR, reset will move EB to 
     * aborting state. Device will respond with Error_Ind for sync request and 
     * will not handle the next abort request because it is in Aborting state.
     * In these cases we need to clear PSKeys and abort the DFU. */
    if((!result && resumePoint == UPGRADE_RESUME_POINT_POST_REBOOT) || 
    (resumePoint == UPGRADE_RESUME_POINT_ERROR && UpgradeInProgressId() == 0))
    {
        /* Clear the PsKeys */
        PsStore(dataPskey, 0, 0);
        UpgradeClearHeaderPSKeys();
        /* Inform DFU domain about reverting the upgrade for required actions*/
        MessageSend(UpgradeGetAppTask(), UPGRADE_REVERT_RESET, NULL);
    }
}

void UpgradeApplicationValidationStatus(bool pass)
{
    MESSAGE_MAKE(msg, UPGRADE_VM_EXE_FS_VALIDATION_STATUS_T);
    msg->result = pass;
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_VM_EXE_FS_VALIDATION_STATUS, msg);
}

bool UpgradeIsDataTransferComplete(void)
{
    return (UpgradeCtxGetPSKeys()->upgrade_in_progress_key > UPGRADE_RESUME_POINT_START);
}

/****************************************************************************
NAME
    UpgradeImageSwap

DESCRIPTION
     This function will eventually call the ImageUpgradeSwapTry() trap to initiate a full chip reset,
      load and run images from the other image bank.

RETURNS
    None
*/
void UpgradeImageSwap(void)
{
    DEBUG_LOG("UpgradeImageSwap");
    UpgradeSMHandleMsg(UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS, NULL);
}

/*!
    @brief Flow off or on processing of received upgrade data packets residing
           in Source Buffer.

    @note Scheme especially required for DFU over LE but currently commonly
          applied to DFU over LE or BR/EDR and when upgrade data is relayed from
          Primary to Secondary too.

    Returns None
*/
void UpgradeFlowOffProcessDataRequest(bool enable)
{
    /*
     * TODO: Can be skipped for DFU over BR/EDR transport and also when
     *       upgrade data is relayed from Primary to Secondary over BR/EDR.
     */
    UpgradeCtxGet()->dfu_rx_flow_off = enable;
}

/*!
    @brief Check if processing of received upgrade data packets residing
           in Source Buffer is flowed off or on.

    @note Scheme especially required for DFU over LE but currently commonly
          applied to DFU over LE or BR/EDR and when upgrade data is relayed from
          Primary to Secondary too.

    Returns TRUE when Source Buffer draining is flowed off in order to limit
            queued messages within acceptable limits to prevent pmalloc pools
            exhaustion, else FALSE.
*/
bool UpgradeIsProcessDataRequestFlowedOff(void)
{
    /*
     * TODO: Can be skipped for DFU over BR/EDR transport and also when
     *       upgrade data is relayed from Primary to Secondary over BR/EDR.
     */
    return UpgradeCtxGet()->dfu_rx_flow_off;
}

/***************************************************************************
NAME
    UpgradeIsInProgress

DESCRIPTION
    Return boolean indicating if an upgrade is currently in progress.
*/
bool UpgradeIsInProgress(void)
{
    return UpgradeSMUpgradeInProgress();
}

/***************************************************************************
NAME
    UpgradeIsAborting

DESCRIPTION
    Return boolean indicating if an upgrade is currently aborting.
*/
bool UpgradeIsAborting(void)
{
    return UpgradeSMGetState() == UPGRADE_STATE_ABORTING;
}

/***************************************************************************
NAME
    UpgradeIsScoActive

DESCRIPTION
    Finds whether indicating if SCO is active or not by accessing the upgrade context.
RETURN
    Returns pointer to uint16.
*/
uint16 *UpgradeIsScoActive(void)
{
    return &(UpgradeCtxGet()->isScoActive);
}

/****************************************************************************
NAME
    UpgradeSetScoActive

DESCRIPTION
    Used to assign required value (0 or 1) to SCO flag in upgrade context depending on active ongoing call.
*/
void UpgradeSetScoActive(bool scoState)
{
    UpgradeCtxGet()->isScoActive = (uint16)scoState;
    DEBUG_LOG("UpgradeSetScoActive state : %u", UpgradeCtxGet()->isScoActive);
}

void UpgradeRestartReconnectionTimer(void)
{
    /*
     * In the post reboot DFU commit phase, now main role (Primary/Secondary)
     * are no longer fixed rather dynamically selected by Topology using role
     * selection. This process may take time so its recommendable to reset this
     * reconnection timer in linkloss scenarios (if any) in the post reboot
     * DFU commit phase.
     */
    if (MessageCancelAll(UpgradeGetUpgradeTask(),
                            UPGRADE_INTERNAL_RECONNECTION_TIMEOUT))
    {
        DEBUG_LOG("UpgradeRestartReconnectionTimer UPGRADE_INTERNAL_RECONNECTION_TIMEOUT");
        MessageSendLater(UpgradeGetUpgradeTask(),
                            UPGRADE_INTERNAL_RECONNECTION_TIMEOUT, NULL,
                            D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
    }
    if (MessageCancelAll(UpgradeGetUpgradeTask(),
                            UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT))
    {
        DEBUG_LOG("UpgradeRestartReconnectionTimer UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT");
        MessageSendLater(UpgradeGetUpgradeTask(),
                    UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT, NULL,
                    D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
    }
}

uint32 UpgradeInProgressId(void)
{
    return (UpgradeCtxGetPSKeys()->id_in_progress);
}

void UpgradeSetInProgressId(uint32 id_in_progress)
{
    UpgradeCtxGetPSKeys()->id_in_progress = id_in_progress;
    UpgradeSavePSKeys();
}

bool UpgradeIsSilentCommitEnabled(void)
{
    return (UpgradeCtxGetPSKeys()->is_silent_commit != 0);
}

/****************************************************************************
NAME
    UpgradeRebootForSilentCommit

DESCRIPTION
     Initiate DFU reboot for silent commit. This function will eventually call
     the ImageUpgradeSwapTry() trap to initiate a full chip reset, load and run
     images from the other image bank.

RETURNS
    None
*/
void UpgradeRebootForSilentCommit(void)
{
    uint16 in_progress_key = UpgradeCtxGetPSKeys()->upgrade_in_progress_key;
    if(in_progress_key == UPGRADE_RESUME_POINT_PRE_REBOOT)
    {
        DEBUG_LOG("UpgradeRebootForSilentCommit: Send message to reboot");
        UpgradeSMHandleMsg(UPGRADE_INTERNAL_SILENT_COMMIT_REBOOT, NULL);
    }
    else
    {
        DEBUG_LOG("UpgradeRebootForSilentCommit: Ignored since resume point is %d",
                   in_progress_key);
    }
}

/***************************************************************************
NAME
    UpgradeStartValidation

DESCRIPTION
    Inform upgrade_partition_data SM to start post-data-xfer/validation process
*/
void UpgradeStartValidation(void)
{
    UpgradePartitionDataHandleMessage(UPGRADE_PARTITION_DATA_INTERNAL_START_VALIDATION, NULL);
}

/****************************************************************************
NAME
    UpgradeSetSilentCommitSupported

DESCRIPTION
    Used to assign required value (0 or 1) to isSilentCommitSupported flag in
    upgrade context by application.
*/
void UpgradeSetSilentCommitSupported(uint8 is_silent_commit_supported)
{
    UpgradeCtxGet()->isSilentCommitSupported = is_silent_commit_supported;
    DEBUG_LOG("UpgradeSetSilentCommitSupported: %u", UpgradeCtxGet()->isSilentCommitSupported);
}

/****************************************************************************
NAME
    UpgradeSetPeerDfuSupport

DESCRIPTION
    Used to assign TRUE to is_peer_dfu_supported flag in
    upgrade context by dfu peer domain.
*/
void UpgradeSetPeerDfuSupport(bool is_peer_dfu_supported)
{
    UpgradeCtxGet()->isUpgradePeerDfuSupported = is_peer_dfu_supported;
}

/****************************************************************************
NAME
    UpgradeGetPeerDfuSupport

DESCRIPTION
    Get the value stored in isUpgradePeerDfuSupported
*/
bool UpgradeGetPeerDfuSupport(void)
{
    return UpgradeCtxGet()->isUpgradePeerDfuSupported;
}

/****************************************************************************
NAME
    UpgradeClientConnect

DESCRIPTION
    Set the clientTask in upgradeCtx.
*/
void UpgradeClientConnect(Task clientTask)
{
    UpgradeHostIFClientConnect(clientTask);

    /* We need to delay the transport data confirmation if application is going to process the data.
     * Application can send the confirmation using UpgradeTransportDataCfm after it processes the data. 
     */
    UpgradeCtxGet()->isTransportCfmDelayed = TRUE;
}

/****************************************************************************
NAME
    UpgradeClientReconnectLib

DESCRIPTION
    Reset the upgrade lib task as clientTask.
*/
void UpgradeClientReconnectLib(void)
{
     UpgradeHostIFClientConnect(&UpgradeCtxGet()->smTaskData);
     UpgradeCtxGet()->isTransportCfmDelayed = FALSE;
}

/****************************************************************************
NAME
    UpgradeSetResumePoint

DESCRIPTION
    Set new resume-point in the pskey
*/
void UpgradeSetResumePoint(UpdateResumePoint resume_point)
{
     UpgradeCtxGetPSKeys()->upgrade_in_progress_key = resume_point;
    UpgradeSavePSKeys();
}

/****************************************************************************
NAME
    UpgradeGetResumePoint

DESCRIPTION
    Get the resume point stored in the pskey
*/
UpdateResumePoint UpgradeGetResumePoint(void)
{
    return UpgradeCtxGetPSKeys()->upgrade_in_progress_key;
}


/****************************************************************************
NAME
    UpgradeSetFPtr

DESCRIPTION
     To set the appropriate fptr in UpgradeCtxGet()->funcs.

RETURNS
    None
*/
void UpgradeSetFPtr(const upgrade_response_functions_t *fptr)
{
    UpgradeCtxGet()->funcs = fptr;
}

/****************************************************************************
NAME
    UpgradeClientSendData

DESCRIPTION
    Wrapper function which invokes UpgradeHostIFClientSendData()
*/
void UpgradeClientSendData(uint8 *data, uint16 dataSize)
{
    UpgradeHostIFClientSendData(data, dataSize);
}

/****************************************************************************
NAME
    UpgradeAbortCaseDfuOnReboot

DESCRIPTION
    Check if the Case DFU was in progress before the device rebooted and set the status as Aborting.
*/
static void UpgradeAbortCaseDfuOnReboot(void)
{
    if(UpgradeCtxGetPSKeys()->id_in_progress && UpgradeIsCaseDfuHeaderInPSKeys())
    {
        DEBUG_LOG_INFO("UpgradeAbortCaseDfuOnReboot Aborting case DFU");
        UpgradeCtxGet()->isCaseDfuSupported = TRUE;
        UpgradeCtxGet()->caseDfuStatus = UPGRADE_CASE_DFU_ABORTING;
    }
}

/****************************************************************************
NAME
    UpgradeSetCaseDfuSupport

DESCRIPTION
    Used to assign TRUE to is_case_dfu_supported flag in
    upgrade context by dfu case domain.
*/
void UpgradeSetCaseDfuSupport(bool is_case_dfu_supported)
{
    UpgradeCtxGet()->isCaseDfuSupported = is_case_dfu_supported;
}

/****************************************************************************
NAME
    UpgradeGetCaseDfuSupport

DESCRIPTION
    Get the value stored in isUpgradeCaseDfuSupported
*/
bool UpgradeGetCaseDfuSupport(void)
{
    return UpgradeCtxGet()->isCaseDfuSupported;
}

/****************************************************************************
NAME
    UpgradeIsCaseDfuInProgress

DESCRIPTION
    returns TRUE if Case DFU file has been detected and DFU is in progress
*/
bool UpgradeIsCaseDfuInProgress(void)
{
    return UpgradeCtxGet()->isCaseDfuSupported && UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_IN_PROGRESS;
}

/****************************************************************************
NAME
    UpgradeGetCaseDfuHeaderLength

DESCRIPTION
    Get the stored header length if the last message was header first part
*/
uint32 UpgradeGetCaseDfuHeaderLength(void)
{
    return UpgradeCtxGet()->caseDfuHeaderLength;
}

/****************************************************************************
NAME
    UpgradeCleanUpCaseDFU

DESCRIPTION
    Clean-up the upgrade library context after the case DFU ends.
*/
void UpgradeCleanUpCaseDfu(bool isError)
{
    if(UpgradeCtxGetPSKeys()->id_in_progress)
    {
        UpgradeHostIFClientConnect(&UpgradeCtxGet()->smTaskData);
        UpgradeSmCleanUpCaseDfu(isError);
    }
}

/****************************************************************************
NAME
    UpgradeGetCurrentPartitionFirstWord

DESCRIPTION
    Get the first word of the current partition from PSKEY
*/
uint32 UpgradeGetCurrentPartitionFirstWord(void)
{
    return UpgradeCtxGetPSKeys()->first_word;
}


uint32 UpgradeGetDfuFileOffset(void)
{
    return UpgradeCtxGet()->dfu_file_offset;
}

bool UpgradeGetPartitionStateFromDfuFileOffset(uint32 req_offset, upgrade_partition_state_t* state)
{
    return UpgradePartitionOffsetCalculatorGetPartitionStateFromDfuFileOffset(req_offset, state);
}

/***********************************************************************
NAME
    Upgrade_SetContext

DESCRIPTION
    Sets the context of the UPGRADE module
    The value is stored in the UPGRADE PsKey and hence is non-volatile
*/
void Upgrade_SetContext(upgrade_context_t context)
{
    UpgradeCtxGetPSKeys()->upgrade_context = context;
    UpgradeSavePSKeys();
}

/***********************************************************************
NAME
    Upgrade_GetContext

DESCRIPTION
    Gets the context of the UPGRADE module
    The value is retreived from the non-volatile UPGRADE PsKey.
*/
upgrade_context_t Upgrade_GetContext(void)
{
    if(!UpgradeCtxGetPSKeys())
    {
        DEBUG_LOG_ERROR("Upgrade_GetContext : Upgrade PSKey not found !");
        Panic();
    }
    return UpgradeCtxGetPSKeys()->upgrade_context;
}

/***************************************************************************
NAME
    Upgrade_RevertReboot

DESCRIPTION
    Wrapper to internal message sent to upgrade lib to delay reboot due to revert by UPGRADE_WAIT_FOR_REBOOT time.
*/
void Upgrade_RevertReboot(bool delayed)
{
    if(delayed)
    {
        MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_DELAY_REVERT_REBOOT, 
                     NULL, UPGRADE_WAIT_FOR_REBOOT);
    }
    else
    {
        MessageSend(UpgradeGetUpgradeTask(),UPGRADE_INTERNAL_DELAY_REVERT_REBOOT,NULL);
    }
}

/****************************************************************************
NAME
    UpgradeIsAbortDfuDueToSyncIdMismatch

RETURNS
    upgradeCtx->isDfuAbortDueToSyncIdMismatch
*/

bool UpgradeIsAbortDfuDueToSyncIdMismatch(void)
{
    return UpgradeCtxGet()->isDfuAbortDueToSyncIdMismatch;
}

/****************************************************************************
NAME
    UpgradeIsResumePointPreReboot

RETURNS
    returns TRUE if resume point is UPGRADE_RESUME_POINT_PRE_REBOOT, else FALSE
*/

bool UpgradeIsResumePointPreReboot(void)
{
    return (UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_PRE_REBOOT);
}

/****************************************************************************
NAME
    UpgradeIsResumePointStart

RETURNS
    returns TRUE if resume point is UPGRADE_RESUME_POINT_START, else FALSE
*/

bool UpgradeIsResumePointStart(void)
{
    return (UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_START);
}

/****************************************************************************
NAME
    UpgradeIsResumePoinPreValidate

RETURNS
    returns TRUE if resume point is UPGRADE_RESUME_POINT_PRE_VALIDATE, else FALSE
*/

bool UpgradeIsResumePoinPreValidate(void)
{
    return (UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_PRE_VALIDATE);
}

/****************************************************************************
NAME
    UpgradeIsResumePointPostReboot
RETURNS
    returns TRUE if resume point is UPGRADE_RESUME_POINT_POST_REBOOT, else FALSE
*/
bool UpgradeIsResumePointPostReboot(void)
{
    return (UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_POST_REBOOT);
}

/***************************************************************************
NAME
    UpgradeHandleValidationComplete

DESCRIPTION
    Handle the validation complete scenario
*/
void UpgradeHandleValidationComplete(void)
{
    /* Set the upgrade resume point to PRE_REBOOT */
    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_PRE_REBOOT;
    UpgradeSavePSKeys();

    DEBUG_LOG("UpgradeHandleValidationComplete: Send msg to inform vldtn complete");
    /* TODO : Check if sending the message can be an asynchronous call */
    UpgradeSMHandleMsg(UPGRADE_INTERNAL_VALIDATION_COMPLETE, NULL);
}

/***************************************************************************
NAME
    UpgradeIsValidationComplete

DESCRIPTION
    Check if the Partition data state is validated 
*/
bool UpgradeIsValidationComplete(void)
{
    return (UpgradeCtxGetPartitionData() && 
            UpgradePartitionDataGetState() == UPGRADE_PARTITION_DATA_STATE_VALIDATION_COMPLETE);
}

/****************************************************************************
NAME
  UpgradeIsWaitForValidation
RETURNS
    returns TRUE if partition data state is in VALIDATION, else FALSE
*/
bool UpgradeIsWaitForValidation(void)
{
    return (UpgradeCtxGetPartitionData() && UpgradeCtxGetPartitionData()->state == UPGRADE_PARTITION_DATA_STATE_WAIT_FOR_VALIDATION);
}

/****************************************************************************
NAME
    UpgradePartitionGetTotalPartitions  - Returns number of partitions 
RETURNS
    Returns number of partitions 
*/
uint8 UpgradePartitionGetTotalPartitions(void)
{
   return UpgradeCtxGetPartitionData()->totalPartitions;
}

/****************************************************************************
NAME
    UpgradeIsTransferCompleteReceived  - Returns the status of Transfer Complete Res
RETURNS
    TRUE if UPGRADE_HOST_TRANSFER_COMPLETE_RES is received and handled, else FALSE
*/
bool UpgradeIsTransferCompleteResponseReceived(void)
{
   return UpgradeCtxGet()->transferCompleteResReceived;
}

