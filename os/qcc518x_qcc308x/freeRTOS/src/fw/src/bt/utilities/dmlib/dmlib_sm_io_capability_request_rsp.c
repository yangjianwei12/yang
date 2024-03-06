/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_sm_io_capability_request_res
 *
 *  DESCRIPTION
 *      Build and send a DM_SM_IO_CAPABILITY_REQUEST_RSP primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_sm_io_capability_request_rsp(
    TP_BD_ADDR_T    *tp_addrt,
    uint8_t     io_capability,
    uint8_t     authentication_requirements,
    uint8_t     oob_data_present,
    uint8_t     *oob_hash_c,
    uint8_t     *oob_rand_r,
    uint16_t    key_distribution,
    DM_UPRIM_T  **pp_prim
    )
{
    uint8_t len_hash = 0;
    uint8_t len_rand = 0;
    DM_SM_IO_CAPABILITY_REQUEST_RSP_T *p_prim
                                    = zpnew(DM_SM_IO_CAPABILITY_REQUEST_RSP_T);

    p_prim->type = DM_SM_IO_CAPABILITY_REQUEST_RSP;
    p_prim->io_capability = io_capability;
    p_prim->authentication_requirements = authentication_requirements;
    if ((p_prim->oob_data_present = oob_data_present) != DM_SM_OOB_DATA_NONE)
    {
        if( p_prim->oob_data_present == 
            (DM_SM_OOB_DATA_P256 | DM_SM_OOB_DATA_P192) )
        {
            len_hash = SIZE_OOB_DATA*2;
            if(tp_addrt->tp_type == BREDR_ACL)
            {
                len_rand = SIZE_OOB_DATA*2;
            }
            else
            {
                len_rand = SIZE_OOB_DATA;
            }
        }
        else if(p_prim->oob_data_present == DM_SM_OOB_DATA_P256)
        {
            len_hash = SIZE_OOB_DATA;
            len_rand = SIZE_OOB_DATA;
        }
        else if(p_prim->oob_data_present == DM_SM_OOB_DATA_P192)
        {
            len_hash = SIZE_OOB_DATA;
            if(tp_addrt->tp_type == BREDR_ACL)
            {
                len_rand = SIZE_OOB_DATA;
            }
        }
        else
        {
            /* out of allowed values so free it and return */
            if(oob_hash_c)
                pfree(oob_hash_c);
            if(oob_rand_r)
                pfree(oob_rand_r);
            pfree(p_prim);
            return;
        }
        /* 
         * This ensures that the right length of data is being allocated and sent
         * down to the application irrespective of what was passed into this 
         * interface API framing function. Any API translation if involved will be 
         * able to assume the length of the data being passed as per the structure.
         */
        p_prim->oob_hash_c = (uint8_t*) pcopy(oob_hash_c, len_hash);
        if(len_rand)
            p_prim->oob_rand_r = (uint8_t*) pcopy(oob_rand_r, len_rand);
    }

    if(oob_hash_c)
        pfree(oob_hash_c);
    if(oob_rand_r)
        pfree(oob_rand_r);

    p_prim->key_distribution = key_distribution;
    p_prim->tp_addrt.tp_type = tp_addrt->tp_type;
    tbdaddr_copy(&p_prim->tp_addrt.addrt, &tp_addrt->addrt);

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

