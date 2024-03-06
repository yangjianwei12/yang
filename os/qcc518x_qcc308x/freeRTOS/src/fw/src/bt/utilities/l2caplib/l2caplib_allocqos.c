/*******************************************************************************

Copyright (C) 2010 - 2018 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Allocate and return a best effort QoS structure
*/
#ifdef INSTALL_L2CAP_QOS_SUPPORT
L2CA_QOS_T *L2CA_AllocQoS(void)
{
    L2CA_QOS_T *qos = pnew(L2CA_QOS_T);
    qos->flags        = 0;
    qos->service_type = L2CA_QOS_DEFAULT_SERVICE_TYPE;
    qos->token_rate   = L2CA_QOS_DEFAULT_TOKEN_RATE;
    qos->token_bucket = L2CA_QOS_DEFAULT_TOKEN_BUCKET;
    qos->peak_bw      = L2CA_QOS_DEFAULT_PEAK_BW;
    qos->latency      = L2CA_QOS_DEFAULT_LATENCY;
    qos->delay_var    = L2CA_QOS_DEFAULT_DELAY_VAR;
    return qos;
}
#endif
