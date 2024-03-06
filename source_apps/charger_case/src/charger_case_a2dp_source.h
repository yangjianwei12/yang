/*!
\copyright  Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application A2DP source module
*/

#ifndef CHARGER_CASE_A2DP_SOURCE_H
#define CHARGER_CASE_A2DP_SOURCE_H


#include <logging.h>
#include <vmtypes.h>
#include <os.h>
#include <panic.h>
#include <operator.h>
#include <connection.h>
#include <a2dp.h>
#include <bdaddr.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <stdlib.h>
#include <vmal.h>
#include <audio_clock.h>
#include <cap_id_prim.h>
#include "kymera.h"
#include "a2dp.h"
#include "av_instance.h"

#include "kymera_adaptation_audio_protected.h"

typedef enum
{
    CHARGER_CASE_ADAPTIVE_LATENCY = CHARGER_CASE_MESSAGE_BASE
} charger_case_app_messages_t;

typedef enum
{
    CHARGER_CASE_A2DP_SOURCE_STATE_DISCONNECTED,
    CHARGER_CASE_A2DP_SOURCE_STATE_INQUIRY,
    CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTING,
    CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED,
    CHARGER_CASE_A2DP_SOURCE_STATE_CONNECTED_MEDIA,
    CHARGER_CASE_A2DP_SOURCE_STATE_STREAMING
} charger_case_a2dp_source_state_t;

typedef struct
{
    TaskData task;              /*!< Init's local task */
    charger_case_a2dp_source_state_t state;

    device_t sink_device;
    a2dp_codec_settings a2dp_settings;
    avInstanceTaskData *active_av_instance;    /*!< The AV instance this applies to */

    bdaddr inquiry_bd_addr[2]; /*!< Bluetooth addressed of the 2 'loudest' devices found during inquiry */
    int16 inquiry_rssi[2];    /*!< RSSI measurement of the 2 'loudest' devices */
    uint32 inquiry_cod[2];

} charger_case_a2dp_source_data_t;


/*!< Structure used while initialising */
extern charger_case_a2dp_source_data_t a2dp_source_data;

void ChargerCase_A2dpSourceInit(void);

void ChargerCase_A2dpSourceConnect(void);

void ChargerCase_A2dpSourceDisconnect(void);

void ChargerCase_A2dpSourceSuspend(void);

void ChargerCase_A2dpSourceResume(void);

void ChargerCase_ResetSinkDevice(void);

void ChargerCase_InquiryStart(void);

#endif /* CHARGER_CASE_A2DP_SOURCE_H */
