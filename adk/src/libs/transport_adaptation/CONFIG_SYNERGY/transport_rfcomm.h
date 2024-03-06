/****************************************************************************
Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd.
 
FILE NAME
    transport_rfcomm.h
 
DESCRIPTION
    This file provides necessary routines for creating and managing a transport
    link using RFCOMM.
*/

#ifndef TRANSPORT_RFCOMM_H
#define TRANSPORT_RFCOMM_H

/******************************************************************************
DESCRIPTION
       This function is called to allocate a RFCOMM server channel.
*/
void transportRfcommRegister(Task task, uint8 server_channel);

/******************************************************************************
DESCRIPTION
       This function is called to de-allocate a RFCOMM server channel.
*/
void transportRfcommDeregister(Task task, uint8 server_channel);

/******************************************************************************
DESCRIPTION
       This function is called to request a RFCOMM connection to the device 
        with the specified Bluetooth device Address and server channel.
*/
void transportRfcommConnect(Task task,
                            const bdaddr* bd_addr,
                            uint16 local_transport_id,
                            uint8 remote_transport_id);

/******************************************************************************
DESCRIPTION
       This function is called to accept/reject connection request from the remote 
       device with the specified Bluetooth device Address.
*/
void transportRfcommConnectResponse(Task task,
                                    const bdaddr* bd_addr,
                                    uint16 transport_id,
                                    Sink sink,
                                    bool response);


/******************************************************************************
DESCRIPTION
       This function is called to request a disconnection of the RFCOMM channel.
*/
void transportRfcommDisconnect(Task task, Sink sink);

/******************************************************************************
DESCRIPTION
       Handler function for CM messages pertaining to RFCOMM.
*/
void handleRfcommCmMessage(Task task, Message message);

#endif /* TRANSPORT_RFCOMM_H */
