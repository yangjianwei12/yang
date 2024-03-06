/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private defines and functions for GATT QSS server
*/

#ifndef GATT_SERVER_QSS_PRIVATE_H
#define GATT_SERVER_QSS_PRIVATE_H

#include "service_handle.h"
#include "task_list.h"
#include "bt_types.h"
#include "gatt_server_qss.h"

#define QSS_INVALID_SERVICE_HANDLE                   0x0000
#define GATT_SERVER_QSS_USER_DESCRIPTION_DEFAULT     "QSS support"
#define GATT_SERVER_QSS_USER_DESCRIPTION_LEN_DEFAULT 0x0B
#define GATT_SERVER_QSS_USER_DESCRIPTION_MAX_LEN     0x17
#define GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_ENABLE    0x01
#define GATT_SERVER_QSS_LOSSLESS_AUDIO_NTF_DISABLE   0x00
#define GATT_SERVER_QSS_SUPPORTED                    0x01
#define GATT_SERVER_QSS_NOT_SUPPORTED                0x00

/*! \brief qss server context. */
typedef struct
{
    /*! TRUE if lossless audio notification is enabled, else FALSE */
    bool          ntf_enable;

    /*! User description for GATT QSS Service */
    char          user_description[GATT_SERVER_QSS_USER_DESCRIPTION_MAX_LEN];

    /*! User description length */
    uint8         user_description_len;

    /*! qss service handle */
    ServiceHandle qss_service_handle;

    /*! Connection ID */
    gatt_cid_t    cid;

    /*! qss server task */
    TaskData      task_data;

    /*! Periodically updated lossless data */
    uint32        lossless_audio_data;

    /*! List of client tasks registered for notifications */
    task_list_t *client_tasks;
} gatt_server_qss_data_t;

extern gatt_server_qss_data_t gatt_server_qss_data;

/*! Returns the QSS Service Handle */
#define GattServerQss_GetQssHandle() gatt_server_qss_data.qss_service_handle

/*! Returns TRUE if lossless audio notifications are enabled by the client, else FALSE */
#define GattServerQss_IsLosslessAudioNotfnEnabled() gatt_server_qss_data.ntf_enable

/*! Reads QSS cliet config from PS store */
bool GattServerQss_ReadClientConfigFromStore(gatt_cid_t cid, uint16 *config);

void GattServerQss_SendUpdate(gatt_cid_t cid, gatt_qss_server_msg_t msg_id);

#endif /* GATT_SERVER_QSS_PRIVATE_H */

