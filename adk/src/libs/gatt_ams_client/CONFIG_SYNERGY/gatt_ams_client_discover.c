/* Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_discover.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_write.h"
#include "gatt_ams_client_notification.h"
#include "gatt_ams_client_external_msg_send.h"
#include "gatt_ams_client_ready_state.h"
#include <uuid.h>
#include <bdaddr.h>

static CsrBtUuid128 ams_remote_command_uuid = {UUID_128_FORMAT_uint8(AMS_CLIENT_REMOTE_COMMAND_UUID128)};
static CsrBtUuid128 ams_entity_update_uuid = {UUID_128_FORMAT_uint8(AMS_CLIENT_ENTITY_UPDATE_UUID128)};
static CsrBtUuid128 ams_entity_attribute_uuid = {UUID_128_FORMAT_uint8(AMS_CLIENT_ENTITY_ATTRIBUTE_UUID128)};


static void nextAfterDiscoverCharacteristics(GAMS *ams)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_init:
        {
            /* All characteristics found, there are no mandatory characteristics */
            gattAmsSendInitResponse(ams, gatt_ams_status_success);
            gattAmsReadyStateUpdate(ams, TRUE);
        }
        break;

        default:
        {
            DEBUG_PANIC(("AMS: No action after characteristic discovery [0x%04x]\n", ams->pending_cmd));
        }
        break;
    }
}

static void processDiscoveredDescriptor(GAMS *ams, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_remote_command_notify_enable:
        case ams_pending_remote_command_notify_disable:
        {
            if (ind->uuid.length == CSR_BT_UUID16_SIZE)
            {
                if (CSR_BT_UUID_GET_16(ind->uuid) == ATT_UUID_CH_C_CONFIG)
                {
                    bool notify_pending = (ams->pending_cmd == ams_pending_remote_command_notify_enable);

                    PRINT(("AMS: Found Remote Command CCD handle = [0x%04x]\n", ind->descriptorHandle));

                    gattAmsWriteCharacteristicNotifyConfig(ams, notify_pending, ind->descriptorHandle);
                    ams->remote_command_ccd = ind->descriptorHandle;
                    ams->pending_cmd = ams_pending_write_remote_command_cconfig;
                }
            }
        }
        break;

        case ams_pending_entity_update_notify_enable:
        case ams_pending_entity_update_notify_disable:
        {
            if (ind->uuid.length == CSR_BT_UUID16_SIZE)
            {
                if (CSR_BT_UUID_GET_16(ind->uuid) == ATT_UUID_CH_C_CONFIG)
                {
                    bool notify_pending = (ams->pending_cmd == ams_pending_entity_update_notify_enable);

                    PRINT(("AMS: Found Entity Update CCD handle = [0x%04x]\n", ind->descriptorHandle));

                    gattAmsWriteCharacteristicNotifyConfig(ams, notify_pending, ind->descriptorHandle);
                    ams->entity_update_ccd = ind->descriptorHandle;
                    ams->pending_cmd = ams_pending_write_entity_update_cconfig;
                }
            }
        }
        break;

        case ams_pending_write_remote_command_cconfig:
        case ams_pending_write_entity_update_cconfig:
            PRINT(("AMS: Processing descriptor, state [0x%04x]\n", ams->pending_cmd));
        break;
        
        default:
            DEBUG_PANIC(("AMS: Wrong state for descriptor processing, state [0x%04x], handle [0x%04x]\n", ams->pending_cmd, ind->descriptorHandle));
        break;
    }
}

static void nextAfterDiscoverDescriptors(GAMS *ams)
{
    switch (ams->pending_cmd)
    {
        case ams_pending_remote_command_notify_enable:
        case ams_pending_remote_command_notify_disable:
            gattAmsSendSetRemoteCommandNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
            break;

        case ams_pending_entity_update_notify_enable:
        case ams_pending_entity_update_notify_disable:
            gattAmsSendSetEntityUpdateNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
            break;

        case ams_pending_write_remote_command_cconfig:
        case ams_pending_write_entity_update_cconfig:
            /* No action needed as write of client configuration descriptor will happen next */
            break;

        default:
            DEBUG_PANIC(("AMS: No action after descriptor discovery [0x%04x]\n", ams->pending_cmd));
            break;
    }
}

uint16 gattAmsfindEndHandleForCharDesc(GAMS *ams, uint16 startHandle, uint16 endHandle, uint8 characteristic)
{
    uint8 charIndex = 0;
    uint8 charVal = 0;
    uint8 mask;
    uint8 char_report_mask = ams->char_report_mask;

    unsigned retHandle = 0;

    /* The characteristics are 2 bit fields overlaid in the same byte
       Our task data has a mask for what to report, and the required
       characteristic value
     */

    /* if and only if there is proper characteristic request for the descriptor */
    while( charIndex < GATT_AMS_CLIENT_MAX_CHAR )
    {
        if(char_report_mask)
        {
            mask = GATT_AMS_CLIENT_FIELD_MASK(charIndex);
            /* Mask the value and shift */
            charVal = char_report_mask & mask;
            charVal = charVal >> GATT_AMS_CLIENT_FIELD_START(charIndex);

            /* Remove the value we have just checked from the report mask */
            char_report_mask = (char_report_mask & ~mask);

            /* Did the value match the one we wanted */
            if( charVal == characteristic)
            {
                /* Check the next field */
                mask = GATT_AMS_CLIENT_FIELD_MASK(charIndex+1);;
                charVal = (char_report_mask & mask);
                charVal = charVal >> GATT_AMS_CLIENT_FIELD_START(charIndex+1);

                switch( charVal )
                {
                    case GATT_AMS_CLIENT_REMOTE_COMMAND:
                        retHandle = ams->remote_command_handle- 1;
                    break;

                    case GATT_AMS_CLIENT_ENTITY_UPDATE:
                        retHandle = ams->entity_update_handle - 1;
                    break;

                    case GATT_AMS_CLIENT_ENTITY_ATTRIBUTE:
                        retHandle = ams->entity_attribute_handle - 1;
                    break;

                    default:
                    {
                        /* TODO : Need to check this */
                        if(startHandle < endHandle)
                        {
                            retHandle = endHandle;
                        }
                        else
                        {
                            retHandle = startHandle;
                        }
                    }
                    break;
                }
                /* Exit loop */
                break;
            }
       }
       charIndex ++;
    }

    return (uint16)retHandle;
}

void handleAmsClientRegisterCfm(GAMS *ams,const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        CsrBtTypedAddr address;
        ams->gattId = cfm->gattId;

        PRINT(("AMS Client Initialised\n"));

        GattClientUtilFindAddrByConnId(ams->cid, &address);

        PRINT(("handleAmsClientRegisterCfm  lap 0x%04x uap 0x%01x nap 0x%02x start handle:%d end_handle:%d\n" , address.addr.lap, address.addr.uap, address.addr.nap, ams->service_start_handle, ams->service_end_handle));
        GattClientRegisterServiceReqSend(ams->gattId, ams->service_start_handle, ams->service_end_handle, address);
    }
}

void handleAmsClientUnRegisterCfm(GAMS *ams, const CsrBtGattUnregisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        gattAmsReadyStateUpdate(ams, FALSE);

        MessageFlushTask(&ams->lib_task);

        PRINT(("AMS: Destroyed instance [%p] with cid [0x%02x]\n", (void *) ams, ams->cid));
    }

    MessageSend(ams->app_task, GATT_AMS_CLIENT_DEINIT_CFM, NULL);
}

void handleAmsClientRegisterServiceCfm(GAMS *ams, const CsrBtGattClientRegisterServiceCfm *cfm)
{
    PRINT(("handleAmsClientRegisterServiceCfm status:0x%04x supplier:0x%04x \n", cfm->resultCode, cfm->resultSupplier));

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        GattDiscoverAllCharacOfAServiceReqSend(ams->gattId,
                                               ams->cid,
                                               ams->service_start_handle,
                                               ams->service_end_handle);
    }
}

void handleAmsDiscoverAllCharacteristicsInd(GAMS *ams, const CsrBtGattDiscoverCharacInd *ind)
{
    uint8  charIndex = 0;
    uint8  charVal = 0;
        /* char_report_mask is 8 bit value which is divided as 2 bits each for the 3 characteristic. 
        The least significant 2 bits indicates the first characteristic discovered, 
        the second significant 2 bits indicates the second characteristic discovered and so on & so forth.
        This mask is later used for getting the start & end handle for discovering characteristic descriptor
        for each characteristic
    */
    charIndex = ams->char_report_mask_index;
        
    if(ind->uuid.length == CSR_BT_UUID128_SIZE)
    {
        if(!CsrMemCmp(ind->uuid.uuid, ams_remote_command_uuid, CSR_BT_UUID128_SIZE))
        {
            PRINT(("AMS: Found Remote Command handle [0x%04x]n", ind->valueHandle));
            ams->remote_command_handle = ind->valueHandle;
            charVal = GATT_AMS_CLIENT_REMOTE_COMMAND;
        }
        else if(!CsrMemCmp(ind->uuid.uuid, ams_entity_update_uuid, CSR_BT_UUID128_SIZE))
        {
            PRINT(("AMS: Found Entity Update handle [0x%04x]\n", ind->valueHandle));
            ams->entity_update_handle = ind->valueHandle;
            charVal = GATT_AMS_CLIENT_ENTITY_UPDATE;
        }
        else if(!CsrMemCmp(ind->uuid.uuid, ams_entity_attribute_uuid, CSR_BT_UUID128_SIZE))
        {
            PRINT(("AMS: Found Entity Attribute handle [0x%04x]\n", ind->valueHandle));
            ams->entity_attribute_handle = ind->valueHandle;
            charVal = GATT_AMS_CLIENT_ENTITY_ATTRIBUTE;
        }

        if (charVal)
        {
            charVal = (uint8)(charVal << GATT_AMS_CLIENT_FIELD_START(charIndex));
            ams->char_report_mask |= charVal;
            charIndex++;
        }
        ams->char_report_mask_index = charIndex;
    }
        /* Ignore unwanted characteristics */
}


void handleAmsDiscoverAllCharacteristicsCfm(GAMS *ams, const CsrBtGattDiscoverCharacCfm *cfm)
{
    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        PRINT(("handleAmsDiscoverAllCharacteristicsCfm resultCode:0x%04x resultSupplier:0x%04x\n", cfm->resultCode, cfm->resultSupplier));
        return;
    }

    /* No more to come, so process the characteristics */
    /* Reset the index as this is going to be used in getting the descriptor */
    ams->char_report_mask_index = 0;
    nextAfterDiscoverCharacteristics(ams);
}

bool gattAmsDiscoverAllCharacteristicDescriptors(GAMS *ams, uint16 start_handle, uint16 end_handle)
{
    GattDiscoverAllCharacDescriptorsReqSend(ams->gattId,
                                            ams->cid,
                                            start_handle,
                                            end_handle);
    return TRUE;
}

void handleAmsDiscoverAllCharacteristicDescriptorsInd(GAMS *ams, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    processDiscoveredDescriptor(ams, ind);
}

void handleAmsDiscoverAllCharacteristicDescriptorsCfm(GAMS *ams, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm)
{
    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        PRINT(("handleAmsDiscoverAllCharacteristicDescriptorsCfm resultCode:0x%04x resultSupplier:0x%04x \n", cfm->resultCode, cfm->resultSupplier));
        return;
    }

    nextAfterDiscoverDescriptors(ams);
}

