/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\addtogroup event_bus
\ingroup    domains
\brief      Subscribe API for the Event Bus
            The Event Bus component is provided for the centralized distribution of events throughout the system, 
            decoupling event publishers from their subscribers.  
@{
*/      

#ifndef EVENT_BUS_SUBSCRIBE_H_
#define EVENT_BUS_SUBSCRIBE_H_

#include "event_bus_types.h"

/*! \brief Event bus channel handler callback

    \param channel - channel the delivered event is arriving on
    \param event - channel specific event being delivered
    \param data - pointer to data associated with the delivered event, only guaranteed to be valid at the time of this callback.
    \param data_size - size of the data 
*/
typedef void (*event_bus_channel_handler_t)(event_bus_channel_t channel, event_bus_event_t event, void * data, uint16 data_size);

/*! \brief Subscribe to an event bus channel.

    \param channel - the event bus channel to subscribe to.
    \param channel_handler - A function pointer to the subscribers channel handler.
*/
void EventBus_Subscribe(event_bus_channel_t channel, event_bus_channel_handler_t channel_handler);

/*! \brief Unsubscribe from an event bus channel.

    \param channel - the event bus channel to unsubscribe from.
    \param channel_handler - A function pointer to the channel handler that was previously used to subscribe.
*/
void EventBus_Unsubscribe(event_bus_channel_t channel, event_bus_channel_handler_t channel_handler);

/*! \brief Unsubscribe all previously subscribed channels.
*/
void EventBus_UnsubscribeAll(void);

#endif
/*! @} End of group documentation */