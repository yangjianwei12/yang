/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private header for the application state machine
*/

#ifndef USB_DONGLE_SM_PRIVATE_H
#define USB_DONGLE_SM_PRIVATE_H

/* Delay in ms before we rescan audio inputs,
 * usually following changes in the USB audio interface */
#define USB_DONGLE_SM_RESCAN_INPUTS_DELAY   (50)

/* Delay in ms before we restart the audio graph after a audio config change. Recommended to
   keep this same as USB_DONGLE_SM_RESCAN_INPUTS_DELAY */
#define USB_DONGLE_SM_RESTART_GRAPH_DELAY   (50)

/*! \brief Application state machine internal message IDs */
enum sm_internal_message_ids
{
    SM_INTERNAL_DELETE_PAIRED_DEVICES,  /*!< Delete all paired devices */
    SM_INTERNAL_FACTORY_RESET,          /*!< Reset device to factory defaults */
    SM_INTERNAL_PAIRING_ACL_COMPLETE,   /*!< Internal pairing ACL done/failed */
    SM_INTERNAL_RESCAN_AUDIO_INPUTS,    /*!< Rescan audio inputs */
    SM_INTERNAL_AUDIO_MODE_TOGGLE,      /*!< Cyclic toggle of audio mode (See \ref usb_dongle_audio_mode_t for various modes)*/

    SM_INTERNAL_NEW_MODE_SWITCH,        /*!< Act on new mode */
    SM_INTERNAL_AUDIO_GRAPH_RESTART,    /*!< Restart the audio graph if required */
};

#endif /* USB_DONGLE_SM_PRIVATE_H */
