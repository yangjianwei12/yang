/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup kip Kymera Inter Processor Adaptor
 * \ingroup adaptor
 *
 * The KIP adaptor provides a messaging interface between the primary core and
 * the secondary cores in the audio subsystem.
 */
/**
 * \file kip_msg_adaptor.h
 * \ingroup kip
 *
 * Unlike other adaptors, "kip_adaptor_send_message", the function used to
 * send a message, can be used outside of the adaptor component as it is more
 * powerful.
 */

#ifndef KIP_MSG_ADAPTOR_H
#define KIP_MSG_ADAPTOR_H

#include "types.h"
#include "adaptor/connection_id.h"
#include "kip_msg_prim.h"

/****************************************************************************
Public Functions
*/

/**
 * \brief Send a message over the Kymera Inter Processor interface.
 *
 * \param conn_id  As defined in connection_id.h with processor field
 * \param msg_id   Identify the type of message transmitted
 * \param length   Number of words of msg_data
 * \param msg_data Pointer to the message data
 * \param context  Pointer that will be passed to the function in
 *                 charge of handling the response.
 *
 * \return True in case of success
 *
 * \note It is recommended to use adaptor_send_message instead of
 *       kip_adaptor_send_message when context is NULL.
 */
extern bool kip_adaptor_send_message(CONNECTION_LINK conn_id,
                                     KIP_MSG_ID msg_id,
                                     unsigned length,
                                     uint16 *msg_data,
                                     void* context);

#endif /* KIP_MSG_ADAPTOR_H */
