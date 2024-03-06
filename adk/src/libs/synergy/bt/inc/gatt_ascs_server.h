/* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

/*!
@file
@brief   Header file for the GATT Audio Stream Control Service library.

        This file provides documentation for the GATT ASCS library
        API (library name: gatt_ascs_server).
*/

#ifndef GATT_ASCS_SERVER_H
#define GATT_ASCS_SERVER_H

#include "service_handle.h"

#include "csr_bt_types.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"


typedef uint16 ClientConfig;

#define GATT_ASCS_CLIENT_CONFIG_NOT_SET                 ((ClientConfig)0x00)
#define GATT_ASCS_CLIENT_CONFIG_NOTIFY                  ((ClientConfig)0x01)
#define GATT_ASCS_CLIENT_CONFIG_INDICATE                ((ClientConfig)0x02)

typedef uint8 GattAscsAseDirection;
/*! Uninitialised direction */
#define GATT_ASCS_ASE_DIRECTION_SERVER_UNINITIALISED    ((GattAscsAseDirection)0x00)
/*! Describes an ASE on which audio data travels from the client to the server */
#define GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK    ((GattAscsAseDirection)0x01)
/*! Describes an ASE on which audio data travels from the server to the client */
#define GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE  ((GattAscsAseDirection)0x02)

#define MAX_ASE_ID_PER_CIS                               (0x02)

typedef uint8 GattAscsAseResultValue;
#define GATT_ASCS_ASE_RESULT_SUCCESS                                     ((GattAscsAseResultValue)0x00)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_AUDIO_CAPABILITIES              ((GattAscsAseResultValue)0x06)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE   ((GattAscsAseResultValue)0x07)
#define GATT_ASCS_ASE_RESULT_REJECTED_CONFIGURATION_PARAMETER_VALUE      ((GattAscsAseResultValue)0x08)
#define GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE       ((GattAscsAseResultValue)0x09)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_METADATA                        ((GattAscsAseResultValue)0x0A)
#define GATT_ASCS_ASE_RESULT_REJECTED_METADATA                           ((GattAscsAseResultValue)0x0B)
#define GATT_ASCS_ASE_RESULT_INVALID_METADATA                            ((GattAscsAseResultValue)0x0C)
#define GATT_ASCS_ASE_RESULT_INSUFFICIENT_RESOURCES                      ((GattAscsAseResultValue)0x0D)
#define GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR                           ((GattAscsAseResultValue)0x0E)

typedef uint8 GattAscsAseResultAdditionalInfo;
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED                                    ((GattAscsAseResultAdditionalInfo)0x00)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_ID                             ((GattAscsAseResultAdditionalInfo)0x01)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION         ((GattAscsAseResultAdditionalInfo)0x02)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_SDU_INTERVAL                         ((GattAscsAseResultAdditionalInfo)0x03)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING                              ((GattAscsAseResultAdditionalInfo)0x04)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PHY                                  ((GattAscsAseResultAdditionalInfo)0x05)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAXIMUM_SDU_SIZE                     ((GattAscsAseResultAdditionalInfo)0x06)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_RETRANSMISSION_NUMBER                ((GattAscsAseResultAdditionalInfo)0x07)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAX_TRANSPORT_LATENCY                ((GattAscsAseResultAdditionalInfo)0x08)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PRESENTATION_DELAY                   ((GattAscsAseResultAdditionalInfo)0x09)

/*! @brief The set of Framing options used during the QOS Configuration Procedure.
 *         Note: The ASCS specification describes the Framing options differently
 *         for the following procedures:
 *           - Codec Configure and
 *           - QOS Configure
 *           .
 */
typedef uint8 GattAscsFraming;
#define GATT_ASCS_FRAMING_UNFRAMED_ISOAL_PDUS       ((GattAscsFraming)0x00)
#define GATT_ASCS_FRAMING_FRAMED_ISOAL_PDUS         ((GattAscsFraming)0x01)

/*! @brief The set of Framing options used during the Codec Configuration Procedure.
 *         Note: The ASCS specification describes the Framing options differently
 *         for the following procedures:
 *           - Codec Configure and
 *           - QOS Configure
 *           .
 */
typedef uint8 GattAscsCodecConfiguredFraming;
#define GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_SUPPORTED       ((GattAscsCodecConfiguredFraming)0x00)
#define GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_NOT_SUPPORTED   ((GattAscsCodecConfiguredFraming)0x01)

/*! @brief The set of messages an application task can receive from the ASCS library.
 */
typedef uint16 GattAscsServerMessageId;

#define GATT_ASCS_SERVER_INIT_CFM                       ((GattAscsServerMessageId)0)
/* Audio Stream Endpoint messages */
#define GATT_ASCS_SERVER_CONFIGURE_CODEC_IND            ((GattAscsServerMessageId)1)
#define GATT_ASCS_SERVER_CONFIGURE_QOS_IND              ((GattAscsServerMessageId)2)
#define GATT_ASCS_SERVER_ENABLE_IND                     ((GattAscsServerMessageId)3)
#define GATT_ASCS_SERVER_RECEIVER_READY_IND             ((GattAscsServerMessageId)4)
#define GATT_ASCS_SERVER_RECEIVER_READY_CFM             ((GattAscsServerMessageId)5)
#define GATT_ASCS_SERVER_UPDATE_METADATA_IND            ((GattAscsServerMessageId)6)
#define GATT_ASCS_SERVER_DISABLE_IND                    ((GattAscsServerMessageId)7)
#define GATT_ASCS_SERVER_RELEASE_IND                    ((GattAscsServerMessageId)8)
#define GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND        ((GattAscsServerMessageId)9)
#define GATT_ASCS_SERVER_CONFIG_CHANGE_IND              ((GattAscsServerMessageId)10)
/* Library message limit */

/*!
*  @brief The set of PHYs preferred by the server.
*         The server shall not specify a PHY that it does not support.
*         Values are bitwise ORed together to specify that multiple PHYs
*         are supported.
*/
typedef uint8 GattAscsPhy;
/*! 1 Mbps PHY */
#define GATT_ASCS_PHY_1M_BITS_PER_SECOND  ((GattAscsPhy)0x01)
/*! 2 Mbps PHY */
#define GATT_ASCS_PHY_2M_BITS_PER_SECOND  ((GattAscsPhy)0x02)
/*! LE coded PHY */
#define GATT_ASCS_PHY_LE_CODED_PHY        ((GattAscsPhy)0x04)

/*! @brief The target PHY requested by the client during a codec configuration operation.
 */
typedef uint8 GattAscsTargetPhy;
/*! 1 Mbps PHY */
#define GATT_ASCS_TARGET_PHY_LE_1M_PHY       ((GattAscsTargetPhy)0x01)
/*! 2 Mbps PHY */
#define GATT_ASCS_TARGET_PHY_LE_2M_PHY       ((GattAscsTargetPhy)0x02)
/*! LE coded PHY */
#define GATT_ASCS_TARGET_PHY_LE_CODED_PHY    ((GattAscsTargetPhy)0x03)

/*! @brief The target latency requested by the client during a codec configuration operation.
 */
typedef uint8 GattAscsTargetLatency;
/*! Target lower latency in preference to high reliability */
#define GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY                       ((GattAscsTargetLatency)0x01)
/*! Target a balance between latency and reliability */
#define GATT_ASCS_TARGET_LATENCY_TARGET_BALANCED_LATENCY_AND_RELIABILITY    ((GattAscsTargetLatency)0x02)
/*! Target high reliability in preference to low latency */
#define GATT_ASCS_TARGET_LATENCY_TARGET_HIGHER_RELIABILITY                  ((GattAscsTargetLatency)0x03)

/*! @brief The GattAscsAseResult is used in several responses from the Profile/Application to the ASCS library.
 *         It associates the ASE Id with the result value (and any additional information) for that ASE and
 *         is used in responses that references multiple ASEs, i.e. one instance of this structure is
 *         included per referenced ASE.
 */
typedef struct
{
    uint8 aseId;
    /*! The gattAscsAseResultValue field reports the success (or failure) of the currently in progress procedure.
    */
    GattAscsAseResultValue value;
    /*!
     * The additionalInfo field can convey useful information when the profile/app detects an error in a metadata field
     * or in a parameter received from ASCS. Depending on the context in which this field is used (the context is
     * determined by the GatAscsAseResultValue), it can be set to either:
     *      -# the defined GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_XXX value that corresponds to the parameter that has an error
     *      -# the type value of the metadata LTV that has an error
     *      -# GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED
     *
     * This value can be set to GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED in all circumstances, however setting it
     * appropriately can assist in resolving product compatibility issues.
     */
    GattAscsAseResultAdditionalInfo additionalInfo;
} GattAscsAseResult;

/*! @brief This structure uniquely identifies a codec. The codec can either be defined in the Bluetooth
 *         specifications or vendor specific.
 */
typedef struct
{
    /*! The coding format value as defined in the Bluetooth Assigned Numbers.
     *
     *  A value of GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT (0xFF) specifies a Vendor Specific coding format.*/
    uint8 codingFormat;
    /*! For Vendor Specific coding formats this field specifies the Company ID as defined in Bluetooth Assigned Numbers.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be:
     *  GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_COMPANY_ID (0x0000). */
    uint16 companyId;
    /*! For Vendor Specific coding formats this field specifies the Vendor specific Codec Id.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be:
     *  GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_CODEC_ID (0x0000). */
    uint16 vendorSpecificCodecId;
} GattAscsCodecId;

#define GATT_ASCS_CODEC_ID_LEN (5)
#define GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT      (0xFF)
#define GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_COMPANY_ID     (0x0000)
#define GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_CODEC_ID       (0x0000)

/*!
    @brief The fields in this structure are populated by the Profile/Application.
           This structure is passed to the ASCS library when the Profile/Application
           calls GattAscsServerConfigureCodecResponse() in the response to receiving a
           GATT_ASCS_SERVER_CONFIGURE_CODEC_IND.
*/
typedef struct
{
    /*! Maximum number of times that every CIS Data PDU should be retransmitted. This is a recommendation
     *  only. It is meant to select between different levels of stream reliability.
     *
     *  Range: 0x00 to 0x0F
     *
     *  All other values: RFU */
    uint8 retransmissionNumber;
    /*! A bitfield representing the server supported PHYs. The server shall not specify a PHY that is does not support
     *
     *  00000001: LE 1M PHY preferred
     *
     *  00000010: LE 2M PHY preferred
     *
     *  00000100: LE Coded PHY preferred
     */
    GattAscsPhy phyPreference;
    /*! The type of framing supported by this ASE */
    GattAscsCodecConfiguredFraming framing;
    /*! Maximum supported transport latency.
     *
     *  The server preferred value for the Max_Transport_Latency parameter
     *  to be written by the client for this ASE in the Config QoS operation
     *  in units of milliseconds.
     *  The range is 5 milliseconds to 4000 milliseconds, and typical values are between
     *  7.5 milliseconds to 90 milliseconds.
     *
     *  Range: 0x0005 to 0x0FA0
     *
     *  All other values: RFU
     */
    uint16 transportLatencyMax;
    /*! Minimum supported presentation delay in [microseconds]
     *
     *  Range: 0x00000000 to 0x00FFFFFF */
    uint32 presentationDelayMin;
    /*! Maximum supported presentation delay in [microseconds]
     *
     *  Range: 0x00000000 to 0x00FFFFFF */
    uint32 presentationDelayMax;
    /*! Preferred Minimum supported presentation delay in [microseconds]
     *  A value of zero means that the server has no preferred minimum
     *  presentation delay
     *
     *  Range: 0x00000000 to 0x00FFFFFF */
    uint32 preferredPresentationDelayMin;
    /*! Preferred Maximum supported presentation delay in [microseconds]
     *  A value of zero means that the server has no preferred maximum
     *  presentation delay
     *
     *  Range: 0x00000000 to 0x00FFFFFF */
    uint32 preferredPresentationDelayMax;
    uint8  codecConfigurationLength;
    /*! The ownership of the memory pointed to by this field is dependent on the context
     *  in which the GattAscsServerConfigureCodecServerInfo is used. The ownership of
     *  this memory is described in the documentation for the following structures:
     *   - GattAscsServerConfigureCodecRspAse
     *   - GattAscsServerConfigureCodecInfo
     *   - GattAscsServerConfigureCodecReqAse
     *   - GattAscsServerReleaseCompleteAse
     *   .
     */
    uint8* codecConfiguration;
} GattAscsServerConfigureCodecServerReqInfo;

typedef struct
{
    uint32 presentationDelayMin;
    uint8  codecConfigurationLength;
    /*! The ownership of the memory pointed to by this field is dependent on the context
     *  in which the GattAscsServerConfigureCodecServerInfo is used. The ownership of
     *  this memory is described in the documentation for the following structures:
     *   - GattAscsServerConfigureCodecRspAse
     *   - GattAscsServerConfigureCodecInfo
     *   - GattAscsServerConfigureCodecReqAse
     *   - GattAscsServerReleaseCompleteAse
     *   .
     */
    uint8* codecConfiguration;
} GattAscsServerConfigureCodecServerInfo;

/*! @brief This structure contains all the Codec Configuration data collected
 *         from both the client and the Profile/Application throughout the Codec
 *         Configuration procedure.
 *         This structure can be retrieved by the Profile/Application by calling the
 *         GattAscsReadCodecConfiguration() function.
 *         The structure can be requested by the Profile/Application at any time
 *         after the ASE has completed the Codec Configuration procedure (and
 *         before the ASE is released back to an IDLE state).
 */
typedef struct
{
    /*! The Codec ID provided by the client during the Configure Codec Procedure */
    GattAscsCodecId        codecId;
    /*! The target latency provided by the client during the Configure Codec Procedure */
    GattAscsTargetLatency  targetLatency;
    /*! The target PHY provided by the client during the Configure Codec Procedure */
    GattAscsTargetPhy      targetPhy;
    /*! The Codec Configuration data provided by the Profile/Application
     *  during the Configure Codec Procedure.
     *  The ASCS library is responsible for freeing the memory pointed to by
     *  'infoFromServer.codecConfiguration', the Profile/Application must not free this. */
    GattAscsServerConfigureCodecServerInfo infoFromServer;
} GattAscsServerConfigureCodecInfo;

/*! @brief This structure contains all the information provided by the client for a particular
 *         ASE during the Codec Configuration procedure. These structures (one per ASE) are
 *         sent to the Profile/Application within a GattAscsServerConfigureCodecInd structure
 *         when the ASCS library receives a Configure Codec Operation from
 *         the client.
 *
 *         The Profile/Application does not need to retain these values for future use,
 *         they can be retrieved later from the ASCS library by calling the
 *         GattAscsReadCodecConfiguration() function.
 */
typedef struct
{
    /*! The id of the ASE id that is being configured with the fields in this structure.*/
    uint8                  aseId;
    /*! The 'direction' of the ASE characteristic that is being configured.
     *  The 'direction' is either sink or source with respect to the server. */
    GattAscsAseDirection   direction;
    /*! The Target Latency requested by the client during the Configure Codec Procedure */
    GattAscsTargetLatency  targetLatency;
    /*! The Target PHY requested by the client during the Configure Codec Procedure */
    GattAscsTargetPhy      targetPhy;
    /*! Uniquely identifies a codec to use for this ASE. The codec may be vendor defined or defined
     *  by the BT SIG */
    GattAscsCodecId        codecId;
    /*! The length, in octets, of the codec configuration data pointed to by 'codecConfiguration' */
    uint8                  codecConfigurationLength;
    /*! LTV formatted codec configuration data for this ASE.
     *  The ASCS library is responsible for managing the memory pointed to by 'codecConfiguration',
     *  the Profile/Application must not free the memory pointed to by 'codecConfiguration'.
     */
    uint8*                 codecConfiguration;
} GattAscsServerConfigureCodecClientInfo;

/*! @brief This structure is sent to the Profile/Application to indicate that a client has
 *         initiated the Configure Codec procedure (i.e. a client has sent a Configure Codec
 *         operation to the ASCS server). The Profile/Application must call
 *         GattAscsServerConfigureCodecResponse() in response to this indication.
 *         The Profile/Application does not need to store the information provided in
 *         this indication; the information can be retrieved at a later time by
 *         calling: GattAscsReadCodecConfiguration()
 */
typedef struct
{
    /*! The identifier used to distinguish between different messages sent from the ASCS library to
     *  the Profile/Application.
     *  The ASCS library sets this value to GATT_ASCS_SERVER_CONFIGURE_CODEC_IND. */
    GattAscsServerMessageId id;
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    /*! The number of ASES being configured during the Configure Codec Procedure */
    uint8        numAses;
    /*!
     *  The array of GattAscsServerConfigureCodecClientInfos has 'numAses' elements.
     *  There is one GattAscsServerConfigureCodecClientInfo per ASE.
     *
     *  The memory allocated to store the GattAscsServerConfigureCodecInd structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerConfigureCodecClientInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerConfigureCodecInd is greater than sizeof(GattAscsServerConfigureCodecInd).
     */
    GattAscsServerConfigureCodecClientInfo gattAscsServerConfigureCodecClientInfo[1];
} GattAscsServerConfigureCodecInd;

/*! @brief This structure is used to inform the ASCS library of the result (success or failure)
 *         of configuring an ASE in the indication: GATT_ASCS_SERVER_CONFIGURE_CODEC_IND.
 *         The ASCS library is responsible for freeing the memory pointed to by
 *         'gattAscsServerConfigureCodecServerInfo': this can either be valid dynamically
 *         allocated memory or NULL (e.g. NULL may be used in the case where gattAscsAseResult.value
 *         is not GATT_ASCS_ASE_RESULT_SUCCESS).
 *         Additionally, the ASCS library is responsible for freeing the memory pointed to by
 *         'gattAscsServerConfigureCodecServerInfo.codecConfiguration', the Profile/Application
 *         must not free this.
 */
typedef struct
{
    /*! The gattAscsAseResult field includes the aseId.
     *  This field reports the result of configuring
     *  the corresponding ASE after the Profile/Application receives the
     *  GattAscsServerConfigureCodecInd. */
    GattAscsAseResult gattAscsAseResult;
    /*! The codec configuration data provided by the Profile/Application. This
     *  codec configuration data is sent to the client during the Configure Codec procedure.
     *
     *  The ASCS Library takes ownership of (and ultimately frees) the memory pointed
     *  'gattAscsServerConfigureCodecServerInfo', the Profile/Application must not free
     *  this memory. Additionally, the ASCS library takes ownership of (and ultimately
     *  frees) the memory pointed to by:
     *  'gattAscsServerConfigureCodecServerInfo.codecConfiguration'
     *  To avoid the possibility of 'double freeing' memory, the Profile/Application must
     *  ensure that no two ASEs point to the same memory for either of:
     *    - gattAscsServerConfigureCodecServerInfo or
     *    - gattAscsServerConfigureCodecServerInfo.codecConfiguration
     *    .
     */
    GattAscsServerConfigureCodecServerInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerConfigureCodecRspAse;

/*!
 * @brief  The Profile/Application sends this response by calling GattAscsServerConfigureCodecResponse().
 *         The configure codec response must include the same set of ase ids that were included in the
 *         GattAscsServerConfigureCodecInd.
 *
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureCodecRspAses has 'numAses' elements.
     *  The Profile/Application allocates and must free the memory pointed to by the 'ase' field. */
    GattAscsServerConfigureCodecRspAse *ase;
} GattAscsServerConfigureCodecRsp;

/*!
 * @brief  This structure is used by the Profile/Application to set the codec configuration data
 *         for an individual ASE. It is used during a server initiated Configure Codec procedure.
 */
typedef struct
{
    /*! The aseId for which the codec configuration data is being set. */
    uint8 aseId;
    GattAscsCodecId   codecId;
    /*! The codec configuration data provided by the Profile/Application to be sent to
     *  the client during the server initiated Configure Codec procedure.
     *
     *  The ASCS Library takes ownership of (and ultimately frees) the memory pointed
     *  'gattAscsServerConfigureCodecServerInfo', the Profile/Application must not free
     *  this memory. Additionally, the ASCS library takes ownership of (and ultimately
     *  frees) the memory pointed to by:
     *  'gattAscsServerConfigureCodecServerInfo.codecConfiguration'
     *  To avoid the possibility of 'double freeing' memory, the Profile/Application must
     *  ensure that no two ASEs point to the same memory for either of:
     *    - gattAscsServerConfigureCodecServerInfo or
     *    - gattAscsServerConfigureCodecServerInfo.codecConfiguration
     *    .
     */
    GattAscsServerConfigureCodecServerReqInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerConfigureCodecReqAse;

/*!
 * @brief  The Profile/Application sends this request to initiate the Configure Codec procedure.
 *         This procedure informs the client of the codec parameters the server will use
 *         for a given set of ASEs.
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureCodecReqAses has 'numAses' elements
     *  The Profile/Application allocates and must free the memory pointed to by the 'ase' field. */
    GattAscsServerConfigureCodecReqAse *ase;
} GattAscsServerConfigureCodecReq;

typedef struct
{
    /*!  CIS_ID  1   Identifier for a CIS that has been assigned by the client host */
    uint8 cisId;
    /*!  CIG_ID  1   Identifier for a CIG that has been assigned by the client host */
    uint8 cigId;
    /*!  The RTN is a recommendation for the number of retransmissions a CIS Data
     *   PDU should be given. This is intended to select between different levels of stream reliability.
     *   It cannot be guaranteed that every CIS Data PDU will be given this number of retransmissions.
     *
     *   Range: 0x00 to 0x0F
     *
     *   All other values: RFU
     */
    uint8 retransmissionNumber;
    /*! The Framing parameter value written by the client for this ASE. */
    GattAscsFraming framing;
    /*! The PHY parameter value written by the client for this ASE. */
    GattAscsPhy phy;
    /*!  The Maximum SDU parameter value written by the client for this ASE.
     *
     *   Range: 0x00 to 0x0FFF (12 bits are meaningful)
     *
     *   All other values: RFU
     */
    uint16 maximumSduSize;
    /*!  The Maximum Transport Latency parameter value written by the client for this ASE in
     *   units of milliseconds.
     *
     *   The time range is 5 milliseconds to 4000 milliseconds.
     *
     *   Range: 0x0005 to 0x0FA0
     *
     *   All other values: RFU
     */
    uint16 maxTransportLatency;
    /*!  The SDU Interval parameter value written by the client for this ASE in units of [microseconds]
     *
     *   Range: 0x000000FF to 0x000FFFFF
     *
     *   All other values: RFU
     */
    uint32 sduInterval;
     /*! Presentation delay for this ASE in [microseconds]
      *
      *  Range: 0x00000000 to 0x00FFFFFF (24 bits are meaningful)
      */
    uint32 presentationDelay;
} GattAscsServerConfigureQosInfo;

typedef struct
{
    uint8 aseId;
    /*!
     * The ASCS library is responsible for allocating and deallocating the memory
     * used for the GattAscsServerConfigureQosInfo structure. The Profile/Application
     * must not free this memory.
     */
    GattAscsServerConfigureQosInfo* qosInfo;
} GattAscsServerConfigureQosInfoWithAseId;

typedef struct
{
    /*! The identifier used to distinguish between different messages sent from the ASCS library to
     *  the Profile/Application
     *  The ASCS library sets this value to GATT_ASCS_SERVER_CONFIGURE_QOS_IND */
    GattAscsServerMessageId id;
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId  cid;
    /*! The number of ASES being configured during the Configure QOS Procedure */
    uint8         numAses;
    /*!
     *  The array of GattAscsServerConfigureQosInfoWithAseIds has 'numAses' elements.
     *  There is one GattAscsServerConfigureQosInfoWithAseId per ASE.
     *
     *  The memory allocated to store the GattAscsServerConfigureQosInd structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerConfigureQosInfoWithAseIds (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerConfigureQosInd is greater than sizeof(GattAscsServerConfigureQosInd).
     *
     *  The Profile/Application does not need to retain the information given in this indication,
     *  if the information is needed at a later time it can retrieved by calling
     *  GattAscsReadQosConfiguration().
     *
     *  The Profile/Application must return the memory allocated for each
     *  GattAscsServerConfigureQosInfo structure to the ASCS library in the
     *  GattAscsServerConfigureQosResponse().
     *
     *  The Profile/Application must not modify the contents of the
     *  GattAscsServerConfigureQosIndInfo structure for any ASE; the values for an ASE must be either
     *  accepted or rejected by the Profile/Application.
     */
    GattAscsServerConfigureQosInfoWithAseId ase[1];
} GattAscsServerConfigureQosInd;

typedef struct
{
    /*! The gattAscsAseResult field includes the aseId. This field reports the result of the
     *  Profile/Application configuring the corresponding ASE in accordance with the
     *  configuration data in the GATT_ASCS_SERVER_CONFIGURE_QOS_IND. */
    GattAscsAseResult gattAscsAseResult;
    /*!
     *  The ASCS library takes ownership (and ultimately frees) the memory pointed to by
     *  'gattAscsServerConfigureQosRspInfo'. It is the same memory, with contents unchanged, that
     *  was passed to the Profile/Application in the GattAscsServerConfigureQosInfo field(s) within
     *  the GATT_ASCS_SERVER_CONFIGURE_QOS_IND */
    GattAscsServerConfigureQosInfo* gattAscsServerConfigureQosRspInfo;
} GattAscsServerConfigureQosRspAse;

/*!
 * @brief  The configure QOS response must include the same set of ase ids that were included in the
 *         GATT_ASCS_SERVER_CONFIGURE_QOS_IND.
 *         The Profile/Application sends this response by calling GattAscsServerConfigureQosResponse().
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureQosRspAses has 'numAses' elements
     *  The Profile/Application allocates and must free the memory pointed to by the 'ase' field. */
    GattAscsServerConfigureQosRspAse *ase;
} GattAscsServerConfigureQosRsp;

typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureQosInfoWithAseIds has 'numAses' elements.
     *
     *  The memory allocated to store the GattAscsServerConfigureQosReq structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerConfigureQosInfoWithAseIds (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerConfigureQosReq is greater than sizeof(GattAscsServerConfigureQosReq).
     *
     *  The ASCS library takes ownership of the memory pointed to by: 'ase[i].qosInfo', the
     *  Profile/Application must not free this memory.
     */
    GattAscsServerConfigureQosInfoWithAseId ase[1];
} GattAscsServerConfigureQosReq;

typedef struct
{
    /*! ASE_ID for this ASE. */
    uint8 aseId;
    uint8 cigId;
    uint8 cisId;
    uint8 metadataLength;
    /*! Metadata will contain LTV fields defined in BAP or by higher layer specifications.
        The ASCS library retains ownership of the memory pointed to by 'metadata';
        this memory must not be freed by the profile/app */
    uint8* metadata;
} GattAscsServerEnableIndInfo;

/*!
 *  @brief This indication is sent to the Profile/Application to indicate that the client is
 *         enabling the specified ASEs and to provide any metadata that is applicable to
 *         those ASEs.
 *
 *         The Profile/Application responds to this indication by calling GattAscsServerEnableResponse().
 */
typedef struct
{
    /*! The identifier used to distinguish between different messages sent from the ASCS library to
     *  the Profile/Application.
     *  The ASCS library sets this value to GATT_ASCS_SERVER_ENABLE_IND. */
    GattAscsServerMessageId id;
    /*! The identifier for the ACL connection on which ASEs are being enabled. */
    ConnectionId cid;
    /*! The number of ASES being Enabled */
    uint8        numAses;
    /*! The array of GattAscsServerEnableIndInfos has 'numAses' elements.
     *  There is one GattAscsServerEnableIndInfo per ASE
     *
     *  The memory allocated to store the GattAscsServerEnableInd structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerEnableIndInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerEnableInd is greater than sizeof(GattAscsServerEnableInd).
     */
    GattAscsServerEnableIndInfo gattAscsServerEnableIndInfo[1];
} GattAscsServerEnableInd;

typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array gattAscsAseResult has 'numAses' elements
    *  The Profile/Application allocates and must free the memory pointed to by the 'gattAscsAseResult' field. */
    GattAscsAseResult *gattAscsAseResult;
} GattAscsServerGenericRsp;

/*!
 *  @brief The Profile/Application sends this response to the ASCS library after receiving the
 *         GattAscsServerEnableInd. This response is sent by calling
 *         GattAscsServerEnableResponse().
 *
 *         This enable response includes the GattAscsAseResult, after validating the metadata
 *         for each ASE specified in the GATT_ASCS_SERVER_ENABLE_IND.
 */
typedef GattAscsServerGenericRsp GattAscsServerEnableRsp;

typedef struct
{
    uint8 aseId;
    uint8 metadataLength;
    /*! Metadata will contain LTV fields defined in BAP or by higher layer specifications.
        When used in an Update Metadata Indication the ASCS library retains ownership of the memory
        pointed to by 'metadata'; this memory must not be freed by the profile/app.
        When used in an Update Metadata Request the Profile/Application passes ownership of the memory
        pointed to by 'metadata'; this memory must not be freed by the profile/app. */
    uint8* metadata;
} GattAscsServerUpdateMetadataInfo;

typedef struct
{
    /*! The identifier for the ACL connection on which ASE metadata is being updated. */
    ConnectionId cid;
    uint8 numAses;
    /*! The array of GattAscsServerUpdateMetadataInfos has 'numAses' elements.
     *  There is one GattAscsServerUpdateMetadataInfo per ASE
     *
     *  The memory allocated to store the GattAscsServerUpdateMetadataReq structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerUpdateMetadataInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerUpdateMetadataReq is greater than
     *  sizeof(GattAscsServerUpdateMetadataReq).
     */
    GattAscsServerUpdateMetadataInfo updateMetadataReqInfo[1];
} GattAscsServerUpdateMetadataReq;

/*!
 *  @brief The ASCS library populates this structure within the API function:
 *         GattAscsServerUpdateMetadataRequest(), i.e. the confirmation is
 *         provided synchronously to the Profile/Application.
 */
typedef struct
{
    /*! The number of aseResults in this GattAscsServerUpdateMetadataCfm */
    uint8 numAses;
    /*! The array of aseResults has 'numAses' elements.
     *  There is one aseResult per ASE
     *
     *  The memory allocated to store the GattAscsServerUpdateMetadataCfm structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  aseResults (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerUpdateMetadataCfm is greater than
     *  sizeof(GattAscsServerUpdateMetadataCfm).
     */
    struct
    {
        uint8 aseId;
        /*! The result of update metadata request for an individual ASE. This value is TRUE if both
         *  of the following conditions are true:
         *     * the ASE Id is valid in the ASCS library and
         *     * the ASE is in either of the following states: ENABLING or STREAMING.
         *     .
         */
        bool value;
    } aseResult[1];
} GattAscsServerUpdateMetadataCfm;

typedef struct
{
    /*! The identifier used to distinguish between different messages sent from the ASCS library to
     *  the Profile/Application
     *  This structure is common to multiple client initiated operations. The ASCS library
     *  sets this value to correspond with the operation initiated by the client, it is set to one
     *  of the following:
     *    - GATT_ASCS_SERVER_DISABLE_IND
     *    - GATT_ASCS_SERVER_RECEIVER_READY_IND
     *    - GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND
     *    - GATT_ASCS_SERVER_RELEASE_IND
     *    .
     */
    GattAscsServerMessageId id;
    /*! The identifier for the ACL connection on which ASEs are being configured. */
    ConnectionId cid;
    /*! The number of aseIds that this indication is operating on */
    uint8 numAses;
    /*! The array of aseIds has 'numAses' elements.
     *
     *  The memory allocated to store the GattAscsServerGenericInd structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  uint8s (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerGenericInd is greater than sizeof(GattAscsServerGenericInd).
     */
    uint8 aseId[1];
} GattAscsServerGenericInd;

/*!
 *  @brief This indication is sent to the Profile/Application to indicate that the client
 *         is ready to receive audio on the specified ASE(s) and the Profile/Application can
 *         now start transmitting audio on those ASEs. This indication should only include
 *         ASEs for which the server is the source of audio data.
 *
 *         The Profile/Application responds to this indication by calling
 *         GattAscsReceiverReadyResponse(), after it has started transmitting audio data
 *         on the specified ASEs.
 */
typedef GattAscsServerGenericInd GattAscsServerReceiverReadyInd;

/*!
 *  @brief The Profile/Application sends this structure to the ASCS library by calling
 *         GattAscsReceiverReadyResponse(). Please refer to the function documentation
 *         for details of when this is function must be called.
 */
typedef GattAscsServerGenericRsp GattAscsServerReceiverReadyRsp;

/*!
 *  @brief The Profile/Application sends this structure to the ASCS library by calling
 *         GattAscsReceiverStopReadyResponse(). Please refer to the function documentation
 *         for details of when this is function must be called.
 */
typedef GattAscsServerGenericRsp GattAscsServerReceiverStopReadyRsp;

/*!
 *  @brief The Profile/Application sends this structure to the ASCS library by calling
 *         GattAscsReceiverReadyRequest(). Please refer to the function documentation
 *         for details of when this is function must be called.
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are ready to receive. */
    ConnectionId cid;
    /*! Total number of ASE_IDs used in the Enable operation */
    uint8 numAses;
    /*! The array aseId has 'numAses' elements
     *  The Profile/Application allocates and must free the memory pointed to by the 'aseId' field.*/
    uint8 *aseId;
} GattAscsServerReceiverReadyReq;

typedef struct
{
    /*! The identifier used to distinguish between different messages sent from the ASCS library to
     *  the Profile/Application.
     *  The ASCS library sets this value to GATT_ASCS_SERVER_UPDATE_METADATA_IND. */
    GattAscsServerMessageId id;
    /*! The identifier for the ACL connection on which ASE metadata is being updated. */
    ConnectionId cid;
    /*! Total number of GattAscsServerUpdateMetadataIndInfos in the Update Metadata operation */
    uint8 numAses;
    /*! There is one GattAscsServerUpdateMetadataInfo per ASE
     *
     *  The memory allocated to store the GattAscsServerUpdateMetadataInd structure is
     *  one contiguous block of memory that is large enough to store the appropriate number of
     *  GattAscsServerUpdateMetadataInfos (i.e. the number specified by the numAses field).
     *  This means that, if numAses is greater than 1, then the memory allocated to store
     *  the GattAscsServerUpdateMetadataInd is greater than sizeof(GattAscsServerUpdateMetadataInd).
     */
    GattAscsServerUpdateMetadataInfo gattAscsServerUpdateMetadataIndInfo[1];
} GattAscsServerUpdateMetadataInd;

/*!
 *  @brief The Profile/Application sends this response to the ASCS library after receiving the
 *         GATT_ASCS_SERVER_UPDATE_METADATA_IND and after processing the updated metadata for
 *         each ASE in that indication.
 *         This response is sent by calling GattAscsServerUpdateMetadataResponse().
 */
typedef GattAscsServerGenericRsp GattAscsServerUpdateMetadataRsp;

/*!
 *  @brief This indication is sent to the Profile/Application to indicate that the client
 *         is disabling the specified ASE(s).
 *
 *         The Profile/Application responds to this indication by calling GattAscsDisableResponse().
 *         The response must include the same set of ASE Ids as the indication.
 *
 *         The Profile/Application responds to this indication after it has stopped consuming
 *         audio data for all Sink ASEs (i.e. the Server is the Sink) specified in the indication.
 *
 *         After responding to this indication the Profile/Application continues to transmit
 *         audio data on all Source ASEs (i.e. the Server is the Source) specified in the
 *         indication until a subsequent GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND is received.
 */
typedef GattAscsServerGenericInd GattAscsServerDisableInd;

/*!
 *  @brief This indication is sent to the Profile/Application to indicate that the client
 *         is ready to stop consuming data.
 *
 *         This indication should only include ASEs for which the server is the source of
 *         audio data.
 *
 *         The Profile/Application responds to this indication by calling
 *         GattAscsReceiverStopReadyResponse() after it has stopped transmitting
 *         audio data for all Source ASEs specified in the indication.
 *         The response must include the same set of ASE Ids as the indication.
 */
typedef GattAscsServerGenericInd GattAscsServerReceiverStopReadyInd;

/*!
 *  @brief The Profile/Application sends this response to the ASCS library after receiving the
 *         GATT_ASCS_SERVER_DISABLE_IND and after it has stopped consuming audio data for all
 *         Sink ASEs (i.e. the Server is the Sink) specified in the indication.
 *
 *         This response must include the same set of ASE Ids as the indication.
 *
 *         This response is sent by calling GattAscsServerDisableResponse().
 *
 *         After calling GattAscsServerDisableResponse() the Profile/Application continues to
 *         transmit audio data on all Source ASEs (i.e. the Server is the Source) specified in the
 *         indication until a subsequent GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND is received.
 */
typedef GattAscsServerGenericRsp GattAscsServerDisableRsp;

/*!
 *  @brief This indication is sent by the ASCS library to inform the Profile/Application that all
 *         resources associated with the specified ASEs can be released. After releasing all resources
 *         associated with the specified ASEs, the Profile/Application responds to the indication
 *         by calling GattAscsServerReleaseCompleteRequest().
 */
typedef GattAscsServerGenericInd GattAscsServerReleaseInd;

/*! @brief  This structure is used within the GattAscsServerReleaseComplete structure that is used to
 *          complete the release procedure.
 *
 *  When completing the release procedure the Profile/Application can choose to cache codec configuration
 *  for any of the released ASEs. If caching code configuration for any ASEs the Profile/Application can
 *  choose to:
 *     -# cache a 'preferred' codec configuration, which may differ from the codec configuration
 *        currently used by the ASE(s) being released.
 *     -# cache codec configuration that is the same as that currently used by the ASEs being released.
 *     .
 *
 *  If the Profile/Application caches a 'preferred' codec configuration (that differs from the
 *  codec configuration currently in use by an ASE being released) then that preferred configuration is
 *  pointed to by 'gattAscsServerConfigureCodecServerInfo'.
 *
 *  If the Profile/Application chooses to cache the codec configuration currently in use
 *  by an ASE being released, or if the Profile/Application chooses not to cache codec
 *  configuration, then the 'gattAscsServerConfigureCodecServerInfo' pointer is
 *  set to NULL.
 *
 *  The ASCS library takes ownership of and ultimately frees the memory pointed to by
 *  'gattAscsServerConfigureCodecServerInfo'.
 *  Additionally the ASCS library takes ownership of and ultimately frees, the memory
 *  pointed to by: gattAscsServerConfigureCodecServerInfo.codecConfiguration
 *
 *  To avoid the possibility of 'double freeing' memory, the Profile/Application must
 *  ensure that no two ASEs point to the same memory for either of:
 *    - gattAscsServerConfigureCodecServerInfo or
 *    - gattAscsServerConfigureCodecServerInfo.codecConfiguration
 */
typedef struct
{
    /*! The id of the ASE being released*/
    uint8 aseId;
    /*! If the Profile/Application wants codec configuration to be cached for
     *   the ASEs being released then it sets cacheCodecConfiguration
     *   to TRUE, otherwise it is set to FALSE. */
    bool cacheCodecConfiguration;
    /*! The ASCS library takes ownership of and ultimately frees this memory.
     *  Additionally the ASCS library takes ownership of and ultimately frees, the memory
     *  pointed to by: gattAscsServerConfigureCodecServerInfo.codecConfiguration.
     *  If 'cacheCodecConfiguration' is TRUE, then 'gattAscsServerConfigureCodecServerInfo'
     *  must either be NULL or point to valid memory that can be freed. */
    GattAscsServerConfigureCodecServerInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerReleaseCompleteAse;

/*!
 *  @brief This structure is used to inform the ASCS library that the Profile/Application
 *         has released all resources associated with the specified ASEs.
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being released. */
    ConnectionId cid;
    /*! The number of ASEs for which the release procedure has completed. */
    uint8        numAses;
    /*! The array of GattAscsServerReleaseCompleteAses contains 'numAses' elements.
     *  The Profile/Application allocates and must free the memory pointed to by the 'ase' field. */
    GattAscsServerReleaseCompleteAse *ase;
} GattAscsServerReleaseComplete;

/*!
 *  @brief This structure is sent from the Profile/Application to the ASCS library
 *         to start a server initiated release procedure for the specified ASEs.
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being released. */
    ConnectionId cid;
    /*! The number of ASEs being released in this procedure */
    uint8        numAses;
    /*! The array of uint8s contains 'numAses' elements
     *  The Profile/Application allocates and must free the memory pointed to by the 'aseId' field. */
    uint8        *aseId;
} GattAscsServerReleaseReq;

/*!
 *  @brief This structure is sent from the Profile/Application to the ASCS library
 *         to initiate the disable procedure for the specified ASEs.
 */
typedef struct
{
    /*! The identifier for the ACL connection on which ASEs are being disabled. */
    ConnectionId cid;
    uint8        numAses;
    /*! The array of uint8s contains 'numAses' elements
     *  The Profile/Application allocates and must free the memory pointed to by the 'aseId' field. */
    uint8        *aseId;
} GattAscsServerDisableReq;

typedef struct
{
    uint8         numAses;
    ClientConfig  aseControlPointCharClientCfg;
    ClientConfig  *aseCharClientCfg;
} GattAscsClientConfig;

typedef struct
{
    uint8         numAses;
    uint8         aseIds[MAX_ASE_ID_PER_CIS];
} GattAscsServerReleasingAseInfo;

/*! @brief Contents of the GATT_ASCS_SERVER_CONFIG_CHANGE_IND message that is sent by the library,
    when any CCCD is toggled by the remote client.
 */
typedef struct 
{
    GattAscsServerMessageId id;
    ConnectionId cid;
    bool configChangeComplete; /* will be TRUE if all CCCD of ASCS are written once */
} GattAscsServerConfigChangeInd;

/*!
    @brief Initialises the Audio Stream Control Service library.

    @param theAppTask The Task that will receive the messages sent to this Audio Stream Control Service library.
    @param startHandle This indicates the start handle of the Audio Stream Control Service
    @param endHandle This indicates the end handle of the Audio Stream Control Service

    @return The service handle for the ASCS instance.

*/
ServiceHandle GattAscsServerInit(AppTask theAppTask, uint16 startHandle, uint16 endHandle);

/*!
    @brief Complete the ASE Enable procedure

    @param serviceHandle  The service handle of the ASCS instance.
    @param enableResponse A pointer to a set of result values, one per ASE being enabled.
                          Note: The ASCS library does not free the memory pointed to by
                          this parameter.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_ENABLE_IND,
    after validating the metadata for each ASE specified in that indication.

    This response must include the same set of ASE Ids as the indication GATT_ASCS_SERVER_ENABLE_IND.
*/
void GattAscsServerEnableResponse(ServiceHandle serviceHandle, GattAscsServerEnableRsp* enableResponse);

/*!
    @brief Complete the QOS Configuration procedure

    @param serviceHandle The service handle of the ASCS instance.
    @param configureQosResponse   A pointer to the set of QOS configuration values and ranges
                                  that the server can support for each ASE.
                                  Note: The ASCS library does not free the memory pointed to by
                                  this parameter.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_CONFIGURE_QOS_IND,
    after validating the QOS Configuration for each ASE specified in that indication.

    This response must include the same set of ASE Ids as the indication GATT_ASCS_SERVER_CONFIGURE_QOS_IND.

*/
void GattAscsServerConfigureQosResponse(ServiceHandle serviceHandle, GattAscsServerConfigureQosRsp* configureQosResponse);

/*!
    @brief Complete the Codec Configuration procedure

    @param serviceHandle The service handle of the ASCS instance.
    @param configureCodecResponse A pointer to the set of codec configuration values and ranges
                                  that the server can support for each ASE.
                                  Note: The ASCS library does not free the memory pointed to by
                                  this parameter.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_CONFIGURE_CODEC_IND,
    after validating the Codec Configuration for each ASE specified in that indication.

    This response must include the same set of ASE Ids as the GATT_ASCS_SERVER_CONFIGURE_CODEC_IND.

*/
void GattAscsServerConfigureCodecResponse(ServiceHandle serviceHandle, GattAscsServerConfigureCodecRsp* configureCodecResponse);

/*!
    @brief Complete the ASE Update Metadata procedure

    @param serviceHandle The service handle of the ASCS instance.
    @param updateMetadataResponse A pointer to a set of GattAscsAseResult values. There is one
                                  GattAscsAseResult value for each ASE affected by the Update
                                  Metadata procedure.
                                  Note: The ASCS library does not free the memory pointed to by
                                  this parameter.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_UPDATE_METADATA_IND,
    after validating the metadata for each ASE specified in that indication.

    This response must include the same set of ASE Ids as the GATT_ASCS_SERVER_UPDATE_METADATA_IND.
*/
void GattAscsServerUpdateMetadataResponse(ServiceHandle serviceHandle, GattAscsServerUpdateMetadataRsp* updateMetadataResponse);

/*!
    @brief Complete the ASE Disable procedure

    @param serviceHandle The service handle of the ASCS instance.
    @param disableResponse A pointer to a set of GattAscsAseResult values. There is one
                           GattAscsAseResult value for each ASE affected by the Disable
                           procedure.
                           Note: The ASCS library does not free the memory pointed to by
                           this parameter.

    The Profile/Application sends this response to the ASCS library after receiving the
    GATT_ASCS_SERVER_DISABLE_IND and after it has stopped consuming audio data for all
    Sink ASEs (i.e. the Server is the Sink) specified in the indication.

    After calling GattAscsServerDisableResponse() the Profile/Application continues to
    transmit audio data on all Source ASEs (i.e. the Server is the Source) specified in the
    indication until a subsequent GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND is received.

    This response must include the same set of ASE Ids as the GATT_ASCS_SERVER_DISABLE_IND.
*/
void GattAscsServerDisableResponse(ServiceHandle serviceHandle, GattAscsServerDisableRsp* disableResponse);

/*!
    @brief Initiate the ASE Configure Codec procedure from the Server

    @param serviceHandle The service handle of the ASCS instance.
    @param configureCodecRequest A pointer to a GattAscsServerConfigureCodecReq structure
                                 that describes the ASEs to be taken to the codec configured state
                                 and the codec configuration data for each of those ASEs.
*/
void GattAscsServerConfigureCodecRequest(ServiceHandle serviceHandle, GattAscsServerConfigureCodecReq* configureCodecRequest);

/*!
    @brief Initiate the ASE Configure QOS procedure from the Server

    @param serviceHandle The service handle of the ASCS instance.
    @param configureQosRequest A pointer to a GattAscsServerConfigureQosReq structure
                               that describes the ASEs to be taken to the QOS configured state
                               and the QOS configuration data for each of those ASEs.

*/
void GattAscsServerConfigureQosRequest(ServiceHandle serviceHandle, GattAscsServerConfigureQosReq* configureQosRequest);

/*!
    @brief Initiate the Update Metadata procedure from the Server.
           The Profile/Application calls this function to initiate the Update Metadata Procedure.

    @param serviceHandle     The service handle of the ASCS instance.
    @param updateMetadataReq A pointer to a GattAscsServerUpdateMetadataReq structure
                             that specifies the ASE Ids affected by this request and
                             the updated metadata for those ASEs.
*/
void GattAscsServerUpdateMetadataRequest(ServiceHandle serviceHandle,
                                         GattAscsServerUpdateMetadataReq* updateMetadataReq);
/*!
    @brief Initiate the ASE Disable procedure from the Server
           The Profile/Application calls this function to initiate disabling ASEs.

    @param serviceHandle  The service handle of the ASCS instance.
    @param disableRequest A pointer to a GattAscsServerDisableReq structure
                          that describes the ases to disable and the ACL connection they are on.
    @param cisLoss This flag should be set to true when the CIS is lost.

    If the server is the sink for any of the specified ASEs then the Profile/Application must already
    have stopped consuming audio data for those ASEs before calling this function.

    If the server is the source for any of the specified ASEs then the Profile/Application must
    continue transmitting on those ASEs until it receives a GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND.
*/
void GattAscsServerDisableRequest(ServiceHandle serviceHandle, GattAscsServerDisableReq* disableRequest, bool cisLoss);

/*!
    @brief Inform the ASCS library that the ASE(s) can be released and that the underlying CIS has been
           cleared down

    @param serviceHandle The service handle of the ASCS instance.
    @param releaseComplete A pointer to a GattAscsServerReleaseComplete structure comprising (for each ASE
                           being released):
                               - A result value (e.g. GATT_ASCS_ASE_RESULT_SUCCESS).
                               - Optional 'preferred codec configuration' data (details on the usage of
                                 this field, and memory ownership, can be found in the documentation for
                                 GattAscsServerReleaseComplete).

    This function is called either:
       - autonomously by the Profile/Application, e.g. if the server detects a loss of the LE-ACL connection or
       - in response to an GATT_ASCS_SERVER_RELEASE_IND.

    When this function is called autonomously, the Profile/Application must call GattAscsServerReleaseRequest() first.

    This function should only be called when either:
      - all resources have been released and the underlying CIS has been cleared down for the specified ASEs.
      - the server has detected the loss of the LE-ACL for the specified ASEs
*/
void GattAscsServerReleaseCompleteRequest(ServiceHandle serviceHandle, GattAscsServerReleaseComplete* releaseComplete);

/*!
    @brief Initiate the ASE Release procedure from the Server

    @param serviceHandle The service handle of the ASCS instance.
    @param releaseRequest A pointer to a GattAscsServerReleaseReq structure
                          that describes the ASEs to release and the ACL connection they are on.

    The Profile/Application calls this function to begin the Server initiated release procedure for the
    specified ASEs. When initiating the release procedure from the server side, the Profile/Application
    calls this function before calling GattAscsServerReleaseCompleteRequest().
*/
void GattAscsServerReleaseRequest(ServiceHandle serviceHandle, GattAscsServerReleaseReq* releaseRequest);

/*!
    @brief Initiate the Receiver Ready procedure - this function is used exclusively for Sink ASEs

    @param serviceHandle The service handle of the ASCS instance.
    @param receiverReadyRequest A pointer to a GattAscsServerReceiverReadyReq structure
                                that describes the ASEs on the server for which audio data can now be received.

    The Profile/Application calls this function to inform the client that it (the server) is now ready to receive
    audio data on the specified ASEs.

    This function is used exclusively for Sink ASEs (i.e. the Server is the Sink).
*/
void GattAscsReceiverReadyRequest(ServiceHandle serviceHandle, GattAscsServerReceiverReadyReq* receiverReadyRequest);

/*!
    @brief Complete the Receiver Ready procedure - this function is used exclusively for Source ASEs.

    @param serviceHandle The service handle of the ASCS instance.
    @param receiverReadyResponse A pointer to a GattAscsServerReceiverReadyRsp structure
                                 that describes the ASEs on which the server is now transmitting audio data.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_RECEIVER_READY_IND.

    This function includes the result of setting up the audio path for the specified Source ASEs.

    This response must include the same set of ASE Ids as the GATT_ASCS_SERVER_RECEIVER_READY_IND.
*/
void GattAscsReceiverReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverReadyRsp* receiverReadyResponse);

/*!
    @brief Complete the Receiver Stop Ready Procedure - this function is used exclusively for Source ASEs

    @param serviceHandle The service handle of the ASCS instance.
    @param receiverStopReadyResponse A pointer to a GattAscsServerReceiverStopReadyRsp structure
                                     that describes the ASEs for which the server is no longer transmitting
                                     audio data.

    The Profile/Application calls this function in response to a GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND.

    Profile/Application calls this function after it has stopped transmitting audio data on the ASEs
    specified in the GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND.

    This response must include the same set of ASE Ids as the GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND.
*/
void GattAscsReceiverStopReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverStopReadyRsp* receiverStopReadyResponse);

/*!
    \brief Remove the configuration for a peer device, identified by its
           ACL Connection Id.

    The Profile/Application calls this functions to remove the configuration
    for a peer device from the ASCS library, freeing the resources used for
    that configuration. This function should only be called when the peer
    device is disconnecting the ACL Connection.

    \param serviceHandle The service handle of the ASCS instance.
    \param cid The identifier for the ACL connection to the peer device.

    \return GattAscsClientConfig Pointer to the peer device configuration
            data. It is the application's responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
*/
GattAscsClientConfig* GattAscsRemoveConfig(ServiceHandle serviceHandle, ConnectionId cid);
/*!
    \brief Add configuration for a previously paired peer device, identified by its
           ACL Connection Id.

    \param serviceHandle The service handle of the ASCS instance.
    \param cid The identifier for the ACL connection to the peer device.
    \param config The client characteristic configurations for this connection.
                  A NULL value indicates that a default configuration should be
                  used for the peer device.
    \return status_t The status of the Add Configuration operation. For example,
           if memory cannot be allocated for the added configuration, the status
           status_insufficient_resources could be returned.
*/
status_t GattAscsAddConfig(ServiceHandle serviceHandle, ConnectionId cid, const GattAscsClientConfig* config);

/*!
    \brief Gets the configuration for a peer device, identified by its
           Connection ID.

    This gets the configuration for that peer device from the
    service library.
    It is recommnded to call this API after GATT_ASCS_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    \param srvcHndl The GATT service instance handle.
    \param cid A Connection ID for the peer device.

    \return GattAscsClientConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the connection_id_t is not found, the function will return NULL.
*/

GattAscsClientConfig* GattAscsServerGetConfig(ServiceHandle srvcHndl, ConnectionId cid);

/*!
    @brief An API to retrieve the Codec Configuration of an ASE.

    @param serviceHandle The service handle of the ASCS instance.
    @param cid  The identifier for the ACL connection on which the ASE is configured.
    @param aseId  The id of the ASE from which the codec configuration is to be retrieved.
    @return A pointer to a GattAscsServerConfigureCodecInfo structure for an ASE.
            This structure contains the codec configuration data provided by both the
            server and the client during the codec configuration procedure.
*/
GattAscsServerConfigureCodecInfo* GattAscsReadCodecConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId);
/*!
    @brief An API to retrieve the direction of an ASE.

    @param serviceHandle The service handle of the ASCS instance.
    @param cid  The identifier for the ACL connection on which the ASE is configured.
    @param aseId The id of the ASE for which the direction is being requested.
    @return The direction of the specified ASE, the direction is either:
              - GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK or
              - GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE.
*/
GattAscsAseDirection GattAscsReadAseDirection(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId);
/*!
    @brief An API to retrieve the QOS Configuration of an ASE.

    @param serviceHandle The service handle of the ASCS instance.
    @param cid  The identifier for the ACL connection on which the ASE is configured.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
    @return A pointer to a GattAscsServerConfigureQosInfo structure for an ASE.
            This structure contains the QOS configuration data provided by the
            client during the QOS configuration procedure.
*/
GattAscsServerConfigureQosInfo* GattAscsReadQosConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId);

/*!
    @brief An API to retrieve the ASE Ids which are in Releasing state associated with a CIS.

    @param serviceHandle The service handle of the ASCS instance.
    @param cid  The identifier for the ACL connection on which the ASE is configured.
    @param cisId  The id of the CIS for which ASE ID's is to be retrieved.
    @aseIds aseIds  A pointer to AseIds to be filled for matching CIS id. This pointer
                    can be allocated max size of 2 which is the max ASE ids can be linked.

    @return A pointer to a GattAscsServerReleasingAseInfo structure of matching ASE Id's.
            This structure contains the number of ASE Id's matched for the CIS ID.
            This pointer to be freed by the caller.
*/
GattAscsServerReleasingAseInfo*  GattAscsReadReleasingAseIdsByCisId(ServiceHandle serviceHandle, ConnectionId cid, uint8 cisId);

/*!
    @brief An API to retrieve the ASE data as per current ASE state of a given ASE Id.

    @param serviceHandle The service handle of the ASCS instance.
    @param cid  The identifier for the ACL connection on which the ASE is configured.
    @param aseId The id of the ASE for which the direction is being requested.
    @param dataLength length of the ASE data being returned. Upper layer
                      should initialize this value to 0 before passing.

    @return pointer to data whose content is same as notifications sent as part of
            Unicast operations if ASE ID is valid else NULL.
            It is the caller responsibility to free the pointer memory.
*/
uint8* GattAscsServerGetAseData(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId, uint8* dataLength);


#ifdef INSTALL_ASCS_NOTIFY_CB

/*!
    @brief Callback function registered using GattAscsServerRegisterCallback().

    @param cid  The identifier for the ACL connection of remote unicast client.
    @param aseId The ASE ID for which notificaion 'param'to be notified.
    @param param A pointer to the ASE characteristics values to be notified.

*/
typedef void (*GattAscsAseNtfHook)(ConnectionId cid, uint8_t aseId, void* param);

/*!
    @brief An API to update ASE notification parameter values for an ASE Id
    before sending it to remote Unicast Client.

    @param aseNotificationCb callback funnction.

*/
void GattAscsServerRegisterCallback( GattAscsAseNtfHook aseNotificationCb);
#endif /* INSTALL_ASCS_NOTIFY_CB */

#endif /* GATT_ASCS_SERVER_H */

