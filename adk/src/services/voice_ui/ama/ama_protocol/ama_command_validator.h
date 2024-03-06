/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       ama_command_validator.h
    \addtogroup ama_protocol
    @{
    \brief  Definition of the APIs to validate AMA commands from the phone
*/

#ifndef AMA_COMMAND_VALIDATOR_H
#define AMA_COMMAND_VALIDATOR_H

#include "accessories.pb-c.h"
#include "csrtypes.h"

#define PAYLOAD_NOT_USED 0xFFFF

typedef struct
{
    Command cmd;
    ControlEnvelope__PayloadCase supported_payload_case;
}const cmd_payload_case_map_t;

/*! \brief Checks if command received from phone is valid
 *  \param cmd The command ID
 *  \param payload_case The payload case
 *  \param payload Pointer to the relevant union member
 *  \param cmd_payload_cases Pointer to a map of command IDs to supported payload cases
 *  \param cmd_payload_cases_size Size of the map of commands IDs to supported payload cases
 *  \return TRUE if command is valid, otherwise FALSE
 */
bool AmaProtocol_IsValidCommand(Command cmd, ControlEnvelope__PayloadCase payload_case, void * payload,
                                cmd_payload_case_map_t * cmd_payload_cases, uint16 cmd_payload_cases_size);


#endif // AMA_COMMAND_VALIDATOR_H
/*! @} */