/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB Dongle application DFU API.
*/

#ifndef USB_DONGLE_DFU_H
#define USB_DONGLE_DFU_H

#ifdef INCLUDE_DFU

#include <message.h>

/*! \brief Initialise the application DFU module. */
bool UsbDongleDfu_Init(Task init_task);

/*! \brief Check if an upgrade is currently in progress.
    \returns TRUE if upgrade in progress.
*/
bool UsbDongleDfu_UpgradeInProgress(void);

#else
#define UsbDongleDfu_Init(init_task) (TRUE)
#define UsbDongleDfu_UpgradeInProgress() (FALSE)
#endif /* INCLUDE_DFU */

#endif /* USB_DONGLE_DFU_H */
