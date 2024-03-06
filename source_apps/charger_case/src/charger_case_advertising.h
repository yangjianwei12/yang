/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for advertising module
*/

#ifndef CHARGER_CASE_ADVERTISING_H
#define CHARGER_CASE_ADVERTISING_H

#include <message.h>

/*! \brief Initialise the advertising module

    This function will register the necessary parameters for both BRDER page
    scanning and LE advertising, and configure them to accept connections /
    allow advertising as required.
*/
bool ChargerCaseAdvertising_Init(Task init_task);

/*! \brief Enable random Resolvable Private Addresses (RPA) for LE.

    Configure LE advertising to use random Resolvable Private Addresses.
*/
bool ChargerCaseAdvertising_EnableLePrivateAddresses(Task init_task);

#endif /* CHARGER_CASE_ADVERTISING_H */
