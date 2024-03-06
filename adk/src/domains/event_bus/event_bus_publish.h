/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\addtogroup event_bus
\ingroup    domains
\brief      Publish API for the Event Bus
            The Event Bus component is provided for the centralized distribution of events throughout the system, 
            decoupling event publishers from their subscribers.  
@{
*/      

#ifndef EVENT_BUS_PUBLISH_H_
#define EVENT_BUS_PUBLISH_H_

#include "event_bus_types.h"

/*! \brief Publish an event bus event on a channel.

    \param channel - the event bus channel to publishe to.
    \param event - event bus event to publish.
    \param data - data associated with the event
    \param data_size - size of the data
*/
void EventBus_Publish(event_bus_channel_t channel, event_bus_event_t event, void * data, uint16 data_size);

#endif
/*! @} End of group documentation */