/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Charger Case application DFU API.
*/

#ifndef CHARGER_CASE_DFU_H
#define CHARGER_CASE_DFU_H

#ifdef INCLUDE_DFU

#include <message.h>

/*! \brief Initialise the application DFU module. */
bool ChargerCaseDfu_Init(Task init_task);

/*! \brief Enable DFU handset pairing for a limited time. */
void ChargerCaseDfu_HandsetPairingStart(void);

/*! \brief Disable DFU handset pairing mode. */
void ChargerCaseDfu_HandsetPairingCancel(void);

/*! \brief Check if an upgrade is currently in progress.
    \returns TRUE if upgrade in progress or handset pairing active.
*/
bool ChargerCaseDfu_UpgradeInProgress(void);

#else
#define ChargerCaseDfu_Init(init_task) (TRUE)
#define ChargerCaseDfu_HandsetPairingStart() ((void)0)
#define ChargerCaseDfu_HandsetPairingCancel() ((void)0)
#define ChargerCaseDfu_UpgradeInProgress() (FALSE)
#endif /* INCLUDE_DFU */

#endif /* CHARGER_CASE_DFU_H */
