/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup    av_state_machines
\brief      AV instance management

Main AV task.
*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV

#include "av.h"
#include "av_instance.h"

#include <device_properties.h>
#include <device_list.h>
#include <panic.h>
#include <stdlib.h>

static void avInstance_AddDeviceAvInstanceToIterator(device_t device, void* iterator_data)
{
    avInstanceTaskData* av_instance = AvInstance_GetInstanceForDevice(device);
    
    if(av_instance)
    {
        av_instance_iterator_t* iterator = (av_instance_iterator_t*)iterator_data;
        iterator->instances[iterator->index] = av_instance;
        iterator->index++;
    }
}

avInstanceTaskData* AvInstance_GetFirst(av_instance_iterator_t* iterator)
{
    memset(iterator, 0, sizeof(av_instance_iterator_t));
    
    DeviceList_Iterate(avInstance_AddDeviceAvInstanceToIterator, iterator);
    
    iterator->index = 0;
    
    return iterator->instances[iterator->index];
}

avInstanceTaskData* AvInstance_GetNext(av_instance_iterator_t* iterator)
{
    iterator->index++;
    
    if(iterator->index >= AV_MAX_NUM_INSTANCES)
        return NULL;
    
    return iterator->instances[iterator->index];
}

avInstanceTaskData* AvInstance_GetInstanceForDevice(device_t device)
{
    avInstanceTaskData** pointer_to_av_instance;
    size_t size_pointer_to_av_instance;
    
    if(device && Device_GetProperty(device, device_property_av_instance, (void**)&pointer_to_av_instance, &size_pointer_to_av_instance))
    {
        PanicFalse(size_pointer_to_av_instance == sizeof(avInstanceTaskData*));
        return *pointer_to_av_instance;
    }
    return NULL;
}

void AvInstance_SetInstanceForDevice(device_t device, avInstanceTaskData* av_instance)
{
    PanicFalse(Device_SetProperty(device, device_property_av_instance, &av_instance, sizeof(avInstanceTaskData*)));
}

device_t Av_GetDeviceForInstance(avInstanceTaskData* av_instance)
{
    return DeviceList_GetFirstDeviceWithPropertyValue(device_property_av_instance, &av_instance, sizeof(avInstanceTaskData*));
}

avInstanceTaskData *AvInstance_FindFromFocusHandset(void)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    unsigned index;
    deviceType type = DEVICE_TYPE_HANDSET;
    avInstanceTaskData* av_instance = NULL;
    
    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);
    
    for(index = 0; index < num_devices; index++)
    {
        av_instance = AvInstance_GetInstanceForDevice(devices[index]);
        if(av_instance)
        {
            break;
        }
    }
    
    free(devices);
    return av_instance;
}

audio_source_t Av_GetSourceForInstance(avInstanceTaskData* instance)
{
    device_t device = Av_FindDeviceFromInstance(instance);
    if(device)
    {
        return DeviceProperties_GetAudioSource(device);
    }
    return audio_source_none;
}

typedef struct av_instance_audio_source_search_data
{
    /*! The audio source associated with the device to find */
    audio_source_t source_to_find;
    /*! Set to valid instance if a device with the source is found */
    avInstanceTaskData *instance_found;
} av_instance_audio_source_search_data_t;

static void AvInstance_SearchForDeviceWithAudioSource(device_t device, void * data)
{
    av_instance_audio_source_search_data_t *search_data = data;

    if (DeviceProperties_GetAudioSource(device) == search_data->source_to_find)
    {
        deviceType device_type = BtDevice_GetDeviceType(device);
        if (device_type == DEVICE_TYPE_HANDSET || device_type == DEVICE_TYPE_SINK)
        {
            search_data->instance_found = Av_InstanceFindFromDevice(device);
        }
    }
}

avInstanceTaskData* Av_GetInstanceForHandsetSource(audio_source_t source)
{
    av_instance_audio_source_search_data_t search_data = {source, NULL};
    DeviceList_Iterate(AvInstance_SearchForDeviceWithAudioSource, &search_data);
    return search_data.instance_found;
}

static void avInstance_SearchForAvInstanceWithAudioSource(device_t device, void * data)
{
    av_instance_audio_source_search_data_t *search_data = data;

    if (DeviceProperties_GetAudioSource(device) == search_data->source_to_find)
    {
        avInstanceTaskData * av_instance = Av_InstanceFindFromDevice(device);
        if (av_instance)
        {
            search_data->instance_found = av_instance;
        }
    }
}

avInstanceTaskData* AvInstance_GetSinkInstanceForAudioSource(audio_source_t source)
{
    av_instance_audio_source_search_data_t search_data = {source, NULL};
    DeviceList_Iterate(avInstance_SearchForAvInstanceWithAudioSource, &search_data);
    return search_data.instance_found;
}

#ifdef USE_SYNERGY
/*! \brief Find AV instance using A2DP/AVRCP profile's search id (connection Id/ stream handle)

    This function finds the AV instance from the existing device records by matching
    the requested A2DP/AVRCP connection id or A2DP stream handle.

    \param  search_id A2DP/AVRCP connection Id or A2DP stream handle
    \param  search_by_conn_id Whether to find the AV instance using connection id or stream handle
    \param  profile_to_check Profile for which AV instance to be found
*/
avInstanceTaskData *AvInstance_GetInstanceForSearchId(uint8 search_id,
                                                      bool search_by_conn_id,
                                                      uint32 profile_to_check)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    unsigned index;

    avInstanceTaskData* av_instance = NULL;

    DeviceList_GetAllDevicesWithProperty(device_property_av_instance, &devices, &num_devices);

    for (index = 0; index < num_devices; index++)
    {
        av_instance = AvInstance_GetInstanceForDevice(devices[index]);

        if (av_instance)
        {
            if (profile_to_check == DEVICE_PROFILE_A2DP)
            {
                uint8 id;

                id = (search_by_conn_id == TRUE) ? av_instance->a2dp.device_id : av_instance->a2dp.stream_id;

                if (id == search_id)
                {
                    break;
                }
            }
            else if ((profile_to_check == DEVICE_PROFILE_AVRCP) && 
                     (search_id == av_instance->avrcp.connectionId))
            {
                break;
            }

            /* for the next iteration */
            av_instance = NULL;
        }
    }

    free(devices);
    return av_instance;
}
#endif /* USE_SYNERGY */

#endif /* INCLUDE_AV */
