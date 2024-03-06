/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header for Inquiry Manager setup
*/

#ifndef USB_DONGLE_INQUIRY_H
#define USB_DONGLE_INQUIRY_H

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
/*! \brief Initialise the Inquiry Manager

    This function will register the necessary parameters for BRDER Inquiry Scanning

    \param init_task Not used
    \return TRUE
*/
bool UsbDongleInquiry_Init(Task init_task);
#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#endif /* USB_DONGLE_INQUIRY_H */
