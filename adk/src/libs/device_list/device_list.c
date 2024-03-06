/*!
\copyright  Copyright (c) 2018 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      A collection of device objects that can be retrieved by property type and value.
*/

#include <string.h>
#include <stdlib.h>

#include <device_list.h>
#include <panic.h>

/* MRU index of most recently used device */
#define DEVICE_LIST_MRU_DEVICE_INDEX    0

static device_t *device_list = NULL;
static device_list_mru_index_t *mru_index_list = NULL;
static uint8 trusted_device_list = 0;


static void deviceList_ClearAllMruIndex(void)
{
    if(mru_index_list)
    {
        for (uint8 i = 0; i<trusted_device_list; i++)
        {
            mru_index_list[i] = DEVICE_LIST_MRU_INDEX_NOT_SET;
        }
    }
}

static void deviceList_ClearMruIndex(device_list_mru_index_t mru_index)
{
    if(mru_index == DEVICE_LIST_MRU_INDEX_NOT_SET)
    {
        return;
    }
    else
    {
        for (uint8 i = 0; i<trusted_device_list; i++)
        {
            if((mru_index_list[i] != DEVICE_LIST_MRU_INDEX_NOT_SET) && (mru_index_list[i] > mru_index))
            {
                mru_index_list[i]--;
            }
            else if(mru_index_list[i] == mru_index)
            {
                mru_index_list[i] = DEVICE_LIST_MRU_INDEX_NOT_SET;
            }
        }
    }
}

void DeviceList_Init(uint8 num_devices)
{
    PanicNotZero(device_list);

    trusted_device_list = num_devices;

    device_list = (device_t *)PanicUnlessMalloc(trusted_device_list * sizeof(device_t));
    memset(device_list, 0, (trusted_device_list * sizeof(device_t)));

    mru_index_list = (device_list_mru_index_t *)PanicUnlessMalloc(trusted_device_list * sizeof(device_list_mru_index_t));
    deviceList_ClearAllMruIndex();
}

unsigned DeviceList_GetNumOfDevices(void)
{
    int i;
    unsigned count = 0;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
            count++;
    }

    return count;
}

void DeviceList_RemoveAllDevices(void)
{
    int i;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            /* Should the device be destroyed by this function? */
            device_list[i] = 0;
        }
    }
    free(device_list);
    device_list = NULL;
    free(mru_index_list);
    mru_index_list = NULL;

}

bool DeviceList_AddDevice(device_t device)
{
    int i;
    bool added = FALSE;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == 0)
        {
            device_list[i] = device;
            added = TRUE;
            DeviceList_DeviceWasUsed(device);
            break;
        }
        else if (device_list[i] == device)
        {
            added = FALSE;
            break;
        }
    }

    return added;
}

void DeviceList_RemoveDevice(device_t device)
{
    int i;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == device)
        {
            device_list[i] = 0;
            deviceList_ClearMruIndex(mru_index_list[i]);
            /* Should the device be destroyed by this function? */
            break;
        }
    }
}

bool DeviceList_IsDeviceOnList(device_t device)
{
    int i;

    if(device == 0)
    {
        return FALSE;
    }

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == device)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void deviceList_FindMatchingDevices(device_property_t id, const void *value, size_t size, bool just_first, device_t *device_array, unsigned *len_device_array)
{
    int i;
    unsigned num_found_devices = 0;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            void *property;
            size_t property_size;

            if (Device_GetProperty(device_list[i], id, &property, &property_size))
            {
                if ((size == property_size) && !memcmp(value, property, size))
                {
                    device_array[num_found_devices] = device_list[i];
                    num_found_devices += 1;

                    if (just_first)
                        break;
                }
            }
        }
    }
    *len_device_array = num_found_devices;
}

static void deviceList_FindDevicesWithProperty(device_property_t id, device_t *device_array, unsigned *len_device_array)
{
    int i;
    unsigned num_found_devices = 0;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            void *property;
            size_t property_size;

            if (Device_GetProperty(device_list[i], id, &property, &property_size))
            {
                device_array[num_found_devices] = device_list[i];
                num_found_devices += 1;
            }
        }
    }
    *len_device_array = num_found_devices;
}

device_t DeviceList_GetFirstDeviceWithPropertyValue(device_property_t id, const void *value, size_t size)
{
    device_t found_devices[1] = {0};
    unsigned num_found_devices = 0;

    deviceList_FindMatchingDevices(id, value, size, TRUE, found_devices, &num_found_devices);
    PanicFalse(num_found_devices <= 1);

    return found_devices[0];
}

void DeviceList_GetAllDevicesWithPropertyValue(device_property_t id, void *value, size_t size, device_t **device_array, unsigned *len_device_array)
{
    unsigned num_found_devices = 0;

    device_t *found_devices = NULL;

    found_devices = (device_t*)PanicUnlessMalloc(trusted_device_list * sizeof(device_t));
    memset(found_devices, 0, trusted_device_list * sizeof(device_t) );

    deviceList_FindMatchingDevices(id, value, size, FALSE, found_devices, &num_found_devices);
	*device_array = found_devices;

    *len_device_array = num_found_devices;
}

void DeviceList_GetAllDevicesWithProperty(device_property_t id, device_t **device_array, unsigned *len_device_array)
{
    unsigned num_found_devices = 0;

    device_t *found_devices = NULL;

    found_devices = (device_t*)PanicUnlessMalloc(trusted_device_list * sizeof(device_t));
    memset(found_devices, 0, trusted_device_list * sizeof(device_t) );

    deviceList_FindDevicesWithProperty(id, found_devices, &num_found_devices);
    *device_array = found_devices;

    *len_device_array = num_found_devices;
}

void DeviceList_Iterate(device_list_iterate_callback_t action, void *action_data)
{
    int i;

    PanicFalse(action);

    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i])
        {
            action(device_list[i], action_data);
        }
    }
}

uint8 DeviceList_GetMaxTrustedDevices(void)
{
    PanicZero(trusted_device_list);

    return trusted_device_list;
}

uint8 DeviceList_GetIndexOfDevice(device_t device)
{
    uint8 i;
    for (i = 0; device && i < trusted_device_list; i++)
    {
        if (device_list[i] == device)
        {
            return i;
        }
    }
    return 0xFF;
}

device_t DeviceList_GetDeviceAtIndex(uint8 index)
{
    device_t device = NULL;

    if (index < trusted_device_list)
    {
        device = device_list[index];
    }
    return device;
}

void DeviceList_DeviceWasUsed(device_t device)
{
    device_list_mru_index_t present_mru_index = 0;
    uint8 index = DeviceList_GetIndexOfDevice(device);

    PanicFalse(index < trusted_device_list);

    present_mru_index = mru_index_list[index];

    if(present_mru_index != DEVICE_LIST_MRU_DEVICE_INDEX)
    {
        for (uint8 i = 0; i<trusted_device_list; i++)
        {
            if((mru_index_list[i] != DEVICE_LIST_MRU_INDEX_NOT_SET) && (mru_index_list[i] < present_mru_index))
            {
                mru_index_list[i]++;
            }
        }

        mru_index_list[index] = DEVICE_LIST_MRU_DEVICE_INDEX;
    }
}

void DeviceList_ClearMruIndexOfDevice(device_t device)
{
    uint8 i;
    for (i = 0; i < trusted_device_list; i++)
    {
        if (device_list[i] == device)
        {
            deviceList_ClearMruIndex(mru_index_list[i]);
        }
    }
}

device_list_mru_index_t DeviceList_GetMruIndex(device_t device)
{
    uint8 i;
    device_list_mru_index_t mru_index = DEVICE_LIST_MRU_INDEX_NOT_SET;

    if(device)
    {
        for (i = 0; i < trusted_device_list; i++)
        {
            if (device_list[i] == device)
            {
                mru_index = mru_index_list[i];
            }
        }
    }

    return mru_index;
}

device_t DeviceList_GetDeviceAtMruIndex(device_list_mru_index_t mru_index)
{
    device_t mru_device = NULL;
    uint8 i;

    for (i = 0; i < trusted_device_list; i++)
    {
        if (mru_index_list[i] == mru_index)
        {
            mru_device = device_list[i];
            break;
        }
    }

    return mru_device;
}
