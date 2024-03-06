/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Data type for describing the current BLE context of a handset device.
*/

#ifndef HANDSET_BLE_CONTEXT_H
#define HANDSET_BLE_CONTEXT_H

/*! \brief Describes the current BLE context of a handset device. */
typedef enum
{
    /*! Undefined or no context for the handset. */
    handset_ble_context_none = 0,

    /*! Handset was disconnected, either by the user, the application, or the handset. */
    handset_ble_context_disconnected,

    /*! Handset was disconnected due to link loss. */
    handset_ble_context_link_loss,

    /*! Handset was disconnected due to link loss while streaming */
    handset_ble_context_link_loss_streaming,

    /*! Handset was connected. */
    handset_ble_context_connected,
} handset_ble_context_t;

#endif // HANDSET_BLE_CONTEXT_H
