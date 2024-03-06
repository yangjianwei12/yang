/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines and types for TMAP unicast client
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_UNICAST_PRIVATE_H_
#define TMAP_CLIENT_SOURCE_UNICAST_PRIVATE_H_

#ifdef INCLUDE_LE_AUDIO_UNICAST_SOURCE
#include "tmap_client_source_unicast.h"

/*! \brief Tmap client context. */
typedef struct
{
    /*! TMAP Cig ID */
    uint8 cig_id;

    /*! TMAP Client profile callback handler */
    tmap_client_source_unicast_callback_handler_t callback_handler;
} tmap_src_unicast_task_data_t;

/*! \brief Handler that process the unicast related messages alone */
void tmapClientSource_HandleTmapUnicastMessage(Message message);

#endif /* INCLUDE_LE_AUDIO_UNICAST_SOURCE */
#endif /* TMAP_CLIENT_SOURCE_UNICAST_PRIVATE_H_ */
/*! @} */