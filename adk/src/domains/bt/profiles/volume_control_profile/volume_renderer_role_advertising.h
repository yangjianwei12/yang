/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup volume_profile
    \brief      LE advertising data required for the volume renderer role of VCS.
    @{
*/

#ifndef VOLUME_RENDERER_ROLE_ADVERTISING_H
#define VOLUME_RENDERER_ROLE_ADVERTISING_H

/*! \brief Setup the LE advertising data for the VCS volume renderer role

    \returns TRUE if the advertising data was setup successfully, else FALSE
 */
bool VolumeRenderer_SetupLeAdvertisingData(void);

#endif /* VOLUME_RENDERER_ROLE_ADVERTISING_H */

/*! @} */