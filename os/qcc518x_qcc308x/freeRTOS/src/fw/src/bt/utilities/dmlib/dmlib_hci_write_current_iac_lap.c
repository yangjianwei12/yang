/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/* Spec defined max number of counts of IAC that can be set */
#define DMLIB_HCI_IAC_MAX_LAP_COUNTS 0x40

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_write_current_iac_lap
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_WRITE_CURRENT_IAC_LAP_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_write_current_iac_lap(
    uint8_t num_iac,
    uint24_t *a_iacs,   /* Array of IACs */
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_WRITE_CURRENT_IAC_LAP_REQ_T *p_prim = zpnew(DM_HCI_WRITE_CURRENT_IAC_LAP_REQ_T);
    uint8_t i, j;
    uint8_t block;

    p_prim->common.op_code = DM_HCI_WRITE_CURRENT_IAC_LAP_REQ;

    /* 
     * Minimum of what primitive allocation can support and spec allows to what
     * has been asked from the application 
     */
    p_prim->num_current_iac = num_iac = MIN(DMLIB_HCI_IAC_MAX_LAP_COUNTS, 
                            MIN(num_iac, HCI_IAC_LAP_PER_PTR*HCI_IAC_LAP_PTRS));
    for (block = 0, i = 0; i < num_iac; i += HCI_IAC_LAP_PER_PTR)
    {
        p_prim->iac_lap[block] = 
            (uint24_t *) pmalloc(sizeof(uint24_t) * HCI_IAC_LAP_PER_PTR);
        for (j = i; (j < i + HCI_IAC_LAP_PER_PTR) && (j < num_iac); j++)
        {
            p_prim->iac_lap[block][j - i] = a_iacs[j];
        }
        block++;
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

