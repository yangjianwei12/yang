/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_engine.h
 * \ingroup capabilities
 *
 * MLE Capability public header file. <br>
 *
 */

#ifndef ML_ENGINE_H
#define ML_ENGINE_H

#include "../../capabilities/ml_engine/ml_engine_struct.h"

/* MLE Helper functions */

/**
 * \brief Helper function to reset the persistent tensors
 *
 * \param opx_data ML Engine extra operator data
 * \param usecase_id usecase identifier of the model
 */
extern void ml_engine_reset_buffer(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id);

/**
 * \brief Helper function to load the model file.
 *
 * \param opx_data ML Engine extra operator data.
 * \param usecase_id Usecase identifier for which to load model.
 * \param file_handle Kymera file handle to the downloaded model file.
 * \param auto_remove Flag to remove the file manager data.
 * \return TRUE if model load is successful else FALSE.
 */
extern bool ml_engine_load(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id, unsigned batch_reset_count, unsigned file_handle, unsigned auto_remove);

/**
 * \brief Helper function to activate the model file.
 *
 * \param opx_data ML Engine extra operator data.
 * \param usecase_id Usecase ID for which to activate the model.
 * \return TRUE if model activate is successful else FALSE.
 */
extern bool ml_engine_activate(ML_ENGINE_OP_DATA* opx_data, unsigned usecase_id);

/**
 * \brief Helper function to check the status of ML_ENGINE.
 *        This is used by other ML capabilities to check if
 *        ML_Engine is correctly instantiated.
 *
 * \return TRUE if ML_ENGINE is instantiated properly else FALSE
 */
extern bool ml_engine_check_status(void);

#endif /* ML_ENGINE_H */
