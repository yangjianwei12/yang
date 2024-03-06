/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Backup of earbud properties.

*/

#include "device_db_serialiser_backup.h"
#include "device_db_serialiser_pddu.h"
#include "device_db_serialiser_pskey.h"
#include "device_properties.h"

#include <device_types.h>
#include <ps_key_map.h>

#include <vmtypes.h>
#include <device_list.h>
#include <ps.h>
#include <connection.h>
#include <panic.h>
#include <logging.h>

static pdd_size_t deviceDbSerialiser_GetFrameSizeForDevice(device_t device)
{
    pdd_size_t *pddu_frame_sizes = DeviceDbSerialiser_GetAllPdduFrameSizes(device);
    pdd_size_t pdd_payload_size = DeviceDbSerialiser_SumAllPdduFrameSizes(pddu_frame_sizes);
    free(pddu_frame_sizes);
    return DeviceBdSerialiser_ConvertPddPayloadSizeToFrameSize(pdd_payload_size);
}

static void deviceDbSerialiser_PopulatePddFrame(device_t device, uint8 *frame, pdd_size_t frame_size)
{
    pdd_size_t *pddu_frame_sizes = DeviceDbSerialiser_GetAllPdduFrameSizes(device);
    DeviceDbSerialiser_InitPddFrameHeader(frame, DBS_PDD_FRAME_TYPE, frame_size);
    DeviceDbSerialiser_PopulatePddPayloadWithPdduFrames(device, frame, pddu_frame_sizes);
    free(pddu_frame_sizes);
}

void DeviceDbSerialiser_MakeBackup(void)
{
    DEBUG_LOG_VERBOSE("DeviceDbSerialiser_MakeBackup");

    if (DeviceDbSerialiser_GetNumOfRegisteredPddu() == 0)
    {
        return;
    }

    device_t first_device = DeviceDbSerialiser_GetDeviceFromPsKey(device_order_first);

    if(!first_device)
    {
        return;
    }

    pdd_size_t first_size = deviceDbSerialiser_GetFrameSizeForDevice(first_device);


    device_t second_device = DeviceDbSerialiser_GetDeviceFromPsKey(device_order_second);

    if(!second_device)
    {
        return;
    }

    pdd_size_t second_size = deviceDbSerialiser_GetFrameSizeForDevice(second_device);

    DEBUG_LOG_VERBOSE("DeviceDbSerialiser_MakeBackup self_size %d, earbud_size %d", first_size, second_size);

    uint16 num_of_words = PS_SIZE_ADJ(first_size) + PS_SIZE_ADJ(second_size);
    uint16 offset_to_second = PS_SIZE_ADJ(first_size)*sizeof(uint16);

    uint8 *buffer = (uint8 *)PanicUnlessMalloc(num_of_words * sizeof(uint16));
    memset(buffer, 0, num_of_words * sizeof(uint16));

    deviceDbSerialiser_PopulatePddFrame(first_device, buffer, first_size);
    deviceDbSerialiser_PopulatePddFrame(second_device, &buffer[offset_to_second], second_size);

    uint16 num_of_words_written = PsStore(PS_KEY_EARBUD_DEVICES_BACKUP, buffer, num_of_words);

    DEBUG_LOG_VERBOSE("DeviceDbSerialiser_MakeBackup num_of_words %d, num_of_words_written %d", num_of_words, num_of_words_written);

    free(buffer);
}

void DeviceDbSerialiser_RestoreBackup(void)
{
    uint8 *buffer;
    pdd_size_t first_size;
    pdd_size_t offset_to_second;
    pdd_size_t second_size;
    uint16 num_of_words = PsRetrieve(PS_KEY_EARBUD_DEVICES_BACKUP, NULL, 0);

    DEBUG_LOG_ALWAYS("DeviceDbSerialiser_RestoreBackup num_of_words %d", num_of_words);

    if(num_of_words > 2)
    {
        buffer = PanicUnlessMalloc(num_of_words*sizeof(uint16));

        PsRetrieve(PS_KEY_EARBUD_DEVICES_BACKUP, buffer, num_of_words);

        first_size = DeviceDbSerialiser_GetPddFrameSize(buffer);
        offset_to_second = PS_SIZE_ADJ(first_size)*sizeof(uint16);
        second_size = DeviceDbSerialiser_GetPddFrameSize(&buffer[offset_to_second]);

        PsStore(DeviceDbSerialiser_GetAttributePsKey(device_order_first), buffer, PS_SIZE_ADJ(first_size));
        PsStore(DeviceDbSerialiser_GetAttributePsKey(device_order_second), &buffer[offset_to_second], PS_SIZE_ADJ(second_size));

        DEBUG_LOG_ALWAYS("DeviceDbSerialiser_RestoreBackup first buffer");
        DEBUG_LOG_DATA_ERROR(buffer, PS_SIZE_ADJ(first_size)*sizeof(uint16));

        DEBUG_LOG_ALWAYS("DeviceDbSerialiser_RestoreBackup second buffer");
        DEBUG_LOG_DATA_ERROR(&buffer[offset_to_second], PS_SIZE_ADJ(second_size)*sizeof(uint16));

        free(buffer);
    }
}
