/*******************************************************************************

Copyright (C) 2008 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:          $Revision: #2 $

*******************************************************************************/

#include "sdplib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdp_free_upstream_primitive
 *
 *  DESCRIPTION
 *      Free upstream SDP primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void sdp_free_upstream_primitive(
    SDS_UPRIM_T *p_uprim
    )
{
    if (!p_uprim)
        return;

    switch (p_uprim->type)
    {
        case SDC_SERVICE_SEARCH_CFM:
            {
                SDC_SERVICE_SEARCH_CFM_T *prim;

                prim = (SDC_SERVICE_SEARCH_CFM_T *) p_uprim;
                pfree(prim->rec_list);
                pfree(prim->err_info);
                break;
            }
        case SDC_SERVICE_ATTRIBUTE_CFM:
            {
                SDC_SERVICE_ATTRIBUTE_CFM_T *prim;

                prim = (SDC_SERVICE_ATTRIBUTE_CFM_T *) p_uprim;
                pfree(prim->attr_list);
                pfree(prim->err_info);
                break;
            }
        case SDC_SERVICE_SEARCH_ATTRIBUTE_CFM:
            {
                SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *prim;

                prim = (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *) p_uprim;
                pfree(prim->attr_list);
                pfree(prim->err_info);
                break;
            }
        default:
            {
                break;
            }
    }

    primfree(SDP_PRIM, p_uprim);
}

