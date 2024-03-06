/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef EXCLUDE_CSR_BT_L2CA_MODULE

#include "csr_synergy.h"
#include "csr_bt_cm_l2cap.h"

/***************************************************************************
  CsrBtCmL2CaArrivalHandler:
****************************************************************************/
void CsrBtCmL2CaArrivalHandler(cmInstanceData_t *cmData)
{ /* Identify the event type from the L2CAP layer and handle it */

    L2CA_UPRIM_T    *l2caPrim;

    l2caPrim = (L2CA_UPRIM_T *)cmData->recvMsgP;

    switch(l2caPrim->type)
    {
#ifndef CSR_STREAMS_ENABLE
        case L2CA_DATAWRITE_CFM :
        {
            CsrBtCmL2caDataWriteCfmHandler(cmData);
            break;
        }
        case L2CA_DATAREAD_IND :
        {
            CsrBtCmL2caDataReadIndHandler(cmData);
            break;
        }
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_DATA_ABORT
        case L2CA_DATAWRITE_ABORT_CFM:
        {
            CsrBtCmL2caDatawriteAbortCfmHandler(cmData);
            break;
        }
#endif
#endif /* !CSR_STREAMS_ENABLE */

        case L2CA_AUTO_CONNECT_CFM :
        {
            L2CA_AUTO_CONNECT_CFM_T *prim = (L2CA_AUTO_CONNECT_CFM_T*)cmData->recvMsgP;
            /* Convert L2CA_AUTO_CONNECT_CFM to L2CA_AUTO_TP_CONNECT_CFM */
            CmL2caConvertConnectCfmToTpPrim(cmData);
            CsrBtCmL2caAutoConnectCfmHandler(cmData);
            /* Free the L2CA_AUTO_TP_CONNECT_CFM prim */
            CsrPmemFree(cmData->recvMsgP);
            /* Update the received prim message */
            cmData->recvMsgP = prim;
            break;
        }
        case L2CA_AUTO_TP_CONNECT_CFM :
        {
            CsrBtCmL2caAutoConnectCfmHandler(cmData);
            break;
        }
        case L2CA_AUTO_CONNECT_IND:
        {
            CsrBtCmL2caAutoConnectIndHandler(cmData);
            break;
        }
        case L2CA_AUTO_TP_CONNECT_IND:
        {
            CsrBtCmL2caAutoConnectIndHandler(cmData);
            break;
        }
        case L2CA_ADD_CREDIT_CFM:
        {
            CmL2caAddCreditCfmHandler(cmData);
            break;
        }
        case L2CA_DISCONNECT_IND :
        {
            CsrBtCmL2caDisconnectIndHandler(cmData);
            break;
        }
        case L2CA_DISCONNECT_CFM :
        {
            CsrBtCmL2caDisconnectCfmHandler(cmData);
            break;
        }
        case L2CA_REGISTER_CFM :
        {
            CsrBtCmL2caRegisterCfmHandler(cmData);
            break;
        }
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_UNREGISTER
        case L2CA_UNREGISTER_CFM :
        {
            CsrBtCmL2caUnRegisterCfmHandler(cmData);
            break;
        }
#endif
        case L2CA_TIMEOUT_IND :
        {
            CsrBtCmL2caTimeoutIndHandler(cmData);
            break;
        }
#ifdef CSR_AMP_ENABLE
        case L2CA_MOVE_CHANNEL_IND:
        {
            CsrBtCmL2caMoveChannelIndHandler(cmData);
            break;
        }
        case L2CA_MOVE_CHANNEL_CFM:
        {
            CsrBtCmL2caMoveChannelCfmHandler(cmData);
            break;
        }
        case L2CA_MOVE_CHANNEL_CMP_IND:
        {
            CsrBtCmL2caMoveChannelCmpIndHandler(cmData);
            break;
        }
        case L2CA_AMP_LINK_LOSS_IND:
        {
            CsrBtCmL2caAmpLinkLossIndHandler(cmData);
            break;
        }
#endif
#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
        case L2CA_MAP_FIXED_CID_IND:
        {
            CsrBtCmL2caMapFixedCidIndHandler(cmData);
            break;
        }
        case L2CA_MAP_FIXED_CID_CFM:
        {
            CsrBtCmL2caMapFixedCidCfmHandler(cmData);
            break;
        }
        case L2CA_UNMAP_FIXED_CID_IND:
        {
            CsrBtCmL2caUnmapFixedCidIndHandler(cmData);
            break;
        }
#endif        
        case L2CA_BUSY_IND:
        {
            CsrBtCmL2caBusyIndHandler(cmData);
            break;
        }
        case L2CA_GET_CHANNEL_INFO_CFM:
        {
            CsrBtCmL2caGetChannelInfoCfmHandler(cmData);
            break;
        }
        case L2CA_PING_CFM:
        {
            CmL2caPingCfmHandler(cmData);
            break;
        }
#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
        case L2CA_PING_IND:
        {
            CmL2caPingIndHandler(cmData);
            break;
        }
#endif
        case L2CA_REGISTER_FIXED_CID_CFM:
        {
            CmL2caRegisterFixedCidCfmHandler(cmData);
            break;
        }
        default:
        {
            CsrBtCmGeneralException(L2CAP_PRIM,
                                    l2caPrim->type,
                                    cmData->globalState,
                                    "");
            break;
        }
    }

    L2CA_FreePrimitive(cmData->recvMsgP);
    cmData->recvMsgP = NULL;
}

#endif /* !EXCLUDE_CSR_BT_L2CA_MODULE */

