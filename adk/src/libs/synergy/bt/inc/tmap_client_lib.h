/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #60 $
******************************************************************************

FILE NAME
    tmap_client_lib.h
    
DESCRIPTION
    Header file for the telephony and media audio profile (TMAP) APIs.
*/

#ifndef TMAP_CLIENT_LIB_H
#define TMAP_CLIENT_LIB_H

#include "tmap_client_prim.h"

/*!
    @brief Initialises the TMAP client Library.

    This interface need to be invoked for a connected remote device that has
    TMAS service UUID and TMAP Role in extended adevertising.
    
    Application receives TMAP_CLIENT_INIT_CFM with 'status' as "TMAP_CLIENT_STATUS_IN_PROGRESS"
    once profile library initialisation started.A final TMAP_CLIENT_INIT_CFM will
    be received with a 'status' "TMAP_CLIENT_STATUS_SUCCESS" once initialisation is complete.
          
    @param appTask           The Task that will receive the messages sent to this profile library
    @param clientInitParams  Pointer to Initialisation parameters for TMAP client.
                             Application to free the pointer.
    @param deviceData        A Pointer to TMAS service handles information if available.

    NOTE: A TMAP_CLIENT_INIT_CFM will be received with 'status'as 
          TMAP_CLIENT_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND remote device doesn't 
          support tmas server. In this case application can still perform other
          API.
*/
void TmapClientInitReq(AppTask appTask,
                       TmapClientInitData *clientInitParams,
                       TmapClientHandles  *deviceData);

/*!
    @brief This API is used to read the Role characteristic from Remote connected
           TMAS service. A TMAP_CLIENT_ROLE_CFM message will be sent to the 
           registered application Task.

    @param profileHandle  The Profile handle.

    NOTE: A TMAP_CLIENT_ROLE_CFM message will be sent to the registered application Task.

*/
void TmapClientReadRoleReq(TmapClientProfileHandle profileHandle);

/*!
    @brief This API is used to read Service attribute handles of the remote 
    connected TMAS server.

    @param profileHandle  The Profile handle received in TMAP_CLIENT_INIT_CFM.

    @return Pointer to GattTmasClientDeviceData on successful else NULL .

*/
GattTmasClientDeviceData* TmapClientGetDevicedata(TmapClientProfileHandle profileHandle);

/*!
    @brief This API is used to add new connected device from the coordinated set
    group to an existing profileHandle TMAP of the same group.

    @param profileHandle  The Profile handle received in TMAP_CLIENT_INIT_CFM.
	@param cid ConnectionId of the new connected device to be added

    @return TRUE on successful addition else FALSE.
*/
bool TmapClientAddNewDevice(TmapClientProfileHandle profileHandle,
                            uint32 cid);

/*!
    @brief This API is used to remove an already added device using from
    TmapClientProfileHandle instance added using TmapClientAddNewDevice().

    @param profileHandle  The Profile handle received in TMAP_CLIENT_INIT_CFM.
	@param cid ConnectionId of the connected device to be already added.

    @return TRUE on successful removal else FALSE.

    Note: This API to be called once connected device is disconnected and before
    TmapClientDestroyReq() called

*/
bool TmapClientRemoveDevice(TmapClientProfileHandle profileHandle,
                            uint32 cid);

/*!
    @brief When GATT connection disconnected or lost for single or group of devices 
    for cordinated set, the application must call this API to remove
    TMAP profile client instances associated with the connections.
    This is to clean the instance created by the TmapInitReq().

    @param profileHandle The Profile handle.

    NOTE: A TMAP_CLIENT_DESTROY_CFM with intermediate TmapClientStatus "TMAP_CLIENT_STATUS_IN_PROGRESS"
      and final TmapClientStatus "TMAP_CLIENT_STATUS_SUCCESS" will be send the 
      caller incase of success.
*/
void TmapClientDestroyReq(TmapClientProfileHandle profileHandle);

/*!
    @brief This API is used to retrieve the stream id (cis/bis) list

    @param groupId the ID of the device group which share SIRK with new device.
    @param id cig/big ID.
    @param flag : Unicast - TMAP_CLIENT_CIG
                  Broadcast - TMAP_CLIENT_BIG

    NOTE: This API is synchronous call. If cigId or bigId is valid then valid values of stream id are returned.
          If not valid then NULL is returned.
          Higher layer has to free the memory for TmapClientGroupInfo and its members.

*/

TmapClientGroupInfo *TmapClientGetStreamInfo(ServiceHandle groupId, uint8 id, TmapClientIsoGroupType flag);

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*!
    @brief This API is to registers "groupId" of CAP to TMAP profiles to enable 
          Unicast and Broadcast specific operations.

    @param groupId       In case of Unicast,the ID received in CAP_CLIENT_INIT_CFM. 
                         In case of Broadcast Source, this is  bcastSrcProfileHandle which is returned as part of
                         TMAP_CLIENT_BROADCAST_SRC_INIT_CFM
    @param profileHandle profile handle of TMAP received in TMAP_CLIENT_INIT_CFM

    @return None
*/

void TmapClientRegisterTaskReq(ServiceHandle groupId,
                               TmapClientProfileHandle profileHandle);

/*!
    @brief This API is use to Establishes LEA Unicast connection to a Remote Devices.
    Application recieves TMAP_CLIENT_UNICAST_CONNECT_CFM when this API is called.
    This allows Unicast Client to negotiate Codec and QOS parameters with the
    remoted unicast server for the intended usecase.

    @param The Profile handle received in TMAP_CLIENT_INIT_CFM.
    @param groupId group id received in CAP_CLIENT_INIT_CFM
    @param sinkConfig  Codec capability setting value for Sink.
           from the remote device discovered capability
                    in CapClientDiscoverStreamCapabilitiesCfm for Sink
    @param srcConfig  The audio config Codec settings interest which is discovered using
                    CapDiscoverStreamCapabilitiesReq for Source
    @param useCase  Audio contexts bitmask value, example Media or Voice|Ringtone
    @param codecId  Codec Id of the negotaited Capabilities
    @param sinkAudioLocations  set audio location bitmask for Sink stream.
    @param srcAudioLocations   set audio location bitmask for Source stream.
    @param numOfMic   Number of Mic input desired by application. Actual number 
                      of MIC configured depends on remote capability and any change
                      in number of MIC will be set in TMAP layer in TMAP_UNICAST_CONNECT_CFM
                      Set to 0 if no MIC path in Source Stream
    @param cigConfigMode   Default or QHS mode. 

                           In Default mode, CAP will use internal SIG recomended parameters
                           In QHS mode, application can configure few CIG params in TmapClientQhsConfig.
                           When joint stereo mode bit is set TMAP expects only 2 bits to be set in sinkAudioLocations/ srcAudioLocations
    @param cigConfig   A pointer to config params for CIG in QHS mode. Applicable
                       only if cigConfigMode is set to QHS mode otherwise NULL

    Note : Recommended SinkConfig values are 
       TMAP_CLIENT_STREAM_CAPABILITY_16_1
       TMAP_CLIENT_STREAM_CAPABILITY_32_1, TMAP_CLIENT_STREAM_CAPABILITY_32_2
       TMAP_CLIENT_STREAM_CAPABILITY_48_1, TMAP_CLIENT_STREAM_CAPABILITY_48_6
       TMAP_CLIENT_STREAM_CAPABILITY_48_2, TMAP_CLIENT_STREAM_CAPABILITY_48_3,
       TMAP_CLIENT_STREAM_CAPABILITY_48_4, TMAP_CLIENT_STREAM_CAPABILITY_48_5
       
       
       Recommended SourceConfig values are TMAP_CLIENT_STREAM_CAPABILITY_16_1
       TMAP_CLIENT_STREAM_CAPABILITY_32_1, TMAP_CLIENT_STREAM_CAPABILITY_32_2
*/

void TmapClientUnicastConnectReq(TmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId,
                                 TmapClientStreamCapability sinkConfig,
                                 TmapClientStreamCapability srcConfig,
                                 TmapClientContext useCase,
                                 uint32 sinkAudioLocations,
                                 uint32 srcAudioLocations,
                                 uint8 numOfMic,
                                 TmapClientCigConfigMode cigConfigMode,
                                 const TmapClientQhsConfig* cigConfig);

/*!
    @brief This API initiate Unicast Stream start procedure to Remote Devices.
     Application recieves multiple TMAP_CLIENT_UNICAST_START_STREAM_IND
     based on number of connected device from the coordinated set and a final
     TMAP_CLIENT_UNICAST_START_STREAM_CFM.

     NOTE:Each successfull TMAP_CLIENT_UNICAST_START_STREAM_IND and TMAP_CLIENT_UNICAST_START_STREAM_CFM
     indicates audio graph can be setup and remote is ready for streaming.

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media
    @param ccId  Content control Id of Voice/Media and valid ccId value range is 1-255
    @param metadataLen Length of metadata being passed
    @param metadata metadata as defined in Assigned number document shall be
          filled in LTV format. "Streaming_Audio_Contexts" is not required
*/
void TmapClientUnicastStartStreamReq(TmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     TmapClientContext useCase,
                                     uint16 ccId,
                                     uint8 metadataLen,
                                     const uint8* metadata);

/*!
    @brief This API initiate Unicast Audio Upadates Stream procedure to update
    an existing audio contexts in use. Application recieves TMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM 
    when this message is called

    Note:This procedure shall be called only after TMAP_CLIENT_UNICAST_START_STREAM_CFM 
    to update Streaming Audio context metadata for which matches existing audio configuration

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media
    @param ccId  Content control Id of Voice/Media  and valid ccId value range is 1-255
    @param metadataLen Length of metadata parameter
    @param metadata metadata in LTV format for Streaming Audio contexts.
*/
void TmapClientUnicastUpdateAudioReq(TmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     TmapClientContext useCase,
                                     uint16 ccId,
                                     uint8 metadataLen,
                                     const uint8* metadata);

/*!
    @brief SThis API initiate Unicast Stream stop procedure to stop ongoing streaming.
    Application recieves TMAP_CLIENT_UNICAST_STOP_STREAM_CFM when this message is called.

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param doRelease  Whether the stream need to be released or not.
*/
void TmapClientUnicastStopStreamReq(TmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    bool doRelease);

/*!
    @brief Removes the configuration done for an Context.
    Application recieves a TMAP_CLIENT_UNICAST_DISCONNECT_CFM when this API is called.

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media
*/
void TmapClientUnicastDisconnectReq(TmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    TmapClientContext useCase);

/*!
    @brief Mutes/Unmutes the connected Volume renderer device
    Application recieves TMAP_CLIENT_MUTE_CFM when this message is called

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param mute Mute state
*/
void TmapClientMuteReq(TmapClientProfileHandle profileHandle,
                       ServiceHandle groupId,
                       bool mute);

/*!
    @brief Sets the absolute voulme state of connected Volume renderer device
    Application recieves TMAP_CHANGE_VOLUME_CFM when this API is called

    @param profileHandle profile handle of TMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param volumeSetting new volume state of the connected devices
*/
void TmapClientSetAbsVolumeReq(TmapClientProfileHandle profileHandle,
                               ServiceHandle groupId,
                               uint8 volumeSetting);

/*!
    @brief DeRegister with CAP for msgs/indications
    Application receives TMAP_CLIENT_DEREGISTER_TASK_CFM when this message is called

    @param profileHandle Profile handle of TMAP.
    @param groupId      The ID of the device group which share SIRK
*/
void TmapClientDeRegisterTaskReq(TmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId);

#endif /* INSTALL_LEA_UNICAST_CLIENT */

#if defined(INSTALL_LEA_UNICAST_CLIENT) || defined(INSTALL_LEA_BROADCAST_SOURCE)
/*!
    @brief This API can be used to set custom CIG params for Unicast or BIG params for
           Broadcast with user defined values for a particular usecase before calling 
           TmapClientUnicastConnectReq() for Unicast or TmapClientBroadcastSrcConfigReq()
           for Broadcast.
           These values are used for for particular usecase as long as CIG or BIG is active.
           Once the CIG or BIG is destroyed then this API needs to be called again before
           CapClientUnicastConnectReq() or CapClientBcastSrcConfigReq() for a usecase is
           invoked.
           These parameters have precedence over QHS and standard CIG or BIG params.
           Application receives TMAP_CLIENT_SET_PARAMS_CFM when this API is called.

    @param profileHandle profile handle of TMAP Unicast/Broadcast
    @param groupId       In case of Unicast,the ID received in CAP_CLIENT_INIT_CFM. 
                         In case of Broadcast, this is  bcastSrcProfileHandle which is returned as part of
                         TMAP_CLIENT_BROADCAST_SRC_INIT_CFM
    @param sinkConfig    Sink CIG config params app wants to apply on usecase to be established
                         using TmapClientUnicastConnectReq()
    @param srcConfig     Source CIG config params app wants to apply on usecase to be established
                         using TmapClientUnicastConnectReq()
    @param type          Refer TmapClientParamsType for details
    @param numOfParamElem In case of Unicast this num has to be 1,
                          In case of Broadcast , this number is equal to number of subgroups.
    @param paramElems    Application supplied parameters corresponding to config type.
                         For Unicast use TmapClientUnicastConnectParams or TmapClientUnicastConnectParamsV1
                         and for broadcast use TmapClientBroadcastConfigParams
    @return None

    NOTE: a) In case of broadcast sinkConfig, srcConfig values will be ignored, it is advised to fill
             TMAP_CLIENT_STREAM_CAPABILITY_UNKNOWN in these parameters in case of Broadcast.
          b) In case of unicast for unidirectional use cases(Central to Peripheral or vice-versa),
             it is advised to set parameter values for CIG in the other direction(Peripheral to Central
             or vice-versa) same as Central to Peripheral(or vice-versa).
             sdu_size and rtn values for CIS must be set based on direction otherwise shall be set to 0.

*/
void TmapClientSetParamsReq(TmapClientProfileHandle profileHandle,
                            ServiceHandle groupId,
                            TmapClientStreamCapability sinkConfig,
                            TmapClientStreamCapability srcConfig,
                            TmapClientParamsType type,
                            uint8 numOfParamsElems,
                            const void* paramsElems);
#endif /* defined(INSTALL_LEA_UNICAST_CLIENT) || defined(INSTALL_LEA_BROADCAST_SOURCE) */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
/*!
    @brief Initialises the TMAP Broadcast Source. The initialised Source can be
    collocated or non-colocated source.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_INIT_CFM when this API is called

    @param appTask The Task that will receive the messages sent from this library.
*/
void TmapClientBroadcastSrcInitReq(AppTask appTask);

/*!
    @brief Deinitialises the TMAP Broadcast Source.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
*/
void TmapClientBroadcastSrcDeinitReq(TmapClientProfileHandle brcstSrcProfileHandle);

/*!
     @brief Set Broadcast Source "Broadcast ID".
 
     This optional API allows application to set unique Broadcast ID.
     This API shall be called before TmapClientBroadcastSrcConfigReq() once for
     the life time of the Broadcast session.
     If a Broadcast ID is set from the application, it shall not change it again 
     until TmapClientBroadcastSrcRemoveStreamReq() called.

     @param brcstSrcProfileHandle  broadcast Source Profile handle returned as 
            part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
     @param broadcastId generated Broadcast ID values based on random number 
            generation as defined in Volume 3, Part H, Section 2 in Core spec.
            If Value TMAP_CLIENT_BROADCAST_ID_DONT_CARE is set, profile will 
            generate it internally. 
            Only 24 bit value will be set as Broadcast id.
.
     Note:  Always recommended to use new random broadcastId for each Broadcast session. 

     @return None
*/
void TmapClientBroadcastSrcSetBroadcastId(TmapClientProfileHandle brcstSrcProfileHandle, uint32 broadcastId);

/*!
    @brief This API is deprecated, please use TmapClientBroadcastSrcConfigSubgroupReq.
           This API Configures the TMAP Broadcast Source for single subgroup.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param ownAddrType Address type of the local device
    @param presentationDelay Presentation delay to synchronize the presentation
                             of multiple BISs in a BIG.
    @param numSubgroup Total number of sub groups in the broadcast group. 
                                  This field value should be non-zero and it should be atleast 0x01 
    @param subgroupInfo Sub group related information in CapBigSubgroups format. This field should not be NULL
                        and subgroupInfo->numBis value should be non-zero and it should be atleast 0x01.
                        Please note if common codec configurations parameters(config, targetLatency, lc3BlocksPerSdu) are
                        provided(i.e. non-zero) at both level-2 and level-3 then parameters provided at level-3 will be
                        overwritten by parameters provided at level-2. Recommended value for common codec configurations
                        parameters at level-3 is zero.
    @param brcstSrcNameLen   length of the broadcast source name
    @param brcstSrcName   Broadcast source name
    @param bigConfigMode   Default or QHS mode and/or Joint Stereo mode. In Default mode, CAP will use internal parameters
                           In QHS mode, application can configure some of BIG params by passing TmapClientQhsBigConfig.
                           If Joint Stereo mode is set then Broadcast Source will always configure for Audio Configuration 14
                           (Multiple Audio Channels, 1 BIS). Application needs to ensure that valid subgroupInfo for 
                           Audio Configuration 14 is passed in this mode.
    @param qhsBigConfig       config param for BIG in QHS mode. Applicable only if cigConfigMode is set to QHS mode.

    Note : Recommended config TmapClientStreamCapability values are 
TMAP_CLIENT_STREAM_CAPABILITY_48_1, 
       TMAP_CLIENT_STREAM_CAPABILITY_48_2, TMAP_CLIENT_STREAM_CAPABILITY_48_3,
       TMAP_CLIENT_STREAM_CAPABILITY_48_4, TMAP_CLIENT_STREAM_CAPABILITY_48_5
       TMAP_CLIENT_STREAM_CAPABILITY_48_6
*/
void TmapClientBroadcastSrcConfigReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                     uint8 ownAddrType,
                                     uint32 presentationDelay,
                                     uint8 numSubgroup,
                                     const TmapClientBigSubGroup* subgroupInfo,
                                     uint8 brcstSrcNameLen,
                                     const uint8* brcstSrcName,
                                     TmapClientBigConfigMode bigConfigMode,
                                     const TmapClientQhsBigConfig *qhsBigConfig);


/*!
    @brief Configures the TMAP Broadcast Source for multiple subgroups.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param ownAddrType Address type of the local device
    @param presentationDelay Presentation delay to synchronize the presentation
                             of multiple BISs in a BIG.
    @param numSubgroup Total number of sub groups in the broadcast group. 
                                  This field value should be non-zero and it should be atleast 0x01 
    @param subgroupInfo Sub group related information in CapBigSubgroups format. This field should not be NULL
                        and subgroupInfo->numBis value should be non-zero and it should be atleast 0x01.
                        Please note if common codec configurations parameters(config, targetLatency, lc3BlocksPerSdu) are
                        provided(i.e. non-zero) at both level-2 and level-3 then parameters provided at level-3 will be
                        overwritten by parameters provided at level-2. Recommended value for common codec configurations
                        parameters at level-3 is zero.
    @param brcstSrcNameLen   length of the broadcast source name
    @param brcstSrcName   Broadcast source name
    @param bigConfigMode   Default or QHS mode and/or Joint Stereo mode. In Default mode, CAP will use internal parameters
                           In QHS mode, application can configure some of BIG params by passing TmapClientQhsBigConfig.
                           If Joint Stereo mode is set then Broadcast Source will always configure for Audio Configuration 14
                           (Multiple Audio Channels, 1 BIS). Application needs to ensure that valid subgroupInfo for Audio
                           Configuration 14 is passed in this mode.
    @param qhsBigConfig       config param for BIG in QHS mode. Applicable only if cigConfigMode is set to QHS mode.

    Note : Recommended config TmapClientStreamCapability values are 
TMAP_CLIENT_STREAM_CAPABILITY_48_1, 
       TMAP_CLIENT_STREAM_CAPABILITY_48_2, TMAP_CLIENT_STREAM_CAPABILITY_48_3,
       TMAP_CLIENT_STREAM_CAPABILITY_48_4, TMAP_CLIENT_STREAM_CAPABILITY_48_5
       TMAP_CLIENT_STREAM_CAPABILITY_48_6
*/
void TmapClientBroadcastSrcConfigSubgroupReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                             uint8 ownAddrType,
                                             uint32 presentationDelay,
                                             uint8 numSubgroup,
                                             const TmapClientBigMultiSubGroup* subgroupInfo,
                                             uint8 brcstSrcNameLen,
                                             const uint8* brcstSrcName,
                                             TmapClientBigConfigMode bigConfigMode,
                                             const TmapClientQhsBigConfig *qhsBigConfig);

/*!
    @brief Start the streaming of TMAP Broadcast Source.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param encryption   Whenther to encrypt BIG stream or not
    @param broadcastCode 16 octets code to encrypt the BIG stream if parameter encryption is TRUE else NULL.

*/
void TmapClientBroadcastSrcStartStreamReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                          bool  encryption,
                                          const uint8* broadcastCode);

/*!
    @brief Updates metadata of the TMAP  Broadcast Source when in streaming state.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param useCase  Streaming audio context value for broadcast usecase
    @param numSubgroup  number of subgroups requires updated for an existing broadcast source
    @param metadataLen  length of metadata
    @param metadata     metadata shall be in LTV format as per Generic Audio specification.

*/
void TmapClientBroadcastSrcUpdateStreamReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                           TmapClientContext useCase,
                                           uint8_t numSubgroup,
                                           uint8 metadataLen,
                                           const uint8* metadata);

/*!
    @brief Stop the streaming of TMAP Broadcast Source.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast SSourcerc Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.

*/
void TmapClientBroadcastSrcStopStreamReq(TmapClientProfileHandle brcstSrcProfileHandle);

/*!
    @brief Remove the TMAP Broadcast Source stream and audio announcements.
    Application recieves TMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.

*/
void TmapClientBroadcastSrcRemoveStreamReq(TmapClientProfileHandle brcstSrcProfileHandle);

/*!
    @brief Configures the Broadcast Src Extended and periodic advertising parameters

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of TMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param srcAdvPaParams    extended and Periodic Advertisement parameters to set.
      default parameter values used if this API is not exercised: 
      advEventProperties = 0
      advIntervalMin = CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN/ 0x00A0 - 100 msec
      advIntervalMax = CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN/ 0x00A0 - 100 msec
      primaryAdvPhy = BAP_LE_1M_PHY /0x01
      primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_DEFAULT /i.e. HCI_ULP_ADVERT_CHANNEL_ALL
      secondaryAdvMaxSkip = 0
      secondaryAdvPhy = BAP_LE_1M_PHY /0x01
      advSid = CM_EXT_ADV_SID_ASSIGNED_BY_STACK /0x400
      periodicAdvIntervalMin = 0x320/ 1Sec
      periodicAdvIntervalMax = 0x640/ 2Sec
      advertisingTransmitPower = 0x7F /Host No preference

*/
void TmapClientBroadcastSrcSetAdvParamsReq(TmapClientProfileHandle brcstSrcProfileHandle,
                                           const TmapClientBcastSrcAdvParams *srcAdvPaParams);
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/*!
*
*  NOTE: Before Calling broadcast Assistant Procedure make sure to Call
*        CapClientStreamAndContrilInitReq from Application. Only after
*        successful initiation of Stream and Control Procedure,
*        the CAP Broadcast Assistant Procedures can be excercised.
*/

/*!
     @brief Broadcast Assistant starts scan for broadcast audio announcement that meet a 
            specified scan filter.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid btConnid for the device which is currently written to.
     @param bcastSrcType Broadcast Source is local device or not
     @param audioFilterContext Looks for particular SRC's meeting a specified filter(s) eg:media
     @param scanFlags Scanning flags
     @param ownAddressType Local address type that shall be used for scanning
     @param scanningFilterPolicy Scan filter policy to be used for scanning

     @return TMAP_CLIENT_BROADCASTST_ASST_START_SRC_SCAN_CFM confirmation message will be 
      sent to application when search for periodic train is started or failed.
      TMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND will be sent to
      application on receiving reports from broadcast sources.
 */
void TmapClientBroadcastAsstStartSrcScanReq(TmapClientProfileHandle profileHandle,
                                            ServiceHandle groupId,
                                            uint32 cid,
                                            TmapClientBroadcastSrcType bcastSrcType,
                                            TmapClientContext audioFilterContext,
                                            uint8 scanFlags,
                                            uint8 ownAddressType,
                                            uint8 scanningFilterPolicy);

/*!
     @brief Broadcast Assistant stops scan for broadcast audio announcement of a given scan handle

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid btConnid for the device which is currently written to.
     @param scanHandle Scan handle passed as part of TMAP_CLIENT_BROADCASTST_ASST_START_SRC_SCAN_CFM

     @return TMAP_CLIENT_BROADCASTST_ASST_STOP_SRC_SCAN_CFM confirmation message will be 
      sent to application when search for periodic train is stopped or failed.
 */
void TmapClientBroadcastAsstStopSrcScanReq(TmapClientProfileHandle profileHandle,
                                           ServiceHandle groupId,
                                           uint32 cid,
                                           uint16 scanHandle);

/*!
     @brief Writes the Broadcast Sink Receiver State CCCDs in order to
            subscribe/unsubscribe GATT notifications.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID . If cid is zero notifications will be
                enabled for all sources.
     @parm sourceId Source identifier
     @param notificationEnable enable/disable the notifications on remote

     @return TMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM confirmation message
             will be sent to application.
*/

void TmapClientBroadcastAsstRegisterNotificationReq(TmapClientProfileHandle profileHandle,
                                                    ServiceHandle groupId,
                                                    uint32 cid,
                                                    uint8 sourceId,
                                                    bool noficationEnable);

/*!
     @brief Request to read the Broadcast Sink Receiver State for all sources.This API is optional
	        App anyway will receive BRS indication if registered for notifications.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID . If cid is zero BRS inidications will be
                sent for all sources.

     @return TMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND(s) will be sent if cid is valid and also
             TMAP_CLIENT_BROADCAST_ASST_READ_RECEIVE_STATE_CFM confirmation message will be 
             sent to application about the status of operation.
*/
void TmapClientBroadcastAsstReadReceiverSinkStateReq(TmapClientProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32  cid);

/*!
     @brief Broadcast Assistant establishes sync to periodic train of given address.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param addrt BT address of the Broadcast Source along with type
     @param advSid Advertising SID

     @return TMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM confirmation message
             will be sent to application

*/
void TmapClientBroadcastAsstStartSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              TYPED_BD_ADDR_T *addrt,
                                              uint8 advSid);

/*!
     @brief Broadcast Assistant is terminated to syncing to a periodic train.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param syncHandle sync handle of PA to be terminated

     @return TMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM confirmation
             message will be sent to application

*/
void TmapClientBroadcastAsstTerminateSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                                  ServiceHandle groupId,
												  uint16 syncHandle);

/*!
     @brief TMAP cancels establishing sync to a periodic train.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of TMAP
     @param groupId  CAP groupId received in CapInitCfm message on CAP Init.

     @return TMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM confirmation message
             will be sent to application

*/
void TmapClientBroadcastAsstCancelSyncToSrcReq(TmapClientProfileHandle profileHandle,
                                               ServiceHandle groupId);

/*!
     @brief This API used to Add Broadcast Audio stream information of the Broadcast
           source to the connected Broadcast Sink device.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Connectio Identifier of the connected Broadcast Sink device to be
                synced to Broadcast Source
     @param sourceAddrt Advertiser_Address for the Broadcast Source along with type
     @param bcastSrcType Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising Set ID of the Broadcast Source
     @param paSyncState PA Synchronization state
     @param paInterval SyncInfo field Interval parameter value
     @param broadcastId Identifier of Broadcast Source's BIG, obtained as a part of
                        Extended Adv
     @param numbSubGroups Number of subgroups  present in the BIG as defined by
                          the Num_Subgroups parameter of the BASE.
     @param subgroupInfo Pointer to BIG first subgroupInfo pointed by
                         numbSubGroups in the BIG

     @return TMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM message will be
             sent to application.

*/
void TmapClientBroadcastAsstAddSrcReq(TmapClientProfileHandle profileHandle,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      TYPED_BD_ADDR_T *sourceAddrt,
                                      TmapClientBroadcastSrcType bcastSrcType,
                                      uint16 syncHandle,
                                      uint8 sourceAdvSid,
                                      uint8 paSyncState,
                                      uint16 paInterval,
                                      uint32 broadcastId,
                                      uint8 numbSubGroups,
                                      TmapClientSubgroupInfo *subgroupInfo);

/*!
     @brief This API used to modify Broadcast Audio stream information of the Broadcast
           source to the connected Broadcast Sink device.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param bcastSrcType Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising Set ID of the Broadcast Source
     @param paSyncState PA Synchronization state
     @param paInterval SyncInfo field Interval parameter value
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink
     @param numbSubGroups Number of subgroups  present in the BIG as defined by
                          the Num_Subgroups parameter of the BASE.
     @param subgroupInfo Pointer to BIG first subgroupInfo pointed by
                         numbSubGroups in the BIG.
                         App needs to free this pointer.

     @return TMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM message will be
             sent to application.

*/
void TmapClientBroadcastAsstModifySrcReq(TmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         TmapClientBroadcastSrcType bcastSrcType,
                                         uint16 syncHandle,
                                         uint8 sourceAdvSid,
                                         uint8 paSyncState,
                                         uint16 paInterval,
                                         uint8 numBrcastSinkInfo,
                                         TmapClientBroadcastSinkInfo* brcastSinkInfo,
                                         uint8 numbSubGroups,
                                         TmapClientSubgroupInfo *subgroupInfo);

/*!
     @brief This API used to remove Broadcast Audio stream information of the Broadcast
           source to the connected Broadcast Sink device.
           This API should be called only when PA sync state is in NOT_SYNCHRONIZED state.

     @param profileHandle profile handle of TMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink

     @return TMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM confirmation message will be
             sent to application.
*/
void TmapClientBroadcastAsstRemoveSrcReq(TmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint8 numBrcastSinkInfo,
                                         TmapClientBroadcastSinkInfo* brcastSinkInfo);

/*!
     @brief This API sends broadcast code to Broadcast Sink synced to an encrypted
            Non-collocated Broadcast Source added using TmapClientBroadcastAsstAddSrcReq()
            A response to TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND will be received.

            Note: this indication is received only in case of non-collocated Source
            cases and also this API must be called for non-collocated cases only.

     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Connection ID
     @param broadcastcode Value of Broadcast Code to set in case of non-collocated source
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink

     @return None.
*/
void TmapClientBroadcastAsstSetCodeRsp(ServiceHandle groupId,
                                       uint32  cid,
                                       uint8* broadcastcode,
                                       uint8 numBrcastSinkInfo,
                                       TmapClientBroadcastSinkInfo* brcastSinkInfo);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif /* TMAP_CLIENT_LIB_H */

