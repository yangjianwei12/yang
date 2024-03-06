/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup pbp_client_source
    \brief      Private defines and types for PBP Client Source
    @{
*/

#ifndef PBP_CLIENT_SOURCE_PRIVATE_H_
#define PBP_CLIENT_SOURCE_PRIVATE_H_

#ifdef INCLUDE_LE_AUDIO_BROADCAST_SOURCE
#include <logging.h>
#include <panic.h>
#include "message_.h"
#include "gatt_connect.h"
#include "pbp_client_source.h"

/*! \brief PBP client context. */
typedef struct
{
    /*! PBP profile task */
    TaskData                             task_data;

    /*! PBP Client profile callback handler */
    pbp_client_source_callback_handler_t callback_handler;

    /*! PBP Client profile callback handler */
    PbpProfileHandle                     profile_handle;

    /*! PBP broadcast source handle */
    PbpProfileHandle                     bcast_handle;

    /*! PBP broadcast assistant PA scan handle */
    uint16                               scan_handle;
} pbp_client_source_task_data_t;

/* Invalid Broadcast Profile Handle*/
#define PBP_CLIENT_SOURCE_INVALID_HANDLE    ((ServiceHandle) (0x0000))

#endif /* INCLUDE_LE_AUDIO_BROADCAST_SOURCE */
#endif /* PBP_CLIENT_SOURCE_PRIVATE_H_ */
/*! @} */