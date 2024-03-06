/* Copyright (c) 2014 - 2022 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client_discover.h"
#include "gatt_apple_notification_client_private.h"
#include "gatt_apple_notification_client_external_msg_send.h"
#include "gatt_apple_notification_client_ready_state.h"
#include "gatt_apple_notification_client_write.h"
#include <uuid.h>
#include <bdaddr.h>
#include <logging.h>

static const uint8 apple_notification_ns_uuid[] = {UUID_128_FORMAT_uint8(APPLE_NOTIFICATION_NS_UUID128)};
static const uint8 apple_notification_cp_uuid[] = {UUID_128_FORMAT_uint8(APPLE_NOTIFICATION_CP_UUID128)};
static const uint8 apple_notification_ds_uuid[] = {UUID_128_FORMAT_uint8(APPLE_NOTIFICATION_DS_UUID128)};

static void nextAfterDiscoverCharacteristics(GANCS *ancs)
{
    switch (ancs->pending_cmd)
    {
        case ancs_pending_discover_all_characteristics:
            /* Check if the mandatory Notification Source is supported by the server, otherwise there is no point to initialise client */
            if (CHECK_VALID_HANDLE(ancs->notification_source))
            {
                gattAncsSendInitResponse(ancs, CSR_BT_GATT_RESULT_SUCCESS);
                gattAncsReadyStateUpdate(ancs, TRUE);
            }
            else
                gattAncsSendInitResponse(ancs, CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR);
        break;

        default:
            DEBUG_PANIC("nextAfterDiscoverCharacteristics: no action; pending_cmd=enum:ancs_pending_cmd_t:%d", ancs->pending_cmd);
        break;
    }
}

static void processDiscoveredDescriptor(GANCS *ancs, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    switch (ancs->pending_cmd)
    {
        case ancs_pending_discover_all_characteristic_descriptors:
            /* Expected discovered descriptor, wait for more */
        break;

        case ancs_pending_set_ns_notify_enable:
        case ancs_pending_set_ns_notify_disable:
            if (ind->uuid.length == CSR_BT_UUID16_SIZE)
            {
                if (CSR_BT_UUID_GET_16(ind->uuid) == ATT_UUID_CH_C_CONFIG)
                {
                    bool notify_pending = (ancs->pending_cmd == ancs_pending_set_ns_notify_enable);

                    DEBUG_LOG_INFO("processDiscoveredDescriptor: found Notification Source CCD handle=0x%04X", ind->descriptorHandle);

                    writeClientConfigNotifyValue(ancs, notify_pending, ind->descriptorHandle);
                    ancs->ns_ccd = ind->descriptorHandle;
                    ancs->pending_cmd = ancs_pending_write_ns_cconfig;
                }
            }
        break;

        case ancs_pending_set_ds_notify_enable:
        case ancs_pending_set_ds_notify_disable:
            if (ind->uuid.length == CSR_BT_UUID16_SIZE)
            {
                if (CSR_BT_UUID_GET_16(ind->uuid) == ATT_UUID_CH_C_CONFIG)
                {
                    bool notify_pending = (ancs->pending_cmd == ancs_pending_set_ds_notify_enable);

                    DEBUG_LOG_INFO("processDiscoveredDescriptor: found Data Source CCD handle=0x%04X", ind->descriptorHandle);

                    writeClientConfigNotifyValue(ancs, notify_pending, ind->descriptorHandle);
                    ancs->ds_ccd = ind->descriptorHandle;
                    ancs->pending_cmd = ancs_pending_write_ds_cconfig;
                }
            }
        break;

        case ancs_pending_write_ns_cconfig:
        case ancs_pending_write_ds_cconfig:
            DEBUG_LOG_INFO("processDiscoveredDescriptor: pending_cmd=enum:ancs_pending_cmd_t:%d", ancs->pending_cmd);
        break;

        default:
            DEBUG_PANIC("processDiscoveredDescriptor: wrong state; pending_cmd=enum:ancs_pending_cmd_t:%d, handle=0x%04X", ancs->pending_cmd, ind->descriptorHandle);
        break;
    }
}

static void nextAfterDiscoverDescriptors(GANCS *ancs)
{
    switch (ancs->pending_cmd)
    {
        case ancs_pending_discover_all_characteristic_descriptors:
            ancs->pending_cmd = ancs_pending_none;
            break;

        case ancs_pending_set_ns_notify_enable:
        case ancs_pending_set_ns_notify_disable:
            gattAncsSendSetNotificationSourceNotificationResponse(ancs, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
            break;

        case ancs_pending_set_ds_notify_enable:
        case ancs_pending_set_ds_notify_disable:
            gattAncsSendSetDataSourceNotificationResponse(ancs, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
            break;

        case ancs_pending_write_ns_cconfig:
        case ancs_pending_write_ds_cconfig:
            /* No action needed as write of client configuration descriptor will happen next */
            break;

        default:
            DEBUG_PANIC("nextAfterDiscoverDescriptors: no action; pending_cmd=enum:ancs_pending_cmd_t:%d", ancs->pending_cmd);
            break;
    }
}


/*******************************************************************************
 * Helper function to get the endhandle for discovering characteristic descriptor of NS.
 */
uint16 findEndHandleForCharDesc(GANCS *ancs, uint16 startHandle, uint16 endHandle, uint8 characteristic_wanted)
{
    uint8 charIndex = 0;
    uint8 charVal = 0;
    uint8 mask;
    uint8 char_report_mask = ancs->char_report_mask;

    unsigned retHandle = 0;

    /* The characteristics are 2 bit fields overlaid in the same byte
       Our task data has a mask for what to report, and the required
       characteristic value
     */

    /* if and only if there is proper characteristic request for the descriptor */
    while( charIndex < GATT_APPLE_NOTIFICATION_MAX_CHAR )
    {
        if(char_report_mask)
        {
            mask = GATT_APPLE_NOTIFICATION_FIELD_MASK(charIndex);
            /* Mask the value and shift */
            charVal = char_report_mask & mask;
            charVal = charVal >> GATT_APPLE_NOTIFICATION_FIELD_START(charIndex);

            /* Remove the value we have just checked from the report mask */
            char_report_mask = (char_report_mask & ~mask);

            /* Did the value match the one we wanted */
            if( charVal == characteristic_wanted)
            {
                /* Check the next field */
                mask = GATT_APPLE_NOTIFICATION_FIELD_MASK(charIndex+1);;
                charVal = (char_report_mask & mask);
                charVal = charVal >> GATT_APPLE_NOTIFICATION_FIELD_START(charIndex+1);

                switch( charVal )
                {
                    case GATT_APPLE_NOTIFICATION_NS:
                        retHandle = ancs->notification_source- 1;
                    break;

                    case GATT_APPLE_NOTIFICATION_DS:
                        retHandle = ancs->data_source - 1;
                    break;

                    case GATT_APPLE_NOTIFICATION_CP:
                        retHandle = ancs->control_point - 1;
                    break;

                    default:
                    {
                        /* TODO : This is probably wrong! */
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

/****************************************************************************/
void handleAncsClientRegisterCfm(GANCS *ancs,const CsrBtGattRegisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        CsrBtTypedAddr address;
        ancs->gattId = cfm->gattId;

        DEBUG_LOG_FN_ENTRY("handleAncsClientRegisterCfm: client initialised");

        GattClientUtilFindAddrByConnId(ancs->cid, &address);

        DEBUG_LOG_INFO("handleAncsClientRegisterCfm: addr=%04x %02x %06x start handle=%u end_handle=%u",
            address.addr.nap, address.addr.uap, address.addr.lap,
            ancs->service_start_handle, ancs->service_end_handle);
            
        GattClientRegisterServiceReqSend(ancs->gattId, ancs->service_start_handle, ancs->service_end_handle, address);
    }
}

/****************************************************************************/
void handleAncsClientUnRegisterCfm(GANCS *ancs, const CsrBtGattUnregisterCfm *cfm)
{
    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
    {
        gattAncsReadyStateUpdate(ancs, FALSE);

        MessageFlushTask(&ancs->lib_task);

        DEBUG_LOG_INFO("handleAncsClientUnRegisterCfm: destroyed instance=%p cid=0x%X", (void *) ancs, ancs->cid);
    }
    MessageSend(ancs->app_task, GATT_ANCS_DEINIT_CFM, NULL);
}

/****************************************************************************/
void handleAncsClientRegisterServiceCfm(GANCS *ancs, const CsrBtGattClientRegisterServiceCfm *cfm)
{
    DEBUG_LOG_FN_ENTRY("handleAncsClientRegisterServiceCfm: status=0x%04x supplier=0x%04x", cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        GattDiscoverAllCharacOfAServiceReqSend(ancs->gattId,
                                               ancs->cid,
                                               ancs->service_start_handle,
                                               ancs->service_end_handle);
    }
}


/****************************************************************************/
void handleAncsDiscoverAllCharacteristicsInd(GANCS *ancs, const CsrBtGattDiscoverCharacInd *ind)
{
    uint8  charIndex = 0;
    uint8  charVal = 0;
        /* char_report_mask is 8 bit value which is divided as 2 bits each for the 3 characteristic.
        The least significant 2 bits indicates the first characteristic discovered,
        the second significant 2 bits indicates the second characteristic discovered and so on & so forth.
        This mask is later used for getting the start & end handle for discovernig characteristic descriptor
        for each characteristic
    */
    charIndex = ancs->char_report_mask_index;

    if (ind->uuid.length == CSR_BT_UUID128_SIZE)
    {
        if (!CsrMemCmp(ind->uuid.uuid, apple_notification_ns_uuid, CSR_BT_UUID128_SIZE))
        {
            DEBUG_LOG_INFO("handleAncsDiscoverAllCharacteristicsInd: found Notification Source handle=0x%04X", ind->valueHandle);
            ancs->notification_source = ind->valueHandle;
            charVal = GATT_APPLE_NOTIFICATION_NS;
        }
        else if(!CsrMemCmp(ind->uuid.uuid, apple_notification_ds_uuid, CSR_BT_UUID128_SIZE))
        {
            DEBUG_LOG_INFO("handleAncsDiscoverAllCharacteristicsInd: found Data Source handle=0x%04X", ind->valueHandle);
            ancs->data_source = ind->valueHandle;
            charVal = GATT_APPLE_NOTIFICATION_DS;
        }
        else if(!CsrMemCmp(ind->uuid.uuid, apple_notification_cp_uuid, CSR_BT_UUID128_SIZE))
        {
            DEBUG_LOG_INFO("handleAncsDiscoverAllCharacteristicsInd: found Control Point handle=0x%04X", ind->valueHandle);
            ancs->control_point = ind->valueHandle;
            charVal = GATT_APPLE_NOTIFICATION_CP;
        }

        if (charVal)
        {
            charVal = (uint8)(charVal << GATT_APPLE_NOTIFICATION_FIELD_START(charIndex));
            ancs->char_report_mask |= charVal;
            charIndex++;
        }
        ancs->char_report_mask_index = charIndex;
    }
        /* Ignore unwanted characteristics */
}

/****************************************************************************/
void handleAncsDiscoverAllCharacteristicsCfm(GANCS *ancs, const CsrBtGattDiscoverCharacCfm *cfm)
{
    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        DEBUG_LOG_WARN("handleAncsDiscoverAllCharacteristicsCfm: error 0x%04x supplier 0x%04x", cfm->resultCode, cfm->resultSupplier);
        return;
    }

    /* No more to come, so process the characteristics */
    /* Reset the index as this is going to be used in getting the descriptor */
    ancs->char_report_mask_index = 0;
    nextAfterDiscoverCharacteristics(ancs);
}

/****************************************************************************/
bool ancsDiscoverAllCharacteristicDescriptors(GANCS *ancs, uint16 start_handle, uint16 end_handle)
{
    GattDiscoverAllCharacDescriptorsReqSend(ancs->gattId,
                                            ancs->cid,
                                            start_handle,
                                            end_handle);
    return TRUE;
}

/****************************************************************************/
void handleAncsDiscoverAllCharacteristicDescriptorsInd(GANCS *ancs, const CsrBtGattDiscoverCharacDescriptorsInd *ind)
{
    processDiscoveredDescriptor(ancs, ind);
}


/****************************************************************************/
void handleAncsDiscoverAllCharacteristicDescriptorsCfm(GANCS *ancs, const CsrBtGattDiscoverCharacDescriptorsCfm *cfm)
{
    if (!(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS && cfm->resultSupplier == CSR_BT_SUPPLIER_GATT))
    {
        DEBUG_LOG_WARN("handleAncsDiscoverAllCharacteristicDescriptorsCfm: error 0x%04x supplier 0x%04x", cfm->resultCode, cfm->resultSupplier);
        return;
    }

    nextAfterDiscoverDescriptors(ancs);
}
