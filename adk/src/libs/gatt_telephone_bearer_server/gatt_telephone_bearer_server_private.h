/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H
#define GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_telephone_bearer_server.h"
#include "gatt_telephone_bearer_server_db.h"
#include "gatt_telephone_bearer_server_debug.h"

#define MAKE_TBS_MESSAGE(TYPE) \
    TYPE##_T* message = (TYPE##_T *)PanicNull(calloc(1, sizeof(TYPE##_T)))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_TBS_MESSAGE_WITH_LEN_U8(TYPE, LEN)                           \
    TYPE##_T *message = (TYPE##_T*)PanicUnlessMalloc(sizeof(TYPE##_T) + \
                                                     ((LEN) ? (LEN) - 1 : 0))

#define MAKE_TBS_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_TBS_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)
        
        
#define FREE_CALL_SLOT (0xFF) /* identifies a free slot in the call list */

/* TBS Library private messages */
#define TBS_MSG_BASE     (0x0)

/* TBS Internal Messages */
typedef enum
{
    GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER = TBS_MSG_BASE
} GATT_TBS_INTERNAL_T;

typedef struct
{
    uint8   signalStrength;
} GATT_TBS_INTERNAL_SIGNAL_STRENGTH_TIMER_T;


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
    tbsCurrentCallListChracteristic currentCallsList[TBS_CURRENT_CALLS_LIST_SIZE];
    uint8                                 contentControlId;
    uint16                                statusFlags;
    uint16                                callControlPointOpcodes;
    tbsIncomingCallTargetUriChracteristic incomingTargetBearerUri;
    tbsCallFriendlyNameChracteristic callFriendlyName;
    tbsIncomingCallChracteristic     incomingCall;

    uint8                                 nextCallId;
    unsigned                              signal_strength_timer_flag:1;
    unsigned                              unused:7;

    uint16                                num_clients;
    TbsClientData                     connected_clients[TBS_MAX_CONNECTIONS];
} tbs_data;


/*! @brief The Telephone Bearer service internal structure for the server role.

    This structure is visible to the application as it is responsible for
    managing resource to pass to the Telephone Bearer library.
    The elements of this structure are only modified by the Telephone Bearer Server library.
    The application SHOULD NOT modify the values at any time as it could lead
    to undefined behaviour.

 */
typedef struct GTBS_T
{
    TaskData lib_task;
    Task     appTask;

    /* Service handle of the instance */
    ServiceHandle srvc_handle;

    /*! Information to be provided in service characteristics. */
    tbs_data data;
} GTBS_T;


/**************************************************************************
NAME
    tbsFindCid

DESCRIPTION
    Find the index of a specific Cid.
*/
bool tbsFindCid(const GTBS_T *telephone_bearer_server, uint16 cid, uint8 *index);

#endif /* GATT_TELEPHONE_BEARER_SERVER_PRIVATE_H */
