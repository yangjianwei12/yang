/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Merge config pointer block

    Copy/merge configuration pointer blocks, possibly allocating them
    on the way. Note that we *merge* bits. We don't remove bits.
*/
void L2CA_MergeConfigPtrs(L2CA_CONFIG_T *target,
                          L2CA_CONFIG_T *source)
{
    target->options |= source->options;

    /* MTU */
    if(source->options & L2CA_SPECIFY_MTU)
    {
        target->mtu = source->mtu;
    }

    /* Flush timeout */
    if(source->options & L2CA_SPECIFY_FLUSH)
    {
        target->flush_to = source->flush_to;
    }

    /* QoS - allocates structure if needed */
    if(source->options & L2CA_SPECIFY_QOS)
    {
        if(target->qos == NULL)
        {
            target->qos = pnew(L2CA_QOS_T);
        }
        qbl_memscpy(target->qos, sizeof(L2CA_QOS_T), source->qos, sizeof(L2CA_QOS_T));
    }

    /* Flow control - allocates structure if needed */
    if(source->options & L2CA_SPECIFY_FLOW)
    {
        if(target->flow == NULL)
        {
            target->flow = pnew(L2CA_FLOW_T);
        }
        qbl_memscpy(target->flow, sizeof(L2CA_FLOW_T), 
                source->flow, sizeof(L2CA_FLOW_T));
    }

    /* Frame check sequence */
    if(source->options & L2CA_SPECIFY_FCS)
    {
        target->fcs = source->fcs;
    }

    /* FlowSpec - allocates structure if needed */
    if(source->options & L2CA_SPECIFY_FLOWSPEC)
    {
        if(target->flowspec == NULL)
        {
            target->flowspec = pnew(L2CA_FLOWSPEC_T);
        }
        qbl_memscpy(target->flowspec, sizeof(L2CA_FLOWSPEC_T),
                source->flowspec, sizeof(L2CA_FLOWSPEC_T));
    }
    
    /* Extended windows */
    if(source->options & L2CA_SPECIFY_EXT_WINDOW)
    {
        target->ext_window = source->ext_window;
    }   
}
#endif 
