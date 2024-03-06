/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_ASCS_CLIENT_PRIVATE_H
#define GATT_ASCS_CLIENT_PRIVATE_H

#include "csr_bt_tasks.h"
#include "gatt_ascs_client.h"
#include "csr_bt_gatt_client_util_lib.h"


#define CSR_BT_GATT_UUID_SINK_ASE         ((CsrBtUuid16) 0x2BC4)
#define CSR_BT_GATT_UUID_SOURCE_ASE       ((CsrBtUuid16) 0x2BC5)

#define CSR_BT_GATT_UUID_ASCS_CONTROL_POINT      ((CsrBtUuid16) 0x2BC6)

#define CSR_BT_WRITE_CCCD_SIZE                   ((CsrUint16) 0x0002)
#define GATT_NOTIFICATION_ENABLE_VALUE            0x01

#define GATT_WRITE_WITHOUT_RESPONSE            0x00
#define GATT_WRITE                             0x01

/*Service handle initialization failure*/
#define INVALID_ASCS_SERVICE_HANDLE ((ServiceHandle)(0x0000))


/* This structure is private to ascs client.
 * Application can't access this instance at any point of time.
*/
typedef struct GattAscsClient
{
    CsrCmnList_t    clientHandleList;

}GattAscsClient;

/*! @brief GATT Audio Stream Endpoint Service Client[AscsC]Library Instance.
 */
typedef struct GattAscsClient AscsC;


typedef struct AscsServiceHandleListElementTag
{
    struct AscsServiceHandleListElementTag    *next;
    struct AscsServiceHandleListElementTag    *prev;
    ServiceHandle                      clientHandle;
} AscsClientHandleListElmentTag;


typedef struct CsrBtAseCharacElementTag
{
    struct CsrBtAseCharacElementTag    *next; /* must be first */
    struct CsrBtAseCharacElementTag    *prev; /* must be second */
    uint8                                   aseId; /* Needed as several charac can have same UUID */
    CsrBtGattHandle                         declarationHandle; /* Handle for the characteristic declaration*/
    CsrBtGattHandle                         valueHandle; /* Characteristic Value Handle */
    CsrBtGattHandle                         aseCccdHandle;
    CsrBtGattHandle                         endHandle;

} CsrBtAseCharacElement;

typedef struct GAscsC
{
    AppTaskData          lib_task;                          /*! Lib Task */
    AppTask              app_task;                          /*! Application Registered Task */

    /* GattId, cid and service handle is a part of this structure */
    ServiceHandleListElm_t* srvcElem;

    /* Any read/write command pending */
    uint16             pendingCmd;

    /* This is used to store the handle of the characteristic when a descriptor discovery is pending */
    uint16             pendingHandle;

    uint16             startHandle;
    uint16             endHandle;

    CsrCmnList_t       asesSinkCharacList;     /* SINK ASE Char */
    CsrCmnList_t       asesSourceCharacList;  /* SOURCE ASE Char */
    uint8              aseSinkCharacCount;           /* Sink ASE Char count */
    uint8              aseSourceCharacCount;     /* SOURCE ASE Char count */

    CsrBtGattHandle    asesAseControlPointHandle;  /*! ASCS Ase control point Handle: Used by lib, unused by app*/
    CsrBtGattHandle    asesAseControlPointCcdHandle;  /*! ASE control point CC Handle */

    uint8              readCount;
    bool               readSinkChar;
    bool               readAseInfo;
    uint8              writeCount;
    uint8              writeType;   /* GATT write operation type on ASE control point */

}GAscsC;


CsrBool ascsFindCharacFromAseId(CsrCmnListElm_t *elem, void *value);
CsrBool ascsFindCharacFromHandle(CsrCmnListElm_t *elem, void *value);
CsrBool ascsFindCharacFromCCCDHandle(CsrCmnListElm_t *elem, void *value);


#define ASE_ADD_PRIM_CHARAC(_List) \
    (CsrBtAseCharacElement *)CsrCmnListElementAddLast(&(_List), \
                                    sizeof(CsrBtAseCharacElement))

#define ASCS_FIND_PRIM_CHARAC_BY_ASEID(_List,_id) \
    ((CsrBtAseCharacElement *)CsrCmnListSearch(&(_List), \
                                    ascsFindCharacFromAseId, \
                                    (void *)(_id)))

#define ASCS_FIND_PRIM_CHARAC_BY_HANDLE(_List,_id) \
    ((CsrBtAseCharacElement *)CsrCmnListSearch(&(_List), \
                                    ascsFindCharacFromHandle, \
                                    (void *)(_id)))

#define ASCS_FIND_PRIM_CHARAC_BY_CCD_HANDLE(_List,_id) \
    ((CsrBtAseCharacElement *)CsrCmnListSearch(&(_List), \
                                    ascsFindCharacFromCCCDHandle, \
                                    (void *)(_id)))

/* Defines For LIB internal messages */

typedef uint16 AscsClientInternalMsg;

#define ASCS_CLIENT_INTERNAL_MSG_BASE                           (AscsClientInternalMsg)0x000A
#define ASCS_CLIENT_INTERNAL_MSG_DISCOVER                       (AscsClientInternalMsg)(ASCS_CLIENT_INTERNAL_MSG_BASE + 0x0001u) /*! For a discover Request */
#define ASCS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ               (AscsClientInternalMsg)(ASCS_CLIENT_INTERNAL_MSG_BASE + 0x0002u) /*! For enable/disable Notification */
#define ASCS_CLIENT_INTERNAL_MSG_READ_ASE_INFO_REQ              (AscsClientInternalMsg)(ASCS_CLIENT_INTERNAL_MSG_BASE + 0x0003u) /*! For Reading ASES and Content Availability */
#define ASCS_CLIENT_INTERNAL_MSG_READ_ASE_STATE_REQ             (AscsClientInternalMsg)(ASCS_CLIENT_INTERNAL_MSG_BASE + 0x0004u) /*! For Reading ASES State */
#define ASCS_CLIENT_INTERNAL_MSG_WRITE_ASES_CONTROL_POINT_REQ   (AscsClientInternalMsg)(ASCS_CLIENT_INTERNAL_MSG_BASE + 0x0005u) /*! For Writing ASES Control Point */

typedef AscsClientInternalMsg GattAscsPrim;

/* Internal message structure for discover request  */
typedef struct
{
    GattAscsPrim          prim;         /* Prim type */
    ServiceHandle         clientHandle; /* service handle for client instance */
    CsrBtGattHandle       startHandle;  /* Start handle of the service */
    CsrBtGattHandle       endHandle;    /* End handle of the service */
} AscsClientInternalMsgDiscover;

/* Internal message structure for Read request  */
typedef struct
{
    GattAscsPrim    prim;         /* Prim type */
    ServiceHandle   clientHandle; /* service handle for client instance */
} AscsClientInternalMsgReadAseInfoReq;

typedef struct
{
    GattAscsPrim    prim;         /* Prim type */
    ServiceHandle   clientHandle; /* service handle for client instance */
    CsrBtAseId      aseId;        /* ASE Id */
} AscsClientInternalMsgReadAseStateReq;

/* Internal message structure for Read request  */
typedef struct
{
    GattAscsPrim    prim;         /* Prim type */
    ServiceHandle   clientHandle; /* service handle for client instance */
    CsrUint16       sizeValue;    /* size of the value */
    CsrUint8        value[1];     /* value */
} AscsClientInternalMsgWriteAsesControlPointReq;

/* Internal Message Structure to Initiate registering notification */
typedef struct
{
    GattAscsPrim    prim;         /* Prim type */
    ServiceHandle   clientHandle; /* service handle for client instance */
    CsrBtAseId      aseId;        /* ASE Id */
    bool            enable;       /* Enable/Disable indication */
}AscsClientInternalMsgNotificationReq;

#endif /* GATT_ASCS_CLIENT_PRIVATE_H */

