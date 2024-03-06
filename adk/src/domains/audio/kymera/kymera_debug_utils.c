/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       kymera_debug_utils.c
\brief      Kymera Audio interface to monitor audio-DSP statistics for debug purpose
*/
#ifdef INCLUDE_KYMERA_AUDIO_DEBUG
#include "kymera_debug_utils.h"
#include "kymera_state.h"
#include "kymera_a2dp.h"
#include "kymera_sco_private.h"
#include "kymera_data.h"
#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
#include "kymera_le_voice.h"
#include "le_unicast_manager.h"
#include "kymera_le_audio.h"
#endif

#include <custom_operator.h>
#include <cap_id_prim.h>
#include <logging.h>
#include <opmsg_prim.h>
#include <byte_utils.h>

#ifdef GC_SECTIONS
/* Move all functions in KEEP_PM section to ensure they are not removed during
* garbage collection */
#pragma unitcodesection KEEP_PM
#endif

#define NUMBER_OF_PARAMS              12

/*! \brief Kymera Debug task structure */
typedef struct
{
    /*! Kymera Debug task */
    TaskData task;
    /*! Is MCPS monitor operator instantiated */
    bool is_debug_enabled;
    /*! Bundle ID to track capability download bundle */
    BundleID op_bundle_id;
    /*! Operator instantiated on Audio0 */
    Operator mcps_op_audio0;
    /*! Operator instantiated on Audio1 */
    Operator mcps_op_audio1;
} kymeraDebugUtilTaskData;

/*! \brief Handle notification/confirmation from audio-DSP */
static void kymeraDebugUtil_HandleMessage(Task task, MessageId id, Message message);

kymeraDebugUtilTaskData kymera_debug_util_task_data = {kymeraDebugUtil_HandleMessage};

static void kymeraDebugUtil_HandleMessage(Task task, MessageId id, Message message)
{
    MessageFromOperator* op_msg = NULL;
    uint32 cycles, time;

    UNUSED(task);

    switch(id)
    {
        case MESSAGE_FROM_OPERATOR:
            /*
            Each unsolicited message will contain following info [Word 0 - Word 7]:
            [0] apps0 connection Id
            [1] Message ID (0x70 for this operator message)
            [2] Length of the message.
            [3] Processor ID
            [4] Higher word of time_delta
            [5] Lower word of time_delta
            [6] Higher word of clks_delta
            [7] Lower word of clks_delta
            */
            op_msg = (MessageFromOperator*)message;
            time = MAKELONG(op_msg->message[5], op_msg->message[4]);
            cycles = MAKELONG(op_msg->message[7], op_msg->message[6]);
            DEBUG_LOG("MCPS Usage from processor %d, %d.%d MCPS", op_msg->message[3], cycles/time, (cycles%time)/(time/1000));

            if(Kymera_A2dpIsActive())
            {
                KymeraDebugUtil_GetA2dpDecoderStats();
            }
            
            if(Kymera_ScoIsActive())
            {
                KymeraDebugUtil_GetScoDecoderStats();
            }

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
            if ((KymeraGetTaskData()->state == KYMERA_STATE_LE_AUDIO_ACTIVE) ||
                (KymeraGetTaskData()->state == KYMERA_STATE_LE_VOICE_ACTIVE))
            {
                KymeraDebugUtil_GetLc3DecoderStats();
            }
#endif

        break;

        default:
            DEBUG_LOG("kymeraDebugUtil_HandleMessage: Unhandled Message");
        break;
     }
}

/*! \brief To check if any of the processpr framework are enabled
           before starting MCPS monitor*/
static bool kymeraDebugUtil_CanStartMcpsMonitor(void)
{
    return (vmalOperatorIsProcessorFrameworkEnabled(main_processor) ||
            vmalOperatorIsProcessorFrameworkEnabled(second_processor));
}

void KymeraDebugUtil_StartAudioDspMonitor(void)
{
    const char dkcs_name[] = "download_mips_monitor.edkcs";
    FILE_INDEX index = FileFind(FILE_ROOT, dkcs_name, strlen(dkcs_name));
    bool is_dual_core = FALSE;

    PanicFalse(index != FILE_NONE);

    DEBUG_LOG("KymeraDebugUtil_StartMcpsMonitor enabled %d", kymera_debug_util_task_data.is_debug_enabled);

    if ((kymera_debug_util_task_data.is_debug_enabled) ||
        !kymeraDebugUtil_CanStartMcpsMonitor())
    {
        /* Could not start the MCPS monitor now!*/
        return;
    }

    is_dual_core = vmalOperatorIsProcessorFrameworkEnabled(second_processor);
    kymera_debug_util_task_data.op_bundle_id = PanicZero(OperatorBundleLoad(index, is_dual_core ? capability_load_to_p0_use_on_both :
                                                         capability_load_to_p0_use_on_p0_only));
    kymera_debug_util_task_data.mcps_op_audio0 = OperatorsCreate(CAP_ID_DOWNLOAD_MIPS_MONITOR, OPERATOR_PROCESSOR_ID_0, operator_priority_lowest);
    MessageOperatorTask(kymera_debug_util_task_data.mcps_op_audio0, &kymera_debug_util_task_data.task);

    if (is_dual_core)
    {
        kymera_debug_util_task_data.mcps_op_audio1 = OperatorsCreate(CAP_ID_DOWNLOAD_MIPS_MONITOR, OPERATOR_PROCESSOR_ID_1, operator_priority_lowest);
        MessageOperatorTask(kymera_debug_util_task_data.mcps_op_audio1, &kymera_debug_util_task_data.task);
    }

    kymera_debug_util_task_data.is_debug_enabled = TRUE;
}

void KymeraDebugUtil_StopAudioDspMonitor(void)
{

    DEBUG_LOG("KymeraDebugUtil_StopMcpsMonitor enabled %d", kymera_debug_util_task_data.is_debug_enabled);

    if (!kymera_debug_util_task_data.is_debug_enabled)
    {
        return;
    }

    if (kymera_debug_util_task_data.op_bundle_id)
    {
        if (kymera_debug_util_task_data.mcps_op_audio0 != INVALID_OPERATOR)
        {
            OperatorsDestroy(&kymera_debug_util_task_data.mcps_op_audio0, 1);
            kymera_debug_util_task_data.mcps_op_audio0 = INVALID_OPERATOR;
        }

        if (kymera_debug_util_task_data.mcps_op_audio1 != INVALID_OPERATOR)
        {
            OperatorsDestroy(&kymera_debug_util_task_data.mcps_op_audio1, 1);
            kymera_debug_util_task_data.mcps_op_audio1 = INVALID_OPERATOR;
        }

        OperatorBundleUnload(kymera_debug_util_task_data.op_bundle_id);
        kymera_debug_util_task_data.op_bundle_id = 0;
    }

    kymera_debug_util_task_data.is_debug_enabled = FALSE;

}

void KymeraDebugUtil_GetA2dpDecoderStats(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    uint32 seq_errs = 0;
    Operator op = INVALID_OPERATOR;
    get_status_data_t* get_status = NULL;
    vm_transform_query_ttp_stats_t query_ttp_stats = {0};

#ifdef INCLUDE_MIRRORING
    Transform packetiser = theKymera->hashu.packetiser;
#else
    Transform packetiser = theKymera->packetiser;
#endif

    if(packetiser)
    {
        if(!TransformQuery(packetiser, VM_TRANSFORM_QUERY_SEQ_ERRORS, &seq_errs))
        {
            DEBUG_LOG_ERROR("KymeraDebugUtil_GetA2dpSeqErrors query transforms for seq errors failed");
        }

        DEBUG_LOG("KymeraDebugUtil_GetA2dpSeqErrors Error-Count %d", seq_errs);
    }

    if(theKymera->q2q_mode)
    {
        if(packetiser)
        {
            if(!TransformQuery(packetiser, VM_TRANSFORM_QUERY_TTP_STATS, (uint32 *)&query_ttp_stats))
            {
                DEBUG_LOG_ERROR("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats query transform ttp stats failed");
                return;
            }

            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP <0ms %d", query_ttp_stats.bins[0]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP 0ms-25ms %d", query_ttp_stats.bins[1]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP 25ms-50ms %d", query_ttp_stats.bins[2]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP 50ms-100ms %d", query_ttp_stats.bins[3]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP 100ms-150ms %d", query_ttp_stats.bins[4]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpQ2qPacketiserTtpStats: A2DP TTP >150ms %d", query_ttp_stats.bins[5]);
        }

    }
    else
    {
        op = ChainGetOperatorByRole(KymeraGetTaskData()->chain_input_handle, OPR_RTP_DECODER);

        if(op == INVALID_OPERATOR)
        {
            DEBUG_LOG_ALWAYS("kymeraStatistics_GetRtpDecoderTtpStats RTP decoder not found");
            return;
        }

        get_status = OperatorsCreateGetStatusData(6);
        PanicNull(get_status);

        OperatorsGetStatus(op, get_status);

        if(get_status->result == obpm_ok)
        {
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP <0ms %d", get_status->value[0]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP 0ms-25ms %d", get_status->value[1]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP 25ms-50ms %d", get_status->value[2]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP 50ms-100ms %d", get_status->value[3]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP 100ms-150ms %d", get_status->value[4]);
            DEBUG_LOG("KymeraDebugUtil_GetA2dpRtpDecoderTtpStats: A2DP TTP >150ms %d", get_status->value[5]);
        }
        else
        {
            DEBUG_LOG_ERROR("kymeraStatistics_GetRtpDecoderTtpStats failed: %d", get_status->result);
        }

        free(get_status);
    }
}

void KymeraDebugUtil_GetScoDecoderStats(void)
{
    get_status_data_t * get_status = Kymera_GetOperatorStatusDataInScoChain(OPR_SCO_RECEIVE, NUMBER_OF_PARAMS);

    if(get_status == NULL)
    {
        DEBUG_LOG_WARN("KymeraDebugUtil_GetScoDecoderStats Fail");
        return;
    }

    if(get_status->result == obpm_ok)
    {
        /* get_status will contain Total No of packets count and error packet count as part of value[2] and value[3] */
        DEBUG_LOG("KymeraDebugUtil_GetScoDecoderStats: Total No Of Packets %d, Error Packets %d", get_status->value[2], get_status->value[3]);
    }
    else
    {
        DEBUG_LOG_ERROR("KymeraDebugUtil_GetScoDecoderStats failed: %d", get_status->result);
    }

    free(get_status);

    return;
}

#if defined(INCLUDE_LE_AUDIO_BROADCAST) || defined(INCLUDE_LE_AUDIO_UNICAST)
void KymeraDebugUtil_GetLc3DecoderStats(void)
{
    get_status_data_t * get_status = NULL;
    kymera_chain_handle_t chain = NULL;
    Operator op = INVALID_OPERATOR;

#ifdef INCLUDE_LE_STEREO_RECORDING
    /* In case of stereo recording Kymera will be in KYMERA_STATE_LE_AUDIO_ACTIVE but 
	   only LC3 encoders are in use
	*/
    if (LeUnicastManager_IsLeStereoRecordingActive())
	{
        return;
	}
#endif
    /* aptX R3 decoder does not support debug statistics */
    if(KymeraLeAudio_IsAptxAdaptiveStreaming())
    {
        return;
    }

    if (KymeraGetTaskData()->state == KYMERA_STATE_LE_AUDIO_ACTIVE)
    {
        chain = KymeraGetTaskData()->chain_input_handle;
    }
    else if (KymeraGetTaskData()->state == KYMERA_STATE_LE_VOICE_ACTIVE)
    {
        chain = Kymera_LeVoiceGetCvcChain();
    }

    op = ChainGetOperatorByRole(chain, OPR_LC3_DECODE_SCO_ISO);

    if (op == INVALID_OPERATOR)
    {
        DEBUG_LOG_WARN("KymeraDebugUtil_GetLc3DecoderStats LC3 decoder not found");
        return;
    }

    get_status = OperatorsCreateGetStatusData(2);
    OperatorsGetStatus(op, get_status);

    if (get_status == NULL)
    {
        DEBUG_LOG_WARN("KymeraDebugUtil_GetLc3DecoderStats Fail");
        return;
    }

    if(get_status->result == obpm_ok)
    {
        DEBUG_LOG("KymeraDebugUtil_GetLc3DecoderStats: voice packets total %d, error %d", get_status->value[0], get_status->value[1]);
    }
    else
    {
        DEBUG_LOG_WARN("KymeraDebugUtil_GetLc3DecoderStats failed %d", get_status->result);
    }

    free(get_status);
}
#endif

#endif /* INCLUDE_KYMERA_AUDIO_DEBUG */
