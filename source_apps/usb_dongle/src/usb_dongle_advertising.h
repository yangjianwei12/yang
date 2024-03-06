/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for advertising module
*/

#ifndef USB_DONGLE_ADVERTISING_H
#define USB_DONGLE_ADVERTISING_H

#include <message.h>

/*! \brief Initialise the advertising module

    This function will register the necessary parameters for BRDER page
    scanning and configure it to accept connections
    
*/
bool UsbDongleAdvertising_Init(Task init_task);

#endif // USB_DONGLE_ADVERTISING_H
