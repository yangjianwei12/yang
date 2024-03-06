/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to machine learning kymera chain.
*/

#include "ml.h"
#include "ml_chain_roles.h"
#include "chains/chain_ml.h"

#include <operators_ml.h>
#include <logging.h>
#include <chain.h>
#include <vmal.h>
#include <panic.h>

static const capability_bundle_t capability_bundle[] =
{
    {
        "download_ml_example_svad.dkcs",
        capability_load_to_p0_use_on_p0_only
    },
    {
        "download_ml_engine_lib.edkcs",
        capability_load_to_p0_use_on_p0_only
    },
    {
        0, 0
    }
};

static const capability_bundle_config_t bundle_config = {capability_bundle, ARRAY_DIM(capability_bundle) - 1};

static void Configure_MlExampleSvad(kymera_chain_handle_t chain, Task listner, const ml_config_t *ml_config)
{
    Operator ml_example_svad = PanicZero(ChainGetOperatorByRole(chain, OPR_ML_EXAMPLE_SVAD));

    MessageOperatorTask(ml_example_svad, listner);

    FILE_INDEX model_file = FileFind(FILE_ROOT, ml_config->model_file,strlen(ml_config->model_file));
    if(model_file == FILE_NONE)
    {
        Panic();
    }

    OperatorsMlLoadModel(ml_example_svad, ml_config->use_case_id, model_file ,ml_config->batch_reset_count,
                         ml_config->access_method);

    OperatorsMlActivateModel(ml_example_svad, ml_config->use_case_id);
}

void Ml_Start(Source input, Task listener, const ml_config_t *ml_config)
{
    ChainSetDownloadableCapabilityBundleConfig(&bundle_config);

    kymera_chain_handle_t ml_svad_chain = PanicNull(ChainCreate(&chain_ml_config));

    Configure_MlExampleSvad(ml_svad_chain, listener, ml_config);

    /* Connect chain */
    ChainConnect(ml_svad_chain);
    ChainConnectInput(ml_svad_chain, input, EPR_PCM_IN);

    /* Start chain */
    ChainStart(ml_svad_chain);
}
