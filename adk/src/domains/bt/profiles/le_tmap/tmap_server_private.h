/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines and functions for TMAP server
    @{
*/

#ifndef TMAP_SERVER_PRIVATE_H
#define TMAP_SERVER_PRIVATE_H

#include "bt_types.h"

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
/*! \brief Initialise MCS server and TBS server
    \param app_task Task to receive messages from MCS/TBS server
*/
void tmapServer_McsTbsServerInit(Task app_task);

/*! \brief Add configurations to MCS and TBS servers
    \param cid  GATT Connection identifier on which configuration
                needs to be done.
*/
void tmapServer_McsTbsServerAddConfig(gatt_cid_t cid);

/*! \brief Remove configurations of the MCS and TBS server
    \param cid  GATT Connection identifier on which configuration
                needs to be removed
*/
void tmapServer_McsTbsServerRemoveConfig(gatt_cid_t cid);

/*! \brief Process the messages received from MCS/TBS server
    \param id  Identifier of the synergy library message
    \param message  Message to process
*/
void tmapServer_ProcessMcsTbsServerMessage(MessageId id, Message message);
#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */

#endif /* TMAP_SERVER_PRIVATE_H */
/*! @} */