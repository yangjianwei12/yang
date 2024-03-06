/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       
    \ingroup    local_addr
    \brief      Bluetooth Local Address component

*/

#include "connection_abstraction.h"
#include <connection.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <bdaddr.h>

#include "local_addr_protected.h"

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(local_addr_message_t)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(LOCAL_ADDR, LOCAL_ADDR_MESSAGE_END)

typedef enum
{
    local_addr_not_configured,
    local_addr_configuring,
    local_addr_configured
} local_addr_state_t;

typedef struct
{
    Task  client_task;
    local_addr_state_t state;
    local_addr_host_gen_t host;
    local_addr_controller_gen_t controller;

    /*! The programmed BT address of this device. */ 
    bdaddr prog_bt_address;
} local_addr_t;

struct local_address_context_struct 
{
    Task  client_task;
    local_addr_host_gen_t host;
    local_addr_controller_gen_t controller;
};

static local_addr_t local_addr;

#ifdef USE_SYNERGY
static void localAddr_HandleMessages(Task task, MessageId id, Message message);

static TaskData local_addr_task = {localAddr_HandleMessages};
#endif

static void localAddr_Reset(void)
{
    local_addr.client_task = NULL;
    local_addr.state = local_addr_not_configured;
    local_addr.host = local_addr_host_gen_none;
    local_addr.controller = local_addr_controller_gen_none;
    /* do not reset the programmed address */
}

bool LocalAddr_Init(Task init_task)
{
    UNUSED(init_task);
    localAddr_Reset();
#ifdef USE_RPA
    LocalAddr_ConfigureBleGeneration(NULL, local_addr_host_gen_resolvable, local_addr_controller_gen_none);
#endif
    return TRUE;
}

uint8 LocalAddr_GetBleType(void)
{
#ifdef INCLUDE_SM_PRIVACY_1P2
    if(local_addr.controller == local_addr_controller_gen_rpa)
    {
        if(local_addr.host == local_addr_host_gen_none)
        {
            return OWN_ADDRESS_GENERATE_RPA_FBP;
        }
        
        return OWN_ADDRESS_GENERATE_RPA_FBR;
    }
#endif
    
    if(local_addr.host == local_addr_host_gen_none)
    {
        return OWN_ADDRESS_PUBLIC;
    }
    
    return OWN_ADDRESS_RANDOM;
}

static void localAddr_SendConfigureBleGenerationCfm(Task task, local_addr_status_t status)
{
    if(task)
    {
        MESSAGE_MAKE(msg, LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM_T);
        msg->status = status;
        MessageSend(task, LOCAL_ADDR_CONFIGURE_BLE_GENERATION_CFM, msg);
    }
    else
    {
        DEBUG_LOG("localAddr_SendConfigureBleGenerationCfm status : %d", status);
    }
}

static void localAddr_ConfigureRandomAddressGeneration(ble_local_addr_type type)
{
    local_addr.state = local_addr_configuring;
#ifdef USE_SYNERGY
    UNUSED(type);
    CmLeSetOwnAddressTypeReqSend(&local_addr_task, LocalAddr_GetBleType());
#else
    ConnectionDmBleConfigureLocalAddressAutoReq(type, NULL, BLE_RPA_TIMEOUT_DEFAULT);
#endif
}

static bool localAddrHostGenToType(local_addr_host_gen_t host, ble_local_addr_type* type)
{
    switch(host)
    {
        case local_addr_host_gen_none:
            return FALSE;
        
        case local_addr_host_gen_static:
            *type = ble_local_addr_generate_static;
            return TRUE;
        
        case local_addr_host_gen_resolvable:
            *type = ble_local_addr_generate_resolvable;
            return TRUE;
        
        case local_addr_host_gen_non_resolvable:
            *type = ble_local_addr_generate_non_resolvable;
            return TRUE;
        
        default:
            Panic();
            return FALSE;
    }
}

void LocalAddr_ReconfigureBleGeneration(void)
{
    local_addr_state_t old_state = local_addr.state;
    Task  task = local_addr.client_task;
    local_addr_host_gen_t host = local_addr.host;
    local_addr_controller_gen_t controller = local_addr.controller;

    LocalAddr_ReleaseBleGeneration(task);

    LocalAddr_ConfigureBleGeneration(task, host, controller);

    DEBUG_LOG("LocalAddr_ReconfigureBleGeneration. Task is:%p State was enum:local_addr_state_t:%d now enum:local_addr_state_t:%d",
                    local_addr.client_task, old_state, local_addr.state);
}

void LocalAddr_ConfigureBleGeneration(Task task, local_addr_host_gen_t host, local_addr_controller_gen_t controller)
{
    ble_local_addr_type type;

    if (local_addr.state != local_addr_not_configured)
    {
        local_addr_status_t status = local_addr_failure;

        if(local_addr.host == host && local_addr.controller == controller)
        {
            status = local_addr_success;
        }

        localAddr_SendConfigureBleGenerationCfm(task, status);
        return;
    }
    
    local_addr.client_task = task;
    local_addr.host = host;
    local_addr.controller = controller;
    
    if(localAddrHostGenToType(host, &type))
    {
        localAddr_ConfigureRandomAddressGeneration(type);
    }
    else
    {
        local_addr.state = local_addr_configured;
        localAddr_SendConfigureBleGenerationCfm(task, local_addr_success);
    }
}

bool LocalAddr_ReleaseBleGeneration(Task task)
{
    if(local_addr.state != local_addr_configured)
    {
        return FALSE;
    }
    
    if(task != local_addr.client_task)
    {
        return FALSE;
    }
    
    localAddr_Reset();
    return TRUE;
}


local_address_context_t  LocalAddr_OverrideBleGeneration(
                                    Task task, 
                                    local_addr_host_gen_t host, 
                                    local_addr_controller_gen_t controller)
{
    local_address_context_t context;

    context = PanicNull(calloc(1,sizeof(*context)));

    context->client_task = local_addr.client_task;
    context->host = local_addr.host;
    context->controller = local_addr.controller;

    local_addr.client_task = task;
    local_addr.host = host;
    local_addr.controller = controller;

    LocalAddr_ReconfigureBleGeneration();

    return context;
}


void LocalAddr_ReleaseOverride(local_address_context_t *context)
{
    PanicNull(context);
    PanicNull(*context);

    local_addr.client_task = (*context)->client_task;
    local_addr.host = (*context)->host;
    local_addr.controller = (*context)->controller;
    free(*context);
    *context = NULL;

    LocalAddr_ReconfigureBleGeneration();

}

static void localAddr_HandleDmBleConfigureLocalAddressCfm(bool configured)
{
    if(configured)
    {
        local_addr.state = local_addr_configured;
    }
    else
    {
        local_addr.state = local_addr_not_configured;
    }

    localAddr_SendConfigureBleGenerationCfm(local_addr.client_task, configured ? local_addr_success : local_addr_failure);
}

#ifdef USE_SYNERGY
static void localAddr_HandleCmLeSetOwnAddressTypeCfm(CsrBtCmLeSetOwnAddressTypeCfm *cfm)
{
    DEBUG_LOG("localAddr_HandleCmLeSetOwnAddressTypeCfm, status 0x%04x supplier 0x%04x",
              cfm->resultCode, cfm->resultSupplier);

    if (local_addr.state == local_addr_configuring)
    {
        if (CSR_BT_RESULT_CODE_CM_SUCCESS == cfm->resultCode)
        {
            CmLeSetPvtAddrTimeoutReqSend(&local_addr_task,
                                         CSR_BT_LE_DEFAULT_PVT_ADDR_TIMEOUT);
        }
        else
        {
            local_addr.state = local_addr_not_configured;
            localAddr_SendConfigureBleGenerationCfm(local_addr.client_task, local_addr_failure);
        }
    }
}

static void localAddr_HandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_LE_SET_OWN_ADDRESS_TYPE_CFM:
            localAddr_HandleCmLeSetOwnAddressTypeCfm((CsrBtCmLeSetOwnAddressTypeCfm *) message);
            break;

        case CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM:
        {
            if(local_addr.state == local_addr_configuring)
            {
                CsrBtCmLeSetPvtAddrTimeoutCfm *cfm = (CsrBtCmLeSetPvtAddrTimeoutCfm *) message;

                if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    DEBUG_LOG("CSR_BT_CM_LE_SET_PVT_ADDR_TIMEOUT_CFM, result:0x%04x supplier:0x%04x",
                              cfm->resultCode, cfm->resultSupplier);
                }
                localAddr_HandleDmBleConfigureLocalAddressCfm(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS);
            }
            break;
        }

        default:
            DEBUG_LOG("localAddr_HandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }

    CmFreeUpstreamMessageContents((void *) message);
}

static void localAddr_HandleMessages(Task task, MessageId id, Message message)
{
    switch (id)
    {
        case CM_PRIM:
            localAddr_HandleCmPrim(message);
            break;

        default:
            DEBUG_LOG("localAddr_HandleMessages, unexpected message 0x%04x", id);
            break;
    }

    UNUSED(task);
}
#else
bool LocalAddr_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{    
    if(id == CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM && local_addr.state == local_addr_configuring)
    {
        localAddr_HandleDmBleConfigureLocalAddressCfm(((CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM_T*)message)->status == success);
        return TRUE;
    }
    
    return already_handled;
}
#endif

bool LocalAddr_IsPublic(void)
{
    return (LocalAddr_GetBleType() == OWN_ADDRESS_PUBLIC);
}

void LocalAddr_SetProgrammedBtAddress(const bdaddr* addr)
{
    local_addr.prog_bt_address = *addr;
}

bool LocalAddr_GetProgrammedBtAddress(bdaddr* addr)
{
    if (BdaddrIsZero(&local_addr.prog_bt_address))
    {
        DEBUG_LOG_ERROR("LocalAddr_GetProgrammedBtAddress programmed address not set.");
        return FALSE;
    }
    *addr = local_addr.prog_bt_address;
    return TRUE;
}
