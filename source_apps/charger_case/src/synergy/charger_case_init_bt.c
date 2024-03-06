/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   charger_case\src\synergy\charger_case_init_bt.h
\brief      Short description.

Synergy version of the charger case bluetooth initialization

*/

#include "charger_case_init_bt.h"
#include "charger_case_init.h"
#include "bt_device_class.h"
#include "bt_device.h"
#include "charger_case_config.h"
#include <connection_no_ble.h>
#include <unexpected_message.h>
#include <system_state.h>
#include <connection_message_dispatcher.h>
#include <app_task.h>
#include <app/bluestack/dm_prim.h>
#include <multidevice.h>

#include <panic.h>
#include <logging.h>


#define IS_ACL_DISCONNECT_FOR_BLE(flags) (DM_ACL_FLAG_ULP & flags)

#define ISOC_TYPE_UNICAST DM_ISOC_TYPE_UNICAST
#define ISOC_TYPE_BROADCAST DM_ISOC_TYPE_BROADCAST

/*!< Structure used while initialising */
initData    app_init;

static void chargerCaseHandleClMessage(Task task, MessageId id, Message message);

bool AppConnectionInit(Task init_task)
{
    SynergyInit(appConfigChargerCaseMaxDevicesSupported());
    return TRUE;
}


void ChargerCase_StartBtInit(void)
{
}

bool ChargerCase_RegisterForBtMessages(Task init_task)
{
    UNUSED(init_task);
    return TRUE;
}

bool AppMessageDispatcherRegister(Task init_task)
{
    UNUSED(init_task);
    return TRUE;
}

static void chargerCaseInitCmInitHandler(CsrBtCmBluecoreInitializedInd *ind)
{
    UNUSED(ind);

    DEBUG_LOG("chargerCaseInitCmInitHandler");

    CmWriteCodReqSend(init_task,
                          (AUDIO_MAJOR_SERV_CLASS | RENDER_MAJOR_SERV_CLASS |
                           AV_MAJOR_DEVICE_CLASS | PORTABLE_AUDIO_MINOR_DEVICE_CLASS
#ifndef INCLUDE_MIRRORING
                           | CAPTURING_MAJOR_SERV_CLASS /* for A2DP SRC */
#endif
#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
                           | LE_AUDIO_MAJOR_SERV_CLASS
#endif
                          ),
                          AV_MAJOR_DEVICE_CLASS,
                          UNCATEGORIZED_MINOR_DEVICE_CLASS);

    MessageSend(SystemState_GetTransitionTask(), INIT_CL_CFM, NULL);
}


static void chargerCaseHandleClMessage(Task task, MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(task);

    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_BLUECORE_INITIALIZED_IND:
            chargerCaseInitCmInitHandler((CsrBtCmBluecoreInitializedInd *) message);
            break;

        default:
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}

