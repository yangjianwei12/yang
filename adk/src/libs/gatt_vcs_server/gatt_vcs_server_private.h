/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_VCS_SERVER_PRIVATE_H
#define GATT_VCS_SERVER_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_vcs_server.h"
#include "gatt_vcs_server_db.h"
#include "gatt_vcs_server_debug.h"

/* Client data. This structure contains data for each connected client */
typedef struct
{
    connection_id_t           cid;
    GattVcsServerConfig  client_cfg;
} gatt_vcs_client_data;

/* Definition of data required for association. */
typedef struct
{
    uint8                 volume_setting;
    uint8                 mute;
    uint8                 change_counter;
    uint8                 step_size;
    uint8                 volume_flag;
    gatt_vcs_client_data  connected_clients[GATT_VCS_MAX_CONNECTIONS];
} gatt_vcs_data;

/* The Volume Control service internal structure for the server role. */
typedef struct __GVCS
{
    TaskData lib_task;
    Task     app_task;

    /* Service handle provided by the service_handle lib when the server
     * memory instance is created
     */
    ServiceHandle srvc_hndl;

    /* Inizalisation parameters. */
    gatt_vcs_data data;
} GVCS;

#define MAKE_VCS_MESSAGE(TYPE) \
    TYPE * message = (TYPE *)PanicNull(calloc(1, sizeof(TYPE)))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_VCS_MESSAGE_WITH_LEN_U8(TYPE, LEN)                           \
    TYPE##_T *message = (TYPE##_T*)PanicUnlessMalloc(sizeof(TYPE##_T) + \
                                                     ((LEN) ? (LEN) - 1 : 0))

#define MAKE_VCS_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_VCS_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)

#endif /* GATT_VCS_SERVER_PRIVATE_H */
