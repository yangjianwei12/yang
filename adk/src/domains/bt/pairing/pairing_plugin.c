/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       
    \ingroup    pairing
    \brief      Pairing plugin interface
*/
#include "pairing.h"
#include "pairing_plugin.h"

#include <panic.h>
#include <stdlib.h>

#ifdef USE_SYNERGY
#include <bdaddr.h>
#endif /* USE_SYNERGY */

static pairing_plugin_t pairing_plugin;
#ifdef USE_SYNERGY
static CsrBtCmSmUserConfirmationRequestInd* pending_confirmation_ind;

static cl_sm_auth_requirements pairing_ConvertToClAuthReq(uint8 authentication_requirements)
{
    switch (authentication_requirements)
    {
        case HCI_MITM_NOT_REQUIRED_NO_BONDING:
            return cl_sm_no_bonding_no_mitm;

        case HCI_MITM_REQUIRED_NO_BONDING:
            return cl_sm_no_bonding_mitm;

        case HCI_MITM_NOT_REQUIRED_DEDICATED_BONDING:
            return cl_sm_dedicated_bonding_no_mitm;

        case HCI_MITM_REQUIRED_DEDICATED_BONDING:
            return cl_sm_dedicated_bonding_mitm;

        case HCI_MITM_NOT_REQUIRED_GENERAL_BONDING:
            return cl_sm_general_bonding_no_mitm;

        case HCI_MITM_REQUIRED_GENERAL_BONDING:
            return cl_sm_general_bonding_mitm;

        default:
            return cl_sm_authentication_requirements_unknown;
    }
}

static cl_sm_io_capability pairing_ConvertToClIoCap(uint8 io_capability)
{
    switch (io_capability)
    {
        case HCI_IO_CAP_DISPLAY_ONLY:
            return cl_sm_io_cap_display_only;

        case HCI_IO_CAP_DISPLAY_YES_NO:
            return cl_sm_io_cap_display_yes_no;

        case HCI_IO_CAP_KEYBOARD_ONLY:
            return cl_sm_io_cap_keyboard_only;

        case HCI_IO_CAP_NO_INPUT_NO_OUTPUT:
            return cl_sm_io_cap_no_input_no_output;

        case HCI_IO_CAP_KEYBOARD_DISPLAY:
            return cl_sm_io_cap_keyboard_display;

        default:
            return cl_sm_io_cap_no_input_no_output;
    }
}
#else
static CL_SM_USER_CONFIRMATION_REQ_IND_T* pending_confirmation_ind;
#endif

static void pairing_PluginResetPending(void)
{
    if(pending_confirmation_ind)
    {
        free(pending_confirmation_ind);
        pending_confirmation_ind = NULL;
    }
}

void Pairing_PluginInit(void)
{
    memset(&pairing_plugin, 0, sizeof(pairing_plugin));
    pending_confirmation_ind = NULL;
}

bool Pairing_PluginIsRegistered(void)
{
    return pairing_plugin.handle_remote_io_capability != NULL;
}

void Pairing_PluginRegister(pairing_plugin_t plugin)
{
    PanicNull((void*)plugin.handle_remote_io_capability);
    PanicNull((void*)plugin.handle_io_capability_req);
    PanicNull((void*)plugin.handle_user_confirmation_req);
    
    PanicNotNull((void*)pairing_plugin.handle_remote_io_capability);
    PanicNotNull((void*)pairing_plugin.handle_io_capability_req);
    PanicNotNull((void*)pairing_plugin.handle_user_confirmation_req);

    pairing_plugin = plugin;
}

#ifdef USE_SYNERGY
bool Pairing_PluginHandleRemoteIoCapability(const CsrBtCmSmIoCapabilityResponseInd *ind)
{
    if(pairing_plugin.handle_remote_io_capability)
    {
        CL_SM_REMOTE_IO_CAPABILITY_IND_T cl_ind = { 0 };

        BdaddrConvertTpBluestackToVm(&cl_ind.tpaddr, &ind->tp_addrt);

        if (ind->flags & DM_SM_FLAGS_SECURITY_MANAGER)
        {
            /* If using SM for security e.g. BLE or CTKD, then just
             * copy the DM_SM_SECURITY_* bitfield
             */
            cl_ind.authentication_requirements = ind->authentication_requirements;
        }
        else
        {
            /* Otherwise, convert to the BR/EDR CL type. */
            cl_ind.authentication_requirements =  pairing_ConvertToClAuthReq(ind->authentication_requirements);
        }

        cl_ind.io_capability = pairing_ConvertToClIoCap(ind->io_capability);
        cl_ind.oob_data_present = ind->oob_data_present;

        pairing_plugin.handle_remote_io_capability(&cl_ind);
        return TRUE;
    }
    return FALSE;
}

bool Pairing_PluginHandleIoCapabilityRequest(const CsrBtCmSmIoCapabilityRequestInd *ind, pairing_io_capability_rsp_t* response)
{
    if(pairing_plugin.handle_io_capability_req)
    {
        CL_SM_IO_CAPABILITY_REQ_IND_T cl_ind = { 0 };

        BdaddrConvertTpBluestackToVm(&cl_ind.tpaddr, &ind->tp_addrt);
        cl_ind.sm_over_bredr = ind->flags & DM_SM_FLAGS_SMP_OVER_BREDR;

        *response = pairing_plugin.handle_io_capability_req(&cl_ind);
        return TRUE;
    }
    return FALSE;
}

bool Pairing_PluginHandleBondIndication(const bond_indication_ind_t *ind)
{
    if(pairing_plugin.handle_bond_indication)
    {
        pairing_plugin.handle_bond_indication(ind);
        return TRUE;
    }
    return FALSE;
}

#else
bool Pairing_PluginHandleRemoteIoCapability(const CL_SM_REMOTE_IO_CAPABILITY_IND_T *ind)
{
    if(pairing_plugin.handle_remote_io_capability)
    {
        pairing_plugin.handle_remote_io_capability(ind);
        return TRUE;
    }
    return FALSE;
}

bool Pairing_PluginHandleIoCapabilityRequest(const CL_SM_IO_CAPABILITY_REQ_IND_T *ind, pairing_io_capability_rsp_t* response)
{
    if(pairing_plugin.handle_io_capability_req)
    {
        *response = pairing_plugin.handle_io_capability_req(ind);
        return TRUE;
    }
    return FALSE;
}
#endif /* USE_SYNERGY */

#ifdef USE_SYNERGY
bool Pairing_PluginHandleUserConfirmationRequest(const CsrBtCmSmUserConfirmationRequestInd *ind, pairing_user_confirmation_rsp_t* response)
{
    CL_SM_USER_CONFIRMATION_REQ_IND_T cl_ind = { 0 };
    if(!pairing_plugin.handle_user_confirmation_req)
    {
        return FALSE;
    }

    BdaddrConvertTpBluestackToVm(&cl_ind.tpaddr, &ind->tp_addrt);
    cl_ind.numeric_value = ind->numeric_value;

    *response = pairing_plugin.handle_user_confirmation_req(&cl_ind);

    if(*response == pairing_user_confirmation_wait)
    {
        pending_confirmation_ind = malloc(sizeof(CsrBtCmSmUserConfirmationRequestInd));

        if(pending_confirmation_ind)
        {
            *pending_confirmation_ind = *ind;
        }
        else
        {
            /* Couldn't wait, override plugin and reject */
            *response = pairing_user_confirmation_reject;
        }
    }
    return TRUE;
}
#else
bool Pairing_PluginHandleUserConfirmationRequest(const CL_SM_USER_CONFIRMATION_REQ_IND_T *ind, pairing_user_confirmation_rsp_t* response)
{
    if(!pairing_plugin.handle_user_confirmation_req)
    {
        return FALSE;
    }
    
    *response = pairing_plugin.handle_user_confirmation_req(ind);
    
    if(*response == pairing_user_confirmation_wait)
    {
        pending_confirmation_ind = malloc(sizeof(CL_SM_USER_CONFIRMATION_REQ_IND_T));
        
        if(pending_confirmation_ind)
        {
            *pending_confirmation_ind = *ind;
        }
        else
        {
            /* Couldn't wait, override plugin and reject */
            *response = pairing_user_confirmation_reject;
        }
    }
    return TRUE;
}
#endif

#ifdef USE_SYNERGY
bool Pairing_PluginRetryUserConfirmation(void)
{
    if(pairing_plugin.handle_user_confirmation_req && pending_confirmation_ind)
    {
        pairing_user_confirmation_rsp_t response;
        Pairing_PluginHandleUserConfirmationRequest(pending_confirmation_ind, &response);

        if(response != pairing_user_confirmation_wait)
        {
            if (response != pairing_user_confirmation_reject)
            {
                CsrBtCmScDmUserConfirmationRequestRes(pending_confirmation_ind->tp_addrt.addrt.addr,
                                                      pending_confirmation_ind->tp_addrt.addrt.type,
                                                      pending_confirmation_ind->tp_addrt.tp_type);
            }
            else
            {
                CsrBtCmScDmUserConfirmationRequestNegRes(pending_confirmation_ind->tp_addrt.addrt.addr,
                                                         pending_confirmation_ind->tp_addrt.addrt.type,
                                                         pending_confirmation_ind->tp_addrt.tp_type);
            }
            pairing_PluginResetPending();
        }
        return TRUE;
    }
    return FALSE;
}
#else
bool Pairing_PluginRetryUserConfirmation(void)
{
    if(pairing_plugin.handle_user_confirmation_req && pending_confirmation_ind)
    {
        pairing_user_confirmation_rsp_t response;
        Pairing_PluginHandleUserConfirmationRequest(pending_confirmation_ind, &response);
        
        if(response != pairing_user_confirmation_wait)
        {
            bool accept = (response != pairing_user_confirmation_reject);
            ConnectionSmUserConfirmationResponse(&pending_confirmation_ind->tpaddr, accept);
            pairing_PluginResetPending();
        }
        return TRUE;
    }
    return FALSE;
}
#endif

void Pairing_PluginPairingComplete(void)
{
    pairing_PluginResetPending();
}

void Pairing_PluginUnregister(pairing_plugin_t plugin)
{
    PanicFalse(pairing_plugin.handle_remote_io_capability == plugin.handle_remote_io_capability);
    PanicFalse(pairing_plugin.handle_io_capability_req == plugin.handle_io_capability_req);
    PanicFalse(pairing_plugin.handle_user_confirmation_req == plugin.handle_user_confirmation_req);
#ifdef USE_SYNERGY
    PanicFalse(pairing_plugin.handle_bond_indication == plugin.handle_bond_indication);
#endif /* USE_SYNERGY */
    memset(&pairing_plugin, 0, sizeof(pairing_plugin));
    pending_confirmation_ind = NULL;
}
