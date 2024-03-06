/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_watchdog.h
\brief      Provide watchdog support.
*/

#ifndef HEADSET_WATCHDOG_H
#define HEADSET_WATCHDOG_H

#ifdef INCLUDE_WATCHDOG

#include <message.h>

/*! Initialize watchdog component
 *
 * \param init_task Unused
 * \return always returns TRUE
*/
bool AppWatchdog_Init(Task init_task);

/*! \brief Routine to start watchdog timer
 * \return TRUE if operation is successful, otherwise False */
bool AppWatchdog_Start(void);

/*! \brief Routine to stop watchdog timer
 * \return TRUE if operation is successful, otherwise False */
bool AppWatchdog_Stop(void);
#endif /* INCLUDE_WATCHDOG */

#endif // HEADSET_WATCHDOG_H
