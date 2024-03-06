/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Earbud Prompts UI Indicator configuration table
*/
#include "earbud_prompts_config_table.h"

#include <domain_message.h>
#include <ui_indicator_prompts.h>

#include <av.h>
#include <pairing.h>
#include <handset_service_protected.h>
#include <power_manager.h>
#include <voice_ui.h>

/*! \ingroup configuration

   \brief System events to voice prompts configuration table

   This is an ordered table that associates system events with voice prompt files.
*/
#ifdef INCLUDE_PROMPTS
const ui_event_indicator_table_t app_ui_prompts_table[] =
{
    {.sys_event=PAIRING_ACTIVE,         { .prompt.filename = PROMPT_FILENAME("pairing"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE }},
    {.sys_event=PAIRING_COMPLETE,       { .prompt.filename = PROMPT_FILENAME("pairing_successful"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE }},
    {.sys_event=PAIRING_FAILED,         { .prompt.filename = PROMPT_FILENAME("pairing_failed"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE }},
#ifndef EXCLUDE_POWER_PROMPTS
    {.sys_event=POWER_ON,                {.prompt.filename = PROMPT_FILENAME("power_on"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE,
                                          .prompt.local_feedback = TRUE }},
    {.sys_event=POWER_OFF,              { .prompt.filename = PROMPT_FILENAME("power_off"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE,
                                          .prompt.local_feedback = TRUE },
                                          .await_indication_completion = TRUE },
#endif
#ifndef EXCLUDE_CONN_PROMPTS
    {.sys_event=HANDSET_SERVICE_FIRST_TRANSPORT_CONNECTED_IND,  { .prompt.filename = PROMPT_FILENAME("connected"),
                                                                  .prompt.rate = 48000,
                                                                  .prompt.format = PROMPT_FORMAT,
                                                                  .prompt.interruptible = FALSE,
                                                                  .prompt.queueable = TRUE,
                                                                  .prompt.requires_repeat_delay = FALSE }},
    {.sys_event=HANDSET_SERVICE_ALL_TRANSPORTS_DISCONNECTED_IND,{ .prompt.filename = PROMPT_FILENAME("disconnected"),
                                                                  .prompt.rate = 48000,
                                                                  .prompt.format = PROMPT_FORMAT,
                                                                  .prompt.interruptible = FALSE,
                                                                  .prompt.queueable = TRUE,
                                                                  .prompt.requires_repeat_delay = FALSE }},
#endif
#ifdef INCLUDE_GAA
    {.sys_event=VOICE_UI_MIC_OPEN,      { .prompt.filename = PROMPT_FILENAME("mic_open"),
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = TRUE,
                                          .prompt.queueable = FALSE,
                                          .prompt.requires_repeat_delay = FALSE }},
    {.sys_event=VOICE_UI_MIC_CLOSE,     { .prompt.filename = PROMPT_FILENAME("mic_close"),
                                          .prompt.rate = 16000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = TRUE,
                                          .prompt.queueable = FALSE,
                                          .prompt.requires_repeat_delay = FALSE }},
    {.sys_event=VOICE_UI_DISCONNECTED,  { .prompt.filename = PROMPT_FILENAME("bt_va_not_connected"),
                                          .prompt.rate = 48000,
                                          .prompt.format = PROMPT_FORMAT,
                                          .prompt.interruptible = FALSE,
                                          .prompt.queueable = TRUE,
                                          .prompt.requires_repeat_delay = TRUE }}
#endif /* INCLUDE_GAA */
};
#endif

uint8 AppPromptsConfigTable_GetSize(void)
{
#ifdef INCLUDE_PROMPTS
    return ARRAY_DIM(app_ui_prompts_table);
#else
    return 0;
#endif
}

