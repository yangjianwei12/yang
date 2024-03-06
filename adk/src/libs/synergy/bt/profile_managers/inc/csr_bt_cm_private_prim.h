#ifndef CSR_BT_CM_PRIVATE_PRIM_H__
#define CSR_BT_CM_PRIVATE_PRIM_H__
/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_td_db_sc.h"
#include "csr_bt_profiles.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Subscription masks */
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ADDR_MAPPED_IND          ((CsrBtCmEventMask) 0x00040000)
#define CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND       ((CsrBtCmEventMask) 0x00080000)

#define CSR_BT_CM_AUTHORISE_FOREVER                 0xFFFF

/* Database entries. Internally used values - may change at any time! */
#define CSR_BT_CM_KEY_TYPE_BREDR                    (0)    /* see CSR_BT_TD_DB_SC_KEY_BREDR_KEY in csr_bt_td_db_sc.h */
#define CSR_BT_CM_KEY_TYPE_LE                       (1)    /* see CSR_BT_TD_DB_SC_KEY_LE_KEYS in csr_bt_td_db_sc.h  */
#define CSR_BT_CM_KEY_TYPE_ALL                      ((1 << CSR_BT_CM_KEY_TYPE_BREDR) | (1 << CSR_BT_CM_KEY_TYPE_LE))

/* Definitions for internal functions */
#define CSR_BT_CM_DB_OP_READ                        ((CsrUint8) 0x00)
#define CSR_BT_CM_DB_OP_WRITE                       ((CsrUint8) 0x01)

#define CSR_BT_RESULT_CODE_CM_PUBLIC_MAX            0x001A
#define CSR_BT_RESULT_CODE_CM_UNKNOWN_DEVICE        ((CsrBtResultCode) CSR_BT_RESULT_CODE_CM_PUBLIC_MAX + 1)

/* search_string="CsrBtCmPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */
#define CSR_BT_CM_PRIVATE_PRIM_DOWNSTREAM_LOWEST    ((CSR_BT_CM_PRIM_DOWNSTREAM_HIGHEST & 0xFF00) + 0x0100)

#define CSR_BT_CM_PRIVATE_PRIM_UPSTREAM_LOWEST      ((CSR_BT_CM_PRIM_UPSTREAM_HIGHEST & 0xFF00) + 0x0100)

/* Upstream */
#define CSR_BT_CM_LE_ADDRESS_MAPPED_IND             ((CsrBtCmPrim) (0x0001 + CSR_BT_CM_PRIVATE_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_SECURITY_EVENT_IND                ((CsrBtCmPrim) (0x0002 + CSR_BT_CM_PRIVATE_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_CM_PRIVATE_PRIM_UPSTREAM_HIGHEST                    (0x0002 + CSR_BT_CM_PRIVATE_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_CM_PRIVATE_PRIM                      CSR_BT_CM_PRIM

/* ---------- End of Primitives ----------*/
typedef struct
{
    CsrBtCmPrim             type;                       /* Primitive/message identity */
    CsrBtDeviceAddr         randomAddr;                 /* Random Address of device which has to be replaced by ID address */
    CsrBtTypedAddr          idAddr;                     /* ID Address of the peer device */
} CsrBtCmLeAddressMappedInd;

typedef CsrUint8 CsrBtCmSecurityEvent;
#define CSR_BT_CM_SECURITY_EVENT_BOND                   ((CsrBtCmSecurityEvent) 0)
#define CSR_BT_CM_SECURITY_EVENT_DEBOND                 ((CsrBtCmSecurityEvent) 1)
#define CSR_BT_CM_SECURITY_EVENT_CTKD                   ((CsrBtCmSecurityEvent) 2)
#define CSR_BT_CM_SECURITY_EVENT_DEVICE_UPDATE_FAILED   ((CsrBtCmSecurityEvent) 3)

typedef struct
{
    CsrBtCmPrim             type;                       /* Primitive/message identity */
    CsrBtTransportMask      transportMask;              /* Transports */
    CsrBtTypedAddr          addrt;                      /* Peer device */
    CsrBtCmSecurityEvent    event;
} CsrBtCmSecurityEventInd;

typedef CsrUint8 CsrBtCmKeyType;

typedef union
{
    CsrBtTdDbBredrKey bredrKey;
    CsrBtTdDbLeKeys leKeys;
} CsrBtCmKey;

typedef struct
{
    CsrBtCmPrim             type;
    CsrSchedQid             appHandle;
    CsrBtAddressType        addressType;
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                opcode;
    CsrBtCmKeyType          keyType;
    CsrBtCmKey             *key;
} CsrBtCmDatabaseReq;

typedef struct
{
    CsrBtCmPrim             type;
    CsrBtAddressType        addressType;                /* LE address type */
    CsrBtDeviceAddr         deviceAddr;
    CsrUint8                opcode;
    CsrBtCmKeyType          keyType;
    CsrBtCmKey             *key;
    CsrBtResultCode         resultCode;
    CsrBtSupplier           resultSupplier;
} CsrBtCmDatabaseCfm;

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_CM_PRIVATE_PRIM_H__ */

