/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   volume_profile Volume Control Profile
    @{
    \ingroup    profiles
    \brief      Interface to the volume renderer role of the volume control profile

                The Volume Control Service (VCS) and Volume Control Profile (VCP) 
                are GATT features for volume control. Output devices should implement
                this in the Volume Renderer Role.
*/

#ifndef VOLUME_RENDERER_ROLE_H_
#define VOLUME_RENDERER_ROLE_H_

#include "bt_types.h"
#include "service_handle.h"

/*! \brief Definition of data required for the initialisation
 *         of the Volume Renderer.
 */
typedef struct
{
    uint8                 volume_setting;
    uint8                 mute;
    uint8                 step_size;
} volume_renderer_init_t;

/*! \brief Data sent with the VolumeRenderer_VolumeChanged callback.
 */
typedef struct
{
    uint8                 volume_setting;
    uint8                 mute;
    gatt_cid_t            cid;
} volume_renderer_volume_changed_t;

/*! \brief Callback interface

    Implemented and registered by the application to receive and influence VCS behaviour
 */
typedef struct
{
    /*! \brief Called to retrieve a VCS client config
    */
    void (*VolumeRenderer_VolumeChanged)(volume_renderer_volume_changed_t * volume);
    /*! \brief Called to retrieve a VCS client config
    */
    void * (*VolumeRenderer_RetrieveClientConfig)(gatt_cid_t cid);
    /*! \brief Called to store an VCS client config
    */
    void (*VolumeRenderer_StoreClientConfig)(gatt_cid_t cid, void * config, uint8 size);
} volume_renderer_callback_interface_t;

/*! \brief Initialises the Volume Renderer role

    Initialises the VCS and profile.

    \param init_params Initialisation parameters. Should not be NULL.
    \param callbacks_to_register VolumeRenderer_callback_interface_t to register
 */
void VolumeRenderer_Init(const volume_renderer_init_t * init_params, const volume_renderer_callback_interface_t * callbacks_to_register);

/*!
    @brief Gets the current volume.

    @return uint8 Volume value.
*/
uint8 VolumeRenderer_GetVolume(void);

/*!
    @brief Sets the current volume.

    @param volume Value of Volume Setting to set
    
    @return TRUE if successful, FALSE otherwise
*/
bool VolumeRenderer_SetVolume(uint8 volume);

/*!
    @brief Gets the current mute.

    @return uint8 Mute value.
*/
uint8 VolumeRenderer_GetMute(void);

/*!
    @brief Sets the current mute.

    @param mute Value of Mute Setting to set
    
    @return TRUE if successful, FALSE otherwise
*/
bool VolumeRenderer_SetMute(uint8 mute);

/*!
    @brief Get the vcs handle.
*/
#define VolumeRenderer_GetVcsHandle() (vcs_handle)

#ifndef HOSTED_TEST_ENVIRONMENT
void VolumeRenderer_SimulateVcsClientVolumeChange(uint8 volume, uint8 mute);
#endif

extern ServiceHandle vcs_handle;

#endif /* VOLUME_RENDERER_ROLE_H_ */
/* @} */