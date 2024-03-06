/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    csip
    \brief      CSIP client
*/

#include "csip_client.h"
#include "csip_client_private.h"
#include "synergy.h"

/*! \brief CAP client task data. */
csip_client_task_data_t csip_client_taskdata;

/*! Sends CSIP_CLIENT_MSG_ID_INIT_COMPLETE to registered clients */
static void csipClient_SendInitCfm(csip_client_status_t status)
{
    csip_client_msg_t msg;

    msg.id = CSIP_CLIENT_MSG_ID_INIT_COMPLETE;
    msg.body.init_complete.status = status;

    csip_client_taskdata.callback_handler(&msg);
}

/*! Sends CSIP_CLIENT_MSG_ID_DEVICE_ADDED to registered clients */
static void csipClient_DeviceAddedCfm(gatt_cid_t cid, csip_client_status_t status, bool more_devices_needed)
{
    csip_client_msg_t msg;

    msg.id = CSIP_CLIENT_MSG_ID_DEVICE_ADDED;
    msg.body.device_added.cid = cid;
    msg.body.device_added.status = status;
    msg.body.device_added.more_devices_needed = more_devices_needed;

    csip_client_taskdata.callback_handler(&msg);
}

/*! Sends CSIP_CLIENT_MSG_ID_DEVICE_REMOVED to registered clients */
static void csipClient_DeviceRemovedCfm(gatt_cid_t cid, csip_client_status_t status, bool more_devices_present)
{
    csip_client_msg_t msg;

    msg.id = CSIP_CLIENT_MSG_ID_DEVICE_REMOVED;
    msg.body.device_removed.cid = cid;
    msg.body.device_removed.status = status;
    msg.body.device_removed.more_devices_present = more_devices_present;

    csip_client_taskdata.callback_handler(&msg);
}

/*! Sends CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT to registered clients */
static void csipClient_DisconnectCfm(csip_client_status_t status)
{
    csip_client_msg_t msg;

    msg.id = CSIP_CLIENT_MSG_ID_PROFILE_DISCONNECT;
    msg.body.disconnected.status = status;

    csip_client_taskdata.callback_handler(&msg);
}

static void CsipClient_ResetCsipInstance(csip_client_device_instance_t *instance)
{
    if (instance != NULL)
    {
        memset(instance, 0, sizeof(csip_client_device_instance_t));
    }
}

/*! \brief Function that checks whether the CAP client instance matches based on the compare type */
static bool csipClient_Compare(csip_client_instance_compare_by_type_t type,
                               unsigned compare_value,
                               csip_client_device_instance_t *instance)
{
    bool found = FALSE;

    switch (type)
    {
        case csip_client_compare_by_cid:
            found = instance->cid == (gatt_cid_t) compare_value;
        break;

        case csip_client_compare_by_state:
            found = instance->state == (csip_client_state_t) compare_value;
        break;

        case csip_client_compare_by_profile_handle:
            found = instance->csip_profile_handle == (CsipProfileHandle) compare_value;
        break;

        case csip_client_compare_by_valid_invalid_cid:
            found = instance->state == csip_client_state_connected &&
                    (instance->cid == (gatt_cid_t) compare_value || compare_value == INVALID_CID);
        break;

        default:
        break;
    }

    return found;
}

/*! \brief Get the CSIP instance based on the compare type */
static csip_client_device_instance_t * CsipClient_GetDeviceInstance(csip_client_instance_compare_by_type_t type, unsigned cmp_value)
{
    csip_client_device_instance_t *instance = NULL;
    csip_client_task_data_t *client_context = CsipClient_GetContext();

    for (instance = &client_context->device_instance[0];
         instance < &client_context->device_instance[MAX_CSIP_DEVICES_SUPPORTED];
         instance++)
    {
        if (csipClient_Compare(type, cmp_value, instance))
        {
            return instance;
        }
    }

    return NULL;
}

/*! \brief Get the number of connected devices in the coordinated set */
static uint8 CsipClient_GetConnectedDeviceCount(void)
{
    uint8 i, device_count = 0;
    csip_client_task_data_t *client_context = CsipClient_GetContext();

    for (i = 0; i < MAX_CSIP_DEVICES_SUPPORTED; i++)
    {
        if (client_context->device_instance[i].state == csip_client_state_connected)
        {
            device_count++;
        }
    }

    return device_count;
}

static void CsipClient_PrintCsipInfo(CsipProfileHandle profile_handle)
{
    uint8 i;
    csip_client_device_instance_t *instance;

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_profile_handle, (unsigned)profile_handle);

    if (instance != NULL)
    {
        DEBUG_LOG("CsipClient_PrintCsipInfo cid:0x%x set_size:%d rank:%d lock:%d sirk_encrypted:%d",
                   instance->cid, instance->set_size, instance->rank, instance->lock_status, instance->sirk_encrypted);

        DEBUG_LOG("CsipClient_PrintCsipInfo SIRK:");
        for (i = 0; i < CSIP_SIRK_SIZE; i++)
        {
            DEBUG_LOG("0x%x", instance->sirk[i]);
        }
    }
}

/*! \brief Handle the CSIP initialisation confirm message */
static void csipClient_HandleCsipInitCfm(CsipInitCfm *cfm)
{
    csip_client_device_instance_t *instance;

    DEBUG_LOG("csipClient_HandleCsipInitCfm cid 0x%04x prflHndl 0x%04x status %d",
               cfm->cid, cfm->prflHndl, cfm->status);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_cid, (unsigned)cfm->cid);
    PanicNull(instance);

    if (cfm->status == CSIP_STATUS_SUCCESS)
    {
        instance->csip_profile_handle = cfm->prflHndl;

        /* Need to read the sirk, set size, rank and lock status. First read the SIRK */
        CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_SIRK);

        GattCsisClientDeviceData *data = CsipGetAttributeHandles(instance->csip_profile_handle);
        if (data != NULL)
        {
            DEBUG_LOG("csipClient_HandleCsipInitCfm start handle: 0x%x end handle 0x%x", data->startHandle, data->endHandle);
            DEBUG_LOG("csipClient_HandleCsipInitCfm csisSirkHandle: 0x%x csisSizeHandle 0x%x", data->csisSirkHandle, data->csisSizeHandle);
            DEBUG_LOG("csipClient_HandleCsipInitCfm csisLockHandle: 0x%x csisRankHandle 0x%x", data->csisLockHandle, data->csisRankHandle);
            DEBUG_LOG("csipClient_HandleCsipInitCfm csisLockCcdHandle: 0x%x csisSizeCcdHandle 0x%x csisSirkCcdHandle 0x%x",
                       data->csisLockCcdHandle, data->csisSizeCcdHandle, data->csisSirkCcdHandle);
        }
    }
    else if(cfm->status != CSIP_STATUS_IN_PROGRESS)
    {
        /* Send both device add indication and init complete indication with status as failure */
        csipClient_DeviceAddedCfm(instance->cid, CSIP_CLIENT_STATUS_FAILED, TRUE);
        csipClient_SendInitCfm(CSIP_CLIENT_STATUS_FAILED);
        CsipClient_ResetCsipInstance(instance);
    }
}

static void csipClient_HandleCsipDestroyCfm(CsipDestroyCfm *cfm)
{
    csip_client_device_instance_t *instance;
    gatt_cid_t cid;

    if (cfm->status == CSIP_STATUS_IN_PROGRESS)
    {
        return;
    }

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_profile_handle, (unsigned)cfm->prflHndl);
    PanicNull(instance);

    DEBUG_LOG("csipClient_HandleCsipDestroyCfm prflHndl 0x%04x status %d", cfm->prflHndl, cfm->status);

    cid = instance->cid;

    CsipClient_ResetCsipInstance(instance);

    /* Send disconnect indication if all instances are removed */
    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_valid_invalid_cid, (unsigned)INVALID_CID);
    if (instance == NULL)
    {
        csipClient_DeviceRemovedCfm(cid, cfm->status == CSIP_STATUS_SUCCESS ? CSIP_CLIENT_STATUS_SUCCESS:
                                                                              CSIP_CLIENT_STATUS_FAILED, FALSE);
        csipClient_DisconnectCfm(CSIP_CLIENT_STATUS_SUCCESS);
    }
    else
    {
        csipClient_DeviceRemovedCfm(cid, cfm->status == CSIP_STATUS_SUCCESS ? CSIP_CLIENT_STATUS_SUCCESS:
                                                                              CSIP_CLIENT_STATUS_FAILED, TRUE);
    }

    pfree(cfm->handles);
}

static void csipClient_HandleCsipReadCsInfoCfm(CsipReadCsInfoCfm *cfm)
{
    csip_client_device_instance_t *instance;

    DEBUG_LOG("csipClient_HandleCsipReadCsInfoCfm type %d size_value %d status %d",
               cfm->csInfoType, cfm->sizeValue, cfm->status);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_profile_handle, (unsigned)cfm->prflHndl);
    PanicNull(instance);

    if (cfm->status == CSIP_STATUS_SUCCESS)
    {
        switch (cfm->csInfoType)
        {
            case CSIP_SIRK:
            {
                instance->sirk_encrypted = (cfm->value[0] == CSIP_CLIENT_ENCRYPTED_SIRK_TYPE);
                memcpy(&instance->sirk[0], &cfm->value[1], sizeof(instance->sirk));

                if (instance->sirk_encrypted)
                {
                    CsipDecryptSirk(instance->csip_profile_handle, &instance->sirk[0]);
                    return;
                }

                if (instance->state == csip_client_state_discovery)
                {
                    /* Read the size */
                    CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_SIZE);
                }
            }
            break;

            case CSIP_SIZE:
            {
                instance->set_size = cfm->value[0];

                if (instance->state == csip_client_state_discovery)
                {
                    /* Read the rank */
                    CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_RANK);
                }
            }
            break;

            case CSIP_RANK:
            {
                instance->rank = cfm->value[0];

                if (instance->state == csip_client_state_discovery)
                {
                    /* Read the lock status */
                    CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_LOCK);
                }
            }
            break;

            case CSIP_LOCK:
            {
                instance->lock_status = cfm->value[0];

                if (instance->state == csip_client_state_discovery)
                {
                    instance->state = csip_client_state_connected;
                    CsipClient_PrintCsipInfo(instance->csip_profile_handle);

                    /* Send connect indication if all instances got connected */
                    if (instance->set_size == CsipClient_GetConnectedDeviceCount())
                    {
                        /* Send indication that a device got added successfully */
                        csipClient_DeviceAddedCfm(instance->cid, CSIP_CLIENT_STATUS_SUCCESS, FALSE);
                        /* Send indication that all instance are initialized successfully */
                        csipClient_SendInitCfm(CSIP_CLIENT_STATUS_SUCCESS);
                    }
                    else
                    {
                        csipClient_DeviceAddedCfm(instance->cid, CSIP_CLIENT_STATUS_SUCCESS, TRUE);
                    }
                }

                /* Enable notifications */
                CsipCSRegisterForNotificationReq(instance->csip_profile_handle, CSIP_LOCK, TRUE);
                CsipCSRegisterForNotificationReq(instance->csip_profile_handle, CSIP_RANK, TRUE);
                CsipCSRegisterForNotificationReq(instance->csip_profile_handle, CSIP_SIZE, TRUE);
            }
            break;

            default:
            break;
        }
    }
    else
    {
        csipClient_DeviceAddedCfm(instance->cid, CSIP_CLIENT_STATUS_FAILED, TRUE);
        csipClient_SendInitCfm(CSIP_CLIENT_STATUS_FAILED);
    }

    pfree(cfm->value);
}

static void csipClient_HandleCsipSetLockCfm(CsipSetLockCfm *cfm)
{
    csip_client_device_instance_t *instance = NULL;
    csip_client_device_instance_t *dev_instance;
    csip_client_task_data_t *client_context = CsipClient_GetContext();

    DEBUG_LOG("csipClient_HandleCsipSetLockCfm prflHndl:0x%x status :%d", cfm->prflHndl, cfm->status);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_profile_handle, (unsigned)cfm->prflHndl);
    PanicNull(instance);

    if (cfm->status == CSIP_STATUS_SUCCESS)
    {

         if (client_context->pending_request == csip_client_set_lock_request)
         {
             /* Find the next device to perform the lock (in ascending order) */
             instance++;
             for (dev_instance = instance; dev_instance < &client_context->device_instance[MAX_CSIP_DEVICES_SUPPORTED]; dev_instance++)
             {
                 if (dev_instance->state == csip_client_state_connected)
                 {
                     CsipClient_SetLockForMember(dev_instance->cid, TRUE);
                     break;
                 }
             }

             if (dev_instance >= &client_context->device_instance[MAX_CSIP_DEVICES_SUPPORTED])
             {
                /* Stop the operation as all devices locked */
                client_context->pending_request = csip_client_no_request_in_progress;
             }
         }
         else if(client_context->pending_request == csip_client_release_lock_request)
         {
             /* Find the next device to perform the release (in descending order) */
             instance--;
             for (dev_instance = instance; dev_instance >= &client_context->device_instance[0]; dev_instance--)
             {
                 if (dev_instance->state == csip_client_state_connected)
                 {
                     CsipClient_SetLockForMember(dev_instance->cid, FALSE);
                     break;
                 }
             }

             if (dev_instance < &client_context->device_instance[0])
             {
                 /* Stop the operation as all devices released */
                client_context->pending_request = csip_client_no_request_in_progress;
             }
         }
    }
    else
    {
        /* Lock process failed. Unlock the previously locked devices */
        if (client_context->pending_request == csip_client_set_lock_request)
        {
            /* Set the lock in the next device if there is any */
            instance--;
            for (dev_instance = instance; dev_instance >= &client_context->device_instance[0]; dev_instance--)
            {
                if (dev_instance->state == csip_client_state_connected)
                {
                    /* Unlock the previous */
                    CsipClient_SetLockForMember(dev_instance->cid, FALSE);
                    break;
                }
            }

            /* Change to release request as we are now releasing the previously locked devices */
            client_context->pending_request = csip_client_release_lock_request;

            if (dev_instance < &client_context->device_instance[0])
            {
               /* Stop the operation as all devices released */
               client_context->pending_request = csip_client_no_request_in_progress;
            }
        }
        else
        {
            /* Todo : Check how to handle if unlock is failed.  */
        }
    }
}

static void csipClient_HandleCsipSetNotifyCfm(CsipCsSetNtfCfm *cfm)
{
    DEBUG_LOG("csipClient_HandleCsipSetNotifyCfm status %d", cfm->status);
}

static void csipClient_HandleCsipDecryptCfm(CsipSirkDecryptCfm *cfm)
{
    csip_client_device_instance_t *instance;

    DEBUG_LOG("csipClient_HandleCsipSetNotifyCfm status %d", cfm->status);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_profile_handle, (unsigned)cfm->prflHndl);
    PanicNull(instance);

    if (cfm->status == CSIP_STATUS_SUCCESS)
    {
        memcpy(&instance->sirk[0], &cfm->sirkValue[0], sizeof(instance->sirk));

        if (instance->state == csip_client_state_discovery)
        {
            /* Read the size */
            CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_SIZE);
        }
    }
    else
    {
        if (instance->state == csip_client_state_discovery)
        {
            /* Read the size */
            csipClient_DeviceAddedCfm(instance->cid, CSIP_CLIENT_STATUS_FAILED, TRUE);
        }
    }
}

static void csipClient_HandleCsipMessage(Message message)
{
    CsrBtGattPrim *csip_id = (CsrBtGattPrim *)message;

    switch(*csip_id)
    {
        case CSIP_INIT_CFM:
            csipClient_HandleCsipInitCfm((CsipInitCfm *) message);
        break;

        case CSIP_DESTROY_CFM:
            csipClient_HandleCsipDestroyCfm((CsipDestroyCfm *) message);
        break;

        case CSIP_READ_CS_INFO_CFM:
            csipClient_HandleCsipReadCsInfoCfm((CsipReadCsInfoCfm *) message);
        break;

        case CSIP_SET_LOCK_CFM:
            csipClient_HandleCsipSetLockCfm((CsipSetLockCfm *) message);
        break;

        case CSIP_CS_SET_NTF_CFM:
            csipClient_HandleCsipSetNotifyCfm((CsipCsSetNtfCfm *) message);
        break;

        case CSIP_SIRK_DECRYPT_CFM:
            csipClient_HandleCsipDecryptCfm((CsipSirkDecryptCfm *) message);
        break;

        default:
            DEBUG_LOG("csipClient_HandleCsipMessage Unhandled Message Id : 0x%x", csip_id);
        break;
    }
}

/*! \brief Handler that receives notification from CSIP Profile library */
static void CsipClient_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch (id)
    {
        case CSIP_PROFILE_PRIM:
            csipClient_HandleCsipMessage(message);
        break;

        default:
            DEBUG_LOG("CsipClient_HandleMessage Unhandled Message Id : 0x%x", id);
        break;
    }
}

void CsipClient_Init(csip_client_callback_handler_t handler)
{
    DEBUG_LOG("CsipClient_Init");

    memset(&csip_client_taskdata, 0, sizeof(csip_client_task_data_t));
    csip_client_taskdata.task_data.handler = CsipClient_HandleMessage;
    csip_client_taskdata.callback_handler = handler;
}

bool CsipClient_CreateInstance(gatt_cid_t cid)
{
    bool status = FALSE;
    CsipInitData csip_init_data;
    csip_client_device_instance_t *instance;

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_state, csip_client_state_idle);

    DEBUG_LOG("CsipClient_CreateInstance instance 0x%x", instance);

    if (instance != NULL)
    {
        csip_init_data.cid = cid;
        instance->cid = cid;
        instance->state = csip_client_state_discovery;

        CsipInitReq(TrapToOxygenTask((Task) &csip_client_taskdata.task_data),
                    &csip_init_data, NULL);

        status = TRUE;
    }

    return status;
}

bool CsipClient_DestroyInstance(gatt_cid_t cid)
{
    bool status = FALSE;
    csip_client_device_instance_t *instance;

    DEBUG_LOG("CsipClient_DestroyInstance cid 0x%x", cid);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_cid, (unsigned)cid);
    if (instance != NULL)
    {
        CsipDestroyReq(instance->csip_profile_handle);
        status = TRUE;
    }
    else if(cid == 0)
    {
        /* Todo : Group disconnect needs to be implemented */
    }

    return status;
}

bool CsipClient_SetLockForMember(gatt_cid_t cid, bool lock_enabled)
{
    bool status = FALSE;
    csip_client_device_instance_t *instance;

    /* Todo : Implement a message response for this operation */
    DEBUG_LOG("CsipClient_SetLockForMember cid 0x%x lock:%d", cid, lock_enabled);

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_cid, (unsigned)cid);
    if (instance != NULL)
    {
        CsipSetLockRequest(instance->csip_profile_handle, lock_enabled);
        status = TRUE;
    }

    return status;
}

static void CsipClient_SwapInstances(csip_client_device_instance_t* first, csip_client_device_instance_t* second)
{
    csip_client_device_instance_t temp;

    memcpy(&temp, first, sizeof(csip_client_device_instance_t));
    memcpy(first, second, sizeof(csip_client_device_instance_t));
    memcpy(second, &temp, sizeof(csip_client_device_instance_t));
}

/*! Sort device instances in ascending order */
static void CsipClient_SortDeviceInstances(csip_client_device_instance_t instances[], uint8 n)
{
    uint8 i, j;

    for (i = 0; i < n - 1; i++)
    {
        /* Last i elements are sorted now */
        for (j = 0; j < n - i - 1; j++)
        {
            if (instances[j].rank > instances[j + 1].rank)
            {
                CsipClient_SwapInstances(&instances[j], &instances[j + 1]);
            }
        }
    }
}

bool CsipClient_SetLockForAllMembers(bool lock_enabled)
{
    bool status = FALSE;
    uint8 i;
    csip_client_task_data_t *client_context = CsipClient_GetContext();

    DEBUG_LOG("CsipClient_SetLockForAllMembers lock:%d", lock_enabled);

    /* Todo : Implement a message response for this operation */
    /* First sort the device instances based on the rank */
    CsipClient_SortDeviceInstances(&client_context->device_instance[0], MAX_CSIP_DEVICES_SUPPORTED);

    for (i = 0; i < MAX_CSIP_DEVICES_SUPPORTED; i++)
    {
        if (client_context->device_instance[i].state == csip_client_state_connected)
        {
            CsipClient_SetLockForMember(client_context->device_instance[i].cid, lock_enabled);
            client_context->pending_request = lock_enabled ? csip_client_set_lock_request :
                                                             csip_client_release_lock_request;
            status = TRUE;
            break;
        }
    }

    return status;
}

bool CsipClient_IsAdvertFromSetMember(uint8 *adv_data, uint16 adv_data_len)
{
    bool status = FALSE;
    uint8 rsi_data[6];
    csip_client_device_instance_t *instance = NULL;

    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_valid_invalid_cid, (unsigned)INVALID_CID);

    if (instance != NULL)
    {
        if (CsipGetRsiFromAdvData(adv_data, adv_data_len, &rsi_data[0]))
        {
            if (CsipIsSetMember(&rsi_data[0], sizeof(rsi_data), &instance->sirk[0],
                                sizeof(instance->sirk)))
            {
                status = TRUE;
            }
        }
    }

    DEBUG_LOG("CsipClient_IsAdvertFromSetMember status %d", status);

    return status;
}

bool CsipClient_ReadSetSize(void)
{
    bool status = FALSE;
    csip_client_device_instance_t *instance = NULL;

    DEBUG_LOG("CsipClient_ReadSetSize");

    /* Todo : Implement a message response for this operation */
    instance = CsipClient_GetDeviceInstance(csip_client_compare_by_valid_invalid_cid, (unsigned)INVALID_CID);

    if (instance != NULL)
    {
        status = TRUE;
        CsipReadCSInfoRequest(instance->csip_profile_handle, CSIP_SIZE);
    }

    return status;
}
