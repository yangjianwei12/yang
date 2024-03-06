/* Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd. */
/*    */
/**
 * \file
 * Routines for managing all the underlying transports 
 */

#include "cm_lib.h"

#include <sink.h>
#include <print.h>
#include <panic.h>
#include <logging.h>
#include <vm.h>
#include "transport_accessory.h"
#include "transport_adaptation.h"
#include "transport_rfcomm.h"
#include "transport_adaptation_common.h"
#include <marshal_common_desc.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(transport_adaptation_message_id_t)

/******************************************************************************
Main handler for Transport adaptation module. This handles all the 
communication with underlying transports.
*/
void TransportHandleMessage(Task task, MessageId id, Message message)
{
    PRINT(("TransportHandleMessage\n"));


    if (id == CM_PRIM)
    {
        handleRfcommCmMessage(task, message);
    }
    else
    {
        handleAccessoryMessage(task, id, message);
    }
}

/******************************************************************************/
bool TransportInit(Task app_task)
{
    PRINT(("TA: TransportInit\n"));

    if((app_task == NULL) ||(transportIsInitialised() == TRUE))
    {
        /* Either Application task is NULL or TA task already intialised. */
        return FALSE;
    }
    else
    {
        /* Initailase the transport manager data base. */
        transportInitialise(app_task);
        return TRUE;
    }
}

/******************************************************************************/
bool TransportRegisterReq(transport_type_t transport, uint16 transport_id)
{
    PRINT(("TA: TransportRegisterReq\n"));

    if(transportIsInitialised())
    {
        if(transport== TRANSPORT_RFCOMM)
        {
            transportRfcommRegister(transportGetLibTask(), (uint8)transport_id);
            return TRUE;
        }
        else if(transport == TRANSPORT_ACCESSORY)
        {
            transportAccessoryRegisterReq(transportGetLibTask(), transport_id);
            return TRUE;
        }
    }
    
    return FALSE;
}

/******************************************************************************/
bool TransportDeregisterReq(transport_type_t transport, uint16 transport_id)
{
    PRINT(("TA: TransportDeregisterReq\n"));

    if(transportIsInitialised())
    {
        if(transport == TRANSPORT_RFCOMM)
        {
            transportRfcommDeregister(transportGetLibTask(), (uint8)transport_id);
            return TRUE;
        }
        else if(transport == TRANSPORT_ACCESSORY)
        {
            transportAccessoryDeregisterReq(transportGetLibTask(), transport_id);
            return TRUE;
        }
    }
    return FALSE;
}

/******************************************************************************/
bool TransportConnectReq(transport_type_t transport,
                         const tp_bdaddr *tpaddr,
                         uint16 transport_id,
                         uint16 remote_transport_id)
{
    PRINT(("TA: TransportConnectReq\n"));

    if(transportIsInitialised())
    {
        if(tpaddr == NULL)
            return FALSE;

        if(transport == TRANSPORT_RFCOMM)
         {
            transportRfcommConnect((Task)transportGetLibTask(),
                                   &tpaddr->taddr.addr,
                                   transport_id,
                                   (uint8)remote_transport_id);
             return TRUE;
         }
    }
     return FALSE;
}

/******************************************************************************/
bool TransportDisconnectReq(transport_type_t transport, Sink sink)
{
    PRINT(("TA: TransportDisconnectReq\n"));

    if(transportIsInitialised())
    {
        if(!SinkIsValid(sink))
            return FALSE;

        if(transport == TRANSPORT_RFCOMM)
         {
            transportRfcommDisconnect(transportGetLibTask(), sink);
             return TRUE;
         }
    }
     return FALSE;
}

/******************************************************************************/
void TransportRegisterTaskWithConnectionId(uint16 cid)
{
    /* Assosiate the task with the synergy connection manager. */
    configureRfcommTaskToConnectionId(cid, transportGetLibTask());
}

/******************************************************************************/
bool TransportConnectResponse(transport_type_t transport,
                              const tp_bdaddr *tpaddr,
                              uint16 transport_id,
                              Sink sink,
                              bool response)
{
    if(transport == TRANSPORT_RFCOMM)
    {
        transportRfcommConnectResponse(transportGetLibTask(), &tpaddr->taddr.addr, transport_id, sink, response);
        return TRUE;
    }
    return FALSE;
}

