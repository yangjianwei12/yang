/*!
Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   dmlib_get_local_irk.c

\brief  Get Local IRK of the device

*/

#include INC_DIR(dmlib,dmlib_private.h)
#include INC_DIR(sm,sm.h)

/**
 * Retrives the local IRK of the device. If DM is able to find a valid IRK then
 * it is copied and returned in irk.
 *
 * This function fetches the local IRK value from SM Module.
 *
 * \param irk Pointer to packed_irk structure where the valid local IRK is copied.
 * \return TRUE if valid local IRK is found, otherwise FALSE.
 */

bool_t dm_get_local_irk(DM_SM_KEY_ID_T *irk)
{
#if defined (INSTALL_SM_PRIVACY_1P2) || defined(BUILD_FOR_HOST)
    uint16_t *local_irk = sm_get_local_irk();
    if (irk != NULL && !sm_is_buf_zero(local_irk, SM_SIZE16_128 * sizeof(uint16_t)))
    {
        qbl_memscpy(irk->irk, sizeof(irk->irk), 
                local_irk, SM_SIZE16_128 * sizeof(uint16_t));
        return TRUE;
    }
    else
#else
    QBL_UNUSED(irk);
#endif
    {
    /* Local IRK has not been initialized yet due to unavailability of ER
     * and IR root keys.
     */
        return FALSE;
    }
}

