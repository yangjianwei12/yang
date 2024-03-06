/****************************************************************************
* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_PRIVATE_H
#define BAP_PRIVATE_H

#include <stdlib.h>

#include "csr_panic.h"
#include "csr_bt_gatt_lib.h"
#include "csr_list.h"
#include <service_handle.h>
#include "bap_server_prim.h"

/*! QOS parameter settings used to indicate to the unicast client 
    values the BT controller supports/prefers */
#define ASE_RETRANSMISSION_NUMBER   27
#define ASE_PHY_PREFERENCE          2 /* 0b00000010: LE 2M PHY preferred */
#define ASE_PREFERRED_FRAMING       0
#define ASE_MAXIMUM_SDU_SIZE        0xfff
#define ASE_SDU_INTERVAL_MIN        0xff
#define ASE_SDU_INTERVAL_MAX        0xfffff
#define ASE_PRESENTATION_DELAY_MIN  25000
#define ASE_PRESENTATION_DELAY_MAX  40000
#define ASE_PREFERRED_PRESENTATION_DELAY_MAX  0x000000
#define ASE_PREFERRED_PRESENTATION_DELAY_MAX  0x000000
#define ASE_TRANSPORT_LOW_LATENCY_MAX       40
#define ASE_TRANSPORT_BALANCE_LATENCY_MAX   80
#define ASE_TRANSPORT_LATENCY_MAX           120

#define ASE_PRESENTATION_DELAY_MIN_GAMING  18000


#define BAP_SERVER_MAX_NUM_ASES    8

/* Maximum number of GATT connections */
#define BAP_SERVER_MAX_CONNECTIONS (3)

/* Sampling frequency values used in LTV of Codec Config operations  */
#define BAP_SERVER_SAMPLING_FREQUENCY_8kHz          (0x01)
#define BAP_SERVER_SAMPLING_FREQUENCY_11_025kHz     (0x02)
#define BAP_SERVER_SAMPLING_FREQUENCY_16kHz         (0x03)
#define BAP_SERVER_SAMPLING_FREQUENCY_22_050kHz     (0x04)
#define BAP_SERVER_SAMPLING_FREQUENCY_24kHz         (0x05)
#define BAP_SERVER_SAMPLING_FREQUENCY_32kHz         (0x06)
#define BAP_SERVER_SAMPLING_FREQUENCY_44_1kHz       (0x07)
#define BAP_SERVER_SAMPLING_FREQUENCY_48kHz         (0x08)
#define BAP_SERVER_SAMPLING_FREQUENCY_88_200kHz     (0x09)
#define BAP_SERVER_SAMPLING_FREQUENCY_96kHz         (0x0A)
#define BAP_SERVER_SAMPLING_FREQUENCY_176_420kHz    (0x0B)
#define BAP_SERVER_SAMPLING_FREQUENCY_192kHz        (0x0C)
#define BAP_SERVER_SAMPLING_FREQUENCY_384kHz        (0x0D)

#define BAP_SERVER_FRAME_DURATION_7P5MS  (0x00)
#define BAP_SERVER_FRAME_DURATION_10MS   (0x01)

#define BAP_SERVER_LTV_LENGTH_OFFSET         0
#define BAP_SERVER_LTV_TYPE_OFFSET           1
#define BAP_SERVER_AUDIO_CONTEXT_LTV_SIZE    4

/* The Basic Audio Profile internal structure. */
typedef struct __BAP
{
    AppTaskData libTask;
    AppTask appUnicastTask;
    AppTask appBroadcastTask;
    AppTask appSinkTask;
    AppTask pendingTask;

    /*! ID of the connection */
    ConnId cid[BAP_SERVER_MAX_CONNECTIONS];
    uint8 numConfigInstance[BAP_SERVER_MAX_CONNECTIONS];

    /*! Profile handle of the BAP instance*/
    bapProfileHandle profileHandle;

    /*! services handles fo ASCS PACS and BASS */
    ServiceHandle ascsHandle;
    ServiceHandle pacsHandle;
    ServiceHandle bassHandle;
} BAP;

typedef struct
{
    uint8* ltvStart;
} LTV;

typedef struct
{
    const BapServerCodecPdMin* pdLookup;
    uint8 numEntries;
} PdLookupTable;

#define BapServerMessageSend(_appTask, _msg) \
                      CsrSchedMessagePut(_appTask, BAP_SERVER_PRIM, ((void*)_msg))

#define MAKE_BAP_MESSAGE(TYPE) \
    TYPE##_T* message = (TYPE##_T *)CsrPmemZalloc(sizeof(TYPE##_T))

#define MAKE_BAP_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) calloc(1,sizeof(TYPE##_T) + ((LEN) - 1) * sizeof(uint8))

/* Assumes message struct with
 *    uint16 size_value;
 *    uint8 value[1];
 */
#define MAKE_BAP_MESSAGE_WITH_LEN_U8(TYPE, LEN)                           \
    TYPE##_T *message = (TYPE##_T*)CsrPmemZalloc(sizeof(TYPE##_T) + \
                                                     ((LEN) ? (LEN) - 1 : 0))

#define MAKE_BAP_MESSAGE_WITH_VALUE(TYPE, SIZE, VALUE) \
        MAKE_BAP_MESSAGE_WITH_LEN_U8(TYPE, SIZE);          \
        memmove(message->value, (VALUE), (SIZE));           \
        message->size_value = (SIZE)

#define FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(_Handle) \
                              (BAP *)ServiceHandleGetInstanceData(_Handle)

/* Allocation for ASCS Server Request and Response messages */
#define MAKE_ASCS_SERVER_REQUEST_ASE_ID(TYPE, LEN) TYPE *request = (TYPE *) calloc(1, sizeof(TYPE)); \
                request->aseId = (uint8 *)calloc(1, sizeof(uint8) * LEN);
#define MAKE_ASCS_SERVER_RESPONSE_ASE(TYPE, LEN) TYPE *response = (TYPE *) calloc(1, sizeof(TYPE)); \
                response->ase = (TYPE##Ase *)calloc(1, sizeof(TYPE##Ase) * LEN);
#define MAKE_ASCS_SERVER_RESPONSE_ASE_RESULT(TYPE, LEN) TYPE *response = (TYPE *) calloc(1, sizeof(TYPE)); \
                response->gattAscsAseResult = (GattAscsAseResult *)calloc(1, sizeof(GattAscsAseResult) * LEN);

/* Deallocatioin for ASCS Server Request and Response messages */
#define FREE_REQUEST_ASE_ID(request)  free(request->aseId); free(request);
#define FREE_RESPONSE_ASE(response)  free(response->ase); free(response);
#define FREE_RESPONSE_ASE_RESULT(response)  free(response->gattAscsAseResult); free(response);

#define LTV_INITIALISE(ltv, buffer)  ((ltv)->ltvStart = (buffer))
#define LTV_NEXT(ltv) ((ltv)->ltvStart + LTV_LEN(ltv))

/* NOTE: The '+ 1' is necessary because the LTV length (the 'L' value stored within the LTV) does not include the
 *       length of the length field itself (i.e. 1 octet) */
#define LTV_LEN(ltv)   ((ltv)->ltvStart[BAP_SERVER_LTV_LENGTH_OFFSET] + 1)
#define LTV_TYPE(ltv)  ((ltv)->ltvStart[BAP_SERVER_LTV_TYPE_OFFSET])

#endif /* BAP_PRIVATE_H */
