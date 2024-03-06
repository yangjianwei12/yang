/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       ama_command_validator.c
    \ingroup    ama_protocol
    \brief  Implementation of the APIs to validate AMA commands from the phone
*/

#ifdef INCLUDE_AMA

#include "ama_command_validator.h"

static bool amaProtocol_IsCommandPayloadCaseSupported(Command cmd, ControlEnvelope__PayloadCase payload_case,
                                                      cmd_payload_case_map_t * cmd_payload_cases, uint16 cmd_payload_cases_size)
{
    bool payload_case_supported = FALSE;
    unsigned i=0;

    for(; i<cmd_payload_cases_size;i++)
    {
        if((cmd_payload_cases[i].cmd == cmd) &&
          ((cmd_payload_cases[i].supported_payload_case == payload_case) || (cmd_payload_cases[i].supported_payload_case == PAYLOAD_NOT_USED)))
        {
            payload_case_supported = TRUE;
            break;
        }
    }

    return payload_case_supported;
}

static bool amaProtocol_IsCommandPayloadValid(Command cmd, void * payload)
{
    bool is_valid_payload = (payload == NULL) ? FALSE : TRUE;

    if(is_valid_payload)
    {
        switch(cmd)
        {
            case COMMAND__SET_LOCALE:
            {
                SetLocale * set_locale = (SetLocale *)payload;
                is_valid_payload = set_locale->locale == NULL ? FALSE : TRUE;
            }
            break;

            case COMMAND__SET_STATE:
            {
                SetState * set_state = (SetState *)payload;
                is_valid_payload = set_state->state == NULL ? FALSE : TRUE;
            }
            break;

            default:
                break;
        }
    }

    return is_valid_payload;
}

bool AmaProtocol_IsValidCommand(Command cmd, ControlEnvelope__PayloadCase payload_case, void * payload,
                                       cmd_payload_case_map_t * cmd_payload_cases, uint16 cmd_payload_cases_size)
{
    bool is_valid_command = FALSE;

    is_valid_command = amaProtocol_IsCommandPayloadCaseSupported(cmd, payload_case, cmd_payload_cases, cmd_payload_cases_size);

    if((payload_case != CONTROL_ENVELOPE__PAYLOAD__NOT_SET) && is_valid_command)
    {
        is_valid_command = amaProtocol_IsCommandPayloadValid(cmd, payload);
    }

    return is_valid_command;
}

#endif /* INCLUDE_AMA */
