/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup sink_service
    \brief      Sink service pairing state implementation
    @{
*/

#ifndef SINK_SERVICE_PAIRING_H
#define SINK_SERVICE_PAIRING_H

#include "sink_service_private.h"

/*! \brief Message handler to route all messages when sink service is in pairing mode

    \param[in] sm State machine
    \param[in] id Received message id
    \param[in] message Received message data
*/
void SinkServicePairing_HandleMessage(sink_service_state_machine_t *sm, MessageId id, Message message);

/*! \brief Request a SM instance to pair to a new device
           This will start rssi pairing to search for an suitable candidate

    \param sm The SM to start pairing on
    \return TRUE if the request was successfully made
*/
bool SinkServicePairing_PairingRequest(sink_service_state_machine_t *sm);

#endif /* SINK_SERVICE_PAIRING_H */

/*! @} */