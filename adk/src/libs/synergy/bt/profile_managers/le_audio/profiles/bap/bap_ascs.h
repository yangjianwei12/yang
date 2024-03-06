/*******************************************************************************

Copyright (C) 2020-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief This file contains ASCS message format.
 *
 */

#ifndef BAP_ASCS_H_
#define BAP_ASCS_H_

#include "qbl_types.h"
#include "bluetooth.h"
#include "bap_client_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Message type for bap */
typedef uint8 AscsMsg; /*!< Message identity */
typedef uint8 PacsMsg; /*!< Message identity */

/* AscsMsg(ASE notification) offsets */
#define ASE_ID_OFFSET       (0)
#define ASE_STATE_OFFSET    (1)
#define ASE_RESULT_OFFSET   (3)
#define ASE_OPCODE_OFFSET   (4)

/* ASCS Control Point operations offsets */
#define ASE_CP_OPCODE_OFFSET      (0)
#define ASE_CP_NUM_ASES_OFFSET    (1)
#define ASE_CP_MSG_START_OFFSET   (2)
#define ASE_CP_ASE_ID_OFFSET      (3)
#define ASE_CP_CCID_OFFSET        (3)  /* from ASE ID */

/* Size of ASE Control point common parameters */
#define ASE_CP_CMD_HDR_SIZE       (2) /* sizeof(Opcode + Num_ASEs) */

#define ASE_CP_NOTIFY_OPCODE_OFFSET      (0)
#define ASE_CP_NOTIFY_NUM_ASE_OFFSET     (1)
#define ASE_CP_NOTIFY_ASE_ID_OFFSET      (2)
#define ASE_CP_NOTIFY_RESULT_OFFSET      (3)
#define SIZE_OF_ASE_CP_NOTIFY            (3)

/* ASE OPCODES */
#define ASE_OPCODE_CONFIG_CODEC      ((uint8)0x01)
#define ASE_OPCODE_CONFIG_QOS        ((uint8)0x02)
#define ASE_OPCODE_ENABLE            ((uint8)0x03)
#define ASE_OPCODE_START_READY       ((uint8)0x04)
#define ASE_OPCODE_DISABLE           ((uint8)0x05)
#define ASE_OPCODE_STOP_READY        ((uint8)0x06)
#define ASE_OPCODE_METADATA         ((uint8)0x07)
#define ASE_OPCODE_RELEASE           ((uint8)0x08)

/* ASE States */
#define ASE_STATE_IDLE                  ((uint8)0x00)
#define ASE_STATE_CODEC_CONFIGURED      ((uint8)0x01)
#define ASE_STATE_QOS_CONFIGURED        ((uint8)0x02)
#define ASE_STATE_ENABLING              ((uint8)0x03)
#define ASE_STATE_STREAMING             ((uint8)0x04)
#define ASE_STATE_DISABLING             ((uint8)0x05)
#define ASE_STATE_RELEASING             ((uint8)0x06)

/* Response Code */
#define INVALID_ASE_STATE_TRANSITION     0x01
#define UNSUPPORTED_AUDIO_CAPABILITY     0x02
#define UNSUPPORTED_PARAMETER_VALUE      0x03
#define REJECTED_PARAMETER_VALUE         0x04
#define INVALID_PARAMETER_VALUE          0x05
#define UNSPECIFIED_ERROR                0x06
#define INSUFFICIENT_RESOURCE            0x07

#define INVALID_ASE_ID_ERROR             0x01
#define NO_ASE_AVAILABLE                 0x02

#define ASE_CONTROL_POINT_ERROR          0xFE
#define ASE_CP_TRUNCATED_OPERATION       0xFF

/* Error codes */

/*! Operation was successful */
#define ASCS_RESULT_SUCCESS                     ((uint8)0x00)
/*! The Opcode given was not valid */
#define ASCS_INVALID_CMD_OPCODE                 ((uint8)0x01)
/*! Request rejected */
#define REQUEST_REJECTED                        ((uint8)0x02)
/*! Invalid ASE_ID */
#define ASCS_INVALID_ASE_ID                     ((uint8)0xAA)
/*! Invalid operation */
#define ASCS_INVALID_OPERATION                  ((uint8)0x05)
/*! Resource unavailable */
#define ASCS_RESOURCE_UNAVAILABLE               ((uint8)0x06)
/*! Unspecified error */
#define ASCS_UNSPECIFIED_ERROR                  ((uint8)0x05)
/*! Request not supported */
#define ASCS_REQUEST_NOT_SUPPORTED              ((uint8)0x08)
/*! Configuration rejected */
#define ASCS_CONFIGURATION_REJECTED             ((uint8)0x11)
/*! Device busy */
#define ASCS_DEVICE_BUSY                        ((uint8)0x0A)

#define ASCS_INVALID_DIRECTION                  ((uint8)0x01)
#define INVALID_CONTENT_TYPE                    ((uint8)0x0C)
#define ASCS_INVALID_SAMPLING_FREQUENCY         ((uint8)0x03)

/* Audio Quality of Service parameter length */
#define MAX_QOS_LEN                             ((uint8)10)
#define MAX_SERVER_SUPPORTED_QOS_LEN            ((uint8)15)
#define MAX_SUPPORTED_CCID                      ((uint8)5) /* TODO */

/* 
   As per table 4.2 and table 4.5 of ASCSv1.0 spec, ase characteristics metadataLength
   offset is 4 and metadata offset is 5 if metadataLength is not zero
*/
#define ASE_METADATA_LENGTH_OFFSET              ((uint8)0x04)
#define ASE_METADATA_OFFSET                     ((uint8)0x05)

/*
 * From ASCS d09r03_1
 */

typedef struct
{
    uint8 codecId;
    uint8 companyId[2];
    uint8 vendorCodecId[2];
} AscsCodecId;

typedef enum
{
    ascs_response_code_invalid_ase_state_machine_transition = 0x01,
    ascs_response_code_unsupported_audio_capabilities       = 0x02,
    ascs_response_code_unsupported_parameter_value          = 0x03,
    ascs_response_code_rejected_parameter_value             = 0x04,
    ascs_response_code_invalid_parameter_value              = 0x05,
    ascs_response_code_unspecified_error                    = 0x06,
    ascs_response_code_insufficient_resources               = 0x07
} ascs_response_code_t;

/*
 * From ASCS d09r03_1
 * The error response is set to ascs_error_response_unspecified
 * unless the ascs_response_code is one of:
 *     ascs_response_code_unsupported_parameter_value
 *     ascs_response_code_rejected_parameter_value
 *     ascs_response_code_invalid_parameter_value
 *
 */
typedef enum
{
    ascs_error_response_unspecified                         = 0x00,
    ascs_error_response_codec_id                            = 0x01,
    ascs_error_response_codec_specific_configuration_length = 0x02,
    ascs_error_response_codec_specific_configuration        = 0x03,
    ascs_error_response_sdu_interval                        = 0x04,
    ascs_error_response_framing                             = 0x05,
    ascs_error_response_phy                                 = 0x06,
    ascs_error_response_maximum_sdu_size                    = 0x07,
    ascs_error_response_retransmission_number               = 0x08,
    ascs_error_response_transport_latency                   = 0x09,
    ascs_error_response_presentation_delay                  = 0x0A,
    ascs_error_response_content_type                        = 0x0B,
    ascs_error_response_ccid                                = 0x0C
} ascs_error_response_t;
/*
 * ASCS d0r02 Table 4.3: The error status is of the form 0xRRSSTT where:
 * (lsb)TT = Reason
 *      SS = Error Code
 * (msb)RR = Failed ASE Control Point operation opcode
 *
 * ASCS d0r03_1 Table 4.3: The error status is of the form 0xRRSSTT where:
 * (lsb)TT = Error_Response
 *      SS = Response_Code
 * (msb)RR = Failed ASE control operation opcode
 *
 */
typedef struct
{
    uint8 errorResponse;
    uint8 responseCode;
    uint8 failedOpcode;
} AscsAseErrorStatus;

typedef struct
{
    uint8             aseId; 
    uint8             aseState;
    AscsAseErrorStatus     aseErrorStatus;
}aseStatusInfo;

/*! \brief ASCS Codec Configure operation message format

    The pusrpose of this message is to configure Audio control endpoint from the
    discovered PAC records
*/
typedef struct
{
    uint8             aseId; 
    uint8             direction;       /* Supported audio direction (sink or source) */
    AscsCodecId         codecId;
}AscsCodecInfo;

/* Codec config notification message format */
typedef struct
{
    uint8             aseId; 
    uint8             aseState;
    uint8             preferredFraming;
    uint8             preferredPhy;
    uint8             retransmissionNumber;
    uint8             transportLatency[2];
    uint8             presentationDelayMin[3];
    uint8             presentationDelayMax[3];
    uint8             preferredPDmin[3];
    uint8             preferredPDmax[3];
    AscsCodecId         codecId;
    uint8             codecSpecificConfigLength;
    uint8             codecSpecificConfig[1];
} AscsConfigureCodecNotify;


typedef struct
{
    uint8             aseId; 
    uint8             cigId;
    uint8             cisId;
    uint8             sduInterval[3];
    uint8             framing;
    uint8             phy;
    uint8             maximumSduSize[2];
    uint8             retransmissionNumber;
    uint8             transportLatency[2];
    uint8             presentationDelay[3];
}AscsConfigQosInfo;

typedef struct
{
    uint8         aseId;
    uint8         responseCode;
    uint8         reason;
}AseErrorNotify;


#ifdef __cplusplus
}
#endif

#endif /* BAP_ASCS_H_ */

