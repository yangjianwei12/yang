/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_ui.h
\brief      Header file for the application User Interface
*/
#ifndef CHARGER_CASE_UI_H
#define CHARGER_CASE_UI_H

#include "domain_message.h"
#include "charger_case_led.h"

/*! brief Initialise indicator module */
bool ChargerCaseUi_Init(Task init_task);

#endif // CHARGER_CASE_UI_H
