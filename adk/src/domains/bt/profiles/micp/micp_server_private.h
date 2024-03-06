/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup micp_server
    \brief      Private defines and functions for MICP server
    @{
*/

#ifndef MICP_SERVER_PRIVATE_H
#define MICP_SERVER_PRIVATE_H

#include "service_handle.h"
#include "message.h"
#include "task_list.h"

#define MICS_INVALID_SERVICE_HANDLE     (0x0000)

/*! \brief micp server context. */
typedef struct
{
    /*! micp server task */
    TaskData task_data;

    /*! mics service handle */
    ServiceHandle mics_service_handle;

    /*! List of client tasks registered for notifications */
    task_list_t *client_tasks;
} micp_server_task_data_t;

extern micp_server_task_data_t micp_task_data;

/*! Returns the MICP Server task */
#define MicpServer_GetTask()         (&micp_task_data.task_data)

/*! Returns the MICS Service Handle */
#define MicpServer_GetMicsHandle()   micp_task_data.mics_service_handle

#endif /* MICP_SERVER_PRIVATE_H */

/*! @} */