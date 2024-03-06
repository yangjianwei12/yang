/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       headset_init_bt.c
\brief      Initialise Headset Bluetooth functionality, Synergy version
*/
#include "headset_init_bt.h"

#include "headset_init.h"
#include "bt_device_class.h"
#include "bt_device.h"
#include "headset_config.h"
#include "device_db_serialiser.h"
#include "device_properties.h"
#include "system_reboot.h"
#include <connection_no_ble.h>
#include <unexpected_message.h>
#include <system_state.h>
#include <connection_message_dispatcher.h>
#include <app_task.h>
#include <dm_prim.h>
#include <multidevice.h>

#include <panic.h>
#include <logging.h>

#define SELF_DVICE_KEY_SIZE             (0x08)

#define INVALID_LINK_KEY                (0xFFFF)

#define IS_ACL_DISCONNECT_FOR_BLE(flags) (DM_ACL_FLAG_ULP & flags)

#define ISOC_TYPE_UNICAST DM_ISOC_TYPE_UNICAST
#define ISOC_TYPE_BROADCAST DM_ISOC_TYPE_BROADCAST

/*!< Structure used while initialising */
initData    app_init;

static void appInitBt_CmPrimHandler(Task task, MessageId id, Message message);

bool AppInitBt_ConnectionInit(Task init_task)
{
    UNUSED(init_task);

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    SynergyEnableLEATasks();
#endif

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
    SynergyEnableLEABroadcastSourceTask();
#endif

#ifdef INCLUDE_HIDD_PROFILE
    SynergyEnableHIDDTask();
#endif
    app_init.task.handler = appInitBt_CmPrimHandler;

    /* Set security configuration, needs to be called before synergy initialization. */
#if defined(BREDR_SECURE_CONNECTION_ALL_HANDSETS)
    CmScSetSecurityConfigReq(CM_SECURITY_CONFIG_OPTION_SECURE_CONNECTIONS);
#else
    CmScSetSecurityConfigReq(CM_SECURITY_CONFIG_OPTION_NONE);
#endif

    SynergyInit(appConfigMaxDevicesSupported());

    CmSetEventMaskReqSend(&app_init.task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
    return TRUE;
}

/*! \brief Handle Connection library confirmation message to add SELF BT device */
static void appInitBt_HandleClSmAddAuthDeviceCfm(const CsrBtCmDatabaseCfm *cfm)
{
    DEBUG_LOG("appInitBt_HandleClSmAddAuthDeviceCfm opcode=%d result=%d",
              cfm->opcode,
              cfm->resultCode);

    if(cfm->resultCode != CSR_BT_RESULT_CODE_SUCCESS)
        Panic();

    if (cfm->opcode == CSR_BT_CM_DB_OP_WRITE)
    {
        bdaddr bdaddr_self;
#ifdef INCLUDE_ACCESSORY
        uint32 supported_profiles = (DEVICE_PROFILE_AVRCP | DEVICE_PROFILE_A2DP | DEVICE_PROFILE_HFP | DEVICE_PROFILE_ACCESSORY);
#else
        uint32 supported_profiles = (DEVICE_PROFILE_AVRCP | DEVICE_PROFILE_A2DP | DEVICE_PROFILE_HFP);
#endif
        BdaddrConvertBluestackToVm(&bdaddr_self, &cfm->deviceAddr);

        device_t device = BtDevice_GetDeviceCreateIfNew(&bdaddr_self, DEVICE_TYPE_SELF);
        PanicNull(device);

        /* Set the protection for SELF device so just in case when trusted device list(appConfigMaxTrustedDevices)
         is exhasuted, connection library will not overwrite entry of this SELF device in the PDL */
        ConnectionAuthSetPriorityDevice(&bdaddr_self, TRUE);

        /* Setting profiles supported by SELF. */
        BtDevice_AddSupportedProfilesToDevice(device, supported_profiles);

        /* Save the self BT device in PS store so that we don't lose any data in case of unexpected power cycle */
        DeviceDbSerialiser_Serialise();
        BtDevice_PrintAllDevices();
    }
}

static void appInitBt_CmInitHandler(CsrBtCmBluecoreInitializedInd *ind)
{
    UNUSED(ind);

    DEBUG_LOG("appInitBt_CmInitHandler");

    CmWriteCodReqSend(NULL,
                      (AUDIO_MAJOR_SERV_CLASS | RENDER_MAJOR_SERV_CLASS
                       | CAPTURING_MAJOR_SERV_CLASS /* for A2DP SRC */
#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
                       | LE_AUDIO_MAJOR_SERV_CLASS
#endif
                      ),
                      AV_MAJOR_DEVICE_CLASS,
                      HEADSET_MINOR_DEVICE_CLASS);

    MessageSend(SystemState_GetTransitionTask(), INIT_CL_CFM, NULL);
}


static void appInitBt_CmPrimHandler(Task task, MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(task);
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_DATABASE_CFM:
            appInitBt_HandleClSmAddAuthDeviceCfm((const CsrBtCmDatabaseCfm *) message);
            /* Ignore */
            break;

        case CSR_BT_CM_BLUECORE_INITIALIZED_IND:
            appInitBt_CmInitHandler((CsrBtCmBluecoreInitializedInd *) message);
            break;
        
        default:
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}

/*! \brief Handle Connection library confirmation message to store SELF device in PDL */
bool AppInitBt_InitHandleClDmLocalBdAddrCfm(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;
    uint16 self_device_lk[SELF_DVICE_KEY_SIZE] = {INVALID_LINK_KEY};

    if (*prim != CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM)
        Panic();

    CsrBtCmReadLocalBdAddrCfm *cfm = (CsrBtCmReadLocalBdAddrCfm *) prim;
    CsrBtTdDbBredrKey *bredrKey = PanicUnlessMalloc(sizeof(*bredrKey));
    device_t device = BtDevice_GetSelfDevice();
    bdaddr local_bd_addr, self_device_bd_addr;

    if (device)
    {
        self_device_bd_addr = DeviceProperties_GetBdAddr(device);

        BdaddrConvertBluestackToVm(&local_bd_addr, &cfm->deviceAddr);
        if (!BdaddrIsSame(&local_bd_addr, &self_device_bd_addr))
        {
            DEBUG_LOG_ERROR("HeadsetInitBt: Local address in filesystem doesn't match SELF address in PS");
            DEBUG_LOG_ERROR("HeadsetInitBt: Clear the TDL and create a new SELF on next boot");
            /* Local address in filesystem doesn't match SELF address in PS
             * Clear the TDL and create a new SELF on next boot */

            /* Clear the Synergy trusted device database index key in PS without clearing other keys. */
            CsrBtTdDbDeleteAll(CSR_BT_TD_DB_FILTER_EXCLUDE_NONE);

            SystemReboot_Reboot();
        }
    }
    else
    {
        bredrKey->linkkeyType = DM_SM_LINK_KEY_UNAUTHENTICATED_P192;
        bredrKey->linkkeyLen = SELF_DVICE_KEY_SIZE;
        memcpy(&bredrKey->linkkey, self_device_lk, SELF_DVICE_KEY_SIZE * sizeof(self_device_lk[0]));
        bredrKey->authorised = FALSE;


        /* Store the link key in the PDL */
        CmWriteBredrKeysReqSend(&app_init.task,
                                CSR_BT_ADDR_PUBLIC,
                                &cfm->deviceAddr,
                                (CsrBtCmKey *) bredrKey);
    }

    CmFreeUpstreamMessageContents((void *) message);
    return TRUE;
}
