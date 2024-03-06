/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\ingroup    event_bus
\brief      Event Bus implementation
@{
*/

#include "event_bus_logging.h"
#include "event_bus_subscribe.h"
#include "event_bus_publish.h"

#include <panic.h>

#include <stdlib.h>
#include <stdio.h>

#define EVENT_BUS_LOG       DEBUG_LOG
#define EVENT_BUS_LOG_DATA  DEBUG_LOG_DATA

typedef struct
{
    event_bus_channel_t channel;
    uint8 number_of_subscribers;
    event_bus_channel_handler_t * subscribers;
} channel_registry_t;

typedef struct
{
    uint8 number_of_active_channels;
    channel_registry_t * active_channel_registries;
} subscriber_registry_t;

static subscriber_registry_t subscriber_registry = { 0 };

static void eventBus_PrintRegistry(void)
{
    uint16 calculated_size = 0;
    EVENT_BUS_LOG("EventBus: active channels=%d", subscriber_registry.number_of_active_channels);
    for(int i = 0; i < subscriber_registry.number_of_active_channels; i++)
    {
        EVENT_BUS_LOG("EventBus:     channel=%d subscribers=%d",subscriber_registry.active_channel_registries[i].channel, subscriber_registry.active_channel_registries[i].number_of_subscribers);
        for(int j = 0; j < subscriber_registry.active_channel_registries[i].number_of_subscribers; j++)
        {
            EVENT_BUS_LOG("EventBus:         subscriber=%p", subscriber_registry.active_channel_registries[i].subscribers[j]);
        }
        calculated_size += sizeof(channel_registry_t) + (sizeof(event_bus_channel_handler_t) * subscriber_registry.active_channel_registries[i].number_of_subscribers);
    }
    calculated_size += sizeof(subscriber_registry_t) + (sizeof(channel_registry_t) * subscriber_registry.number_of_active_channels);
    EVENT_BUS_LOG("EventBus: registry size=%d",calculated_size);
}

static channel_registry_t * eventBus_AddChannelRegistry(event_bus_channel_t channel)
{
    subscriber_registry.active_channel_registries = PanicNull(realloc(subscriber_registry.active_channel_registries, 
                    (sizeof(*subscriber_registry.active_channel_registries) * (subscriber_registry.number_of_active_channels+1))));
                    
    channel_registry_t * channel_registry = &subscriber_registry.active_channel_registries[subscriber_registry.number_of_active_channels];
    memset(channel_registry, 0, sizeof(*channel_registry));                
    channel_registry->channel = channel;
    
    subscriber_registry.number_of_active_channels++;
    return channel_registry;
}

static void eventBus_RemoveChannelRegistry(channel_registry_t * channel_registry)
{
    for(int i = 0; i < subscriber_registry.number_of_active_channels; i++)
    {
        if(&subscriber_registry.active_channel_registries[i] == channel_registry)
        {
            subscriber_registry.number_of_active_channels--;
            memcpy(&subscriber_registry.active_channel_registries[i], &subscriber_registry.active_channel_registries[i+1], 
                            (sizeof(*subscriber_registry.active_channel_registries) * (subscriber_registry.number_of_active_channels-i)));
            if(subscriber_registry.number_of_active_channels)
            {
                subscriber_registry.active_channel_registries = PanicNull(realloc(subscriber_registry.active_channel_registries, 
                                  (sizeof(*subscriber_registry.active_channel_registries) * subscriber_registry.number_of_active_channels)));
            }
            else
            {
                free(subscriber_registry.active_channel_registries);
                subscriber_registry.active_channel_registries = NULL;
            }
            break;
        }
    }
}

static channel_registry_t * eventBus_GetChannelRegistry(event_bus_channel_t channel)
{
    channel_registry_t * subscriptions = NULL;
    for(int i = 0; i < subscriber_registry.number_of_active_channels; i++)
    {
        if(subscriber_registry.active_channel_registries[i].channel == channel)
        {
            subscriptions = &subscriber_registry.active_channel_registries[i];
            break;
        }
    }
    return subscriptions;
}

static bool eventBus_DoesSubscriptionExist(channel_registry_t * channel_registry, event_bus_channel_handler_t subscriber)
{
    PanicNull(channel_registry);
    bool subscription_found = FALSE;
    for(int i = 0; i < channel_registry->number_of_subscribers; i++)
    {
        if(channel_registry->subscribers[i] == subscriber)
        {
            subscription_found = TRUE;
        }
    }
    return subscription_found;
}

void EventBus_Subscribe(event_bus_channel_t channel, event_bus_channel_handler_t channel_handler)
{
    PanicNull((void *)channel_handler);
    EVENT_BUS_LOG("EventBus_Subscribe");
    channel_registry_t * channel_registry = eventBus_GetChannelRegistry(channel);
    if(!channel_registry)
    {
        EVENT_BUS_LOG("    adding channel channel=%d", channel);
        channel_registry = eventBus_AddChannelRegistry(channel);
    }

    if(!eventBus_DoesSubscriptionExist(channel_registry, channel_handler))
    {
        EVENT_BUS_LOG("    handler %p to channel %d", channel_handler, channel);
        channel_registry->subscribers = PanicNull(realloc(channel_registry->subscribers, 
                                            (sizeof(*channel_registry->subscribers) * (channel_registry->number_of_subscribers+1))));
        channel_registry->subscribers[channel_registry->number_of_subscribers] = channel_handler;
        channel_registry->number_of_subscribers++;
    }

    if(EventBus_GetCurrentLoggingLevel() == DEBUG_LOG_LEVEL_V_VERBOSE)
    {
        eventBus_PrintRegistry();
    }
}

void EventBus_Unsubscribe(event_bus_channel_t channel, event_bus_channel_handler_t channel_handler)
{
    EVENT_BUS_LOG("EventBus_Unsubscribe");
    channel_registry_t * channel_registry = eventBus_GetChannelRegistry(channel);
    if(channel_registry && eventBus_DoesSubscriptionExist(channel_registry, channel_handler))
    {
        for(int i = 0; i < channel_registry->number_of_subscribers; i++)
        {
            if(channel_registry->subscribers[i] == channel_handler)
            {
                EVENT_BUS_LOG("    handler %p from channel %d", channel_handler, channel);
                channel_registry->number_of_subscribers--;
                memcpy(&channel_registry->subscribers[i], &channel_registry->subscribers[i+1], (sizeof(*channel_registry->subscribers) * (channel_registry->number_of_subscribers-i)));
                if(channel_registry->number_of_subscribers)
                {
                    channel_registry->subscribers = PanicNull(realloc(channel_registry->subscribers, 
                                                (sizeof(*channel_registry->subscribers) * channel_registry->number_of_subscribers)));
                }
                else
                {
                    free(channel_registry->subscribers);
                    channel_registry->subscribers = NULL;
                    EVENT_BUS_LOG("    removing channel %d", channel_registry->channel);
                    eventBus_RemoveChannelRegistry(channel_registry);
                }
                break;
            }
        }
    }
    if(EventBus_GetCurrentLoggingLevel() == DEBUG_LOG_LEVEL_V_VERBOSE)
    {
        eventBus_PrintRegistry();
    }
}

void EventBus_UnsubscribeAll(void)
{
    for(int i = 0; i < subscriber_registry.number_of_active_channels; i++)
    {
        free(subscriber_registry.active_channel_registries[i].subscribers);
        memset(&subscriber_registry.active_channel_registries[i], 0, sizeof(channel_registry_t));
    }
    free(subscriber_registry.active_channel_registries);
    memset(&subscriber_registry, 0, sizeof(subscriber_registry_t));
}

static inline void eventBus_PublishToChannelRegistry(channel_registry_t * channel_registry, event_bus_channel_t channel, event_bus_event_t event, void * data, uint16 data_size)
{
    for(int i = 0; i < channel_registry->number_of_subscribers; i++)
    {
        EVENT_BUS_LOG("    to subscriber=%p", channel_registry->subscribers[i]);
        EVENT_BUS_LOG_DATA(data, data_size);
        channel_registry->subscribers[i](channel, event, data, data_size);
    }
}

void EventBus_Publish(event_bus_channel_t channel, event_bus_event_t event, void * data, uint16 data_size)
{
    EVENT_BUS_LOG("EventBus_Publish channel=%d event=%d data_size=%d", channel, event, data_size);
    channel_registry_t * channel_registry = eventBus_GetChannelRegistry(channel);
    if(channel_registry)
    {
        eventBus_PublishToChannelRegistry(channel_registry, channel, event, data, data_size);
    }
}

/*! @} End of group documentation */