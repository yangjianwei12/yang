#ifndef CSR_BT_GATT_CLIENT_UTIL_PRIM_H__
#define CSR_BT_GATT_CLIENT_UTIL_PRIM_H__
/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_addr.h"
#include "csr_list.h"
#include "csr_log_text_2.h"
#include "service_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsrBtGattClientUtilDeviceElementTag
{
    struct CsrBtGattClientUtilDeviceElementTag    *next; /* must be first */
    struct CsrBtGattClientUtilDeviceElementTag    *prev; /* must be second */
    CsrUint32                               btConnId;
    CsrBtTypedAddr                          addr;
#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
    CsrBool                                 encrypted;
#endif
} CsrBtGattClientUtilDeviceElement;

typedef struct
{
    CsrBtGattId             gattId;
    CsrCmnList_t            addressList;      /* Connected devices list */
    void                    *recvMsg;
#ifdef CSR_LOG_ENABLE
    CsrLogTextHandle *logTextHandle;
#endif
} CsrBtGattClientUtilInst;

/*!
    \brief Response message for GattDiscoverAllCharacteristicsRequest().
*/
typedef struct
{
    /*! Connection identifier of the remote device */
    uint16 cid;
    /*! Handle of the characteristic declaration */
    uint16 declaration;
    /*! Handle of the characteristic value */
    uint16 handle;
    /*! Characteristic properties */
    uint8 properties;
    /*! UUID type of the characteristic */
    att_uuid_type_t uuid_type;
    /*! UUID of the characteristic */
    CsrUint32 uuid[4];
    /*! Flag indicating if more services will follow (TRUE) or not (FALSE) */
    bool more_to_come;
    /*! Status of the request */
    att_result_t status;
} GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T;

/*!
    \brief Response message for
    GattDiscoverAllCharacteristicDescriptorsRequest().
*/
typedef struct
{
    /*! Connection identifier of the remote device */
    uint16 cid;
    /*! Handle of the characteristic descriptor */
    uint16 handle;
    /*! UUID type of the characteristic descriptor */
    att_uuid_type_t uuid_type;
    /*! UUID of the characteristic descriptor */
    CsrUint32 uuid[4];
    /*! Flag indicating if more services will follow (TRUE) or not (FALSE) */
    bool more_to_come;
    /*! Status of the request */
    att_result_t status;
} GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T;

typedef struct ServiceHandleListElement
{
    struct ServiceHandleListElement    *next;
    struct ServiceHandleListElement    *prev;
    CsrBtGattId                        gattId;
    connection_id_t                    cid;   /*! Connection ID */
    ServiceHandle                   service_handle;
} ServiceHandleListElm_t;

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_CLIENT_UTIL_PRIM_H__ */
