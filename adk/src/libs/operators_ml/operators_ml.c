/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Low level operator functions

This is temporary file to keep ML code out of releases for time being.
When time is right it should be moved to operators.c.
*/

#include "operators_ml.h"

#include <vmal.h>
#include <panic.h>
#include <operator.h>

/*! Common Operator messages IDs for machine learning operator */
typedef enum
{
    OPMSG_ML_COMMON_LOAD_MODEL  = 0x14,
    OPMSG_ML_COMMON_UNLOAD_MODEL = 0x16,
    OPMSG_ML_COMMON_ACTIVATE_MODEL = 0x15
} OPMSG_ML_COMMON;

DataFileID OperatorsMlLoadModel(Operator op, uint16 use_case_id, uint16 file_index, uint16 batch_reset_count, uint16 access_method)
{
    uint16 op_send_msg[5];

    DataFileID file_handle = PanicFalse(OperatorDataLoadEx(file_index, DATAFILE_BIN, STORAGE_ADD_HEAP, TRUE));

    op_send_msg[0] = OPMSG_ML_COMMON_LOAD_MODEL;
    op_send_msg[1] = use_case_id;
    op_send_msg[2] = batch_reset_count;
    op_send_msg[3] = file_handle;
    op_send_msg[4] = access_method;

    PanicFalse(VmalOperatorMessage(op, op_send_msg, SIZEOF_OPERATOR_MESSAGE(op_send_msg), NULL, 0));

    return file_handle;
}

void OperatorsMlActivateModel(Operator op, uint16 use_case_id)
{
    uint16 op_send_msg[2];

    op_send_msg[0] = OPMSG_ML_COMMON_ACTIVATE_MODEL;
    op_send_msg[1] = use_case_id;

    PanicFalse(VmalOperatorMessage(op, op_send_msg, SIZEOF_OPERATOR_MESSAGE(op_send_msg), NULL, 0));
}

void OperatorsMlUnloadModel(Operator op, uint16 use_case_id, DataFileID file_id)
{
    uint16 op_send_msg[3];
    
    op_send_msg[0] = OPMSG_ML_COMMON_UNLOAD_MODEL;
    op_send_msg[1] = use_case_id;
    op_send_msg[2] = file_id;
    
    /* OperatorDataUnloadEx trap not needed as this is done implicitly by audio when handling OPMSG_ML_COMMON_UNLOAD_MODEL */
    PanicFalse(VmalOperatorMessage(op, &op_send_msg, SIZEOF_OPERATOR_MESSAGE(op_send_msg), NULL, 0));
}
