/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Initialisation module
*/

#include "charger_case_init_bt.h"
#include "charger_case_init.h"
#include "charger_case_config.h"
#include "bt_device_class.h"

#include <app_task.h>
#include <connection_no_ble.h>
#include <system_state.h>
#include <unexpected_message.h>
#include <connection_message_dispatcher.h>
#include <logging.h>
#include <panic.h>

#include <pairing.h>
#include <connection_manager.h>
#include <authentication.h>
#include <le_advertising_manager.h>
#include <local_addr.h>
#include <bt_device.h>

/*! Structure used while initialising */
initData app_init;


/*! \brief Forward CL_INIT_CFM message to the init task handler. */
static void chargerCase_InitFwdClInitCfm(const CL_INIT_CFM_T * cfm)
{
    CL_INIT_CFM_T *copy = PanicUnlessNew(CL_INIT_CFM_T);
    *copy = *cfm;
    MessageSend(SystemState_GetTransitionTask(), CL_INIT_CFM, copy);
}

/*! \brief Handle Connection library confirmation message */
static void chargerCase_InitHandleClInitCfm(const CL_INIT_CFM_T *cfm)
{
    DEBUG_LOG_FN_ENTRY("chargerCase_InitHandleClInitCfm");

    if (cfm->status != success)
        Panic();

    /* Set the class of device to indicate this is a charger_case */
    ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | RENDER_MAJOR_SERV_CLASS |
                                 AV_MAJOR_DEVICE_CLASS | PORTABLE_AUDIO_MINOR_DEVICE_CLASS);

    /* Allow SDP without security, requires authorisation */
    ConnectionSmSetSecurityLevel(0, 1, ssp_secl4_l0, TRUE, TRUE, FALSE);

    /* Reset security mode config - always turn off debug keys on power on */
    ConnectionSmSecModeConfig(appGetAppTask(), cl_sm_wae_acl_owner_none, FALSE, TRUE);

    chargerCase_InitFwdClInitCfm(cfm);

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
            chargerCase_InitHandleClInitCfm((const CL_INIT_CFM_T *)message);
            return;
        }

        /* Pass connection library messages in turn to the modules that
           are interested in them.
         */
        handled |= Pairing_HandleConnectionLibraryMessages(id, message, handled);
        handled |= ConManagerHandleConnectionLibraryMessages(id, message, handled);
        handled |= appAuthHandleConnectionLibraryMessages(id, message, handled);
        handled |= LeAdvertisingManager_HandleConnectionLibraryMessages(id, message, handled);
        handled |= LocalAddr_HandleConnectionLibraryMessages(id, message, handled);
        handled |= BtDevice_HandleConnectionLibraryMessages(id, message, handled);
        if (handled)
        {
            return;
        }
    }

    DEBUG_LOG_WARN("charger_caseHandleClMessage, unhandled MSG:ConnectionMessageId:0x%x", id);
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
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, appConfigChargerCaseMaxDevicesSupported(), CONNLIB_OPTIONS_SC_ENABLE);
#else
    ConnectionInitEx3(ConnectionMessageDispatcher_GetHandler(), &filter, appConfigChargerCaseMaxDevicesSupported(), CONNLIB_OPTIONS_NONE);
#endif
    ConnectionMessageDispatcher_RegisterInitClient(&app_init.task);

    return TRUE;
}


void ChargerCase_StartBtInit(void)
{
	app_init.task.handler = appHandleClMessage;
}


bool ChargerCase_RegisterForBtMessages(Task init_task)
{
    UNUSED(init_task);

    return TRUE;
}

/*! \brief Register connection message dispatcher  */
bool AppMessageDispatcherRegister(Task init_task)
{
    Task client = &app_init.task;

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

