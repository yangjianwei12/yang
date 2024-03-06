/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_VCS_SERVER_PRIVATE_H
#define GATT_VCS_SERVER_PRIVATE_H

#include "gatt_vcs_server.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include <marshal.h>
#endif

/* default client config 0xFFFF means remote client has not written
 * any CCCD which is as good as CCCD disable */
#define GATT_VCS_SERVER_INVALID_CLIENT_CONFIG   0xFFFF

/* Client data. This structure contains data for each connected client */
typedef struct
{
    connection_id_t      cid;
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

#ifdef CSR_TARGET_PRODUCT_VM
/* Definition of data required for handover. */

typedef struct
{
    uint8                 volume_setting;
    uint8                 mute;
    uint8                 change_counter;
    uint8                 step_size;
    uint8                 volume_flag;
    gatt_vcs_client_data  connectedClient;
} gattVcsFirstHandoverData;

typedef struct
{
    bool           marshallerInitialised;
    marshaller_t   marshaller;
    bool           unMarshallerInitialised;
    unmarshaller_t unMarshaller;
    gattVcsFirstHandoverData *handoverFirstConnectionInfo;
    gatt_vcs_client_data *handoverSubsequentConnectionInfo[GATT_VCS_MAX_CONNECTIONS-1];
} VcsHandoverMgr;
#endif

/* The Volume Control service internal structure for the server role. */
typedef struct __GVCS
{
    AppTaskData lib_task;
    AppTask     app_task;

    /* Service handle provided by the service_handle lib when the server
     * memory instance is created
     */
    ServiceHandle srvc_hndl;
    uint16 start_handle;
    uint16 end_handle;

    CsrBtGattId gattId;

    /* Inizalisation parameters. */
    gatt_vcs_data data;

#ifdef CSR_TARGET_PRODUCT_VM
    uint8 handoverStep;

    /*Handvover data */
    VcsHandoverMgr* vcsHandoverMgr;
#endif
} GVCS;

#define MAKE_VCS_MESSAGE(TYPE) \
    TYPE * message = (TYPE *)CsrPmemZalloc(sizeof(TYPE))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_VCS_MESSAGE_WITH_LEN_U8(TYPE, LEN) MAKE_VCS_MESSAGE(TYPE)

#define MAKE_VCS_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_VCS_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        CsrPmemCpy(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)

#define VcsServerMessageSend(_appTask, _msg) \
                      CsrSchedMessagePut(_appTask, VCS_SERVER_PRIM, ((void *)_msg))


#endif /* GATT_VCS_SERVER_PRIVATE_H */
