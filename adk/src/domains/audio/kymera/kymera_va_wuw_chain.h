/*!
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA Wake-Up-Word chain

*/

#ifndef KYMERA_VA_WUW_CHAIN_H_
#define KYMERA_VA_WUW_CHAIN_H_

#include "kymera.h"
#include "va_audio_types.h"

/*! \brief Parameters used to configure the VA Wake-Up-Word chain operators */
typedef struct
{
    Task       wuw_detection_handler;
    FILE_INDEX wuw_model;
    DataFileID (*LoadWakeUpWordModel)(wuw_model_id_t model);
    /*! Sets the time offset to send to the splitter at VAD trigger */
    uint16     engine_init_preroll_ms;
} va_wuw_chain_op_params_t;

/*! \brief Parameters used to create the VA Wake-Up-Word chain */
typedef struct
{
    kymera_va_wuw_chain_params_t chain_params;
    va_wuw_chain_op_params_t operators_params;
} va_wuw_chain_create_params_t;

/*! \brief Create VA Wake-Up-Word chain.
           Must be called after VA mic chain is instantiated, since it will connect to it.
    \param params Parameters used to create/configure the chain.
*/
void Kymera_CreateVaWuwChain(const va_wuw_chain_create_params_t *params);
void Kymera_DestroyVaWuwChain(void);

void Kymera_StartVaWuwChain(void);
void Kymera_StopVaWuwChain(void);

void Kymera_ConnectVaWuwChainToMicChain(void);

void Kymera_VaWuwChainSleep(void);
void Kymera_VaWuwChainWake(void);

void Kymera_VaWuwChainStartGraphManagerDelegation(void);
void Kymera_VaWuwChainStopGraphManagerDelegation(void);

/*! \brief Used to inform WuW chain about any DSP clock changes needed.
    \param active_mode The clock to use when WuW engine is not in full processing mode.
    \param trigger_mode The clock to use when WuW engine is in full processing mode.
    \return TRUE when VA GM controls the clock, FALSE otherwise.
*/
bool Kymera_VaWuwChainSetDspClock(audio_dsp_clock_type active_mode, audio_dsp_clock_type trigger_mode);

/*! \brief Used to get the metadata associated with a trigger from WuW engine.
           Should only be called after detection and after Graph Manager delegation is stopped.
    \param size The size of the metadata
    \return Pointer to the metadata, must be freed by the user.
*/
void * Kymera_VaWuwChainGetMetadata(size_t size);

#endif /* KYMERA_VA_WUW_CHAIN_H_ */
