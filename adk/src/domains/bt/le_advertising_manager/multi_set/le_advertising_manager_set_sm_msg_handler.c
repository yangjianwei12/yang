/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_multi_set
    \brief      Message handler functions for the advertising set state machine.
*/

#include "le_advertising_manager.h"
#include "le_advertising_manager_set_sm.h"
#include "le_advertising_manager_set_sm_msg_handler.h"

#include <connection.h>
#include <logging.h>
#include <bdaddr.h>

LOGGING_PRESERVE_MESSAGE_TYPE(le_advertising_manager_internal_msg_t)
ASSERT_INTERNAL_MESSAGES_NOT_OVERFLOWED(LE_ADVERTISING_MANAGER_SET_INTERNAL_MESSAGE_END)

static void LeAdvertisingManager_SetSmSendRandomAddressRotateTimeout(le_advertising_manager_set_state_machine_t *sm);
static void LeAdvertisingManager_SetSmSendRandomAddressChangedInd(le_advertising_manager_set_state_machine_t *sm, bdaddr new_bdaddr);

static void leAdvertisingManager_SetSmHandleInternalRegister(const LE_ADVERTISING_MANAGER_SET_INTERNAL_REGISTER_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalRegister sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED:
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_REGISTERING);
        break;

    default:
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalUpdateParamsReq(const LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_PARAMS_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalUpdateParamsReq sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    sm->params = req->params;

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_IDLE:
    case LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE:
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_PARAMS);
        break;

    default:
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalUpdateDataReq(const LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_DATA_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalUpdateDataReq sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    sm->data = req->data;

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_IDLE:
    case LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE:
        if (sm->data.adv_data.data_size)
        {
            LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_DATA);
        }
        else if (sm->data.scan_resp_data.data_size)
        {
            LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_SCAN_RESP_DATA);
        }
        else
        {
            /* No advert or scan data to send to the firmware. */
            LeAdvertisingManager_SetSmSendUpdateDataCfm(sm, TRUE);
        }
        break;

    default:
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalEnable(const LE_ADVERTISING_MANAGER_SET_INTERNAL_ENABLE_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalEnable sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    LeAdvertisingManager_SetSmSetTargetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_IDLE:
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ENABLING);
        break;

    default:
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalDisable(const LE_ADVERTISING_MANAGER_SET_INTERNAL_DISABLE_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalDisable sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    LeAdvertisingManager_SetSmSetTargetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE:
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_SUSPENDING);
        break;

    default:
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalUnregister(const LE_ADVERTISING_MANAGER_SET_INTERNAL_UNREGISTER_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalUnregister sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    LeAdvertisingManager_SetSmSetTargetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_NULL);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_IDLE:
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERING);
        break;

    default:
        DEBUG_LOG_WARN("LEAM SetSmHandleInternalUnregister unexpected state - completing with error");
        LeAdvertisingManager_SetSmSendUnregisterCfm(sm, FALSE);
        break;
    }
}

static void leAdvertisingManager_SetSmHandleInternalRandomAddressRotateTimerExpired(const LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER_T *req)
{
    le_advertising_manager_set_state_machine_t *sm = req->sm;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleInternalRandomAddressRotateTimerExpired sm %p state enum:le_advertising_manager_set_state_t:%u",
                      sm, LeAdvertisingManager_SetSmGetState(sm));

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE:
        DEBUG_LOG("LEAM SetSmHandleInternalRandomAddressRotateTimerExpired in Active State");
        LeAdvertisingManager_SetSmSendRandomAddressRotateTimeout(sm);
        break;

    default:
        DEBUG_LOG_WARN("LEAM SetSmHandleInternalRandomAddressRotateTimerExpired unexpected state");
        break;
    }

}
void LeAdvertisingManager_SetSmHandleExtendedAdvertisingRegisterAppAdvSetCfm(const CL_DM_BLE_EXT_ADV_REGISTER_APP_ADV_SET_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);

    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingRegisterAppAdvSetCfm sm %p result 0x%x adv_hdl %u",
              sm, cfm->status, cfm->adv_handle);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_REGISTERING:
        {
            bool registered = (hci_success == cfm->status);

            LeAdvertisingManager_SetSmSendRegisterCfm(sm, registered);

            if (registered)
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);
            }
            else
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED);
            }
        }
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetParamCfm(const CL_DM_BLE_SET_EXT_ADV_PARAMS_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);

    PanicNull(sm);
        
    le_advertising_manager_set_state_t next_state = sm->target_state;

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingSetParamCfm sm %p result 0x%x adv_hdl %u adv_sid %u",
              sm, cfm->status, cfm->adv_handle, cfm->adv_sid);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_PARAMS:
        {
            if (success == cfm->status)
            {
                le_advertising_manager_set_params_t *params = LeAdvertisingManager_SetSmGetParams(sm);
                
                /* For extended advert parameters... if a random or resolvable address is used
                   then the address details must be supplied separately. If this is the case
                   request that the response to setting data is sent locally - so that the address
                   can be set correctly before continuing. */
                switch(params->own_addr_type)
                {
                    case OWN_ADDRESS_PUBLIC:
                    case OWN_ADDRESS_GENERATE_RPA_FBP:
                        /* No need to program address */
                        break;

                    case OWN_ADDRESS_RANDOM:
                    case OWN_ADDRESS_GENERATE_RPA_FBR:
                        next_state = LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_ADDRESS;
                        break;

                    default:
                        break;
                }

                if (LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_ADDRESS != next_state)
                {
                    LeAdvertisingManager_SetSmSendUpdateParamsCfm(sm, TRUE);
                }
                LeAdvertisingManager_SetSmSetState(sm, next_state);
            }
            else
            {
                LeAdvertisingManager_SetSmSendUpdateParamsCfm(sm, FALSE);
                LeAdvertisingManager_SetSmSetState(sm, next_state);
            }
        }
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetRandomAddressCfm(const CL_DM_BLE_EXT_ADV_SET_RANDOM_ADDRESS_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);
    
    PanicNull(sm);
        
    le_advertising_manager_set_state_t next_state = sm->target_state;

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingSetRandomAddressCfm sm %p result 0x%x adv_hdl %u addr %04x,%02x,%06lx",
              sm, cfm->status, cfm->adv_handle, cfm->random_addr.nap, cfm->random_addr.uap, cfm->random_addr.lap);

    le_advertising_manager_set_params_t *params = LeAdvertisingManager_SetSmGetParams(sm);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_ADDRESS:
        {
            if (   cfm->status == hci_error_controller_busy /* Can be busy behind the scenes */
                && sm->extended_advert_rpa_retries)
            {

                sm->extended_advert_rpa_retries--;

                ConnectionDmBleExtAdvSetRandomAddressReq(LeAdvertisingManager_SetSmGetTask(sm),
                                                         LeAdvertisingManager_SetSmGetAdvHandle(sm),
                                                         params->random_addr_type,
                                                         params->random_addr.addr);
            }
            else
            {
                if(BdaddrTypedIsEmpty(&params->random_addr))
                {
                    params->random_addr.type = TYPED_BDADDR_RANDOM;
                    params->random_addr.addr.lap = cfm->random_addr.lap;
                    params->random_addr.addr.nap = cfm->random_addr.nap;
                    params->random_addr.addr.uap = cfm->random_addr.uap;

                }
                LeAdvertisingManager_SetSmSendRandomAddressChangedInd(sm, cfm->random_addr);
                LeAdvertisingManager_SetSmSendUpdateParamsCfm(sm, TRUE);
                LeAdvertisingManager_SetSmSetState(sm, next_state);
            }
        }
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetDataCfm(const CL_DM_BLE_SET_EXT_ADV_DATA_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);

    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingSetDataCfm sm %p status 0x%x adv_hdl %u",
              sm, cfm->status, cfm->adv_handle);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_DATA:
        {
            if (success == cfm->status)
            {
                if (sm->data.scan_resp_data.data_size)
                {
                    LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_SCAN_RESP_DATA);
                }
                else
                {
                    LeAdvertisingManager_SetSmSendUpdateDataCfm(sm, TRUE);
                    LeAdvertisingManager_SetSmSetState(sm, sm->target_state);
                }
            }
            else
            {
                /* TBD: Go back to REGISTERED state? */
                Panic();
            }
        }

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetScanResponseDataCfm(const CL_DM_BLE_EXT_ADV_SET_SCAN_RESPONSE_DATA_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);
    
    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingSetScanResponseDataCfm sm %p result 0x%x adv_hdl %u",
              sm, cfm->status, cfm->adv_handle);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_SCAN_RESP_DATA:
        {
            if (success == cfm->status)
            {
                LeAdvertisingManager_SetSmSendUpdateDataCfm(sm, TRUE);
                LeAdvertisingManager_SetSmSetState(sm, sm->target_state);
            }
            else
            {
                /* TBD: Go back to REGISTERED state? */
                Panic();
            }
        }

    default:
        break;
    }
}



void LeAdvertisingManager_SetSmHandleExtendedAdvertisingEnableCfm(const CL_DM_BLE_EXT_ADVERTISE_ENABLE_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);

    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingEnableCfm sm %p result 0x%x adv_hdl %u",
              sm, cfm->status, cfm->adv_handle);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_ENABLING:
        {
            if(sm->adv_terminate_ind_processing)
            {
                DEBUG_LOG("LEAM Reset Terminate Indication Processing Flag");
                sm->adv_terminate_ind_processing = FALSE;
            }
            else
            {
                LeAdvertisingManager_SetSmSendEnableCfm(sm, (hci_success == cfm->status));
                leAdvertisingManager_SetSmEnableAddressRotateTimer(sm, TRUE);
            }

            if (hci_success == cfm->status)
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE);
            }
            else
            {
                LeAdvertisingManager_SetSmSetTargetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);
            }
        }
        break;

    case LE_ADVERTISING_MANAGER_SET_STATE_SUSPENDING:
        {
            LeAdvertisingManager_SetSmSendDisableCfm(sm, (hci_success == cfm->status));
            leAdvertisingManager_SetSmEnableAddressRotateTimer(sm, FALSE);

            if (hci_success == cfm->status)
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);
            }
            else
            {
                LeAdvertisingManager_SetSmSetTargetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE);
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE);
            }
        }
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingTerminatedInd(const CL_DM_BLE_EXT_ADV_TERMINATED_IND_T *ind)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(ind->adv_handle);

    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingTerminatedInd sm %p adv_hdl %u enum:le_advertising_manager_set_state_t:%d %06x Reason:%d 0x%x",
        sm,
        ind->adv_handle,
        LeAdvertisingManager_SetSmGetState(sm),
        ind->taddr.addr.lap,
        ind->reason,
        ind->adv_bits);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE:
        sm->adv_terminate_ind_processing = TRUE;
        /* Current behaviour is to always re-start advertising this set */
        LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_ENABLING);
        break;

    default:
        break;
    }
}

void LeAdvertisingManager_SetSmHandleExtendedAdvertisingUnregisterAppAdvSetCfm(const CL_DM_BLE_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM_T *cfm)
{
    le_advertising_manager_set_state_machine_t *sm = LeAdvertisingManager_SetSmGetByAdvHandle(cfm->adv_handle);

    PanicNull(sm);

    DEBUG_LOG("LEAM SetSmHandleExtendedAdvertisingUnregisterAppAdvSetCfm sm %p result 0x%x adv_hdl %u",
              sm, cfm->status, cfm->adv_handle);

    switch (LeAdvertisingManager_SetSmGetState(sm))
    {
    case LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERING:
        {
            bool unregistered = (hci_success == cfm->status);

            if (unregistered)
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED);
            }
            else
            {
                LeAdvertisingManager_SetSmSetState(sm, LE_ADVERTISING_MANAGER_SET_STATE_IDLE);
            }

            LeAdvertisingManager_SetSmSendUnregisterCfm(sm, unregistered);
        }
        break;

    default:
        break;
    }
}

#ifdef USE_SYNERGY
/* API function to use as the handler for CM messages */
static void LeAdvertisingManager_SetSmHandleCmPrim(void *msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *)msg;

    DEBUG_LOG_VERBOSE("LEAM SetSmHandleCmPrim 0x%x", *prim);

    switch (*prim)
    {
    case CSR_BT_CM_EXT_ADV_SET_DATA_CFM:
        {
            CmExtAdvSetDataCfm *cmMessage = (CmExtAdvSetDataCfm *) msg;
            CL_DM_BLE_SET_EXT_ADV_DATA_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetDataCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM:
        {
            CmExtAdvSetScanRespDataCfm *cmMessage = (CmExtAdvSetScanRespDataCfm *) msg;
            CL_DM_BLE_EXT_ADV_SET_SCAN_RESPONSE_DATA_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetScanResponseDataCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM:
        {
            CmExtAdvSetRandomAddrCfm *cmMessage = (CmExtAdvSetRandomAddrCfm *) msg;
            CL_DM_BLE_EXT_ADV_SET_RANDOM_ADDRESS_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            clMessage.random_addr.lap = cmMessage->randomAddr.lap;
            clMessage.random_addr.uap = cmMessage->randomAddr.uap;
            clMessage.random_addr.nap = cmMessage->randomAddr.nap;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetRandomAddressCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM:
        {
            CmExtAdvSetParamsCfm *cmMessage = (CmExtAdvSetParamsCfm *) msg;
            CL_DM_BLE_SET_EXT_ADV_PARAMS_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            clMessage.adv_sid = cmMessage->advSid;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingSetParamCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_ENABLE_CFM:
        {
            CmExtAdvEnableCfm *cmMessage = (CmExtAdvEnableCfm *) msg;
            CL_DM_BLE_EXT_ADVERTISE_ENABLE_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingEnableCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
        {
            CmExtAdvRegisterAppAdvSetCfm *cmMessage = (CmExtAdvRegisterAppAdvSetCfm *) msg;
            CL_DM_BLE_EXT_ADV_REGISTER_APP_ADV_SET_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingRegisterAppAdvSetCfm(&clMessage);
        }
        break;

    case CSR_BT_CM_EXT_ADV_TERMINATED_IND:
        {
            CmExtAdvTerminatedInd *cmInd = (CmExtAdvTerminatedInd *) msg;
            CL_DM_BLE_EXT_ADV_TERMINATED_IND_T termInd;
            /* Not full message translation */
            termInd.adv_handle = cmInd->advHandle;
            termInd.adv_bits = cmInd->advBits;
            termInd.reason = cmInd->reason;
            termInd.taddr.addr.lap = cmInd->addrt.addr.lap;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingTerminatedInd(&termInd);
        }
        break;

    case CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
        {
            CmExtAdvUnregisterAppAdvSetCfm *cmMessage = (CmExtAdvUnregisterAppAdvSetCfm *) msg;
            CL_DM_BLE_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM_T clMessage;
            clMessage.status = cmMessage->resultCode;
            clMessage.adv_handle = cmMessage->advHandle;
            LeAdvertisingManager_SetSmHandleExtendedAdvertisingUnregisterAppAdvSetCfm(&clMessage);
        }
        break;

    default:
        break;
    }

    CmFreeUpstreamMessageContents(msg);
}
#endif

void LeAdvertisingManager_SetSmMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    DEBUG_LOG("LEAM SetSmMessageHandler Task %p id enum:le_advertising_manager_internal_msg_t:%u",
              task, id);

    switch (id)
    {
    case LE_ADVERTISING_MANAGER_SET_INTERNAL_REGISTER:
        leAdvertisingManager_SetSmHandleInternalRegister((const LE_ADVERTISING_MANAGER_SET_INTERNAL_REGISTER_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_PARAMS_REQ:
        leAdvertisingManager_SetSmHandleInternalUpdateParamsReq((const LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_PARAMS_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_DATA_REQ:
        leAdvertisingManager_SetSmHandleInternalUpdateDataReq((const LE_ADVERTISING_MANAGER_SET_INTERNAL_UPDATE_DATA_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_ENABLE:
        leAdvertisingManager_SetSmHandleInternalEnable((const LE_ADVERTISING_MANAGER_SET_INTERNAL_ENABLE_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_DISABLE:
        leAdvertisingManager_SetSmHandleInternalDisable((const LE_ADVERTISING_MANAGER_SET_INTERNAL_DISABLE_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_UNREGISTER:
        leAdvertisingManager_SetSmHandleInternalUnregister((const LE_ADVERTISING_MANAGER_SET_INTERNAL_UNREGISTER_T *)message);
        break;

    case LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER:
        leAdvertisingManager_SetSmHandleInternalRandomAddressRotateTimerExpired((const LE_ADVERTISING_MANAGER_SET_INTERNAL_RANDOM_ADDRESS_ROTATE_TIMER_T *)message);
        break;

#ifdef USE_SYNERGY
    case CM_PRIM:
        LeAdvertisingManager_SetSmHandleCmPrim((void *)message);
        break;
#endif

    default:
       break;
    }
}

void LeAdvertisingManager_SetSmSendRegisterCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->RegisterCfm(sm, success);    
    }
}

void LeAdvertisingManager_SetSmSendUnregisterCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->UnregisterCfm(sm, success);
    }
}

void LeAdvertisingManager_SetSmSendUpdateParamsCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->UpdateParamsCfm(sm, success);
    }
}

void LeAdvertisingManager_SetSmSendUpdateDataCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->UpdateDataCfm(sm, success);
    }
}

void LeAdvertisingManager_SetSmSendEnableCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->EnableCfm(sm, success);
    }
}

void LeAdvertisingManager_SetSmSendDisableCfm(le_advertising_manager_set_state_machine_t *sm, bool success)
{
    if (sm->client_if)
    {
        sm->client_if->DisableCfm(sm, success);
    }
}

static void LeAdvertisingManager_SetSmSendRandomAddressRotateTimeout(le_advertising_manager_set_state_machine_t *sm)
{
    if (sm->client_if)
    {
        sm->client_if->RandomAddressRotateTimeout(sm);
    }
}

static void LeAdvertisingManager_SetSmSendRandomAddressChangedInd(le_advertising_manager_set_state_machine_t *sm, bdaddr new_bdaddr)
{
    if (sm->client_if)
    {
        sm->client_if->RandomAddressChangedInd(sm, new_bdaddr);
    }
}
