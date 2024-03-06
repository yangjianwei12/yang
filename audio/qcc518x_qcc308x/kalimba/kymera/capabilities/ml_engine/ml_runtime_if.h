/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup ml_runtime ml_modules
 * \file  ml_runtime_if.h
 *
 * ML Runtime interface header
 *
 */

#ifndef ML_RUNTIME_INTERFACE_H
#define ML_RUNTIME_INTERFACE_H 

/****************************************************************************
Include Files
*/
#include "ml_struct.h"

/**
 * \brief Loads a single model file to RAM.
 *
 * \param usecase_id usecase-id of the model file
 * \param file_handle the file handle returned by OperatorDataLoadEx()
 * \param access_method Access method for the model.
 * \param ml_context the ML_CONTEXT pointer provided by the caller.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_load(unsigned usecase_id, unsigned file_handle, unsigned access_method, ML_CONTEXT **ml_context);

/**
 * \brief Frees the file manager data and removes the record for the given usecase-id
 *
 * \param usecase_id usecase-id of the model file
 * \param file_handle the file handle returned by OperatorDataLoadEx()
 * \return TRUE if success, FALSE otherwise
 */
bool ml_unload(unsigned usecase_id, unsigned file_handle);

/**
 * \brief Frees the context returned by ml_load()
 *
 * \param ml_context the context to be freed which was returned by ml_load()
 * \return TRUE if success, FALSE otherwise
 */
bool ml_free_context(ML_CONTEXT *ml_context);

/**
 * \brief A common interface to set the model property.
 *
 * \param ml_context the context returned by model_loader_load_model()
 * \param property_id an identifier for the property to be set.
 * \param property structure of the property to be set.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_set_config(ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property);

/**
 * \brief A common interface to get the model property.
 *
 * \param ml_context the context returned by model_loader_load_model()
 * \param property_id an identifier for the property to be set.
 * \param property structure of the property to be set.
 * \return TRUE if success, FALSE otherwise
 */
bool ml_get_config(ML_CONTEXT *ml_context, PropertyID property_id, ML_PROPERTY *property);

/**
 * \brief Executes the neural network model
 * \param mle_context context object containing parameters for this model
 * \return int status of inference
 */
int ml_execute(ML_CONTEXT *ml_context);

#endif /* ML_RUNTIME_INTERFACE_H */

