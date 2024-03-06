/*!
Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   dmlib_update_sm_key_state.c

\brief  Update root keys for the device.

*/

#include "dmlib_private.h"
#include INC_DIR(sm,sm_private.h)

/**
 * Handle root key update request from application.
 *
 * SM keys state contains ER and IR values, which are root keys of the device
 * for LE transport. These keys should be non zero and valid random numbers.
 * This request is ideally excepted only when two device required to
 * synchronise their root keys.
 *
 * \param sm_key_state Pointer to DM_SM_KEY_STATE_T.
 * \param cb sm_key_state update callback.
 * \return DM_SM_UPDATE_SM_KEY_STATUS Status of SM root key updation.
 *
 */
DM_SM_UPDATE_SM_KEY_STATUS dm_update_sm_key_state(DM_SM_KEY_STATE_T *sm_key_state,
                                                  dm_update_sm_key_state_cb cb)
{
    DM_SM_UPDATE_SM_KEY_STATUS status = DM_SM_UPDATE_KEY_STATUS_FAILED;

    if (sm_key_state != NULL && !sm_is_buf_zero(sm_key_state->er, SM_SIZE16_128 * sizeof(uint16_t))
     && !sm_is_buf_zero(sm_key_state->ir, SM_SIZE16_128 * sizeof(uint16_t)))
    {
        qbl_memscpy(&smcb.sm_key_state, sizeof(DM_SM_KEY_STATE_T), 
                sm_key_state, sizeof(DM_SM_KEY_STATE_T));
        /* Trigger new local IRK generation */
        sm_generate_local_irk();
#ifdef SM_HAS_FUNCTION_FOR_AES
        status = DM_SM_UPDATE_KEY_STATUS_SUCCESS;
#else
        smcb.cb = cb;
        status = DM_SM_UPDATE_KEY_STATUS_INPROGRESS;
#endif
    }
    return status;
}

