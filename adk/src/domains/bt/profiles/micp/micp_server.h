/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   micp_server  MICP Server
    @{
    \ingroup    profiles
    \brief      Header file for MICP Server profile
*/

#ifndef MICP_SERVER_H_
#define MICP_SERVER_H_

#include "task_list.h"
#include "domain_message.h"
#include "bt_types.h"

/*! \brief Events sent by MICP to other modules. */
typedef enum
{
    /*! Event to indicate the mute state change */
    MICP_SERVER_MUTE_IND = MICP_SERVER_MESSAGE_BASE,
} micp_server_msg_t;

typedef struct
{
    /*! Mute state */
    uint8 mute_state;
} MICP_SERVER_MUTE_IND_T;

/*! \brief Initialize the MICP profile. This function initializes the gatt mics server.
 */
void MicpServer_Init(void);

/*! \brief Function to register for the Persistent storage.
 */
void MicpServer_RegisterAsPersistentDeviceDataUser(void);

/*! \brief Function to update the MIC state.
 */
void MicpServer_SetMicState(uint8 mic_state);

/*! \brief Return the current MIC state
 */
uint8 MicpServer_GetMicState(void);

/*! \brief Toggles the MIC mute
 */
void MicpServer_ToggleMicMute(void);

/*! \brief Register a Task to receive notifications from MICP server Module.

     Once registered, #client_task will receive MICP client messages

    \param client_task Task to register to receive MICP server messages
*/
void MicpServer_ClientRegister(Task client_task);

/*! \brief Un-register a Task to receive notifications from MICP server Module.

     If the task is not currently registered then nothing will be changed.

    \param client_task Task to Un-register to receive MICP server messages
*/
void MicpServer_ClientUnregister(Task client_task);

#endif /* MICP_SERVER_H_ */

/*! @} */