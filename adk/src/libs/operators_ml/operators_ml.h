/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Low level operator functions
 
This is temporary file to keep ML code out of releases for time being.
When time is right it should be moved to operators.h.
 
*/

#ifndef OPERATORS_ML_H_
#define OPERATORS_ML_H_

#include <operator.h>

/*@{*/

/*!
 * \brief Loads the model associated with the usecase.
 * 
 * \param op operator using the model
 * \param use_case_id use case id
 * \param file_index file index corresponding to the model file in the Read-Only filesystem
 * \param batch_reset_count batch reset count value
 * \param access_method model file access method type
 * 
 * \return File Id from the DSP file manager. This reference is necessary when unloading the model
*/
DataFileID OperatorsMlLoadModel(Operator op, uint16 use_case_id, uint16 file_index, uint16 batch_reset_count, uint16 access_method);

/*!
 * \brief Activates the model for the usecase.
 * \param op operator using the model
 * \param use_case_id use case id
*/
void OperatorsMlActivateModel(Operator op, uint16 use_case_id);

/*!
 * \brief Unloads the model associated with the usecase.
 * \param op operator using the model
 * \param use_case_id use case id
 * \param file_id File Id from the DSP file manager. Obtained when loading the model
*/
void OperatorsMlUnloadModel(Operator op, uint16 use_case_id, DataFileID file_id);

/*@}*/

#endif /* OPERATORS_ML_H_ */
