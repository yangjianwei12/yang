/*!
\copyright  Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Implementation of the Device Database Serialiser module.
*/

#include "device_db_serialiser.h"
#include "device_db_serialiser_pddu.h"
#include "device_properties.h"
#ifndef DISABLE_DEVICE_DB_ASSERT
#include "device_db_serialiser_assert.h"
#endif
#ifdef INCLUDE_DEVICE_DB_BACKUP
#include "device_db_serialiser_backup.h"
#endif

#include <bt_device.h>
#include <device_types.h>
#include <connection_manager_config.h>
#include <connection_no_ble.h>
#include <device_list.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>

/* Only BT devices, which have device_property_bdaddr is stored in PDL */
#define MAX_NUM_DEVICES_IN_PDL  BtDevice_GetMaxTrustedDevices()

/* Delay serialisation can have a strict deadline or a lazy deadline.
   Strict delayed messages are sent offset by this number */
#define DELAYED_SERIALISE_STRICT_MESSAGE_OFFSET 0x8000


static bool deserialised = FALSE;

static void deviceDbSeraliser_SerialiseDeviceLaterMessageHandler(Task task, MessageId id, Message message);

/*! This task is for the sole use of the serialise later functionality */
static const TaskData ddb_serialise_device_later_task = {.handler = deviceDbSeraliser_SerialiseDeviceLaterMessageHandler};

void DeviceDbSerialiser_Init(void)
{
    DeviceDbSerialiser_PdduInit();
    deserialised = FALSE;
}

static uint8 * deviceDbSerialiser_CreatePddFrame(pdd_size_t pdd_payload_size)
{
    pdd_size_t pdd_frame_size = DeviceBdSerialiser_ConvertPddPayloadSizeToFrameSize(pdd_payload_size);
    size_t word_adjusted_size = PS_SIZE_ADJ(pdd_frame_size) * sizeof(uint16);
    void *buffer = PanicUnlessMalloc(word_adjusted_size);
    memset(buffer, 0, word_adjusted_size);
    DeviceDbSerialiser_InitPddFrameHeader(buffer, DBS_PDD_FRAME_TYPE, pdd_frame_size);
    return buffer;
}

static void deviceDbSerialiser_CancelDelayedSerialise(device_t device)
{
    Task t = (Task)&ddb_serialise_device_later_task;
    MessageId id = DeviceList_GetIndexOfDevice(device);

    if (id != 0xff)
    {
        MessageCancelFirst(t, id);
        MessageCancelFirst(t, id + DELAYED_SERIALISE_STRICT_MESSAGE_OFFSET);
    }
}

static bool deviceDbSerialiser_SerialiseDevice(device_t device)
{
    pdd_size_t pdd_payload_size = 0;
    pdd_size_t *pddu_frame_sizes = NULL;
    bdaddr *device_bdaddr = NULL;
    size_t bdaddr_size;

    PanicFalse(DeviceList_IsDeviceOnList(device));

    DEBUG_LOG_INFO("DeviceDbSerialiser_SerialiseDevice(%p)", device);

    if (DeviceDbSerialiser_GetNumOfRegisteredPddu() == 0)
    {
        return FALSE;
    }

    // Only store bluetooth devices in the PDL, so the device must have a bdaddr
    if (!Device_GetProperty(device, device_property_bdaddr, (void *)&device_bdaddr, &bdaddr_size))
    {
        return FALSE;
    }

    uint16_t flags = 0;
    appDeviceGetFlagsForDevice(device, &flags);
    
    if ((flags & DEVICE_FLAGS_NOT_PAIRED) == DEVICE_FLAGS_NOT_PAIRED)
    {
        DEBUG_LOG_INFO("deviceDbSerialiser_SerialiseDevice: Device not paired; not serialising.");
        return FALSE;
    }    
    
    PanicFalse(bdaddr_size == sizeof(bdaddr));

    pddu_frame_sizes = DeviceDbSerialiser_GetAllPdduFrameSizes(device);

    pdd_payload_size = DeviceDbSerialiser_SumAllPdduFrameSizes(pddu_frame_sizes);

    if (pdd_payload_size)
    {
        uint8 *pdd_frame = deviceDbSerialiser_CreatePddFrame(pdd_payload_size);

        DeviceDbSerialiser_PopulatePddPayloadWithPdduFrames(device, pdd_frame, pddu_frame_sizes);

        ConnectionSmPutAttributeReq(0, TYPED_BDADDR_PUBLIC, device_bdaddr, DeviceDbSerialiser_GetPddFrameSize(pdd_frame), pdd_frame);

        free(pdd_frame);
    }

    free(pddu_frame_sizes);

    deviceDbSerialiser_CancelDelayedSerialise(device);

    return pdd_payload_size ? TRUE : FALSE;
}

static void deviceDbSerialiser_SerialiseDeviceIter(device_t device, void *data)
{
    UNUSED(data);
    deviceDbSerialiser_SerialiseDevice(device);
}

static bool deviceDbSerialiser_AreBothPeersPresent(void)
{
    deviceType device_type = DEVICE_TYPE_EARBUD;

    if(DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &device_type, sizeof(deviceType)))
    {
        device_type = DEVICE_TYPE_SELF;
        if(DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &device_type, sizeof(deviceType)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void DeviceDbSerialiser_Serialise(void)
{
    /* Useful to record serialisation as updates persistent storage */
    DEBUG_LOG_INFO("DeviceDbSerialiser_Serialise");

#ifdef INCLUDE_DEVICE_DB_BACKUP
    if(deviceDbSerialiser_AreBothPeersPresent())
    {
        DeviceDbSerialiser_MakeBackup();
    }
#endif

    DeviceList_Iterate(deviceDbSerialiser_SerialiseDeviceIter, NULL);

    MessageFlushTask((Task)&ddb_serialise_device_later_task);

    /* Completion of the serialisation less important */
    DEBUG_LOG_DEBUG("DeviceDbSerialiser_Serialise Completed");
}

inline static void deviceDbSerialiser_SerialiseOtherPeerDevice(deviceType device_type)
{
    deviceType other_device_type = (device_type == DEVICE_TYPE_SELF) ?
            DEVICE_TYPE_EARBUD : DEVICE_TYPE_SELF;

    device_t other_peer_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type,
            &other_device_type, sizeof(deviceType));

    deviceDbSerialiser_SerialiseDevice(other_peer_device);
}

/*! \brief Special handling of earbuds devices.

   On earbuds there are two devices SELF and EARBUD.
   They need to be kept in sync.

   Code below is checking if it is earbud by checking if both
   SELF and EARBUD exists.
   If device passed as parameter is SELF or EARBUD,
   then backup is created to prevent device db state corruption when e.g.
   hard reset happens during swapping of SELF and EARBUD keys.

   Additionally, to limit the risk of SELF and EARBUD keys stored in PS keys going out of sync,
   when SELF or EARBUD is going to be serialised other device is also stored.
   This way SELF and EARBUD are always stored at the same time.
*/
inline static void deviceDbSerialiser_HandleSelfAndEarbudPair(device_t device)
{
    if(deviceDbSerialiser_AreBothPeersPresent())
    {
        deviceType *device_type = NULL;
        size_t size = sizeof(deviceType);
        if(Device_GetProperty(device, device_property_type, (void *)&device_type, &size))
        {

            if(*device_type == DEVICE_TYPE_SELF || *device_type == DEVICE_TYPE_EARBUD)
            {
#ifdef INCLUDE_DEVICE_DB_BACKUP
                DeviceDbSerialiser_MakeBackup();
#endif
                deviceDbSerialiser_SerialiseOtherPeerDevice(*device_type);
            }
        }
    }
}


bool DeviceDbSerialiser_SerialiseDevice(device_t device)
{
    deviceDbSerialiser_HandleSelfAndEarbudPair(device);

    return deviceDbSerialiser_SerialiseDevice(device);
}

static void deviceDbSerialiser_DeserialisePddFrame(device_t device, uint8 *pdd_frame)
{
    pdd_type_t this_pddu_id;
    pdd_size_t this_pddu_frame_size;
    device_db_serialiser_registered_pddu_t *curr_pddu;
    uint8 pddu_frame_counter_watchdog = 1;
    uint8 *this_pddu_frame = DeviceDbSerialiser_GetPddFramePayload(pdd_frame);
    pdd_size_t pdd_payload_size_left = DeviceDbSerialiser_GetPddPayloadSize(pdd_frame);

    while (pdd_payload_size_left && pddu_frame_counter_watchdog)
    {
        this_pddu_id = DeviceDbSerialiser_GetPddFrameType(this_pddu_frame);
        this_pddu_frame_size = DeviceDbSerialiser_GetPddFrameSize(this_pddu_frame);
        curr_pddu = DeviceDbSerialiser_GetRegisteredPddu(this_pddu_id);

        if (curr_pddu)
        {
            curr_pddu->deser(device, DeviceDbSerialiser_GetPddFramePayload(this_pddu_frame), DeviceDbSerialiser_GetPddPayloadSize(this_pddu_frame), 0);
        } // If we don't know the PDDU, ignore it (for now - could panic - it may indicate corrupt data in PDL)

        PanicFalse(this_pddu_frame_size <= pdd_payload_size_left);
        pdd_payload_size_left -= this_pddu_frame_size;
        this_pddu_frame += this_pddu_frame_size;
        pddu_frame_counter_watchdog += 1;
    }
    PanicFalse(pddu_frame_counter_watchdog);
}

static void deviceDbSerialiser_CheckAttributesAndDeserialiseDevice(uint8 pdl_index)
{
    typed_bdaddr taddr = {0};
    bool attributes_read_ok = FALSE;
    bool device_attributes_valid = FALSE;
    pdd_size_t pdd_frame_length = ConnectionSmGetIndexedAttributeSizeNowReq(pdl_index);
    uint8 *pdd_frame = (uint8 *)PanicUnlessMalloc(pdd_frame_length);

    /* Even if there are no device attributes (i.e. pdd_frame_length == 0), call API
       to get bdaddr so that we can remove it from the PDL. */
    attributes_read_ok = ConnectionSmGetIndexedAttributeNowReq(0, pdl_index, pdd_frame_length, pdd_frame, &taddr);
    device_attributes_valid = attributes_read_ok && pdd_frame_length;

    if (device_attributes_valid)
    {
#ifdef USE_SYNERGY
        /* This check is required for the case where the device exists in the record
         * but the device attributes have not yet been updated in PS. In this case
         * keys wil be initialized to 16-bit zero value.
         */
        if (pdd_frame_length == 2 && pdd_frame[0] == 0x00 && pdd_frame[1] == 0x00)
        {
            device_attributes_valid = FALSE;
        }
        else if (DeviceDbSerialiser_GetPddFrameType(pdd_frame) != DBS_PDD_FRAME_TYPE)
        {
            /* Validation failed, delete complete database before panicking.
             * This is avoid same failure on restart since this called on initialization */
            DeviceDbSerialiser_HandleCorruption();
        }
#else
        PanicFalse(DeviceDbSerialiser_GetPddFrameType(pdd_frame) == DBS_PDD_FRAME_TYPE);
#endif
    }

    if (device_attributes_valid)
    {
        device_t device = Device_Create();
        Device_SetProperty(device, device_property_bdaddr, &taddr.addr, sizeof(bdaddr));
        DeviceList_AddDevice(device);

        deviceDbSerialiser_DeserialisePddFrame(device, pdd_frame);
    }
    else if (attributes_read_ok)
    {
        /* Remove any device from the PDL for which there are no device attributes. */
        ConnectionSmDeleteAuthDeviceReq(TYPED_BDADDR_PUBLIC, &taddr.addr);
    }

    free(pdd_frame);
}

void DeviceDbSerialiser_Deserialise(void)
{
    if(!deserialised)
    {
#ifndef DISABLE_DEVICE_DB_ASSERT
        /* Validate ps store before populating device db*/
        if(!DeviceDbSerialiser_IsPsStoreValid())
        {
#ifdef INCLUDE_DEVICE_DB_BACKUP
            DeviceDbSerialiser_RestoreBackup();
#else
            DeviceDbSerialiser_HandleCorruption();
#endif
        }
#endif

        for (uint8 pdl_index = 0; pdl_index < MAX_NUM_DEVICES_IN_PDL; pdl_index++)
        {
            deviceDbSerialiser_CheckAttributesAndDeserialiseDevice(pdl_index);
        }

        deserialised = TRUE;
    }
}

void DeviceDbSerialiser_SerialiseDeviceLater(device_t device, int32 delay)
{
    Task t = (Task)&ddb_serialise_device_later_task;
    MessageId id_lazy = DeviceList_GetIndexOfDevice(device);
    PanicFalse(id_lazy != 0xFF);
    MessageId id_strict = id_lazy + DELAYED_SERIALISE_STRICT_MESSAGE_OFFSET;

    int32 due_lazy = 0;
    int32 due_strict = INT32_MAX;
    bool pending_lazy = MessagePendingFirst(t, id_lazy, &due_lazy);
    bool pending_strict = MessagePendingFirst(t, id_strict, &due_strict);

    if (delay < 0)
    {
        // strict delay
        delay = -delay;

        if (due_strict > delay)
        {
            if (pending_strict)
            {
                MessageCancelFirst(t, id_strict);
            }

            MessageSendLater(t, id_strict, NULL, delay);
            DEBUG_LOG_INFO("DeviceDbSerialiser_SerialiseDeviceLater(%p) strict delay=%d", device, delay);

            if (pending_lazy)
            {
                // Having set a strict deadline, cancel the lazy one, serialisation
                // will occur at the strict deadline.
                MessageCancelFirst(t, id_lazy);
            }
        }
    }
    else
    {
        // lazy delay

        if (pending_strict)
        {
            // no need to set a lazy timeout if a strict one is already set
        }
        else
        {
            if (due_lazy < delay)
            {
                if (pending_lazy)
                {
                    MessageCancelFirst(t, id_lazy);
                }
                MessageSendLater(t, id_lazy, NULL, delay);
                DEBUG_LOG_INFO("DeviceDbSerialiser_SerialiseDeviceLater(%p) lazy delay=%d", device, delay);
            }
        }
    }
}

static void deviceDbSeraliser_SerialiseDeviceLaterMessageHandler(Task task, MessageId id, Message message)
{
    device_t device;

    if (id >= DELAYED_SERIALISE_STRICT_MESSAGE_OFFSET)
    {
        id -= DELAYED_SERIALISE_STRICT_MESSAGE_OFFSET;
    }

    device = DeviceList_GetDeviceAtIndex(id);

    if (device)
    {
        DeviceDbSerialiser_SerialiseDevice(device);
    }
    else
    {
        DEBUG_LOG_INFO("deviceDbSeraliser_SerialiseDeviceLaterMessageHandler(%p) not found", device);
    }

    UNUSED(task);
    UNUSED(message);
}
