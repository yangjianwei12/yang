#ifndef GATT_DATA_LOGGER_PRIM_H__
#define GATT_DATA_LOGGER_PRIM_H__

/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "csr_bt_uuids.h"
#include "csr_bt_gatt_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef CsrUint8        GattOpType;
#define GATT_READ_REQUEST                             ((GattOpType)0x00)
#define GATT_WRITE_REQUEST                            ((GattOpType)0x01)
#define GATT_READ_RESPONSE                            ((GattOpType)0x02)
#define GATT_WRITE_RESPONSE                           ((GattOpType)0x03)
#define GATT_NOTIFICATION                             ((GattOpType)0x04)
#define GATT_WRITE_RESPONSE_MORE                      ((GattOpType)0x05)
#define GATT_DISCOVER_SERVICES                        ((GattOpType)0x06)
#define GATT_ATT_ERROR_RSP                            ((GattOpType)0x07)
#define GATT_WRITE_REQUEST_MORE                       ((GattOpType)0x08)


/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/

/* Upstream */
#define  GATT_UUID_DATA_IND     GATT_RESERVED1_IND

/* Subscribe using CsrBtGattSetEventMaskReqSend() to obtain the Gatt Data logger GATT_UUID_DATA_IND. */
#define GATT_EVENT_MASK_SUBSCRIBE_DATA_LOGGER    GATT_EVENT_MASK_SUBSCRIBE_RESERVED1  


/*******************************************************************************
 * Primitive signal type definitions - APPLICATION INTERFACE
 *******************************************************************************/

typedef struct
{
    CsrBtGattPrim          type;                /* Identity */
    CsrBtConnId            btConnId;            /* Connection identifier */
    GattOpType             operationType;       /* See OpType definition above */
    bool                   locallyOriginated;   /* If operation is local operations, TRUE for any local Client read, FALSE for remote operations */
    CsrBtUuid              uuid;                /* The Service UUID */
    uint8                  dataLength;          /* Length of data associated with uuid */
    uint8                  *data;               /*  Data corresponds to the UUID for GattOperationType
                                                    i.e read rsp, write rsp, notify, NULL for Read Req*/
} GattUuidDataInd;


#ifdef __cplusplus
}
#endif

#endif /* GATT_DATA_LOGGER_PRIM_H__ */
