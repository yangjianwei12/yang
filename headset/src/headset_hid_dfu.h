/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Headset application HID DFU API.
*/

#ifndef HEADSET_DFU_H
#define HEADSET_DFU_H

#ifdef INCLUDE_HID_DFU

#include <message.h>

/*! \brief Initialise the application DFU module. */
bool headsetHIDDFU_Init(Task init_task);

/*! \brief Check if an upgrade is currently in progress.
    \returns TRUE if upgrade in progress.
*/
bool headsetDFU_UpgradeInProgress(void);

#else
#define headsetHIDDFU_Init(init_task) (TRUE)
#define headsetDFU_UpgradeInProgress() (FALSE)
#endif /* INCLUDE_HID_DFU */

#endif /* HEADSET_DFU_H */
