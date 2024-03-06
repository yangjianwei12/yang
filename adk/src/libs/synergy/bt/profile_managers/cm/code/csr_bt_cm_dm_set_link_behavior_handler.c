/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "csr_synergy.h"
#ifdef CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

void CsrBtCmDmSetLinkBehaviorReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmDmSetLinkBehaviorReq *cmPrim = (CsrBtCmDmSetLinkBehaviorReq *) cmData->recvMsgP;
    CsrUint16 conftab_default[] = {
              0x8000,                                                   /* 0 Separator */
              DM_LINK_BEHAVIOR_L2CAP_RETRY,                             /* 1 Key */
              DM_LINK_BEHAVIOR_L2CAP_RETRY_ON,                          /* 2 Value */
              DM_LINK_BEHAVIOR_APP_HANDLE_L2CAP_PING,                   /* 3 Key */
              DM_LINK_BEHAVIOR_APP_HANDLE_L2CAP_PING_OFF,               /* 4 Value */
              DM_LINK_BEHAVIOR_DONT_ESTABLISH_ACL_ON_L2CAP_CONNECT,     /* 5 Key */
              DM_LINK_BEHAVIOR_DONT_ESTABLISH_ACL_ON_L2CAP_CONNECT_OFF, /* 6 Value */
              0xFF00                                                    /* 7 Terminator */
              };
    CsrSize    conftab_size   = sizeof(conftab_default);
    CsrUint16 *conftab        = (CsrUint16 *)CsrPmemZalloc(conftab_size);
    CsrUint16  conftab_length = (CsrUint16)(conftab_size/sizeof(CsrUint16));
    TYPED_BD_ADDR_T ad;

    cmData->dmVar.appHandle = cmPrim->appHandle;
    ad.addr                 = cmPrim->addr;
    ad.type                 = cmPrim->addrType;

    if ((cmPrim->flags & CM_SET_LINK_BEHAVIOR_L2CAP_RETRY_ON) == 0)
        conftab_default[2] = DM_LINK_BEHAVIOR_L2CAP_RETRY_OFF;

    if (cmPrim->flags & CM_SET_LINK_BEHAVIOR_APP_HANDLE_PING)
        conftab_default[4] = DM_LINK_BEHAVIOR_APP_HANDLE_L2CAP_PING_ON;

    if (cmPrim->flags & CM_SET_LINK_BEHAVIOR_DONT_ESTABLISH_ACL_ON_L2CAP_CONNECT)
        conftab_default[6] = DM_LINK_BEHAVIOR_DONT_ESTABLISH_ACL_ON_L2CAP_CONNECT_ON;

    SynMemCpyS(conftab, conftab_size, conftab_default, conftab_size);

    dm_set_link_behavior_req(&ad, conftab_length, conftab, NULL);
}


void CsrBtCmDmSetLinkBehaviorCfmHandler(cmInstanceData_t *cmData)
{
    DM_SET_LINK_BEHAVIOR_CFM_T *cfm = (DM_SET_LINK_BEHAVIOR_CFM_T *) cmData->recvMsgP;
    CsrBtCmDmSetLinkBehaviorCfm *prim;

    prim = (CsrBtCmDmSetLinkBehaviorCfm *)CsrPmemZalloc(sizeof(CsrBtCmDmSetLinkBehaviorCfm));
    prim->type = CSR_BT_CM_DM_SET_LINK_BEHAVIOUR_CFM;
    prim->status = (CsrBtResultCode) (cfm->status == DM_SET_LINK_BEHAVIOR_SUCCESS ?
                                         CSR_BT_RESULT_CODE_CM_SUCCESS : cfm->status);
    prim->addr = cfm->addrt.addr;
    prim->addrType = cfm->addrt.type;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, prim);

    /* Restore the DM queue */
    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_CM_SET_LINK_BEHAVIOR */
