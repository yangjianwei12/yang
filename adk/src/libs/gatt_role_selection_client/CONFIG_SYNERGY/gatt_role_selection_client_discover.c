/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_role_selection_client_discover.h"

#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_debug.h"

#include "gatt_role_selection_server_uuids.h"

#include <uuid.h>

#include <hydra_macros.h>

/*******************************************************************************
 * Helper function to perform next function after discovering primary service handles.
 */ 
static void roleSelectionNextAfterDiscoverPrimaryService(GATT_ROLE_SELECTION_CLIENT *instance)
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if (   !instance->handles.handle_service_start 
        || !instance->handles.handle_service_end)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionNextAfterDiscoverPrimaryService: Did not find all expected handled",
                                         state);
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
    }
    else
    {
        roleSelectionDiscoverAllCharacteristics(instance);
        gattRoleSelectionClientSetState(instance, role_selection_client_finding_handles);
    }
}


/*******************************************************************************
 * Helper function to perform next function after discovering all characteristics of the service.
 */ 
static void roleSelectionNextAfterDiscoverCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance) 
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if (role_selection_client_finding_handles == state)
    {
        if (   !instance->handles.handle_state
            || !instance->handles.handle_figure_of_merit
            || !instance->handles.handle_role_control)
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionNextAfterDiscoverCharacteristics: Did not find all expected handles %d %d %d",
                                                instance->handles.handle_state,
                                                instance->handles.handle_figure_of_merit,
                                                instance->handles.handle_role_control);
            gattRoleSelectionClientSetState(instance, role_selection_client_error);
        }
        else
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        }
    }
    else
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionNextAfterDiscoverCharacteristics: discover characteristics in bad state [0x%x]",
                                          state);
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
    }
}


/*******************************************************************************
 * process a discovered characteristic descriptor, saving if we have none
 */ 
static void roleSelectionProcessDiscoveredDescriptor(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                     const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if ( !instance->handles.handle_state_config
        && CSR_BT_UUID16_SIZE == ind->uuid.length
        && ATT_UUID_CH_C_CONFIG == CSR_BT_UUID_GET_16(ind->uuid))
    {
        if (role_selection_client_finding_notification_handle == state)
        {
            instance->handles.handle_state_config = ind->descriptorHandle;
        }
        else if (role_selection_client_finding_notification_handle_fom == state)
        {
            instance->handles.handle_figure_of_merit_config = ind->descriptorHandle;
        }
        else
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionProcessDiscoveredDescriptor: unexpected cfm in state:%d",
                                             state);
        }
    }
    else
    {
        /* We don't cause an error here as it is feasible that additional
           descriptors could be added. So... we wait for the "end of descriptors" */
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionProcessDiscoveredDescriptor: unexpected cfm");
    }
}

/****************************************************************************/

void roleSelectionDiscoverPrimaryService(GATT_ROLE_SELECTION_CLIENT *instance)
{
    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionDiscoverPrimaryService called");

    /* Initiate primary service discovery */
    GattDiscoverPrimaryServicesBy16BitUuidReqSend(instance->gattId,
                                                  instance->cid,
                                                  UUID_ROLE_SELECTION_SERVICE);
}

/****************************************************************************/
void roleSelectionHandleDiscoverPrimaryServiceInd(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                  const CsrBtGattDiscoverServicesInd *ind)
{
    if (role_selection_client_finding_service_handle != gattRoleSelectionClientGetState(instance))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverPrimaryServiceInd. Not expecting in state %d.",
                                   gattRoleSelectionClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverPrimaryServiceInd. cid:%d start:%d end:%d",
                                     ind->btConnId, ind->startHandle, ind->endHandle);

    instance->handles.handle_service_start = ind->startHandle;
    instance->handles.handle_service_end = ind->endHandle;

    instance->handles.handle_state_end = ind->endHandle;
    instance->handles.handle_figure_of_merit_end = ind->endHandle;
}


/****************************************************************************/
void roleSelectionHandleDiscoverPrimaryServiceCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                  const CsrBtGattDiscoverServicesCfm *cfm)
{
    CsrBtTypedAddr address;
    if (role_selection_client_finding_service_handle != gattRoleSelectionClientGetState(instance))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverPrimaryServiceCfm. Not expecting in state %d.",
                                   gattRoleSelectionClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverPrimaryServiceCfm. status:0x%04x supplier:0x%04x",
                                     cfm->resultCode, cfm->resultSupplier);

    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    GattClientUtilFindAddrByConnId(instance->cid, &address);

    DEBUG_LOG("roleSelectionHandleDiscoverPrimaryServiceCfm  lap 0x%04x uap 0x%01x nap 0x%02x start handle:%d end_handle:%d" , address.addr.lap, address.addr.uap, address.addr.nap, instance->handles.handle_service_start, instance->handles.handle_service_end);
    GattClientRegisterServiceReqSend(instance->gattId, instance->handles.handle_service_start, instance->handles.handle_service_end, address);

}

/****************************************************************************/
void roleSelectionHandleRegisterServiceCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                           const CsrBtGattClientRegisterServiceCfm *cfm)
{
    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleRegisterServiceCfm. status:0x%04x supplier:0x%04x",
                                     cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        if(instance->handles_cached)
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
            instance->handles_cached = FALSE;
        }
        else
        {
            roleSelectionNextAfterDiscoverPrimaryService(instance);
        }
    }
}


/****************************************************************************/
void roleSelectionDiscoverAllCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance)
{
    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionDiscoverAllCharacteristics called");
    /* Initiate characteristic discovery */
    GattDiscoverAllCharacOfAServiceReqSend(instance->gattId,
                                           instance->cid,
                                           instance->handles.handle_service_start,
                                           instance->handles.handle_service_end);
}

static const struct {
    CsrBtUuid   expected_uuid;
    size_t      offset_in_instance;
} expectedCharacteristics[] = {
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROLE_SEL_MIRRORING_STATE)}},     offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_state) },
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROLE_SEL_CONTROL_POINT)}},       offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_role_control) },
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROLE_SEL_FIGURE_OF_MERIT)}},     offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_figure_of_merit) } };

static void roleSelectionMatchAndStoreHandle(GATT_ROLE_SELECTION_CLIENT *instance, const CsrBtUuid *found_uuid, uint16 found_handle)
{
    unsigned i;

    for (i = 0; i< ARRAY_DIM(expectedCharacteristics); i++)
    {
        if (CsrBtUuidCompare(found_uuid,&expectedCharacteristics[i].expected_uuid))
        {
            uint16 *handle = (uint16*)(((uint8 *)instance) + expectedCharacteristics[i].offset_in_instance);
            if (0 == *handle)
            {
                *handle = found_handle;
            }
            else
            {
                GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionMatchAndStoreHandle. duplicate UUID");
            }
            return;
        }
    }
}


/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicsInd(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                      const CsrBtGattDiscoverCharacInd *ind)
{
    if (role_selection_client_finding_handles != gattRoleSelectionClientGetState(instance))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicsInd. Not expecting in state:%d",
                                         gattRoleSelectionClientGetState(instance));

        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    /* The challenge control point can have indications enabled, and we
    apparently need a handle for that. Limit the range we need to look
    in by saving the last possible handle it could be*/
    if (    instance->handles.handle_state
        && (instance->handles.handle_state < ind->valueHandle) 
        && (ind->valueHandle <= instance->handles.handle_state_end))
    {
        instance->handles.handle_state_end = ind->valueHandle - 1;
    }

    if (    instance->handles.handle_figure_of_merit
        && (instance->handles.handle_figure_of_merit < ind->valueHandle) 
        && (ind->valueHandle <= instance->handles.handle_figure_of_merit_end))
    {
        instance->handles.handle_figure_of_merit_end = ind->valueHandle - 1;
    }

    roleSelectionMatchAndStoreHandle(instance, &ind->uuid, ind->valueHandle);
}

/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicsCfm(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                      const CsrBtGattDiscoverCharacCfm *cfm)
{
    if (role_selection_client_finding_handles != gattRoleSelectionClientGetState(instance))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicsCfm. Not expecting in state:%d",
                                         gattRoleSelectionClientGetState(instance));

        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicsResp. Gatt error:0x%04x supplier:0x%04x",
                                     cfm->resultCode, cfm->resultSupplier);
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    roleSelectionNextAfterDiscoverCharacteristics(instance);
}


/****************************************************************************/
void roleSelectionDiscoverAllCharacteristicDescriptors(GATT_ROLE_SELECTION_CLIENT *instance, uint16 start_handle, uint16 end_handle)
{
    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionDiscoverAllCharacteristicDescriptors called");
    /* Initiate characteristic descriptors discovery */
    GattDiscoverAllCharacDescriptorsReqSend(instance->gattId,
                                            instance->cid,
                                            start_handle,
                                            end_handle);;
}

/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsInd(GATT_ROLE_SELECTION_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    roleSelectionProcessDiscoveredDescriptor(instance, ind);
}

/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsCfm(GATT_ROLE_SELECTION_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm)
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsCfm. gatt status:0x%04x supplier:0x%04x",
                                     cfm->resultCode, cfm->resultSupplier);
    }

    switch (state)
    {
        case role_selection_client_finding_notification_handle:
            if (instance->handles.handle_state_config)
            {
                roleSelectionWriteStateClientConfigValue(instance);

                gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification);
            }
            else
            {
                GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. Ended search with no descriptor");

                gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
            }
            break;

        case role_selection_client_finding_notification_handle_fom:
            if (instance->handles.handle_figure_of_merit_config)
            {
                roleSelectionWriteFigureOfMeritClientConfigValue(instance);

                gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification_fom);
            }
            else 
            {
                GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. Ended search with no FOM descriptor");

                gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
            }
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. descriptor in unxpected state:%d",
                                             state);
            GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC();
            break;
    }
}
