/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_init_bt.c
\brief      Initialization module for non-synergy version
*/

#include "usb_dongle_init_bt.h"
#include "usb_dongle_init.h"
#include "bt_device_class.h"
#include "usb_dongle_config.h"

#include <connection_no_ble.h>
#include <unexpected_message.h>
#include <system_state.h>
#include <connection_message_dispatcher.h>
#include <app_task.h>
#include <app/bluestack/dm_prim.h>
#include <multidevice.h>

#include <panic.h>
#include <logging.h>

/* Needed only for message dispatcher registration */
#include <le_scan_manager.h>
#include <pairing.h>
#include <rssi_pairing.h>
#include <connection_manager.h>
#include <link_policy.h>
#include <authentication.h>
#include <le_advertising_manager.h>
#include <local_addr.h>
#include <bt_device.h>
#include <device_db_serialiser.h>

#define SELF_DVICE_KEY_SIZE             (0x08)

#define INVALID_LINK_KEY                (0xFFFF)
/*! Structure used while initialising */
initData usb_dongle_init;

/*! \brief Forward CL_INIT_CFM message to the init task handler. */
static void usbDongle_InitFwdClInitCfm(const CL_INIT_CFM_T * cfm)
{
    CL_INIT_CFM_T *copy = PanicUnlessNew(CL_INIT_CFM_T);
    *copy = *cfm;
    MessageSend(SystemState_GetTransitionTask(), CL_INIT_CFM, copy);
}

/*! \brief Handle Connection library confirmation message */
static void usbDongle_InitHandleClInitCfm(const CL_INIT_CFM_T *cfm)
{
    DEBUG_LOG_FN_ENTRY("usbDongle_InitHandleClInitCfm");

    if (cfm->status != success)
        Panic();

    /* Set the class of device to indicate this is a usb dongle */
    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | CAPTURING_MAJOR_SERV_CLASS |
                                    AV_MAJOR_DEVICE_CLASS |
                                    PORTABLE_AUDIO_MINOR_DEVICE_CLASS );

    /* Allow SDP without security, requires authorisation */
    ConnectionSmSetSecurityLevel(0, 1, ssp_secl4_l0, TRUE, TRUE, FALSE);

    /* Increase default SDP server MTU to reduce the chances
     * that we need to send partial responses to unsupporting sinks */
    ConnectionSetSdpServerMtu(64);
    /* Reset security mode config - always turn off debug keys on power on */
    ConnectionSmSecModeConfig(appGetAppTask(), cl_sm_wae_acl_owner_none, FALSE, TRUE);

    usbDongle_InitFwdClInitCfm(cfm);

}

/*! \brief Connection library Message Handler

    This function is the main message handler for the main application task, every
    message is handled in it's own seperate handler function.  The switch
    statement is broken into seperate blocks to reduce code size, if execution
    reaches the end of the function then it is assumed that the message is
    unhandled.
*/
static void appHandleClMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    /* Handle Connection Library messages that are not sent directly to
       the requestor */

    if (CL_MESSAGE_BASE <= id && id < CL_MESSAGE_TOP)
    {
        bool handled = FALSE;

        if (id == CL_INIT_CFM)
        {
            usbDongle_InitHandleClInitCfm((const CL_INIT_CFM_T *)message);
            return;
        }

        /* Pass connection library messages in turn to the modules that
           are interested in them.
         */
        handled |= Pairing_HandleConnectionLibraryMessages(id, message, handled);
        handled |= ConManagerHandleConnectionLibraryMessages(id, message, handled);
        handled |= appLinkPolicyHandleConnectionLibraryMessages(id, message, handled);
        handled |= appAuthHandleConnectionLibraryMessages(id, message, handled);

        handled |= LocalAddr_HandleConnectionLibraryMessages(id, message, handled);
        handled |= BtDevice_HandleConnectionLibraryMessages(id, message, handled);
        if (handled)
        {
            return;
        }
    }

    DEBUG_LOG_WARN("appHandleClMessage, unhandled 0x%x", id);
    UnexpectedMessage_HandleMessage(id);
}


/*! \brief Connection library initialisation */
bool AppConnectionInit(Task init_task)
{
    static const msg_filter filter = {msg_group_acl | msg_group_mode_change};

    UNUSED(init_task);

    ConnectionMessageDispatcher_Init();

    /* Initialise the Connection Manager */
#if defined(APP_SECURE_CONNECTIONS)
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, APP_CONFIG_MAX_PAIRED_DEVICES, CONNLIB_OPTIONS_SC_ENABLE);
#else
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, APP_CONFIG_MAX_PAIRED_DEVICES, CONNLIB_OPTIONS_NONE);
#endif
    ConnectionMessageDispatcher_RegisterInitClient(&usb_dongle_init.task);

    return TRUE;
}

/*! \brief Register connection message dispatcher  */
bool AppMessageDispatcherRegister(Task init_task)
{
    Task client = &usb_dongle_init.task;
    
    UNUSED(init_task);

    ConnectionMessageDispatcher_RegisterInquiryClient(client);
    ConnectionMessageDispatcher_RegisterCryptoClient(client);
    ConnectionMessageDispatcher_RegisterLeClient(client);
    ConnectionMessageDispatcher_RegisterTdlClient(client);
    ConnectionMessageDispatcher_RegisterL2capClient(client);
    ConnectionMessageDispatcher_RegisterLocalDeviceClient(client);
    ConnectionMessageDispatcher_RegisterPairingClient(client);
    ConnectionMessageDispatcher_RegisterLinkPolicyClient(client);
    ConnectionMessageDispatcher_RegisterTestClient(client);
    ConnectionMessageDispatcher_RegisterRemoteConnectionClient(client);
    ConnectionMessageDispatcher_RegisterRfcommClient(client);
    ConnectionMessageDispatcher_RegisterScoClient(client);
    ConnectionMessageDispatcher_RegisterSdpClient(client);

    return TRUE;
}

void UsbDongle_StartBtInit(void)
{
	usb_dongle_init.task.handler = appHandleClMessage;
}


bool UsbDongle_RegisterForBtMessages(Task init_task)
{
    UNUSED(init_task);

    return TRUE;
}

