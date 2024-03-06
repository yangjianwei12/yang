/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \ingroup    generic_broadcast_scan_server
    \brief      Implementation of the Generic Broadcast Scan Server module.
*/
#ifdef INCLUDE_GBSS

#include "generic_broadcast_scan_server_msg_handler.h"
#include "generic_broadcast_scan_server_access_ind.h"
#include "le_broadcast_manager_self_scan.h"
#include "gatt_handler_db_if.h"

#define GBSS_SCANNING_STOPED (0x00u)
#define GBSS_SCANNING (0x01u)
#define GBSS_SCANNING_IND  (0x02u) 

static void genericBroadcastScanServer_RegisterCfm(gbss_srv_data_t *GBSS, const CsrBtGattRegisterCfm *cfm)
{
    DEBUG_LOG("genericBroadcastScanServer_RegisterCfm: resultCode(0x%04x) resultSupplier(0x%04x) ", cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
        cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        GBSS->gattId = cfm->gattId;
        GattFlatDbRegisterHandleRangeReqSend(GBSS->gattId,
                                             HANDLE_GENERIC_BROADCAST_SCAN_SERVICE,
                                             HANDLE_GENERIC_BROADCAST_SCAN_SERVICE_END);
    }
    else
    {
        DEBUG_LOG("genericBroadcastScanServer_RegisterCfm: Gatt Registration failed");
        Panic();
    }
}

static void genericBroadcastScanServer_DbRegisterHandleRangeCfm(gbss_srv_data_t *GBSS, const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    UNUSED(GBSS);
    DEBUG_LOG("genericBroadcastScanServer_DbRegisterHandleRangeCfm: resultCode(0x%04x) resultSupplier(0x%04x) ",
              cfm->resultCode, cfm->resultSupplier);
}

static void genericBroadcastScanServer_handleGattPrim(gbss_srv_data_t *GBSS, Message message)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)message;

    switch (*prim)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            genericBroadcastScanServer_RegisterCfm(GBSS, (const CsrBtGattRegisterCfm*) message);
            break;

        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            genericBroadcastScanServer_DbRegisterHandleRangeCfm(GBSS, (const CsrBtGattFlatDbRegisterHandleRangeCfm*)message);
            break;

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            genericBroadcastScanServer_HandleReadAccessInd(GBSS, (const CsrBtGattDbAccessReadInd *)message);
        break;

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            genericBroadcastScanServer_HandleWriteAccessInd(GBSS, (const CsrBtGattDbAccessWriteInd *) message);
        }
        break;

        default:
            DEBUG_LOG("genericBroadcastScanServer_handleGattPrim. Unhandled message MESSAGE:0x%04x", *prim);
        break;
    }
    GattFreeUpstreamMessageContents((void *)message);
}

static gbss_self_scan_status_t genericBroadcastScanServer_convertLeBroadcastManagerStatus(Message message)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *prim = (LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *)message;
    gbss_self_scan_status_t gbssScanStatus = 0;

    switch(prim->status)
    {
        case lebmss_success:
            gbssScanStatus = 0;
        break;

        case lebmss_fail:
            gbssScanStatus = SCANNING_FAILED;
        break;

        case lebmss_bad_parameters:
            gbssScanStatus = SCANNING_BAD_PARAMETERS;
        break;

        case lebmss_timeout:
            gbssScanStatus = SCANNING_TIMEOUT;
        break;

        case lebmss_in_progress:
            gbssScanStatus = SCANNING_IN_PROGRESS;
        break;

        case lebmss_stopped:
            gbssScanStatus = SCANNING_STOPPED;
        break;

        default:
            DEBUG_LOG("genericBroadcastScanServer_convertLeBroadcastManagerStatus. Unhandled status:%d", prim->status);
        break;
    }

    return gbssScanStatus;
}

static void genericBroadcastScanServer_handleGbssScanStartPrim(gbss_srv_data_t *GBSS, Message message)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *prim = (LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM_T *)message;

    if (prim->status == lebmss_success)
    {
            GBSS->gbss_report.scan_active = GBSS_SCAN_ACTIVE;
            GBSS->gbss_report.scan_result = SCANNING_NO_SOURCE;
    }
    else
    {
            GBSS->gbss_report.scan_result = genericBroadcastScanServer_convertLeBroadcastManagerStatus(message);
    }

    DEBUG_LOG("genericBroadcastScanServer_handleGbssScanStartPrim Scan Start CFM received with Status %d\n", prim->status);
    GenericBroadcastScanServer_NotifyGbssScanReport();
}

static void genericBroadcastScanServer_handleGbssScanStopPrim(gbss_srv_data_t *GBSS, Message message)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T *prim = (LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM_T *)message;

    if (prim->status == lebmss_success)
    {
            GBSS->gbss_report.scan_active = GBSS_SCAN_INACTIVE;
    }
    GBSS->gbss_report.scan_result = genericBroadcastScanServer_convertLeBroadcastManagerStatus(message);

    DEBUG_LOG("genericBroadcastScanServer_handleGbssScanStopPrim Scan Stop CFM received with Status %d\n", prim->status);
    GenericBroadcastScanServer_NotifyGbssScanReport();
}

static void genericBroadcastScanServer_handleGbssScanIndPrim(gbss_srv_data_t *GBSS, Message message)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T *prim = (LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND_T *)message;

            GBSS->gbss_report.scan_active = GBSS_SCAN_ACTIVE;
            GBSS->gbss_report.scan_result = SCANNING_SOURCE_FOUND;
            GBSS->gbss_report.encryption_required = prim->encryption_required;
            GBSS->gbss_report.rssi = prim->rssi;
            GBSS->gbss_report.broadcast_name_len = prim->broadcast_name_len;
            free(GBSS->gbss_report.broadcast_name); /* free previous values*/
            GBSS->gbss_report.broadcast_name = CsrPmemZalloc(prim->broadcast_name_len);
            SynMemCpyS(GBSS->gbss_report.broadcast_name, prim->broadcast_name_len, prim->broadcast_name, prim->broadcast_name_len);
            GBSS->gbss_report.source_address = prim->source_tpaddr.taddr;
            GBSS->gbss_report.source_adv_sid = prim->adv_sid;
            GBSS->gbss_report.broadcast_id = prim->broadcast_id;
            GBSS->gbss_report.pa_interval = prim->pa_interval;
            GBSS->gbss_report.num_subgroups = prim->num_subgroups;

            if(GBSS->gbss_report.subgroups)
            {
                free(GBSS->gbss_report.subgroups);
            }
            GBSS->gbss_report.subgroups = CsrPmemZalloc(prim->num_subgroups * sizeof(gbss_subgroups_t));

            for (uint8 subgroup = 0; subgroup < GBSS->gbss_report.num_subgroups; subgroup++)
            {
                GBSS->gbss_report.subgroups[subgroup].metadata_length = prim->subgroups[subgroup].metadata_length;
                free(GBSS->gbss_report.subgroups[subgroup].metadata); /* free previous values*/
                GBSS->gbss_report.subgroups[subgroup].metadata = CsrPmemZalloc(prim->subgroups[subgroup].metadata_length);
                SynMemCpyS(GBSS->gbss_report.subgroups[subgroup].metadata, prim->subgroups[subgroup].metadata_length, prim->subgroups[subgroup].metadata, prim->subgroups[subgroup].metadata_length);
            }

    DEBUG_LOG("genericBroadcastScanServer_handleGbssScanIndPrim Scan IND received with Bid %d\n", prim->broadcast_id);
    GenericBroadcastScanServer_NotifyGbssScanReport();
}

static void genericBroadcastScanServer_handleGbssScanStatusInd(gbss_srv_data_t *GBSS, Message message)
{
    LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T *prim = (LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND_T *)message;

    GBSS->gbss_report.scan_active = GBSS_SCAN_IND;
    GBSS->gbss_report.scan_result = (gbss_self_scan_status_t)prim->status;

    DEBUG_LOG("genericBroadcastScanServer_handleGbssScanStatusInd Scan Status ind received with Status %d\n", prim->status);
    GenericBroadcastScanServer_NotifyGbssScanReport();
}

void genericBroadcastScanServer_MessageHandler(Task task, MessageId id, Message message)
{
    gbss_srv_data_t *GBSS = (gbss_srv_data_t *)task;

    DEBUG_LOG("genericBroadcastScanServer_MessageHandler MESSAGE:0x%x", id);

    switch (id)
    {
    case GATT_PRIM:
            genericBroadcastScanServer_handleGattPrim(GBSS, message);
            break;

    case LE_BROADCAST_MANAGER_SELF_SCAN_START_CFM:
            genericBroadcastScanServer_handleGbssScanStartPrim(GBSS, message);
            break;
    case LE_BROADCAST_MANAGER_SELF_SCAN_STOP_CFM:
            genericBroadcastScanServer_handleGbssScanStopPrim(GBSS, message);
            break;
    case LE_BROADCAST_MANAGER_SELF_SCAN_DISCOVERED_SOURCE_IND:
            genericBroadcastScanServer_handleGbssScanIndPrim(GBSS, message);
            break;
    case LE_BROADCAST_MANAGER_SELF_SCAN_STATUS_IND:
            genericBroadcastScanServer_handleGbssScanStatusInd(GBSS, message);
    default:
            DEBUG_LOG("genericBroadcastScanServer_MessageHandler. Unhandled message MESSAGE:0x%x", id);
            break;
    }
}
#endif /* INCLUDE_GBSS */
