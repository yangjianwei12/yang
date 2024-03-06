/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ASCS_SERVER_PRIVATE_H
#define GATT_ASCS_SERVER_PRIVATE_H

/*#include <csrtypes.h>*/
#include <message.h>
#include <panic.h>

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_ascs_server.h"
#include "gatt_ascs_server_db.h"
#include "gatt_ascs_server_debug.h"

#define GATT_ASCS_NUM_CONNECTIONS_MAX   3

/*This should be the same as the number of ASE characteristics defined in the 
dbi file. ASE characteristic number == instances of GATT_ASCS_SERVER_ASE_T*/

#if defined(HANDLE_ASCS_ASE_CHAR_4)
#define GATT_ASCS_NUM_ASES_MAX                             (4)
#elif  defined(HANDLE_ASCS_ASE_CHAR_3)
#define GATT_ASCS_NUM_ASES_MAX                             (3)
#elif defined(HANDLE_ASCS_ASE_CHAR_2)
#define GATT_ASCS_NUM_ASES_MAX                             (2)
#elif defined(HANDLE_ASCS_ASE_CHAR_1)
#define GATT_ASCS_NUM_ASES_MAX                             (1)
#else
#error "No ASCS characteristics"
#endif

/* Check Init Input prams are valid */
#define INPUT_PARAM_NULL(app_task,ascs) (app_task == NULL) || (ascs == NULL )


/* Macros for creating messages */
#define MAKE_ASCS_SERVER_MESSAGE(TYPE) TYPE* message = (TYPE *)PanicNull(calloc(1,sizeof(TYPE)))

/*To be used for WORD order values*/
#define MAKE_ASCS_SERVER_FLEX_MESSAGE(TYPE, LEN) TYPE *message = (TYPE *) PanicNull(calloc(1,sizeof(TYPE) + LEN))


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
} gatt_ascs_server_opcodes;

/* Response codes are in accordance with ASCS 'Validation r03' */
enum
{
    GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS                                    = GATT_ASCS_ASE_RESULT_SUCCESS,
    GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_OPCODE                         = 0x01,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_LENGTH_OPERATION                   = 0x02,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_ID                             = 0x03,
    GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_STATE_TRANSITION               = 0x04,
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
    GATT_ASCS_ASE_REASON_DIRECTION                          =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_DIRECTION,
    GATT_ASCS_ASE_REASON_CODEC_ID                           =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_ID,
    GATT_ASCS_ASE_REASON_CODEC_SPECIFIC_CONFIGURATION       =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION,
    GATT_ASCS_ASE_REASON_SDU_INTERVAL                       =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_SDU_INTERVAL,
    GATT_ASCS_ASE_REASON_FRAMING                            =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING,
    GATT_ASCS_ASE_REASON_PHY                                =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PHY,
    GATT_ASCS_ASE_REASON_MAXIMUM_SDU_SIZE                   =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAXIMUM_SDU_SIZE,
    GATT_ASCS_ASE_REASON_RETRANSMISSION_NUMBER              =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_RETRANSMISSION_NUMBER,
    GATT_ASCS_ASE_REASON_MAX_TRANSPORT_LATENCY              =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAX_TRANSPORT_LATENCY,
    GATT_ASCS_ASE_REASON_PRESENTATION_DELAY                 =  GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PRESENTATION_DELAY,
    GATT_ASCS_ASE_REASON_INVALID_ASE_CIS_MAPPING            =  0x0B /* NOTE: This is not exposed to the app/profile, so a corresponding 'gatt_ascs_result_additional_info_*' enum does not exist */
};

enum
{
    GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES = 0xFF,   /* ASCS 'validation r03' uses numAses = 0xFF in response to an invalid length op or unrecognised op code */
    GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID   = 0x00    /* ASCS 'validation r03' uses aseId = 0x00 in response to an invalid length op or unrecognised op code */
};

typedef struct
{
    uint8 aseId;
    /* response_code shall be set to gatt_ascs_ase_response_CODE_success == GATT_ASCS_ASE_RESULT_SUCCESS == 0  if the
     * server successfully completes a client-initiated ASE Control operation,
     * otherwise shall be set as defined in ASCS d09r04 Table 5.1
     */
    uint8 responseCode;
    /* reason shall be set to GATT_ASCS_ASE_REASON_unspecified == gatt_ascs_result_additional_info_unspecified == 0x00
     * if the server successfully completes a client-initiated ASE Control operation,
     * otherwise shall be set as defined in ASCS d09r04 Table 5.1
     */
    uint8 reason;
} AscsAseResponse;

/*
 * TODO : Many of the values stored in the ASE struct are only relevant when the ASE is in a particular state,
 *        e.g. codec_config is only relevant when the ASE is in the codec configured state (used when
 *        'codec configured' characteristic is being read by the client).
 *        Having said that; will the profile/app expect to be able to retrieve these values from us at any time in the future?
 */
typedef struct
{
    uint8                   aseId;
    GattAscsServerAseState  state;
    ClientConfig            clientCfg;
    uint8                   metadataLength; /* Provided in the Enable Ind from the client, notified to client in many states: enabling, streaming, disabling and releasing.  */
    uint8*                  metadata;        /* Provided in the Enable Ind from the client, notified to client in many states: enabling, streaming, disabling and releasing.  */
    bool                    qosInfoValid;
    GattAscsServerConfigureCodecInfo cachedConfigureCodecInfo;
    GattAscsServerConfigureQosInfo   cachedConfigureQosInfoFromServer;
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

typedef struct
{
    /* This is in the BASS server - is a similar thing needed here?
    uint16 handle_audio_stream_endpoint_control_point;
    uint16 handle_audio_stream_endpoint;
    */
    GattAscsAseControlPointNotify aseControlPointNotify;
    ConnectionId      cid;
    uint8             numAses;
    GattAscsServerAse ase[GATT_ASCS_NUM_ASES_MAX];
} GattAscsConnection;

/*
 * This structure is private and should not be visible to the application, there is one instance of this structure per ASCS service instance.
 * An instance of this structure is referenced by a service_handle - the application uses the service handle to identify a
 * particular ASCS service instance.
 */
typedef struct
{
    TaskData            libTask;
    Task                appTask;
    uint8               numConnections;
    GattAscsConnection* connection[GATT_ASCS_NUM_CONNECTIONS_MAX];
} GattAscsServer;

typedef struct
{
    uint16 size;
    bool   error;
} GattAscsBuffIteratorCommon;

typedef struct
{
    GattAscsBuffIteratorCommon common;
    uint8* dataStart;
    uint8* data;
} GattAscsBuffIterator;

typedef struct
{
    GattAscsBuffIteratorCommon common;
    const uint8* dataStart;
    const uint8* data;
} GattAscsBuffIteratorReadOnly;

void ascsBuffIteratorInitialise(GattAscsBuffIterator* iter, uint8* buffer, uint16 size);
void ascsBuffIteratorInitialiseReadOnly(GattAscsBuffIteratorReadOnly* iter, const uint8* buffer, uint16 size);

uint8  ascsBuffIteratorGet8(GattAscsBuffIteratorReadOnly* iter);
uint16 ascsBuffIteratorGet16(GattAscsBuffIteratorReadOnly* iter);
uint32 ascsBuffIteratorGet24(GattAscsBuffIteratorReadOnly* iter);
uint8* ascsBuffIteratorGetMultipleOctets(GattAscsBuffIteratorReadOnly* iter, uint8 num_octets);
void   ascsBuffIteratorSkipOctets(GattAscsBuffIterator* iter, uint8 num_octets);
void   ascsBuffIteratorSkipOctetsReadOnly(GattAscsBuffIteratorReadOnly* iter, uint8 num_octets);


bool ascsBuffIteratorWrite8(GattAscsBuffIterator* iter, uint8 value);
bool ascsBuffIteratorWrite16(GattAscsBuffIterator* iter, uint16 value);
bool ascsBuffIteratorWrite24(GattAscsBuffIterator* iter, uint32 value);
bool ascsBuffIteratorWriteMultipleOctets(GattAscsBuffIterator* iter, uint8* value, uint8 num_octets);

#define ascsBuffIteratorErrorDetected(iter)     ((iter)->common.error)
#define ascsBuffIteratorGetCurrentDataPtr(iter) ((iter)->data)
#define ascsBuffIteratorSetError(iter)          ((iter)->common.error = TRUE)

bool ascsServerSetAseStateAndNotify(const GattAscsServer *ascs_server,
                                    ConnectionId cid,
                                    GattAscsServerAse* ase,
                                    GattAscsServerAseState new_state);

GattAscsServerAse* ascsServerFindAse(const GattAscsServer *ascs_server, ConnectionId cid, uint8 aseId);
GattAscsServerAse* ascsConnectionFindAse(GattAscsConnection *connection, uint8 aseId);

GattAscsConnection* ascsFindConnection(const GattAscsServer *ascs_server, ConnectionId cid);

uint8* ascsAseConstructCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_size);
uint8* ascsAseControlPointConstructCharacteristicValue(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                       uint8* characteristic_length);

void ascsAseControlPointCharacteristicReset(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 op_code);
void ascsAseControlPointCharacteristicAddAseResponse(GattAscsAseControlPointNotify* ase_control_point_notify, AscsAseResponse* ase_response);
void ascsAseControlPointCharacteristicAddSuccessResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify);
void ascsAseControlPointCharacteristicAddUnsupportedOpcodeResponse(GattAscsAseControlPointNotify* ase_control_point_notify);
void ascsAseControlPointCharacteristicAddInvalidTransitionResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                       uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidAseIdResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId);
void ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                           uint8 aseId, uint8 reason_code);
void ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                            uint8 aseId,
                                                                            uint8 reason_code);
void ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId);

void ascsAseControlPointNotify(const GattAscsServer* ascs_server, ConnectionId cid);
#endif /* GATT_ASCS_SERVER_PRIVATE_H */

