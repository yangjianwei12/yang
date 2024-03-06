/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  ml_nnops_layer.c
 * \ingroup  capabilities
 *
 * Source File for constructing NN Operator layer
 *
 */

/****************************************************************************
Include Files
*/
#include "ml_nnops_layer.h"
#include "all_operator_kalimba.h"
#include <stdfix.h>


/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Updates input and output tensorlist structure.
 *
 * \param h_ml_op ML layer handle
 *
 * \notes Creates input and output tensors. Also attaches them
 *        to the layer
 */
static bool ml_layer_update_tensorlists(ml_oph_t h_ml_op)
{
    /* Create 'Tensor' data structure for all tensors of a layer. In the
     * example provided here, we have a mul layer with two input tensors
     * i.e. TENSOR_ID_0 and TENSOR_ID_1 and a single output tensor TENSOR_ID_1.
     * See the header file 'ml_nnops_tensor_def.h' for further clarity.
     */
    Tensor* tensor_id_0 = xzpnew(Tensor);
    if(NULL == tensor_id_0)
    {
        return FALSE;
    }
    tensor_id_0->ndim = TENSOR_ID_0_NUM_DIMS;
    tensor_id_0->num_elems = TENSOR_ID_0_NUM_ELEMS;
    tensor_id_0->dims[tensor_id_0->ndim - 1] = TENSOR_ID_0_NUM_ELEMS;
    tensor_id_0->data_type = TENSOR_ID_0_DATA_TYPE;
    tensor_id_0->elem_size = sizeof(sat fract);
    unsigned stride = tensor_id_0->elem_size;
    for(int i = tensor_id_0->ndim-1; i>=0; --i)
    {
        tensor_id_0->strides[i] = stride;
        stride *= tensor_id_0->dims[i];
    }
    /* We create a linear buffer to store data for tensor_id_0 */
    tensor_id_0->data = (uint8 *)xzpnewn(TENSOR_ID_0_NUM_ELEMS, int32);
    if(NULL == tensor_id_0->data)
    {
        return FALSE;
    }

    Tensor* tensor_id_1 = xzpnew(Tensor);
    if(NULL == tensor_id_1)
    {
        return FALSE;
    }    
    tensor_id_1->ndim = TENSOR_ID_1_NUM_DIMS;
    tensor_id_1->num_elems = TENSOR_ID_1_NUM_ELEMS;
    tensor_id_1->dims[tensor_id_1->ndim - 1] = TENSOR_ID_1_NUM_ELEMS;
    tensor_id_1->data_type = TENSOR_ID_1_DATA_TYPE;
    tensor_id_1->elem_size = sizeof(sat fract);
    stride = tensor_id_1->elem_size;
    for(int i = tensor_id_1->ndim-1; i>=0; --i)
    {
        tensor_id_1->strides[i] = stride;
        stride *= tensor_id_1->dims[i];
    }
    /* We create a linear buffer to store data for tensor_id_1 */
    tensor_id_1->data = (uint8 *)xzpnewn(TENSOR_ID_1_NUM_ELEMS, int32);
    if(NULL == tensor_id_1->data)
    {
        return FALSE;
    }

    Tensor* tensor_id_2 = xzpnew(Tensor);
    if(NULL == tensor_id_2)
    {
        return FALSE;
    }
    tensor_id_2->ndim = TENSOR_ID_2_NUM_DIMS;
    tensor_id_2->num_elems = TENSOR_ID_2_NUM_ELEMS;
    tensor_id_2->dims[tensor_id_2->ndim - 1] = TENSOR_ID_2_NUM_ELEMS;
    tensor_id_2->data_type = TENSOR_ID_2_DATA_TYPE;
    tensor_id_2->elem_size = sizeof(sat fract);
    stride = tensor_id_2->elem_size;
    for(int i = tensor_id_2->ndim-1; i>=0; --i)
    {
        tensor_id_2->strides[i] = stride;
        stride *= tensor_id_2->dims[i];
    }
    /* We do not allocate data buffer for tensor_id_2, which is the output
     * tensor of the MUL operator, since we set the 'is_inplace' field of
     * the mul structure created.
     */
    tensor_id_2->data = NULL;

    /* Attach tensors to layer - we are setting a property 
     * Hence the flag 'get' is 0.
     */
    bool get = 0;

    if(ML_FAIL == ml_op_intf_property(h_ml_op, get,
                                      ML_OP_INTF_PROP_ATTACH_INPUT_TENSOR,
                                      tensor_id_0))
    {
        return FALSE;
    }
    if(ML_FAIL == ml_op_intf_property(h_ml_op, get,
                                      ML_OP_INTF_PROP_ATTACH_INPUT_TENSOR,
                                      tensor_id_1))
    {
        return FALSE;
    }
    if(ML_FAIL == ml_op_intf_property(h_ml_op, get,
                                      ML_OP_INTF_PROP_ATTACH_OUTPUT_TENSOR,
                                      tensor_id_2))
    {
        return FALSE;
    }
    
    return TRUE;
    
}
/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Creates machine learning layer
 *
 * \param ml_nnops_layer Pointer to the 'ML_NNOPS_LAYER' structure.
 *
 * \return TRUE is success, FALSE otherwise
 */
bool create_layer(ML_NNOPS_LAYER* ml_nnops_layer)
{
    /* Following are the steps needed to create any kymera layer with a
     * specific machine learning operator using the ML Operator interface.
     * Step1: Create the ML operator interface using the API ml_op_intf_create().
     *        We are also required to input the number of input and output
     *        tensors that this layer expects.
     * Step2: Create Input and output 'Tensors'.
     * Step3: Attach input and output tensors to the ML operator
     *        interface using the API ml_op_intf_property() and property
     *        ML_OP_INTF_PROP_ATTACH_INPUT_TENSOR for attaching input tensor
     *        and ML_OP_INTF_PROP_ATTACH_OUTPUT_TENSOR for attaching output
     *        tensor.
     * Step4: Create an operator for the layer.In the example shown here,
     *        we are creating a MUL operator.
     * Step5: Load the operator to the ML operator interface using the API
     *        ml_op_intf_load()
     */ 

    /* Create the ML Operator interface
     * No specific flags to begin with */
    unsigned flags = 0;
    if(ML_FAIL == ml_op_intf_create(&ml_nnops_layer->h_ml_op,flags,
                                    NUM_INPUT_TENSORS, NUM_OUTPUT_TENSORS))
    {
        return FALSE;
    }

    /* Step2/Step3: Create tensors and attach to the interface */
    if(!ml_layer_update_tensorlists(ml_nnops_layer->h_ml_op))
    {
        return FALSE;
    }

    /* Step4: Create an operator for the layer. Load it to the layer */
    Multiply *mul = xzpnew(Multiply);
    if(NULL == mul)
    {
        return FALSE;
    }
    /* No scaling for the first operand */
    mul->scale_A = 0;
    /* No scaling for the second operand */
    mul->scale_B = 0;
    /* First operand is a vector, second operand is a scalar */
    mul->kernel = KERNEL_VS;
    /* In place operation: data buffer for the output
     * tensor is same as one of the input tensor
     */
    mul->is_inplace = 1;

    /* Step5: Load the 'MUL' operator to the ML operator interface.
     *        Update the flags to indicate that 'mul' operation 
     *        will be inplace */
    flags = LAYER_IS_IN_PLACE;   
    
    if(ML_FAIL == ml_op_intf_load(ml_nnops_layer->h_ml_op, mul, flags))
    {
        return FALSE;
    }
    
    ml_nnops_layer->op_type = LAYER_TYPE_MUL; 
    ml_nnops_layer->next = NULL;
      
    return TRUE;
}

/**
 * \brief Destroys machine learning layer
 *
 * \param ml_nnops_layer Pointer to the 'ML_NNOPS_LAYER' structure.
 *
 * \return TRUE is success, FALSE otherwise
 */
bool destroy_layer(ML_NNOPS_LAYER* ml_nnops_layer)
{
    if(ML_FAIL == ml_op_intf_destroy(ml_nnops_layer->h_ml_op))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

