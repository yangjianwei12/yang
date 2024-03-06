/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_sm.c

DESCRIPTION

NOTES

*/

#define DEBUG_LOG_MODULE_NAME upgrade
#include <logging.h>

#include <stdlib.h>
#include <boot.h>
#include <loader.h>
#include <ps.h>
#include <panic.h>
#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_host_if_data.h"
#include "upgrade_partition_data.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_partitions.h"
#include "upgrade_sm.h"
#include "upgrade_msg_vm.h"
#include <upgrade_protocol.h>
#include "upgrade_msg_internal.h"
#include "upgrade_fw_if.h"
#include <app/message/system_message.h>

#define VALIDATAION_BACKOFF_TIME_MS 100

static bool upgradeSm_HandleCheckStatus(MessageId id, Message message);
static bool upgradeSm_HandleSync(MessageId id, Message message);
static bool upgradeSm_HandleReady(MessageId id, Message message);
static bool upgradeSm_HandleProhibited(MessageId id, Message message);
static bool upgradeSm_HandleAborting(MessageId id, Message message);
static bool upgradeSm_HandleDataReady(MessageId id, Message message);
static bool upgradeSm_HandleDataTransfer(MessageId id, Message message);
static bool upgradeSm_HandleDataTransferSuspended(MessageId id, Message message);
static bool upgradeSm_HandleValidation(MessageId id, Message message);
static bool upgradeSm_HandleProcessComplete(MessageId id, Message message);
static bool upgradeSm_HandleRestartedForCommit(MessageId id, Message message);
static bool upgradeSm_HandleCommitHostContinue(MessageId id, Message message);
static bool upgradeSm_HandleCommitVerification(MessageId id, Message message);
static bool upgradeSm_HandleCommitConfirm(MessageId id,Message message);
static bool upgradeSm_HandleCommit(MessageId id, Message message);
static bool upgradeSm_HandlePsJournal(MessageId id, Message message);
static bool upgradeSm_HandleRebootToResume(MessageId id,Message message);
static bool upgradeSm_HandleBatteryLow(MessageId id, Message message);

static bool upgradeSm_DefaultHandler(MessageId id, Message message, bool handledAlready);

static void upgradeSm_PsSpaceError(void);
static void FileTransferCompleted(bool is_silent_commit, bool inform_peer);


/* permission related functions */
static void PsFloodAndReboot(void);
static void InformAppsCompleteGotoSync(bool is_silent_commit);

/***************************************************************************
NAME
    UpgradeSMInit  -  Initialise the State Machine

DESCRIPTION
    This function performs relevant initialisation of the state machine,
    currently just setting the initial state.

    This is currently determined by checking whether an upgraded
    application is running.

*/
void UpgradeSMInit(void)
{
    DEBUG_LOG_DEBUG("UpgradeSMInit: resume point enum:UpdateResumePoint:%d",
              UpgradeCtxGetPSKeys()->upgrade_in_progress_key);

    /* If the earbud rebooted during case DFU and is not able to connect with handset
     * then it will get stuck in aborting state so, start a timer to abort the case dfu
     * without host.
     */
    if(UPGRADE_IS_CASE_SUPPORTED && UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_ABORTING)
    {
        MessageSendLater(UpgradeGetUpgradeTask(),
                UPGRADE_INTERNAL_CASE_DFU_RECONNECTION_TIMEOUT, NULL,
                D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
    }

    switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
    /* UPGRADE_RESUME_POINT_PRE_VALIDATE:
        @todo: What do we do in this case ? */
    /* UPGRADE_RESUME_POINT_POST_REBOOT:
       UPGRADE_RESUME_POINT_COMMIT:
        @todo: Are these right, we want host to chat */
    default:
        UpgradeSMSetState(UPGRADE_STATE_CHECK_STATUS);
        break;

    /*case UPGRADE_RESUME_POINT_PRE_REBOOT:
        UpgradeSMSetState(UPGRADE_STATE_VALIDATED);
        break;*/

    case UPGRADE_RESUME_POINT_POST_REBOOT:
        /* Any abort in the post reboot phase should be followed by reboot
         * to restore the running image from boot bank. */
        if(UpgradeIsSilentCommitEnabled())
        {
            MessageSendLater(UpgradeGetUpgradeTask(),
                UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT, NULL,
                D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
        }
        else
        {
            UpgradeCtxGet()->isImageRevertNeededOnAbort = TRUE;
            UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
            MessageSendLater(UpgradeGetUpgradeTask(),
                UPGRADE_INTERNAL_RECONNECTION_TIMEOUT, NULL,
                D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
        }
        break;

    /*case UPGRADE_RESUME_POINT_ERASE:
        UpgradeSMMoveToState(UPGRADE_STATE_COMMIT);
        break;*/

    case UPGRADE_RESUME_POINT_ERROR:
        UpgradeSMSetState(UPGRADE_STATE_ABORTING);
        break;
    }
}

UpgradeState UpgradeSMGetState(void)
{
    return UpgradeCtxGet()->smState;
}

void UpgradeSMHandleMsg(MessageId id, Message message)
{
    bool handled = FALSE;

    DEBUG_LOG("UpgradeSMHandleMsg, state %u, message_id 0x%04x", UpgradeSMGetState(), id);

    switch (UpgradeSMGetState())
    {
    case UPGRADE_STATE_BATTERY_LOW:
        handled = upgradeSm_HandleBatteryLow(id, message);
        break;

    case UPGRADE_STATE_CHECK_STATUS:
        handled = upgradeSm_HandleCheckStatus(id, message);
        break;

    case UPGRADE_STATE_SYNC:
        handled = upgradeSm_HandleSync(id, message);
        break;

    case UPGRADE_STATE_READY:
        handled = upgradeSm_HandleReady(id, message);
        break;

    case UPGRADE_STATE_PROHIBITED:
        handled = upgradeSm_HandleProhibited(id, message);
        break;

    case UPGRADE_STATE_ABORTING:
        handled = upgradeSm_HandleAborting(id, message);
        break;

    case UPGRADE_STATE_DATA_READY:
        handled = upgradeSm_HandleDataReady(id, message);
        break;

    case UPGRADE_STATE_DATA_TRANSFER:
        handled = upgradeSm_HandleDataTransfer(id, message);
        break;

    case UPGRADE_STATE_DATA_TRANSFER_SUSPENDED:
        handled = upgradeSm_HandleDataTransferSuspended(id, message);
        break;

    case UPGRADE_STATE_VALIDATION:
        handled = upgradeSm_HandleValidation(id, message);
        break;

    case UPGRADE_STATE_PROCESS_COMPLETE:
        handled = upgradeSm_HandleProcessComplete(id, message);
        break;

    case UPGRADE_STATE_RESTARTED_FOR_COMMIT:
        handled = upgradeSm_HandleRestartedForCommit(id, message);
        break;

    case UPGRADE_STATE_COMMIT_HOST_CONTINUE:
        handled = upgradeSm_HandleCommitHostContinue(id, message);
        break;

    case UPGRADE_STATE_COMMIT_VERIFICATION:
        handled = upgradeSm_HandleCommitVerification(id, message);
        break;

    case UPGRADE_STATE_COMMIT_CONFIRM:
        handled = upgradeSm_HandleCommitConfirm(id, message);
        break;

    case UPGRADE_STATE_COMMIT:
        handled = upgradeSm_HandleCommit(id, message);
        break;

    case UPGRADE_STATE_PS_JOURNAL:
        handled = upgradeSm_HandlePsJournal(id, message);
        break;

    case UPGRADE_STATE_REBOOT_TO_RESUME:
        handled = upgradeSm_HandleRebootToResume(id,message);
        break;

    default:
        DEBUG_LOG("UpgradeSMHandleMsg, unknown state %u", UpgradeSMGetState());
        break;
    }

    if (UpgradeSMGetState() != UPGRADE_STATE_CHECK_STATUS)
    {
        handled = upgradeSm_DefaultHandler(id, message, handled);
    }

    if (!handled)
    {
        DEBUG_LOG("UpgradeSMHandleMsg: MESSAGE:0x%04x not handled", id);
    }

    /*
     * TODO: Can be skipped for DFU over BR/EDR transport and also when
     *       upgrade data is relayed from Primary to Secondary over
     *       BR/EDR.
     */
    {
        /* Conditionally decrement only when a packet is processed.*/
        if (UpgradeCtxGet()->pendingDataReq)
        {
            UpgradeCtxGet()->pendingDataReq--;
        }
        /* Unconditionally flow on so that the DFU doesn't stall.*/
        UpgradeFlowOffProcessDataRequest(FALSE);
    }

    DEBUG_LOG("UpgradeSMHandleMsg, new state %u", UpgradeSMGetState());
}

bool upgradeSm_HandleCheckStatus(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_VM_PERMIT_UPGRADE:
        UpgradeSMSetState(UPGRADE_STATE_SYNC);
        break;

    case UPGRADE_INTERNAL_IN_PROGRESS:
        UpgradeSMSetState(UPGRADE_STATE_RESTARTED_FOR_COMMIT);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleBatteryLow(MessageId id, Message message)
{
    if(UpgradeCtxGet()->power_state != upgrade_battery_low)
    {
        /* this forces DefaultHandle to be called to handle sync request */
        return FALSE;
    }

    /* in this state always send low battery error message */
    if (id != UPGRADE_HOST_ERRORWARN_RES)
    {
        if (id == UPGRADE_HOST_DATA)
        {
            UPGRADE_HOST_DATA_T *msg = (UPGRADE_HOST_DATA_T *)message;

            /* There may be several of these messages in a row, and
             * we only want to send on error message.
             * Part-process the message. If we get an error response
             * then we can assume that we have already sent an error 
             * and do not send another */
            if (UPGRADE_PARTITION_DATA_XFER_ERROR ==
                          UpgradePartitionDataParse((uint8 *)&msg->data[0], msg->length))
            {
                return TRUE;
            }
            UpgradePartitionDataStopData();
        }

        UpgradeCtxGet()->funcs->SendErrorInd(UPGRADE_HOST_ERROR_BATTERY_LOW);
    }

    return TRUE;
}

bool upgradeSm_HandleSync(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
#ifdef UPGRADE_SYNC_WILL_FORCE_COMMIT_PHASE
    /* @todo
     * This is a temporary solution to force state machine to
     * proceed to commit phase after reboot.
     */
    case UPGRADE_HOST_SYNC_AFTER_REBOOT_REQ:
        DEBUG_LOG_INFO("upgradeSm_HandleSync: UPGRADE_HOST_SYNC_AFTER_REBOOT_REQ UpgradeIsSilentCommitEnabled: %d",
                       UpgradeIsSilentCommitEnabled());
        /* Do not force state machine if it is silent commit */
        if(!UpgradeIsSilentCommitEnabled())
        {
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_IN_PROGRESS_IND);
            UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
        }
        break;
#endif

    case UPGRADE_INTERNAL_RECONNECTION_TIMEOUT:
    case UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT:
        BootSetMode(BootGetMode());
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleReady(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_START_REQ:
        {
            bool error = FALSE;
            bool newUpgrade = TRUE;

            DEBUG_LOG("upgradeSm_HandleReady: UPGRADE_HOST_START_REQ, resume point enum:UpdateResumePoint:%d",
                      UpgradeCtxGetPSKeys()->upgrade_in_progress_key);

            if(UPGRADE_IS_CASE_SUPPORTED && UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_IN_PROGRESS)
            {
                /* Inform the application to resume case DFU and move to suspended state as 
                 * upgrade library is no longer required to process the data. */
                MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_CASE_DFU_RESUME_IND, NULL);
                UpgradeSMSetState(UPGRADE_STATE_DATA_TRANSFER_SUSPENDED);
                newUpgrade = FALSE;
            }
            else
            {
                switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
                {
                case UPGRADE_RESUME_POINT_START:
                    UpgradeSMSetState(UPGRADE_STATE_DATA_READY);
                    break;
                case UPGRADE_RESUME_POINT_PRE_VALIDATE:
                    UpgradeSMSetState(UPGRADE_STATE_VALIDATION);
                    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_RESUME_IND, NULL);
                    break;
                case UPGRADE_RESUME_POINT_PRE_REBOOT:
                    UpgradeSMSetState(UPGRADE_STATE_PROCESS_COMPLETE);
                    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_RESUME_IND, NULL);
                    newUpgrade = FALSE;
                    break;
                case UPGRADE_RESUME_POINT_POST_REBOOT:
                    UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
                    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_RESUME_IND, NULL);
                    newUpgrade = FALSE;
                    break;
                /*case UPGRADE_RESUME_POINT_COMMIT:
                    UpgradeSMSetState(UPGRADE_STATE_COMMIT_CONFIRM);
                    newUpgrade = FALSE;
                    break;*/
                case UPGRADE_RESUME_POINT_ERASE:
                    UpgradeSMMoveToState(UPGRADE_STATE_COMMIT);
                    newUpgrade = FALSE;
                    break;
                case UPGRADE_RESUME_POINT_ERROR:
                    UpgradeSMSetState(UPGRADE_STATE_ABORTING);
                    /* @todo do we need to set error = TRUE here? We need to send an error to the host
                     * or we'll get stuck in the ABORTING state */
                    break;
                default:
                    DEBUG_LOG_ERROR("upgradeSm_HandleReady: unexpected in progress key enum:UpdateResumePoint:%d",
                                    UpgradeCtxGetPSKeys()->upgrade_in_progress_key);
                    error = TRUE;
                    break;
                }
            }

            if(!error)
            {
                UpgradeCtxGet()->funcs->SendStartCfm(0, 0x666);
            }
            else
            {
                UpgradeFatalError(UPGRADE_HOST_ERROR_INTERNAL_ERROR_4);
            }

            /* We are starting/resuming an upgrade so update target partitions */
            if (newUpgrade)
            {
                UpgradePartitionsUpgradeStarted();
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleProhibited(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

bool upgradeSm_HandleAborting(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_ERRORWARN_RES:
        DEBUG_LOG("upgradeSm_HandleAborting UPGRADE_HOST_ERRORWARN_RES");
        /* Do nothing. ERRORWARN_RES should be followed by ABORT_REQ which 
         * will handle the aborting process. */
        break;

    case MESSAGE_IMAGE_UPGRADE_ERASE_STATUS:
#ifndef HOSTED_TEST_ENVIRONMENT
        /* Erase has completed but send the ABORT_CFM only after peer device also completes.
         * No need to check the erase status as we are aborting anyway. */
        MessageSendConditionally(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_SEND_ABORT_CFM, NULL,
                                 (const uint16*)&(UpgradeCtxGet()->waitForPeerAbort));
#else
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_SEND_ABORT_CFM, NULL);
#endif
        UpgradeSMSetState(UPGRADE_STATE_SYNC);

        break;

    case UPGRADE_HOST_SYNC_REQ:
        /* TODO: Maybe we also should send something like last error here */
        UpgradeCtxGet()->funcs->SendErrorInd(UPGRADE_HOST_ERROR_IN_ERROR_STATE);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

bool upgradeSm_HandleDataReady(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
        case UPGRADE_HOST_START_DATA_REQ:
        {
            if (UpgradePartitionDataInit())
            {
                /* Notify application of UPGRADE_START_DATA_IND */

                MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_START_DATA_IND, NULL);
            }
            else
            {
                UpgradeFatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
            }
        }
        break;

        case UPGRADE_INTERNAL_REQUEST_DATA:
        {
            uint32 req_size = UpgradePartitionDataGetNextReqSize();
            uint32 offset = UpgradePartitionDataGetNextOffset();
            DEBUG_LOG_INFO("UPGRADE_INTERNAL_REQUEST_DATA, requesting %ld bytes from offset %ld", req_size, offset);
            UpgradeCtxGet()->funcs->SendBytesReq(req_size, offset);
            UpgradeSMSetState(UPGRADE_STATE_DATA_TRANSFER);
        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}

static bool upgradeSm_HandleDataTransfer(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_DATA:
        if(message)
        {
            UPGRADE_HOST_DATA_T *msg = (UPGRADE_HOST_DATA_T *)message;
            UpgradeHostErrorCode rc;

            /* TODO: Validate message length */

            rc = UpgradePartitionDataParse(msg->data, msg->length);
            DEBUG_LOG_VERBOSE("upgradeSm_HandleDataTransfer: rc enum:UpgradeHostErrorCode:%u", rc);

            /* Check for upgrade file size errors */
            if(rc == UPGRADE_HOST_SUCCESS && msg->lastPacket)
            {
                rc = UPGRADE_HOST_ERROR_FILE_TOO_SMALL;
            }
            else if(rc == UPGRADE_HOST_DATA_TRANSFER_COMPLETE && !msg->lastPacket)
            {
                rc = UPGRADE_HOST_ERROR_FILE_TOO_BIG;
            }

            if(rc == UPGRADE_HOST_DATA_TRANSFER_COMPLETE)
            {
                DEBUG_LOG_INFO("upgradeSm_HandleDataTransfer: transfer complete");

                /*    Calculate and validate data hash(s).   */
                UpgradeCtxGet()->isCsrValidDoneReqReceived = FALSE;
                /*
                 * change the resume point, now that all data has been
                 * downloaded, and ensure we remember it. */
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_PRE_VALIDATE;
                UpgradeSavePSKeys();

                /* Check for if concurrent DFU is in progress and check if
                 * link loss occurs between the peers.Delay the hash checking of
                 * primary device until the peer dfu is completed or connection between peers is back
                 */
                UpgradeSMSetState(UPGRADE_STATE_VALIDATION);

            }
            else if(rc != UPGRADE_HOST_SUCCESS)
            {
                if(rc == UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE)
                {
                    upgradeSm_PsSpaceError();
                }
                else
                {
                    UpgradeFatalError(rc);
                }
            }
        }
        break;

    case UPGRADE_INTERNAL_REQUEST_DATA:
    {
        uint32 req_size = UpgradePartitionDataGetNextReqSize();
        if(req_size)
        {
            uint32 offset = UpgradePartitionDataGetNextOffset();
            DEBUG_LOG_INFO("upgradeSm_HandleDataTransfer: requesting %u bytes at offset %u", req_size, offset);
            UpgradeCtxGet()->funcs->SendBytesReq(req_size, offset);
        }
    }
    break;

    case UPGRADE_INTERNAL_SUSPEND_DATA_TRANSFER:
        /* Move to suspended state as upgrade library is no longer required to process the data. */
        UpgradeSMSetState(UPGRADE_STATE_DATA_TRANSFER_SUSPENDED);
        //TO_DO: Should we UpgradePartitionDataDestroy() 
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleDataTransferSuspended(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

bool upgradeSm_HandleValidation(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
       {
            UpgradeCtx *ctx = UpgradeCtxGet();
            UpgradeCtxGet()->funcs->SendIsCsrValidDoneCfm(VALIDATAION_BACKOFF_TIME_MS);
            ctx->isCsrValidDoneReqReceived = TRUE;
            break;
       }

    case UPGRADE_INTERNAL_VALIDATION_COMPLETE:
        {
            /* Set the upgrade SM state to PROCESS_COMPLETE */
            UpgradeSMSetState(UPGRADE_STATE_PROCESS_COMPLETE);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleProcessComplete(MessageId id, Message message)
{
    UpgradeCtx *ctx = UpgradeCtxGet();
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_TRANSFER_COMPLETE_RES:
        {
            DEBUG_LOG_DEBUG("upgradeSm_HandleProcessComplete: UPGRADE_HOST_TRANSFER_COMPLETE_RES received from Host");
            UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *msg = (UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *)message;

            if(msg !=NULL)
            {
                ctx->commitMode = msg->action;
                ctx->transferCompleteResReceived = TRUE;
                /* Inform that the dfu file validation is completed and TRANSFER_COMPLETE_RES is received */
                MessageSend(UpgradeGetAppTask(), UPGRADE_REBOOT_IND, NULL);
            }
            /* Abort */
            else
            {
                UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(),
                upgrade_ops_abort_post_transfer_complete, NO_ACTION);
            }
        }
        break;

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        {
            /* Validation completed, and now waiting for UPGRADE_TRANSFER_COMPLETE_RES
             * protocol message. Update resume point and ensure we remember it.
             */
            DEBUG_LOG_DEBUG("upgradeSm_HandleProcessComplete: UPGRADE_HOST_TRANSFER_COMPLETE_IND sent to Host");
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
        }
        break;

    case UPGRADE_INTERNAL_SILENT_COMMIT_REBOOT:
        {
            /* Initiate DFU reboot since we got go ahead to reboot for silent commit case. 
             * There is no need to inform peer about this since, DFU domain will handle it. */
            FileTransferCompleted(FALSE, FALSE);
        }
        break;

    case UPGRADE_INTERNAL_TRIGGER_REBOOT:
        {
            /* if we have permission, go ahead and call loader/reboot */
            if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
            {
                UpgradeApplyAudioShutDown();
            }
        }
        break;

    /* application finally gave permission, warm reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            UpgradeApplyAudioShutDown();
        }
        break;

    case UPGRADE_VM_AUDIO_DFU_FAILURE:
        UpgradeApplyAudioCopyFailed();
        break;

    case UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS:
        {
            DEBUG_LOG_DEBUG("UpgradeSMHandleValidated: UPGRADE_RESUME_POINT_POST_REBOOT saved");
            UpgradeCtxGetPSKeys()->upgrade_in_progress_key = 
                                            UPGRADE_RESUME_POINT_POST_REBOOT;
            UpgradeSavePSKeys();
            DEBUG_LOG_DEBUG("upgradeSm_HandleProcessComplete: ImageUpgradeSwapTry() in UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS");
            ImageUpgradeSwapTry();
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleRestartedForCommit(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_SYNC_AFTER_REBOOT_REQ:
        UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_send_host_in_progress_ind, NO_ACTION);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/* We end up here after reboot */
bool upgradeSm_HandleCommitHostContinue(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_PROCEED_TO_COMMIT:
        {
            UPGRADE_HOST_PROCEED_TO_COMMIT_T *msg = (UPGRADE_HOST_PROCEED_TO_COMMIT_T *)message;

            MessageCancelFirst(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_RECONNECTION_TIMEOUT);

            UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_relay_peer_proceed_to_commit, msg->action);
        }
        break;

    case UPGRADE_INTERNAL_RECONNECTION_TIMEOUT:
        {
            bool dfu = UpgradePartitionDataIsDfuUpdate();
            uint16 err = UpgradeSMNewImageStatus();

            if(dfu && !err)
            {
                /* Carry on */
                UpgradeSmCommitConfirmYes();
            }
            else
            {
                /* Revert */
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERROR;
                UpgradeSavePSKeys();
                DEBUG_LOG_ERROR("upgradeSm_HandleCommitHostContinue: UPGRADE_RESUME_POINT_ERROR saved");
                UpgradeSMSetState(UPGRADE_STATE_SYNC);
                /* Inform DFU domain about Upgrade revert and marshall it to other 
                 * device such that both primary and secondary reboot together.
                 * Both devices (primary and sec)have their own timers and 
                 * since it is not fixed which device will time out first.
                 * Therefore this message needs to be sent from both devices.
                 */
                MessageSend(UpgradeGetAppTask(), UPGRADE_SYNC_REVERT_REBOOT, NULL);
                Upgrade_RevertReboot(TRUE);
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleCommitVerification(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_INTERNAL_CONTINUE:
        {
            UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_check_peer_commit, NO_ACTION);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandleCommitConfirm(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_COMMIT_CFM:
        {
            UPGRADE_HOST_COMMIT_CFM_T *cfm = (UPGRADE_HOST_COMMIT_CFM_T *)message;

            uint8 action = (uint8)cfm->action;

            UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_check_peer_during_commit, action);

            switch (cfm->action)
            {
            case UPGRADE_HOSTACTION_YES:
                UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_delay_prim_commit, NO_ACTION);
                break;

            case UPGRADE_HOSTACTION_NO:
                /* By design, UPGRADE_HOSTACTION_NO should be followed by abort_req 
                 * where the DFU will be aborted. */
                /* Set the state to UPGRADE_STATE_SYNC to satisfy the unit tests */
                UpgradeSMSetState(UPGRADE_STATE_SYNC);
                DEBUG_LOG("upgradeSm_HandleCommitConfirm isImageRevertNeededOnAbort set");
                break;
            /** default:
               @todo: Error case. Should we handle ? Who cancels timeout */
            }
        }
        break;

    case UPGRADE_INTERNAL_CONTINUE:
        UpgradeSmCommitConfirmYes();
        break;

    case UPGRADE_HOST_PROCEED_TO_COMMIT:
        /* In case of GAIA reconnection, sync req., start req., and in progress res., will be resent.
         * Processing these messages would bring primary EB to UPGRADE_STATE_COMMIT_HOST_CONTINUE but, 
         * secondary EB will receive only in progress res. so, it needs to be moved back to 
         * UPGRADE_STATE_COMMIT_HOST_CONTINUE to process this message.
        */
        UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
        UpgradeSMHandleMsg(id, message);
        break;

    default:
        /** @todo We don't handle unexpected messages in most cases, error messages
          are dealt with in the default handler.

           BUT how do we deal with timeout ?  It *should* be in the state
          machine */
        return FALSE;
    }

    return TRUE;
}

/*
NAME
    InformAppsCompleteGotoSync

DESCRIPTION
    Process the required actions from upgradeSm_HandleCommit.

    Send messages to both the Host and VM applications to inform them that
    the upgrade is complete.

    Called either immediately or once we receive permission from
    the VM application.
*/
static void InformAppsCompleteGotoSync(bool is_silent_commit)
{
    /* For silent commit, no message need to be sent to host */
    if(!is_silent_commit)
    {
        if(UpgradeFWIIFIsCommitStatusAvailable())
        {
            uint8 hostStatus = UpgradeCtxGet()->fwCtx.commit_status;
            uint8 peerCommitStatus = UpgradeCtxGet()->peercommitStatus;
            
            if(peerCommitStatus != hostStatus)
            {
                if(hostStatus > peerCommitStatus )
                {
                    UpgradeCtxGet()->commitStatus = peerCommitStatus;
                }
                else
                {
                    UpgradeCtxGet()->commitStatus = hostStatus;
                }
            }
            else
            {
                UpgradeCtxGet()->commitStatus = hostStatus;
            }
            /* tell host application we're complete */
            UpgradeCtxGet()->funcs->SendCommitStatusCfm(UpgradeCtxGet()->commitStatus);
            DEBUG_LOG_INFO("InformAppsCompleteGotoSync: Sending commit complete indication with \
                status, %d", UpgradeCtxGet()->commitStatus);
        }
        else
        {
            /* tell host application we're complete */
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_COMPLETE_IND);
            DEBUG_LOG_INFO("InformAppsCompleteGotoSync: Sending commit complete indication without status");
        }
        /* TODO: Work on proper fix. Adding delay temporarily to allow above message
        * to reach to host before it gets disconnected. */
        /* tell VM application we're complete */
        UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_done, 2000);
    }
    else
    {
        /* Delay is not needed in case of silent commit because we are not sending
        * any message to the host. */
        UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_done, 0);
    }

    /* go back to SYNC state to be ready to start again */
    UpgradeSMSetState(UPGRADE_STATE_SYNC);
}

/*
NAME
    upgradeSm_HandleCommit

DESCRIPTION
    Handle event messages in the Commit state of the FSM.
*/
bool upgradeSm_HandleCommit(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_INTERNAL_CONTINUE:
        {
            UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERASE;
            UpgradeSavePSKeys();
            DEBUG_LOG_DEBUG("upgradeSm_HandleCommit: UPGRADE_RESUME_POINT_ERASE saved");

            /* After the commit, current bank is the new boot bank so,
             * we do not need to reboot. */
            UpgradeCtxGet()->isImageRevertNeededOnAbort = FALSE;

            /* We erase all partitions at this point.
             * We have taken the hit of disrupting audio etc. by doing a reboot
             * to get here.
             */
            UpgradeCtxGetPSKeys()->version = UpgradeCtxGetPSKeys()->version_in_progress;
            UpgradeCtxGetPSKeys()->config_version = UpgradeCtxGetPSKeys()->config_version_in_progress;

            /* only erase if we already have permission, get permission if we don't */
            if (UpgradeSMHavePermissionToProceed(UPGRADE_BLOCKING_IND))
            {
                UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_notify_host_of_upgrade_complete, NO_ACTION);
            }
        }
        break;

    case UPGRADE_HOST_ABORT_REQ:
        /* ignore abort from host when we are in the commit state */
        DEBUG_LOG("upgradeSm_HandleCommit UPGRADE_HOST_ABORT_REQ recvd but ignored");
        break;

        /* VM application permission granted for erase */
    case UPGRADE_INTERNAL_ERASE:
        {
             UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_handle_notify_host_of_commit, NO_ACTION);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool upgradeSm_HandlePsJournal(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

/*
NAME
    PsFloodAndReboot

DESCRIPTION
    Process the required actions from upgradeSm_HandleRebootToResume.

    Flood PS to force a defrag on next boot, then do a warm reboot.

    Called either immediately or once we receive permission from
    the VM application.
*/
static void PsFloodAndReboot(void)
{
    PsFlood();
    BootSetMode(BootGetMode());
}

/*
NAME
    upgradeSm_HandleRebootToResume - In UPGRADE_STATE_REBOOT_TO_RESUME

DESCRIPTION
    Unable to continue with an upgrade as we cannot guarantee the required
    PSKEY operations.

    Once the error that got us to this state is acknowledged we will
    reboot if the VM application permits, otherwise remaining in this state
    handling all messages that might otherwise cause an activity.
*/
bool upgradeSm_HandleRebootToResume(MessageId id, Message message)
{
    UPGRADE_HOST_ERRORWARN_RES_T *errorwarn_res = (UPGRADE_HOST_ERRORWARN_RES_T *)message;

    switch(id)
    {
    case UPGRADE_HOST_ERRORWARN_RES:
        if (errorwarn_res->errorCode == UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE)
        {
            if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
            {
                PsFloodAndReboot();
            }
        }
        else
        {
            /* Message not handled */
            return FALSE;
        }
        break;
    
        /* got permission from the application, go ahead with reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            PsFloodAndReboot();
        }
        break;

    case UPGRADE_HOST_SYNC_REQ:
    case UPGRADE_HOST_START_REQ:
    case UPGRADE_HOST_ABORT_REQ:
        /*! @todo: Should we be sending the respective CFM messages back in this case ?
         *
         * In most cases there is no error code.
         */
        DEBUG_LOG("upgradeSm_HandleRebootToResume, cmd_id:%d recvd and notified", id);
        UpgradeCtxGet()->funcs->SendErrorInd(UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


static void upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq(const UPGRADE_HOST_SYNC_REQ_T *sync_req)
{
    UpgradeCtx *ctx = UpgradeCtxGet();
    UPGRADE_LIB_PSKEY *upg_pskeys = UpgradeCtxGetPSKeys();

    DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, in_progress_id 0x%lx", sync_req->inProgressId);

    /* reset on ever sync */
    ctx->force_erase = FALSE;
    ctx->isDfuAbortDueToSyncIdMismatch = FALSE;

    /* refuse to sync if upgrade is not permitted */
    if (ctx->perms == upgrade_perm_no)
    {
        DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, not permitted");
        ctx->funcs->SendErrorInd(UPGRADE_HOST_ERROR_APP_NOT_READY);
    }
    /* Send a warning if a pending case DFU needs to be aborted. */
    else if(UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_ABORTING)
    {
        DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, rebooted during case DFU");
        ctx->funcs->SendErrorInd(UPGRADE_HOST_ERROR_INTERNAL_ERROR_1);
    }
    /* Check upgrade ID */
    else if(sync_req->inProgressId == 0)
    {
        if(UpgradeIsSilentCommitEnabled())
        {
            DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, zero sync id to abort pending silent commit");
            ctx->funcs->SendErrorInd(UPGRADE_HOST_WARN_SYNC_ID_IS_ZERO);
        }
        else
        {
            DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, invalid sync id");
            ctx->funcs->SendErrorInd(UPGRADE_HOST_ERROR_INVALID_SYNC_ID);
        }
    }
    else if(   upg_pskeys->id_in_progress == 0
            || upg_pskeys->id_in_progress == sync_req->inProgressId)
    {
        DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, allowed, id_in_progress 0x%lx", upg_pskeys->id_in_progress);

        ctx->funcs->SendSyncCfm(upg_pskeys->upgrade_in_progress_key, sync_req->inProgressId);

        upg_pskeys->id_in_progress = sync_req->inProgressId;
        /*!
            @todo Need to minimise the number of times that we write to the PS
                  so this may not be the optimal place. It will do for now.
        */
        UpgradeSavePSKeys();

        UpgradeSMSetState(UPGRADE_STATE_READY);

    }
    else
    {
        DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq, expecting 0x%lx", upg_pskeys->id_in_progress);

        /* Send a warning to a host, which then can force upgrade with this
           file by sending ABORT_REQ and SYNC_REQ again.
         */
        ctx->funcs->SendErrorInd(UPGRADE_HOST_WARN_SYNC_ID_IS_DIFFERENT);
    }
}

static void upgradeSmupgradeSm_DefaultHandlerHandleUpgradeSilentCommitSupportedReq(void)
{
    UpgradeCtx *ctx = UpgradeCtxGet();

    DEBUG_LOG_INFO("upgradeSmupgradeSm_DefaultHandlerHandleUpgradeSilentCommitSupportedReq isSilentCommitSupported %d",
                    ctx->isSilentCommitSupported);
    ctx->funcs->SendSilentCommitSupportedCfm(ctx->isSilentCommitSupported);
}

/* Send a message to Device upgrade to be sent to application for calling 
 * DFU specific routine for cleanup process
 */
void UpgradeCleanupOnAbort(void)
{
    DEBUG_LOG("UpgradeCleanupOnAbort()");
    MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_CLEANUP_ON_ABORT, NULL);
}

/*
NAME
    upgradeSm_DefaultHandler - Deal with messages which we want to handle in all states

DESCRIPTION
    Default processing of messages which may be handled at any time.

    These are NOT normally processed if they have been dealt with already in
    the state machine, but this can be done.
 */
bool upgradeSm_DefaultHandler(MessageId id, Message message, bool handled)
{
    DEBUG_LOG_DEBUG("upgradeSm_DefaultHandler: id=MESSAGE:UpgradeMsgHost:%d, handled=%u", id, handled);

    if (!handled)
    {
        switch(id)
        {
        case UPGRADE_HOST_SYNC_REQ:
            upgradeSmupgradeSm_DefaultHandlerHandleUpgradeHostSyncReq((const UPGRADE_HOST_SYNC_REQ_T*)message);
            break;

        case UPGRADE_HOST_ABORT_REQ:
        {
            bool is_erasing = FALSE;
            /*
             * Host initiated abort (both for Primary and Secondary), shall be
             * handled here as upgrade can be any of the upgrade states.
             * Whereas for Primary device initiated abort owing to internal
             * errors reported via UpgradeFatalError(), shall be handled here owing to
             * following reason:
             * - Primary has transitioned to UPGRADE_STATE_ABORTING and sending
             *   UPGRADE_HOST_ERRORWARN_IND to the Host.
             * - Primary in response receives UPGRADE_HOST_ERRORWARN_RES from
             *   the Host. Primary can further relay this as
             *   UPGRADE_PEER_ABORT_REQ/UPGRADE_HOST_ABORT_REQ based on Peer
             *   (Secondary) upgrade has started. And Primary then tranistions
             *   to UPGRADE_STATE_SYNC.
             * - Subsequently UPGRADE_HOST_ABORT_REQ can be received from the
             *   Host.
             * - Hence UPGRADE_HOST_ABORT_REQ be handled here for the
             *   Primary as Primary has transitioned out of
             *   UPGRADE_STATE_ABORTING state.
             *
             */
            if(UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_ABORTING)
            {
                /* If we are aborting case DFU then cleanup Case DFU related context.
                 * Normally, application handles the case DFU related messages but if primary EB has been 
                 * reset during case DFU data transfer then, upgrade library aborts the case DFU when it 
                 * connects with the host without informing the application. 
                 */
                UpgradeSmCleanUpCaseDfu(TRUE);
            }
            else if(UpgradeCtxGet()->caseDfuStatus == UPGRADE_CASE_DFU_IN_PROGRESS)
            {
                /* In case, GAIA client is killed when case DFU is in progress,
                 * and the subsequent DFU started with a new DFU file, abort the
                 * current DFU and clear the states of case DFU.
                 */
                MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_CASE_DFU_CASE_ABORT, NULL);
            }
            else
            {
                is_erasing = UpgradeSMAbort();
            }
            DEBUG_LOG_DEBUG("upgradeSm_DefaultHandler: UpgradeSMAbort() is_erasing %u is_reverting %u", 
                            is_erasing, UpgradeCtxGet()->isImageRevertNeededOnAbort);
            if (!is_erasing)
            {
#ifndef HOSTED_TEST_ENVIRONMENT
                MessageSendConditionally(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_SEND_ABORT_CFM, NULL,
                                                    (const uint16*)&(UpgradeCtxGet()->waitForPeerAbort));
                UpgradeSMSetState(UPGRADE_STATE_SYNC);
#endif
            }
            else
            {
                UpgradeSMSetState(UPGRADE_STATE_ABORTING);
            }

            if(UpgradeCtxGet()->isImageRevertNeededOnAbort)
            {
                MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_DELAY_REVERT_REBOOT, NULL,
                                                           UPGRADE_WAIT_FOR_REBOOT);
            }
        }
        break;

        case UPGRADE_INTERNAL_DELAY_REVERT_REBOOT:
            /* Reboot the device to revert the commit. On detecting the boot bank after reboot, upgrade library will clear the PSKeys */
            DEBUG_LOG_INFO("upgradeSm_DefaultHandler UPGRADE_INTERNAL_DELAY_REVERT_REBOOT rebooting the device");
            BootSetMode(BootGetMode());
            break;

        case UPGRADE_HOST_VERSION_REQ:
            if (UpgradeCtxGet()->funcs->SendVersionCfm != NULL)
            {
                /* reply to UPGRADE_VERSION_REQ with UPGRADE_VERSION_CFM
                 * sending our current upgrade and PS config version numbers */
                UpgradeCtxGet()->funcs->SendVersionCfm(UpgradeCtxGetPSKeys()->version.major,
                                                UpgradeCtxGetPSKeys()->version.minor,
                                                UpgradeCtxGetPSKeys()->config_version);
            }
            break;

        case UPGRADE_HOST_VARIANT_REQ:
            if (UpgradeCtxGet()->funcs->SendVariantCfm != NULL)
            {
                UpgradeCtxGet()->funcs->SendVariantCfm(UpgradeFWIFGetDeviceVariant());
            }
            break;

        case UPGRADE_INTERNAL_BATTERY_LOW:
            UpgradeSMSetState(UPGRADE_STATE_BATTERY_LOW);
            break;

        /* got required permission from VM app, erase and return to SYNC state */
        case UPGRADE_INTERNAL_ERASE:
            {
                UpgradeSMErase();
                UpgradeSMSetState(UPGRADE_STATE_SYNC);
            }
            break;

       case UPGRADE_HOST_ERRORWARN_RES:
            {
                UPGRADE_HOST_ERRORWARN_IND_T *msg = (UPGRADE_HOST_ERRORWARN_IND_T *)message;
                /* If the ERRORWARN_RES is due to Sync ID mismatch, update the 
                 * isDfuAbortDueToSyncIdMismatch of upgradeCtx
                 */
                if(msg->errorCode == UPGRADE_HOST_WARN_SYNC_ID_IS_DIFFERENT)
                {
                    UpgradeCtxGet()->isDfuAbortDueToSyncIdMismatch = TRUE;
                }

                UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_clean_up_on_abort, NO_ACTION);
            }
            break;

        case UPGRADE_HOST_COMMIT_CFM:
            {
                /* This message should have been handled in UPGRADE_STATE_COMMIT_CONFIRM. We are here because peer device 
                 * is not in sync. We should abort the DFU*/
                 DEBUG_LOG("upgradeSm_DefaultHandler UPGRADE_HOST_COMMIT_CFM Abort due to incorrect state");
                 UpgradeFatalError(UPGRADE_HOST_ERROR_APP_NOT_READY);
            }
            break;

        case UPGRADE_HOST_SILENT_COMMIT_SUPPORTED_REQ:
            upgradeSmupgradeSm_DefaultHandlerHandleUpgradeSilentCommitSupportedReq();
            break;

        case UPGRADE_INTERNAL_SEND_ABORT_CFM_LATER:
            {
                DEBUG_LOG_INFO("upgradeSm_DefaultHandler Send UPGRADE_HOST_ABORT_CFM");
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_ABORT_CFM);

                UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort,UPGRADE_SEND_END_DATA_IND_WITHOUT_DELAY);
            }
            break;
       case UPGRADE_INTERNAL_SEND_ABORT_CFM:

            if(UpgradeIsAbortDfuDueToSyncIdMismatch())
            {
                /* If we are aborting DFU due to SYNC ID mismatch, then wait for 200ms
                 * before cleaning up the APPS state and sending Abort CFM to GAIA
                 * client. This is done to make sure dfu peer is disconnected
                 * before sending Abort CFM for earbud DFU. So that, if the new
                 * SYNC REQ arrives immediately, earbud is ready for new DFU.
                 */
                 MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_SEND_ABORT_CFM_LATER, NULL,
                                                           UPGRADE_SEND_ABORT_CFM_WITH_DELAY);
            }
            else
            {
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_ABORT_CFM);
                /* Let the application do the required clean up now after 300msec.
                 * The delay is added so that we are sure that the ABORT_CFM is
                 * sent to Host before disconnecting the GAIA link.
                 */
                UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort, 
                                             UPGRADE_SEND_END_DATA_IND_WITH_DELAY);
            }
            break;

        case UPGRADE_INTERNAL_CASE_DFU_RECONNECTION_TIMEOUT:
            UpgradeSmCleanUpCaseDfu(TRUE);
            break;

        case UPGRADE_INTERNAL_SCHEDULE_TRANSPORT_DISCONNECT_CFM:
        {
            MESSAGE_MAKE(disconnectCfm, UPGRADE_TRANSPORT_DISCONNECT_CFM_T);
            disconnectCfm->status = 0;

            UPGRADE_INTERNAL_SCHEDULE_TRANSPORT_DISCONNECT_CFM_T* msg = (UPGRADE_INTERNAL_SCHEDULE_TRANSPORT_DISCONNECT_CFM_T*) message;
#ifndef HOSTED_TEST_ENVIRONMENT
            MessageSendConditionally(msg->transportTask, UPGRADE_TRANSPORT_DISCONNECT_CFM, disconnectCfm,(uint16 *)&UpgradeCtxGet()->delayDataCfm);
#endif
        }
        break;

        default:
            return FALSE;
        }
        handled = TRUE;
    }

    return handled;
}

void UpgradeSMSetState(UpgradeState nextState)
{
    UpgradeCtxGet()->smState = nextState;
}

void UpgradeSMMoveToState(UpgradeState nextState)
{
    UpgradeSMSetState(nextState);
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_CONTINUE, NULL);
}

void UpgradeFatalError(UpgradeHostErrorCode error_code)
{
    DEBUG_LOG("UpgradeFatalError: enum:UpgradeHostErrorCode:%d", error_code);
    UpgradeCtxGet()->funcs->SendErrorInd((uint16)error_code);
    UpgradeSMSetState(UPGRADE_STATE_ABORTING);

    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERROR;
    UpgradeSavePSKeys();
    /*
     * Defer notification to the application until the device initiated abort
     * are responded/acknowledged by the Host application with 
     * UPGRADE_HOST_ERRORWARN_RES and UPGRADE_HOST_ABORT_REQ.
     */
}

void upgradeSm_PsSpaceError(void)
{
    UpgradeCtxGet()->funcs->SendErrorInd((uint16)UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE);
    UpgradeSMSetState(UPGRADE_STATE_REBOOT_TO_RESUME);
}

/*
NAME
    UpgradeSMErase - Clean up after an upgrade, even if it was aborted.

DESCRIPTION
    UpgradeSMErase any partitions that will be needed for any future upgrade.
    Clear the transient data in the upgrade PS key data.
    It returns TRUE if the asynchronous erase is in progress.

    Note: Before calling this function make sure it is ok to erase
          partitions on the external flash because it will block other
          services during the erase.
*/
bool UpgradeSMErase(void)
{
    UpgradePartitionsState old_ipk;
    bool is_erasing = FALSE;

    DEBUG_LOG("UpgradeSMErase: begin");
    MessageCancelAll(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_CASE_DFU_RECONNECTION_TIMEOUT);

    UpgradePartitionDataCtx *pdatactx = UpgradeCtxGetPartitionData();

    UpgradeCtx *ctx = UpgradeCtxGet();

    if (pdatactx)
    {
        /* If a partition is currently open, close it now.
           Otherwise the f/w will think it is in use and not erase it. */
        if (pdatactx->partitionHdl)
        {
             UpgradeFWIFPartitionClose(pdatactx->partitionHdl);
             pdatactx->partitionHdl = NULL;
        }
    }

    /* If the hash check is interuppted by aborting the DFU process by user,
     * reset the hash ctx, so that in the next round of DFU, stale
     * values are not used.
     */
    if(ctx)
    {
        ctx->vctx = NULL;
    }

    /* Free the partition related contex memory */
    UpgradePartitionDataDestroy();

    /*
     * Store upgrade_in_progress_key since pskey is stored prior to assessment
     * whether the alternate bank is required to be erased.
     */
    old_ipk = UpgradeCtxGetPSKeys()->upgrade_in_progress_key;

    /* Reset transient state data in upgrade pskey (i.e. both local & peer). */
    UpgradePartitionsUpgradeStarted();
    /*
     * Note: UpgradePartitionsUpgradeStarted() needs to preceed
     *       UpgradeCtxClearPSKeys() to be eventually stored via psstore.
     */
    UpgradeCtxClearPSKeys();
    UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_clear_peer_pskeys, NO_ACTION);

    /* ClearHeaderPSKeys */
    UpgradeClearHeaderPSKeys();

    DEBUG_LOG_DEBUG("UpgradeSMErase: UPGRADE_RESUME_POINT_START saved");

    /* UpgradeSMErase any partitions that require it. */
    /*
     * Restore upgrade_in_progress_key as alternate bank is now actually
     * assessed for erase.
     */
    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = old_ipk;
    is_erasing = (UpgradePartitionsEraseAllManaged() == UPGRADE_PARTITIONS_ERASING);
    /* Synchronize to psstore saved state (above). */
    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_START;

    ctx->dfu_file_offset = 0;

    ctx->transferCompleteResReceived = FALSE;

    /* Let application know that erase is done */
    if (UpgradeSMCheckEraseComplete())
    {
        UpgradeSMBlockingOpIsDone();
    }
    DEBUG_LOG("UpgradeSMErase: end");
    return is_erasing;
}

/***************************************************************************
NAME
    UpgradeSmCommitConfirmYes

DESCRIPTION
    Commit the new image
*/
void UpgradeSmCommitConfirmYes(void)
{
    
    if(UpgradeIsSilentCommitEnabled())
    {
        /* Cancel reconnection timer */
        MessageCancelFirst(UpgradeGetUpgradeTask(),
                           UPGRADE_INTERNAL_SILENT_COMMIT_RECONNECTION_TIMEOUT);
    }

    UpgradeFWIFCommitUpgrades();
    
    /* tell VM application we're committing the upgrade */
    UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_commiting, 0);

    UpgradeSMMoveToState(UPGRADE_STATE_COMMIT);
}

/***************************************************************************
NAME
    UpgradeSMUpgradeInProgress

DESCRIPTION
    Return boolean indicating if an upgrade is currently in progress.
*/
bool UpgradeSMUpgradeInProgress(void)
{
    return (UpgradeSMGetState() >= UPGRADE_STATE_READY);
}

/***************************************************************************
NAME
    UpgradeSMHavePermissionToProceed

DESCRIPTION
    Decide if we should perform an action now, not at all, or ask the
    application for permission.
    
*/
bool UpgradeSMHavePermissionToProceed(MessageId id)
{
    DEBUG_LOG("UpgradeSMHavePermissionToProceed perms:%d MESSAGE:upgrade_application_message:0x%04X",
              UpgradeCtxGet()->perms, id);

    switch (UpgradeCtxGet()->perms)
    {
        /* we currently have no permission to upgrade, so no permission
         * to do anything */
        case upgrade_perm_no:
            {
                /* @todo if we're here, for reboot or erase, but have no
                 * permission to do so, should we clean up/abort?
                 */
                return FALSE;
            }

            /* we have permission to do anything without question */
        case upgrade_perm_assume_yes:
            return TRUE;

            /* we have permission, but must ask the application, send
             * a message AND return FALSE so the caller doesn't do anything
             * yet, but waits until the application responds. */
        case upgrade_perm_always_ask:
            MessageSend(UpgradeCtxGet()->mainTask, id, NULL);
            return FALSE;
    }

    return FALSE;
}

/***************************************************************************
NAME
    UpgradeSMBlockingOpIsDone

DESCRIPTION
    Let know application that blocking operation is finished.
*/
void UpgradeSMBlockingOpIsDone(void)
{
    if(UpgradeCtxGet()->perms == upgrade_perm_always_ask)
    {
        /* Cancelling messages shouldn't be needed but do it anyway */
        MessageCancelAll(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IND);
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IS_DONE_IND, NULL);
    }
}

/***************************************************************************
NAME
    UpgradeSendUpgradeStatusInd

DESCRIPTION
    Build and send an UPGRADE_STATUS_IND message to the VM application.
*/
void UpgradeSendUpgradeStatusInd(Task task, upgrade_state_t state, uint32 delay)
{
    UPGRADE_STATUS_IND_T *upgradeStatusInd = (UPGRADE_STATUS_IND_T *)PanicUnlessMalloc(sizeof(UPGRADE_STATUS_IND_T));
    upgradeStatusInd->state = state;

    if(delay == 0)
        MessageSend(task, UPGRADE_STATUS_IND, upgradeStatusInd);
    else
        MessageSendLater(task, UPGRADE_STATUS_IND, upgradeStatusInd, delay);
}

#ifndef MESSAGE_IMAGE_UPGRADE_COPY_STATUS
/*
 * In the host tests it is picking up the BlueCore version of systme_message.h
 * rather the CONFIG_HYDRACORE version, so define the structure here.
 */
typedef struct
{
    bool copy_status; /*!< TRUE if image copy is successful, else FALSE */
} MessageImageUpgradeCopyStatus;
#endif

#ifdef MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS
void UpgradeSMCopyAudioStatus(Message message)
{
    MessageImageUpgradeAudioStatus *msg = (MessageImageUpgradeAudioStatus *)message;
    DEBUG_LOG("UpgradeSMCopyAudioStatus, audio_status %u", msg->audio_status);
    /* Let application know that copy is done */
    UpgradeSMBlockingOpIsDone();

    if (msg->audio_status)
    {
        /*
         * The SQIF has been copied successfully.
         */
        UpgradeSMHandleMsg(UPGRADE_VM_DFU_COPY_VALIDATION_SUCCESS, NULL);
    }
    else
    {
        /* Tell the host that the attempt to copy the SQIF failed. */
        UpgradeFatalError(UPGRADE_HOST_ERROR_AUDIO_SQIF_COPY);
        UpgradeSMHandleMsg(UPGRADE_VM_AUDIO_DFU_FAILURE, NULL);
    }
}
#endif

void UpgradeErrorMsgFromUpgradePeer(uint16 error)
{
    DEBUG_LOG("UpgradeErrorMsgFromUpgradePeer: Peer (Secondary) initiated Abort");
    UpgradeFatalError((UpgradeHostErrorCode)error);
}

void UpgradeCommitMsgFromUpgradePeer(void)
{
        bool dfu = UpgradePartitionDataIsDfuUpdate();
        uint16 err = UpgradeSMNewImageStatus();

        if(err)
        {
            /* TODO: Delete transient store = revert upgrade */
            UpgradeFatalError(err);
        }
        else
        {
            /*UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_NONE;
            UpgradeCtxGetPSKeys()->dfu_partition_num = 0;
            UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_COMMIT;

            UpgradeSavePSKeys();
            DEBUG_LOG_DEBUG("UpgradeCommitMsgFromUpgradePeer: UPGRADE_RESUME_POINT_COMMIT saved");*/

            if(dfu)
            {
                UpgradeSMMoveToState(UPGRADE_STATE_COMMIT_CONFIRM);
            }
            else
            {
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_COMMIT_REQ);
                UpgradeSMSetState(UPGRADE_STATE_COMMIT_CONFIRM);
            }
        }
}

void UpgradeCompleteMsgFromUpgradePeer(uint8 status)
{
    DEBUG_LOG("UpgradeCompleteMsgFromUpgradePeer: Peer has successfully commited, commit status %d", status);
    
    UpgradeSmCommitConfirmYes();
    /* save peer commit status for future use */
    UpgradeCtxGet()->peercommitStatus = status;
}

/****************************************************************************
NAME
    UpgradeCommitConfirmForSilentCommit

DESCRIPTION
     Auto commit new image (without host intervention) if silent commit is
     selected.

RETURNS
    None
*/
void UpgradeCommitConfirmForSilentCommit(void)
{
    UpgradeSmCommitConfirmYes();
}

/***************************************************************************
NAME
    UpgradeSendUpgradeOpsStatus

DESCRIPTION
    Build and send an UPGRADE_OPERATION_IND_T message to the DFU domain application.
*/
void UpgradeSendUpgradeOpsStatus(Task task, upgrade_ops_t ops, uint8 action)
{
    UPGRADE_OPERATION_IND_T *upgradeOperationInd = (UPGRADE_OPERATION_IND_T *)PanicUnlessMalloc(sizeof(UPGRADE_OPERATION_IND_T));
    upgradeOperationInd->ops = ops;
    upgradeOperationInd->action = action;

    MessageSend(task, UPGRADE_OPERATION_IND, upgradeOperationInd);
}

/***************************************************************************
NAME
    UpgradeFatalErrorAppNotReady

DESCRIPTION
    Wrapper function to invoke UpgradeFatalError(UPGRADE_HOST_ERROR_APP_NOT_READY)
*/

void UpgradeFatalErrorAppNotReady(void)
{
    UpgradeFatalError(UPGRADE_HOST_ERROR_APP_NOT_READY);
}
/***************************************************************************
NAME
    UpgradeSmSendHostInProgressInd

DESCRIPTION
    Handle sending Host IN_PROGRESS_IND based on peer DFU starts and DFU states
*/
void UpgradeSmSendHostInProgressInd(bool is_peer_dfu_started, bool is_state_commit_host_continue)
{
    if(is_peer_dfu_started)
    {
        if(is_state_commit_host_continue)
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_IN_PROGRESS_IND);
        else
            UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
    }
    else
    {
        UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_IN_PROGRESS_IND);
        UpgradeSMSetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
    }
}

/***************************************************************************
NAME
    UpgradeSmHandleCommitVerifyProceed

DESCRIPTION
    Handle Commit Verification Proceeding
*/
void UpgradeSmHandleCommitVerifyProceed(void)
{
    UpgradeFWIFApplicationValidationStatus status;
    
    status = UpgradeFWIFValidateApplication();
    if (UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP != status)
    {                
        if (UPGRADE_FW_IF_APPLICATION_VALIDATION_RUNNING == status)
        {
            UpgradeSMMoveToState(UPGRADE_STATE_COMMIT_VERIFICATION);
        }
        else
        {
            UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_COMMIT_REQ);
            UpgradeSMSetState(UPGRADE_STATE_COMMIT_CONFIRM);
        }
    }
    else
    {
        bool dfu = UpgradePartitionDataIsDfuUpdate();
        uint16 err = UpgradeSMNewImageStatus();
    
        if(err)
        {
            /* TODO: Delete transient store = revert upgrade */
            UpgradeFatalError(err);
        }
        else
        {
            if(dfu)
            {
                UpgradeSMMoveToState(UPGRADE_STATE_COMMIT_CONFIRM);
            }
            else
            {
                UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_COMMIT_REQ);
                UpgradeSMSetState(UPGRADE_STATE_COMMIT_CONFIRM);
            }
        }
    }
}

/***************************************************************************
NAME
    UpgradeSmHandleProceedToCommit

DESCRIPTION
    Handle Upgrade commit state after reboot
*/
void UpgradeSmHandleProceedToCommit(bool is_peer_not_connected, uint32 peer_poll_interval, uint8 action)
{
    if(is_peer_not_connected && peer_poll_interval)
    {
        UPGRADE_HOST_PROCEED_TO_COMMIT_T *msg;
        msg = (UPGRADE_HOST_PROCEED_TO_COMMIT_T *)
                    PanicUnlessMalloc(sizeof(UPGRADE_HOST_PROCEED_TO_COMMIT_T));
        msg->action = action;
        MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_HOST_PROCEED_TO_COMMIT,
                         msg, peer_poll_interval);
    }
    else
    {
        if(action == 0)
        {
            UpgradeSMMoveToState(UPGRADE_STATE_COMMIT_VERIFICATION);
        }
        else
        {
            UpgradeSMMoveToState(UPGRADE_STATE_SYNC);
        }
    }
}

/***************************************************************************
NAME
    UpgradeSmHandleNotifyHostOfCommit

DESCRIPTION
    Handle Notifying Host of Commit based on the type of commit
*/
void UpgradeSmHandleNotifyHostOfCommit(bool has_peer_dfu_not_ended, uint32 peer_poll_interval)
{
    bool is_silent_commit = UpgradeIsSilentCommitEnabled();
    if(has_peer_dfu_not_ended)
    {
        MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_ERASE,
                         NULL, peer_poll_interval);
    }
    else
    {
        UpgradeSMErase();
        InformAppsCompleteGotoSync(is_silent_commit);
    }
}

/***************************************************************************
NAME
    UpgradeSmHandleNotifyHostOfComplete

DESCRIPTION
    Handle Notifying Host of Upgrade Completion based on the type of commit
*/
void UpgradeSmHandleNotifyHostOfComplete(bool is_silent_commit, uint32 peer_poll_interval, bool is_primary)
{
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
    if(!is_primary)
    {
        UpgradeSMErase();
        /* Secondary should always send Complete_ind to peer even
         * during the silent commit.*/
        InformAppsCompleteGotoSync(FALSE);
    }
    else if(is_silent_commit)
    {
        UpgradeSMErase();
        InformAppsCompleteGotoSync(is_silent_commit);
    }
    else 
    {
        MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_ERASE,
                         NULL, peer_poll_interval);
    }
}

/****************************************************************************
NAME
    UpgradeSmCleanUpCaseDfu

DESCRIPTION
    Clean-up the upgrade library context after the case DFU ends.
*/
void UpgradeSmCleanUpCaseDfu(bool isError)
{
    UpgradeSMErase();
    UpgradeCtxGet()->caseDfuHeaderLength = 0;
    UpgradeCtxGet()->caseDfuStatus = UPGRADE_CASE_DFU_NOT_STARTED;
    UpgradeCtxGet()->isTransportCfmDelayed = FALSE;
    UpgradeCtxGet()->delayDataCfm = FALSE;

    if(isError)
    {
        if(!UpgradeIsAbortDfuDueToSyncIdMismatch())
        {
            UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort, UPGRADE_SEND_END_DATA_IND_WITH_DELAY);
        }
    }
    else
    {
        UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_done, UPGRADE_SEND_STATUS_IND_WITH_DELAY);
    }

    UpgradeSMSetState(UPGRADE_STATE_SYNC);
}

/***************************************************************************
NAME
    UpgradeSmIsStateCommitHostContinue

DESCRIPTION
    Check if the UpgradeSM state is set as UPGRADE_STATE_COMMIT_HOST_CONTINUE
*/
bool UpgradeSmIsStateCommitHostContinue(void)
{
    return (UpgradeSMGetState() == UPGRADE_STATE_COMMIT_HOST_CONTINUE);
}

/***************************************************************************
NAME
    UpgradeSmStateIsDataHashChecking

DESCRIPTION
    Check if the Partition hash checking is currently going on
*/
bool UpgradeSmStateIsDataHashChecking(void)
{
    return UpgradePartitionDataStateIsHashCheck();
}

/***************************************************************************
NAME
    UpgradeSmStateIsBeforeDataTransfer

DESCRIPTION
    Check if the UpgradeSM state is less as DATA_TRANSFER
*/

bool UpgradeSmStateIsBeforeDataTransfer(void)
{
    return (UpgradeSMGetState() < UPGRADE_STATE_DATA_TRANSFER);
}


/*
NAME
    UpgradeSMCheckEraseComplete

DESCRIPTION
    Indicate whether the erase has completed.
    Returns FALSE for CONFIG_HYDRACORE as UpgradePartitionsEraseAllManaged
    is non-blocking and completion is indicated by the
    MESSAGE_IMAGE_UPGRADE_ERASE_STATUS message.
*/
bool UpgradeSMCheckEraseComplete(void)
{
    if (UPGRADE_RESUME_POINT_ERASE != UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
        if(UpgradeCtxGet()->smState == UPGRADE_STATE_COMMIT)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return TRUE;
    }
}

/*
NAME
    FileTransferCompleted - Operation post file transfer completion

DESCRIPTION
    If it is a silent commit, only store silent commit flag in PSKEY and inform
    peer (if applicable).
    If it is either interactive commit, or a go ahead for reboot post silent
    commit, store resume point in PSKEY and initiate DFU reboot.
*/
void FileTransferCompleted(bool is_silent_commit, bool inform_peer)
{
    uint8 action = (uint8)is_silent_commit;
    UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_init_peer_context, NO_ACTION);
    if(inform_peer)
    {
        UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_trnsfr_complt_res_send_to_peer,
                                    action);
    }

    if(is_silent_commit)
    {
        UpgradeCtxGetPSKeys()->is_silent_commit = UPGRADE_COMMIT_SILENT;
        UpgradeSavePSKeys();
        DEBUG_LOG_DEBUG("FileTransferCompleted: is_silent_commit saved");
        /* tell host application we're complete */
        UpgradeCtxGet()->funcs->SendShortMsg(UPGRADE_HOST_SILENT_COMMIT_CFM);
        /* If the current device is not a primary device then, inform the application 
         * that silent commit command has been received from the host.
         * primary device should wait for secondary to process the command first. */

        UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_send_silent_commit_ind_to_host, NO_ACTION);
    }
    else
    {
#ifndef HOSTED_TEST_ENVIRONMENT
        if(UPGRADE_IS_PEER_SUPPORTED)
        {
            /* After the UPGRADE_PEER_TRANSFER_COMPLETE_RES msg is sent to the
             *  peer device, the primary device wait for 1 sec before
             * reboot. The handling of device reboot will be done in
             * UPGRADE_INTERNAL_TRIGGER_REBOOT case.
             */
            DEBUG_LOG_DEBUG("FileTransferCompleted: Reboot after 1sec");
            MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_TRIGGER_REBOOT,
                             NULL, UPGRADE_WAIT_FOR_REBOOT);
        }
        else
        {
            /* For a standalone DFU, immediately go ahead and reboot. The handling
             * of device reboot will be done in UPGRADE_INTERNAL_TRIGGER_REBOOT case.
             */
            DEBUG_LOG_INFO("FileTransferCompleted: Reboot now");
            MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_TRIGGER_REBOOT, NULL);
        }
#endif
    }
}

/*
NAME
    UpgradeSMHandleRebootAction - Handle Upgrade Reboot Process

DESCRIPTION
    Based on commit mode, handle the reboot process of Upgrade
*/

void UpgradeSMHandleRebootAction(void)
{
        UpgradeCtx *ctx = UpgradeCtxGet();
        /* Interactive Commit */
        if(ctx->commitMode == UPGRADE_COMMIT_INTERACTIVE)
        {
            DEBUG_LOG_INFO("UpgradeSMHandleRebootAction: Interactive Commit");
            /* Host could have reconnected and selected interactive commit while a 
             * silent commit was pending so, reset silent commit flag if its set. */
            if(UpgradeCtxGetPSKeys()->is_silent_commit)
            {
                UpgradeCtxGetPSKeys()->is_silent_commit = FALSE;
                UpgradeSavePSKeys();
            }
            /* Initiate DFU reboot */
            FileTransferCompleted(FALSE, TRUE);
        }

        /* Silent Commit */
        else if(ctx->commitMode == UPGRADE_COMMIT_SILENT)
        {
            DEBUG_LOG_INFO("UpgradeSMHandleRebootAction: Silent Commit");

            /* If the host sends silent commit request and device does not support it,
             * send error message to the host. */
            if(!UpgradeCtxGet()->isSilentCommitSupported)
            {
                UpgradeFatalError(UPGRADE_HOST_ERROR_SILENT_COMMIT_NOT_SUPPORTED);
            }
            else
            {
                /* For silent commit, update PSKEY but do not initiate DFU reboot */
                FileTransferCompleted(TRUE, TRUE);
            }
        }
        /* Abort */
        else
        {
            UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(),
            upgrade_ops_abort_post_transfer_complete, NO_ACTION);
        }
}

/*
NAME
    UpgradeSMAbort - Clean everything and go to the sync state.

DESCRIPTION
    Common handler for clearing up an upgrade after an abort
    and going back to a state that is ready for a new upgrade.
*/
bool UpgradeSMAbort(void)
{
    /*If we received an abort request before starting the DFU*/
    if(UpgradeInProgressId() == 0)
    {
        DEBUG_LOG("UpgradeSMAbort return false to inform synchronous abort, Upgrade not yet started");
        return FALSE;
    }

    UpgradeSendUpgradeOpsStatus(UpgradeGetAppTask(), upgrade_ops_handle_abort, NO_ACTION);

    /* if we are going to reboot to revert commit, then we are alredy running from alternate bank
     * so, we shouldn't erase. return False to inform synchronous abort*/
    if(UpgradeCtxGet()->isImageRevertNeededOnAbort)
    {
        DEBUG_LOG("UpgradeSMAbort return false to inform synchronous abort and without erase");
        return FALSE;
    }


#ifdef MESSAGE_IMAGE_UPGRADE_COPY_STATUS
    /*
     * There may be non-blocking traps such as ImageUpgradeCopy in progress.
     * Call the ImageUpgradeAbortCommand() trap to abort any of those. It
     * will do no harm if there are no non-blocking traps in progress.
     */
    DEBUG_LOG_DEBUG("UpgradeSMAbort: ImageUpgradeAbortCommand()");
    ImageUpgradeAbortCommand();
#endif  /* MESSAGE_IMAGE_UPGRADE_COPY_STATUS */
    return UpgradeSMErase();
}

/*
NAME
    UpgradeSMNewImageStatus - Check the ImageUpgradeSwapTryStatus()

DESCRIPTION
    Return the status of ImageUpgradeSwapTryStatus()
*/
uint16 UpgradeSMNewImageStatus(void)
{
    uint16 err = 0;
    bool result = ImageUpgradeSwapTryStatus();
    DEBUG_LOG_DEBUG("UpgradeSMNewImageStatus: ImageUpgradeSwapTryStatus() = %u", result);
    if (!result)
    {
        err = UPGRADE_HOST_ERROR_LOADER_ERROR;
    }
    return err;
}


/*
NAME
    UpgradeSMHandleAbort

DESCRIPTION
    Perform Upgrade Abort related activities
*/
void UpgradeSMHandleAbort(void)
{
    UpgradeSendEndUpgradeDataInd(upgrade_end_state_abort,
                                 UPGRADE_SEND_END_DATA_IND_WITHOUT_DELAY);
    UpgradeSMMoveToState(UPGRADE_STATE_SYNC);
}

/*
NAME
    UpgradeSMHandleAudioDFU

DESCRIPTION
    Calls UpgradeSMHandleAudioImageCopy().
*/
void UpgradeSMHandleAudioDFU(void)
{
#ifdef MESSAGE_IMAGE_UPGRADE_AUDIO_STATUS
    DEBUG_LOG_DEBUG("UpgradeSMHandleAudioDFU: ImageUpgradeAudio()");
    ImageUpgradeAudio();
#endif
}

/*
NAME
    UpgradeSMSetPermission
DESCRIPTION
    Set the Upgrade Ctx perms
*/
void UpgradeSMSetPermission(upgrade_permission_t perm)
{
    UpgradeCtxGet()->perms = perm;
}

