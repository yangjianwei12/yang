/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to machine learning Kymera chain

*/

#ifndef ML_LOOPBACK_H
#define ML_LOOPBACK_H

#include "ml_loopback_setup_audio.h"
#include <message.h>

/*! Model access methods */
typedef enum
{
    MODEL_DIRECT_ACCESS = 0,
    MODEL_COPY_AND_AUTO_UNLOAD = 1,
    MODEL_COPY_AND_MANUAL_UNLOAD = 2
} MODEL_ACCESS_METHODS;

/*! Common Operator messages IDs for machine learning operator */
typedef enum
{
    OPMSG_ML_COMMON_LOAD_MODEL  = 0x14,
    OPMSG_ML_COMMON_UNLOAD_MODEL = 0x16,
    OPMSG_ML_COMMON_ACTIVATE_MODEL = 0x15
} OPMSG_ML_COMMON;

typedef struct
{
    uint16 use_case_id;
    uint16 capability_id;
    uint16 access_method;
    uint16 batch_reset_count;
    char *model_file;
    char *operator_bundle;
    char *dnld_ml_engine_lib_bundle;
} ml_config_t;

/*! \brief Configure and start ML loopback chain

    \param input     Audio endpoints
    \param input     Machine learning use-case config

*/
void Ml_Start(audio_endpoint_t audio_endpoint, const ml_config_t *ml_config);

#endif // ML_LOOPBACK_H
