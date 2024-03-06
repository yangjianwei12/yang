/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_server_access.h"
#include "gatt_role_selection_server_notify.h"
#include "gatt_role_selection_server_debug.h"
#include "gatt_role_selection_server_db.h"
#include "gatt_role_selection_server_private.h"

#include <string.h>

/*! Send an error read access response.
*/
static void sendRoleSelectionReadAccessErrorRsp(const GATT_ROLE_SELECTION_SERVER *instance, 
                                           const CsrBtGattDbAccessReadInd *access_ind, 
                                           uint16 error)
{
    GattDbReadAccessResSend(instance->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error,
                            0,
                            NULL);
}

static void roleSelectionSendChangeRoleInd(GATT_ROLE_SELECTION_SERVER *instance, 
                                           GattRoleSelectionServiceControlOpCode opcode)
{
    MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND);

    message->command = opcode;
    MessageSend(instance->app_task, GATT_ROLE_SELECTION_SERVER_CHANGE_ROLE_IND, message);
}

static void roleSelectionControlAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                       const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;

    if (access_ind->writeUnit[0].valueLength >= GRSS_SIZE_CONTROL_PRI_SEC_PDU_OCTETS)
    {
        uint8 opcode = access_ind->writeUnit[0].value[GRSS_CONTROL_PRI_SEC_OFFSET_OPCODE];
        response = CSR_BT_GATT_ACCESS_RES_INVALID_PDU;

        switch (opcode)
        {
            case GrssOpcodeBecomePrimary:
            case GrssOpcodeBecomeSecondary:
                if (GRSS_SIZE_CONTROL_PRI_SEC_PDU_OCTETS == access_ind->writeUnit[0].valueLength)
                {
                    roleSelectionSendChangeRoleInd(instance, opcode);
                    response = CSR_BT_GATT_ACCESS_RES_SUCCESS;
                }
                else
                {
                    response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
                }
                break;

            default:
                break;
        }
    }

    /*! \todo Probably need to only send this here for some limited error cases
        and have the app/profile send the remainder */
    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);
}


/*!
    Handle write access to the client config option for the mirroring state

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void roleSelectionStateClientConfigWriteAccess(GATT_ROLE_SELECTION_SERVER *instance,
                                                      const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
    uint16 old_config = instance->mirror_client_config;
    uint16 config_value = 0;
    if (access_ind->writeUnit[0].valueLength == GRSS_CLIENT_CONFIG_OCTET_SIZE)
    {
        config_value =    (access_ind->writeUnit[0].value[0] & 0xFF) 
                              + ((access_ind->writeUnit[0].value[1] << 8) & 0xFF00);

        instance->mirror_client_config = config_value;

        response = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);

    if (response == CSR_BT_GATT_ACCESS_RES_SUCCESS && config_value && 
        !old_config && !instance->mirror_state_notified)
    {
        /* Now enabled and latest state has not been read/notified */
        sendInternalMirrorStateChanged(instance, access_ind->btConnId);
    }
}


/*!
    Handle write access to the client config option for the figure of merit

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void roleSelectionFigureClientConfigWriteAccess(GATT_ROLE_SELECTION_SERVER *instance,
                                                       const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
    uint16 config_value = 0;
    if (access_ind->writeUnit[0].valueLength == GRSS_CLIENT_CONFIG_OCTET_SIZE)
    {
        config_value = (access_ind->writeUnit[0].value[0] & 0xFF) 
                     + ((access_ind->writeUnit[0].value[1] << 8) & 0xFF00);

        instance->merit_client_config = config_value;

        response = CSR_BT_GATT_ACCESS_RES_SUCCESS;

    }
    
    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);

    /* If notifications are enabled, always send the current value. */
    if (response == CSR_BT_GATT_ACCESS_RES_SUCCESS && config_value)
    {
        /* Reset the notified flag because the client has explicitly
         * asked for notifications and may not have received previous ones. 
        */
        instance->figure_of_merit_notified = FALSE;
         
        sendInternalFigureOfMeritChanged(instance, access_ind->btConnId);
    }
}



/***************************************************************************/
void handleRoleSelectionServiceReadAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                          const CsrBtGattDbAccessReadInd *access_ind)
{
    GATT_ROLE_SELECTION_SERVER_DEBUG("handleRoleSelectionServiceReadAccess: handle %d",access_ind->attrHandle);
    uint16 handle = access_ind->attrHandle;
    uint16 size_value = 0;
    uint8  *value = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    
    switch (handle)
    {
        case HANDLE_ROLE_SELECTION_SERVICE:
            break;

        case HANDLE_ROLE_SELECTION_MIRRORING_STATE:
        {
            value = (uint8*) CsrPmemAlloc(GRSS_SIZE_MIRROR_STATE_PDU_OCTETS);
            value[0] = (uint8)instance->mirror_state;
            instance->mirror_state_notified = TRUE;
            size_value = GRSS_SIZE_MIRROR_STATE_PDU_OCTETS;
        }
        break;

        case HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT:
        {
            value = (uint8*) CsrPmemAlloc(GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS);
            value[0] = instance->figure_of_merit & 0xFF;
            value[1] = (instance->figure_of_merit >> 8) & 0xFF;
            instance->figure_of_merit_notified = TRUE;
            size_value = GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS;
        }
        break;

        case HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG:
        {
            value = (uint8*) CsrPmemAlloc(GRSS_CLIENT_CONFIG_OCTET_SIZE);
            value[0] = (instance->mirror_client_config) & 0xFF;
            value[1] = ((instance->mirror_client_config) >> 8) & 0xFF;
            size_value = GRSS_CLIENT_CONFIG_OCTET_SIZE;
        }
        break;

        case HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG:
        {
            value = (uint8*) CsrPmemAlloc(GRSS_CLIENT_CONFIG_OCTET_SIZE);
            value[0] = (instance->merit_client_config) & 0xFF;
            value[1] = ((instance->merit_client_config) >> 8) & 0xFF;
            size_value = GRSS_CLIENT_CONFIG_OCTET_SIZE;
         }
         break;

        default:
            sendRoleSelectionReadAccessErrorRsp(instance, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }

    GattDbReadAccessResSend(instance->gattId, access_ind->btConnId, 
                            access_ind->attrHandle,
                            status, size_value,
                            value);
}

/***************************************************************************/
void handleRoleSelectionServiceWriteAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                           const CsrBtGattDbAccessWriteInd *access_ind)
{
   GATT_ROLE_SELECTION_SERVER_DEBUG("handleRoleSelectionServiceWriteAccess: handle %d",access_ind->attrHandle);
   uint16 handle = access_ind->attrHandle;
   
    switch (handle)
    {
        case HANDLE_ROLE_SELECTION_CONTROL:
            roleSelectionControlAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_MIRRORING_STATE_CLIENT_CONFIG:
            roleSelectionStateClientConfigWriteAccess(instance, access_ind);
            break;

        case HANDLE_ROLE_SELECTION_FIGURE_OF_MERIT_CLIENT_CONFIG:
            roleSelectionFigureClientConfigWriteAccess(instance, access_ind);
            break;

        default:
            GattDbWriteAccessResSend(instance->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }
}


/***************************************************************************/

void GattRoleSelectionServerSetMirrorState(GATT_ROLE_SELECTION_SERVER *instance,
                                           uint32 cid,
                                           GattRoleSelectionServiceMirroringState mirror_state)
{
    GattRoleSelectionServiceMirroringState old_state = instance->mirror_state;
    bool old_notified = instance->mirror_state_notified;

    if (mirror_state != old_state)
    {
        instance->mirror_state = mirror_state;
        instance->mirror_state_notified = FALSE;
        sendInternalMirrorStateChanged(instance, cid);
    }
    else if (!old_notified)
    {
        sendInternalMirrorStateChanged(instance, cid);
    }
}


bool GattRoleSelectionServerSetFigureOfMerit(GATT_ROLE_SELECTION_SERVER *instance,
                                             uint32 cid,
                                             grss_figure_of_merit_t figure_of_merit,
                                             bool force_notify)
{

    if (!instance->initialised)
    {
        return FALSE;
    }

    grss_figure_of_merit_t old_fom = instance->figure_of_merit;
    bool old_notified = instance->figure_of_merit_notified;

    GATT_ROLE_SELECTION_SERVER_DEBUG("GattRoleSelectionServerSetFigureOfMerit old_fom 0x%x old_notified %d", old_fom, old_notified);

    if ((figure_of_merit != old_fom) || force_notify)
    {
        instance->figure_of_merit = figure_of_merit;
        instance->figure_of_merit_notified = FALSE;
        sendInternalFigureOfMeritChanged(instance, cid);
    }
    else if (!old_notified)
    {
        sendInternalFigureOfMeritChanged(instance, cid);
    }

    return TRUE;
}

