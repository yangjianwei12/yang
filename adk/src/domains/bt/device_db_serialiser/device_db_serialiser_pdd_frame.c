/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Utility functions for pdd frames.

*/

#include "device_db_serialiser_pdd_frame.h"
#include <byte_utils.h>
#include <panic.h>

// Using -1 for an unsigned means use the maximum value for that type
#define MAX_PDD_SIZE ((pdd_size_t)-1)
// Originally the PDD frame header used a single byte for size, this was extended and an old size of zero indicates the extended size frame is being used
#define EXTENDED_SIZE_FORMAT (0)

typedef enum
{
    type_offset = 0,
    size_offset,
    old_header_size,
    extended_size_msb_offset = old_header_size,
    extended_size_lsb_offset,
    new_header_size
} format_descriptors_t;

inline static bool deviceDbSerialiser_IsOldFormat(const uint8 *frame)
{
    return frame[size_offset] != EXTENDED_SIZE_FORMAT;
}

static pdd_size_t deviceDbSerialiser_GetFrameSize(const uint8 *frame)
{
    if (deviceDbSerialiser_IsOldFormat(frame))
        return frame[size_offset];
    else
        return MAKEWORD_HI_LO(frame[extended_size_msb_offset], frame[extended_size_lsb_offset]);
}

static pdd_size_t deviceBdSerialiser_GetHeaderSize(const uint8 *frame)
{
    if (deviceDbSerialiser_IsOldFormat(frame))
        return old_header_size;
    else
        return new_header_size;
}

inline static void deviceDbSerialiser_AssertValidFrame(const uint8 *frame)
{
    PanicFalse(DeviceDbSerialiser_IsValidPddFrame(frame));
}

void DeviceDbSerialiser_InitPddFrameHeader(uint8 *frame, pdd_type_t type, pdd_size_t frame_size)
{
    frame[type_offset] = type;
    frame[size_offset] = EXTENDED_SIZE_FORMAT;
    frame[extended_size_msb_offset] = HIBYTE(frame_size);
    frame[extended_size_lsb_offset] = LOBYTE(frame_size);
}

bool DeviceDbSerialiser_IsValidPddFrame(const uint8 *frame)
{
    return deviceDbSerialiser_GetFrameSize(frame) >= deviceBdSerialiser_GetHeaderSize(frame);
}

pdd_size_t DeviceDbSerialiser_GetPddFrameSize(const uint8 *frame)
{
    deviceDbSerialiser_AssertValidFrame(frame);
    return deviceDbSerialiser_GetFrameSize(frame);
}

pdd_size_t DeviceDbSerialiser_GetPddPayloadSize(const uint8 *frame)
{
    deviceDbSerialiser_AssertValidFrame(frame);
    return DeviceDbSerialiser_GetPddFrameSize(frame) - deviceBdSerialiser_GetHeaderSize(frame);
}

pdd_type_t DeviceDbSerialiser_GetPddFrameType(const uint8 *frame)
{
    deviceDbSerialiser_AssertValidFrame(frame);
    return frame[type_offset];
}

uint8 * DeviceDbSerialiser_GetPddFramePayload(uint8 *frame)
{
    deviceDbSerialiser_AssertValidFrame(frame);
    return &frame[deviceBdSerialiser_GetHeaderSize(frame)];
}

pdd_size_t DeviceBdSerialiser_ConvertPddPayloadSizeToFrameSize(pdd_size_t payload_size)
{
    return DeviceDbSerialiser_AddPddSizes(payload_size, new_header_size);
}

pdd_size_t DeviceDbSerialiser_AddPddSizes(pdd_size_t pdd_size_1, pdd_size_t pdd_size_2)
{
    // Check for overflow
    PanicFalse(MAX_PDD_SIZE - pdd_size_2 > pdd_size_1);
    return pdd_size_1 + pdd_size_2;
}
