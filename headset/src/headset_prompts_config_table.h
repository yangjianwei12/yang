/*!
\copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_prompts_config_table.c
\brief      Headset prompts configuration table module
*/
#ifndef HEADSET_PROMPTS_CONFIG_TABLE_H
#define HEADSET_PROMPTS_CONFIG_TABLE_H
#include <csrtypes.h>
#include <ui_indicator_prompts.h>

extern const ui_event_indicator_table_t app_ui_prompts_table[];

uint8 AppPromptsConfigTable_GetSize(void);

#endif // HEADSET_PROMPTS_CONFIG_TABLE_H
