/*******************************************************************************

Copyright (C) 2008 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:          $Revision: #2 $

*******************************************************************************/

#include "sdplib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdp_free_primitive
 *
 *  DESCRIPTION
 *      Free downstream and upstream SDP primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void sdp_free_primitive(
    SDS_UPRIM_T *p_uprim
    )
{
    if (!p_uprim)
        return;

    switch (p_uprim->type)
    {
        case SDC_SERVICE_SEARCH_REQ:
        case SDC_SERVICE_ATTRIBUTE_REQ:
        case SDC_SERVICE_SEARCH_ATTRIBUTE_REQ:
        case SDS_REGISTER_REQ:
            {
                sdp_free_downstream_primitive(p_uprim);
                break;
            }
        case SDC_SERVICE_SEARCH_CFM:
        case SDC_SERVICE_ATTRIBUTE_CFM:
        case SDC_SERVICE_SEARCH_ATTRIBUTE_CFM:
            {
                sdp_free_upstream_primitive(p_uprim);
                break;
            }

        default:
            {
                primfree(SDP_PRIM, p_uprim);
                break;
            }
    }
}

