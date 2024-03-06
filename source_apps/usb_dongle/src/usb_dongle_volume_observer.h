/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application volume observer
*/

#ifndef USB_DONGLE_VOLUME_OBSERVER_H
#define USB_DONGLE_VOLUME_OBSERVER_H

#if defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE)
/*! \brief Initialise the volume observer
    \param init_task Unused
 */
bool UsbDongle_VolumeObserverInit(Task init_task);

/*! \brief Sets the absolute sink volume
    \param volume The absolute volume that has to be applied.
*/
void UsbDongle_VolumeObserverSetAbsoluteSinkVolume(uint8 volume);
#endif /* defined(INCLUDE_SOURCE_APP_BREDR_AUDIO) || defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) */

#endif /* USB_DONGLE_VOLUME_OBSERVER_H */
