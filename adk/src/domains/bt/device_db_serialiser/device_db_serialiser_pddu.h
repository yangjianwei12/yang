/*!
\copyright  Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup device_database_serialiser
\brief      PDDU operations.
 
Internal module to be used only inside of device_db_serialiser component.
 
*/

#ifndef DEVICE_DB_SERIALISER_PDDU_H_
#define DEVICE_DB_SERIALISER_PDDU_H_

#include "device_db_serialiser.h"
#include "device_db_serialiser_pdd_frame.h"

/*! @{ */

#define DBS_PDD_FRAME_TYPE      0xDB

/*! \brief Stores the function pointers for a registered PDDU */
typedef struct
{
    pdd_type_t id;
    get_persistent_device_data_len get_len;
    serialise_persistent_device_data ser;
    deserialise_persistent_device_data deser;
	
} device_db_serialiser_registered_pddu_t;

/*! \brief  Init the PDDU module.
*/
void DeviceDbSerialiser_PdduInit(void);

/*! \brief  Get the number of registered PDDUs.

    \return The number of registered PDDUs.
*/
uint8 DeviceDbSerialiser_GetNumOfRegisteredPddu(void);

/*  \brief Retrieve the PDDU callbacks by id.

    \param id Id of PDDU.

    \return The structure containing PDDU callbacks.
*/
device_db_serialiser_registered_pddu_t *DeviceDbSerialiser_GetRegisteredPddu(pdd_type_t id);

/*  \brief Get a list of PDDU frame sizes for a given device.

    \param The device handle.

    \return Array of PDDU frame sizes (dynamically allocated, must be freed).
*/
pdd_size_t *DeviceDbSerialiser_GetAllPdduFrameSizes(device_t device);

/*  \brief Get total size for all PDDU frames (this is essentially the PDD payload size).

    \param pddu_frame_sizes Obtained from DeviceDbSerialiser_GetAllPdduFrameSizes().

    \return Total size for all PDDU frames.
*/
pdd_size_t DeviceDbSerialiser_SumAllPdduFrameSizes(const pdd_size_t *pddu_frame_sizes);

/*  \brief Populate PDD frame with PDDU frames for a given device.

    \param device       The device handle.

    \param pdd_frame    PDD frame buffer.

    \param pddu_frame_sizes Obtained from DeviceDbSerialiser_GetAllPdduFrameSizes().
*/
void DeviceDbSerialiser_PopulatePddPayloadWithPdduFrames(device_t device, uint8 *pdd_frame, const pdd_size_t *pddu_frame_sizes);

/*! @} */

#endif /* DEVICE_DB_SERIALISER_PDDU_H_ */
