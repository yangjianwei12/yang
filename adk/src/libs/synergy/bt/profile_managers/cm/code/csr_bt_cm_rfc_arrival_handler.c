/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef EXCLUDE_CSR_BT_RFC_MODULE

#include "csr_synergy.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#include "csr_bt_cm_dm.h"

#ifndef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE
#include "csr_bt_cm_bccmd.h"
#endif

/***************************************************************************
  CsrBtCmRfcArrivalHandler:
****************************************************************************/
void CsrBtCmRfcArrivalHandler(cmInstanceData_t *cmData)
{
    RFCOMM_UPRIM_T  *rfcPrim;
    rfcPrim = (RFCOMM_UPRIM_T *)cmData->recvMsgP;

    switch(rfcPrim->type)
    {
#ifndef CSR_STREAMS_ENABLE
        case RFC_DATAREAD_IND :
        {
            CsrBtCmRfcDataIndHandler(cmData);
            break;
        }
        case RFC_DATAWRITE_CFM :
        {
            CsrBtCmRfcDataWriteCfmHandler(cmData);
            break;
        }
#endif
        case RFC_PORTNEG_IND:
        {
            CsrBtCmRfcPortNegIndHandler(cmData);
            break;
        }
        case RFC_PORTNEG_CFM:
        {
            CsrBtCmRfcPortNegCfmHandler(cmData);
            break;
        }
        case RFC_MODEM_STATUS_CFM:
        {
            CsrBtCmRfcModemStatusCfmHandler(cmData);
            break;
        }
        case RFC_MODEM_STATUS_IND :
        {
            CsrBtCmRfcModemStatusIndHandler(cmData);
            break;
        }
        case RFC_DISCONNECT_CFM:
        {
            CsrBtCmRfcReleaseIndHandler(cmData);
            break;
        }
        case RFC_DISCONNECT_IND :
        {
            CsrBtCmRfcReleaseIndHandler(cmData);
            break;
        }
        case RFC_SERVER_CONNECT_IND:
        {
            CsrBtCmRfcServerConnectIndHandler(cmData);
            break;
        }
        case RFC_SERVER_CONNECT_CFM:
        {
            CsrBtCmRfcServerConnectCfmHandler(cmData);
            break;
        }
        case RFC_CLIENT_CONNECT_CFM:
        {
            CsrBtCmRfcClientConnectCfmHandler(cmData);
            break;
        }
        case RFC_UNREGISTER_CFM:
        {
            break;
        }
        case RFC_REGISTER_CFM :
        {
            CsrBtCmRfcRegisterCfmHandler(cmData);
            break;
        }
        case RFC_INIT_CFM:
        {
            /* There is no such BCCMD on QC chips so directly go ahead 
                with resigration */
#ifdef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE

#ifndef CSR_TARGET_PRODUCT_VM
            cmData->rfcBuild = FALSE;
#endif /* ifndef CSR_TARGET_PRODUCT_VM */
            CmInitSequenceHandler(cmData,
                                  CM_INIT_RFC_INIT_CFM,
                                  CSR_BT_RESULT_CODE_CM_SUCCESS,
                                  CSR_BT_SUPPLIER_CM);

#else
            CsrBccmdReadPsValueReqSend(CSR_BT_CM_IFACEQUEUE,
                                       0x0000, /* seqno */
                                       PSKEY_ONCHIP_HCI_CLIENT,
                                       CSR_BCCMD_STORES_DEFAULT,
                                       sizeof(CsrUint16));
#endif /* ifdef EXCLUDE_CSR_BT_CM_BCCMD_FEATURE */
            break;
        }
        case RFC_LINESTATUS_IND:
        {
            break;
        }
        case RFC_LINESTATUS_CFM:
        {
            break;
        }
#ifndef CSR_TARGET_PRODUCT_VM
        case RFC_TEST_CFM:
        {
            RFC_TEST_CFM_T *ptr = (RFC_TEST_CFM_T *) cmData->recvMsgP;
            
            CsrMblkDestroy(ptr->test_data);
            ptr->test_data = NULL;
            break;
        }
#endif
        case RFC_ERROR_IND:
        {
            break;
        }
        case RFC_NSC_IND:
        {
            break;
        }

#ifdef CSR_AMP_ENABLE
        case RFC_L2CA_MOVE_CHANNEL_CFM:
        {
            CsrBtCmRfcAmpMoveChannelCfmHandler(cmData);
            break;
        }
        case RFC_L2CA_MOVE_CHANNEL_CMP_IND:
        {
            CsrBtCmRfcAmpMoveChannelCmpIndHandler(cmData);
            break;
        }
        case RFC_L2CA_MOVE_CHANNEL_IND:
        {
            CsrBtCmRfcAmpMoveChannelIndHandler(cmData);
            break;
        }
        case RFC_L2CA_AMP_LINK_LOSS_IND:
        {
            CsrBtCmRfcAmpLinkLossIndHandler(cmData);
            break;
        }
#endif

        default:
        {
            CsrBtCmGeneralException(RFCOMM_PRIM,
                                    rfcPrim->type,
                                    cmData->globalState,
                                    "");
            break;
        }
    }

    rfc_free_primitive(cmData->recvMsgP);
    cmData->recvMsgP = NULL;
}

#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */

