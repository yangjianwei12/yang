/* Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_ASCS_SERVER_PRIVATE_H
#define GATT_ASCS_SERVER_PRIVATE_H

#include "gatt_ascs_server.h"
#include "gatt_ascs_server_db.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_bluestack_types.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_ascs_server_handover.h"

#define GATT_ASCS_NUM_CONNECTIONS_MAX   3

/* Check Init Input prams are valid */
#define INPUT_PARAM_NULL(appTask,ascs) (appTask == NULL) || (ascs == NULL )

/* even ASEs will be used as sources */
#define GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) (((ase)->aseId <= 0x04)? (GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK) : (GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE))

/* default client config 0xFFFF implies remote client has not written
 * any CCCD which is as good as CCCD disable by remote client */
#define GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG   0xFFFF

/* Macros for creating messages */
/* Not used as of now */
/* #define MAKE_ASCS_SERVER_MESSAGE(TYPE) TYPE* message = (TYPE *)malloc(sizeof(TYPE))*/

/*To be used for WORD order values*/
/* Note: To be compliant with older compilers 'flexible' arrays are declared as arrays of size 1, e.g.
 * struct NameOfSomethingWithAFlexibleArray
 * {
 *     .
 *     .
 *     .
 *     FlexibleArrayType flexibleArrayName[1];
 * };
 * The outcome of this is that the sizeof(NameOfSomethingWithAFlexibleArray) _already_ has memory
 * to store 1 instance of FlexibleArrayType, so when allocating memory for a structure of this type
 * we need to additionally add sufficient memory to store (NUM_ELEMENTS - 1) instances.
 * */
#define MAKE_ASCS_SERVER_FLEX_MESSAGE(TYPE, NUM_ELEMENTS, ELEMENT_SIZE) TYPE *message = (TYPE *) zpmalloc(sizeof(TYPE) + ((NUM_ELEMENTS - 1) * (ELEMENT_SIZE)))

/* To correlate ASE characteristics with their array index */
enum ASE_IDX
{
#if defined(HANDLE_ASCS_ASE_CHAR_1)
    GATT_ASCS_ASE_CHAR_1_IDX    = 0,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_2)
    GATT_ASCS_ASE_CHAR_2_IDX    = 1,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_3)
    GATT_ASCS_ASE_CHAR_3_IDX    = 2,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_4)
    GATT_ASCS_ASE_CHAR_4_IDX    = 3,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_5)
    GATT_ASCS_ASE_CHAR_5_IDX    = 4,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_6)
    GATT_ASCS_ASE_CHAR_6_IDX    = 5,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_7)
    GATT_ASCS_ASE_CHAR_7_IDX    = 6,
#endif
#if defined(HANDLE_ASCS_ASE_CHAR_8)
    GATT_ASCS_ASE_CHAR_8_IDX    = 7,
#endif
    GATT_ASCS_NUM_ASES_MAX
};

/* State values reported in the Audio Stream Endpoint (ASE) Characteristic */
typedef enum
{
    GATT_ASCS_SERVER_ASE_STATE_IDLE             = 0x00,
    GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED = 0x01,
    GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED   = 0x02,
    GATT_ASCS_SERVER_ASE_STATE_ENABLING         = 0x03,
    GATT_ASCS_SERVER_ASE_STATE_STREAMING        = 0x04,
    GATT_ASCS_SERVER_ASE_STATE_DISABLING        = 0x05,
    GATT_ASCS_SERVER_ASE_STATE_RELEASING        = 0x06
} GattAscsServerAseState;

/* Opcode values for the Audio Stream Endpoint (ASE) Control Point Characteristic */
typedef enum
{
    GATT_ASCS_SERVER_INVALID_OPCODE               = 0x00,
    GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE          = 0x01,
    GATT_ASCS_SERVER_CONFIG_QOS_OPCODE            = 0x02,
    GATT_ASCS_SERVER_ENABLE_OPCODE                = 0x03,
    GATT_ASCS_SERVER_RECEIVER_START_READY_OPCODE  = 0x04,
    GATT_ASCS_SERVER_DISABLE_OPCODE               = 0x05,
    GATT_ASCS_SERVER_RECEIVER_STOP_READY_OPCODE   = 0x06,
    GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE       = 0x07,
    GATT_ASCS_SERVER_RELEASE_OPCODE               = 0x08
} gattAscsServerOpcodes;

/* Response codes are in accordance with ASCS 'Validation r03' */
enum
{
    GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS                                    = GATT_ASCS_ASE_RESULT_SUCCESS,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_OPCODE                         = 0x01,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_LENGTH_OPERATION                   = 0x02,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_ID                             = 0x03,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_STATE_TRANSITION               = 0x04,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_DIRECTION                      = 0x05,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_AUDIO_CAPABILITIES             = GATT_ASCS_ASE_RESULT_UNSUPPORTED_AUDIO_CAPABILITIES,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE  = GATT_ASCS_ASE_RESULT_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE,
    GATT_ASCS_ASE_RESPONSE_CODE_REJECTED_CONFIGURATION_PARAMETER_VALUE     = GATT_ASCS_ASE_RESULT_REJECTED_CONFIGURATION_PARAMETER_VALUE,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_CONFIGURATION_PARAMETER_VALUE      = GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_METADATA                       = GATT_ASCS_ASE_RESULT_UNSUPPORTED_METADATA,
    GATT_ASCS_ASE_RESPONSE_CODE_REJECTED_METADATA                          = GATT_ASCS_ASE_RESULT_REJECTED_METADATA,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_METADATA                           = GATT_ASCS_ASE_RESULT_INVALID_METADATA,
    GATT_ASCS_ASE_RESPONSE_CODE_INSUFFICIENT_RESOURCES                     = GATT_ASCS_ASE_RESULT_INSUFFICIENT_RESOURCES,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSPECIFIED_ERROR                          = GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR
};

#define GATT_ASCS_SDU_INTERVAL_MIN       0x000000FF
/* ASCS validation r05 has the SDU Interval range as: 0x000000FF - 0x000FFFFF */
#define GATT_ASCS_SDU_INTERVAL_MAX       0x000FFFFF

#define GATT_ASCS_MAXIMUM_SDU_SIZE_MIN   0x0000
#define GATT_ASCS_MAXIMUM_SDU_SIZE_MAX   0x0FFF

#define GATT_ASCS_RETRANSMISSION_NUMBER_MIN   0x00
/* ASCS validation r01 has the Retransmission number range as: 0x00 - 0xFF */
#define GATT_ASCS_RETRANSMISSION_NUMBER_MAX   0xFF

#define GATT_ASCS_MAX_TRANSPORT_LATENCY_MIN   0x0005
#define GATT_ASCS_MAX_TRANSPORT_LATENCY_MAX   0x0FA0

/* Reason values are in accordance with ASCS 'Validation r03' */
enum
{
    GATT_ASCS_ASE_REASON_UNSPECIFIED                        =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED,
    GATT_ASCS_ASE_REASON_CODEC_ID                           =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_ID,
    GATT_ASCS_ASE_REASON_CODEC_SPECIFIC_CONFIGURATION       =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION,
    GATT_ASCS_ASE_REASON_SDU_INTERVAL                       =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_SDU_INTERVAL,
    GATT_ASCS_ASE_REASON_FRAMING                            =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING,
    GATT_ASCS_ASE_REASON_PHY                                =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PHY,
    GATT_ASCS_ASE_REASON_MAXIMUM_SDU_SIZE                   =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAXIMUM_SDU_SIZE,
    GATT_ASCS_ASE_REASON_RETRANSMISSION_NUMBER              =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_RETRANSMISSION_NUMBER,
    GATT_ASCS_ASE_REASON_MAX_TRANSPORT_LATENCY              =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAX_TRANSPORT_LATENCY,
    GATT_ASCS_ASE_REASON_PRESENTATION_DELAY                 =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PRESENTATION_DELAY,
    GATT_ASCS_ASE_REASON_INVALID_ASE_CIS_MAPPING            =  0x0A /* NOTE: This is not exposed to the app/profile, so a corresponding 'GATT_ASCS_RESULT_ADDITIONAL_INFO_*' enum does not exist */
};

enum
{
    GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES = 0xFF,   /* ASCS 'validation r03' uses numAses = 0xFF in response to an invalid length op or unrecognised op code */
    GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID   = 0x00    /* ASCS 'validation r03' uses aseId = 0x00 in response to an invalid length op or unrecognised op code */
};

typedef struct
{
    uint8 aseId;
    /* responseCode shall be set to GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS == GATT_ASCS_ASE_RESULT_SUCCESS == 0  if the
     * server successfully completes a client-initiated ASE Control operation,
     * otherwise shall be set as defined in ASCS d09r04 Table 5.1
     */
    uint8 responseCode;
    /* reason shall be set to GATT_ASCS_ASE_REASON_UNSPECIFIED == GATT_ASCS_RESULT_ADDITIONAL_INFO_UNSPECIFIED == 0x00
     * if the server successfully completes a client-initiated ASE Control operation,
     * otherwise shall be set as defined in ASCS d09r04 Table 5.1
     */
    uint8 reason;
} AscsAseResponse;

typedef struct
{
    uint8 metadataLength;
    uint8 *metadata;
    GattAscsServerConfigureCodecInfo cachedConfigureCodecInfo;
    GattAscsServerConfigureQosInfo* cachedConfigureQosInfoFromServer;
} AscsAseDynamicData;

typedef struct
{
    uint8 aseId;
    GattAscsServerAseState state;
    ClientConfig clientCfg;
    AscsAseDynamicData *dynamicData;
} GattAscsServerAse;

typedef struct
{
    ClientConfig clientCfg;
    /* Opcode of the client-initiated ASE Control operation causing this response */
    uint8        opCode;
    /* Total number of ASEs the server is providing a response for */
    uint8        numAses;
    AscsAseResponse aseResponses[GATT_ASCS_NUM_ASES_MAX];
} GattAscsAseControlPointNotify;

/*! @brief Audio Stream Endpoint Service library connection structure type .
 */

typedef struct GattAscsConnection
{
    ConnectionId      cid;
    GattAscsAseControlPointNotify aseControlPointNotify;
    GattAscsServerAse ase[GATT_ASCS_NUM_ASES_MAX];
} GattAscsConnection;

/*
 * This structure is private and should not be visible to the application, there is one instance of this structure per ASCS service instance.
 * An instance of this structure is referenced by a serviceHandle - the application uses the service handle to identify a
 * particular ASCS service instance.
 */

typedef struct GattAscsServer
{
    AppTaskData         libTask;
    AppTask             appTask;
    CsrBtGattId         gattId;
    ServiceHandle       serviceHandle;
    uint8               numConnections;
    GattAscsConnection* connection[GATT_ASCS_NUM_CONNECTIONS_MAX];
    struct AscsHandoverMgr*     ascsHandoverMgr;
} GattAscsServer;

typedef struct
{
    uint16 size;
    bool   error;
    uint8* dataStart;
    uint8* data;
} GattAscsBuffIterator;

typedef struct
{
    const CsrBtGattAccessInd* accessInd;
    uint8  bufferIndex;
    uint16 offset;
    uint8 error;
} AccessIndIterator;

typedef struct
{
    /*
    * Fields in the ASE Characteristic in Codec Configured state
    */
    uint8 aseId;
    uint8 aseState;
    uint8 framing;
    uint8 preferredPhy;
    uint8 preferredRetransmissionNumber;
    uint8 maxTransportLatency[2];
    uint8 presentationDelayMin[3];
    uint8 presentationDelayMax[3];
    uint8 preferredPresentationDelayMin[3];
    uint8 preferredPresentationDelayMax[3];
    uint8 codecId[5];
    uint8 codecSpecificConfigurationLength;
   /* codecSpecificConfiguration variable length field if codecSpecificConfigurationLength > 0*/
} CodecConfigInfo;

void accessIndIteratorInitialise(AccessIndIterator* accessIndIterator, const CsrBtGattAccessInd* accessInd);
uint8 accessIndIteratorRead8(AccessIndIterator* accessIndIter);
uint16 accessIndIteratorRead16(AccessIndIterator* accessIndIter);
uint32 accessIndIteratorRead24(AccessIndIterator* accessIndIter);
uint8* accessIndIteratorReadMultipleOctets(AccessIndIterator* accessIndIter, uint8 numOctets);

void ascsBuffIteratorInitialise(GattAscsBuffIterator* iter, uint8* buffer, uint16 size);
void   ascsBuffIteratorSkipOctets(GattAscsBuffIterator* iter, uint8 numOctets);
bool ascsBuffIteratorWrite8(GattAscsBuffIterator* iter, uint8 value);
bool ascsBuffIteratorWrite16(GattAscsBuffIterator* iter, uint16 value);
bool ascsBuffIteratorWrite24(GattAscsBuffIterator* iter, uint32 value);
bool ascsBuffIteratorWriteMultipleOctets(GattAscsBuffIterator* iter, uint8* value, uint8 numOctets);

void ascsConnectionDestruct(GattAscsConnection* connection);

#define ascsBuffIteratorErrorDetected(iter)     ((iter)->error)
#define ascsBuffIteratorGetCurrentDataPtr(iter) ((iter)->data)
#define ascsBuffIteratorSetError(iter)          ((iter)->error = TRUE)

bool ascsServerSetAseStateAndNotify(const GattAscsServer *ascsServer,
                                    ConnectionId cid,
                                    GattAscsServerAse* ase,
                                    GattAscsServerAseState newState,
                                    GattAscsServerConfigureCodecServerReqInfo* stateSpecificData);

GattAscsServerAse* ascsServerFindAse(const GattAscsServer *ascsServer, ConnectionId cid, uint8 aseId);
GattAscsServerAse* ascsConnectionFindAse(GattAscsConnection *connection, uint8 aseId);

GattAscsConnection* ascsFindConnection(const GattAscsServer *ascsServer, ConnectionId cid);
void ascsRemoveConnection(GattAscsServer *ascsServer, GattAscsConnection* connection);

uint8* ascsAseConstructCharacteristicValue(GattAscsServerAse* ase,
                                           uint8* characteristicSize,
                                           GattAscsServerConfigureCodecServerReqInfo* configureCodecServerInfo);
uint8* ascsAseControlPointConstructCharacteristicValue(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                       uint8* characteristicLength);

void ascsAseControlPointCharacteristicReset(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 opCode);
void ascsAseControlPointCharacteristicAddAseResponse(GattAscsAseControlPointNotify* aseControlPointNotify, AscsAseResponse* aseResponse);
void ascsAseControlPointCharacteristicAddSuccessResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify);
void ascsAseControlPointCharacteristicAddUnsupportedOpcodeResponse(GattAscsAseControlPointNotify* aseControlPointNotify);
void ascsAseControlPointCharacteristicAddInvalidTransitionResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                       uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidAseIdResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                           uint8 aseId, uint8 reasonCode);
void ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                            uint8 aseId,
                                                                            uint8 reasonCode);
void ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId);

void ascsAseControlPointNotify(const GattAscsServer* ascsServer, ConnectionId cid);

void gattAscsServerPanic(void);

GattAscsServerReleasingAseInfo* gattAscsFindAseIdsByCisIdAndState(ServiceHandle serviceHandle, ConnectionId cid,
                uint8 cisId, GattAscsServerAseState state);

#define AscsServerMessageSend(_appTask, _msg) \
                      CsrSchedMessagePut(_appTask, ASCS_SERVER_PRIM, ((void*)_msg))

#endif /* GATT_ASCS_SERVER_PRIVATE_H */

