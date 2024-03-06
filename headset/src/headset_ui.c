/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_ui.c
\brief      Headset application User Interface Indications
*/

#include "headset_ui.h"
#include "ui.h"
#include "led_manager.h"
#include "logging.h"
#include "headset_sm.h"
#include "headset_leds_config_table.h"
#include "headset_prompts_config_table.h"
#include "headset_tones_config_table.h"
#include <power_manager.h>
#include <ui_indicator_prompts.h>
#include <ui_indicator_tones.h>

bool AppUi_Init(Task init_task)
{
    UNUSED(init_task);
  
    UiPrompts_SetPromptPlaybackEnabled(TRUE);
    UiTones_SetTonePlaybackEnabled(TRUE);

#ifdef INCLUDE_PROMPTS
    UiPrompts_SetPromptConfiguration(
                app_ui_prompts_table,
                AppPromptsConfigTable_GetSize());
#endif

#if INCLUDE_TONES
    UiTones_SetToneConfiguration(
                app_ui_tones_table,
                AppTonesConfigTable_SingleGetSize(),
                app_ui_repeating_tones_table,
                AppTonesConfigTable_RepeatingGetSize());
#endif

    UiLeds_SetLedConfiguration(
                app_ui_leds_table,
                AppLedsConfigTable_EventsTableGetSize(),
                app_ui_leds_context_indications_table,
                AppLedsConfigTable_ContextsTableGetSize());

    return TRUE;
}
