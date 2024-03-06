/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to connect/manage to AEC chain
*/

#include "kymera_aec.h"
#include "kymera_connect_utils.h"
#include "kymera_setup.h"
#include "kymera_data.h"
#include <logging.h>
#include <vmal.h>
#include <opmsg_prim.h>
#include <stdlib.h>

#define DEFAULT_MIC_SAMPLE_RATE     (16000)
#define DEFAULT_OUTPUT_SAMPLE_RATE  (48000)

#define FIRST_TERMINAL 1
#define SECOND_TERMINAL (FIRST_TERMINAL + 1)
#define THIRD_TERMINAL (SECOND_TERMINAL + 1)
#define FOURTH_TERMINAL (THIRD_TERMINAL + 1)
#define CONNECT_TERMINAL(bit_map, pos)  (bit_map & (1 << (pos - 1)))
#define DISCONNECT_TERMINAL(bit_map, pos) CONNECT_TERMINAL(bit_map, pos)
#define AEC_8_KHZ_RATE     (8000)
#define AEC_16_KHZ_RATE   (16000)
#define AEC_32_KHZ_RATE   (32000)
#define AEC_44_1_KHZ_RATE (44100)
#define AEC_48_KHZ_RATE   (48000)

/*! Buffer sizes [ms] for speaker input and mic output of aec_ref */
#define AEC_REF_DEFAULT_MIC_INPUT_BUFFER_SIZE_MS 15
#define AEC_REF_DEFAULT_MIC_OUTPUT_BUFFER_SIZE_MS 15
#define AEC_REF_AANC_MIC_INPUT_BUFFER_SIZE_MS 45
#define AEC_REF_AANC_MIC_OUTPUT_BUFFER_SIZE_MS 45

#define AEC_TASK_PERIOD_DEFAULT (2000U) //2msec
#define AEC_DECIM_FACTOR (1U)   //sidetone path task period: AEC_TASK_PERIOD/AEC_DECIM_FACTOR i.e., 2msec/1 => 2msec

typedef enum
{
    speaker_path_connected = (1 << 0),
    mic_path_input_connected  = (1 << 1),
    mic_path_output_connected  = (1 << 2)
} aec_connections_t;

static const struct
{
    chain_endpoint_role_t input_role;
    chain_endpoint_role_t output_role;
} aec_mic_endpoints[AEC_MAX_NUM_OF_MICS] =
{
    {EPR_AEC_MIC1_IN, EPR_AEC_MIC1_OUT},
    {EPR_AEC_MIC2_IN, EPR_AEC_MIC2_OUT},
    {EPR_AEC_MIC3_IN, EPR_AEC_MIC3_OUT},
    {EPR_AEC_MIC4_IN, EPR_AEC_MIC4_OUT},
};

static struct
{
    uint32            mic_sample_rate;
    uint32            spk_sample_rate;
    aec_connections_t aec_connections;
    aec_usecase_t     use_case;
    uint32            task_period;
    unsigned          activate_timestamps:1;
    uint32            ttp_delay;
    uint32            configured_ttp_delay;
} aec_config;

typedef struct
{
    uint32 speaker_input_buffer_size;
    uint32 mic_output_buffer_size;
    bool is_source_clock_same;
}aec_additional_config_t;

static const aec_additional_config_t aec_additional_config =
{
    /* Extended buffer sizes are only needed for AANC */
#ifdef ENABLE_ADAPTIVE_ANC
    .speaker_input_buffer_size = AEC_REF_AANC_MIC_INPUT_BUFFER_SIZE_MS,
    .mic_output_buffer_size = AEC_REF_AANC_MIC_OUTPUT_BUFFER_SIZE_MS,
#else
    .speaker_input_buffer_size = AEC_REF_DEFAULT_MIC_INPUT_BUFFER_SIZE_MS,
    .mic_output_buffer_size = AEC_REF_DEFAULT_MIC_OUTPUT_BUFFER_SIZE_MS,
#endif
#ifdef ENABLE_AEC_LEAKTHROUGH
    .is_source_clock_same = TRUE, //Same clock source for speaker and mic path for AEC-leakthrough.
                                  //Should have no implication on normal aec operation.
#else
    .is_source_clock_same = FALSE,
#endif
};

kymera_chain_handle_t aec_chain;

static kymera_chain_handle_t kymera_GetAecChain(void)
{
    return aec_chain;
}

static void kymera_SetAecChain(kymera_chain_handle_t chain)
{
    aec_chain = chain;
}

static uint32 kymera_GetOutputSampleRate(void)
{
    uint32 sample_rate = DEFAULT_OUTPUT_SAMPLE_RATE;

    if (aec_config.spk_sample_rate)
    {
        sample_rate = aec_config.spk_sample_rate;
    }
    return sample_rate;
}

static uint32 kymera_GetMicSampleRate(void)
{
    uint32 sample_rate = DEFAULT_MIC_SAMPLE_RATE;

    if (aec_config.mic_sample_rate)
    {
        sample_rate = aec_config.mic_sample_rate;
    }
    return sample_rate;
}

static void kymera_SetMicSampleRate(uint32 sample_rate)
{
    aec_config.mic_sample_rate = sample_rate;
}

static void kymera_SetSpkSampleRate(uint32 sample_rate)
{
    aec_config.spk_sample_rate = sample_rate;
}

static Operator kymera_GetAecOperator(void)
{
    Operator aec = ChainGetOperatorByRole(kymera_GetAecChain(), OPR_AEC);
    return aec;
}

static Sink kymera_GetAecInput(chain_endpoint_role_t input_role)
{
    return ChainGetInput(kymera_GetAecChain(), input_role);
}

static Source kymera_GetAecOutput(chain_endpoint_role_t output_role)
{
    return ChainGetOutput(kymera_GetAecChain(), output_role);
}

static bool kymera_IsMicPathOutputConnected(void)
{
    return (aec_config.aec_connections & mic_path_output_connected) !=  0;
}

static bool kymera_IsMicPathInputConnected(void)
{
    return (aec_config.aec_connections & mic_path_input_connected) !=  0;
}

static bool kymera_IsAudioInputConnected(void)
{
    return (Kymera_AecIsMicPathInputConnected() && kymera_IsMicPathOutputConnected());
}

static bool kymera_IsAudioOutputConnected(void)
{
    return (aec_config.aec_connections & speaker_path_connected) != 0;
}

static bool kymera_IsAecConnected(void)
{
    return kymera_IsAudioInputConnected() || kymera_IsAudioOutputConnected();
}

static void kymera_AddAecConnection(aec_connections_t connection)
{
    aec_config.aec_connections |= connection;
}

static void kymera_RemoveAecConnection(aec_connections_t connection)
{
    aec_config.aec_connections &= ~connection;
}

static void kymera_ConfigureAecChain(void)
{
    Operator aec = kymera_GetAecOperator();
    if (aec != INVALID_OPERATOR)
    {
        kymera_operator_ucid_t ucid = Kymera_GetAecUcid();
        DEBUG_LOG("kymera_ConfigureAecChain: speaker sample rate %u, mic sample rate %u, UCID enum:kymera_operator_ucid_t:%d",
                kymera_GetOutputSampleRate(), kymera_GetMicSampleRate(), ucid);
        OperatorsStandardSetUCID(aec, ucid);
        OperatorsAecSetSampleRate(aec, kymera_GetOutputSampleRate(), kymera_GetMicSampleRate());
    }
}

static void kymera_AecSetSameInputOutputClkSource(Operator op, bool enable)
{
    aec_ref_set_same_in_out_clk_src_msg_t aec_ref_set_same_in_out_clk_src_msg;

    aec_ref_set_same_in_out_clk_src_msg.id = AEC_REF_SAME_INPUT_OUTPUT_CLK_SOURCE;
    aec_ref_set_same_in_out_clk_src_msg.value = (uint16)enable;

    PanicFalse(VmalOperatorMessage(op, &aec_ref_set_same_in_out_clk_src_msg, SIZEOF_OPERATOR_MESSAGE(aec_ref_set_same_in_out_clk_src_msg), NULL, 0));

}

static void kymera_SetAecTerminalBufferSize(Operator op, uint32 rate, uint32 buffer_size_ms,
                                           uint16 input_terminals, uint16 output_terminals)
{
    uint16 msg[4];
    msg[0] = OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE;
    msg[1] = (rate * buffer_size_ms) / 1000;
    msg[2] = input_terminals;
    msg[3] = output_terminals;
    OperatorMessage(op, msg, ARRAY_DIM(msg), NULL, 0);
}

static uint32 kymera_ValidateTaskPeriod(uint32 requested_task_period)
{
    const uint16 valid_task_periods_us[] = {1000, 1250, 1600, 2000, 2500, 3125, 4000, 5000};
    for(uint8 i = ARRAY_DIM(valid_task_periods_us); i > 0; i--)
    {
        if(requested_task_period >= valid_task_periods_us[i-1])
        {
            DEBUG_LOG("kymera_ValidateTaskPeriod: Selected %d us", valid_task_periods_us[i-1]);
            return valid_task_periods_us[i-1];
        }
    }
    DEBUG_LOG_ERROR("kymera_ValidateTaskPeriod: Invalid task period found: %d us", requested_task_period);
    Panic();
    return 0;
}

uint32 Kymera_AecGetTaskPeriod(void)
{
    uint32 task_period = AEC_TASK_PERIOD_DEFAULT;

    if (aec_config.task_period)
    {
        task_period = aec_config.task_period;
    }
    return task_period;
}

void Kymera_AecSetTaskPeriod(uint32 task_period)
{
    aec_config.task_period = kymera_ValidateTaskPeriod(task_period);
}

uint32 Kymera_AecGetConfiguredTtpDelay(void)
{
    uint32 ttp_delay = 0;
    Operator aec = kymera_GetAecOperator();
    if(aec)
    {
        ttp_delay = aec_config.configured_ttp_delay;
    }
    else
    {
        ttp_delay = aec_config.ttp_delay;
    }
    DEBUG_LOG("Kymera_AecGetConfiguredTtpDelay: %u us", ttp_delay);
    return ttp_delay;
}

static uint32 kymera_AecGetTimestamps(void)
{
    uint32 timestamps = 0;
    if(aec_config.activate_timestamps)
    {
        timestamps = AEC_REF_DEFAULT_MIC_TTP_LATENCY;
    }
    DEBUG_LOG("kymera_AecGetTimestamps: %u us", timestamps);
    return timestamps;
}

void Kymera_AecActivateDeactivateTimestamping(bool activate_timestamps)
{
    DEBUG_LOG("Kymera_AecActivateDeactivateTimestamping: %u", activate_timestamps);
    aec_config.activate_timestamps = activate_timestamps;
}

static void kymera_ConfigureSpeakerInputBuffer(void)
{
#define AEC_REF_FIRST_SPK_INPUT_TERMINAL (0U)
    Operator aec = kymera_GetAecOperator();
    if (!aec)
    {
        Panic();
    }
    DEBUG_LOG("kymera_ConfigureSpeakerInputBuffer: Spk In Buffer Size %u",
              aec_additional_config.speaker_input_buffer_size);

    if(aec_additional_config.speaker_input_buffer_size != 0)
    {
        kymera_SetAecTerminalBufferSize(aec, kymera_GetOutputSampleRate(),
                                        aec_additional_config.speaker_input_buffer_size,
                                        (1 << AEC_REF_FIRST_SPK_INPUT_TERMINAL), 0);
    }
}

static void kymera_ConfigureMicOutputBuffer(void)
{
#define AEC_REF_FIRST_MIC_OUTPUT_TERMINAL (3U)
    Operator aec = kymera_GetAecOperator();
    if (!aec)
    {
        Panic();
    }
    DEBUG_LOG("kymera_ConfigureMicOutputBuffer: Mic Out Terminal Buffer Size %u",
              aec_additional_config.mic_output_buffer_size);

    if(aec_additional_config.mic_output_buffer_size != 0)
    {
        kymera_SetAecTerminalBufferSize(aec, kymera_GetMicSampleRate(),
                                        aec_additional_config.mic_output_buffer_size,
                                        0, (1 << AEC_REF_FIRST_MIC_OUTPUT_TERMINAL));
    }
}

static void kymera_ConfigureTaskPeriod(void)
{
    Operator aec = kymera_GetAecOperator();
    if (!aec)
    {
        Panic();
    }

    /*!
        * Default Aec_ref task period is 2msec. For task_period of 2msec and decim_factor of 1,
        * all the processing paths of aec_ref will run at 2ms.
        * For special requirements a different task period can be selected.
    */
    uint32 task_period = Kymera_AecGetTaskPeriod();
    DEBUG_LOG("kymera_ConfigureTaskPeriod: %u us", task_period);
    OperatorsAecSetTaskPeriod(aec, task_period, AEC_DECIM_FACTOR);
}

static void kymera_ConfigureTtpDelay(void)
{
    Operator aec = kymera_GetAecOperator();
    if (!aec)
    {
        Panic();
    }

    uint32 ttp_delay = aec_config.ttp_delay;
    if(ttp_delay == 0)
    {
        ttp_delay = kymera_AecGetTimestamps();
    }

    if (ttp_delay != 0)
    {
        OperatorsStandardSetTimeToPlayLatency(aec, ttp_delay);
        OperatorsStandardSetLatencyLimits(aec, (ttp_delay - 250), (ttp_delay + 250));
    }
    aec_config.configured_ttp_delay = ttp_delay;
}

static void kymera_ConfigureSourceClock(void)
{
    Operator aec = kymera_GetAecOperator();
    if (!aec)
    {
        Panic();
    }

    HYDRA_LOG_STRING(msg_yes, "Yes");
    HYDRA_LOG_STRING(msg_no, "No");
    DEBUG_LOG("kymera_ConfigureSourceClock: Source Clock %s", aec_additional_config.is_source_clock_same ? msg_yes : msg_no);

    if(aec_additional_config.is_source_clock_same != 0)
    {

        /*! Message AECREF operator that the back-end of the operator are coming
            from same clock source. This is for optimisation purpose and it's recommended
            to be enabled for use cases where speaker input and microphone output are
            synchronised (e.g. SCO and USB voice use cases). Note: Send/Resend this message
            when all microphone input/output and REFERENCE output are disconnected.
        */
        kymera_AecSetSameInputOutputClkSource(aec,TRUE);
    }
}

static void kymera_AdditionalConfigureForAecCreation(void)
{
    kymera_ConfigureTaskPeriod();
    kymera_ConfigureTtpDelay();
}

static void kymera_AdditionalConfigureForAecAudioOutput(void)
{
    kymera_ConfigureSpeakerInputBuffer();
}

static void kymera_AdditionalConfigureForAecAudioInput(void)
{
    kymera_ConfigureMicOutputBuffer();
    kymera_ConfigureSourceClock();
}

static void kymera_CreateAecChain(const aec_audio_config_t* config)
{
    DEBUG_LOG("kymera_CreateAecChain");
    PanicNotNull(kymera_GetAecChain());
    kymera_SetAecChain(PanicNull(ChainCreate(Kymera_GetChainConfigs()->chain_aec_config)));
    kymera_ConfigureAecChain();
    kymera_AdditionalConfigureForAecCreation();

    if(KymeraGetTaskData()->chain_config_callbacks && KymeraGetTaskData()->chain_config_callbacks->ConfigureAecChain)
    {
        kymera_aec_config_params_t params = {0};
        params.mic_sample_rate = config->mic_sample_rate;
        params.spk_sample_rate = config->spk_sample_rate;
        KymeraGetTaskData()->chain_config_callbacks->ConfigureAecChain(kymera_GetAecChain(), &params);
    }

    ChainConnect(kymera_GetAecChain());
    ChainStart(kymera_GetAecChain());
}

static void kymera_DestroyAecChain(void)
{
    DEBUG_LOG("kymera_DestroyAecChain");
    PanicNull(kymera_GetAecChain());
    ChainStop(kymera_GetAecChain());
    ChainDestroy(kymera_GetAecChain());
    kymera_SetAecChain(NULL);
    aec_config.ttp_delay = 0;
}

static void kymera_ConnectAudioOutput(const aec_connect_audio_output_t *params)
{
    DEBUG_LOG("kymera_ConnectAudioOutput: Connect audio output to AEC");

    kymera_AddAecConnection(speaker_path_connected);

    /* For a running operator connect the output before the input.
       As per kymera capability user guide, Connect OUTPUT2, 3, ..., N before connecting OUTPUT1 for a running operator */
    Source right_source=kymera_GetAecOutput(EPR_AEC_SPEAKER2_OUT);
    Source left_source=kymera_GetAecOutput(EPR_AEC_SPEAKER1_OUT);

    Kymera_ConnectOutputSource(left_source,right_source,aec_config.spk_sample_rate);

    /* As per kymera capability user guide, connect INPUT2, 3, ..., N before connecting INPUT1 for a running operator */
    Kymera_ConnectIfValid(params->input_2, kymera_GetAecInput(EPR_AEC_INPUT2));
    Kymera_ConnectIfValid(params->input_1, kymera_GetAecInput(EPR_AEC_INPUT1));
}

static void kymera_DisconnectAudioOutput(void)
{
    DEBUG_LOG("kymera_DisconnectAudioOutput: Disconnect audio output from AEC");
    kymera_RemoveAecConnection(speaker_path_connected);
    /* For a running operator disconnect the output before the input */
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_SPEAKER1_OUT), NULL);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_SPEAKER2_OUT), NULL);
    AudioOutputDisconnect();

    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_INPUT1));
    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_INPUT2));
}

static void kymera_PopulateMicSources(uint8 num_of_mics, const aec_connect_audio_input_t *mic_path, Source *mic_sources)
{
    switch(num_of_mics)
    {
        case 8:
            mic_sources[7] = mic_path->mic_input_8;
            // Fallthrough
        case 7:
            mic_sources[6] = mic_path->mic_input_7;
            // Fallthrough
        case 6:
            mic_sources[5] = mic_path->mic_input_6;
            // Fallthrough
        case 5:
            mic_sources[4] = mic_path->mic_input_5;
            // Fallthrough
        case 4:
            mic_sources[3] = mic_path->mic_input_4;
            // Fallthrough
        case 3:
            mic_sources[2] = mic_path->mic_input_3;
            // Fallthrough
        case 2:
            mic_sources[1] = mic_path->mic_input_2;
            // Fallthrough
        case 1:
            mic_sources[0] = mic_path->mic_input_1;
            break;
        default:
            Panic();
            break;
    }
}

static void kymera_PopulateMicSinks(uint8 num_of_mics, const aec_connect_audio_input_t *mic_path, Sink *mic_sinks)
{
    switch(num_of_mics)
    {
        case 8:
            mic_sinks[7] = mic_path->mic_output_8;
            // Fallthrough
        case 7:
            mic_sinks[6] = mic_path->mic_output_7;
            // Fallthrough
        case 6:
            mic_sinks[5] = mic_path->mic_output_6;
            // Fallthrough
        case 5:
            mic_sinks[4] = mic_path->mic_output_5;
            // Fallthrough
        case 4:
            mic_sinks[3] = mic_path->mic_output_4;
            // Fallthrough
        case 3:
            mic_sinks[2] = mic_path->mic_output_3;
            // Fallthrough
        case 2:
            mic_sinks[1] = mic_path->mic_output_2;
            // Fallthrough
        case 1:
            mic_sinks[0] = mic_path->mic_output_1;
            break;
        default:
            Panic();
            break;
    }
}

static void kymera_ConnectMicPathOutput(const aec_mic_path_output_t *params)
{
    DEBUG_LOG("kymera_ConnectMicPathOutput");
    kymera_AddAecConnection(mic_path_output_connected);
    Kymera_ConnectIfValid(kymera_GetAecOutput(EPR_AEC_REFERENCE_OUT), params->aec_reference);
    for(unsigned i = 0; i < MIN(AEC_MAX_NUM_OF_MICS,params->num_of_mics); i++)
    {
        Kymera_ConnectIfValid(kymera_GetAecOutput(aec_mic_endpoints[i].output_role), params->mics[i]);
    }
}

static void kymera_ConnectMicPathInput(const aec_mic_path_input_t *params)
{
    DEBUG_LOG("kymera_ConnectMicPathInput");
    kymera_AddAecConnection(mic_path_input_connected);
    for(unsigned i = 0; i < MIN(AEC_MAX_NUM_OF_MICS,params->num_of_mics); i++)
    {
        Kymera_ConnectIfValid(params->mics[i], kymera_GetAecInput(aec_mic_endpoints[i].input_role));
    }
}

static void kymera_ConnectAudioInput(const aec_connect_audio_input_t *params)
{
    DEBUG_LOG("kymera_ConnectAudioInput: Connect audio input to AEC");
    Source mic_sources[AEC_MAX_NUM_OF_MICS] = {0};
    Sink mic_sinks[AEC_MAX_NUM_OF_MICS] = {0};
    aec_mic_path_input_t input =
    {
        .num_of_mics = AEC_MAX_NUM_OF_MICS,
        .mics = mic_sources
    };
    aec_mic_path_output_t output =
    {
        .num_of_mics = AEC_MAX_NUM_OF_MICS,
        .aec_reference = params->reference_output,
        .mics = mic_sinks
    };

    kymera_PopulateMicSources(AEC_MAX_NUM_OF_MICS, params, mic_sources);
    kymera_PopulateMicSinks(AEC_MAX_NUM_OF_MICS, params, mic_sinks);
    kymera_ConnectMicPathInput(&input);
    kymera_ConnectMicPathOutput(&output);
}

static void kymera_DisconnectAudioInput(void)
{
    DEBUG_LOG("kymera_DisconnectAudioInput: Disconnect audio input from AEC");

    /* For a running operator disconnect the output before the input */
    kymera_RemoveAecConnection(mic_path_output_connected);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC1_OUT), NULL);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC2_OUT), NULL);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC3_OUT), NULL);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_MIC4_OUT), NULL);
    Kymera_DisconnectIfValid(kymera_GetAecOutput(EPR_AEC_REFERENCE_OUT), NULL);

    kymera_RemoveAecConnection(mic_path_input_connected);
    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_MIC1_IN));
    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_MIC2_IN));
    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_MIC3_IN));
    Kymera_DisconnectIfValid(NULL, kymera_GetAecInput(EPR_AEC_MIC4_IN));
}

static void kymera_DisconnectSelectedAudioInput(const aec_disconnect_audio_input_t *params)
{
    uint8 bit_map = params->mic_disconnect_bitmap;
    DEBUG_LOG("kymera_DisconnectAudioInputForConcurrency: Disconnect audio input from AEC");
    /*Disconnect Outputs*/
    if (DISCONNECT_TERMINAL(bit_map, FIRST_TERMINAL))
        StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC1_OUT), NULL);
    if (DISCONNECT_TERMINAL(bit_map, SECOND_TERMINAL))
        StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC2_OUT), NULL);
    if (DISCONNECT_TERMINAL(bit_map, THIRD_TERMINAL))
        StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC3_OUT), NULL);
    if (DISCONNECT_TERMINAL(bit_map, FOURTH_TERMINAL))
        StreamDisconnect(kymera_GetAecOutput(EPR_AEC_MIC4_OUT), NULL);

    /*Disconnect Inputs*/
    if (DISCONNECT_TERMINAL(bit_map, FIRST_TERMINAL))
        StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC1_IN));
    if (DISCONNECT_TERMINAL(bit_map, SECOND_TERMINAL))
        StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC2_IN));
    if (DISCONNECT_TERMINAL(bit_map, THIRD_TERMINAL))
        StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC3_IN));
    if (DISCONNECT_TERMINAL(bit_map, FOURTH_TERMINAL))
        StreamDisconnect(NULL, kymera_GetAecInput(EPR_AEC_MIC4_IN));
}

kymera_operator_ucid_t Kymera_GetAecUcid(void)
{
    kymera_operator_ucid_t ucid;
#ifdef ENABLE_AEC_LEAKTHROUGH
    ucid = UCID_AEC_DEFAULT_LT_ENABLED;
#else
    ucid = UCID_AEC_DEFAULT_LT_DISABLED;
#endif

    if(aec_config.use_case == aec_usecase_default)
    {
        switch(aec_config.mic_sample_rate)
        {
            case AEC_8_KHZ_RATE:
                ucid = UCID_AEC_8_KHZ;
            break;
            case AEC_16_KHZ_RATE:
                ucid = UCID_AEC_16_KHZ;
            break;
            case AEC_32_KHZ_RATE:
                ucid = UCID_AEC_32_KHZ;
            break;
            case AEC_44_1_KHZ_RATE:
                ucid = UCID_AEC_44_1_KHZ;
            break;
            case AEC_48_KHZ_RATE:
                ucid = UCID_AEC_48_KHZ;
            break;
        }
    }
    else if((aec_config.use_case >= aec_usecase_anc) && (aec_config.use_case <= aec_usecase_anc4))
    {
        ucid = UCID_AEC_ANC + (aec_config.use_case - aec_usecase_anc);
    }
    else
    {
        switch(aec_config.mic_sample_rate)
        {
            case AEC_8_KHZ_RATE:
                ucid = UCID_AEC_8_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
            break;
            case AEC_16_KHZ_RATE:
                ucid = UCID_AEC_16_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
            break;
            case AEC_32_KHZ_RATE:
                ucid = UCID_AEC_32_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
            break;
            case AEC_44_1_KHZ_RATE:
                ucid = UCID_AEC_44_1_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
            break;
            case AEC_48_KHZ_RATE:
                ucid = UCID_AEC_48_KHZ_LT_MODE_1 + AecLeakthrough_GetMode();
            break;
        }
    }
    DEBUG_LOG("Kymera_GetAecUcid: enum:kymera_operator_ucid_t:%d", ucid);
    return ucid;
}

void Kymera_ConnectAudioOutputToAec(const aec_connect_audio_output_t *params, const aec_audio_config_t* config)
{
    DEBUG_LOG("Kymera_ConnectAudioOutputToAec");

    PanicNotZero(kymera_IsAudioOutputConnected() || (params == NULL) || (config == NULL));
    kymera_SetSpkSampleRate(config->spk_sample_rate);
    if (kymera_GetAecChain() == NULL)
    {
        kymera_CreateAecChain(config);
    }
    else
    {
        kymera_ConfigureAecChain();
    }
    kymera_AdditionalConfigureForAecAudioOutput();
    kymera_ConnectAudioOutput(params);
}

void Kymera_DisconnectAudioOutputFromAec(void)
{
    if(kymera_GetAecChain() != NULL)
    {
        PanicFalse(kymera_IsAudioOutputConnected());
        kymera_SetSpkSampleRate(0);
        kymera_DisconnectAudioOutput();
        if (kymera_IsAecConnected() == FALSE)
        {
            kymera_DestroyAecChain();
        }
    }
}

void Kymera_ConnectAudioInputToAec(const aec_connect_audio_input_t *params, const aec_audio_config_t* config)
{
    DEBUG_LOG("Kymera_ConnectAudioInputToAec");

    PanicNotZero((params == NULL) || (config == NULL));
    kymera_SetMicSampleRate(config->mic_sample_rate);
    if (kymera_IsAecConnected() == FALSE)
    {
        kymera_CreateAecChain(config);
    }
    else
    {
        kymera_ConfigureAecChain();
    }
    kymera_AdditionalConfigureForAecAudioInput();
    kymera_ConnectAudioInput(params);
}

void Kymera_ConnectToAecMicPathInput(const aec_mic_path_input_t *params, const aec_audio_config_t* config)
{
    DEBUG_LOG("Kymera_ConnectToAecMicPathInput");

    PanicNotZero((params == NULL) || (config == NULL));
    kymera_SetMicSampleRate(config->mic_sample_rate);
    if (kymera_IsAecConnected() == FALSE)
    {
        kymera_CreateAecChain(config);
    }
    else
    {
        kymera_ConfigureAecChain();
    }
    kymera_AdditionalConfigureForAecAudioInput();
    kymera_ConnectMicPathInput(params);
}

void Kymera_ConnectToAecMicPathOutput(const aec_mic_path_output_t *params)
{
    DEBUG_LOG("Kymera_ConnectToAecMicPathOutput");
    PanicNotZero((params == NULL));
    // Mic path input must have already been connected
    PanicFalse(kymera_IsMicPathInputConnected());
    kymera_ConfigureAecChain();
    kymera_ConnectMicPathOutput(params);
}

void Kymera_DisconnectAudioInputFromAec(void)
{
    if (kymera_GetAecChain())
    {
        kymera_SetMicSampleRate(0);
        kymera_DisconnectAudioInput();
        if (kymera_IsAecConnected() == FALSE)
        {
            kymera_DestroyAecChain();
        }
    }
}

void Kymera_DisconnectSelectedAudioInputFromAec(const aec_disconnect_audio_input_t *params)
{
    DEBUG_LOG("Kymera_DisconnectSelectedAudioInputFromAec");
    PanicNotZero(params == NULL);
    PanicFalse(kymera_IsAudioInputConnected());
    kymera_DisconnectSelectedAudioInput(params);
}

void Kymera_AecEnableSidetonePath(bool enable)
{
    Operator aec_ref = Kymera_GetAecOperator();
    if (aec_ref != INVALID_OPERATOR)
    {
        set_params_data_t* set_params_data = OperatorsCreateSetParamsData(1);

        DEBUG_LOG("Kymera_AecEnableSidetonePath: SidetonePath = %d", enable);
        set_params_data->number_of_params = 1;
        set_params_data->standard_params[0].id = AEC_REF_CONFIG_PARAM_INDEX;
        set_params_data->standard_params[0].value =(enable) ?
                    ((uint32)AEC_REF_CONFIG_PARAM_ENABLE_SIDETONE):((uint32)AEC_REF_CONFIG_PARAM_DEFAULT);

        OperatorsStandardSetParameters(aec_ref, set_params_data);
        free(set_params_data);
    }
}

void Kymera_AecSetSidetoneGain(uint32 exponent_value, uint32 mantissa_value)
{
    Operator aec_ref =kymera_GetAecOperator();
    if (aec_ref != INVALID_OPERATOR)
    {
        set_params_data_t* set_params_data = OperatorsCreateSetParamsData(2);

        set_params_data->number_of_params = 2;
        set_params_data->standard_params[0].id = AEC_REF_STF_GAIN_EXP_PARAM_INDEX;
        set_params_data->standard_params[0].value = exponent_value;
        set_params_data->standard_params[1].id = AEC_REF_STF_GAIN_MANTISSA_PARAM_INDEX;
        set_params_data->standard_params[1].value = mantissa_value;

        OperatorsStandardSetParameters(aec_ref, set_params_data);
        free(set_params_data);
    }
}

Operator Kymera_GetAecOperator(void)
{
    return kymera_GetAecOperator();
}

void Kymera_SetAecUseCase(aec_usecase_t usecase)
{
    DEBUG_LOG("Kymera_SetAecUseCase: enum:aec_usecase_t:%d", usecase);
    aec_config.use_case = usecase;
}

aec_usecase_t Kymera_GetAecUseCase(void)
{
    DEBUG_LOG("Kymera_GetAecUseCase: enum:aec_usecase_t:%d", aec_config.use_case);
    return aec_config.use_case;
}

void Kymera_AecSleep(void)
{
    ChainSleep(aec_chain, NULL);
}

void Kymera_AecWake(void)
{
    ChainWake(aec_chain, NULL);
}

bool Kymera_AecIsMicPathInputConnected(void)
{
    return kymera_IsMicPathInputConnected();
}

bool Kymera_AecIsSpeakerPathConnected(void)
{
    return kymera_IsAudioOutputConnected();
}

bool Kymera_AecSetTtpDelayBeforeConnection(uint32 ttp_delay)
{
    DEBUG_LOG("Kymera_AecSetTtpDelayBeforeConnection: before %u us requested %u us",aec_config.ttp_delay, ttp_delay);

    DEBUG_LOG("Kymera_AecSetTtpDelayBeforeConnection %u %u %u %u",(kymera_GetAecOperator() == INVALID_OPERATOR) ,
     (ttp_delay != 0),
     (aec_config.ttp_delay == 0) ,
      (aec_config.ttp_delay == ttp_delay));
    if ((kymera_GetAecOperator() == INVALID_OPERATOR) &&
        (ttp_delay != 0) &&
        ((aec_config.ttp_delay == 0) || (aec_config.ttp_delay == ttp_delay)))
    {
        aec_config.ttp_delay = ttp_delay;
        DEBUG_LOG("Kymera_AecSetTtpDelayBeforeConnection: after %u us", ttp_delay);
        return TRUE;
    }
    return FALSE;
}

