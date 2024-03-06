/*!
    \copyright  Copyright (c) 2018 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_advertising_manager_legacy
    \brief      Manage execution of callbacks to construct adverts and scan response
*/

#ifdef INCLUDE_LEGACY_LE_ADVERTISING_MANAGER

#include "le_advertising_manager_data_legacy.h"

#include "le_advertising_manager_data_common.h"
#include "le_advertising_manager_data_packet.h"

#ifdef USE_SYNERGY
#include <csr_bt_cm_prim.h>
#include <cm_lib.h>
#endif
#include <stdlib.h>
#include <panic.h>


/*! Maximum data length of an advert if advertising length extensions are not used */
#define MAX_AD_DATA_SIZE_IN_OCTETS  (0x1F)


static le_advertising_manager_data_packet_t* le_adv_data_packet[le_adv_manager_data_packet_max];


static bool leAdvertisingManager_createNewLegacyDataPacket(le_adv_manager_data_packet_type_t type)
{
    le_advertising_manager_data_packet_t *new_packet = LeAdvertisingManager_DataPacketCreateDataPacket(MAX_AD_DATA_SIZE_IN_OCTETS);
    le_adv_data_packet[type] = new_packet;
    
    return TRUE;
}

static bool leAdvertisingManager_destroyLegacyDataPacket(le_adv_manager_data_packet_type_t type)
{
    LeAdvertisingManager_DataPacketDestroy(le_adv_data_packet[type]);
    le_adv_data_packet[type] = NULL;
    
    return TRUE;
}

static unsigned leAdvertisingManager_getSizeLegacyDataPacket(le_adv_manager_data_packet_type_t type)
{
    return LeAdvertisingManager_DataPacketGetSize(le_adv_data_packet[type]);
}

static bool leAdvertisingManager_addItemToLegacyDataPacket(le_adv_manager_data_packet_type_t type, const le_adv_data_item_t* item)
{
    return LeAdvertisingManager_DataPacketAddDataItem(le_adv_data_packet[type], item);
}

#ifdef USE_SYNERGY
void ConnectionDmBleSetAdvertisingDataReq(uint8 size_advert, const uint8 *advert_start)
{
    DEBUG_LOG_VERBOSE("ConnectionDmBleSetAdvertisingDataReq, Size is %d", size_advert);

    CmLeAdvertiseReqStartSend(AdvManagerGetTask(),
                              CM_LE_ADVERTISE_REQ_CONTEXT_DATA,
                              CSR_BT_CM_LE_MODE_CONTINUE,
                              CSR_BT_CM_LE_PARCHG_DATA_AD,
                              size_advert,
                              CsrMemDup(advert_start, size_advert),
                              0,
                              NULL,
                              0,
                              0,
                              HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED,
                              HCI_ULP_ADVERT_CHANNEL_DEFAULT,
                              HCI_ULP_ADV_FP_ALLOW_ANY,
                              0,
                              NULL);
}

void ConnectionDmBleSetScanResponseDataReq(uint8 size_scan_rsp, const uint8 *scan_rsp_start)
{
    DEBUG_LOG_VERBOSE("ConnectionDmBleSetScanResponseDataReq, Size is %d", size_scan_rsp);

    CmLeAdvertiseReqStartSend(AdvManagerGetTask(),
                              CM_LE_ADVERTISE_REQ_CONTEXT_SCAN_RSP_DATA,
                              CSR_BT_CM_LE_MODE_CONTINUE,
                              CSR_BT_CM_LE_PARCHG_DATA_SR,
                              0,
                              NULL,
                              size_scan_rsp,
                              CsrMemDup(scan_rsp_start, size_scan_rsp),
                              0,
                              0,
                              HCI_ULP_ADVERT_CONNECTABLE_UNDIRECTED,
                              HCI_ULP_ADVERT_CHANNEL_DEFAULT,
                              HCI_ULP_ADV_FP_ALLOW_ANY,
                              0,
                              NULL);
}
#endif

static void leAdvertisingManager_setupLegacyAdvertData(Task task, uint8 adv_handle)
{
    uint8 size_advert = leAdvertisingManager_getSizeLegacyDataPacket(le_adv_manager_data_packet_advert);
    uint8* advert_start = size_advert ? le_adv_data_packet[le_adv_manager_data_packet_advert]->data[0] : NULL;

    UNUSED(task);
    UNUSED(adv_handle);
    
    DEBUG_LOG_VERBOSE("leAdvertisingManager_setupLegacyAdvertData, Size is %d", size_advert);

    leAdvertisingManager_DebugDataItems(size_advert, advert_start);

    ConnectionDmBleSetAdvertisingDataReq(size_advert, advert_start);
}

static void leAdvertisingManager_setupLegacyScanResponseData(Task task, uint8 adv_handle)
{
    uint8 size_scan_rsp = leAdvertisingManager_getSizeLegacyDataPacket(le_adv_manager_data_packet_scan_response);
    uint8* scan_rsp_start = size_scan_rsp ? le_adv_data_packet[le_adv_manager_data_packet_scan_response]->data[0] : NULL;

    UNUSED(task);
    UNUSED(adv_handle);

    DEBUG_LOG("leAdvertisingManager_setupLegacyScanResponseData, Size is %d", size_scan_rsp);

    leAdvertisingManager_DebugDataItems(size_scan_rsp, scan_rsp_start);

    ConnectionDmBleSetScanResponseDataReq(size_scan_rsp, scan_rsp_start);
}

static bool leAdvertisingManager_getLegacyDataPacket(le_adv_manager_data_packet_type_t type, le_advertising_manager_data_packet_t *packet)
{
    if (le_adv_data_packet[type])
    {
        *packet = *le_adv_data_packet[type];
        return TRUE;
    }

    return FALSE;
}

static const le_advertising_manager_data_packet_if_t le_advertising_manager_legacy_data_fns = 
{
    .createNewDataPacket = leAdvertisingManager_createNewLegacyDataPacket,
    .destroyDataPacket = leAdvertisingManager_destroyLegacyDataPacket,
    .getSizeDataPacket = leAdvertisingManager_getSizeLegacyDataPacket,
    .addItemToDataPacket = leAdvertisingManager_addItemToLegacyDataPacket,
    .setupAdvertData = leAdvertisingManager_setupLegacyAdvertData,
    .setupScanResponseData = leAdvertisingManager_setupLegacyScanResponseData,
    .getDataPacket = leAdvertisingManager_getLegacyDataPacket
};

void leAdvertisingManager_RegisterLegacyDataIf(void)
{
    leAdvertisingManager_RegisterDataClient(LE_ADV_MGR_ADVERTISING_SET_LEGACY,
                                            &le_advertising_manager_legacy_data_fns);

    for (unsigned index = 0; index < le_adv_manager_data_packet_max; index++)
    {
        le_adv_data_packet[index] = NULL;
    }
}

#endif
