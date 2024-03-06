/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_dm.h"

void CsrBtCmL2capStoreConfigOptions(L2CA_TP_CONFIG_T *config, cmL2caConnInstType *thelink)
{
    if(thelink->transportType == LE_ACL)
    {
        thelink->outgoingMtu = config->remote_mtu;
        thelink->incomingMtu = config->mtu;
    }
    else 
    {
        if (config->options & L2CA_SPECIFY_MTU)
        {
            thelink->outgoingMtu = config->mtu;
        }
        else
        {
            thelink->outgoingMtu = L2CA_MTU_DEFAULT;
        }
    }

    if (config->options & L2CA_SPECIFY_FLUSH)
    {
        thelink->outgoingFlush = config->flush_to;
    }
    else
    {
        thelink->outgoingFlush = L2CA_FLUSH_TO_DEFAULT;
    }

    if ((config->options & L2CA_SPECIFY_FLOW) &&
        (config->flow != NULL) &&
        (config->flow->mode == L2CA_FLOW_MODE_ENHANCED_RETRANS))
    {
        thelink->ertm = TRUE;

        CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(thelink, config->flow->tx_window);

        if((config->options & L2CA_SPECIFY_EXT_WINDOW) && (config->flow->tx_window == 0))
        {
            /* use ext win size, overrules flow win ctrl size */
            CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(thelink, config->ext_window);
        }
    }
    else if (thelink->dataPriority == CSR_BT_CM_PRIORITY_HIGH)
    {
        thelink->ertm = FALSE;;
        CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(thelink,
                                         MAX_BUFFERED_L2CAP_HIGH_PRIORITY_REQUESTS);
    }
    else
    {
        thelink->ertm = FALSE;;
        CSR_BT_CM_L2CAP_TX_MAX_COUNT_SET(thelink, MAX_BUFFERED_L2CAP_REQUESTS);
    }
}
#endif /* EXCLUDE_CSR_BT_L2CA_MODULE */

