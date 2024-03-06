/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_configure_cig_test_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_CONFIGURE_CIG_TEST_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Parameter cis_config is an array of pointer for CIS configuration,
 *      caller of this API shall pass the array with cis_count number elements
 *      with valid pointers. Number of CIS shall be atleast 1 and
 *      maximum shall be DM_MAX_SUPPORTED_CIS.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_configure_cig_test_req(
    phandle_t            phandle,
    context_t            context,
    uint24_t             sdu_interval_m_to_s,
    uint24_t             sdu_interval_s_to_m,
    uint16_t             iso_interval,
    uint8_t              cig_id,
    uint8_t              ft_m_to_s,
    uint8_t              ft_s_to_m,
    uint8_t              sca,
    uint8_t              packing,
    uint8_t              framing,
    uint8_t              cis_count,
    DM_CIS_CONFIG_TEST_T *cis_config[],
    DM_UPRIM_T **pp_prim
    )
{
    DM_ISOC_CONFIGURE_CIG_TEST_REQ_T *p_prim;
    uint8_t i;

    if (cis_count == 0 || cis_count > DM_MAX_SUPPORTED_CIS ||
        cis_config == NULL)
    {
        return;
    }

    p_prim = zpnew(DM_ISOC_CONFIGURE_CIG_TEST_REQ_T);
    p_prim->type = DM_ISOC_CONFIGURE_CIG_TEST_REQ;
    p_prim->phandle = phandle;
    p_prim->context = context;
    p_prim->sdu_interval_m_to_s = sdu_interval_m_to_s;
    p_prim->sdu_interval_s_to_m = sdu_interval_s_to_m;
    p_prim->iso_interval = iso_interval;
    p_prim->cig_id = cig_id;
    p_prim->ft_m_to_s = ft_m_to_s;
    p_prim->ft_s_to_m = ft_s_to_m;
    p_prim->sca = sca;
    p_prim->packing = packing;
    p_prim->framing = framing;
    p_prim->cis_count = cis_count;

    for (i = 0; i < cis_count; i++)
    {
        p_prim->cis_config_test[i] = cis_config[i];
        cis_config[i] = NULL;
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

