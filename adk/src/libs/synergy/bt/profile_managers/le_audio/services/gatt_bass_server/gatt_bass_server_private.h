/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_BASS_SERVER_PRIVATE_H_
#define GATT_BASS_SERVER_PRIVATE_H_


#include <stdlib.h>

#include "csr_pmem.h"
#include "gatt_bass_server.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include <marshal.h>
#endif

/* Number of supported connected clients */
#define BASS_SERVER_MAX_CONNECTIONS    (3)

/* Maximum value of Advertising_SID */
#define BASS_SERVER_ADVERTISING_SID_MAX  (0x0Fu)

/* Possible values for the PA_Sync parameter in case of Add Source Operation */
#define BASS_SERVER_PA_SYNC_NOT_SYNCHRONIZE             (0x00u)
#define BASS_SERVER_PA_SYNC_SYNCHRONIZE_PAST            (0x01u)
#define BASS_SERVER_PA_SYNC_SYNCHRONIZE_NO_PAST         (0x02u)
#define BASS_SERVER_PA_SYNC_LAST_VALUE                  (0x03u)

#define VALID_PA_SYNC(x) ((x) >= BASS_SERVER_PA_SYNC_NOT_SYNCHRONIZE && (x)<BASS_SERVER_PA_SYNC_LAST_VALUE)

#define BASS_SERVER_FAILED_SYNC_BIG  (0xFFFFFFFFu)

#define VALID_ADVERTISE_ADDRESS_TYPE(x) ((x) == TBDADDR_PUBLIC || (x) == TBDADDR_RANDOM)

#define VALID_BIG_ENCRYPTION(x) ((x) >= GATT_BASS_SERVER_NOT_ENCRYPTED && (x) < GATT_BASS_BIG_ENCRYPTION_LAST)
#define VALID_PA_SYNC_STATE(x) ((x) >= GATT_BASS_SERVER_NOT_SYNCHRONIZED && (x) < GATT_BASS_SERVER_PA_SYNC_STATE_LAST)
#define VALID_PA_SYNC_STATE_CNTRL_POINT_OP(x) ((x) >= BASS_SERVER_PA_SYNC_NOT_SYNCHRONIZE && (x) < BASS_SERVER_PA_SYNC_LAST_VALUE)

#define BROADCAST_ID_MAX_VALUE (0xFFFFFFu)

/* default client config 0xFFFF means remote client has not written
 * any CCCD which is as good as CCCD disable */
#define GATT_BASS_SERVER_INVALID_CLIENT_CONFIG   0xFFFF

/*
    Definition of data about a broadcast source.
 */
typedef struct
{
    uint8 source_id;
    GattBassServerReceiveState broadcast_source_state;
    uint8 broadcast_code[GATT_BASS_SERVER_BROADCAST_CODE_SIZE];
} gatt_bass_broadcast_source_info_t;

/*
    Client data.

    This structure contains data for each connected client
 */
typedef struct
{
    connection_id_t            cid;
    GattBassServerConfig  client_cfg;
} gatt_bass_server_ccc_data_t;


/*
    Definition of data required for association.
 */
typedef struct
{
    gatt_bass_broadcast_source_info_t **broadcast_source;
    gatt_bass_server_ccc_data_t connected_clients[BASS_SERVER_MAX_CONNECTIONS];
    uint8 broadcast_receive_state_num;
} gatt_bass_server_data_t;

#ifdef CSR_TARGET_PRODUCT_VM

/* We need to have this define, because we can't marshal a dynamic array of pointers.
 * This define MUST reflect the number of Broadcast Receive State characteristics
 * we have in the BASS .dbi file.
 * If this number changes, this define should change accordingly */

#define BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM (2)

typedef struct
{
    uint8 source_id;
    uint8 broadcast_code[GATT_BASS_SERVER_BROADCAST_CODE_SIZE];
    GattBassServerPaSyncState paSyncState;
    GattBassServerBroadcastBigEncryption bigEncryption;
    CsrBtTypedAddr sourceAddress;
    uint32 broadcastId;
    uint8 sourceAdvSid;
    uint8 numSubGroups;
    uint8 badCode[GATT_BASS_SERVER_BROADCAST_CODE_SIZE];
    GattBassServerSubGroupsData *subGroupsData;
} gattBassBroadcastHandoverSourceInfo;

/*
    Definition of data required for handover.
 */
typedef struct
{
    gatt_bass_server_ccc_data_t connectedClient;
    gattBassBroadcastHandoverSourceInfo *broadcast_source[BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM];
} gattBassServerFirstHandoverData;

typedef struct
{
    bool           marshallerInitialised;
    marshaller_t   marshaller;
    bool           unMarshallerInitialised;
    unmarshaller_t unMarshaller;
    gattBassServerFirstHandoverData *handoverFirstConnectionInfo;
    gatt_bass_server_ccc_data_t *handoverSubsequentConnectionInfo[BASS_SERVER_MAX_CONNECTIONS-1];
} BassHandoverMgr;
#endif

/*
    The BASS server internal structure for the server role.
 */
typedef struct __GBASSSS
{
    AppTaskData lib_task;
    AppTask app_task;
    CsrBtGattId gattId;

    ServiceHandle srvc_hndl;
    uint16 start_handle;
    uint16 end_handle;

    /* Information to be provided in service characteristics. */
    gatt_bass_server_data_t data;

#ifdef CSR_TARGET_PRODUCT_VM
    uint8 handoverStep;
    bool isToBeNotified[BASS_SERVER_BROADCAST_RECEIVE_STATE_NUM];

    /*Handvover data */
    BassHandoverMgr* bassHandoverMgr;
#endif
} GBASSSS;

/* Macros for creating messages */
#define MAKE_BASS_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))
#define MAKE_BASS_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *)CsrPmemAlloc(sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8))

#define BassMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, BASS_SERVER_PRIM, MSG);\
    }

#endif
