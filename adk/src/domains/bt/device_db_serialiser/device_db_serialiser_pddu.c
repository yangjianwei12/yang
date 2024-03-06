/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      PDDU operations.

Internal module to be used only inside of device_db_serialiser component.

*/

#include "device_db_serialiser_pddu.h"

#include <panic.h>

static device_db_serialiser_registered_pddu_t *registered_pddu_list = NULL;

static uint8 num_registered_pddus = 0;

void DeviceDbSerialiser_PdduInit(void)
{
    registered_pddu_list = NULL;
    num_registered_pddus = 0;
}

uint8 DeviceDbSerialiser_GetNumOfRegisteredPddu(void)
{
    return num_registered_pddus;
}

void DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        pdd_type_t pddu_id,
        get_persistent_device_data_len get_len,
        serialise_persistent_device_data ser,
        deserialise_persistent_device_data deser)
{
    PanicFalse(get_len);
    PanicFalse(ser);
    PanicFalse(deser);

    if (!registered_pddu_list)
    {
       registered_pddu_list = (device_db_serialiser_registered_pddu_t*)PanicUnlessMalloc(sizeof(device_db_serialiser_registered_pddu_t));
    }
    else
    {
        registered_pddu_list = realloc(registered_pddu_list, sizeof(device_db_serialiser_registered_pddu_t)*(num_registered_pddus + 1));
        PanicNull(registered_pddu_list);
    }

    registered_pddu_list[num_registered_pddus].id = pddu_id;
    registered_pddu_list[num_registered_pddus].get_len = get_len;
    registered_pddu_list[num_registered_pddus].ser = ser;
    registered_pddu_list[num_registered_pddus].deser = deser;

    num_registered_pddus++;
}

pdd_size_t * DeviceDbSerialiser_GetAllPdduFrameSizes(device_t device)
{
    pdd_size_t pddu_size = 0;
    pdd_size_t *pddu_frame_sizes = PanicUnlessMalloc(num_registered_pddus * sizeof(*pddu_frame_sizes));

    for (int i=0; i<num_registered_pddus; i++)
    {
        pddu_size = registered_pddu_list[i].get_len(device);
        if (pddu_size)
            pddu_size = DeviceBdSerialiser_ConvertPddPayloadSizeToFrameSize(pddu_size);
        pddu_frame_sizes[i] = pddu_size;
    }

    return pddu_frame_sizes;
}

pdd_size_t DeviceDbSerialiser_SumAllPdduFrameSizes(const pdd_size_t *pddu_frame_sizes)
{
    unsigned i;
    pdd_size_t sum = 0;

    for(i=0; i<num_registered_pddus; i++)
    {
        sum = DeviceDbSerialiser_AddPddSizes(sum, pddu_frame_sizes[i]);
    }

    return sum;
}

inline static void deviceDbSerialiser_addPdduData(device_t device, device_db_serialiser_registered_pddu_t *pddu, uint8 *pddu_frame, pdd_size_t pddu_frame_size)
{
    DeviceDbSerialiser_InitPddFrameHeader(pddu_frame, pddu->id, pddu_frame_size);
    pddu->ser(device, DeviceDbSerialiser_GetPddFramePayload(pddu_frame), 0);
}

void DeviceDbSerialiser_PopulatePddPayloadWithPdduFrames(device_t device, uint8 *pdd_frame, const pdd_size_t *pddu_frame_sizes)
{
    pdd_size_t offset = 0;
    uint8 * payload = DeviceDbSerialiser_GetPddFramePayload(pdd_frame);

    for (int i=0; i<num_registered_pddus; i++)
    {
        device_db_serialiser_registered_pddu_t *curr_pddu = &registered_pddu_list[i];

        if (pddu_frame_sizes[i])
        {
            deviceDbSerialiser_addPdduData(device, curr_pddu, &payload[offset], pddu_frame_sizes[i]);
            offset += pddu_frame_sizes[i];
        }
    }
}

device_db_serialiser_registered_pddu_t *DeviceDbSerialiser_GetRegisteredPddu(pdd_type_t id)
{
    device_db_serialiser_registered_pddu_t * pddu = NULL;
    if (registered_pddu_list && num_registered_pddus)
    {
        for (int i=0; i<num_registered_pddus; i++)
        {
            if (registered_pddu_list[i].id == id)
            {
                pddu = &registered_pddu_list[i];
                break;
            }
        }
    }
    return pddu;
}
