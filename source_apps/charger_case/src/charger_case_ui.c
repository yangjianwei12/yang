/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       charger_case_ui.c
\brief      ChargerCase application User Interface Indications
*/

#include "charger_case_ui.h"
#include "ui.h"
#include "led_manager.h"
#include "logging.h"
#include "charger_case_leds_config_table.h"
#include <power_manager.h>
#include <ui_indicator_leds.h>

bool ChargerCaseUi_Init(Task init_task)
{
    UNUSED(init_task);

    UiLeds_SetLedConfiguration(
                charger_case_ui_leds_table,
                ChargerCaseLedsConfigTable_GetSize(),
                charger_case_ui_leds_context_indications_table,
                ChargerCaseLedsConfigTable_ContextsTableGetSize());

    return TRUE;
}
