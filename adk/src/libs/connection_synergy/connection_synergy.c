/****************************************************************************
 Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.

 FILE NAME
 connection_synergy.c

 DESCRIPTION
    Synergy implementation of connection library APIs.

 ****************************************************************************/

#include "connection.h"
#include "connection_no_ble.h"
#include <logging.h>
#include <stdlib.h>
#include <bdaddr.h>
#include <cm_lib.h>
#include <vm.h>
#include <sink.h>

#ifdef HYDRACORE
#define VM_POINTER_TYPECAST(ptr)        ptr
#else
#define VM_POINTER_TYPECAST(ptr)        ((uint16) ptr)
#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(ConnectionMessageId)

void ConnectionReadTxPower(Task theAppTask, const tp_bdaddr *tpaddr)
{
    CsrBtDeviceAddr deviceAddr = { 0 };

    BdaddrConvertVmToBluestack(&deviceAddr, &tpaddr->taddr.addr);

    /* read current tx power level */
    CmReadTxPowerLevelReqSend(theAppTask,
                              deviceAddr,
                              tpaddr->taddr.type,
                              tpaddr->transport,
                              0);
}

void ConnectionReadLocalName(Task theAppTask)
{
    CmReadLocalNameReqSend(theAppTask);
}

void ConnectionReadRemoteName(Task theAppTask, const bdaddr *bd_addr)
{
    BD_ADDR_T addr = { 0 };
    BdaddrConvertVmToBluestack(&addr, bd_addr);
    CmReadRemoteNameReqSend(theAppTask, addr);
}

void ConnectionReadRemoteNameCancel(Task theAppTask, const bdaddr *bd_addr)
{
    BD_ADDR_T addr = { 0 };
    BdaddrConvertVmToBluestack(&addr, bd_addr);
    CmCancelReadRemoteNameReqSend(theAppTask, addr);
}

void ConnectionRegisterServiceRecord(Task appTask,
                                     uint16 num_rec_bytes,
                                     const uint8 *service_record)
{
    CmSdsRegisterReqSend(appTask, num_rec_bytes, (uint8*)service_record, CSR_BT_CM_CONTEXT_UNUSED);
}

void ConnectionEnterDutMode(void)
{
    DEBUG_LOG("ConnectionEnterDutMode");
    /* In Connection Library implementation, if Bluetooth version is less than 
     * 2.1, security is disabled. 
     * In Synergy CM ,Bluetooth version is set to 5.1 currently. So security is 
     * not disabled.
     */
    CmEnableDutModeReqSend(NULL);
}

void ConnectionWriteEirData(uint8 fec_required,
                            uint8 size_eir_data,
                            const uint8 *eir_data)
{
    uint8 *eir_ptr = (uint8 *)eir_data;

    UNUSED(fec_required);
    CmSetEirDataReqSend(NULL, FALSE, size_eir_data, eir_ptr);
}

void ConnectionWriteInquiryAccessCode(Task theAppTask,
                                      const uint32 *iac,
                                      uint16 num_iac)
{
    UNUSED(theAppTask);
    UNUSED(num_iac);

    /* Inquiry Scan on GIAC only */
    CmWriteIacReqSend(NULL, iac[0]);
}

#ifndef DISABLE_BLE
/*****************************************************************************/
void ConnectionDmBleReadAdvertisingChannelTxPower(Task theAppTask)
{
    CmReadAdvertisingChTxPowerReqSend(theAppTask, 1234)
}
#endif

void ConnectionReadLocalAddr(Task theAppTask)
{
    CmReadLocalBdAddrReqSend(theAppTask);
}

void ConnectionRfcommAllocateChannel(Task theAppTask,
                                     uint8 suggested_server_channel)
{
    CmRegisterReqSend(theAppTask,
                      0,
                      suggested_server_channel,
                      CM_REGISTER_OPTION_APP_CONNECTION_HANDLING |
                      CM_REGISTER_OPTION_ALLOW_DYNAMIC_SERVER_CHANNEL);
}

/*****************************************************************************/
void ConnectionRfcommDeallocateChannel(Task theAppTask,
                                       uint8 local_server_channel)
{
    UNUSED(theAppTask);
    CmUnRegisterReqSend(local_server_channel);
}

/*****************************************************************************/
void ConnectionRfcommDisconnectRequest(Task appTask, Sink sink)
{
    UNUSED(appTask);
    uint16 cid = SinkGetRfcommConnId(sink);
    CmContextDisconnectReqSend(CM_CREATE_RFC_CONN_ID(cid), 0);
}

/*****************************************************************************/
bool ConnectionSmGetIndexedAttributeNowReq(
        uint16          ps_base,
        uint16          index,
        uint16          size_psdata,
        uint8           *psdata,
        typed_bdaddr    *taddr
        )
{
    UNUSED(ps_base);
    CsrBtTypedAddr deviceAddr;
    if(!CsrBtTdDbReadEntryByIndex(index, CSR_BT_TD_DB_SOURCE_DIRECT, CSR_BT_TD_DB_DIRECT_KEY_ATTR, &size_psdata, psdata, &deviceAddr, NULL))
    {
        BdaddrConvertBluestackToVm(&taddr->addr, &deviceAddr.addr);
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
#ifndef DISABLE_BLE
void ConnectionDmBleConnectionParametersUpdateReq(Task theAppTask,
                                                  typed_bdaddr *taddr,
                                                  uint16 min_interval,
                                                  uint16 max_interval,
                                                  uint16 latency,
                                                  uint16 timeout,
                                                  uint16 min_ce_length,
                                                  uint16 max_ce_length);
void ConnectionDmBleConnectionParametersUpdateReq(Task theAppTask,
                                                  typed_bdaddr *taddr,
                                                  uint16 min_interval,
                                                  uint16 max_interval,
                                                  uint16 latency,
                                                  uint16 timeout,
                                                  uint16 min_ce_length,
                                                  uint16 max_ce_length)
{
    DEBUG_LOG("Stub: ConnectionDmBleConnectionParametersUpdateReq");
    CsrBtTypedAddr peer_addr;
    peer_addr.type = taddr->type;
    BdaddrConvertVmToBluestack(&peer_addr.addr, &taddr->addr);

    CmLeConnparamUpdateReqSend(theAppTask,
                               peer_addr,
                               min_interval,
                               max_interval,
                               latency,
                               timeout,
                               min_ce_length,
                               max_ce_length);
}

void ConnectionDmBleSetAdvertiseEnable(bool enable)
{
    ConnectionDmBleSetAdvertiseEnableReq(NULL, enable);
}

void ConnectionDmBleSetAdvertiseEnableReq(Task theAppTask, bool enable)
{
    CmLeAdvertiseReqStartSend(theAppTask,
                            2 /*CM_LE_ADVERTISE_REQ_CONTEXT_ENABLE*/,
                            (enable)?CSR_BT_CM_LE_MODE_ON:CSR_BT_CM_LE_MODE_OFF,
                            CSR_BT_CM_LE_PARCHG_NONE,
                            0,
                            NULL,
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

void ConnectionDmUlpSetPrivacyModeReq(const typed_bdaddr *peer_taddr,
                                      const privacy_mode mode)
{
    CsrBtTypedAddr addr = { 0 };
    BdaddrConvertTypedVmToBluestack(&addr, peer_taddr);
    CmLeSetPrivacyModeReqSend(NULL, addr, mode);
}

bool ConnectionBleClearAdvertisingReportFilter(void)
{
    return VmClearAdvertisingReportFilter();
}

bool ConnectionBleAddAdvertisingReportFilter(ble_ad_type ad_type,
                                             uint16 interval,
                                             uint16 size_pattern,
                                             const uint8 *pattern)
{
    uint8 *ptrn = (uint8*) PanicUnlessMalloc(size_pattern);
    memmove(ptrn, pattern, size_pattern);

    (void) VmAddAdvertisingReportFilter(0, /* Operation is always OR */
                                        ad_type,
                                        interval,
                                        size_pattern,
                                        VM_POINTER_TYPECAST(ptrn));
    return TRUE;
}
#endif

void ConnectionSmAuthenticate(Task theAppTask,
                              const bdaddr *bd_addr,
                              uint16 timeout)
{
    CsrBtDeviceAddr deviceAddr;

    UNUSED(theAppTask);
    UNUSED(timeout);

    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CmScDmRemoveDeviceReq(deviceAddr, CSR_BT_ADDR_PUBLIC);
    CsrBtCmScDmBondingReq(deviceAddr, 0);
}

void ConnectionL2capRegisterRequest(Task clientTask, uint16 psm, uint16 flags)
{
    UNUSED(flags);
    CmL2caRegisterReqSend(clientTask,
                          psm,
                          L2CA_MODE_MASK_BASIC,
                          0,
                          CSR_BT_CM_CONTEXT_UNUSED,
                          CM_L2CA_REGISTER_OPTION_APP_CONNECTION_HANDLING);
}

void ConnectionL2capDisconnectRequest(Task appTask, Sink sink)
{
    UNUSED(appTask);
    CsrBtConnId btConnId = CM_CREATE_L2CA_CONN_ID(SinkGetL2capCid(sink));
    CmL2caDisconnectReqSend(btConnId, CSR_BT_CM_CONTEXT_UNUSED);
}

bool ConnectionReadRootKeys(cl_root_keys *root_keys)
{
    CsrBtTdDbSystemInfo systemInfo = { 0 };

    if(CsrBtTdDbGetSystemInfo(&systemInfo) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        memmove(root_keys->er, systemInfo.er, sizeof(root_keys->er));
        memmove(root_keys->ir, systemInfo.ir, sizeof(root_keys->ir));

        return TRUE;
    }

    memset(root_keys, 0, sizeof(*root_keys));
    return FALSE;
}

void ConnectionReadRemoteVersionBdaddr(Task theAppTask, const tp_bdaddr *tpaddr)
{
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, &tpaddr->taddr.addr);
    CmReadRemoteVersionReqSend(theAppTask, deviceAddr, tpaddr->taddr.type, tpaddr->transport);
}

void ConnectionUnregisterServiceRecord(Task appTask, uint32 service_record_hdl)
{
    CmSdsUnRegisterReqSend(appTask, service_record_hdl, CSR_BT_CM_CONTEXT_UNUSED);
}

void ConnectionSetRoleBdaddr(Task theAppTask,
                             const bdaddr *bd_addr,
                             hci_role role)
{
    CsrBtDeviceAddr deviceAddr;
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CmSwitchRoleReqSend(theAppTask, deviceAddr, role);
}

void ConnectionGetRoleBdaddr(Task theAppTask, const bdaddr *bd_addr)
{
    CsrBtDeviceAddr deviceAddr;
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CmRoleDiscoveryReqSend(theAppTask, deviceAddr);
}

const handover_interface connection_handover_if;

bool ConnectionAuthSetPriorityDevice(
        const bdaddr* bd_addr,
        bool is_priority_device
        )
{
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CsrBtTdDbPrioritiseDevice(TYPED_BDADDR_PUBLIC,
                              &deviceAddr,
                              (is_priority_device ? CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE : CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE));

    return TRUE;
}
void ConnectionSmSetSdpSecurityIn(bool enable)
{
    if(enable)
    {
        CmSecurityRegisterRequest(SEC_PROTOCOL_L2CAP, SDP_PSM, FALSE, SECL_NONE, 0);
    }
    else
    {
        CmSecurityUnregisterRequest(SEC_PROTOCOL_L2CAP, SDP_PSM);
    }
}

void ConnectionSmDeleteAllAuthDevices(uint16 ps_base)
{
    UNUSED(ps_base);
    /* Remove all but priority devices from the trusted device list */
    CsrBtDeviceAddr deviceAddr = { 0 };
    CmScDmRemoveDeviceReq(deviceAddr, CSR_BT_ADDR_INVALID);
}


void ConnectionSmDeleteAuthDeviceReq(uint8 type, const bdaddr* bd_addr)
{
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CmScDmRemoveDeviceReq(deviceAddr, type);
}

void ConnectionSmUpdateMruDevice(const bdaddr *bd_addr)
{
    CsrBtDeviceAddr syn_addr;
    BdaddrConvertVmToBluestack(&syn_addr, bd_addr);
    CsrBtTdDbPrioritiseDevice(CSR_BT_ADDR_PUBLIC, &syn_addr, CSR_BT_TD_DB_UPDATE_FLAG_MRU);
}

void ConnectionSmPutAttributeReq(
        uint16 ps_base,
        uint8 addr_type,
        const bdaddr* bd_addr,
        uint16 size_psdata,
        const uint8* psdata
        )
{
    UNUSED(ps_base);
    UNUSED(addr_type);
    CsrBtDeviceAddr deviceAddr;
    BdaddrConvertVmToBluestack(&deviceAddr, bd_addr);
    CsrBtTdDbWriteEntry(TYPED_BDADDR_PUBLIC,
                        &deviceAddr,
                        CSR_BT_TD_DB_SOURCE_DIRECT,
                        CSR_BT_TD_DB_DIRECT_KEY_ATTR,
                        (CsrUint16) size_psdata,
                        psdata);
}

uint16 ConnectionSmGetIndexedAttributeSizeNowReq( uint16 index )
{
    uint16 pdd_frame_length = 0;
    CsrBtTdDbReadEntryByIndex(index,
                              CSR_BT_TD_DB_SOURCE_DIRECT,
                              CSR_BT_TD_DB_DIRECT_KEY_ATTR,
                              &pdd_frame_length,
                              NULL,
                              NULL,
                              NULL);
    return pdd_frame_length;
}

void ConnectionSmBleReadRandomAddressTaskReq(
    Task                            theAppTask,
    ble_read_random_address_flags   flags,
    const tp_bdaddr                 *peer_tpaddr
    )
{
    CsrBtTypedAddr deviceAddr;
    if(peer_tpaddr)
    {
        BdaddrConvertTypedVmToBluestack(&deviceAddr, &peer_tpaddr->taddr);
    }
    else
    {
        CsrBtAddrZero(&(deviceAddr));
    }
    CmLeReadRandomAddressReqSend(theAppTask, deviceAddr, flags);
}

void ConnectionSmAddAuthDevice(
        Task theAppTask,
        const bdaddr *peer_bd_addr,
        uint16 trusted,
        uint16 bonded,
        uint8 key_type,
        uint16 size_link_key,
        const uint16* link_key
        )
{
    UNUSED(bonded);
    CsrBtDeviceAddr deviceAddr = { 0 };
    CsrBtCmKey *bredr_key = PanicUnlessMalloc(sizeof(CsrBtCmKey));
    memset(bredr_key, 0, sizeof(CsrBtCmKey));
    BdaddrConvertVmToBluestack(&deviceAddr, peer_bd_addr);

    bredr_key->bredrKey.authorised = trusted;
    bredr_key->bredrKey.linkkeyLen = size_link_key * sizeof(uint16);
    bredr_key->bredrKey.linkkeyType = key_type;
    memcpy(bredr_key->bredrKey.linkkey, link_key, size_link_key * sizeof(uint16));

    CmWriteBredrKeysReqSend(theAppTask,
                            CSR_BT_ADDR_PUBLIC,
                            &deviceAddr,
                            bredr_key);
}

void ConnectionSmAddAuthDeviceRawRequest(
        Task                theAppTask,
        const typed_bdaddr  *taddr,
        uint16              size_data,
        const uint16        *data
        )
{
    UNUSED(size_data);
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, &taddr->addr);
    CmWriteBredrKeysReqSend(theAppTask,
                            CSR_BT_ADDR_PUBLIC,
                            &deviceAddr,
                            (CsrBtCmKey *) CsrMemDup((uint8*)data, sizeof(CsrBtTdDbBredrKey)));
}

void ConnectionSmGetAuthDeviceRawRequest(
        Task                theAppTask,
        const typed_bdaddr  *peer_taddr
        )
{
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, &peer_taddr->addr);
    CmReadBredrKeysReqSend(theAppTask,
                           CSR_BT_ADDR_PUBLIC,
                           &deviceAddr);
}

void ConnectionDmBleSetConnectionParametersReq(
    const ble_connection_params *params)
{
    /* Note:
     * 1)By default conn_attempt_timeout is set to
     *   CSR_BT_LE_DEFAULT_CONN_ATTEMPT_TIMEOUT.The value can be modified in
     *   csr_bt_usr_config.h if required.
     * 2)own_address_type value set initially during application initialisation
     *   is used here. CmLeSetOwnAddressTypeReqSend() can be used to change the
     *   address type if required.
     */
    CmLeConnparamReqSend(NULL,
                         params->scan_interval,
                         params->scan_window,
                         params->conn_interval_min,
                         params->conn_interval_max,
                         params->conn_latency,
                         params->supervision_timeout,
                         params->conn_latency_max,
                         params->supervision_timeout_min,
                         params->supervision_timeout_max);
}

void ConnectionIsocConnectResponse(Task theAppTask,
                                   uint16 cis_handle,
                                   uint8 status
                                   )
{
    CmIsocCisConnectRsp(theAppTask,
                        cis_handle,
                        status);
}

void ConnectionIsocSetupIsochronousDataPathRequest(Task theAppTask,
                                                   uint16 cis_handle,
                                                   uint8 data_path_direction,
                                                   uint8 data_path_id
                                                   )
{
    CsrUint8 codec_id[CM_CODEC_ID_SIZE];
    memset(codec_id, 0, CM_CODEC_ID_SIZE * sizeof(CsrUint8));
    CmIsocSetupIsoDataPathReq(theAppTask,
                              cis_handle,
                              data_path_direction,
                              data_path_id,
                              codec_id,
                              0,
                              0,
                              NULL);
}

void ConnectionDmBleExtScanGetGlobalParamsReq(Task theAppTask)
{
    CmExtScanGetGlobalParamsRequest(theAppTask);
}

void ConnectionDmBleExtScanSetParamsReq(Task theAppTask,
                                        uint8 flags,
                                        uint8 own_address_type,
                                        uint8 scan_filter_policy,
                                        uint8 filter_duplicates,
                                        uint8 scanning_phys,
                                        CL_ES_SCANNING_PHY_T phy_params[EXT_SCAN_MAX_SCANNING_PHYS])
{
    CsrUint8 index;
    CmScanningPhy phys[EXT_SCAN_MAX_SCANNING_PHYS];

    if (own_address_type > CL_LOCAL_ADDR_GENERATE_RPA_FBR)
    {
        DEBUG_LOG_ERROR("ConnectionDmBleExtScanSetParamsReq : own_address_type value %d is not valid !!",own_address_type);
        return;
    }

    if (scanning_phys & CM_EXT_SCAN_LE_2M_PHY_BIT_MASK)
    {
        DEBUG_LOG_ERROR("ConnectionDmBleExtScanSetParamsReq : scanning_phys %d, Scan parameters could not be set for LE 2M !!",scanning_phys);
        return;
    }

    for (index = 0; index < EXT_SCAN_MAX_SCANNING_PHYS; index++)
    {
        phys[index].scan_type = phy_params[index].scan_type;
        phys[index].scan_interval = phy_params[index].scan_interval;
        phys[index].scan_window = phy_params[index].scan_window;
    }
    CmExtScanSetGlobalParamsRequest(theAppTask,
                                    flags,
                                    own_address_type,
                                    scan_filter_policy,
                                    filter_duplicates,
                                    scanning_phys,
                                    phys);
}

void ConnectionDmBleExtScanRegisterScannerReq(Task    theAppTask,
                                              uint32  flags,
                                              uint16  adv_filter,
                                              uint16  adv_filter_sub_field1,
                                              uint32  adv_filter_sub_field2,
                                              uint16  ad_structure_filter,
                                              uint16  ad_structure_filter_sub_field1,
                                              uint32  ad_structure_filter_sub_field2,
                                              uint16  ad_structure_info_len,
                                              uint8   *ad_structure_info[CL_AD_STRUCT_INFO_BYTE_PTRS])
{
    CsrUint8 index;
    CsrUint8 regAdTypes[CL_AD_STRUCT_INFO_BYTE_PTRS];

    if (ad_structure_info_len > CL_AD_STRUCT_INFO_BYTE_PTRS)
    {
        DEBUG_LOG_ERROR("ConnectionDmBleExtScanRegisterScannerReq : ad_structure_info_len %d is more than expected %d !!",ad_structure_info_len, CL_AD_STRUCT_INFO_BYTE_PTRS);
        return;
    }

    for (index = 0; index < ad_structure_info_len; index++)
    {
        regAdTypes[index] = *ad_structure_info[index];
    }
    CmExtScanRegisterScannerReq(theAppTask,
                                flags,
                                adv_filter,
                                adv_filter_sub_field1,
                                adv_filter_sub_field2,
                                ad_structure_filter,
                                ad_structure_filter_sub_field1,
                                ad_structure_filter_sub_field2,
                                ad_structure_info_len,
                                regAdTypes);
}

bool ConnectionDmBleExtScanUnregisterScannerReq(Task theAppTask, uint8 scan_handle)
{
    CmExtScanUnregisterScannerReq(theAppTask, scan_handle);
    return TRUE;
}

void ConnectionDmBleExtScanEnableReq(Task theAppTask,
                                     bool enable,
                                     uint8 num_of_scanners,
                                     uint8 scan_handle[],
                                     uint16 duration[])
{
    CsrUint8 index;
    CmScanners scanners[CM_EXT_SCAN_MAX_SCANNERS];

    if (num_of_scanners > CM_EXT_SCAN_MAX_SCANNERS)
    {
        DEBUG_LOG_ERROR("ConnectionDmBleExtScanEnableReq : num_of_scanners %d is more than expected %d !!",num_of_scanners, CM_EXT_SCAN_MAX_SCANNERS);
        return;
    }

    for(index = 0; index < num_of_scanners; index++)
    {
        scanners[index].scan_handle = scan_handle[index];
        scanners[index].duration = duration[index];
    }
    CmExtScanEnableScannersReq(theAppTask, enable, num_of_scanners, scanners);
}

void ConnectionDmBleExtScanGetCtrlScanInfoReq(Task theAppTask)
{
    CmExtScanGetCtrlScanInfoReq(theAppTask);
}

void ConnectionDmBlePeriodicScanStopFindTrainsReq(Task    theAppTask,
                                                 uint8   scan_handle
                                                 )
{
    CmPeriodicScanStopFindTrainsReq(theAppTask,
                                    scan_handle);
}

void ConnectionDmBlePeriodicScanSyncTransferParamsReq(Task theAppTask,
                                                      typed_bdaddr taddr,
                                                      uint16 skip,
                                                      uint16 sync_timeout,
                                                      uint8 mode,
                                                      uint8 cte_type
                                                      )
{
    TYPED_BD_ADDR_T tbdAddr;
    tbdAddr.type = taddr.type;
    BdaddrConvertVmToBluestack(&tbdAddr.addr, &taddr.addr);
    CmPeriodicScanSyncTransferParamsReq(theAppTask,
                                        tbdAddr,
                                        skip,
                                        sync_timeout,
                                        mode,
                                        cte_type);
}

void ConnectionDmBlePeriodicScanSyncTrainReq(Task    theAppTask,
                                             uint8   report_periodic,
                                             uint16  skip,
                                             uint16  sync_timeout,
                                             uint8   sync_cte_type,
                                             uint16  attempt_sync_for_x_seconds,
                                             uint8   number_of_periodic_trains,
                                             CL_DM_ULP_PERIODIC_SCAN_TRAINS_T periodic_trains[CL_MAX_PERIODIC_TRAIN_LIST_SIZE]
                                             )
{
    uint8 i;
    CmPeriodicScanTrains train_to_sync[CL_MAX_PERIODIC_TRAIN_LIST_SIZE];

    for (i = 0; i < number_of_periodic_trains; i++)
    {
        train_to_sync[i].advSid = periodic_trains[i].adv_sid;
        train_to_sync[i].addrt.type = periodic_trains[i].taddr.type;
        BdaddrConvertVmToBluestack(&train_to_sync[i].addrt.addr, &periodic_trains[i].taddr.addr);
    }

    CmPeriodicScanSyncToTrainReq(theAppTask,
                                 report_periodic,
                                 skip,
                                 sync_timeout,
                                 sync_cte_type,
                                 attempt_sync_for_x_seconds,
                                 number_of_periodic_trains,
                                 train_to_sync);
}

void ConnectionDmBlePeriodicScanSyncCancelReq(Task theAppTask)
{
    CmPeriodicScanSyncToTrainCancelReq(theAppTask);
}

void ConnectionIsocBigCreateSyncRequest(Task theAppTask,
                                        uint16 sync_handle,
                                        uint16 big_sync_timeout,
                                        uint8 big_handle,
                                        uint8 mse,
                                        uint8 encryption,
                                        uint8 *broadcast_code,
                                        uint8 num_bis,
                                        uint8 *bis
                                        )
{
    CmIsocBigCreateSyncReq(theAppTask,
                           sync_handle,
                           big_sync_timeout,
                           big_handle,
                           mse,
                           encryption,
                           broadcast_code,
                           num_bis,
                           bis);
}

bool ConnectionDmBlePeriodicScanSyncTerminateReq(Task theAppTask,
                                                 uint16 sync_handle
                                                 )
{
    CmPeriodicScanSyncTerminateReq(theAppTask,
                                   sync_handle);
    return TRUE;
}

void ConnectionDmBlePeriodicScanSyncTransferReq(Task theAppTask,
                                                typed_bdaddr taddr,
                                                uint16 service_data,
                                                uint16 sync_handle)
{
    TYPED_BD_ADDR_T tbdAddr;
    tbdAddr.type = taddr.type;
    BdaddrConvertVmToBluestack(&tbdAddr.addr, &taddr.addr);
    CmPeriodicScanSyncTransferRequest(theAppTask, tbdAddr, service_data, sync_handle);
}

void ConnectionDmBleExtAdvReadMaxAdvDataLenReq(Task theAppTask, uint8 adv_handle)
{
    CmExtAdvReadMaxAdvDataLenRequest(theAppTask, adv_handle);
}

void ConnectionDmBleExtAdvSetDataReq(Task theAppTask,
                                    uint8 adv_handle,
                                    set_data_req_operation operation,
                                    uint8 adv_data_len,
                                    uint8 *adv_data[8]
                                    )
{
    CmExtAdvSetDataReq(theAppTask,
                       adv_handle,
                       operation,
                       0,
                       adv_data_len,
                       adv_data);
}

void ConnectionDmBleExtAdvSetScanRespDataReq(Task theAppTask,
                                            uint8 adv_handle,
                                            set_data_req_operation operation,
                                            uint8 scan_resp_data_len,
                                            uint8 *scan_resp_data[8]
                                            )
{
    CmExtAdvSetScanRespDataReq(theAppTask,
                               adv_handle,
                               operation,
                               0,
                               scan_resp_data_len,
                               scan_resp_data);
}


void ConnectionDmBleExtAdvertiseEnableReq(Task theAppTask,
                                          bool enable,
                                          uint8 adv_handle
                                          )
{
    CmExtAdvEnableReq(theAppTask,
                      adv_handle,
                      enable);
}

void ConnectionDmBleExtAdvSetParamsReq(Task theAppTask,
                                       uint8 adv_handle,
                                       uint16 adv_event_properties,
                                       uint32 primary_adv_interval_min,
                                       uint32 primary_adv_interval_max,
                                       uint8 primary_adv_channel_map,
                                       uint8 own_addr_type,
                                       typed_bdaddr taddr,
                                       uint8 adv_filter_policy,
                                       uint16 primary_adv_phy,
                                       uint8 secondary_adv_max_skip,
                                       uint16 secondary_adv_phy,
                                       uint16 adv_sid
                                       )
{
    TYPED_BD_ADDR_T tbdAddr;
    tbdAddr.type = taddr.type;
    BdaddrConvertVmToBluestack(&tbdAddr.addr, &taddr.addr);
    CmExtAdvSetParamsReq(theAppTask,
                         adv_handle,
                         adv_event_properties,
                         primary_adv_interval_min,
                         primary_adv_interval_max,
                         primary_adv_channel_map,
                         own_addr_type,
                         tbdAddr,
                         adv_filter_policy,
                         primary_adv_phy,
                         secondary_adv_max_skip,
                         secondary_adv_phy,
                         adv_sid,
                         0);
}


void ConnectionDmBleExtAdvSetRandomAddressReq(Task theAppTask,
                                                uint8 adv_handle,
                                                ble_local_addr_type action,
                                                bdaddr random_addr)
{
    CsrBtDeviceAddr deviceAddr = { 0 };
    BdaddrConvertVmToBluestack(&deviceAddr, &random_addr);
    CmExtAdvSetRandomAddrReq(theAppTask,
                             adv_handle,
                             action,
                             deviceAddr);
}


void ConnectionDmBleExtAdvRegisterAppAdvSetReq(Task theAppTask,
                                               uint8 adv_handle
                                               )
{
    CmExtAdvRegisterAppAdvSetReq(theAppTask,
                                 adv_handle,
                                 0);
}

void ConnectionDmBleExtAdvUnregisterAppAdvSetReq(Task theAppTask,
                                                 uint8 adv_handle)
{
    CmExtAdvUnregisterAppAdvSetReq(theAppTask, adv_handle);
}

void ConnectionIsocRegister(Task theAppTask,
                            uint16 isoc_type
                            )
{
    CmIsocRegisterReq(theAppTask,
                      isoc_type);
}

void ConnectionDmBlePeriodicScanSyncAdvReportEnableReq(
    Task    theAppTask,
    uint16  sync_handle,
    uint8   enable)
{
    CmPeriodicScanSyncAdvReportEnableReq(theAppTask, sync_handle, enable);
}

void ConnectionDmBlePeriodicScanStartFindTrainsReq(
    Task    theAppTask,
    uint32  flags,
    uint16  scan_for_x_seconds,
    uint16  ad_structure_filter,
    uint16  ad_structure_filter_sub_field1,
    uint32  ad_structure_filter_sub_field2,
    uint16  ad_structure_info_len,
    uint8   *ad_structure_info[CL_AD_STRUCT_INFO_BYTE_PTRS])
{
    CsrUint8 *data = NULL;

    ad_structure_info_len = (ad_structure_info_len < CM_PERIODIC_SCAN_AD_STRUCT_INFO_LENGTH) ?
        ad_structure_info_len : CM_PERIODIC_SCAN_AD_STRUCT_INFO_LENGTH;

    if (ad_structure_info_len && ad_structure_info)
    {
        CsrUint8 index, offset, length;
        data = CsrPmemAlloc(ad_structure_info_len);

        for(offset = 0, index = 0; offset < ad_structure_info_len;
                               index++, offset += length)
        {
            length = ad_structure_info_len - offset;
            if(length > CM_PERIODIC_ADV_DATA_BLOCK_SIZE)
                length = CM_PERIODIC_ADV_DATA_BLOCK_SIZE;

            memcpy(&data[offset], ad_structure_info[index], length);
        }
    }
    else
    {
        ad_structure_info_len = 0;
    }

    CmPeriodicScanStartFindTrainsReq(theAppTask,
        flags,
        scan_for_x_seconds,
        ad_structure_filter,
        ad_structure_filter_sub_field1,
        ad_structure_filter_sub_field2,
        ad_structure_info_len,
        data);
}

void ConnectionDmBlePeriodicScanSyncLostRsp(uint16 sync_handle)
{
    CmPeriodicScanSyncLostRsp(sync_handle);
}

void ConnectionSmSirkOperationReq(
    Task theAppTask,
    const tp_bdaddr *tpaddr,
    uint16 flags,
    uint8 sirk_key[CL_SM_SIRK_KEY_LEN])
{
    TP_BD_ADDR_T tpBdAddr;
    BdaddrConvertTpVmToBluestack(&tpBdAddr, tpaddr);
    CmLeSirkOperationReq(theAppTask,
                         tpBdAddr,
                         flags,
                         sirk_key);
}

void ConnectionDmBleGetAdvScanCapabilitiesReq(Task theAppTask)
{
    CmGetAdvScanCapabilitiesReq(theAppTask);
}

void ConnectionDmBleExtAdvSetsInfoReq(Task theAppTask)
{
    CmExtAdvSetsInfoReq(theAppTask);
}

void ConnectionDmBleExtAdvMultiEnableReq(Task theAppTask,
                                            uint8 enable,
                                            uint8 num_sets,
                                            CL_EA_ENABLE_CONFIG_T config[CL_DM_BLE_EXT_ADV_MAX_NUM_ENABLE])
{
    CmExtAdvMultiEnableReq(theAppTask, enable, num_sets, (CmEnableConfig *) config);
}

void ConnectionDmBlePerAdvSetParamsReq( Task theAppTask,
                                        uint8 adv_handle,
                                        uint32 flags,
                                        uint16 periodic_adv_interval_min,
                                        uint16 periodic_adv_interval_max,
                                        uint16 periodic_adv_properties)
{
    CmPeriodicAdvSetParamsReq(theAppTask,
                              adv_handle,
                              flags,
                              periodic_adv_interval_min,
                              periodic_adv_interval_max,
                              periodic_adv_properties);
}

void ConnectionDmBlePerAdvSetDataReq(Task theAppTask, 
                                     uint8 adv_handle, 
                                     set_data_req_operation operation, 
                                     uint8 adv_data_len, 
                                     uint8 *adv_data[8])
{
    CmPeriodicAdvSetDataReq(theAppTask, 
                            adv_handle,
                            operation,
                            adv_data_len,
                            adv_data);
}

void ConnectionDmBlePerAdvReadMaxAdvDataLenReq(Task theAppTask, uint8 adv_handle)
{
    CmPeriodicAdvReadMaxAdvDataLenRequest(theAppTask, adv_handle);
}

void ConnectionDmBlePerAdvStartReq(Task theAppTask, uint8 adv_handle)
{
    CmPeriodicAdvStartRequest(theAppTask, adv_handle);
}

void ConnectionDmBlePerAdvStopReq(Task theAppTask, uint8 adv_handle, uint8 stop_advertising)
{
    if (stop_advertising <= CL_PERIODIC_ADV_STOP_EA)
    {
        CmPeriodicAdvStopRequest(theAppTask, adv_handle, stop_advertising);
    }
    else
    {
        DEBUG_LOG_ERROR("ConnectionDmBlePerAdvStopReq : stop_advertising value %d is not valid !!",stop_advertising);
    }
}

void ConnectionDmBlePerAdvSetTransferReq(Task theAppTask,
                                         typed_bdaddr taddr,
                                         uint16 service_data,
                                         uint8 adv_handle)
{
    TYPED_BD_ADDR_T tbdAddr;
    tbdAddr.type = taddr.type;
    BdaddrConvertVmToBluestack(&tbdAddr.addr, &taddr.addr);
    CmPeriodicAdvSetTransferRequest(theAppTask,
                                    tbdAddr,
                                    service_data,
                                    adv_handle);
}

#ifndef DISABLE_BLE
/****************************************************************************/
void ConnectionBleTransmitterTest(Task theAppTask, uint8 tx_channel, uint8 data_length, uint8 test_pattern)
{
    DEBUG_LOG("ConnectionBleTransmitterTest");
    CmLeTransmitterTestReqSend(theAppTask, tx_channel, data_length, test_pattern);
}

/****************************************************************************/
void ConnectionBleReceiverTest(Task theAppTask, uint8 rx_channel)
{
    DEBUG_LOG("ConnectionBleReceiverTest");
    CmLeReceiverTestReqSend(theAppTask, rx_channel);
}

/****************************************************************************/
void ConnectionBleTestEnd(Task theAppTask)
{
    DEBUG_LOG("ConnectionBleTestEnd");
    CmLeTestEndReqSend(theAppTask);
}

void ConnectionDmBlePeriodicAdvEnableReq(
                                    Task theAppTask,
                                    uint8 adv_handle,
                                    uint16 flags,
                                    uint8 enable)
{
    CmPeriodicAdvEnableReq(theAppTask,
                           adv_handle,
                           flags,
                           enable);
}

#endif /* DISABLE_BLE */

void ConnectionRfcommDisconnectResponse(Sink sink)
{
    if (SinkIsValid(sink))
    {
        uint16 cid = SinkGetRfcommConnId(sink);
        CmRfcDisconnectRspSend(CM_CREATE_RFC_CONN_ID(cid));
    }
}

void ConnectionL2capDisconnectResponse(uint8 identifier, Sink sink)
{
    if (SinkIsValid(sink))
    {
        uint16 cid = SinkGetL2capCid(sink);
        CmL2caDisconnectRspSend(identifier, CM_CREATE_L2CA_CONN_ID(cid));
    }
}

uint16 ConnectionTrustedDeviceListSize(void)
{
    return CsrBtTdDbCountDevices();
}

