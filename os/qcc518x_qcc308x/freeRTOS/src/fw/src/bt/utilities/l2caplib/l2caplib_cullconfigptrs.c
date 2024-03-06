/*******************************************************************************

Copyright (C) 2009 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
/*! \brief Cull config pointer block

    Clear config members based on the "options" bitmask.
*/
void L2CA_CullConfigPtrs(L2CA_CONFIG_T *config)
{
    /* MTU */
    if(!(config->options & L2CA_SPECIFY_MTU))
    {
        config->mtu = 0;
    }

    /* Flush timeout */
    if(!(config->options & L2CA_SPECIFY_FLUSH))
    {
        config->flush_to = 0;
    }

    /* QoS - allocates structure if needed */
    if(!(config->options & L2CA_SPECIFY_QOS))
    {
        bpfree(config->qos);
        config->qos = NULL;
    }

    /* Flow control - allocates structure if needed */
    if(!(config->options & L2CA_SPECIFY_FLOW))
    {
        bpfree(config->flow);
        config->flow = NULL;
    }

    /* Frame check sequence */
    if(!(config->options & L2CA_SPECIFY_FCS))
    {
        config->fcs = 0;
    }

    /* FlowSpec - allocates structure if needed */
    if(!(config->options & L2CA_SPECIFY_FLOWSPEC))
    {
        bpfree(config->flowspec);
        config->flowspec = NULL;
    }
    
    /* Extended windows */
    if(!(config->options & L2CA_SPECIFY_EXT_WINDOW))
    {
        config->ext_window = 0;
    }
}
#endif 
