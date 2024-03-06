/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_root_key_client_discover.h"

#include "gatt_root_key_client_init.h"
#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_indication.h"
#include "gatt_root_key_client_write.h"

#include "gatt_root_key_server_uuids.h"

#include <uuid.h>

#include <hydra_macros.h>

/*******************************************************************************
 * Helper function to perform next function after discovering primary service handles.
 */ 
static void rootKeyNextAfterDiscoverPrimaryService(GATT_ROOT_KEY_CLIENT *instance)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   !instance->handle_service_start 
        || !instance->handle_service_end)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverPrimaryService: Did not find all expected handled",
                                    state);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
    else
    {
        rootKeyDiscoverAllCharacteristics(instance);
        gattRootKeyClientSetState(instance, root_key_client_finding_handles);
    }
}


/* Helper function to perform next function after discovering all characteristics of the service.
 */ 
static void rootKeyNextAfterDiscoverCharacteristics(GATT_ROOT_KEY_CLIENT *instance)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (root_key_client_finding_handles == state)
    {
        if (   !instance->handle_challenge_control 
            || !instance->handle_features
            || !instance->handle_keys_control
            || !instance->handle_status)
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: Did not find all expected handled",
                                        state);
            gattRootKeyClientSetState(instance, root_key_client_error);
        }
        else if (!instance->handle_challenge_control_end)
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: Failed to find challenge_control extent",
                                        state);
            gattRootKeyClientSetState(instance, root_key_client_error);
        }
        else
        {
            rootKeyDiscoverAllCharacteristicDescriptors(instance,
                                                        instance->handle_challenge_control+1, 
                                                        instance->handle_challenge_control_end);
            gattRootKeyClientSetState(instance, root_key_client_finding_indication_handle);

        }
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: discover characteristics in bad state [0x%x]",
                                    state);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


/*******************************************************************************
 * Helper function to perform next function after discovering all descriptors of a characteristic.
 */ 
static void rootKeyNextAfterDiscoverDescriptors(GATT_ROOT_KEY_CLIENT *instance)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_finding_indication_handle == state 
        && instance->handle_challenge_control_config_found)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverDescriptors. Enabling indications");
        rootKeyWriteClientConfigValue(instance);

        gattRootKeyClientSetState(instance, root_key_client_enabling_indications);
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverDescriptors: Bad state:%d or no config handle found:%d", 
                                    state, instance->handle_challenge_control_config_found);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


/****************************************************************************/

void rootKeyDiscoverPrimaryService(GATT_ROOT_KEY_CLIENT *instance)
{
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyDiscoverPrimaryService called");

    CsrBtUuid srv_uuid = {CSR_BT_UUID128_SIZE , {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_SERVICE)}};

    /* Initiate primary service discovery */
    GattDiscoverPrimaryServicesBy128BitUuidReqSend(instance->gattId,
                                                   instance->cid,
                                                   srv_uuid.uuid);
}


void rootKeyDiscoverAllCharacteristics(GATT_ROOT_KEY_CLIENT *instance)
{
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyDiscoverAllCharacteristics called");
    /* Initiate characteristic discovery */
    GattDiscoverAllCharacOfAServiceReqSend(instance->gattId,
                                           instance->cid,
                                           instance->handle_service_start,
                                           instance->handle_service_end);
}

/****************************************************************************/
void rootKeyDiscoverAllCharacteristicDescriptors(GATT_ROOT_KEY_CLIENT *instance, uint16 start_handle, uint16 end_handle)
{
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyDiscoverAllCharacteristicDescriptors called");
    /* Initiate characteristic descriptors discovery */
    GattDiscoverAllCharacDescriptorsReqSend(instance->gattId,
                                            instance->cid,
                                            start_handle,
                                            end_handle);
}

/****************************************************************************/
void rootKeyHandleDiscoverPrimaryServiceInd(GATT_ROOT_KEY_CLIENT *instance, 
                                            const CsrBtGattDiscoverServicesInd *ind)
{
    if (root_key_client_finding_service_handle != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverPrimaryServiceInd. Not expecting in state %d.",
                                   gattRootKeyClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverPrimaryServiceInd. cid:%d start:%d end:%d",
                                ind->btConnId, ind->startHandle, ind->endHandle);

    instance->handle_service_start = ind->startHandle;
    instance->handle_service_end = ind->endHandle;

    /* Set the challenge control characteristic end at the end of the service 
     */
    instance->handle_challenge_control_end = ind->endHandle;

    //TODO
    //GattCancelReqSend(instance->gattId, instance->cid);
}


/****************************************************************************/
void rootKeyHandleDiscoverPrimaryServiceCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                            const CsrBtGattDiscoverServicesCfm *cfm)
{
    CsrBtTypedAddr address;
    if (root_key_client_finding_service_handle != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverPrimaryServiceCfm. Not expecting in state %d.",
                                   gattRootKeyClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverPrimaryServiceCfm. status:0x%04x supplier:0x%04x",
                                cfm->resultCode, cfm->resultSupplier);

    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    GattClientUtilFindAddrByConnId(instance->cid, &address);
    
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverPrimaryServiceCfm addr:lap 0x%04x uap 0x%01x nap 0x%02x",address.addr.lap, address.addr.uap, address.addr.nap);

    GattClientRegisterServiceReqSend(instance->gattId, instance->handle_service_start, instance->handle_service_end, address);

}

/****************************************************************************/
void rootKeyHandleRegisterServiceCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                     const CsrBtGattClientRegisterServiceCfm *cfm)
{
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleRegisterServiceCfm. status:0x%04x supplier:0x%04x",
                                cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        rootKeyNextAfterDiscoverPrimaryService(instance);
    }
}


static const struct {
    CsrBtUuid expected_uuid;
    size_t      offset_in_instance;
} expectedCharacteristics[] = {
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_FEATURES)}},     offsetof(GATT_ROOT_KEY_CLIENT, handle_features) },
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_STATUS)}},       offsetof(GATT_ROOT_KEY_CLIENT, handle_status) },
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_CHALLENGE_CONTROL)}}, offsetof(GATT_ROOT_KEY_CLIENT, handle_challenge_control) },
    { {CSR_BT_UUID128_SIZE, {UUID_128_FORMAT_uint8(UUID128_ROOT_KEY_KEYS_CONTROL)}},  offsetof(GATT_ROOT_KEY_CLIENT, handle_keys_control) }};

static void rootKeyMatchAndStoreHandle(GATT_ROOT_KEY_CLIENT *instance, const CsrBtUuid *found_uuid, uint16 found_handle)
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
                GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyMatchAndStoreHandle. duplicate UUID");
                /*GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyMatchAndStoreHandle. duplicate UUID %8x%8x%8x%8x",
                                found_uuid[0],found_uuid[1],found_uuid[2],found_uuid[3]);*/ //TODO
            }
            return;
        }
    }

    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyMatchAndStoreHandle. Failed to find handle");
}

/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicsInd(GATT_ROOT_KEY_CLIENT *instance, 
                                                 const CsrBtGattDiscoverCharacInd *ind)
{
    if (root_key_client_finding_handles != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicsInd. Not expecting in state %d.",
                                   gattRootKeyClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    /* The challenge control point can have indications enabled, and we
        apparently need a handle for that. Limit the range we need to look
        in by saving the last possible handle it could be*/
    if (    instance->handle_challenge_control
        && (instance->handle_challenge_control < ind->valueHandle) 
        && (ind->valueHandle <= instance->handle_challenge_control_end))
    {
        instance->handle_challenge_control_end = ind->valueHandle - 1;
    }

    /* We only have 128 bit uuids */
    if (ind->uuid.length == CSR_BT_UUID128_SIZE)
    {
        rootKeyMatchAndStoreHandle(instance, &ind->uuid, ind->valueHandle);
    }
}

/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicsCfm(GATT_ROOT_KEY_CLIENT *instance, 
                                                const CsrBtGattDiscoverCharacCfm *cfm)
{
    if (root_key_client_finding_handles != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicsCfm. Not expecting in state %d.",
                                   gattRootKeyClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicsCfm. Not expecting gatt status:0x%04x supplier:0x%04x",
                                cfm->resultCode, cfm->resultSupplier);
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    rootKeyNextAfterDiscoverCharacteristics(instance);

}

/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsInd(GATT_ROOT_KEY_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_finding_indication_handle == state
        && CSR_BT_UUID16_SIZE == ind->uuid.length
        && ATT_UUID_CH_C_CONFIG == CSR_BT_UUID_GET_16(ind->uuid))
    {
        instance->handle_challenge_control_config = ind->descriptorHandle;
        instance->handle_challenge_control_config_found = TRUE;
    }
    else
    {
        /* We don't cause an error here as it is feasible that additional
           descriptors could be added. So... we wait for the "end of descriptors" */
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicDescriptorsInd: unexpected state [0x%x] handle:%d", 
                                    state, ind->descriptorHandle);
    }
}

/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsCfm(GATT_ROOT_KEY_CLIENT *instance, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm)
{
    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicDescriptorsCfm. gatt status:0x%04x supplier:0x%04x",
                                cfm->resultCode, cfm->resultSupplier);
    }
    rootKeyNextAfterDiscoverDescriptors(instance);
}

