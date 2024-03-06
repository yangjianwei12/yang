/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

/*! \brief Send L2CA_CONFIG_REQ using config structure

    Build and send an L2CA_CONFIG_REQ primitive to L2CAP using
    config settings from a L2CA_CONFIG_T structure.

    Note: This function will resume ownership of the pointers in the
    L2CA_CONFIG_T, but will NOT do it for the structure itself!
*/
#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT
void L2CA_ConfigReqCs(l2ca_cid_t cid,
                      L2CA_CONFIG_T *config)
{
    L2CA_CONFIG_REQ_T *prim = pnew(L2CA_CONFIG_REQ_T);

    prim->type = L2CA_CONFIG_REQ;
    prim->cid = cid;

    /* Copy config structure */
    qbl_memscpy(&prim->config, sizeof(L2CA_CONFIG_T), config, sizeof(L2CA_CONFIG_T));

    /* Clear pointers and lengths in old structure */
    config->unknown = NULL;
    config->unknown_length = 0;
    config->qos = NULL;
    config->flow = NULL;
    config->flowspec = NULL;

    L2CA_PutMsg(prim);
}
#endif 
