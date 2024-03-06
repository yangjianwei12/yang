/*******************************************************************************

Copyright (C) 2010 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_change_local_name
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_CHANGE_LOCAL_NAME_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_change_local_name(
    uint8_t *sz_name,   /* Nul-terminated name string */
    DM_UPRIM_T **pp_prim
    )
{
    uint16_t len;
    uint16_t i;
    uint8_t block;

    DM_HCI_CHANGE_LOCAL_NAME_REQ_T *p_prim = zpnew(DM_HCI_CHANGE_LOCAL_NAME_REQ_T);
    p_prim->common.op_code = DM_HCI_CHANGE_LOCAL_NAME_REQ;

    /* Get the name length. Due to the way the following loop works, reduce the
     * length to 247 if the name is the maximum size.
     */
    len = (uint16_t) strlen((char *)sz_name);
    if (len >= 248)
    {
        len = 247;
    }

    /* The following loop terminates after (len + 1) characters, to ensure that
     * there is always a terminating 0 character if the name length is an exact
     * multiple of the number of bytes per pointer.
     * Note: While iterating for the blocks the string terminations need to be
     * done only at the end of sz_name and not at every block iteration.
     */
    for (block = 0, i = 0; i < len + 1; i += HCI_LOCAL_NAME_BYTES_PER_PTR)
    {
        size_t len_remaining = len - i;
        p_prim->name_part[block] = 
            (uint8_t *) pmalloc(HCI_LOCAL_NAME_BYTES_PER_PTR);

        if(len_remaining < HCI_LOCAL_NAME_BYTES_PER_PTR)
        {
            /* memset only when needed */
            memset(p_prim->name_part[block], 0, HCI_LOCAL_NAME_BYTES_PER_PTR);
            qbl_memscpy(p_prim->name_part[block], HCI_LOCAL_NAME_BYTES_PER_PTR,
                    (sz_name + i),len_remaining);
        }
        else
        {
            qbl_memscpy(p_prim->name_part[block], HCI_LOCAL_NAME_BYTES_PER_PTR,
                    (sz_name + i), HCI_LOCAL_NAME_BYTES_PER_PTR);
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

