/*!
    \copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    volume_profile
    \brief
*/

#include "volume_renderer_role.h"
#include "volume_renderer_role_advertising.h"

#include <gatt_connect.h>
#include <gatt_handler_db_if.h>
#include <gatt_vcs_server.h>
#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#include "gatt.h"
#endif

#define VOLUME_RENDERER_ROLE_LOG     DEBUG_LOG

ServiceHandle vcs_handle = 0;
const volume_renderer_callback_interface_t * volume_renderer_registered_callbacks = NULL;

static void volumeRenderer_VcsMessageHandler(Task task, MessageId id, Message message);
static const TaskData vcs_task = { .handler = volumeRenderer_VcsMessageHandler };

static void volumeRenderer_OnGattConnect(gatt_cid_t cid);
static void volumeRenderer_OnGattDisconnect(gatt_cid_t cid);
static void volumeRenderer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted);

static const gatt_connect_observer_callback_t volume_renderer_connect_callbacks =
{
    .OnConnection = volumeRenderer_OnGattConnect,
    .OnDisconnection = volumeRenderer_OnGattDisconnect,
    .OnEncryptionChanged = volumeRenderer_OnEncryptionChanged
};

static void volumeRenderer_OnGattConnect(gatt_cid_t cid)
{
    UNUSED(cid);
}

static void volumeRenderer_OnGattDisconnect(gatt_cid_t cid)
{
    GattVcsServerConfig * vcs_config = GattVcsServerRemoveConfig(vcs_handle, cid);

    if (vcs_config)
    {
        if (volume_renderer_registered_callbacks)
        {
            volume_renderer_registered_callbacks->VolumeRenderer_StoreClientConfig(cid, (void *)vcs_config, sizeof(GattVcsServerConfig));
        }

        free(vcs_config);
    }
}

static void volumeRenderer_OnEncryptionChanged(gatt_cid_t cid, bool encrypted)
{
    if (encrypted)
    {
        GattVcsServerConfig * vcs_config = NULL;

        if (volume_renderer_registered_callbacks)
        {
            vcs_config = (GattVcsServerConfig *)volume_renderer_registered_callbacks->VolumeRenderer_RetrieveClientConfig(cid);
        }

        if (GattVcsServerAddConfig(vcs_handle, cid, vcs_config) != gatt_status_success)
        {
            Panic();
        }
    }
}

static void volumeRenderer_VerifyCallbacksArePresent(const volume_renderer_callback_interface_t * callbacks_to_verify)
{
    PanicNull((void *)callbacks_to_verify);
    PanicNull((void *)callbacks_to_verify->VolumeRenderer_VolumeChanged);
    PanicNull((void *)callbacks_to_verify->VolumeRenderer_RetrieveClientConfig);
    PanicNull((void *)callbacks_to_verify->VolumeRenderer_StoreClientConfig);
}

static void volumeRenderer_HandleVolumeStateInd(const GattVcsServerVolumeStateInd * ind)
{
    if (volume_renderer_registered_callbacks)
    {
        volume_renderer_volume_changed_t vol_change = {0};

        VOLUME_RENDERER_ROLE_LOG("volumeRenderer_HandleVolumeStateInd volume:%d mute:%d cid:0x%x",
                                ind->volumeSetting,
                                ind->mute,
                                ind->cid
                                );

        vol_change.volume_setting = ind->volumeSetting;
        vol_change.mute = ind->mute;
        vol_change.cid = ind->cid;

        volume_renderer_registered_callbacks->VolumeRenderer_VolumeChanged(&vol_change);
    }
}

static void volumeRenderer_VcsMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

#ifdef USE_SYNERGY
    id = *(GattVcsMessageId *) message;
#endif

    switch(id)
    {
        case GATT_VCS_SERVER_VOLUME_STATE_IND:
            volumeRenderer_HandleVolumeStateInd((GattVcsServerVolumeStateInd *)message);
            break;
        case GATT_VCS_SERVER_VOLUME_FLAG_IND:
            break;

#ifdef USE_SYNERGY
        case GATT_VCS_SERVER_CONFIG_CHANGE_IND:
        {
            GattVcsServerConfigChangeInd *ind = (GattVcsServerConfigChangeInd *)message;
            GattVcsServerConfig * service_config = NULL;
            GattVcsServerConfig * stored_config = NULL;

            if (ind->configChangeComplete && volume_renderer_registered_callbacks != NULL)
            {
                VOLUME_RENDERER_ROLE_LOG("volumeRenderer_VcsMessageHandler GATT_VCS_SERVER_CONFIG_CHANGE_IND");
                service_config = GattVcsServerGetConfig(ind->vcsServiceHandle, ind->cid);
                stored_config = (GattVcsServerConfig *)volume_renderer_registered_callbacks->VolumeRenderer_RetrieveClientConfig(ind->cid);
                if (service_config != NULL)
                {
                    if (stored_config == NULL || service_config->volumeFlagClientCfg != stored_config->volumeFlagClientCfg ||
                        service_config->volumeStateClientCfg != stored_config->volumeStateClientCfg)
                    {
                        VOLUME_RENDERER_ROLE_LOG("volumeRenderer_VcsMessageHandler storing GATT_VCS_SERVER_CONFIG_CHANGE_IND");
                        volume_renderer_registered_callbacks->VolumeRenderer_StoreClientConfig(ind->cid, (void *)service_config, sizeof(GattVcsServerConfig));
                    }
                    pfree(service_config);
                }
            }
            break;
        }
#endif

        default:
            VOLUME_RENDERER_ROLE_LOG("volumeRenderer_VcsMessageHandler unhandled messsage id 0x%x", id);
            break;
    }
}


void VolumeRenderer_Init(const volume_renderer_init_t * init_params, const volume_renderer_callback_interface_t * callbacks_to_register)
{
    GattVcsInitData vcs_init = {0};

    PanicNull((void *)init_params);

    VOLUME_RENDERER_ROLE_LOG("VolumeRenderer_Init volume:%d mute:%d step_size:%d",
                                init_params->volume_setting,
                                init_params->mute,
                                init_params->step_size
                                );

    volumeRenderer_VerifyCallbacksArePresent(callbacks_to_register);

    vcs_init.volumeSetting = init_params->volume_setting;
    vcs_init.mute = init_params->mute;
    vcs_init.stepSize = init_params->step_size;

#ifdef USE_SYNERGY
    vcs_handle = GattVcsServerInit(TrapToOxygenTask((Task)&vcs_task),
                                     HANDLE_VOLUME_CONTROL_SERVICE,
                                     HANDLE_VOLUME_CONTROL_SERVICE_END,
                                     &vcs_init);
#else
    vcs_handle = GattVcsServerInit((Task)&vcs_task,
                                   HANDLE_VOLUME_CONTROL_SERVICE,
                                   HANDLE_VOLUME_CONTROL_SERVICE_END,
                                   vcs_init);
#endif

    GattConnect_RegisterObserver(&volume_renderer_connect_callbacks);
    VolumeRenderer_SetupLeAdvertisingData();

    volume_renderer_registered_callbacks = callbacks_to_register;
}

uint8 VolumeRenderer_GetVolume(void)
{
    return GattVcsServerGetVolume(vcs_handle);
}

bool VolumeRenderer_SetVolume(uint8 volume)
{
    return GattVcsServerSetVolume(vcs_handle, volume);
}

uint8 VolumeRenderer_GetMute(void)
{
    return GattVcsServerGetMute(vcs_handle);
}

bool VolumeRenderer_SetMute(uint8 mute)
{
    return GattVcsServerSetMute(vcs_handle, mute);
}

#ifdef HOSTED_TEST_ENVIRONMENT

const TaskData * VolumeRenderer_GetVcsMessageHandler(void)
{
    return &vcs_task;
}

#else
void VolumeRenderer_SimulateVcsClientVolumeChange(uint8 volume, uint8 mute)
{
    MESSAGE_MAKE(ind, GattVcsServerVolumeStateInd);
    ind->vcsServiceHandle = vcs_handle;
    ind->volumeSetting = volume;
    ind->mute = mute;
    ind->changeCounter = 127;
    MessageSend((Task)&vcs_task, GATT_VCS_SERVER_VOLUME_STATE_IND, ind);
}

#endif
