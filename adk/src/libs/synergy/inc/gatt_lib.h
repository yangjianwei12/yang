/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 DESCRIPTION
     Header file providing mapping for Synergy GATT profile's public interfaces.
     Refer to csr_bt_gatt_lib.h for APIs descriptions.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef COMMON_SYNERGY_INC_GATT_LIB_H_
#define COMMON_SYNERGY_INC_GATT_LIB_H_

#include "synergy.h"
#include "csr_bt_gatt_lib.h"

#define GATT_PRIM             (SYNERGY_EVENT_BASE + CSR_BT_GATT_PRIM)

#define GattRegisterReqSend(_task, _context)                                                   \
        CsrBtGattRegisterReqSend(TrapToOxygenTask(_task), _context)

#define GattFreeUpstreamMessageContents(_message)                                              \
        CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, _message);

#define GattUnregisterReqSend(_gattId) \
        CsrBtGattUnregisterReqSend(_gattId)

#define GattDbAllocReqSend(_gattId,_numOfAttrHandles,_preferredStartHandle)                    \
        CsrBtGattDbAllocReqSend(_gattId,_numOfAttrHandles,_preferredStartHandle)

#define GattDbWriteAccessResSend(_gattId, _btConnId, _handle, _rspCode)                        \
        CsrBtGattDbWriteAccessResSend(_gattId, _btConnId, _handle, _rspCode)

#define GattDbReadAccessResSend(_gattId,_btConnId,_handle,_rspCode,_valueLength,_value)        \
        CsrBtGattDbReadAccessResSend(_gattId,_btConnId,_handle,_rspCode,_valueLength,_value)

#define GattIndicationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value)          \
        CsrBtGattIndicationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value)

#define GattNotificationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value)        \
        CsrBtGattNotificationEventReqSend(_gattId,_btConnId,_attrHandle,_valueLength,_value)

#define GattDiscoverPrimaryServicesBy128BitUuidReqSend(_gattId,_btConnId,_uuid128)             \
        CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend(_gattId,_btConnId,_uuid128)

#define GattDiscoverPrimaryServicesBy16BitUuidReqSend(_gattId,_btConnId,_uuid16)               \
        CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(_gattId,_btConnId,_uuid16) 

#define GattDiscoverAllCharacOfAServiceReqSend(_gattId,_btConnId,_startHandle,_endHandle)      \
        CsrBtGattDiscoverAllCharacOfAServiceReqSend(_gattId,_btConnId,_startHandle,_endHandle)

#define GattDiscoverAllCharacDescriptorsReqSend(_gattId,_btConnId,_startHandle,_endHandle)     \
        CsrBtGattDiscoverAllCharacDescriptorsReqSend(_gattId,_btConnId,_startHandle,_endHandle)

#define GattWriteReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value)                \
        CsrBtGattWriteReqSend(_gattId,_btConnId,_handle,_offset,_valueLength,_value)

#define GattReadReqSend(_gattId,_btConnId,_handle,_offset)                                     \
        CsrBtGattReadReqSend(_gattId,_btConnId,_handle,_offset)

#define GattCancelReqSend(_gattId, _btConnId)                                                  \
        CsrBtGattCancelReqSend(_gattId, _btConnId)

#define GattSubscribeForEventMask(_gattId,_eventMask)                                          \
        CsrBtGattSetEventMaskReqSend(_gattId,_eventMask) 

#define GattClientIndicationRspSend(_gattId,_btConnId)                                         \
        CsrBtGattClientIndicationRspSend(_gattId,_btConnId)

#define GattClientRegisterServiceReqSend(_gattId,_start,_end,_address)                         \
        CsrBtGattClientRegisterServiceReqSend(_gattId,_start,_end,_address)

#define GattFlatDbRegisterReqSend(_task,_flags,_size_db,_db)                                   \
        CsrBtGattFlatDbRegisterReqSend(TrapToOxygenTask(_task),_flags,_size_db,_db)

#define GattFlatDbRegisterHandleRangeReqSend(_gattId, _start, _end)                            \
        CsrBtGattFlatDbRegisterHandleRangeReqSend(_gattId, _start, _end)

#define GattClientExchangeMtuReqSend(_gattId, _btConnId, _mtu)                                 \
        CsrBtGattClientExchangeMtuReqSend(_gattId, _btConnId, _mtu)

#define GattRemoteClientExchangeMtuResSend(_gattId, _btConnId, _mtu)                           \
        CsrBtGattRemoteClientExchangeMtuResSend(_gattId, _btConnId, _mtu)

#define GattDiscoverPrimaryServicesByUuidReqSend(_gattId,_btConnId,_uuidType,_uuid)            \
        CsrBtGattDiscoverPrimaryServicesByUuidReqSend(_gattId,_btConnId,_uuidType,_uuid)

#define GattGetMtuFromConnInst(_btConnId)                                                      \
        CsrBtGattGetMtuFromConnInst(_btConnId)

#define GattClientUtilFindAddrByConnId(_btConnId,_addr)                                        \
        CsrBtGattClientUtilFindAddrByConnIdEx(_btConnId,_addr)

#define GattClientUtilGetCidByConnId(_btConnId)                                                \
        CsrBtGattClientUtilGetCidByConnId(_btConnId)

#define GattConnectBredrReqSend(_gattId,_address)                                              \
        CsrBtGattConnectBredrReqSend(_gattId,_address)

#define GattAcceptBredrReqSend(_gattId)                                                        \
        CsrBtGattAcceptBredrReqSend(_gattId)

#define GattCancelAcceptBredrReqSend(_gattId)                                                  \
        CsrBtGattCancelAcceptBredrReqSend(_gattId)

#define GattConnectBredrResSend(_gattId,_address,_rspCode)                                     \
        CsrBtGattConnectBredrResSend(_gattId,_address,_rspCode)

#define GattDisconnectBredrReqSend(_gattId,_address)                                           \
        CsrBtGattDisconnectBredrReqSend(_gattId,_address)

#define GattSetEattMtuReqSend(_desiredEattMtu)                                                  \
        GattSetEattMtu(_desiredEattMtu)

#endif /* COMMON_SYNERGY_INC_GATT_LIB_H_ */
