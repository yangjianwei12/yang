/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_BASS_SERVER_PRIVATE_H_
#define GATT_BASS_SERVER_PRIVATE_H_

#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "gatt_bass_server.h"

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

#define VALID_ADVERTISE_ADDRESS_TYPE(x) ((x) == TYPED_BDADDR_PUBLIC || (x) == TYPED_BDADDR_RANDOM)

#define VALID_BIG_ENCRYPTION(x) ((x) >= GATT_BASS_SERVER_NOT_ENCRYPTED && (x) < GATT_BASS_BIG_ENCRYPTION_LAST)
#define VALID_PA_SYNC_STATE(x) ((x) >= GATT_BASS_SERVER_NOT_SYNCHRONIZED && (x) < GATT_BASS_SERVER_PA_SYNC_STATE_LAST)
#define VALID_PA_SYNC_STATE_CNTRL_POINT_OP(x) ((x) >= BASS_SERVER_PA_SYNC_NOT_SYNCHRONIZE && (x) < BASS_SERVER_PA_SYNC_LAST_VALUE)

#define BROADCAST_ID_MAX_VALUE (0xFFFFFFu)

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
    connection_id_t cid;
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

/*
    The BASS server internal structure for the server role.
 */
typedef struct __GBASSSS
{
    TaskData lib_task;
    Task app_task;

    ServiceHandle srvc_hndl;

    /* Information to be provided in service characteristics. */
    gatt_bass_server_data_t data;
} GBASSSS;

/* Macros for creating messages */
#define MAKE_BASS_MESSAGE(TYPE) TYPE *message = (TYPE *)PanicNull(calloc(1,sizeof(TYPE)))
#define MAKE_BASS_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *) PanicNull(calloc(1,sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8)))

#endif
