/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_ui.h
\brief      Header file for the application User Interface
*/
#ifndef USB_DONGLE_UI_H
#define USB_DONGLE_UI_H

#include "domain_message.h"
#include "usb_dongle_led.h"

/*! brief Initialise indicator module */
bool UsbDongleUi_Init(Task init_task);

#endif // USB_DONGLE_UI_H
