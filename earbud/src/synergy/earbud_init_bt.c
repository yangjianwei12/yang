/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   C:\work\src\csa\vmbi_critical\earbud\src\earbud_init_bt.h
\brief      Short description.

Synergy version

*/

#include "earbud_init_bt.h"

#include "earbud_init.h"
#include "bt_device_class.h"
#include "bt_device.h"
#include "earbud_config.h"
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

static void appInitBt_MessageHandler(Task task, MessageId id, Message message);

static TaskData app_init_task = { appInitBt_MessageHandler };
extern void CsrSchedMessagePut(uint16, uint16, void *);
/*! \brief This function looks for local device address in CSR_BT_CM_ACL_DISCONNECT_IND
    and triggers ACL disconnection indication peer device address.

    Rarely, if dynamic handover fails, the stack can send a CL_DM_ACL_CLOSED_IND
    to the application with the local device's BR/EDR BD_ADDR, instead of the
    peer earbud's BR/EDR BD_ADDR. This occurs due to the handling of address
    swapping during handover.
    To work-around this issue, this function posts a dummy DM_ACL_CLOSED_IND
    for peer device address to CM, which in turn informs all subscribers of
    disconnection from peer device. This means the disconnection is
    handled correctly in the application.

    \param[in] pointer to CsrBtCmAclDisconnectInd message
*/
static void appInitBt_CmAclDisconnectIndHandler(CsrBtCmAclDisconnectInd *ind)
{
    bdaddr my_addr = { 0 };

    if (appDeviceGetMyBdAddr(&my_addr))
    {
        bdaddr bd_addr = { 0 };

        BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);

    //if(ind->deviceAddr..type == TYPED_BDADDR_PUBLIC &&
        /* Check if the address received in CL_DM_ACL_CLOSED_IND_T message and
         * local device address is same
         */
        if (BdaddrIsSame(&my_addr, &bd_addr))
        {
            /* If the address received is same as local device address then post
             * a dummy ACL disconnection ind for peer device to CM */
            if (appDeviceGetPeerBdAddr(&bd_addr))
            {
                DM_ACL_CLOSED_IND_T *dmInd = CsrPmemZalloc(sizeof(*dmInd));

                dmInd->type = DM_ACL_CLOSED_IND;
                dmInd->flags = 0;
                dmInd->phandle = CSR_BT_CM_IFACEQUEUE;
                dmInd->reason = 0;
                dmInd->addrt.type = CSR_BT_ADDR_PUBLIC;
                BdaddrConvertVmToBluestack(&dmInd->addrt.addr, &bd_addr);

                CsrSchedMessagePut(PHANDLE_TO_QUEUEID(CSR_BT_CM_IFACEQUEUE), DM_PRIM, dmInd);

                DEBUG_LOG("appInitBt_CmAclDisconnectIndHandler, Fake DM_ACL_CLOSED_IND posted with addr %04x,%02x,%06lx",
                          bd_addr.nap,
                          bd_addr.uap,
                          bd_addr.lap);
            }
        }
    }
}

static void appInitBt_CmInitHandler(CsrBtCmBluecoreInitializedInd *ind)
{
    UNUSED(ind);

    DEBUG_LOG("appInitBt_CmInitHandler");

    CmWriteCodReqSend(NULL,
                      (AUDIO_MAJOR_SERV_CLASS | RENDER_MAJOR_SERV_CLASS
#ifndef INCLUDE_MIRRORING
                       | CAPTURING_MAJOR_SERV_CLASS /* for A2DP SRC */
#endif
#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
                       | LE_AUDIO_MAJOR_SERV_CLASS
#endif
                      ),
                      AV_MAJOR_DEVICE_CLASS,
                      HEADSET_MINOR_DEVICE_CLASS);

    MessageSend(SystemState_GetTransitionTask(), INIT_CL_CFM, NULL);
}

static void appInitBt_CmPrimHandler(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_ACL_DISCONNECT_IND:
            appInitBt_CmAclDisconnectIndHandler((CsrBtCmAclDisconnectInd *) message);
            break;

        case CSR_BT_CM_BLUECORE_INITIALIZED_IND:
            appInitBt_CmInitHandler((CsrBtCmBluecoreInitializedInd *) message);
            break;

        default:
            DEBUG_LOG("appInitBt_CmPrimHandler, unexpected CM prim 0x%04x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}

static void appInitBt_MessageHandler(Task task, MessageId id, Message message)
{
    switch (id)
    {
        case CM_PRIM:
            appInitBt_CmPrimHandler(message);
            break;

        default:
            DEBUG_LOG("appInitBt_MessageHandler, unexpected message 0x%04x", id);
            break;
    }
    UNUSED(task);
}

bool AppInitBt_ConnectionInit(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("AppInitBt_ConnectionInit");

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
    SynergyEnableLEATasks();
#endif
#ifdef INCLUDE_HIDD_PROFILE
    SynergyEnableHIDDTask();
#endif

/* Set security configuration, needs to be called before synergy initialization. */
#if defined(BREDR_SECURE_CONNECTION_ALL_HANDSETS)
    CmScSetSecurityConfigReq(CM_SECURITY_CONFIG_OPTION_SECURE_CONNECTIONS);
#else
    CmScSetSecurityConfigReq(CM_SECURITY_CONFIG_OPTION_NONE);
#endif

    SynergyInit(appConfigMaxDevicesSupported());

    CmSetEventMaskReqSend(&app_init_task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_BLUECORE_INITIALIZED,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);

    return TRUE;
}

#ifdef USE_BDADDR_FOR_LEFT_RIGHT
bool AppInitBt_ConfigInit(Task init_task)
{
    /* Get local device address */
    ConnectionReadLocalAddr(init_task);

    return TRUE;
}

bool AppInitBt_HandleReadLocalBdAddrCfm(Message message)
{
    bool result = FALSE;
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    if (*prim == CSR_BT_CM_READ_LOCAL_BD_ADDR_CFM)
    {
        CsrBtCmReadLocalBdAddrCfm *cfm = (CsrBtCmReadLocalBdAddrCfm *) prim;

        InitGetTaskData()->appInitIsLeft = cfm->deviceAddr.lap & 0x01;

        DEBUG_LOG("appInit, bdaddr %04x:%02x:%06x",
                   cfm->deviceAddr.nap,
                   cfm->deviceAddr.uap,
                   cfm->deviceAddr.lap);
        DEBUG_LOG("appInit, left %d, right %d",
                   appConfigIsLeft(),
                   appConfigIsRight());

        Multidevice_SetType(multidevice_type_pair);
        Multidevice_SetSide(appConfigIsLeft() ? multidevice_side_left : multidevice_side_right);

        result = TRUE;
    }
    else
    {
        DEBUG_LOG("appInitHandleCmReadLocalBdAddrCfm, unexpected CM prim 0x%04x", *prim);
    }

    CmFreeUpstreamMessageContents((void *) message);

    return result;
}
#endif

bool AppInitBt_RegisterForBtMessages(Task init_task)
{
    UNUSED(init_task);
#ifdef INCLUDE_MIRRORING
    CmSetEventMaskReqSend(&app_init_task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ACL_CONNECTION,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
#endif
    return TRUE;
}
