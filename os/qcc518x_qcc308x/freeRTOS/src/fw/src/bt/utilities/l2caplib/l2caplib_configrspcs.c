/*******************************************************************************

Copyright (C) 2009 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "l2caplib_private.h"

#ifndef DISABLE_L2CAP_CONNECTION_FSM_SUPPORT

/*! \brief Send L2CA_CONFIG_RSP using config structure

    Build and send an L2CA_CONFIG_RSP primitive to L2CAP using
    config settings from a L2CA_CONFIG_T structure.

    Note: This function will resume ownership of the pointers in the
    L2CA_CONFIG_T, but will NOT do it for the structure itself!
*/
void L2CA_ConfigRspCs(l2ca_cid_t cid,
                      l2ca_identifier_t identifier,
                      l2ca_conf_result_t response,
                      L2CA_CONFIG_T *config)
{
    L2CA_CONFIG_RSP_T *prim = pnew(L2CA_CONFIG_RSP_T);

    prim->type = L2CA_CONFIG_RSP;
    prim->cid = cid;
    prim->identifier = identifier;
    prim->response = response;

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
