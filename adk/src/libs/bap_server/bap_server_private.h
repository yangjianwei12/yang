/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#ifndef BAP_PRIVATE_H
#define BAP_PRIVATE_H

#include <stdlib.h>

#include <library.h>
#include <message.h>
#include <panic.h>
#include "bap_server.h"
#include "service_handle.h"

/*! QOS parameter settings used to indicate to the handset 
    values the BT controller supports/prefers */
#define ASE_RETRANSMISSION_NUMBER   0x02
#define ASE_PHY_PREFERENCE          1
#define ASE_PREFERRED_FRAMING       0
#define ASE_MAXIMUM_SDU_SIZE        0xfff
#define ASE_TRANSPORT_LATENCY_MAX   0xfa0
#define ASE_SDU_INTERVAL_MIN        0xff
#define ASE_SDU_INTERVAL_MAX        0xfffff
#define ASE_PRESENTATION_DELAY_MIN  10000
#define ASE_PRESENTATION_DELAY_MAX  40000


#define BAP_SERVER_MAX_NUM_ASES    3

/*! Minimum GATT MTU needed to support LE unicast. */
#define bapServerUnicastConfigGattMtuMinimum() 100

/* Maximum number of GATT connections */
#define BAP_SERVER_MAX_CONNECTIONS (3)


/* The Volume Control Profile internal structure. */
typedef struct __BAP
{
    TaskData libTask;
    Task appUnicastTask;
    Task appBroadcastTask;
    Task appSinkTask;
    Task pendingTask;

    /*! ID of the connection */
    ConnId cid[BAP_SERVER_MAX_CONNECTIONS];
    uint8 numConfigInstance[BAP_SERVER_MAX_CONNECTIONS];

    /*! Profile handle of the BAP instance*/
    bapProfileHandle profileHandle;

    /*! services handles for ASCS PACS and BASS */
    ServiceHandle ascsHandle;
    ServiceHandle pacsHandle;
    ServiceHandle bassHandle;
} BAP;

#define BapServerMessageSend(_appTask, _msg) \
                      MessageSend(_appTask, message->type, ((void*)_msg))

#define MAKE_BAP_MESSAGE(TYPE) \
    TYPE##_T* message = (TYPE##_T *)PanicNull(calloc(1, sizeof(TYPE##_T)))

#define MAKE_BAP_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8)))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_BAP_MESSAGE_WITH_LEN_U8(TYPE, LEN)                           \
    TYPE##_T *message = (TYPE##_T*)PanicUnlessMalloc(sizeof(TYPE##_T) + \
                                                     ((LEN) ? (LEN) - 1 : 0))

#define MAKE_BAP_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_BAP_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)

#define FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(_Handle) \
                              (BAP *)ServiceHandleGetInstanceData(_Handle)

/* Allocation for ASCS Server Request and Response messages */
#define MAKE_ASCS_SERVER_REQUEST_ASE_ID(TYPE, LEN) TYPE *request = (TYPE *) PanicNull(calloc(1, sizeof(TYPE))); \
                request->aseId = (uint8 *)PanicNull(calloc(1, sizeof(uint8) * LEN));
#define MAKE_ASCS_SERVER_RESPONSE_ASE(TYPE, LEN) TYPE *response = (TYPE *) PanicNull(calloc(1, sizeof(TYPE))); \
                response->ase = (TYPE##Ase *)PanicNull(calloc(1, sizeof(TYPE##Ase) * LEN));
#define MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(TYPE, LEN) TYPE *response = (TYPE *) PanicNull(calloc(1, sizeof(TYPE))); \
                response->gattAscsAseResult = (GattAscsAseResult *)PanicNull(calloc(1, sizeof(GattAscsAseResult) * LEN));

/* Deallocatioin for ASCS Server Request and Response messages */
#define FREE_REQUEST_ASE_ID(request)  free(request->aseId); free(request);
#define FREE_RESPONSE_ASE(response)  free(response->ase); free(response);
#define FREE_RESPONSE_ASE_RESULT(response)  free(response->gattAscsAseResult); free(response);

#endif /* BAP_PRIVATE_H */
