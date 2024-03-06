#ifndef CSR_BT_GATT_CLIENT_UTIL_H__
#define CSR_BT_GATT_CLIENT_UTIL_H__
/******************************************************************************
 Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_bt_gatt_client_util_prim.h"

CsrBool CsrBtGattClientUtilFindConnectedBtConnId(CsrCmnListElm_t *elem, void *value);
CsrBool CsrBtGattClientUtilFindConnectedBtAddr(CsrCmnListElm_t *elem, void *value);

#define CSR_BT_GATT_CLIENT_UTIL_ADD_PRIM_DEVICE(_List) \
    (CsrBtGattClientUtilDeviceElement *)CsrCmnListElementAddLast(&(_List), \
                                                                 sizeof(CsrBtGattClientUtilDeviceElement))

#define CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTCONNID(_List,_btConnId) \
    ((CsrBtGattClientUtilDeviceElement *)CsrCmnListSearch(&(_List), \
                                                          CsrBtGattClientUtilFindConnectedBtConnId, \
                                                        (void *)(_btConnId)))

#define CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTADDR(_List,_btAddr) \
    ((CsrBtGattClientUtilDeviceElement *)CsrCmnListSearch(&(_List), \
                                                          CsrBtGattClientUtilFindConnectedBtAddr, \
                                                        (void *)(_btAddr)))

#define CSR_BT_GATT_CLIENT_UTIL_REMOVE_PRIM_DEVICE(_List,_Elem) \
    (CsrCmnListElementRemove((CsrCmnList_t *)&(_List), \
                             (CsrCmnListElm_t *)(_Elem)))

#endif
