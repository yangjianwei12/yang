/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of module managing BT Inquiry.
*/

#include "inquiry_manager.h"

#include <logging.h>
#include <message.h>
#include <task_list.h>
#include <unexpected_message.h>

#include <panic.h>
#include <hci.h>
#ifdef USE_SYNERGY
#include <csr_bt_cm_prim.h>
#include <cm_lib.h>
#include <bdaddr.h>
#endif

/*! \brief Macro for simplifying creating messages */
#define MAKE_INQUIRY_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);

#define INQUIRY_MANAGER_CLIENT_TASKS_LIST_INIT_CAPACITY 1

/*! Inquiry manager state */
typedef enum
{
    INQUIRY_MANAGER_STATE_IDLE,
    INQUIRY_MANAGER_STATE_INQUIRY,
} inquiry_manager_state_t;

/*! Inquiry Manager data */
typedef struct
{
    /*! Init's local task */
    TaskData task;

    /*! Configured collection of parameters */
    const inquiry_manager_scan_parameters_t *parameter_set;

    /*! Number of parameters in configured collection */
    uint16 parameter_set_length;

    /*! List of clients */
    TASK_LIST_WITH_INITIAL_CAPACITY(INQUIRY_MANAGER_CLIENT_TASKS_LIST_INIT_CAPACITY) clients;

    /*! Inquiry manager state */
    inquiry_manager_state_t state;

    /*! The collection index chosen for the inquiry scan */
    uint16 set_filter;

} inquiry_manager_data_t;

inquiry_manager_data_t inquiry_manager_data;

/*! Get pointer to Inquiry Manager task*/
#define InquiryManager_GetTask() (&inquiry_manager_data.task)

/*! Get pointer to Inquiry Manager data structure */
#define InquiryManager_GetTaskData() (&inquiry_manager_data)

/*! Get pointer to Inquiry Manager client list */
#define InquiryManager_GetClientList() (task_list_flexible_t *)(&inquiry_manager_data.clients)



#ifdef USE_SYNERGY
static void inquiryManager_HandleCmSetEventFilterCodCfm(void *msg)
{
    CsrBtCmSetEventFilterCodCfm *prim = (CsrBtCmSetEventFilterCodCfm *)msg;

    DEBUG_LOG_DEBUG("inquiryManager_HandleCmSetEventFilterCodCfm, resultCode 0x%x, resultSupplier 0x%x", prim->resultCode, prim->resultSupplier);

    if (prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS || prim->resultSupplier != CSR_BT_SUPPLIER_CM)
    {   /* Something went wrong - clear all inquiry filters */
        CmClearEventFilterReqSend(InquiryManager_GetTask(), INQUIRY_RESULT_FILTER);
        
        TaskList_MessageSendId(&inquiry_manager_data.clients, INQUIRY_MANAGER_SCAN_COMPLETE);
        inquiry_manager_data.state = INQUIRY_MANAGER_STATE_IDLE;
        inquiry_manager_data.set_filter = 0;

    }
    else
    {
        uint8 maxResponses;
        uint8 inquiryTimeout;
        maxResponses = inquiry_manager_data.parameter_set[inquiry_manager_data.set_filter].max_responses;
        inquiryTimeout = inquiry_manager_data.parameter_set[inquiry_manager_data.set_filter].timeout;
        CmInquiryExtReqSend(InquiryManager_GetTask(), HCI_INQ_CODE_GIAC, CSR_BT_CM_INQUIRY_TX_POWER_LEVEL_DEFAULT, 0 /*configMask */,
                            maxResponses, inquiryTimeout);
    }
}

static void inquiryManager_HandleCmInquiryResultInd(void *msg)
{
    CsrBtCmInquiryResultInd *prim = (CsrBtCmInquiryResultInd *)msg;
    bdaddr bd_addr = { 0 };

    BdaddrConvertBluestackToVm(&bd_addr, &prim->deviceAddr);

    /* Process Inquiry Result */    
    DEBUG_LOG_DEBUG("inquiryManager_HandleCmInquiryResultInd, bdaddr 0x%04x 0x%02x 0x%06lx rssi %d cod %lx",
              bd_addr.nap,
              bd_addr.uap,
              bd_addr.lap,
              prim->rssi,
              prim->classOfDevice);
    
    MAKE_INQUIRY_MESSAGE(INQUIRY_MANAGER_RESULT);
    
    message->bd_addr = bd_addr;
    message->dev_class = prim->classOfDevice;

    message->clock_offset = prim->clockOffset;
    message->page_scan_rep_mode = prim->pageScanRepMode;
    message->page_scan_mode = prim->pageScanMode;
    message->rssi = prim->rssi;
    
    TaskList_MessageSend(&inquiry_manager_data.clients, INQUIRY_MANAGER_RESULT, message);
}
static void inquiryManager_HandleCmInquiryCfm(void *msg)
{
    CsrBtCmInquiryCfm *prim = (CsrBtCmInquiryCfm *)msg;

    DEBUG_LOG_DEBUG("inquiryManager_HandleCmInquiryCfm, resultCode 0x%x", prim->resultCode);
        
    TaskList_MessageSendId(&inquiry_manager_data.clients, INQUIRY_MANAGER_SCAN_COMPLETE);
    inquiry_manager_data.state = INQUIRY_MANAGER_STATE_IDLE;
    inquiry_manager_data.set_filter = 0;
}

static void inquiryManager_HandleCmPrimitives(Task task, void *msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    UNUSED(task);

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_FILTER_COD_CFM:
            inquiryManager_HandleCmSetEventFilterCodCfm(msg);
            break;

        case CSR_BT_CM_CLEAR_EVENT_FILTER_CFM:
            /* Do Nothing */
            break;

        case CSR_BT_CM_INQUIRY_RESULT_IND:
            inquiryManager_HandleCmInquiryResultInd(msg);
            break;

        case CSR_BT_CM_INQUIRY_CFM:
            inquiryManager_HandleCmInquiryCfm(msg);
            break;

        default:
            DEBUG_LOG_DEBUG("inquiryManager_HandleCmPrimitives, unexpected CM prim 0x%x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents(msg);
}
#else
/*! \brief Handler for Inquiry results from the connection library
           Sends a INQUIRY_MANAGER_RESULT if a result is returned from the library.

           If the Status is inquiry_status_ready but no device is found, the
           repeat limit has not been reached and a immediate stop has not been requested via
           InquiryManager_Stop() then the repeat counter will be decremented and a new inquiry
           scan begun.

           Once atleast one device candidate has been found or if the repeats have been exhausted
           then the INQUIRY_MANAGER_SCAN_COMPLETE shall be sent*/
static void inquiryManager_HandleClDmInquireResult(const CL_DM_INQUIRE_RESULT_T *result)
{
    DEBUG_LOG_FN_ENTRY("inquiryManager_HandleClDmInquireResult");

    if (result->status == inquiry_status_result)
    {
        DEBUG_LOG_DEBUG("inquiryManager_HandleClDmInquireResult, bdaddr 0x%04x 0x%02x 0x%06lx rssi %d cod %lx",
                  result->bd_addr.nap,
                  result->bd_addr.uap,
                  result->bd_addr.lap,
                  result->rssi,
                  result->dev_class);

        MAKE_INQUIRY_MESSAGE(INQUIRY_MANAGER_RESULT);

        message->bd_addr = result->bd_addr;
        message->dev_class = result->dev_class;
        message->clock_offset = result->clock_offset;
        message->page_scan_rep_mode = result->page_scan_rep_mode;
        message->page_scan_mode = result->page_scan_mode;
        message->rssi = result->rssi;

        TaskList_MessageSend(&inquiry_manager_data.clients, INQUIRY_MANAGER_RESULT, message);
    }
    else
    {
        DEBUG_LOG_DEBUG("inquiryManager_HandleClDmInquireResult: Scan Complete");

        TaskList_MessageSendId(&inquiry_manager_data.clients, INQUIRY_MANAGER_SCAN_COMPLETE);
        inquiry_manager_data.state = INQUIRY_MANAGER_STATE_IDLE;
        inquiry_manager_data.set_filter = 0;
    }
}
#endif /* USE_SYNERGY */

/*! \brief Handler for connection library messages.*/
static void inquiryManager_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            inquiryManager_HandleCmPrimitives(task, (void *) message);
            break;
#else
        case CL_DM_INQUIRE_RESULT:
            inquiryManager_HandleClDmInquireResult((CL_DM_INQUIRE_RESULT_T *)message);
            break;
#endif /* USE_SYNERGY */

        default:
            UnexpectedMessage_HandleMessage(id);
        break;
    }
}

void InquiryManager_RegisterParameters(const inquiry_manager_scan_parameters_t *params, uint16 set_length)
{
    DEBUG_LOG_FN_ENTRY("InquiryManager_RegisterParameters %p , length:%d", params, set_length);

    PanicNull((void*)params);
    InquiryManager_GetTaskData()->parameter_set = params;
    InquiryManager_GetTaskData()->parameter_set_length = set_length;

}

bool InquiryManager_Start(uint16 filter_id)
{
    DEBUG_LOG_FN_ENTRY("InquiryManager_Start filter:enum:inquiry_manager_filter_t:%d", filter_id);

    if (InquiryManager_IsInquiryActive())
    {
        DEBUG_LOG_DEBUG("InquiryManager_Start: Cannot Start. Inquiry already in progress");
        return FALSE;
    }

    if (InquiryManager_GetTaskData()->parameter_set == NULL)
    {
        DEBUG_LOG_ERROR("InquiryManager_Start: Inquiry Manager not configured");
        Panic();
    }

    if (filter_id >= InquiryManager_GetTaskData()->parameter_set_length)
    {
        DEBUG_LOG_ERROR("InquiryManager_Start: filter_id out of bounds");
        return FALSE;
    }

    inquiry_manager_data.set_filter = filter_id;
    inquiry_manager_data.state = INQUIRY_MANAGER_STATE_INQUIRY;

#ifdef USE_SYNERGY
    uint32 cod = inquiry_manager_data.parameter_set[filter_id].class_of_device;
    /* Send COD event filter request, on CFM start inquiry */
     CmSetEventFilterCodReqSend(InquiryManager_GetTask(),
                                TRUE,
                                0,
                                cod,
                                cod);
#else
    /* Start inquiry */
    ConnectionWriteInquiryMode(InquiryManager_GetTask(), inquiry_mode_rssi);
    ConnectionInquire(InquiryManager_GetTask(),
                      HCI_INQ_CODE_GIAC,
                      inquiry_manager_data.parameter_set[filter_id].max_responses,
                      inquiry_manager_data.parameter_set[filter_id].timeout,
                      inquiry_manager_data.parameter_set[filter_id].class_of_device
                      );
#endif
    TaskList_MessageSendId(&inquiry_manager_data.clients, INQUIRY_MANAGER_SCAN_STARTED);
    return TRUE;
}

bool InquiryManager_Init(Task init_task)
{
    UNUSED(init_task);
    DEBUG_LOG_FN_ENTRY("InquiryManager_Init");

    inquiry_manager_data.state = INQUIRY_MANAGER_STATE_IDLE;

    inquiry_manager_data.parameter_set = NULL;

    TaskList_InitialiseWithCapacity(InquiryManager_GetClientList(),INQUIRY_MANAGER_CLIENT_TASKS_LIST_INIT_CAPACITY);

    inquiry_manager_data.task.handler = inquiryManager_HandleMessage;
    return TRUE;

}

bool InquiryManager_ClientRegister(Task client_task)
{
    return TaskList_AddTask(&inquiry_manager_data.clients, client_task);
}

bool InquiryManager_IsInquiryActive(void){
    return (inquiry_manager_data.state >= INQUIRY_MANAGER_STATE_INQUIRY);
}

void InquiryManager_Stop(void)
{
    DEBUG_LOG_FN_ENTRY("InquiryManager_Stop");
#ifdef USE_SYNERGY
    CmCancelInquiryReqSend(InquiryManager_GetTask());
    /* As per design, synergy does not send response for Inquiry cancel request and
     * the application is expected to move to appropriate state once inquiry cancel request is sent. */
    TaskList_MessageSendId(&inquiry_manager_data.clients, INQUIRY_MANAGER_SCAN_COMPLETE);
    inquiry_manager_data.state = INQUIRY_MANAGER_STATE_IDLE;
    inquiry_manager_data.set_filter = 0;
#else
    ConnectionInquireCancel(InquiryManager_GetTask());
#endif /* USE_SYNERGY */
}

bool InquiryManager_ClientUnregister(Task client_task)
{
    return TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(InquiryManager_GetClientList()), client_task);
}
