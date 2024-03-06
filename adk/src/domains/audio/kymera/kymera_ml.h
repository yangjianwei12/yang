/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       kymera_ml.h
\brief      Kymera Machine Learning Engine API
*/

#ifndef KYMERA_ML_ENGINE_H
#define KYMERA_ML_ENGINE_H

/*! 
 * \brief Model access methods
 */
typedef enum
{
    model_direct_access = 0,
    model_copy_and_auto_unload = 1,
    model_copy_and_manual_unload = 2,
    model_access_method_invalid = -1
} model_access_methods_t;

/*!
 * \brief Configuration parameters for the Machine Learning Engine.
 *
 * Please refer to the Machine Learning Framework User Guide for details on
 * these parameters.
*/
typedef struct
{
    /*! Use case Id. Must match the value inside the model file. */
    uint16 use_case_id;
    /*! Access method used to access the data in the model file.*/
    model_access_methods_t access_method;
    /*! Number of batches after which to reset the internal buffers of the model */
    uint16 batch_reset_count;
    /*! Name of the file in the Read-Only filesystem that contains the model */
    char *model_file;
} kymera_ml_config_t;

#ifdef INCLUDE_AUDIO_ML_ENGINE
/*! \brief Creates an instance of the Machine Learning Engine capability
 *
 * Only one instance is needed. Machine Learning enabled capabilities can share the ML capability
 */
void Kymera_MlEngineCreate(void);

/*! \brief Destroys the Machine Learning Engine capability
 * 
 * All capabilities using the Machine Learning Engine must be destroyed first.
 */
void Kymera_MlEngineDestroy(void);

/*! \brief Activate a Machine Learning model
 * 
 * This action must be performed before starting the operator
 * 
 * \param op target operator
 * \param ml_config model configuration for the Machine Learning Engine
 */
void Kymera_MlActivate(Operator op, const kymera_ml_config_t *ml_config);

/*! \brief Deactivate Machine Learning processing
 * 
 * This action must be performed after stopping and before destroying the operator
 * 
 * \param op target operator
 */
void Kymera_MlDeactivate(Operator op);
#else
#define Kymera_MlEngineCreate()
#define Kymera_MlEngineDestroy()
#define Kymera_MlActivate(op, ml_config)    UNUSED(op);UNUSED(ml_config)
#define Kymera_MlDeactivate(op)             UNUSED(op)
#endif /* INCLUDE_AUDIO_ML_ENGINE */
#endif // KYMERA_ML_ENGINE_H
