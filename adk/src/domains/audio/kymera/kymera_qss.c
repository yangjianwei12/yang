/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Kymera QSS Lossless Audio Data Manager
*/
#ifdef INCLUDE_QSS

#include "kymera_qss.h"
#include "kymera_data.h"
#include "kymera_state.h"
#include "kymera_le_audio.h"
#include "operators.h"
#include "mirror_profile.h"
#include "multidevice.h"

#define KYMERA_QSS_NUMBER_OF_PARAMS_TO_QUERY 0x3

#define KymeraQss_IsAptxadaptiveActive()    KymeraLeAudio_IsAptxAdaptiveStreaming()
                                              /* TODO Add check to query if A2DP streaming is active*/

bool KymeraQss_ReadAptxAdaptiveLosslesssInfo(uint32 *lossless_data)
{
    bool status = FALSE;

    switch (KymeraGetTaskData()->state)
    {
        case KYMERA_STATE_LE_AUDIO_ACTIVE:
        {
            status = KymeraLeAudio_GetAptxAdaptiveLossLessInfo(lossless_data);
        }
        break;

        /* TODO: Check for A2DP aptX Adaptive lossless when supported by audio
        case : KYMERA_STATE_A2DP_ACTIVE
        {
            aptX_adaptive_lossless_info = KymeraA2dpAudio_GetAptxAdaptiveLossLessInfo();
        }
        break;
        */

        default:
        break;
    }

    return status;
}

#endif /* INCLUDE_QSS */
