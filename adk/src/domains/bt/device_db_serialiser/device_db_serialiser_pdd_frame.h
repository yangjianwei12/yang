/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup device_database_serialiser
\brief      PRIVATE HEADER: Utilities to create and manage PDD frames.
            PDD frame for this API is any block of continuous memory that starts with a PDD frame header followed by its payload.
            The module provides APIs to get the size required for a frame as well as initialise its header (user doesn't need to know about PDD frame header format).
            This module is agnostic to the payload format itself.

*/

#ifndef DEVICE_DB_SERIALISER_PDD_FRAME_H_
#define DEVICE_DB_SERIALISER_PDD_FRAME_H_

#include "device_db_serialiser.h"

/*! \brief Populate the header part of the block of memory used for the PDD frame.
    \param frame Start of memory block used for the PDD frame.
    \param type  The PDD frame type.
    \param frame_size The size of the entire frame.
*/
void DeviceDbSerialiser_InitPddFrameHeader(uint8 *frame, pdd_type_t type, pdd_size_t frame_size);

/*! \brief Checks if this PDD frame is valid.
           If valid none of the calls to get information from this frame (in this API) will assert (they will assert otherwise).
    \param frame Start of memory block used for the PDD frame.
    \return TRUE if valid, FALSE otherwise.
*/
bool DeviceDbSerialiser_IsValidPddFrame(const uint8 *frame);

/*! \brief Get the size of the entire PDD frame.
    \param frame Start of memory block used for the PDD frame.
    \return The frame size.
*/
pdd_size_t DeviceDbSerialiser_GetPddFrameSize(const uint8 *frame);

/*! \brief Get the size of the payload of this PDD frame.
    \param frame Start of memory block used for the PDD frame.
    \return The payload size.
*/
pdd_size_t DeviceDbSerialiser_GetPddPayloadSize(const uint8 *frame);

/*! \brief Get the PDD frame type.
    \param frame Start of memory block used for the PDD frame.
    \return The type.
*/
pdd_type_t DeviceDbSerialiser_GetPddFrameType(const uint8 *frame);

/*! \brief Get the start of memory block used for the payload of this PDD frame.
    \param frame Start of memory block used for the PDD frame.
    \return Start of memory block used for the payload.
*/
uint8 * DeviceDbSerialiser_GetPddFramePayload(uint8 *frame);

/*! \brief Convert the payload size required to the frame size required.
           More specifically used to find the size required to store a payload in a PDD frame format.
    \param payload_size The size of the payload required.
    \return The total size of the frame required.
*/
pdd_size_t DeviceBdSerialiser_ConvertPddPayloadSizeToFrameSize(pdd_size_t payload_size);

/*! \brief Used to add two pdd_size_t without any overflows.
           In case overflow is detected it will panic.
    \param pdd_size_1 The first size.
    \param pdd_size_2 The second size.
    \return The sum.
*/
pdd_size_t DeviceDbSerialiser_AddPddSizes(pdd_size_t pdd_size_1, pdd_size_t pdd_size_2);

#endif /* DEVICE_DB_SERIALISER_PDD_FRAME_H_ */
