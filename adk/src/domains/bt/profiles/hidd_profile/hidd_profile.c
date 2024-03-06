/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       hidd_profile.c
    \ingroup    hidd_profile
    \brief      HID Device profile
*/

#include "hidd_profile.h"
#include "hidd_profile_private.h"

#ifdef USE_SYNERGY
#ifdef INCLUDE_HIDD_PROFILE
#include "hidd_lib.h"

#include <logging.h>
#include <panic.h>
#include <vmtypes.h>
#include <stdlib.h>

#include "task_list.h"
#include "local_addr.h"
#include "bdaddr.h"
#include "bt_device.h"
#include "profile_manager.h"
#include "device_properties.h"

/* Constants used to parse HIDD Profile messages */
#define LENGTH_OF_HDR_IN_HID_REPORT       (1)
#define INDEX_OF_HDR_IN_GET_REPORT        (0)
#define INDEX_OF_REPORT_ID_IN_HID_REPORT  (1)
#define LENGTH_OF_REPORT_ID_IN_HID_REPORT (1)
#define INDEX_OF_SIZE_IN_GET_REPORT       (2)
#define HID_GET_REPORT_SIZE_FIELD_MASK  (0x08)
#define HID_GET_REPORT_TYPE_FIELD_MASK  (0x03)

/* Timers */
#define HIDD_REACTIVATION_DELAY (200)   /* 200 ms delay in activation after a deactivation. This gives times for
                                         * SDP de-registration, if we try too early, activation will fail. */

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(hidd_profile_messages_t)
#ifndef HOSTED_TEST_ENVIRONMENT
/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(HIDD_PROFILE, HIDD_PROFILE_MESSAGE_END)
#endif

/* HIDD Profile Task */
hiddInstanceTaskData hiddInstance;
hiddTaskData hidd_profile_task_data;

static void hiddTaskHandler(Task task, MessageId id, Message message);
TaskData hiddTask = {hiddTaskHandler};

/*! Macro to make a message based on type. */
#define MAKE_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! HIDD Instance data access Macros */
#define HiddProfileGetTask() (&hiddTask)
#define HiddProfileGetState() (hiddInstance.state)
#define HiddProfileSetState(s) (hiddInstance.state = s)
#define HiddProfileSetClientTask(t) (hidd_profile_task_data.client_task = t)
#define HiddProfileGetClientTask() (hidd_profile_task_data.client_task)
#define HiddProfileSetConnectionId(id) (hiddInstance.connection_id = id)
#define HiddProfileGetConnectionId() (hiddInstance.connection_id)
#define HiddProfileSetBdAddr(addr) (hiddInstance.hidd_bd_addr = addr)
#define HiddProfileGetBdAddr() (hiddInstance.hidd_bd_addr)
#define HiddProfileGetConnectClientList() (&hidd_profile_task_data.connect_request_clients)
#define HiddProfileGetDisconnectClientList() (&hidd_profile_task_data.disconnect_request_clients)


/* Local Methods */
static void hiddProfile_ReactivateReq(void);
static void hiddProfile_ActivateReq(bdaddr *bd_addr, uint16 len, const uint8* data, uint16 int_flush_timeout);


/************************************************************************************************
 * Utilities to send HIDD Events to Clients
 ************************************************************************************************/
static void hiddProfile_SendActivateCfm(Task task, hidd_profile_status_t status)
{
    MAKE_MESSAGE(HIDD_PROFILE_ACTIVATE_CFM);
    message->status = status;
    MessageSend(task, HIDD_PROFILE_ACTIVATE_CFM, message);
}

static void hiddProfile_SendDeactivateCfm(Task task, hidd_profile_status_t status)
{
    MAKE_MESSAGE(HIDD_PROFILE_DEACTIVATE_CFM);
    message->status = status;
    MessageSend(task, HIDD_PROFILE_DEACTIVATE_CFM, message);
}

static void hiddProfile_SendConnectInd(Task task, hidd_profile_status_t status, bdaddr addr, uint32 connid)
{
    MAKE_MESSAGE(HIDD_PROFILE_CONNECT_IND);
    message->status = status;
    message->addr = addr;
    message->connid = connid;
    MessageSend(task, HIDD_PROFILE_CONNECT_IND, message);
}

static void hiddProfile_SendDisconnectInd(Task task, hidd_profile_status_t status, bdaddr addr)
{
    MAKE_MESSAGE(HIDD_PROFILE_DISCONNECT_IND);
    message->status = status;
    message->addr = addr;
    MessageSend(task, HIDD_PROFILE_DISCONNECT_IND, message);
}

static void hiddProfile_SendGetReport(Task task, hidd_report_type_t type, uint16 size, uint8 id)
{
    MAKE_MESSAGE(HIDD_PROFILE_GET_REPORT_IND);
    message->report_type = type;
    message->reportid = id;
    message->size = size;
    MessageSend(task, HIDD_PROFILE_GET_REPORT_IND, message);
}

static void hiddProfile_SendSetReport(Task task, hidd_report_type_t type, uint8 id, uint16 len, uint8* data)
{
    MAKE_MESSAGE(HIDD_PROFILE_SET_REPORT_IND);
    message->type = type;
    message->reportid = id;
    message->reportLen = len;
    message->data = malloc(len);
    memcpy(message->data, data, len);
    MessageSend(task, HIDD_PROFILE_SET_REPORT_IND, message);
}

static void hiddProfile_SendDataInd(Task task, hidd_report_type_t type, uint16 len, uint8* data)
{
    MAKE_MESSAGE(HIDD_PROFILE_DATA_IND);
    message->type = type;
    message->reportLen = len;
    message->data = malloc(len);
    memcpy(message->data, data, len);
    MessageSend(task, HIDD_PROFILE_DATA_IND, message);
}

static void hiddProfile_SendDataCfm(Task task, hidd_profile_status_t status)
{
    MAKE_MESSAGE(HIDD_PROFILE_DATA_CFM);
    message->status = status;
    MessageSend(task, HIDD_PROFILE_DATA_CFM, message);
}

/************************************************************************************************
 * HIDD Event handlers
 ************************************************************************************************/
static void hiddProfile_ActivateReqCfmHandler(uint16 status)
{
    if (status == CSR_BT_RESULT_CODE_HIDD_SUCCESS)
    {
        if (HiddProfileGetState() >= HIDD_STATE_INITIALIZED)
        {
            if (HiddProfileGetTaskData()->connect_requested == TRUE)
            {
                /* Outgoing connection was requested */
                HiddProfileGetTaskData()->connect_requested = FALSE;
                HiddProfileSetState(HIDD_STATE_CONNECTING);
            }
            else
            {
                /* Waiting for incoming connection */
                HiddProfileSetState(HIDD_STATE_CONNECT_ACCEPT);
            }
        }                
    }
    else
    {
        DEBUG_LOG("hiddProfile_ActivateReqCfmHandler: FAILED. Result Code (%d)", status);
        HiddProfileSetState(HIDD_STATE_INITIALIZED);
    }

    hiddProfile_SendActivateCfm(HiddProfileGetClientTask(), status);
}

static void hiddProfile_DeactivateCfmHandler(uint8 status)
{
    if (status == CSR_BT_RESULT_CODE_HIDD_SUCCESS)
    {        
        HiddProfileSetState(HIDD_STATE_INITIALIZED);
        MessageSendLater(HiddProfileGetTask(), HIDD_INTERNAL_REACTIVATE_REQ, NULL, HIDD_REACTIVATION_DELAY);
    }
    else
    {
        DEBUG_LOG("hiddProfile_DeactivateCfmHandler: FAILED. Result Code (%d)", status);
    }

    hiddProfile_SendDeactivateCfm(HiddProfileGetClientTask(), status);
}

static void hiddProfile_ConnectFailedHandler(bdaddr addr, uint32 connid)
{
    HiddProfileSetState(HIDD_STATE_INITIALIZED);
    hiddProfile_SendConnectInd(HiddProfileGetClientTask(), hidd_profile_status_connect_failed, addr, connid);
}

static void hiddProfile_ReconnectingHandler(bdaddr addr, uint32 connid)
{
    hiddProfile_SendConnectInd(HiddProfileGetClientTask(), hidd_profile_status_reconnecting, addr, connid);
}

static void hiddProfile_ConnectIndHandler(bdaddr addr, uint32 connid)
{
    bool notified = FALSE;
    if (HiddProfileGetState() == HIDD_STATE_CONNECT_ACCEPT ||
        HiddProfileGetState() == HIDD_STATE_CONNECTING ||
        HiddProfileGetState() == HIDD_STATE_DISCONNECTED)
    {
        HiddProfileSetConnectionId(connid);
        HiddProfileSetState(HIDD_STATE_CONNECTED);
        HiddProfileSetBdAddr(addr);
        /* If this is completing a connect request, send confirmation for this device */
        if (TaskList_Size(TaskList_GetBaseTaskList(HiddProfileGetConnectClientList())) != 0)
        {
            notified = ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetConnectClientList()),
                                                         &addr, profile_manager_success,
                                                         profile_manager_hidd_profile, profile_manager_connect);
        }

        if(!notified)
        {
            /* Provide generic indication to the Profile Manager */
            ProfileManager_GenericConnectedInd(profile_manager_hidd_profile, &addr);
        }

        hiddProfile_SendConnectInd(HiddProfileGetClientTask(), hidd_profile_status_connected, addr, connid);
    }
    else
    {
        DEBUG_LOG("hiddProfile_ConnectIndHandler - connect indication received in wrong state (%d)!!!", HiddProfileGetState());
    }
}

static void hiddProfile_DisconnectIndHandler(bdaddr addr)
{
    DEBUG_LOG("hiddProfile_DisconnectIndHandler - Disconnected from %04x:%0x:%02x", addr.lap, addr.uap, addr.nap);
    hiddProfile_SendDisconnectInd(HiddProfileGetClientTask(), hidd_profile_status_disconnected, addr);

    if (HiddProfileGetState() == HIDD_STATE_DISCONNECTING)
    {
        /* Local disconnection */
        HiddProfileSetState(HIDD_STATE_DISCONNECTED);
        HiddDeactivateReqSend();
    }
    else if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        /* Remote Disconnection. We will not auto reconnect with host. Wait for reconnection by remote. */
        HiddProfileSetState(HIDD_STATE_INITIALIZED);
        HiddDeactivateReqSend();
    }
    else
    {
        DEBUG_LOG("hiddProfile_ConnectIndHandler - disconnect indication received in wrong state (%d)!!!", HiddProfileGetState());
    }

    /* Inform Profile Manager */
    if(!ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetDisconnectClientList()),
                                          &addr, profile_manager_success,
                                          profile_manager_hidd_profile, profile_manager_disconnect))
    {
        /* otherwise provide indication to the Profile Manager */
        ProfileManager_GenericDisconnectedInd(profile_manager_hidd_profile, &addr, profile_manager_disconnected_normal);
    }
}

static void hiddProfile_DataCfmHandler(uint8 status)
{
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        hiddProfile_SendDataCfm(HiddProfileGetClientTask(), status);
    }
    else
    {
        DEBUG_LOG("hiddProfile_DataCfmHandler - Data CFM received in wrong state (%d)!!!", HiddProfileGetState());
    }
}

static void hiddProfile_DataIndHandler(hidd_report_type_t reportType, uint16 len, uint8* report)
{
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        /* Send Data to application after trimming off the HID Report Header */
        hiddProfile_SendDataInd(HiddProfileGetClientTask(), reportType,
                                len - LENGTH_OF_HDR_IN_HID_REPORT,
                                report + LENGTH_OF_HDR_IN_HID_REPORT);
    }
}

static void hiddProfile_ModeChangeIndHandler(CsrBtHiddPowerModeType mode, uint16 status)
{
    DEBUG_LOG("hiddProfile_ModeChangeIndHandler - Mode: %d, Status: %d", mode, status);
    if(mode == CSR_BT_HIDD_ACTIVE_MODE && HiddProfileGetState() != HIDD_STATE_CONNECTED)
    {
        hiddProfile_ConnectIndHandler(HiddProfileGetTaskData()->hidd_bd_addr, HiddProfileGetTaskData()->connection_id);
    }
    else if (mode == CSR_BT_HIDD_DISCONNECT_MODE && HiddProfileGetState() != HIDD_STATE_DISCONNECTED)
    {
        hiddProfile_DisconnectIndHandler(HiddProfileGetTaskData()->hidd_bd_addr);
    }
    else
    {
        DEBUG_LOG("hiddProfile_ModeChangeIndHandler - Mode Change received in state %d!!!", HiddProfileGetState());
    }
}

static void hiddProfile_StatusIndHandler(bdaddr addr, uint8 status, uint32 connid)
{
    DEBUG_LOG("hiddProfile_StatusIndHandler - Status:%d, Address: %04x:%0x:%02x", status, addr.lap, addr.uap, addr.nap);
    PanicFalse(HiddProfileGetState() >= HIDD_STATE_INITIALIZED);
    switch(status)
    {
        case CSR_BT_HIDD_DISCONNECTED:
            hiddProfile_DisconnectIndHandler(addr);
        break;
        case CSR_BT_HIDD_CONNECTED:
            hiddProfile_ConnectIndHandler(addr, connid);
        break;
        case CSR_BT_HIDD_CONNECT_FAILED:
            hiddProfile_ConnectFailedHandler(addr, connid);
        break;
        case CSR_BT_HIDD_UNREGISTER_FAILED:
            /* nothing to be done */
        break;
        case CSR_BT_HIDD_RECONNECTING:
            hiddProfile_ReconnectingHandler(addr, connid);
        break;
    }
}

static void hiddProfile_ControlIndHandler(uint8 transactionType, uint8 parameter, uint16 datalen, uint8* data)
{
    DEBUG_LOG("hiddProfile_ControlIndHandler transaction type (%d), parameter (%d)", transactionType, parameter);
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        switch(transactionType)
        {
            case CSR_BT_HIDD_GET_REPORT:
            {
                uint16 size = 0;
                /* HID Spec (1.1.1)
                 * If size bit is 1 then a 2-octet BufferSize field follows the Report ID. Device needs to send data with this size only
                 * If size bit is 0 then size is not specified. It is assumed that host has allocated a buffer equal to the default size of the report
                 */
                if (data[INDEX_OF_HDR_IN_GET_REPORT] & HID_GET_REPORT_SIZE_FIELD_MASK)
                {
                    size = *((uint16*)&data[INDEX_OF_SIZE_IN_GET_REPORT]);
                }

                /* Send data without header */
                hiddProfile_SendGetReport(HiddProfileGetClientTask(),
                                          data[INDEX_OF_HDR_IN_GET_REPORT] & HID_GET_REPORT_TYPE_FIELD_MASK, /*report type in pkt header*/
                                          size,
                                          data[INDEX_OF_REPORT_ID_IN_HID_REPORT]);
            }
            break;
            case CSR_BT_HIDD_SET_REPORT:
            {
                /* Report ID may or may not be present in the pkt. */
                if (data[INDEX_OF_REPORT_ID_IN_HID_REPORT] != 0)
                {
                    /* Report Id present. Send pkt without Header and Report ID */
                    hiddProfile_SendSetReport(HiddProfileGetClientTask(),
                                              parameter, /*report type*/
                                              data[INDEX_OF_REPORT_ID_IN_HID_REPORT],
                                              datalen - (LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT),
                                              data + (LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT));
                }
                else
                {
                    /* No ReportID. Send pkt without Header */
                    hiddProfile_SendSetReport(HiddProfileGetClientTask(),
                                              parameter, /*report type*/
                                              0,
                                              datalen - LENGTH_OF_HDR_IN_HID_REPORT,
                                              data + LENGTH_OF_HDR_IN_HID_REPORT);
                }
            }
            break;
            case CSR_BT_HIDD_HANDSHAKE:
            case CSR_BT_HIDD_CONTROL:
            case CSR_BT_HIDD_GET_PROTOCOL:
            case CSR_BT_HIDD_SET_PROTOCOL:
            case CSR_BT_HIDD_GET_IDLE:
            case CSR_BT_HIDD_SET_IDLE:
            case CSR_BT_HIDD_DATA:
            case CSR_BT_HIDD_DATC:
                /*Ignore*/
            default:
                DEBUG_LOG("Unexpected transaction type (%d) received!", transactionType);
        }

        HiddProfileSetState(HIDD_STATE_CONNECTED);
    }
    else
    {
        DEBUG_LOG("hiddProfile_ControlIndHandler - control indication received in wrong state (%d)!!!", HiddProfileGetState());
    }
}

static void handleSynergyHiddPrim(Task task, Message message)
{
    UNUSED(task);
    CsrBtHiddPrim *primType = (CsrBtHiddPrim*) message;
    DEBUG_LOG("handleSynergyHidPrim - Msg: %d, State: %d", *primType, HiddProfileGetState());
    switch(*primType)
    {
        case CSR_BT_HIDD_ACTIVATE_CFM:
        {
            CsrBtHiddActivateCfm *cfm = (CsrBtHiddActivateCfm*)message;
            hiddProfile_ActivateReqCfmHandler(cfm->resultCode);
        }
        break;
        case CSR_BT_HIDD_DEACTIVATE_CFM:
        {
            CsrBtHiddDeactivateCfm *cfm = (CsrBtHiddDeactivateCfm*)message;
            hiddProfile_DeactivateCfmHandler(cfm->resultCode);           
        }
        break;
        case CSR_BT_HIDD_STATUS_IND:
        {           
            CsrBtHiddStatusInd *ind = (CsrBtHiddStatusInd*)message;
            bdaddr bd_addr = {0};
            BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);
            hiddProfile_StatusIndHandler(bd_addr, ind->status, ind->btConnId);
        }
        break;
        case CSR_BT_HIDD_CONTROL_IND:
        {        
            CsrBtHiddControlInd *ind = (CsrBtHiddControlInd*)message;            
            hiddProfile_ControlIndHandler(ind->transactionType, ind->parameter, ind->dataLen, ind->data);
        }
        break;
        case CSR_BT_HIDD_DATA_CFM:
        {
            CsrBtHiddDataCfm *cfm = (CsrBtHiddDataCfm*)message;            
            hiddProfile_DataCfmHandler(cfm->resultCode);
        }
        break;
        case CSR_BT_HIDD_DATA_IND:
        {
            CsrBtHiddDataInd *ind = (CsrBtHiddDataInd*)message;
            hiddProfile_DataIndHandler(ind->reportType, ind->reportLen, ind->report);
        }
        break;
        case CSR_BT_HIDD_MODE_CHANGE_IND:
        {
            CsrBtHiddModeChangeInd *ind = (CsrBtHiddModeChangeInd*)message;            
            hiddProfile_ModeChangeIndHandler(ind->mode, ind->resultCode);
        }
        break;

        default:
        {
            DEBUG_LOG("handleSynergyHiddPrim: Unexpected HIDD Primitive = 0x%04x", *primType);
            HiddProfile_Handshake(hidd_profile_handshake_unsupported);
        }
    }

    HiddFreeUpstreamMessageContents((void *) message);
}

static void hiddTaskHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch (id)
    {
        case HIDD_PRIM:
            handleSynergyHiddPrim(task ,message);
            break;
        case HIDD_INTERNAL_REACTIVATE_REQ:
            hiddProfile_ReactivateReq();
            break;
        default:
            DEBUG_LOG_VERBOSE("hiddTaskHandler Message id %d, Not handled",id);
    }
}

/************************************************************************************************
 * HIDD External and Internal APIs
 ************************************************************************************************/
static void hiddProfile_ActivateReq(bdaddr *bd_addr, uint16 len, const uint8* data, uint16 int_flush_timeout)
{
    uint8 *sdp  = NULL;
    if(len)
    {
        sdp = PanicUnlessMalloc(len); /* This will be freed by Synergy profile on SDP registration complete */
        memcpy(sdp, data, len);
    }

    if (int_flush_timeout != L2CA_FLUSH_TO_INFINITE)
    {
        L2CA_QOS_T *qos = NULL;
    
        qos = PanicUnlessMalloc(sizeof(L2CA_QOS_T)); /* This will be freed by Synergy profile on SDP registration complete */
        qos->flags        = 0;
        qos->service_type = L2CA_QOS_DEFAULT_SERVICE_TYPE;
        qos->token_rate   = L2CA_QOS_DEFAULT_TOKEN_RATE;
        qos->token_bucket = L2CA_QOS_DEFAULT_TOKEN_BUCKET;
        qos->peak_bw      = L2CA_QOS_DEFAULT_PEAK_BW;
        qos->latency      = L2CA_QOS_DEFAULT_LATENCY;
        qos->delay_var    = L2CA_QOS_DEFAULT_DELAY_VAR;
    
        HiddActivateReqSend(&hiddTask,
                            NULL,                 /* NO QOS for Control channel */
                            qos,                  /* Set QOS for Interrupt channel so we can set flush timeout */
                            int_flush_timeout,    /* flush time used for QoS channels */
                            *((CsrBtDeviceAddr *)bd_addr),
                            len,
                            sdp);
    }
    else
    {
        HiddActivateReqSend(&hiddTask,
                            NULL,                   /* NO QOS for Control channel   */
                            NULL,                   /* NO QOS for Interrupt channel   */
                            L2CA_FLUSH_TO_INFINITE, /* No flush timeout for QoS channels */
                            *((CsrBtDeviceAddr *)bd_addr),
                            len,
                            sdp);
    }
}

static void hiddProfile_ReactivateReq(void)
{
    HiddProfileSetState(HIDD_STATE_INITIALIZED);
    if (HiddProfileGetTaskData()->connect_requested == TRUE)
    {
        DEBUG_LOG("hiddProfile_ReactivateReq: Connect");        
        hiddProfile_ActivateReq(&HiddProfileGetTaskData()->req_bd_addr, 0, NULL, HiddProfileGetTaskData()->int_flush_timeout);

        HiddProfileGetTaskData()->connect_requested = FALSE;
    }
    else
    {
        DEBUG_LOG("hiddProfile_ReactivateReq: Accept Connect");
        bdaddr bd_addr = {0};
        hiddProfile_ActivateReq(&bd_addr, HiddProfileGetTaskData()->sdp_len, HiddProfileGetTaskData()->sdp_data, HiddProfileGetTaskData()->int_flush_timeout);
    }
}

static void hiddProfile_ControlRes(CsrBtHiddTransactionType transaction_type, uint8 parameter, uint16 datalen, uint8* data)
{
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {        
        HiddControlResSend(transaction_type, parameter, datalen, data);
    }
    else
    {
        DEBUG_LOG("hiddProfile_ControlRes. Incorrect State:%d", HiddProfileGetState());
    }
}

static void hiddProfile_Connect(bdaddr *bd_addr)
{
    PanicNull((bdaddr *)bd_addr);
    if (BtDevice_IsProfileSupported(bd_addr, DEVICE_PROFILE_HIDD))
    {
        device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
        if (device)
        {
            ProfileManager_AddToNotifyList(TaskList_GetBaseTaskList(HiddProfileGetConnectClientList()), device);
            if ((HiddProfileGetState() == HIDD_STATE_CONNECTED) && (BdaddrIsSame(bd_addr, &HiddProfileGetBdAddr())))
            {
                /* If already connected, send an immediate confirmation */
                ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetConnectClientList()),
                                                  bd_addr, profile_manager_success,
                                                  profile_manager_hidd_profile, profile_manager_connect);
            }
            else if (!HiddProfile_Connect(bd_addr))
            {
                /* Could not process connect request, respond with failure */
                ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetConnectClientList()),
                                                  bd_addr, profile_manager_failed,
                                                  profile_manager_hidd_profile, profile_manager_connect);

            }
        }
    }
}

static void hiddProfile_Disconnect(bdaddr *bd_addr)
{
    PanicNull((bdaddr *)bd_addr);
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        ProfileManager_AddToNotifyList(TaskList_GetBaseTaskList(HiddProfileGetDisconnectClientList()), device);
        if ((HiddProfileGetState() == HIDD_STATE_DISCONNECTED) && (BdaddrIsSame(bd_addr, &HiddProfileGetBdAddr())))
        {
            /* If already disconnected, send an immediate confirmation */
            ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetDisconnectClientList()),
                                              bd_addr, profile_manager_success,
                                              profile_manager_hidd_profile, profile_manager_disconnect);
        }
        else if (!HiddProfile_Disconnect(bd_addr))
        {
            /* Could not process disconnect request, respond with failure */
            ProfileManager_NotifyConfirmation(TaskList_GetBaseTaskList(HiddProfileGetDisconnectClientList()),
                                              bd_addr, profile_manager_failed,
                                              profile_manager_hidd_profile, profile_manager_disconnect);
        }
    }
}

/* For future use. Single instance maintained for now.*/
hiddInstanceTaskData * HiddProfile_CreateInstance(void)
{
    return HiddProfileGetTaskData();
}

hiddInstanceTaskData * HiddProfile_GetInstanceForBdaddr(const bdaddr * addr)
{
    hiddInstanceTaskData * inst = HiddProfileGetTaskData();
    if(addr->lap == inst->hidd_bd_addr.lap &&
       addr->nap == inst->hidd_bd_addr.nap &&
       addr->uap == inst->hidd_bd_addr.uap)
    {
        return inst;
    }
    else
    {
        return NULL;
    }
}

/* HIDD Profile APIs */
bool HiddProfile_Init(Task task)
{
    UNUSED(task);
    memset(HiddProfileGetTaskData(), 0, sizeof(hiddInstanceTaskData));
    HiddProfileSetState(HIDD_STATE_INITIALIZED);

    ProfileManager_RegisterProfile(profile_manager_hidd_profile, hiddProfile_Connect, hiddProfile_Disconnect);
    /* create lists for connection/disconnection requests */
    TaskList_WithDataInitialise(HiddProfileGetConnectClientList());
    TaskList_WithDataInitialise(HiddProfileGetDisconnectClientList());
    return TRUE;
}

void HiddProfile_RegisterDevice(Task client_task, uint16 len, const uint8 *data, uint16 int_flush_timeout)
{
    DEBUG_LOG("HiddProfile_RegisterDevice");
    bdaddr bd_addr = {0};

    PanicFalse(data != NULL);
    HiddProfileSetClientTask(client_task);

    /* Save SDP len and data pointer for future use */
    HiddProfileGetTaskData()->sdp_data = data;
    HiddProfileGetTaskData()->sdp_len = len;

    /* Save the Interrupt Channel Flush Timeout */
    HiddProfileGetTaskData()->int_flush_timeout = int_flush_timeout;

    /* Activate */
    hiddProfile_ActivateReq(&bd_addr, len, data, int_flush_timeout);
}

bool HiddProfile_DataReq(uint8 reportid, uint16 data_len, const uint8* data)
{
    DEBUG_LOG("HiddProfile_DataReq");
    bool result = FALSE;
    /* We are sending data even when we may have not received confirmation for previous packets transmission.
     * In case previous packet has not been sent and the stream sink is full then this data would get 
     * discarded in the hidd library */
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        /* First byte is used for Report header, which is populated by library */
        uint16 report_length = data_len + LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT;
        uint8* report = PanicUnlessMalloc(report_length);
        memcpy(&report[LENGTH_OF_HDR_IN_HID_REPORT], &reportid, LENGTH_OF_REPORT_ID_IN_HID_REPORT);
        memcpy(&report[LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT], data, data_len);
        HiddDataReqSend(report_length, report);
        result = TRUE;
    }
    else
    {
        DEBUG_LOG("HiddProfile_DataReq. Incorrect State:%d", HiddProfileGetState());
    }

    return result;
}

bool HiddProfile_Connect(bdaddr *bd_addr)
{
    DEBUG_LOG("HiddProfile_Connect - State :%d", HiddProfileGetState());
    bool result = TRUE;
    bool reconnect_to_same_device = TRUE;

    if (!BdaddrIsSame(bd_addr, &HiddProfileGetTaskData()->hidd_bd_addr))
    {
        DEBUG_LOG("HiddProfile_Connect with different bdaddr");
        reconnect_to_same_device = FALSE;
    }

    switch(HiddProfileGetState())
    {
        case HIDD_STATE_DISCONNECTED:
        {
            if (reconnect_to_same_device)
            {
                HiddModeChangeReqSend(CSR_BT_HIDD_ACTIVE_MODE);
            }
            else
            {
                DEBUG_LOG("HiddProfile_Connect - Can't connect to another device in Disconnected Mode. Perform Deinit First!!");
                result = FALSE;
            }
        }
        break;
        case HIDD_STATE_INITIALIZED:
        {
            hiddProfile_ActivateReq(bd_addr, HiddProfileGetTaskData()->sdp_len, HiddProfileGetTaskData()->sdp_data, HiddProfileGetTaskData()->int_flush_timeout);
        }
        break;
        case HIDD_STATE_CONNECT_ACCEPT:
            HiddProfileGetTaskData()->connect_requested = TRUE;
            HiddProfileGetTaskData()->req_bd_addr = *bd_addr;
            HiddDeactivateReqSend();
        break;
        case HIDD_STATE_CONNECTED:
        {
            DEBUG_LOG("Already connected!!!");
            result = FALSE;
        }
        break;
        default:
            DEBUG_LOG("HiddProfile_Connect. Incorrect State: %d", HiddProfileGetState());
            result = FALSE;
    }

    return result;
}

bool HiddProfile_Disconnect(bdaddr *remote_addr)
{
    DEBUG_LOG("HiddProfile_Disconnect");
    UNUSED(remote_addr);
    bool result = TRUE;
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        DEBUG_LOG("HiddProfile_Disconnect: Changing Mode");
        HiddProfileSetState(HIDD_STATE_DISCONNECTING);
        HiddModeChangeReqSend(CSR_BT_HIDD_DISCONNECT_MODE);
    }
    else
    {
        HiddProfileSetState(HIDD_STATE_DISCONNECTING);
        /* Deactivation could be send in any state */
        HiddDeactivateReqSend();
    }

    return result;
}

bool HiddProfile_Handshake(hidd_handshake_result_t status)
{
    DEBUG_LOG("HiddProfile_Handshake, status:%d",status);
    bool result = FALSE;
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        hiddProfile_ControlRes(CSR_BT_HIDD_HANDSHAKE, status, 0, NULL);
        result = TRUE;
    }
    else
    {
        DEBUG_LOG("HiddProfile_Handshake. Incorrect State:%d", HiddProfileGetState());
    }

    return result;
}

bool HiddProfile_DataRes(hidd_report_type_t report_type, uint8 reportid, uint16 datalen, uint8* data)
{
    DEBUG_LOG("HiddProfile_DataRes, type:%d", report_type);
    bool result = FALSE;
    if (HiddProfileGetState() == HIDD_STATE_CONNECTED)
    {
        uint16 report_len = datalen + LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT;
        uint8* report = PanicUnlessMalloc(report_len);
        memcpy(&report[LENGTH_OF_HDR_IN_HID_REPORT], &reportid, LENGTH_OF_REPORT_ID_IN_HID_REPORT);
        memcpy(&report[LENGTH_OF_HDR_IN_HID_REPORT + LENGTH_OF_REPORT_ID_IN_HID_REPORT], data, datalen);
        hiddProfile_ControlRes(CSR_BT_HIDD_DATA, report_type, report_len, report);
        result= TRUE;
    }
    else
    {
        DEBUG_LOG("HiddProfile_DataRes. Incorrect State:%d", HiddProfileGetState());
    }

    return result;
}

void HiddProfile_Deinit(void)
{
    DEBUG_LOG("HiddProfile_Deinit");
    if (HiddProfileGetState() >= HIDD_STATE_INITIALIZED)
    {
        HiddProfileSetState(HIDD_STATE_UNINITIALIZED);
        HiddProfileGetTaskData()->connect_requested = FALSE;
        HiddDeactivateReqSend();
    }
}

uint32 HiddProfile_GetConnectionId(void)
{
    return HiddProfileGetConnectionId();
}

bool HiddProfile_IsConnected(void)
{
    hiddState s = HiddProfileGetState();
    return (s == HIDD_STATE_CONNECTED);
}

#endif /*INCLUDE_HIDD_PROFILE*/
#endif /*USE_SYNERGY*/
