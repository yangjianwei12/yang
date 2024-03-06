/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_init_bt.c
\brief      Initialization module for synergy version

*/

#include "usb_dongle_init_bt.h"
#include "usb_dongle_init.h"
#include "bt_device_class.h"
#include "bt_device.h"
#include "usb_dongle_config.h"
#include "device_db_serialiser.h"
#include "link_policy.h"
#include <connection_no_ble.h>
#include <unexpected_message.h>
#include <system_state.h>
#include <cm_lib.h>
#include <connection_message_dispatcher.h>
#include <app_task.h>
#include <app/bluestack/dm_prim.h>
#include <multidevice.h>

#include <panic.h>
#include <logging.h>

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) && defined(INCLUDE_SOURCE_APP_LE_AUDIO)
#define USB_DONGLE_SECURITY_CONFIG_DISABLE_CTKD     (0)
#else
#define USB_DONGLE_SECURITY_CONFIG_DISABLE_CTKD     (CM_SECURITY_CONFIG_OPTION_DISABLE_CTKD)
#endif

#define IS_ACL_DISCONNECT_FOR_BLE(flags) (DM_ACL_FLAG_ULP & flags)

#define ISOC_TYPE_UNICAST DM_ISOC_TYPE_UNICAST
#define ISOC_TYPE_BROADCAST DM_ISOC_TYPE_BROADCAST

/*!< Structure used while initialising */
initData    usb_dongle_init;

static void usbDongleHandleClMessage(Task task, MessageId id, Message message);

bool AppConnectionInit(Task init_task)
{
    /* Enable CTKD only for dual mode dongle. Otherwise disable it */
    CmSecurityConfigOptions setConfigFlag = USB_DONGLE_SECURITY_CONFIG_DISABLE_CTKD;

    DEBUG_LOG_FN_ENTRY("AppConnectionInit");

    UNUSED(init_task);

#if defined(INCLUDE_SOURCE_APP_LE_AUDIO)
    SynergyEnableLEATasks();
#endif

#if defined(BREDR_SECURE_CONNECTION_ALL_HANDSETS)
    setConfigFlag |= CM_SECURITY_CONFIG_OPTION_SECURE_CONNECTIONS;
#endif

    if (setConfigFlag)
    {
        /* Set security configuration, needs to be called before synergy initialization. */
        CmScSetSecurityConfigReq(setConfigFlag);
    }

    usb_dongle_init.task.handler = usbDongleHandleClMessage;
    SynergyInit(APP_CONFIG_MAX_PAIRED_DEVICES);

    CmSetEventMaskReqSend(&usb_dongle_init.task,
                              CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED |
                              CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE |
                              CSR_BT_CM_EVENT_MASK_SUBSCRIBE_LSTO_CHANGE,
                              CSR_BT_CM_EVENT_MASK_COND_ALL);
    return TRUE;
}

bool AppMessageDispatcherRegister(Task init_task)
{
    UNUSED(init_task);
    return TRUE;
}


/*!< Initialization function to start up BT*/
void UsbDongle_StartBtInit(void)
{
}

bool UsbDongle_RegisterForBtMessages(Task init_task)
{
    UNUSED(init_task);
    return TRUE;
}



static void usbDongleInitCmInitHandler(CsrBtCmBluecoreInitializedInd *ind)
{
    UNUSED(ind);

    DEBUG_LOG("usbDongleInitCmInitHandler");

    CmWriteCodReqSend(NULL,
                      (AUDIO_MAJOR_SERV_CLASS
                       | CAPTURING_MAJOR_SERV_CLASS /* for A2DP SRC */
#if defined(INCLUDE_SOURCE_APP_LE_AUDIO)
                       | LE_AUDIO_MAJOR_SERV_CLASS
#endif
                      ),
                      AV_MAJOR_DEVICE_CLASS,
                      PORTABLE_AUDIO_MINOR_DEVICE_CLASS);

    MessageSend(SystemState_GetTransitionTask(), INIT_CL_CFM, NULL);
}


static void usbDongleHandleClMessage(Task task, MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(task);

    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_BLUECORE_INITIALIZED_IND:
            usbDongleInitCmInitHandler((CsrBtCmBluecoreInitializedInd *) message);
            break;

        default:
            if(id == CM_PRIM)
            {
                appLinkPolicyHandleCMMessage(task, id, message);
                /* Freeing of CM message is handled in appLinkPolicyHandleCMMessage() */
                return;
            }
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}

