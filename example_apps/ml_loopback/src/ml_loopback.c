/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to machine learning Kymera chain

*/

#include "ml_loopback.h"
#include <panic.h>
#include <vmal.h>
#include <operator.h>
#include <stream.h>
#include <cap_id_prim.h>

#define AUDIO_DATA_FORMAT_FIXP 1

static bool ConfigureML(Operator ml_ex_cap, const ml_config_t *ml_config)
{
    bool status = TRUE;

    /* Check for the machine learning model file */
    FILE_INDEX model_index = FileFind(FILE_ROOT, ml_config->model_file, strlen(ml_config->model_file));
    if(model_index == FILE_NONE)
    {
        status = FALSE;
        return status;
    }

    /* Load the model file */
    DataFileID model_handle = OperatorDataLoadEx(model_index, DATAFILE_BIN, STORAGE_ADD_HEAP, TRUE);
    if (model_handle == DATA_FILE_ID_INVALID)
    {
        status = FALSE;
        return status;
    }

    /* machine learning operator messages */
    /* Load model */
    uint16 load_model[] = {OPMSG_ML_COMMON_LOAD_MODEL, ml_config->use_case_id, ml_config->batch_reset_count, model_handle, ml_config->access_method};
    /* Activate model */
    uint16 activate_model[] = {OPMSG_ML_COMMON_ACTIVATE_MODEL, ml_config->use_case_id};

    /* send messages */
    status = VmalOperatorMessage(ml_ex_cap, load_model,
                        sizeof(load_model)/sizeof(load_model[0]),
                        NULL, 0);
    if (status == FALSE)
    {
        return status;
    }
    status = VmalOperatorMessage(ml_ex_cap, activate_model,
                        sizeof(activate_model)/sizeof(activate_model[0]),
                        NULL, 0);
    return status;

}

void Ml_Start(audio_endpoint_t audio_endpoint, const ml_config_t *ml_config)
{
    /* Check whether the operator bundle file actually exists */
    FILE_INDEX index = FileFind(FILE_ROOT, ml_config->operator_bundle, strlen(ml_config->operator_bundle));
    if(index == FILE_NONE)
    {
        Panic();
    }

    BundleID bundle_id = OperatorBundleLoad(index,capability_load_to_p0_use_on_p0_only);

    if(bundle_id == BUNDLE_ID_INVALID)
    {
        Panic();
    }

    /* Check whether the download_ml_engine_lib operator bundle file exists */
    index = FileFind(FILE_ROOT, ml_config->dnld_ml_engine_lib_bundle, strlen(ml_config->dnld_ml_engine_lib_bundle));
    if(index == FILE_NONE)
    {
        Panic();
    }
    bundle_id = OperatorBundleLoad(index,capability_load_to_p0_use_on_p0_only);
    if(bundle_id == BUNDLE_ID_INVALID)
    {
        Panic();
    }

    /* Create download ml_engine_lib capability - Only one instance of this needs to be */
    /* instantiated irrespective of the number of machine learning capabilities in the  */
    /* usecase. Note: The download_ml_engine_lib capability should always be created    */
    /* before the first machine learning capability is created.                         */
    Operator dnld_ml_engine_lib = PanicZero(VmalOperatorCreate(CAP_ID_DOWNLOAD_ML_ENGINE));

    /* Create machine learning operator */
    Operator ml_ex_cap = PanicZero(VmalOperatorCreate( ml_config->capability_id));

    /* Downloads the machine learning model and configures the machine learning operator by sending operator messages */
    PanicFalse(ConfigureML(ml_ex_cap, ml_config));

    /* Connect the operator terminals to the audio endpoints */
    PanicNull(StreamConnect(audio_endpoint.source_ep,  StreamSinkFromOperatorTerminal(ml_ex_cap, 0)));
    PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(ml_ex_cap, 0), audio_endpoint.sink_ep));

    /* Start the operator */
    Operator op_list[] = {
        ml_ex_cap,
        dnld_ml_engine_lib,
    };

    PanicFalse(OperatorStartMultiple(sizeof(op_list)/sizeof(op_list[0]),op_list,NULL));
}
