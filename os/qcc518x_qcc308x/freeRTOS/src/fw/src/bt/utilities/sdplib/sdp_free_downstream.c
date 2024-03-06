/*******************************************************************************

Copyright (C) 2008 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:          $Revision: #1 $

*******************************************************************************/

#include "sdplib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdp_free_downstream_primitive
 *
 *  DESCRIPTION
 *      Free downstream SDP primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void sdp_free_downstream_primitive(SDS_UPRIM_T *p_uprim)
{
    if (!p_uprim)
        return;

    switch (p_uprim->type)
    {
        case SDC_SERVICE_SEARCH_REQ:
            {
                SDC_SERVICE_SEARCH_REQ_T *prim;

                prim = (SDC_SERVICE_SEARCH_REQ_T *) p_uprim;
                pfree(prim->srch_pttrn);
                break;
            }
        case SDC_SERVICE_ATTRIBUTE_REQ:
            {
                SDC_SERVICE_ATTRIBUTE_REQ_T *prim;

                prim = (SDC_SERVICE_ATTRIBUTE_REQ_T *) p_uprim;
                pfree(prim->attr_list);
                break;
            }
        case SDC_SERVICE_SEARCH_ATTRIBUTE_REQ:
            {
                SDC_SERVICE_SEARCH_ATTRIBUTE_REQ_T *prim;

                prim = (SDC_SERVICE_SEARCH_ATTRIBUTE_REQ_T *) p_uprim;
                pfree(prim->attr_list);
                pfree(prim->srch_pttrn);
                break;
            }
        case SDS_REGISTER_REQ:
            {
                SDS_REGISTER_REQ_T *prim;

                prim = (SDS_REGISTER_REQ_T *) p_uprim;
                pfree(prim->service_rec);
                break;
            }
        default:
            {
                break;
            }
    }

    pfree(p_uprim);
}

