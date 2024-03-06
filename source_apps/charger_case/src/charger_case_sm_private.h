/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private header for the application state machine
*/

#ifndef CHARGER_CASE_SM_PRIVATE_H
#define CHARGER_CASE_SM_PRIVATE_H

/*! \brief Application state machine internal message IDs */
enum sm_internal_message_ids
{
    SM_INTERNAL_DELETE_PAIRED_DEVICES,  /*!< Delete all paired devices */
    SM_INTERNAL_CONNECT_SINK_DEVICE,    /*!< Connect to previously paired device
                                             or initiate pairing if required */
    SM_INTERNAL_DISCONNECT_SINK_DEVICE,    /*!< Disconnect from a sink device */
    SM_INTERNAL_START_PAIRING,
    SM_INTERNAL_STOP_PAIRING,
    SM_INTERNAL_FACTORY_RESET,          /*!< Reset device to factory defaults */
};

typedef struct
{
    bool    request_pairing_delete;
} SM_INTERNAL_DISCONNECT_SINK_DEVICE_T;


#endif // CHARGER_CASE_SM_PRIVATE_H
