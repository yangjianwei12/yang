/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H
#define GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H


#include "gatt_telephone_bearer_server_db.h"
#include "gatt_telephone_bearer_server.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_pmem.h"

#define MAKE_TBS_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))
#define MAKE_TBS_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *)CsrPmemAlloc(sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8))
#define MAKE_TBS_MESSAGE_WITH_LEN_U8(TYPE, LEN)  \
    TYPE *message = (TYPE*)CsrPmemAlloc(sizeof(TYPE) + \
                                       ((LEN) ? (LEN) - 1 : 0))

#define FREE_CALL_SLOT (0xFF) /* identifies a free slot in the call list */

/* TBS Library private messages */
#define TBS_MSG_BASE     (0x0)

/* TBS Internal Messages */
typedef uint16 GattTelephoneBearerServerInternal;
#define    GATT_TELEPHONE_BEARER_SERVER_INTERNAL_SIGNAL_STRENGTH_TIMER (0x0000u)

typedef struct
{
    GattTelephoneBearerServerInternal id;
    uint8   signalStrength;
} GattTelephoneBearerServerInternalSignalStrengthTimer;


/*! @brief Definition of data required for association.
 */
typedef struct
{
    char*                                 providerName;
    uint16                                providerNameLen;
    char*                                 uci;
    uint16                                uciLen;
    uint8                                 technology;
    char*                                 uriPrefixesList;
    uint16                                uriPrefixesLen;
    uint8                                 signalStrength;
    uint8                                 signalStrengthNotified; /* the last value actually notified */
    uint8                                 signalStrengthReportingInterval;
    uint8                                 contentControlId;
    uint8                                 nextCallId;
    unsigned                              signalStrengthTimerFlag:1;
    unsigned                              unused:7;
    uint16                                statusFlags;
    uint16                                callControlPointOpcodes;
    TbsCurrentCallListChracteristic       currentCallsList[TBS_CURRENT_CALLS_LIST_SIZE];
    TbsIncomingCallTargetUriChracteristic incomingTargetBearerUri;
    TbsCallFriendlyNameChracteristic      callFriendlyName;
    TbsIncomingCallChracteristic          incomingCall;
    TbsClientData                         connectedClients[TBS_MAX_CONNECTIONS];
} TbsData;


/*! @brief The Telephone Bearer service internal structure for the server role.

    This structure is visible to the application as it is responsible for
    managing resource to pass to the Telephone Bearer library.
    The elements of this structure are only modified by the Telephone Bearer Server library.
    The application SHOULD NOT modify the values at any time as it could lead
    to undefined behaviour.

 */
typedef struct GTBS_T
{
    AppTaskData libTask;
    AppTask     appTask;

    /* Service handle of the instance */
    ServiceHandle srvcHandle;

    /*! Information to be provided in service characteristics. */
    TbsData data;
    /* Gatt id of the TBS server instance*/
    CsrBtGattId gattId;
} GTBS_T;


/**************************************************************************
NAME
    tbsFindCid

DESCRIPTION
    Find the index of a specific Cid.
*/
bool tbsFindCid(const GTBS_T *telephoneBearerServer, connection_id_t cid, uint8 *index);

#define TbsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TBS_SERVER_PRIM, MSG);\
    }
#endif /* GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H */
