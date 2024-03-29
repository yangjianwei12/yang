/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header for Earbud Tones UI Indicator configuration
*/
#ifndef EARBUD_TONES_CONFIG_TABLE_H
#define EARBUD_TONES_CONFIG_TABLE_H

#include <csrtypes.h>
#include <ui_indicator_tones.h>

extern const ui_event_indicator_table_t app_ui_tones_table[];
extern const ui_repeating_indication_table_t app_ui_repeating_tones_table[];

uint8 AppTonesConfigTable_SingleGetSize(void);
uint8 AppTonesConfigTable_RepeatingGetSize(void);

#endif // EARBUD_TONES_CONFIG_TABLE_H
