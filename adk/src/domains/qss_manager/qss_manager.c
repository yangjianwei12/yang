/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\ingroup    qss_manager_domain
\brief      Implementation of the QSS Manager.
*/

#ifdef INCLUDE_QSS

#include "qss_manager.h"
#include "logging.h"
#include "kymera.h"
#include "kymera_qss.h"
#include "gatt_server_qss.h"
#include "mirror_profile.h"
#include "multidevice.h"

#define QSS_MANAGER_POLL_LOSLESS_DATA_IND          0X01
#define QSS_MANAGER_LOSSLESS_DATA_POLL_INTERVAL_MS 10000

/*! \brief qss manager context. */
typedef struct
{
    /*! qss manager task */
    TaskData      task_data;
} qss_manager_data_t;

qss_manager_data_t qss_manager_data;

static void qssManager_MessageHandler(Task task, MessageId id, Message message);

static void qssManager_PollLosslessData(void)
{
    uint32 lossless_audio_data;

    if (KymeraQss_ReadAptxAdaptiveLosslesssInfo(&lossless_audio_data))
    {
        DEBUG_LOG("qssManager_PollLosslessData lossless_data:0x%x", lossless_audio_data);

        MessageSendLater(&qss_manager_data.task_data,
                         QSS_MANAGER_POLL_LOSLESS_DATA_IND,
                         NULL,
                         QSS_MANAGER_LOSSLESS_DATA_POLL_INTERVAL_MS);
        GattServerQss_UpdateLosslessAudiodata(lossless_audio_data);
    }
}

static void qssManager_HandeQssConfigUpdated(const GATT_QSS_SERVER_CONFIG_UPDATED_T *msg)
{
    MessageCancelAll(&qss_manager_data.task_data, QSS_MANAGER_POLL_LOSLESS_DATA_IND);

    if (msg->ntf_enabled)
    {
        QssManager_StartPeriodicLosslessDataPoll();
    }
}

static void qssManager_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case KYMERA_APTX_ADAPTIVE_STREAMING_IND:
            QssManager_StartPeriodicLosslessDataPoll();
        break;

        case QSS_MANAGER_POLL_LOSLESS_DATA_IND:
            qssManager_PollLosslessData();
        break;

        case GATT_QSS_SERVER_CONFIG_UPDATED:
            qssManager_HandeQssConfigUpdated(message);
        break;

        default:
            break;
    }
}

bool QssManager_Init(Task task)
{
    UNUSED(task);

    DEBUG_LOG("QssManager_Init");

    memset(&qss_manager_data, 0, sizeof(qss_manager_data));
    GattServerQss_Init();
    qss_manager_data.task_data.handler = qssManager_MessageHandler;
    Kymera_RegisterNotificationListener(&qss_manager_data.task_data);
    GattServerQss_ClientRegister(&qss_manager_data.task_data);
    return TRUE;
}

void QssManager_StartPeriodicLosslessDataPoll(void)
{
    DEBUG_LOG("qssManager_StartPeriodicLosslessDataPoll");

    MessageSendLater(&qss_manager_data.task_data,
                     QSS_MANAGER_POLL_LOSLESS_DATA_IND,
                     NULL,
                     QSS_MANAGER_LOSSLESS_DATA_POLL_INTERVAL_MS);
}

#endif /* INCLUDE_QSS */
