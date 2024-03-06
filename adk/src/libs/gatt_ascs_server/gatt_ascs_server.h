/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    
@brief   Header file for the GATT Audio Stream Control Service library.

        This file provides documentation for the GATT ASCS library
        API (library name: gatt_ascs_server).
*/

#ifndef GATT_ASCS_SERVER_H
#define GATT_ASCS_SERVER_H

#include <message.h>
#include <library.h>

#include "gatt_manager.h"
#include "service_handle.h"

typedef uint16 ConnectionId;
typedef uint16 ClientConfig;

#define GATT_ASCS_CLIENT_CONFIG_NOT_SET                 (0x00)
#define GATT_ASCS_CLIENT_CONFIG_NOTIFY                  (0x01)
#define GATT_ASCS_CLIENT_CONFIG_INDICATE                (0x02)

typedef uint8 GattAscsAseDirection;
/*! Uninitialised direction */
#define GATT_ASCS_ASE_DIRECTION_SERVER_UNINITIALISED    (0x00)
/*! Describes an ASE on which audio data travels from the client to the server */
#define GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK    (0x01)
/*! Describes an ASE on which audio data travels from the server to the client */
#define GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE  (0x02)

typedef uint8 GattAscsAseResultValue;
#define GATT_ASCS_ASE_RESULT_SUCCESS                                     (0x00)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_AUDIO_CAPABILITIES              (0x05)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE   (0x06)
#define GATT_ASCS_ASE_RESULT_REJECTED_CONFIGURATION_PARAMETER_VALUE      (0x07)
#define GATT_ASCS_ASE_RESULT_INVALID_CONFIGURATION_PARAMETER_VALUE       (0x08)
#define GATT_ASCS_ASE_RESULT_UNSUPPORTED_METADATA                        (0x09)
#define GATT_ASCS_ASE_RESULT_REJECTED_METADATA                           (0x0A)
#define GATT_ASCS_ASE_RESULT_INVALID_METADATA                            (0x0B)
#define GATT_ASCS_ASE_RESULT_INSUFFICIENT_RESOURCES                      (0x0C)
#define GATT_ASCS_ASE_RESULT_UNSPECIFIED_ERROR                           (0x0D)

typedef uint8 GattAscsAseResultAdditionalInfo;
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED                                    (0x00)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_DIRECTION                            (0x01)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_ID                             (0x02)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_CODEC_SPECIFIC_CONFIGURATION         (0x03)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_SDU_INTERVAL                         (0x04)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING                              (0x05)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PHY                                  (0x06)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAXIMUM_SDU_SIZE                     (0x07)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_RETRANSMISSION_NUMBER                (0x08)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_MAX_TRANSPORT_LATENCY                (0x09)
#define GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_PRESENTATION_DELAY                   (0x0A)

typedef uint8 GattAscsFraming;
#define GATT_ASCS_FRAMING_UNFRAMED_ISOAL_PDUS       (0x00)
#define GATT_ASCS_FRAMING_FRAMED_ISOAL_PDUS         (0x01)

typedef uint8 GattAscsCodecConfiguredFraming;
#define GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_SUPPORTED       (0x00)
#define GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_NOT_SUPPORTED   (0x01)

/*!
*  @brief The set of PHYs preferred by the server.
*         The server shall not specify a PHY that it does not support.
*         Values are bitwise ORed together to specify that multiple PHYs
*         are supported.
*/
typedef uint8 GattAscsPhy;
/*! 1 Mbps PHY */
#define GATT_ASCS_PHY_1Mbps             (0x01)
/*! 2 Mbps PHY */
#define GATT_ASCS_PHY_2Mbps             (0x02)
/*! LE coded PHY */
#define GATT_ASCS_PHY_LE_CODED_PHY      (0x04)

/*! @brief The target PHY requested by the client during a codec configuration operation.
 */
typedef uint8 GattAscsTargetPhy;
/*! 1 Mbps PHY */
#define GATT_ASCS_TARGET_PHY_LE_1M_PHY       (0x01)
/*! 2 Mbps PHY */
#define GATT_ASCS_TARGET_PHY_LE_2M_PHY       (0x02)
/*! LE coded PHY */
#define GATT_ASCS_TARGET_PHY_LE_CODED_PHY    (0x03)

/*! @brief The target latency requested by the client during a codec configuration operation.
 */
typedef uint8 GattAscsTargetLatency;
/*! Target lower latency in preference to high reliability */
#define GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY                       (0x01)
/*! Target a balance between latency and reliability */
#define GATT_ASCS_TARGET_LATENCY_TARGET_BALANCED_LATENCY_AND_RELIABILITY    (0x02)
/*! Target high reliability in preference to low latency */
#define GATT_ASCS_TARGET_LATENCY_TARGET_HIGHER_RELIABILITY                  (0x03)

typedef struct
{
    uint8 aseId;
    GattAscsAseResultValue value;
    /*!
     * The additionalInfo field can convey useful information when the profile/app detects an error in a metadata field
     * or in a parameter received from ASCS. This value can be set to either:
     *      -# the defined GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_XXX value that corresponds the the parameter that has an error
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
     *  A value of 0xFF specifies a Vendor Specific coding format.*/
    uint8 codingFormat;
    /*! For Vendor Specific coding formats this field specifies the Company ID as defined in Bluetooth Assigned Numbers.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be 0x0000. */
    uint16 companyId;
    /*! For Vendor Specific coding formats this field specifies the Vendor specific Codec Id.
     *
     *  For coding formats listed in the Bluetooth Assigned Numbers this field shall be 0x0000. */
    uint16 vendorSpecificCodecId;
} GattAscsCodecId;

#define GATT_ASCS_CODEC_ID_LEN (5)
#define GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT      (0xFF)
#define GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_COMPANY_ID     (0x0000)
#define GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_CODEC_ID       (0x0000)

/*!
    @brief The Profile/Application populates this structure and sends it to the ASCS library by calling
           GattAscsServerConfigureCodecResponse()
           This structure is used in the response to The fields in this structure are populated by the profile/application and
           sent to the ASCS library
           within a GattAscsServerConfigureCodecRsp structure
            response to an GATT_ASCS_SERVER_CONFIGURE_CODEC_IND.
*/
typedef struct
{
    /*! Maximum number of times that every CIS Data PDU should be retransmitted. This is a recommendation
     *  only which is meant to select between different levels of stream reliability.
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
    /*! Maximum size in octets of the Host's payload on this ASE.
     *
     *  Range: 0x0000 to 0x0FFF (12 bits meaningful)
     *
     *  All other values: RFU */
    uint16 maximumSduSize;
    /*! Maximum supported transport latency.
     *  The maximum time in milliseconds between first transmission of SDU on this ASE, to the end of
     *  the last attempt to receive the SDU on the far end of the ASE.
     *
     *  The time range is 5 milliseconds to 4000 milliseconds, and typical values are between
     *  7.5 milliseconds to 90 milliseconds.
     *
     *  Range: 0x0005 to 0x0FA0
     *
     *  All other values: RFU
     */
    uint16 transportLatencyMax;
    /*! Minimum supported presentation delay in [microseconds]
     *
     *  Range: 0x00000000 to 0xFFFFFFFF */
    uint32 presentationDelayMin;
    /*! Maximum supported presentation delay in [microseconds]
     *
     *  Range: 0x00000000 to 0xFFFFFFFF */
    uint32 presentationDelayMax;
    uint8  codecConfigurationLength;
    /*! The ASCS service is responsible for freeing the memory pointed to by 'codecConfiguration' */
    uint8* codecConfiguration;
} GattAscsServerConfigureCodecServerInfo;

/*! @brief This structure contains all the Codec Configuration data collected
 *        from both the client and the Profile/Application throughout the Codec
 *        Configuration procedure.
 *        This structure can be retrieved by the Profile/Application by calling the
 *        GattAscsReadCodecConfiguration() function.
 *        The structure can be requested by the Profile/Application at any time
 *        after the ASE has completed the Codec Configuration procedure (and
 *        before the ASE is released back to an IDLE state).
 */
typedef struct
{
    /*! The ASE direction provided by the client during the Configure Codec Procedure */
    GattAscsAseDirection   direction;
    /*! The Codec ID provided by the client during the Configure Codec Procedure */
    GattAscsCodecId        codecId;
    /*! The target latency provded by the client during the Configure Codec Procedure */
    GattAscsTargetLatency  targetLatency;
    /*! The target PHY provided by the client during the Configure Codec Procedure */
    GattAscsTargetPhy      targetPhy;
    /*! The Codec Configuration data provided by the Profile/Application
     *  during the Configure Codec Procedure */
    GattAscsServerConfigureCodecServerInfo infoFromServer;
} GattAscsServerConfigureCodecInfo;

/*! @brief This structure contains all the information provided by the client for a particular
 *         ASE during the Codec Configuration procedure. These structures are sent to the
 *         Profile/Application when the ASCS library receives a Configure Codec Operation from
 *         the client. These structures (one per ASE) are sent to the Profile/Application
 *         in a GattAscsServerConfigureCodecInd structure.
 *
 *         The profile/application does not need to retain these values for future use;
 *         they can be retrieved later from the ASCS library by calling the
 *         GattAscsReadCodecConfiguration() function.
 */
typedef struct
{
    /*! The ASE id that is being configured by the fields in this structure.*/
    uint8                  aseId;
    /*! The ASE direction provided by the client during the Configure Codec Procedure */
    GattAscsAseDirection   direction;
    /*! The Target Latency requested by the client during the Configure Codec Procedure */
    GattAscsTargetLatency  targetLatency;
    /*! The Target PHY requested by the client during the Configure Codec Procedure */
    GattAscsTargetPhy      targetPhy;
    /*! Uniquely identifies a codec to use for this ASE. The codec may be vendor defined or defined
     *  by the BT SIG */
    GattAscsCodecId        codecId;
    uint8                  codecConfigurationLength;
    /*! LTV formatted codec configuration data for this ASE.
     *  The ASCS service is responsible for managing the memory pointed to by 'codecConfiguration',
     *  the profile/application must not free the memory pointed to by 'codecConfiguration'.
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
    /*! The connection on which ASEs are being configured. */
    ConnectionId cid;
    /*! The number of ASES being configured during the Configure Codec Procedure */
    uint8        numAses;
    /*!
     *  The array of GattAscsServerConfigureCodecClientInfo s is 'numAses' in size.
     *  There is one GattAscsServerConfigureCodecClientInfo per ASE.
     *  The ASCS service is responsible for maintaining the memory referenced by
     *  'gattAscsServerConfigureCodecClientInfo',
     *  the profile/application must not free this memory.
     */
    GattAscsServerConfigureCodecClientInfo gattAscsServerConfigureCodecClientInfo[];
} GattAscsServerConfigureCodecInd;

/*! @brief This structure is used to inform the ASCS service of the result (success or failure)
 *         of configuring each ASE in the indication: GattAscsServerConfigureCodecInd.
 *         There is one GattAscsServerConfigureCodecServerInfo per ASE.
 *         Multiple ASEs may point to the same GattAscsServerConfigureCodecServerInfo instance.
 *         The ASCS service is responsible for freeing the memory pointed to by
 *         'gattAscsServerConfigureCodecServerInfo': this can either be valid dynamically
 *         allocated memory or NULL (e.g. NULL may be used in the case where gattAscsAseResult.value
 *          is not GATT_ASCS_ASE_RESULT_SUCCESS). */
typedef struct
{
    /*! The gattAscsAseResult field includes the aseId. This field reports the result of configuring
     *  the corresponding ASE after the Profile/Application receives the
     *  GattAscsServerConfigureCodecInd. */
    GattAscsAseResult gattAscsAseResult;
    /*! The codec configuration data provided by the Profile/Application. This
     *  codec configuration data is sent to the client during the Configure Codec procedure.
     *
     *  The Profile/Application may choose to have one instance of GattAscsServerConfigureCodecServerInfo
     *  referenced by multiple ASEs.
     */
    GattAscsServerConfigureCodecServerInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerConfigureCodecRspAse;

/*!
 * @brief  The profile/application sends this response by calling GattAscsServerConfigureCodecResponse().
 *         The configure codec response must include the same set of ase ids that were included in the
 *         GattAscsServerConfigureCodecInd.
 *
 */
typedef struct
{
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureCodecRspAses is 'numAses' in size */
    GattAscsServerConfigureCodecRspAse ase[];
} GattAscsServerConfigureCodecRsp;

/*!
 * @brief  This structure details the codec configuration data, provided by the Profile/Application,
 *         for an individual ASE.
 */
typedef struct
{
    /*! The aseId for which the codec configuration data is being set. */
    uint8 aseId;
    /*! The codec configuration data provided by the Profile/Application to be sent to
     *  the client during the Configure Codec procedure.
     *
     *  The Profile/Application may choose to have one instance of GattAscsServerConfigureCodecServerInfo
     *  referenced by multiple ASEs.
     */
    GattAscsServerConfigureCodecServerInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerConfigureCodecReqAse;

/*!
 * @brief  The profile/application sends this request to initiate the Configure Codec procedure.
 *         This procedure informs the client of the codec parameters the server will use
 *         for a given set of ASEs.
 */
typedef struct
{
    ConnectionId cid;
    uint8        numAses;
    /*! The array of GattAscsServerConfigureCodecReqAses is 'numAses' in size */
    GattAscsServerConfigureCodecReqAse ase[];
} GattAscsServerConfigureCodecReq;

typedef struct
{
    uint8 aseId;
    /*!  CIS_ID  1   Identifier for a CIS that has been assigned by the client host                                 */
    uint8 cisId;
    /*!  CIG_ID  1   Identifier for a CIG that has been assigned by the client host                                 */
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
    GattAscsFraming framing;
    /*! The PHY parameter value selected by the client for this ASE.                         */
    GattAscsPhy phy;
    /*!  Maximum size in octets of the Host's payload on this ASE
     *
     *   Range: 0x00 to 0x0FFF (12 bits meaningful)
     *
     *   All other values: RFU
     */
    uint16 maximumSduSize;
    /*!  The time in milliseconds between first transmission of an SDU on this ASE, to the end of
     *   the last attempt to receive that SDU by the peer device.
     *
     *   The time range is 5 milliseconds to 4000 milliseconds.
     *
     *   Range: 0x0005 to 0x0FA0
     *
     *   All other values: RFU
     */
    uint16 maxTransportLatency;
    /*!  Maximum time interval, in us, between start of  Host's consecutive SDUs on this ASE
     *
     *   Range: 0x000000FF to 0x000FFFFF
     *
     *   All other values: RFU
     */
    uint32 sduInterval;
     /*! Presentation delay for this ASE in [microseconds]
      *
      *  Range: 0x00000000 to 0xFFFFFFFF
      */
    uint32 presentationDelay;
} GattAscsServerConfigureQosInfo;

typedef struct
{
    GattAscsServerConfigureQosInfo* gattAscsServerConfigureQosIndInfo;
} GattAscsServerConfigureQosIndAse;

typedef struct
{
    ConnectionId  cid;
    uint8         numAses;
    /*! There is one GattAscsServerConfigureQosIndAse per ASE
     *
     *  The memory pointed to by GattAscsServerConfigureQosInfo will remain available
     *  for the profile/application to read until GattAscsServerConfigureQosResponse()
     *  is called. If the profile/application needs to read the qos configuration
     *  after calling GattAscsServerConfigureQosResponse() it must use the
     *  GattAscsReadQosConfiguration() function.
     *
     *  The profile/application must return the memory pointed to by
     *  'gattAscsServerConfigureQosIndInfo' to the ASCS server in the
     *  GattAscsServerConfigureQosResponse().
     *
     *  The profile/application must not modify the contents of the
     *  gattAscsServerConfigureQosIndInfo for any ASE; the values for an ASE must be either
     *  accepted or rejected by the server.
     */
    GattAscsServerConfigureQosIndAse ase[];
} GattAscsServerConfigureQosInd;

typedef struct
{
    /*! The gattAscsAseResultValue field reports the success (or failure) of configuring the QOS parameters for
     *  each ASE in the indication: GattAscsServerConfigureQosInd.
     *
     *  The result corresponds to the aseId in the gattAscsServerConfigureQosRspInfo field */
    GattAscsAseResultValue gattAscsAseResultValue;
    /*!
     * The additionalResultInfo field can convey useful information when the profile/app detects an error in the
     * GattAscsServerConfigureQosInd received from ASCS. This value can be set to either:
     *      -# the defined GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_XXX value that corresponds the the parameter that has an error
     *      -# GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED
     *      .
     *
     * This value can be set to GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_UNSPECIFIED in all circumstances, however setting it
     * appropriately can assist in resolving product compatibility issues.
     */
    uint8 additionalResultInfo;
    /*!
     *  The ASCS service is responsible for freeing the memory pointed to by
     *  'gattAscsServerConfigureQosRspInfo'. It is the same memory that was passed to the
     *  profile/application in the GattAscsServerConfigureQosInfo field(s) within the
     *  GattAscsServerConfigureQosInd */
    GattAscsServerConfigureQosInfo* gattAscsServerConfigureQosRspInfo;
} GattAscsServerConfigureQosRspAse;

/*!
 * @brief  The configure QOS response must include the same set of ase ids that were included in the
 *         GattAscsServerConfigureQosInd.
 *         The profile/application sends this response by calling GattAscsServerConfigureQosResponse().
 */
typedef struct
{
    ConnectionId cid;
    uint8        numAses;

    /*! The array of GattAscsServerConfigureQosRspAses is 'numAses' in size */
    GattAscsServerConfigureQosRspAse ase[];
} GattAscsServerConfigureQosRsp;

typedef struct
{
    ConnectionId cid;
    uint8        numAses;

    /*! The array of GattAscsServerConfigureQosInfos is 'numAses' in size */
    GattAscsServerConfigureQosInfo ase[];
} GattAscsServerConfigureQosReq;

typedef struct
{
    /*! ASE_ID for this ASE. */
    uint8 aseId;
    uint8 cigId;
    uint8 cisId;
    uint8 metadataLength;
    /*! Metadata will contain LTV fields defined in BAP or by higher layer specifications.
        The server retains ownership of the memory pointed to by 'metadata';
        this memory must not be freed by the profile/app */
    uint8* metadata;

} GattAscsServerEnableIndInfo;

/*!
 *  @brief This indication is sent to the profile/application to indicate that the client is
 *         enabling the specified ASEs and to provide any metadata that is applicable to
 *         those ASEs.
 *
 *         The profile/application responds to this indication by calling GattAscsServerEnableResponse().
 */
typedef struct
{
    ConnectionId cid;
    /*! Total number of ASE_IDs used in the Enable operation */
    uint8        numAses;
    /*! There is one GattAscsServerEnableIndInfo per ASE */
    GattAscsServerEnableIndInfo gattAscsServerEnableIndInfo[];
} GattAscsServerEnableInd;

typedef struct
{
    ConnectionId cid;
    uint8        numAses;
    /*! The array gattAscsAseResult is 'numAses' in size */
    GattAscsAseResult gattAscsAseResult[];
} GattAscsServerGenericRsp;

/*!
 *  @brief The profile/application sends this response to the ASCS service after receiving the
 *         GattAscsServerEnableInd. This response is sent by calling
 *         GattAscsServerEnableResponse().
 *
 *         If the server is the sink for any of the ASEs specified in the enable indication, then the
 *         profile/application sends this response after setting up the data path for each
 *         of those ASEs; the ASEs must be ready to receive audio data before sending the response.
 *
 *         If the server is the source for any of the ASEs specified in the enable indication, this
 *         response is sent after the server has set up the audio data path for those ASEs, however
 *         the server does not transmit audio data until it receives the GattAscsServerReceiverReadyInd
 *         for those ASEs.
 *
 *         The enable response must include the same set of ase ids that were included in the
 *         GattAscsServerEnableInd.
 */
typedef GattAscsServerGenericRsp GattAscsServerEnableRsp;

typedef struct
{
    ConnectionId cid;
    /*! Total number of ASE_IDs used in this operation */
    uint8 numAses;
    /*! The array aseId is 'numAses' in size */
    uint8 aseId[];
} GattAscsServerGenericInd;

typedef GattAscsServerGenericInd GattAscsServerReceiverReadyInd;

typedef GattAscsServerGenericRsp GattAscsServerReceiverReadyRsp;

typedef GattAscsServerGenericRsp GattAscsServerReceiverStopReadyRsp;

typedef struct
{
    ConnectionId cid;
    /*! Total number of ASE_IDs used in the Enable operation */
    uint8 numAses;
    /*! The array aseId is 'numAses' in size */
    uint8 aseId[];
} GattAscsServerReceiverReadyReq;

typedef struct
{
    uint8 aseId;
    uint8 metadataLength;
    /*! Metadata will contain LTV fields defined in BAP or by higher layer specifications.
        The server retains ownership of the memory pointed to by 'metadata'; this memory must not be freed by the profile/app */
    uint8* metadata;
} GattAscsServerUpdateMetadataIndInfo;

typedef struct
{
    ConnectionId cid;
    /*! Total number of GattAscsServerUpdateMetadataIndInfos in the Update Metadata operation */
    uint8 numAses;
    /*! There is one GattAscsServerUpdateMetadataIndInfo per ASE
     * The profile/application is responsible for freeing the memory pointed to by
     * 'gattAscsServerUpdateMetadataIndInfo'   */
    GattAscsServerUpdateMetadataIndInfo gattAscsServerUpdateMetadataIndInfo[];
} GattAscsServerUpdateMetadataInd;

/*!
 *  @brief The profile/application sends this response to the ASCS service after receiving the
 *         GattAscsServerUpdateMetadataInd and after updating the Active Content and Content
 *         Control IDs for each ASE in that indication.
 *         This response is sent by calling GattAscsServerUpdateMetadataResponse().
 */
typedef GattAscsServerGenericRsp GattAscsServerUpdateMetadataRsp;

/*!
 *  @brief This indication is sent to the profile/application to indicate that the client
 *         is disabling the ASE(s) and the audio path for those ASEs can now be cleared down.
 *
 *         Note: the server ASCS service handles the disabling and Handshake operations and
 *         provides a simplified interface to the profile/application: when the GattAscsServerDisableInd
 *         is received, the profile/application can clear down audio paths immediately.
 *
 *         The profile/application responds to this indication after it has cleared down the audio
 *         paths for the specified ASEs. The profile/application responds to this indication by
 *         calling GattAscsDisableResponse().
 */
typedef GattAscsServerGenericInd GattAscsServerDisableInd;

/*!
 *  @brief This indication is sent to the profile/application to indicate that the client
 *         is ready to stop consuming data.
*         This response is sent by calling ascsSendReceiverStopReadyInd().
 */
typedef GattAscsServerGenericInd GattAscsServerReceiverStopReadyInd;

/*!
 *  @brief The profile/application sends this response to the ASCS service after receiving the
 *         GattAscsServerDisableInd and after clearing down the audio path for each ASE in that
 *         indication.
 *         This response is sent by calling GattAscsServerDisableResponse().
 */
typedef GattAscsServerGenericRsp GattAscsServerDisableRsp;

/*!
 *  @brief This indication is sent by the ASCS service to inform the profile/application that all
 *         resources associated with the specified ASEs can be released. After releasing all resources
 *         associated with the specified ASEs, the profile/application responds to the indication
 *         by calling GattAscsReleaseResponse().
 */
typedef GattAscsServerGenericInd GattAscsServerReleaseInd;

/*!
 *  If the profile/application chooses to cache codec configuration for any of the released ASEs then
 *  it has two options:
 *     -# cache a 'preferred' codec configuration, which may differ from the codec configuration
 *        used by the ASE(s) being released.
 *     -# cache codec configuration that is the same as that used by the ASEs being released.
 *     .
 *
 *  If the profile/application caches a 'preferred' codec configuration (that differs from the
 *  codec configuration in use by an ASE being released) then that preferred configuration is
 *  pointed to by 'gattAscsServerConfigureCodecServerInfo'.
 *
 *  If the profile/application chooses to cache the codec configuration currently in use
 *  by an ASE being released, or if the profile/application chooses not to cache codec
 *  configuration, then the 'gattAscsServerConfigureCodecServerInfo' pointer is
 *  set to NULL.
 *
 *  The ASCS service is responsible for freeing the memory pointed to by
 *  'gattAscsServerConfigureCodecServerInfo'
 */
typedef struct
{
    uint8 aseId;
    /*! If the profile/application wants codec configuration to be cached for
     *   any of the ASEs being released then it sets cacheCodecConfiguration
     *   to TRUE, otherwise it is set to FALSE. */
    bool cacheCodecConfiguration;
    /*! The profile/app is responsible for maintaining this memory.
     *  The ASCS library will not access it after the GattAscsServerReleaseComplete() function returns */
    GattAscsServerConfigureCodecServerInfo* gattAscsServerConfigureCodecServerInfo;
} GattAscsServerReleaseCompleteAse;

/*!
 *  @brief This structure is used to inform the ASCS service that all resources associated with the
 *         specified ASEs have been released and that the ASEs can now be released.
 */
typedef struct
{
    ConnectionId cid;
    /*!
     * The numAses value must be the same as the numAses value received in the GATT_ASCS_SERVER_RELEASE_IND
     */
    uint8        numAses;
    /*!  The array of GattAscsServerReleaseCompleteAses is 'numAses' in size */
    GattAscsServerReleaseCompleteAse ase[];
} GattAscsServerReleaseComplete;

/*!
 *  @brief This structure is sent from the Profile/Application to the ASCS service
 *         to initiate the release procedure for the specified ASEs.
 */
typedef struct
{
    ConnectionId cid;
    uint8        numAses;
    uint8        aseId[];
} GattAscsServerReleaseReq;

/*!
 *  @brief This structure is sent from the Profile/Application to the ASCS service
 *         to initiate the disable procedure for the specified ASEs.
 */
typedef struct
{
    ConnectionId cid;
    uint8        numAses;
    uint8        aseId[];
} GattAscsServerDisableReq;

typedef struct
{
    uint8         numAses;
    ClientConfig  aseControlPointCharClientCfg;
    ClientConfig  aseCharClientCfg[];
} GattAscsClientConfig;

/*! @brief The set of messages an application task can receive from the ASCS library.
 */
typedef uint16 GattAscsServerMessageId;

#define GATT_ASCS_SERVER_INIT_CFM                       (GATT_ASCS_SERVER_MESSAGE_BASE +  0)
/* Audio Stream Endpoint messages */
#define GATT_ASCS_SERVER_CONFIGURE_CODEC_IND            (GATT_ASCS_SERVER_MESSAGE_BASE +  1)
#define GATT_ASCS_SERVER_CONFIGURE_QOS_IND              (GATT_ASCS_SERVER_MESSAGE_BASE +  2)
#define GATT_ASCS_SERVER_ENABLE_IND                     (GATT_ASCS_SERVER_MESSAGE_BASE +  3)
#define GATT_ASCS_SERVER_RECEIVER_READY_IND             (GATT_ASCS_SERVER_MESSAGE_BASE +  4)
#define GATT_ASCS_SERVER_RECEIVER_READY_CFM             (GATT_ASCS_SERVER_MESSAGE_BASE +  5)
#define GATT_ASCS_SERVER_UPDATE_METADATA_IND            (GATT_ASCS_SERVER_MESSAGE_BASE +  6)
#define GATT_ASCS_SERVER_DISABLE_IND                    (GATT_ASCS_SERVER_MESSAGE_BASE +  7)
#define GATT_ASCS_SERVER_RELEASE_IND                    (GATT_ASCS_SERVER_MESSAGE_BASE +  8)
#define GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND        (GATT_ASCS_SERVER_MESSAGE_BASE +  9)
/* Library message limit */
#define GATT_ASCS_SERVER_MESSAGE_TOP                    (GATT_ASCS_SERVER_MESSAGE_BASE + 10)

/*!
    @brief Initialises the Audio Stream Control service Library.

    @param appTask The Task that will receive the messages sent from this Published Audio Capability service library.
    @param startHandle This indicates the start handle of the ASCS service
    @param endHandle This indicates the end handle of the ASCS service

    @return The service handle for this ASCS instance.

*/
ServiceHandle GattAscsServerInit(Task theAppTask, uint16 startHandle, uint16 endHandle);

/*!
    @brief Complete the ASE Enable procedure

    @param serviceHandle  The service handle of this ASCS instance.
    @param enableResponse A pointer to an array of result values (e.g. GATT_ASCS_ASE_RESULT_SUCCESS) for each
                           ASE that is being enabled. The profile/application retains ownership
                           of the memory pointed to, the ASCS service does not attempt to
                           access this memory after this function returns.


    This function is called in response to an GATT_ASCS_SERVER_ENABLE_IND, to confirm
    that the Enable procedure can continue.

    The server sends this response when it is able to accept the establishment of a Connected
    Isochronous Stream for each ASE specified in the enable indication.

    If the server is the sink for any of the ASEs specified in the enable indication, then the
    profile/application sends this response after setting up the data path for each
    of those ASEs; the ASEs must be ready to receive audio data before sending the response.

    If the server is the source for any of the ASEs specified in the enable indication, this
    response is sent after the server has set up the audio data path for those ASEs, however
    the server does not transmit audio data until it receives the GATT_ASCS_SERVER_RECEIVER_READY_IND
    for those ASEs.

*/
void GattAscsServerEnableResponse(ServiceHandle serviceHandle, GattAscsServerEnableRsp* enableResponse);

/*!
    @brief Complete the QOS Configuration procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param configureQosResponse   A pointer to the set of qos configuration values and ranges
                                  that the server can support for each ASE.

    The GattAscsServerConfigureQosRsp includes pointers to
    GattAscsServerConfigureQosInfo structures. The memory for these structures is supplied to the
    profile/application in the gattAscsServerConfigureQosIndInfo field(s) in the
    GattAscsServerConfigureQosInd. The profile/application shall return this memory to the
    ASCS service in the GattAscsServerConfigureQosResponse(). The profile/application may modify
    the contents of the memory pointed to if necessary, to reflect actual values that it can support,
    that are compatible with (although slightly different from) the QOS values specified by
    the client, however, the profile/application shall not modify the aseId, cisId or cigId fields.

    This function is called in response to an GATT_ASCS_SERVER_CONFIGURE_QOS_IND, to confirm
    that the QOS Configuration procedure can continue.
*/
void GattAscsServerConfigureQosResponse(ServiceHandle serviceHandle, GattAscsServerConfigureQosRsp* configureQosResponse);

/*!
    @brief Complete the Codec Configuration procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param configureCodecResponse A pointer to the set of codec configuration values and ranges
                                  that the server can support for each ASE. The
                                  profile/application retains ownership of the memory
                                  pointed to, the ASCS service does not attempt to access
                                  this memory after this function returns.

    This function is called in response to an GATT_ASCS_SERVER_CONFIGURE_CODEC_IND, to confirm
    that the Codec Configuration procedure can continue.
*/
void GattAscsServerConfigureCodecResponse(ServiceHandle serviceHandle, GattAscsServerConfigureCodecRsp* configureCodecResponse);

/*!
    @brief Complete the ASE Update Metadata procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param updateMetadataResponse A pointer to an array of result values (e.g. GATT_ASCS_ASE_RESULT_SUCCESS) for each
                                   ASE affected by the Update Metadata procedure.
                                   The profile/application retains ownership of the memory
                                   pointed to, the ASCS service does not attempt to access
                                   this memory after this function returns.

    This function is called in response to an GATT_ASCS_SERVER_UPDATE_METADATA_IND, to confirm
    that the Update Metadata procedure can continue.
*/
void GattAscsServerUpdateMetadataResponse(ServiceHandle serviceHandle, GattAscsServerUpdateMetadataRsp* updateMetadataResponse);

/*!
    @brief Complete the ASE Disable procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param disableResponse A pointer to an array of result values (e.g. GATT_ASCS_ASE_RESULT_SUCCESS) for each
                            ASE that is being disabled.
                            The profile/application retains ownership of the memory
                            pointed to, the ASCS service does not attempt to access
                            this memory after this function returns.

           The profile/application calls this function after:
              -# Receiving the GATT_ASCS_SERVER_DISABLE_IND and
              -# The server has stopped consuming audio data for all ASEs for which the server is the sink
              -# The server has stopped transmitting audio data for all ASEs for which the server is the source
              .
*/
void GattAscsServerDisableResponse(ServiceHandle serviceHandle, GattAscsServerDisableRsp* disableResponse);

/*!
    @brief Initiate the ASE Configure Codec procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param releaseRequest A pointer to a GattAscsServerConfigureCodecReq structure
                           that describes the ASEs to be taken to the codec configured state
                           and the codec configuration data for each of those ASEs.
                           The Profile/Application is responsible for all memory associated
                           with GattAscsServerConfigureCodecReq, i.e. the Application/Profile
                           must free any codecConfiguration data referenced in the
                           GattAscsServerConfigureCodecReq.
*/
void GattAscsServerConfigureCodecRequest(ServiceHandle serviceHandle, GattAscsServerConfigureCodecReq* configureCodecRequest);

/*!
    @brief Initiate the ASE Configure QOS procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param releaseRequest A pointer to a GattAscsServerConfigureQosReq structure
                           that describes the ASEs to be taken to the QOS configured state
                           and the QOS configuration data for each of those ASEs.

*/
void GattAscsServerConfigureQosRequest(ServiceHandle serviceHandle, GattAscsServerConfigureQosReq* configureQosRequest);

/*!
    @brief Initiate the ASE Disable procedure
           The profile/application calls this function to initiate disabling ASEs, if the server is the sink
           for any of the specified ASEs then the profile/app must already have stopped consuming audio data
           before calling this function.

    @param serviceHandle The service handle of this ASCS instance.
    @param disableRequest A pointer to a GattAscsServerDisableReq structure
                           that describes the ases to disable and the connection they are on.
*/
void GattAscsServerDisableRequest(ServiceHandle serviceHandle, GattAscsServerDisableReq* disableRequest);

/*!
    @brief Inform the ASCS service that the ASE(s) can be released and that the underlying CIS has been
           cleared down

    @param serviceHandle The service handle of this ASCS instance.
    @param releaseComplete A pointer to a structure comprising an array of structures with the following fields:
                               - A result value (e.g. GATT_ASCS_ASE_RESULT_SUCCESS) for each
                                 ASE that is being released.
                               - An optional preferred codec configuration for each ASE to use if the
                                 profile/application wishes to cache codec configuration values,
                                 further details on the usage of this field can be found in the
                                 documentation for GattAscsServerReleaseComplete.
                               .

                                 The profile/application retains ownership of the memory
                                 pointed to by any fields within this structure, the ASCS
                                 service does not attempt to access this memory after this
                                 function returns.

    This function is called autonomously by the Profile/Application or in response to an GATT_ASCS_SERVER_RELEASE_IND.
    When this function is called autonomously, the Profile/Application must call GattAscsServerReleaseRequest() first.
    This function should only be called when the ASE(s) can be released and after any underlying CIS has been
    cleared down.
*/
void GattAscsServerReleaseCompleteRequest(ServiceHandle serviceHandle, GattAscsServerReleaseComplete* releaseComplete);

/*!
    @brief Initiate the ASE Release procedure

    @param serviceHandle The service handle of this ASCS instance.
    @param releaseRequest A pointer to a GattAscsServerReleaseReq structure
                           that describes the ASEs to release and the connection they are on.
*/
void GattAscsServerReleaseRequest(ServiceHandle serviceHandle, GattAscsServerReleaseReq* releaseRequest);

/*!
    @brief Initiate the Receiver Ready procedure (used when the Server is the Sink)

    @param serviceHandle The service handle of this ASCS instance.
    @param receiverReadyRequest A pointer to a GattAscsServerReceiverReadyReq structure
                                  that describes the ASEs on the server for which audio data can now be received.
*/
void GattAscsReceiverReadyRequest(ServiceHandle serviceHandle, GattAscsServerReceiverReadyReq* receiverReadyRequest);

/*!
    @brief Complete the Receiver Ready procedure (used when the Server is the Source)

    @param serviceHandle The service handle of this ASCS instance.
    @param receiverReadyResponse A pointer to a GattAscsServerReceiverReadyRsp structure
                                   that describes the ASEs on the server for which audio data can now be transmitted.
*/
void GattAscsReceiverReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverReadyRsp* receiverReadyResponse);

/*!
    @brief Inform the ASCS service that all ASEs in the GattAscsServerReceiverStopReadyRsp, for which the server
           is the audio source, have stopped transmitting audio data.

    @param serviceHandle The service handle of this ASCS instance.
    @param receiverStopReadyRequest A pointer to a GattAscsServerReceiverStopReadyRsp structure
                                    that describes the ASEs for which the server is no longer transmitting audio data.
*/
void GattAscsReceiverStopReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverStopReadyRsp* receiverStopReadyResponse);

/*!
    \brief Remove the configuration for a peer device, identified by its
           Connection ID.

    This removes the configuration for that peer device from the
    service library, freeing the resources used for that config.
    This should only be done when the peer device is disconnecting.

    \param serviceHandle The service handle of this ASCS instance.
    \param cid A Connection ID for the peer device.

    \return GattAscsClientConfig Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
*/
GattAscsClientConfig* GattAscsRemoveConfig(ServiceHandle serviceHandle, ConnectionId cid);
/*!
    \brief Add configuration for a previously paired peer device, identified by its
    Connection ID (CID).
 
    The CID can be used to get the Peer Device Reference (PDR) from the GATT profile,
    and from that any other information the Service might have interest in for that
    peer device such as:
       - Encryption Key Size
       - If the connection is authenticated
       - EATT features supported by the peer
       .
    \param service_handle The service handle of this ASCS instance.
    \param cid The Connection ID to the peer device.
    \param config Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    \return gatt_status_t status of the Add Configuration operation. For example,
           if memory cannot be allocated for the added configuration, the status
           gatt_status_insufficient_resources could be returned.           
*/
gatt_status_t GattAscsAddConfig(ServiceHandle serviceHandle, ConnectionId cid, const GattAscsClientConfig* config);

/*!
    @brief An API to retrieve the Codec Configuration of an ASE.

    @param serviceHandle The service handle of this ASCS instance.
    @param cid  The id of the connection that contains the ASE from which the codec configuration is to be retrieved.
    @param aseId  The id of the ASE from which the codec configuration is to be retrieved.
*/
GattAscsServerConfigureCodecInfo* GattAscsReadCodecConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId);
/*!
    @brief An API to retrieve the QOS Configuration of an ASE.

    @param serviceHandle The service handle of this ASCS instance.
    @param cid  The id of the connection that contains the ASE from which the QOS configuration is to be retrieved.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
*/
GattAscsServerConfigureQosInfo* GattAscsReadQosConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId);

#endif /* GATT_ASCS_SERVER_H */

