/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/

#ifndef ML_STRUCT_H
#define ML_STRUCT_H

#include "buffer/buffer.h"

typedef void ML_CONTEXT;
typedef void ML_PROPERTY;

/**
 * @brief structure containing tensor information
 */
typedef struct MODEL_TENSOR_INFO {
    unsigned tensor_id;       /*!< Tensor id */
    unsigned data_length;     /*!< Tensor data length */
    unsigned is_filled;       /*!< Is the tensor filled */
    unsigned is_rolling;      /*!< Rolling tensor support*/
    uint8* data;              /*!< Tensor data */
    tCbuffer* cdata;          /*!< Circular buffer for rolling tensor support - required only for ip tensors*/
} MODEL_TENSOR_INFO ;

/**
 * @brief structure to be used to get the INPUT_TENSOR_INFO and OUTPUT_TENSOR_INFO properties.
 */
typedef struct ML_MODEL_TENSOR{

    unsigned num_tensors;          /*!<  Total no of Tensors  */
    MODEL_TENSOR_INFO *tensors;    /*!<  Tensors */

}ML_MODEL_TENSOR;

/**
 * @brief The USECASE_INFO to hold all the information for one usecase
 */
typedef struct USECASE_INFO
{
    unsigned usecase_id;
    ML_CONTEXT *ml_model_context;
    ML_MODEL_TENSOR input_tensor;
    ML_MODEL_TENSOR output_tensor;
    ML_MODEL_TENSOR persistent_tensor;
    unsigned batch_reset_count;
    unsigned current_batch_count;
    unsigned model_load_status;

}USECASE_INFO;

/*! \struct ML_ENGINE_NODE
    \brief Tensor info linked list to store input, output and persistent tensors across usecases.
*/
typedef struct ML_ENGINE_NODE
{
    uint16 node_id;          /*!< It can be a use-case id or a tensor id that is unique for a usecase. */
    void *p_data;            /*!< Pointer to the structure we want to add in linked list. */
    struct ML_ENGINE_NODE *p_next; /*!< Pointer to next MLE node */
}ML_ENGINE_NODE;


/**
 * @brief Enum for defining property variants
 */
typedef enum PropertyID {
    PROPERTY_INVALID = 0,
    INPUT_TENSOR_COUNT = 1,
    INPUT_TENSOR_INFO = 2,
    OUTPUT_TENSOR_COUNT = 3,
    OUTPUT_TENSOR_INFO = 4,
    PERSISTENT_TENSOR_COUNT = 5,
    PERSISTSENT_TENSOR_INFO = 6,
    ACTIVATE_USECASE = 7,
    RESET_USECASE = 8,
    COMPATIBLE_KEAI_VERSION = 9
} PropertyID;

/**
 * @brief Enum for copy model options
 */
typedef enum CopyOption {
    MODEL_DIRECT_ACCESS = 0,
    MODEL_COPY_AND_UNLOAD = 1,
    MODEL_COPY = 2,
} CopyOption;

#endif /* ML_STRUCT_H */
