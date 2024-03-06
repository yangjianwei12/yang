/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Private defines and types for TMAP broadcast source
    @{
*/

#ifndef TMAP_CLIENT_SOURCE_BROADCAST_PRIVATE_H_
#define TMAP_CLIENT_SOURCE_BROADCAST_PRIVATE_H_

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE

#include "tmap_client_source_broadcast.h"

/*! \brief Tmap broadcast source related parameters */
typedef struct
{
    /*! Length of broadcast source name */
    uint8           broadcast_source_name_len;

    /*! Broadcast source name */
    const uint8*    broadcast_source_name;

    /*! Broadcast code that set for the broadcast source role */
    uint8*          broadcast_code;

    /*! Encryption status for broadcast streaming */
    bool            encryption;
} tmap_client_source_broadcast_src_params_t;

typedef struct
{
    /*! Handle for scanning broadcast source for assistant */
    uint16  scan_handle;
} tmap_client_source_broadcast_asst_params_t;

/*! \brief Tmap broadcast related data */
typedef struct
{
    /*! TMAP broadcast callback handler */
    tmap_client_source_broadcast_callback_handler_t callback_handler;

    /*! TMAP broadcast source handle */
    TmapClientProfileHandle                         handle;

    /*! TMAP broadcast Source parameters */
    tmap_client_source_broadcast_src_params_t       source_params;

    /*! TMAP broadcast assistant parameters */
    tmap_client_source_broadcast_asst_params_t      asst_params;
} tmap_src_bcast_task_data_t;

/* Invalid Broadcast Profile Handle*/
#define TMAP_CLIENT_SOURCE_INVALID_BROADCAST_HANDLE    ((ServiceHandle) (0x0000))

/*! \brief Handler to process broadcast related messages received from TMAP library */
void tmapClientSourceBroadcast_HandleTmapMessage(Message message);

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
#endif /* TMAP_CLIENT_SOURCE_BROADCAST_PRIVATE_H_ */
/*! @} */