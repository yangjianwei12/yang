/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_cm_main.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
#include "csr_bt_cm_bnep.h"
#endif

#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_util.h"

void CsrBtCmRoleSwitchCfmSend(CsrSchedQid appHandle,
                              CsrBtResultCode resultCode,
                              CsrBtSupplier resultSupplier,
                              CsrBtDeviceAddr deviceAddr,
                              CsrUint8 role,
                              CsrBtCmRoleType roleType)
{
    CsrBtCmSwitchRoleCfm        *prim;

    prim                 = (CsrBtCmSwitchRoleCfm *) CsrPmemZalloc(sizeof(*prim));
    prim->type           = CSR_BT_CM_SWITCH_ROLE_CFM;
    prim->deviceAddr     = deviceAddr;
    prim->role           = role;
    prim->roleType       = roleType;
    prim->resultCode     = resultCode;
    prim->resultSupplier = resultSupplier;
    CsrBtCmPutMessage(appHandle, prim);
}

static CsrBool csrBtCmDmIsScoPacketType(hci_pkt_type_t packetType)
{
    if (packetType & 0x0007)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL)
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
static void CmBnepSwitchRoleIndMsgSend(CsrSchedQid appHandle, bnepTable *bnepConnection, CsrUint8 role,
                                            CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{ /* Build and send CSR_BT_CM_BNEP_SWITCH_ROLE_IND to the application */
    CsrBtCmBnepSwitchRoleInd    *cmPrim;

    cmPrim = (CsrBtCmBnepSwitchRoleInd *)CsrPmemAlloc(sizeof(CsrBtCmBnepSwitchRoleInd));

    cmPrim->type            = CSR_BT_CM_BNEP_SWITCH_ROLE_IND;
    cmPrim->id              = bnepConnection->id;
    cmPrim->role            = role;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}

void CmDmBnepSwitchRoleReqHandler(cmInstanceData_t *cmData)
{
    CsrUint8 theIndex;
    CsrBtCmBnepSwitchRoleReq    *cmPrim;

    cmPrim                      = (CsrBtCmBnepSwitchRoleReq *) cmData->recvMsgP;
    theIndex                    = returnBnepIdIndex(cmData, cmPrim->id);

    if (theIndex != CM_ERROR)
    {
        bnepTable *bnepConnection;

        bnepConnection = &(cmData->bnepVar.connectVar[theIndex]);

        if (bnepConnection->state == CSR_BT_CM_BNEP_STATE_CONNECTED)
        { /* The link is in a state where it is allowed to proceed with the
             request. Change link settings before process with dmSwitchRole. */
            aclTable *aclConnectionElement;

            if (returnAclConnectionElement(cmData, bnepConnection->deviceAddr, &aclConnectionElement) != CM_ERROR)
            {
                if (aclConnectionElement->role == cmPrim->role)
                {
                    CmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                    bnepConnection,
                                                    cmPrim->role,
                                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                    CSR_BT_SUPPLIER_CM);
                }
                else
                {
                    if (cmPrim->role == CSR_BT_SLAVE_ROLE)
                    {
                        if (cmData->roleVar.alwaysSupportMasterRole)
                        {
                            CmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                            bnepConnection,
                                                            cmPrim->role,
                                                            CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                                            CSR_BT_SUPPLIER_CM);
                        }
                        else
                        {
                            if (returnNumOfAclConnection(cmData) > 1)
                            {
                                CmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                                bnepConnection,
                                                                cmPrim->role,
                                                                CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                                                CSR_BT_SUPPLIER_CM);
                            }
                            else
                            {
                                cmData->bnepVar.roleSwitchReqIndex = theIndex;
                                if (CmDuHandleAutomaticProcedure(cmData,
                                                                  CM_DU_AUTO_EVENT_BNEP_ROLE_SWITCH,
                                                                  (void *)cmPrim,
                                                                  &aclConnectionElement->deviceAddr))
                                {
                                    /* Control of the DM queue is transferred to device utility.*/
                                    return;
                                }
                            }
                        }
                    }
                    else
                    {
                        cmData->bnepVar.roleSwitchReqIndex  = theIndex;
                        /* Role Switch can be carried out, check if this can be handled by device utility.*/
                        if (CmDuHandleAutomaticProcedure(cmData,
                                                          CM_DU_AUTO_EVENT_BNEP_ROLE_SWITCH,
                                                          (void *)cmPrim,
                                                          &aclConnectionElement->deviceAddr))
                        {
                            /* Control of the DM queue is transferred to device utility.*/
                            return;
                        }
                    }
                }
            }
            else
            {
                CmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                bnepConnection,
                                                cmPrim->role,
                                                CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                                CSR_BT_SUPPLIER_CM);
            }
        }
    }

    /* Reaching here means the command is not successfull/handled, result of the same is sent to the caller.s*/
    CsrBtCmDmLocalQueueHandler();
}
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

#ifdef CSR_BT_INSTALL_CM_PRI_ALWAYS_SUPPORT_MASTER_ROLE
void CsrBtCmAlwaysSupportMasterRoleReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmAlwaysSupportMasterRoleReq        *prim;

    prim = (CsrBtCmAlwaysSupportMasterRoleReq *) cmData->recvMsgP;

    cmData->roleVar.alwaysSupportMasterRole = prim->alwaysSupportMasterRole;
}
#endif /* CSR_BT_INSTALL_CM_PRI_ALWAYS_SUPPORT_MASTER_ROLE */

#ifdef CSR_BT_INSTALL_CM_ROLE_SWITCH_CONFIG
void CsrBtCmRoleSwitchConfigReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmRoleSwitchConfigReq* prim;

    prim = (CsrBtCmRoleSwitchConfigReq *) cmData->recvMsgP;

    switch (prim->config)
    {
        case CSR_BT_CM_ROLE_SWITCH_DEFAULT:
            cmData->roleVar.alwaysSupportMasterRole = FALSE;
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            cmData->roleVar.doMssBeforeScoSetup     = TRUE;
#endif
            cmData->roleVar.doMssBeforeRnr          = FALSE;
            break;

        case CSR_BT_CM_ROLE_SWITCH_ALWAYS:
            cmData->roleVar.alwaysSupportMasterRole = TRUE;
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            cmData->roleVar.doMssBeforeScoSetup     = TRUE;
#endif
            break;

        case CSR_BT_CM_ROLE_SWITCH_BEFORE_SCO:
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            cmData->roleVar.doMssBeforeScoSetup     = TRUE;
#endif
            break;

        case CSR_BT_CM_ROLE_SWITCH_NOT_BEFORE_SCO:
#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            cmData->roleVar.doMssBeforeScoSetup     = FALSE;
#endif
            break;

        case CSR_BT_CM_ROLE_SWITCH_BEFORE_RNR:
            cmData->roleVar.doMssBeforeRnr          = TRUE;
            break;

        case CSR_BT_CM_ROLE_SWITCH_NOT_BEFORE_RNR:
            cmData->roleVar.doMssBeforeRnr          = FALSE;
            break;

        case CSR_BT_CM_ROLE_SWITCH_ALWAYS_ACL:
            cmData->roleVar.alwaysSupportMasterRole = TRUE;
            break;

        case CSR_BT_CM_ROLE_SWITCH_MULTIPLE_ACL:
            cmData->roleVar.alwaysSupportMasterRole = FALSE;
            break;

        default:
            CsrBtCmGeneralException(CSR_BT_CM_PRIM,
                                    prim->type,
                                    0,
                                    "");
            break;
    }
}
#endif /* CSR_BT_INSTALL_CM_ROLE_SWITCH_CONFIG */

void CsrBtCmDmAclRoleVarsClear(aclRoleVars_t *roleVars)
{
    if (roleVars)
    {
        roleVars->role      = CSR_BT_UNDEFINED_ROLE;
        roleVars->roleType  = CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID;
        roleVars->appHandle = CSR_SCHED_QID_INVALID;
    }
}

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
CsrBool CsrBtCmRoleSwitchBeforeScoSetupNeeded(cmInstanceData_t *cmData)
{
    return cmData->roleVar.doMssBeforeScoSetup;
}
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

CsrBool CsrBtCmRoleSwitchAllowedByUpperLayer(aclTable *aclConnectionElement)
{
    aclRoleVars_t *roleVars = CsrBtCmDmGetAclRoleVars(aclConnectionElement);

    if (!aclConnectionElement || roleVars->appHandle != CSR_SCHED_QID_INVALID)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

aclRoleVars_t* CsrBtCmDmGetAclRoleVars(aclTable *aclConnectionElement)
{
    return aclConnectionElement ? &aclConnectionElement->roleVars : NULL;
}

#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
static CsrBool csrBtCmDmAclRoleVarsRegisterAllowed(aclRoleVars_t        *roleVars, CsrSchedQid appHandle)
{
    return roleVars->appHandle == appHandle || roleVars->appHandle == CSR_SCHED_QID_INVALID;
}

static CsrBool csrBtCmDmAclRoleVarsRegisterApphandle(aclRoleVars_t        *roleVars, CsrSchedQid appHandle, CsrUint8 role, CsrBtCmRoleType roleType)
{
    if (roleVars && csrBtCmDmAclRoleVarsRegisterAllowed(roleVars, appHandle))
    {
        roleVars->role      = role;
        roleVars->roleType  = roleType;
        roleVars->appHandle = appHandle;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#else
#define csrBtCmDmAclRoleVarsRegisterApphandle(roleVars, appHandle, role, roleType) TRUE
#endif /* CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC */

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
static void cmDmBnepSwitchRoleIndMsgSend(CsrSchedQid appHandle,
                                         bnepTable *bnepConnection,
                                         CsrUint8 role,
                                         CsrBtResultCode resultCode,
                                         CsrBtSupplier resultSupplier)
{ /* Build and send CSR_BT_CM_BNEP_SWITCH_ROLE_IND to the application */
    CsrBtCmBnepSwitchRoleInd    *cmPrim;

    cmPrim = (CsrBtCmBnepSwitchRoleInd *)CsrPmemAlloc(sizeof(CsrBtCmBnepSwitchRoleInd));

    cmPrim->type            = CSR_BT_CM_BNEP_SWITCH_ROLE_IND;
    cmPrim->id              = bnepConnection->id;
    cmPrim->role            = role;
    cmPrim->resultCode      = resultCode;
    cmPrim->resultSupplier  = resultSupplier;
    CsrBtCmPutMessage(appHandle, cmPrim);
}
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

/* Inform role switch results to the applications associated with the given profile.*/
static void cmDmInformRoleSwitchSuccessToAllApps(cmInstanceData_t *cmData,
                                                 CsrUint8 role,
                                                 CsrBtDeviceAddr *deviceAddr)
{
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    cmData->bnepVar.roleSwitchReqIndex = CM_ERROR;
    {
        CsrUintFast8 index = 0;

        for ( index = 0; index < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS; index++)
        { /* Search through the BNEP connection table */
            if (cmData->bnepVar.connectVar[index].state == CSR_BT_CM_BNEP_STATE_CONNECTED)
            {
                if (CsrBtBdAddrEq(deviceAddr, &(cmData->bnepVar.connectVar[index].deviceAddr)))
                { /* The given device address is right. Build and send
                     CSR_BT_CM_BNEP_SWITCH_ROLE_IND to the profile with the current role. */
                    bnepTable *bnepConnection;

                    bnepConnection    = &(cmData->bnepVar.connectVar[index]);
                    cmDmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                 bnepConnection,
                                                 role,
                                                 CSR_BT_RESULT_CODE_CM_SUCCESS,
                                                 CSR_BT_SUPPLIER_CM);
                }
            }
        }
    }
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */
}

void CmDmInformRoleSwitchStatusToRequester(cmInstanceData_t *cmData,
                                           CsrUint8 role,
                                           CsrBtDeviceAddr *deviceAddr,
                                           CsrBtResultCode resultCode,
                                           CsrBtSupplier resultSupplier)
{
#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
    {
        if (cmData->bnepVar.roleSwitchReqIndex != CM_ERROR)
        {
            if(cmData->bnepVar.roleSwitchReqIndex < CSR_BT_MAX_NUM_OF_SIMULTANEOUS_BNEP_CONNECTIONS)
            {
                bnepTable *bnepConnection;

                bnepConnection    = &(cmData->bnepVar.connectVar[cmData->bnepVar.roleSwitchReqIndex]);
                if(CsrBtBdAddrEq(deviceAddr, &bnepConnection->deviceAddr))
                {
                    if (bnepConnection->state == CSR_BT_CM_BNEP_STATE_CONNECTED)
                    {
                        cmDmBnepSwitchRoleIndMsgSend(cmData->bnepVar.appHandle,
                                                     bnepConnection,
                                                     role,
                                                     resultCode,
                                                     resultSupplier);
                    }
                    cmData->bnepVar.roleSwitchReqIndex = CM_ERROR;
                }
            }
        }
    }
#else
    CSR_UNUSED(cmData);
    CSR_UNUSED(role);
    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(resultCode);
    CSR_UNUSED(resultSupplier);
#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */
}

#else /* !INSTALL_CM_DEVICE_UTILITY || !CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL */
#define csrBtCmDmAclRoleVarsRegisterApphandle(roleVars, appHandle, role, roleType) TRUE
#endif /* INSTALL_CM_DEVICE_UTILITY AND CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL*/

void CsrBtCmDmHciSwitchRoleCompleteHandler(cmInstanceData_t *cmData)
{
    /* A role change event has occurred.*/
    aclTable                 *aclConnectionElement;
    DM_HCI_SWITCH_ROLE_CFM_T *dmPrim = (DM_HCI_SWITCH_ROLE_CFM_T *) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateRoleSwitchEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE,
                         dmPrim->status,
                         dmPrim,
                         NULL);
#endif

    returnAclConnectionElement(cmData, dmPrim->bd_addr, &aclConnectionElement);

    if (aclConnectionElement)
    {
        aclConnectionElement->role = dmPrim->role;
    }

#if (CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT != CSR_BT_HCI_DEFAULT_LSTO)
    if (dmPrim->status == HCI_SUCCESS && dmPrim->role == CSR_BT_MASTER_ROLE)
    {
        CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmData, CSR_BT_CM_IFACEQUEUE, CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT, dmPrim->bd_addr);
    }
#endif

#if defined(INSTALL_CM_DEVICE_UTILITY) && defined(CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL)
    if (dmPrim->status == HCI_SUCCESS)
    {
        /* A role change event has occurred. Inform the involved profiles */
        cmDmInformRoleSwitchSuccessToAllApps(cmData,
                                             dmPrim->role,
                                             &dmPrim->bd_addr);
    }
    else
    {
        /* The request af changing the role failed.
         * Inform the profile that made the request about the error.*/
        CmDmInformRoleSwitchStatusToRequester(cmData,
                                              dmPrim->role,
                                              &dmPrim->bd_addr,
                                              (CsrBtResultCode) dmPrim->status,
                                              CSR_BT_SUPPLIER_HCI);
    }
#endif /* INSTALL_CM_DEVICE_UTILITY && CSR_BT_INSTALL_CM_INTERNAL_ROLE_CONTROL */

    if (!CmDuHandleCommandComplete(cmData, CM_DU_CMD_COMPLETE_ROLE_SWITCH))
    {
        /* Role switch complete is not handled by device utility, continue processing here.*/
        if (CsrBtBdAddrEq(&dmPrim->bd_addr, &cmData->dmVar.operatingBdAddr) &&
            cmData->dmVar.lockMsg == CM_DM_SWITCH_ROLE_REQ)
        {
            aclRoleVars_t *roleVars = CsrBtCmDmGetAclRoleVars(aclConnectionElement);
            CsrBtCmRoleType roleType = roleVars ? roleVars->roleType : CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID;

            if (dmPrim->status == HCI_SUCCESS && aclConnectionElement)
            {
                if (!roleVars || roleVars->role == dmPrim->role)
                {
                    CsrBtCmRoleSwitchCfmSend(cmData->dmVar.appHandle,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM,
                                             cmData->dmVar.operatingBdAddr,
                                             dmPrim->role,
                                             roleType);
                }
                else
                {
                    CsrBtCmRoleSwitchCfmSend(cmData->dmVar.appHandle,
                                             CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                             CSR_BT_SUPPLIER_CM,
                                             cmData->dmVar.operatingBdAddr,
                                             CSR_BT_UNDEFINED_ROLE,
                                             CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID);
                    CsrBtCmDmAclRoleVarsClear(roleVars);
                }
            }
            else
            {
                CsrBtCmRoleSwitchCfmSend(cmData->dmVar.appHandle,
                                         dmPrim->status == HCI_SUCCESS ? CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER: dmPrim->status,
                                         dmPrim->status == HCI_SUCCESS ? CSR_BT_SUPPLIER_CM: CSR_BT_SUPPLIER_HCI,
                                         cmData->smVar.operatingBdAddr,
                                         dmPrim->role,
                                         CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID);
                CsrBtCmDmAclRoleVarsClear(roleVars);
            }

            CsrBtCmDmLocalQueueHandler();
        }
    }
}

void CsrBtCmDmHciRoleDiscoveryCompleteHandler(cmInstanceData_t *cmData)
{
    DM_HCI_ROLE_DISCOVERY_CFM_T *prim = (DM_HCI_ROLE_DISCOVERY_CFM_T*) cmData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE
    CsrBtCmPropgateEvent(cmData,
                         CsrBtCmPropgateRoleSwitchEvents,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ROLE_CHANGE,
                         prim->status,
                         prim,
                         NULL);
#endif

    if (prim->status == HCI_SUCCESS)
    {
        aclTable *aclConnectionElement;

        returnAclConnectionElement(cmData,
                                   prim->bd_addr,
                                   &aclConnectionElement);

        if (aclConnectionElement)
        {
            aclConnectionElement->role = prim->role;
#if (CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT != CSR_BT_HCI_DEFAULT_LSTO)
            if (prim->role == CSR_BT_MASTER_ROLE)
            {
                CsrBtCmWriteDmLinkSuperVisionTimeoutHandler(cmData,
                                                            CSR_BT_CM_IFACEQUEUE,
                                                            CSR_BT_DEFAULT_LINK_SUPERVISION_TIMEOUT,
                                                            prim->bd_addr);
            }
#endif
        }
    }

    CsrBtCmDmLocalQueueHandler();
}

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtCmRfcScoConnectReqHandler(cmInstanceData_t *cmData, CsrBtDeviceAddr deviceAddr)
{
    if (cmData->dmVar.rfcConnIndex != CM_ERROR)
    {
        cmRfcConnElement * theElement = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromIndex, &(cmData->dmVar.rfcConnIndex));

        if (theElement &&
            theElement->cmRfcConnInst->state == CSR_BT_CM_RFC_STATE_CONNECTED &&
            CsrBtBdAddrEq(&(deviceAddr),
                          &(theElement->cmRfcConnInst->deviceAddr)))
        {
            cmRfcConnInstType *theLogicalLink = theElement->cmRfcConnInst;

            if (theLogicalLink->eScoParms)
            {
                if (theLogicalLink->eScoParms->handle == NO_SCO)
                {
                    CsrBtCmScoCommonParms parms;

                    if (CsrBtCmScoGetCurrentNegotiateParms(theLogicalLink->eScoParms->negotiateCnt,
                                                           &parms))
                    {
                        cmData->dmVar.activePlayer = RFC_PLAYER;
                        if (csrBtCmDmIsScoPacketType(parms.audioQuality))
                        {
                            /* Set parameters for legacy SCO */
                            dm_sync_connect_req(CSR_BT_CM_IFACEQUEUE,
                                                0, /* pv_cbarg */
                                                &theLogicalLink->deviceAddr,
                                                8000, /* tx bandwidth */
                                                8000, /* rx bandwidth */
                                                5, /* max latency */
                                                parms.voiceSettings,
                                                HCI_ESCO_NO_RETX, /* retx effort */
                                                parms.audioQuality);
                        }
                        else
                        {
                            dm_sync_connect_req(CSR_BT_CM_IFACEQUEUE,
                                                0, /* pv_cbarg */
                                                &theLogicalLink->deviceAddr,
                                                parms.txBandwidth,
                                                parms.rxBandwidth,
                                                parms.maxLatency,
                                                parms.voiceSettings,
                                                parms.reTxEffort,
                                                parms.audioQuality);
                        }
                    }
                    else
                    { /* No more packets types to try */
                        CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                      theLogicalLink->btConnId,
                                                      NULL,
                                                      0,
                                                      CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ATTEMPT_FAILED,
                                                      CSR_BT_SUPPLIER_CM);
                        CsrBtCmDmLocalQueueHandler();
                    }
                }
                else
                { /* The Sco handle is already reserve */
                    CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                                  theLogicalLink->btConnId,
                                                  NULL,
                                                  0,
                                                  CSR_BT_RESULT_CODE_CM_SYNCHRONOUS_CONNECTION_ALREADY_EXISTS,
                                                  CSR_BT_SUPPLIER_CM);
                    CsrBtCmDmLocalQueueHandler();
                }
            }
            else
            { /* The Sco handle is already reserve */
                CsrBtCmDmScoConnectCfmMsgSend(theLogicalLink->appHandle,
                                              theLogicalLink->btConnId,
                                              NULL,
                                              0,
                                              CSR_BT_RESULT_CODE_CM_UNSPECIFIED_ERROR,
                                              CSR_BT_SUPPLIER_CM);
                CsrBtCmDmLocalQueueHandler();
            }
        }
        else
        {
            CsrBtCmDmLocalQueueHandler();
        }
    }
    else
    {
        CsrBtCmDmLocalQueueHandler();
    }
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
#endif /* EXCLUDE_CSR_BT_SCO_MODULE */

#ifdef CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC
void CmDmSwitchRoleReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSwitchRoleReq *prim;
    aclTable             *aclConnectionElement;

    prim = (CsrBtCmSwitchRoleReq *) cmData->recvMsgP;

    if (returnAclConnectionElement(cmData, prim->deviceAddr, &aclConnectionElement) != CM_ERROR)
    {
        aclRoleVars_t *roleVars = CsrBtCmDmGetAclRoleVars(aclConnectionElement);

        if (prim->roleType != CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID && prim->role != CSR_BT_UNDEFINED_ROLE)
        {
            if (CsrBtCmBnepRoleSwitchAllowed(cmData) &&
                csrBtCmDmAclRoleVarsRegisterApphandle(roleVars, prim->appHandle, prim->role, prim->roleType))
            {
                if (aclConnectionElement->role == prim->role)
                {
                    /* Actual role and requested roles are same, there is no action here, return success to the caller.*/
                    CsrBtCmRoleSwitchCfmSend(prim->appHandle,
                                             CSR_BT_RESULT_CODE_CM_SUCCESS,
                                             CSR_BT_SUPPLIER_CM,
                                             aclConnectionElement->deviceAddr,
                                             aclConnectionElement->role,
                                             prim->roleType);
                }
                else
                {
                    cmData->dmVar.appHandle = prim->appHandle;
                    cmData->dmVar.operatingBdAddr = prim->deviceAddr;

                    if (roleVars)
                    {
                        roleVars->role = prim->role;
                    }

                    dm_hci_switch_role(&prim->deviceAddr, prim->role, NULL);

                    /* Wait for the role switch to complete.*/
                    return;
                }
            }
            else
            {
                CsrBtCmRoleSwitchCfmSend(prim->appHandle,
                                         CSR_BT_RESULT_CODE_CM_COMMAND_DISALLOWED,
                                         CSR_BT_SUPPLIER_CM,
                                         aclConnectionElement->deviceAddr,
                                         CSR_BT_UNDEFINED_ROLE,
                                         CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID);
            }
        }
        else
        { /* App is attempting to release control of the given BD_ADDR */
            CsrBtCmRoleSwitchCfmSend(prim->appHandle,
                                     CSR_BT_RESULT_CODE_CM_SUCCESS,
                                     CSR_BT_SUPPLIER_CM,
                                     aclConnectionElement->deviceAddr,
                                     CSR_BT_UNDEFINED_ROLE,
                                     CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID);
            CsrBtCmDmAclRoleVarsClear(roleVars);
        }
    }
    else
    {
        /* No ACL. Restore queue */
        CsrBtCmRoleSwitchCfmSend(prim->appHandle,
                                 CSR_BT_RESULT_CODE_CM_UNKNOWN_CONNECTION_IDENTIFIER,
                                 CSR_BT_SUPPLIER_CM,
                                 prim->deviceAddr,
                                 CSR_BT_UNDEFINED_ROLE,
                                 CSR_BT_CM_SWITCH_ROLE_TYPE_INVALID);
    }

    /* Reaching here means, the command was not successful and the same is reported back to the caller.*/
    CsrBtCmDmLocalQueueHandler();
}
#endif /* CSR_BT_INSTALL_CM_SWITCH_ROLE_PUBLIC */

void CsrBtCmDmRoleDiscoveryReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmRoleDiscoveryReq    *cmPrim;

    cmPrim                        = (CsrBtCmRoleDiscoveryReq *) cmData->recvMsgP;

    if (cmPrim->phandle == CSR_BT_CM_IFACEQUEUE)
    {
        dm_hci_role_discovery(&cmPrim->deviceAddr, NULL);
    }
    else
    {
        aclTable                       *aclConnectionElement;

        CsrBtCmRoleDiscoveryCfm     *prim;

        prim = (CsrBtCmRoleDiscoveryCfm *)CsrPmemAlloc(sizeof(CsrBtCmRoleDiscoveryCfm));

        returnAclConnectionElement(cmData, cmPrim->deviceAddr, &aclConnectionElement);

        prim->type                    = CSR_BT_CM_ROLE_DISCOVERY_CFM;
        prim->deviceAddr            = cmPrim->deviceAddr;
        if (aclConnectionElement)
        {
            prim->role = aclConnectionElement->role;
        }
        else
        {
            prim->role = CSR_BT_UNDEFINED_ROLE;
        }
        CsrBtCmPutMessage(cmPrim->phandle, prim);
        CsrBtCmDmLocalQueueHandler();
    }
}

