/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \defgroup ml_op_interface ml_modules
 * \file  ml_op_interface.h
 *
 * ML Operator interface header
 *
 * The ML Operator interface provides a common interface for a Kymera capability
 * to access the kalimba machine learning operator library. The operators supported
 * are listed in the header file "ml_defines.h"(can be found at kalimba/lib/ml/common).
 *
 * Following is the sequence of step that one needs to follow to create
 * a ML layer with a specific operator.
 * Step 1: Create an instance of the ML operator interface using the API ml_op_intf_create().
 *         This API expects any configuration option for the layer as well as
 *         the number of input and output tensors expected by this layer.For
 *         multiple layers, one needs to create multiple such interfaces.
 * Step 2: Create input and output tensors for the layer. The Kalimba Tensor
 *         structure can be referred in "tensor_kalimba.h"(can be found at 
 *         kalimba/lib/ml/common). This steps also involves allocating data
 *         memory to store tensor data.
 * Step 3: Attach the input and output tensors to the layer using the API
 *         ml_op_intf_property() and the property ML_OP_INTF_PROP_ATTACH_INPUT_TENSOR
 *         to attach input tensors and the property ML_OP_INTF_PROP_ATTACH_OUTPUT_TENSOR
 *         to attach output tensors.
 * Step 4: Create the specific machine learning operator to be used for the layer.
 *         Each operator expects specific configuration parameters. These parameter
 *         needs to be populated while creating an operator. Please refer to the
 *         operator specific header file in kalimba/lib/ml/nnops.
 * Step 5: Load the operator to the interface created using the API ml_op_intf_load().
 *
 * Once input data is copied into the input tensors, the API ml_op_intf_execute() can be
 * used to run the operator associated with the layer.
 *
 * One can also use the API ml_op_intf_destroy() to destroy an instance of the Kymera
 * ML Operator interface.
 *
 * An example capability ML_NNOPS is provided which can be referred to
 * for better understanding of this whole sequence.
 */

#ifndef ML_OP_INTERFACE_H
#define ML_OP_INTERFACE_H 

/****************************************************************************
Include Files
*/
#include "ml_defines.h"

/**
 * Handle for ML operator instance
 */
typedef void* ml_oph_t;

/**
 * ML_OP_INTF_PROP properties
 *
 */
typedef enum ML_OP_INTF_PROP{
    ML_OP_INTF_PROP_ATTACH_INPUT_TENSOR = 2,  /* Attaches the tensor to the input tensor list */
    ML_OP_INTF_PROP_ATTACH_OUTPUT_TENSOR = 3, /* Attaches the tensor to the output tensor list */
} ML_OP_INTF_PROP;


/**
 * \brief Creates instance of ML operator interface
 *
 * \param h_ml_op[out]    ML layer handle
 * \param flags[in]       Configuration option for the layer
 * \param num_inputs[in]  Number of input tensors
 * \param num_outputs[in] Number of output tensors
 *
 * \return ML_SUCCESS if success, ML_FAIL otherwise
 */
ML_RESULT ml_op_intf_create(ml_oph_t* h_ml_op, unsigned flags,unsigned num_inputs, unsigned num_outputs);

/**
 * \brief Destroy instance of ml operator interface
 *
 * \param h_ml_op[in] ML layer handle
 *
 * \return ML_SUCCESS if success, ML_FAIL otherwise
 */
                            
ML_RESULT ml_op_intf_destroy(ml_oph_t h_ml_op);

/**
 * \brief Operator's forward inference execution
 *
 * \param h_ml_op[in] ML layer handle
 * \param op_type[in] Layer type to be executed
 *
 * \return ML_SUCCESS if success, ML_FAIL otherwise
 */
ML_RESULT ml_op_intf_execute(ml_oph_t h_ml_op, LAYER_TYPE op_type);

/**
 * \brief Loads a specific operator to the ml operator interface
 *
 * \param h_ml_op[in]  ML layer handle
 * \param op_layer[in] Specific operator structure
 * \param flags[in]   Configuration option for the layer
 *
 * \return ML_SUCCESS if success, ML_FAIL otherwise
 */
ML_RESULT ml_op_intf_load(ml_oph_t h_ml_op, void* op_layer, unsigned flags);

/**
 * \brief Set properties, input/output tensor list to the ml operator interface
 *
 * \param h_ml_op[in]   ML layer handle
 * \param get[in]       Indicates if a set or get operation is being performed :
 *                      1 - Query Information, 0 - Set Information
 * \param prop[in]      Identifies the property type being applied
 * \param prop_info[in] Identifies the property type information being applied or queried
 *
 * \return ML_SUCCESS if success, ML_FAIL otherwise
 */
ML_RESULT ml_op_intf_property(ml_oph_t h_ml_op, bool get, ML_OP_INTF_PROP prop, void* prop_info);

#endif /* ML_OP_INTERFACE_H */
