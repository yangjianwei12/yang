/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Low-level access to the PDL data.

*/

#include "device_db_serialiser_pskey.h"

#include <device_properties.h>

#include <device_list.h>
#include <bdaddr.h>
#include <ps.h>
#include <panic.h>
#include <logging.h>
#include <vmtypes.h>

#ifdef USE_SYNERGY

#define BDADDR_KEY 101

#if defined(INCLUDE_EXTENDED_TDL) || defined(INCLUDE_EXTENDED_TDL_DB_SERIALISER)

#define MAX_TDL_DEVICES 12

#define ATTR_KEY_1 138
#define ATTR_KEY_2 139

/* The following structures have the same layout as TdDbIndex and TdDbIndexElem structures used inside of synergy.
   They match data layout in the ps key.
*/
typedef struct
{
    uint16         deviceAddress[3];
    uint8          addressType:2;
    uint8          rank:4;
    uint8          priorityDevice:1;
} td_db_index_elem_t;

#else

#define MAX_TDL_DEVICES 8

#define ATTR_KEY_1 142
#define ATTR_KEY_2 143

/* The following structures have the same layout as TdDbIndex and TdDbIndexElem structures used inside of synergy.
   They match data layout in the ps key.
*/
typedef struct
{
    BD_ADDR_T       deviceAddress;
    uint8           addressType;
    uint8           rank;
    uint8           priorityDevice;
} td_db_index_elem_t;
#endif

typedef struct
{
    uint16               count;
    td_db_index_elem_t   device[MAX_TDL_DEVICES];
} td_db_index_t;

static bool deviceDbSerialiser_GetAddrFromPsKey(device_order_t device_order, bdaddr *addr)
{
    td_db_index_t *buffer = PanicUnlessMalloc(PS_SIZE_ADJ(sizeof(td_db_index_t))*sizeof(uint16));
    bool result = FALSE;

    if(PsRetrieve(BDADDR_KEY, buffer, PS_SIZE_ADJ(sizeof(td_db_index_t))))
    { 
        BD_ADDR_T devAddr;
#if defined(INCLUDE_EXTENDED_TDL) || defined(INCLUDE_EXTENDED_TDL_DB_SERIALISER)
        devAddr.nap = (uint16)buffer->device[device_order].deviceAddress[0];
        devAddr.uap = (uint8)(buffer->device[device_order].deviceAddress[1] >> 8);
        devAddr.lap = (uint24)(buffer->device[device_order].deviceAddress[1] & 0xFF) << 16 | buffer->device[device_order].deviceAddress[2];
#else
        devAddr = buffer->device[device_order].deviceAddress;
#endif
        BdaddrConvertBluestackToVm(addr, &devAddr);

        DEBUG_LOG_VERBOSE("deviceDbSerialiser_GetAddrFromPsKey addr %x:%x:%x",
                        addr->nap, addr->uap, addr->lap);

        result = TRUE;
    }

    free(buffer);

    return result;
}

#else

#define BDADDR_KEY_1 142
#define BDADDR_KEY_2 143

#define ATTR_KEY_1 100
#define ATTR_KEY_2 101

inline static uint16 deviceDbSerialiser_GetBdAddrPsKey(device_order_t device_order)
{
    uint16 key = 0;
    switch(device_order)
    {
        case device_order_first:
            key = BDADDR_KEY_1;
            break;

        case device_order_second:
            key = BDADDR_KEY_2;
            break;

        default:
            Panic();
    }

    return key;
}

static bool deviceDbSerialiser_GetAddrFromPsKey(device_order_t device_order, bdaddr *addr)
{
    uint16 key = deviceDbSerialiser_GetBdAddrPsKey(device_order);

    uint16 num_of_words = PsRetrieve(key, NULL, 0);

    DEBUG_LOG_VERBOSE("deviceDbSerialiser_GetAddrFromPsKey key 0x%x, num_of_words %d", key, num_of_words);

    if(num_of_words > 2)
    {
        uint16 *buffer = PanicUnlessMalloc(num_of_words*sizeof(uint16));

        PsRetrieve(key, buffer, num_of_words);

        addr->nap = buffer[0];
        addr->uap = buffer[1] >> 8;
        addr->lap = buffer[2] | (buffer[1] & 0xff) << 16;

        DEBUG_LOG_VERBOSE("deviceDbSerialiser_GetAddrFromPsKey addr %x:%x:%x",
                        addr->nap, addr->uap, addr->lap);

        free(buffer);

        return TRUE;
    }

    return FALSE;
}

#endif

device_t DeviceDbSerialiser_GetDeviceFromPsKey(device_order_t device_order)
{
    bdaddr addr = {0};

    if(deviceDbSerialiser_GetAddrFromPsKey(device_order, &addr))
    {
        return DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, &addr, sizeof(addr));
    }

    return 0;
}

uint16 DeviceDbSerialiser_GetAttributePsKey(device_order_t device_order)
{
    uint16 ps_key = 0;

    switch(device_order)
    {
        case device_order_first:
            ps_key = ATTR_KEY_1;
            break;

        case device_order_second:
            ps_key = ATTR_KEY_2;
            break;

        default:
            Panic();
    }

    return ps_key;
}
