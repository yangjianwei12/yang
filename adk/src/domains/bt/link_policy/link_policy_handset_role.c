/*!
    \copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \ingroup    link_policy
    \brief      Link policy control of handset role.

                This file controls the link role with the handset.
                The link policy with the peer earbud in TWM products is controlled by the mirror profile.

                For TWM, the handset must be master of the link to the primary earbud - mirroring
                is only supported when the primary earbud is ACL slave. Therefore this code requests a
                role switch to become slave and will prohibit further role switches once slave.

*/

#include "link_policy_private.h"

#include "logging.h"
#include "bdaddr.h"
#include "bt_device.h"
#include "dm_prim.h"
#include "panic.h"

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

static void appLinkPolicyUpdateLinkSupervisionTimeout(const bdaddr *bd_addr)
{
    uint16 timeout;

    if (appDeviceIsPeer(bd_addr))
        timeout = appConfigEarbudLinkSupervisionTimeout() * 1000UL / 625;
    else
        timeout = appConfigDefaultLinkSupervisionTimeout() * 1000UL / 625;
#ifdef USE_SYNERGY
    CsrBtDeviceAddr deviceAddr;
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CmWriteLinkSuperVisionTimeoutReqSend(&LinkPolicyGetTaskData()->task, deviceAddr, timeout);
#else
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_REQ);
    prim->handle = 0;
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->timeout = timeout;
    VmSendDmPrim(prim);
#endif

    DEBUG_LOG("appLinkPolicyUpdateLinkSupervisionTimeout, lap %06lx, %u slots", bd_addr->lap, timeout);
}

/*! \brief Send a prim directly to bluestack to role switch to slave.
    \param The address of remote device link.
    \param The role to request.
*/
static void role_switch(const bdaddr *bd_addr, hci_role new_role)
{
    ConnectionSetRoleBdaddr(&LinkPolicyGetTaskData()->task, bd_addr, new_role);
}

/*! \brief Prevent role switching

    Update the policy for the connection (if any) to the specified
    Bluetooth address, so as to prevent any future role switching.

    \param  bd_addr The Bluetooth address of the device
*/
static void prevent_role_switch(const bdaddr *bd_addr)
{
#ifdef USE_SYNERGY
    appLinkPolicySetMode(bd_addr, ENABLE_SNIFF);
#else
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_SNIFF;
    VmSendDmPrim(prim);
#endif /* USE_SYNERGY */
}

/*! \brief Discover the current role

    \param  bd_addr The Bluetooth address of the remote device
*/
static void discover_role(const bdaddr *bd_addr)
{
    ConnectionGetRoleBdaddr(&LinkPolicyGetTaskData()->task, bd_addr);

}


void appLinkPolicyHandleClDmAclOpenedIndication(const bdaddr *bd_addr, bool is_ble, bool is_local)
{
    if (!is_ble)
    {
        if (appDeviceIsHandset(bd_addr))
        {
            discover_role(bd_addr);
        }

        /* Set default link supervision timeout if locally inititated (i.e. we're master) */
        if (is_local)
        {
            appLinkPolicyUpdateLinkSupervisionTimeout(bd_addr);
        }
    }
#if defined(INCLUDE_LEA_LINK_POLICY)
    else if (!is_local)
    {
        /* Incoming BLE connection */
        appLinkPolicyUpdatePowerTableDeferred(bd_addr);
    }
#endif
}


static void appLinkPolicySendMessageToDiscoverRoleLater(const bdaddr *bd_addr)
{
    DEBUG_LOG("appLinkPolicySendMessageToDiscoverRoleLater: addr:%06lx",bd_addr->lap);
    lpTaskData *theLp = LinkPolicyGetTaskData();

    if(!MessagePendingFirst(&theLp->task, LINK_POLICY_DISCOVER_ROLE, NULL))
    {
        MAKE_LP_MESSAGE(LINK_POLICY_DISCOVER_ROLE);
        message->bd_addr = *bd_addr;
        MessageSendLater(&theLp->task, LINK_POLICY_DISCOVER_ROLE, message, LINK_POLICY_DISCOVER_ROLE_TIMEOUT_MS);
    }
}

/*! Handle a change in role or discovery of role, triggering any further role
 * switches required */
static void appLinkPolicyHandleRole(const bdaddr *bd_addr, hci_status status, hci_role role)
{
    hci_role new_role = hci_role_dont_care;

    if(appDeviceIsPeer(bd_addr))
    {
        DEBUG_LOG("appLinkPolicyHandleRole, peer enum:hci_status:%d enum:hci_role:%d",
                        status, role);

        if(status == hci_success && role == hci_role_master)
        {
            appLinkPolicyUpdateLinkSupervisionTimeout(bd_addr);
        }
    }
	
    if(appDeviceIsSink(bd_addr))
    {
        DEBUG_LOG("appLinkPolicyHandleRole, sink enum:hci_status:%d enum:hci_role:%d",
                        status, role);

        if(status == hci_success && role == hci_role_master)
        {
            appLinkPolicyUpdateLinkSupervisionTimeout(bd_addr);
        }
    }

    if (appDeviceIsHandset(bd_addr))
    {
        DEBUG_LOG("appLinkPolicyHandleRole, handset enum:hci_status:%d enum:hci_role:%d",
                        status, role);

        switch (status)
        {
            case hci_success:
                if (role == hci_role_master)
                {
                    new_role = hci_role_slave;
                }
                else
                {
                    /* This is the desired topology, disallow role switches */
                    prevent_role_switch(bd_addr);
                }
            break;

            case hci_error_role_change_not_allowed:
            case hci_error_role_switch_failed:
            case hci_error_lmp_response_timeout:
            case hci_error_unrecognised:
            case hci_error_controller_busy:
            case hci_error_instant_passed:
            case hci_error_oetc_low_resource:
            case hci_error_different_transaction_collision:
                 appLinkPolicySendMessageToDiscoverRoleLater(bd_addr);
            break;

            default:
            break;
        }
        if (new_role != hci_role_dont_care)
        {
            role_switch(bd_addr, new_role);
        }
    }
}

static void appLinkPolicyHandleScHostSupportOverrideCfm(const bdaddr *bd_addr, hci_status status, uint8 host_support_override)
{
    if (status != hci_success)
    {
        DEBUG_LOG_WARN("appLinkPolicyHandleScHostSupportOverrideCfm lap 0x%x status enum:hci_status:%u override %u",
                       bd_addr->lap, status, host_support_override);
    }
}

#ifdef USE_SYNERGY
static void appHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_SWITCH_ROLE_CFM:
        {
            CsrBtCmSwitchRoleCfm *cfm = (CsrBtCmSwitchRoleCfm*) message;
            uint16 status = cfm->resultCode;
            if(cfm->resultSupplier == CSR_BT_SUPPLIER_CM && cfm->resultCode == CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR)
            {
                status = HCI_ERROR_ROLE_SWITCH_FAILED;
            }

            appLinkPolicyHandleRole((bdaddr*)&cfm->deviceAddr, status, cfm->role);
        }
        break;

        case CSR_BT_CM_ROLE_CHANGE_IND:
        {
            CsrBtCmRoleChangeInd *ind = (CsrBtCmRoleChangeInd*)message;
            appLinkPolicyHandleRole((bdaddr*)&ind->deviceAddr, ind->resultCode, ind->role);
        }
        break;

        case CSR_BT_CM_ROLE_DISCOVERY_CFM:
        {
            CsrBtCmRoleDiscoveryCfm * cfm = (CsrBtCmRoleDiscoveryCfm*)message;
            appLinkPolicyHandleRole((bdaddr*)&cfm->deviceAddr, hci_success, cfm->role);
        }
        break;

        case CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM:
        {
            CsrBtCmWriteLinkSupervTimeoutCfm * cfm = (CsrBtCmWriteLinkSupervTimeoutCfm*)message;
            DEBUG_LOG("appHandleCmPrim, CSR_BT_CM_WRITE_LINK_SUPERV_TIMEOUT_CFM, lap %06lx, result %x", cfm->deviceAddr.lap, cfm->resultCode);
        }
        break;

        case CSR_BT_CM_LSTO_CHANGE_IND:
        {
            CsrBtCmLstoChangeInd * ind = (CsrBtCmLstoChangeInd*)message;
            DEBUG_LOG("appHandleCmPrim, CSR_BT_CM_LSTO_CHANGE_IND, lap %06lx, %u slots", ind->deviceAddr.lap, ind->lsto);
        }
        break;

        case CM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM:
        {
            CmWriteScHostSupportOverrideCfm *cfm = (CmWriteScHostSupportOverrideCfm *)message;
            appLinkPolicyHandleScHostSupportOverrideCfm((bdaddr *)&cfm->deviceAddr, cfm->status, cfm->hostSupportOverride);
        }
        break;

        default:
            DEBUG_LOG("appLinkPolicyHandleMessage appHandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }
    CmFreeUpstreamMessageContents(message);
}


void appLinkPolicyHandleCMMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case CM_PRIM:
            appHandleCmPrim(message);
            break;
            
        default:
            break;
    }
}

#else /* !USE_SYNERGY */

bool appLinkPolicyHandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    switch (id)
    {
        case CL_DM_ROLE_CFM:
        {
            const CL_DM_ROLE_CFM_T *cfm = message;
            appLinkPolicyHandleRole(&cfm->bd_addr, cfm->status, cfm->role);
        }
        return TRUE;

        case CL_DM_ROLE_IND:
        {
            const CL_DM_ROLE_IND_T *ind = message;
            appLinkPolicyHandleRole(&ind->bd_addr, ind->status, ind->role);
        }
        return TRUE;

        case CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM:
        {
            const CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM_T *cfm = (const CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM_T *)message;
            appLinkPolicyHandleScHostSupportOverrideCfm(&cfm->addr, cfm->status, cfm->override_action);
        }
        return TRUE;

        case CL_DM_ULP_SET_PRIVACY_MODE_CFM:
        {
            CL_DM_ULP_SET_PRIVACY_MODE_CFM_T *cfm = (CL_DM_ULP_SET_PRIVACY_MODE_CFM_T *)message;
            PanicFalse(cfm->status == hci_success);
            return TRUE;
        }

        case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        {
            CL_DM_LINK_SUPERVISION_TIMEOUT_IND_T * ind = (CL_DM_LINK_SUPERVISION_TIMEOUT_IND_T*)message;
            DEBUG_LOG("appLinkPolicyHandleConnectionLibraryMessages, CL_DM_LINK_SUPERVISION_TIMEOUT_IND, lap %06lx, %u slots", ind->bd_addr.lap, ind->timeout);
            return TRUE;
        }
    }
    return already_handled;
}
#endif /* USE_SYNERGY */

void appLinkPolicyHandleDiscoverRole(const LINK_POLICY_DISCOVER_ROLE_T * msg)
{
    discover_role(&msg->bd_addr);
}

void appLinkPolicyHandoverForceUpdateHandsetLinkPolicy(const bdaddr *bd_addr)
{
    /* Note that this prevents role switch, but allows sniff */
    prevent_role_switch(bd_addr);
    appLinkPolicyForceUpdatePowerTable(bd_addr);
}
