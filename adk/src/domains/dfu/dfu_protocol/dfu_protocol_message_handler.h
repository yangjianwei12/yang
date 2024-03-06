/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       dfu_protocol_message_handler.h
    \defgroup   dfu_protocol_message_handler Message Handler
    @{
        \ingroup    dfu_protocol
        \brief      Definition of the message handler APIs for the dfu_protocol module
*/

#ifndef DFU_PROTOCOL_MESSAGE_HANDLER_H
#define DFU_PROTOCOL_MESSAGE_HANDLER_H

typedef enum
{
    DFU_PROTOCOL_ABORT_WAITING_FOR_PEER_TIMEOUT,
    DFU_PROTOCOL_RETRY_VALIDATION_REQUEST_IND
} dfu_protocol_internal_messages_t;

/*! \brief Initalise the DFU protocol message handler */
void DfuProtocol_MessageHandlerInit(void);

/*! \brief Get the DFU protocol message handler task
    \return The message handler task */
Task DfuProtocol_GetMessageHandlerTask(void);

/*! \brief Get the DFU protocol internal message handler task
    \return The internal message handler task */
Task DfuProtocol_GetInternalMessageHandlerTask(void);

#endif /* DFU_PROTOCOL_MESSAGE_HANDLER_H */

/*! @} */