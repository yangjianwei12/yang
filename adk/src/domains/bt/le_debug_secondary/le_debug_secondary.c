/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to enable LE connection on secondary for additional debugging
            purpose that can be used by the application.
*/

#ifdef ENABLE_LE_DEBUG_SECONDARY
#include "le_debug_secondary.h"

/*! \brief Octet of root key to be modified */
#define ROOT_KEY_OCTET_TO_BE_UPDATED 4
/*! \brief Random number that is added with root key to generate a local IRK */
#define RANDOM_NUMBER_TO_GENERATE_NEW_IRK 1

/*! \brief Handle notification/confirmation from othe modules */
static void leDebugSecondary_HandleMessage(Task task, MessageId id, Message message);

/*! \brief LE Secondary Debug task structure */
typedef struct
{
    /*! LE Debug Secondary task */
    TaskData task;
    /*! Client task registered for notifications */
    Task client_task;
    /*! is local IRK changed for debug purpose */
    bool is_secondary_irk_in_use;
} leDebugSecondaryTaskData;

leDebugSecondaryTaskData le_debug_secondary_task_data = {leDebugSecondary_HandleMessage};

/*! \brief Get current le_debug_secondary taskdata */
#define leDebugSecondary_Get()                  (&le_debug_secondary_task_data)
/*! \brief Get current le_debug_secondary task */
#define leDebugSecondary_GetTask()              (&leDebugSecondary_Get()->task)

#ifdef USE_SYNERGY
/*! \brief Update the root keys and generate new IRK.

    This function is responsible for
    - read root keys from persistent trusted device database
    - Modify the root keys if IRK update is required
    - Refresh the controller device table with the newly generated IRK

    \return TRUE if root key update is success,FALSE otherwise
*/
static bool leDebugSecondary_UpdateKeys(void)
{
    packed_root_keys    rtks;
    cl_root_keys        root_keys;

    PanicFalse(ConnectionReadRootKeys(&root_keys));
    
    if(LeDebugSecondary_IsSecondaryIRKInUse())
    {
        root_keys.ir[ROOT_KEY_OCTET_TO_BE_UPDATED] += RANDOM_NUMBER_TO_GENERATE_NEW_IRK;
        root_keys.er[ROOT_KEY_OCTET_TO_BE_UPDATED] += RANDOM_NUMBER_TO_GENERATE_NEW_IRK;
        DEBUG_LOG("leDebugSecondary_UpdateKeys IR: %04x %04x %04x %04x %04x", root_keys.ir[0],
                  root_keys.ir[1], root_keys.ir[2], root_keys.ir[3], root_keys.ir[4]);
        DEBUG_LOG("leDebugSecondary_UpdateKeys ER: %04x %04x %04x %04x %04x", root_keys.er[0],
                  root_keys.er[1], root_keys.er[2], root_keys.er[3], root_keys.er[4]);
    }

    memmove(&rtks.er, root_keys.er, sizeof(rtks.er));
    memmove(&rtks.ir, root_keys.ir, sizeof(rtks.ir));

    if (!VmUpdateRootKeys(&rtks))
    {
        return FALSE;
    }

    CmRefreshAllDevices();

    return TRUE;
}

/*! \brief Notify the IRK update status to registered clients

    \param #le_debug_update_irk_status_t
*/
static void leDebugSecondary_SendIRKUpdateCfm(le_debug_update_irk_status_t status)
{
    MESSAGE_MAKE(cfm, LE_DEBUG_SECONDARY_UPDATE_IRK_CFM_T);
    cfm->status = status;
    MessageSend(leDebugSecondary_Get()->client_task, LE_DEBUG_SECONDARY_UPDATE_IRK_CFM, cfm);
}

/*! \brief Handle Remove Device confirmation from CM libs
*/
static void leDebugSecondary_HandleCmDmRemoveDeviceOptionsCfm(const CmDmRemoveDeviceOptionsCfm *dev_option_cfm)
{
    le_debug_update_irk_status_t update_status;

    if (dev_option_cfm ->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && leDebugSecondary_UpdateKeys())
    {
        update_status = le_debug_update_irk_status_success;
    }
    else
    {
        update_status = le_debug_update_irk_status_failed;
        leDebugSecondary_Get()->is_secondary_irk_in_use = FALSE;
    }

    DEBUG_LOG_DEBUG("leDebugSecondary_HandleCmDmRemoveDeviceOptionsCfm: status: 0x%x,update_status: enum:le_debug_update_irk_status_t:%d",
                    dev_option_cfm ->resultCode, update_status);
    leDebugSecondary_SendIRKUpdateCfm(update_status);
}

/*! \brief Handle messages from Synergy libs
 */
static void leDebugSecondary_HandleCmPrim(Task task, void *msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    UNUSED(task);

    switch (*prim)
    {
    /* Confirmation from CM */
    case CM_DM_REMOVE_DEVICE_OPTIONS_CFM:
        leDebugSecondary_HandleCmDmRemoveDeviceOptionsCfm((const CmDmRemoveDeviceOptionsCfm *) msg);
        break;

    default:
        DEBUG_LOG("leDebugSecondary_HandleCmPrim Unhandled CM Prim 0x%04x", prim);
        break;
    }

    CmFreeUpstreamMessageContents(msg);
}
#endif

/*! \brief Handle messages from other components
 */
static void leDebugSecondary_HandleMessage(Task task, MessageId id, Message message)
{
    DEBUG_LOG("leDebugSecondary_HandleMessage: MSG:0x%x", id);
    UNUSED(task);

    switch (id)
    {
#ifdef USE_SYNERGY
    case CM_PRIM:
        leDebugSecondary_HandleCmPrim(task, (void *) message);
        break;
#endif
    default:
        DEBUG_LOG("leDebugSecondary_HandleMessage:Unhandled MSG:0x%x", id);
        break;
    }
}

void LeDebugSecondary_UpdateLocalIRK(bool for_secondary_role, Task client)
{
    CsrBtDeviceAddr device_addr = { 0 };

    for_secondary_role = !!for_secondary_role;

    if(leDebugSecondary_Get()->is_secondary_irk_in_use != for_secondary_role)
    {
        leDebugSecondary_Get()->is_secondary_irk_in_use = for_secondary_role;
        leDebugSecondary_Get()->client_task = client;
        /*  Delete the device entries from Bluetooth Security Manager Database */
        CmRemoveDeviceSmDbOnlyRequest(leDebugSecondary_GetTask(), device_addr, CSR_BT_ADDR_INVALID);
    }

    DEBUG_LOG("LeDebugSecondary_UpdateLocalIRK.is_secondary_irk_in_use : 0x%x Enable : 0x%x",
              leDebugSecondary_Get()->is_secondary_irk_in_use, for_secondary_role);
}

bool LeDebugSecondary_IsSecondaryIRKInUse(void)
{
    return leDebugSecondary_Get()->is_secondary_irk_in_use;
}

#endif /* ENABLE_LE_DEBUG_SECONDARY */
