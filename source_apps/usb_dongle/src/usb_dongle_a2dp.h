/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application A2DP source module
*/

#ifndef USB_DONGLE_A2DP_SOURCE_H
#define USB_DONGLE_A2DP_SOURCE_H

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO

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
    /* Warning: Do not change the order of these states. The implementation
       assumes increasing order of "connectedness", and relative comparisons
       are thus made, e.g. "state >= APP_A2DP_STATE_CONNECTED". */
    APP_A2DP_STATE_DISCONNECTED = 0,
    APP_A2DP_STATE_CONNECTING,
    APP_A2DP_STATE_CONNECTED,
    APP_A2DP_STATE_CONNECTED_MEDIA,
    APP_A2DP_STATE_STREAMING
} usb_dongle_a2dp_source_state_t;

typedef struct
{
    TaskData task;              /*!< Init's local task */
    usb_dongle_a2dp_source_state_t state;
    a2dp_codec_settings a2dp_settings;
    avInstanceTaskData *active_av_instance; /*!< The connected AV instance for the current sink */

    void (*connected_cb)(void);    /*!< Callback for connected state. Returns bool TRUE if successful connection */
    void (*disconnected_cb)(void); /*!< Callback for disconnected state */

    struct {
        bool use_saved;			/*!< use the saved version */
        uint16 latency;         /*!< target latency saved/current value */
    } target_latency;           /*!< target latency override and current value */

    struct {
        bool use_saved;			/*!< use the saved version */
        uint32 rate;            /*!< sample rate saved value */
    } aptxad_hq_sample_rate;    /*!< sample rate override */

} usb_dongle_a2dp_source_data_t;

/*!< Structure used while initialising */
extern usb_dongle_a2dp_source_data_t a2dp_source_data;

void UsbDongle_A2dpSourceInit(void);

bool UsbDongle_A2dpSourceConnect(void);

bool UsbDongle_A2dpSourceSuspend(void);

bool UsbDongle_A2dpSourceResume(void);

void UsbDongle_A2dpRegisterConnectionCallbacks(void (*connected_cb)(void),
                                               void (*disconnected_cb)(void));

bool UsbDongle_A2dpSourceInConnectedState(void);

/*! \brief Enable the target latency override

    NOTE: The target latency is applied on the next A2DP connection.
    No minimum latency checking is done because this can vary between connection types.

    \param[in] target_latency_ms The target latency
 */
void UsbDongle_A2dpLatencyTargetSet(uint16 target_latency);

/*! \brief Return the current target latency

    \return[out] The target latency in ms
 */
uint16 UsbDongle_A2dpLatencyTargetGet(void);

/*! \brief Disable the target latency override
 */
void UsbDongle_A2dpLatencyTargetDefault(void);

/*! \brief Enable the aptX Adaptive HQ mode sample rate override.

    \param[in] sample_rate The desired aptX Adaptive HQ sample rate, in Hz.
 */
void UsbDongle_A2dpAptxAdHqSampleRateSet(uint32 sample_rate);

/*! \brief Return the saved aptX Adaptive HQ mode sample rate.

    \note This may not be equal to the actual current (live) sample rate, if it
          was not possible to negotiate the requested sample rate in the current
          mode (or if the current codec is not aptX Adaptive).

    \return[out] The saved aptX Adaptive HQ sample rate, in Hz (or 0 if no saved
                 rate i.e. override disabled).
 */
uint32 UsbDongle_A2dpAptxAdHqSampleRateGet(void);

/*! \brief Disable the aptX Adaptive HQ mode sample rate override.
 */
void UsbDongle_A2dpAptxAdHqSampleRateDefault(void);

/*! \brief Return the current (live) A2DP sample rate.

    \return[out] The current A2DP media channel sample rate (live value), in Hz
                 (or 0 if no media channel established).
 */
uint32 UsbDongle_A2dpGetCurrentSampleRate(void);

/*! \brief Update preferred A2DP sample rate based on current audio input

    \return TRUE if the new preference resulted in a reconfiguration of the A2DP
            media channel (which will automatically stop & restart audio), FALSE
            otherwise.
*/
bool UsbDongle_A2dpUpdatePreferredSampleRate(void);

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

#endif /* USB_DONGLE_A2DP_SOURCE_H */
