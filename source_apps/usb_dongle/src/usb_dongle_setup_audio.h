/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_setup_audio.h
\brief      Module configure audio chains for USB dongle application
*/
#ifndef HEADESET_SETUP_AUDIO_H
#define HEADESET_SETUP_AUDIO_H

/*! \brief Configure and Set the Audio chains for UsbDongle.
    \param void.
    \return TRUE if audio configuration is success.
*/

bool usb_dongleSetupAudio(void);

/*! \brief Configures downloadable capabilities.
    \param void.
    \return void
*/
void usb_dongleSetBundlesConfig(void);

#endif /* HEADESET_SETUP_AUDIO_H */
