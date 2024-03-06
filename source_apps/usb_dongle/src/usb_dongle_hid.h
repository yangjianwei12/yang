/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for USB dongle HID interface

*/

#ifndef USB_DONGLE_HID_H
#define USB_DONGLE_HID_H

/*! \brief Initializes the USB Dongle HID interface.

    \param init_task Task to receive responses.
    \return TRUE if able to initialize, returns FALSE otherwise.
*/
bool UsbDongleHid_Init(Task task);

#endif /* USB_DONGLE_HID_H */
