/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_leds_config_table.h
\brief      Header file for the charger case led config table.
*/

#ifndef CHARGER_CASE_LEDS_CONFIG_TABLE_H
#define CHARGER_CASE_LEDS_CONFIG_TABLE_H

#include <csrtypes.h>
#include <ui_indicator_leds.h>

extern const ui_event_indicator_table_t charger_case_ui_leds_table[];
extern const ui_provider_context_consumer_indicator_table_t charger_case_ui_leds_context_indications_table[];
uint8 ChargerCaseLedsConfigTable_GetSize(void);
uint8 ChargerCaseLedsConfigTable_ContextsTableGetSize(void);
#endif // CHARGER_CASE_LEDS_CONFIG_TABLE_H
