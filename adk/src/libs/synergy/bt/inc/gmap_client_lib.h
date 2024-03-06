/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #56 $
******************************************************************************

FILE NAME
    gmap_client_lib.h
    
DESCRIPTION
    Header file for the Gaming audio profile (GMAP) APIs.
*/

#ifndef GMAP_CLIENT_LIB_H
#define GMAP_CLIENT_LIB_H

#include "gmap_client_prim.h"

/*!
    @brief Initialises the Gatt GMAP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt GMAP Client library.

    @param appTask           The Task that will receive the messages sent from this profile library
    @param clientInitParams   Initialisation parameters
    @param deviceData         GMAS service handles
    NOTE: A GMAP_CLIENT_INIT_CFM with GmapClientStatus code equal to GMAP_CLIENT_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed either GMAP_CLIENT_INIT_CFM will be received with a GmappStatus
          that indicates the result of the initialisation or GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND  with GmapStatus is received
          only in case of source songle when remote doesn't support gmas server.
*/
void GmapClientInitReq(AppTask appTask,
                       GmapClientInitData *clientInitParams,
                       GmapClientHandles  *deviceData);

/*!
    @brief This API is used to read the role

    @param profileHandle  The Profile handle.

    NOTE: A GMAP_CLIENT_READ_ROLE_CFM message will be sent to the registered application Task.

*/

void GmapClientReadRoleReq(GmapClientProfileHandle profileHandle);

/*!
    @brief This API is used to read unicast features based on given role

    @param profileHandle  The Profile handle of GMAP.
    @param role Unicast role i.e. either GMAP_ROLE_UNICAST_GAME_GATEWAY or GMAP_ROLE_UNICAST_GAME_TERMINAL

    NOTE: A GMAP_CLIENT_READ_UNICAST_FEATURES_CFM message will be sent to the registered application Task.

*/
void GmapClientReadUnicastFeaturesReq(GmapClientProfileHandle profileHandle, uint8 role);

/*!
    @brief This API is used to read broadcast features based on given role

    @param profileHandle  The Profile handle of GMAP.
    @param role Unicast role i.e. either GMAP_ROLE_BROADCAST_GAME_SENDER or GMAP_ROLE_BROADCAST_GAME_RECEIVER

    NOTE: A GMAP_CLIENT_READ_BROADCAST_FEATURES_CFM message will be sent to the registered application Task.

*/
void GmapClientReadBroadcastFeaturesReq(GmapClientProfileHandle profileHandle, uint8 role);

/*!
    @brief This API is used to read Service attribute handles of GMAS.

    @param profileHandle  profile handle of GMAP

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately.

*/
GattGmasClientDeviceData* GmapClientGetDevicedata(GmapClientProfileHandle profileHandle);

/*!
    @brief This API is used to add second device to the co-ordinated group.

    @param profileHandle  profile handle of GMAP
	@param cid ConnectionId of the device to be added

    @return True on successful addition else False.

*/
bool GmapClientAddNewDevice(GmapClientProfileHandle profileHandle,
                            uint32 cid);

/*!
    @brief This API is used to remove device from the co-ordinated group.

    @param profileHandle  profile handle of GMAP
	@param cid ConnectionId of the device to be added

    @return True on successful removal else False.

*/
bool GmapClientRemoveDevice(GmapClientProfileHandle profileHandle,
                            uint32 cid);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the GmapInitReq API.

    @param profileHandle profile handle of GMAP

    NOTE: A GMAP_CLIENT_DESTROY_CFM with GmapClientStatus code equal to GMAP_CLIENT_STATUS_IN_PROGRESS will be received as indication
          that the profile library destroy started. Once completed GMAP_CLIENT_DESTROY_CFM will be received with a GmapClientStatus that indicates the
          result of the destroy.
*/
void GmapClientDestroyReq(GmapClientProfileHandle profileHandle);

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*!
    @brief Registers profiles with CAP for indications/notification that 
    profile is interested in handling

    @param groupId returned in CAP_CLIENT_INIT_CFM
    @param profileHandle profile handle of GMAP Client
*/

void GmapClientRegisterToCapReq(ServiceHandle groupId,
                               GmapClientProfileHandle profileHandle);


/*!
    @brief Establishes Unicast connection to Remote Devices.
    Application recieves GMAP_CLIENT_UNICAST_CONNECT_CFM when this message is called

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param sinkConfig  The audio config Codec settings interest which is discovered using
                       CapDiscoverStreamCapabilitiesReq for Sink
    @param srcConfig   The audio config Codec settings interest which is discovered using
                       CapDiscoverStreamCapabilitiesReq for Source
    @param targetLatency Target/transport latency
    @param codecId  Codec Id of the Capability we are configuring
    @param sinkAudioLocations      array of audio locations to be associated with sink stream.
    @param srcAudioLocations      array of audio locations to be associated with src stream.   
    @param numOfMic     Number of Mic input desired by application.Actual value of mic used
                        by GMAP layer will be send by GMAP layer in GMAP_UNICAST_CONNECT_CFM
                        based on remote device stream capabilty.
                        This value needs to be set to 0 if no MIC path is required by application
    @param cigConfigMode   Default or QHS mode. In Default mode, CAP will use internal parameters
                           In QHS mode, application can configure some of CIG params by passing GmapClientQhsConfig.
                           When joint stereo mode bit is set GMAP expects only 2 bits to be set in sinkAudioLocations/ srcAudioLocations
    @param cigConfig       config param for CIG in QHS mode. Applicable only if cigConfigMode is set to QHS mode. 

    Note : Recommended SinkConfig values are: 
       GMAP_CLIENT_STREAM_CAPABILITY_32_1, GMAP_CLIENT_STREAM_CAPABILITY_32_2
       GMAP_CLIENT_STREAM_CAPABILITY_48_1, GMAP_CLIENT_STREAM_CAPABILITY_48_2
       GMAP_CLIENT_STREAM_CAPABILITY_48_3, GMAP_CLIENT_STREAM_CAPABILITY_48_4,
       
       
       Recommended SourceConfig values are: 
       GMAP_CLIENT_STREAM_CAPABILITY_32_1, GMAP_CLIENT_STREAM_CAPABILITY_32_2
       GMAP_CLIENT_STREAM_CAPABILITY_48_1, GMAP_CLIENT_STREAM_CAPABILITY_48_2

*/
void GmapClientUnicastConnectReq(GmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId,
                                 GmapClientStreamCapability sinkConfig,
                                 GmapClientStreamCapability srcConfig,
                                 GmapClientTargetLatency targetLatency,
                                 uint32 sinkAudioLocations,
                                 uint32 srcAudioLocations,
                                 uint8 numOfMic,
                                 GmapClientCigConfigMode cigConfigMode,
                                 const GmapClientQhsConfig* cigConfig);

/*!
    @brief Establishes Unicast Stream start to Remote Devices.
    Application recieves GMAP_CLIENT_UNICAST_START_STREAM_CFM when this message is called

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param metadataLen Length of metadata being passed
    @param metadata metadata in LTV format and application need to free this pointer.
*/
void GmapClientUnicastStartStreamReq(GmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     uint8 metadataLen,
                                     const uint8* metadata);

/*!
    @brief Upadates Unicast Audio Stream .
    Application recieves GMAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM when this message is called

    Note:This procedure shall be called only in Straming state to switch the 
    Streaming Audio context metadata for which matches existing audio configuration

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param metadataLen Length of metadata  parameter
    @param metadata metadata in LTV format for Streaming Audio context.
*/
void GmapClientUnicastUpdateAudioReq(GmapClientProfileHandle profileHandle,
                                     ServiceHandle groupId,
                                     uint8 metadataLen,
                                     const uint8* metadata);

/*!
    @brief Stops Established Unicast Stream.
    Application recieves GMAP_CLIENT_UNICAST_STOP_STREAM_CFM when this message is called

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param doRelease  Whether the stream need to be released or not.
*/
void GmapClientUnicastStopStreamReq(GmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    bool doRelease);

/*!
    @brief Removes the configuration done for an Context.
    Application recieves a GMAP_CLIENT_UNICAST_DISCONNECT_CFM when this API is called.

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
*/
void GmapClientUnicastDisconnectReq(GmapClientProfileHandle profileHandle,
                                    ServiceHandle groupId);

/*!
    @brief Mutes/Unmutes the Device connected
    Application recieves GMAP_CLIENT_MUTE_CFM when this message is called

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param mute Mute state
*/
void GmapClientMuteReq(GmapClientProfileHandle profileHandle,
                       ServiceHandle groupId,
                       bool mute);

/*!
    @brief Sets the absolute voulme state of Device connected
    Application recieves GMAP_CHANGE_VOLUME_CFM when this message is called

    @param profileHandle profile handle of GMAP
    @param groupId the ID of the device group which share SIRK with new device
    @param volumeSetting new volume state of the connected devices
*/
void GmapClientSetAbsVolumeReq(GmapClientProfileHandle profileHandle,
                               ServiceHandle groupId,
                               uint8 volumeSetting);

/*!
    @brief DeRegister with CAP for msgs/indications
    Application receives GMAP_CLIENT_DEREGISTER_TASK_CFM when this message is called

    @param profileHandle Profile handle of GMAP.
    @param groupId      The ID of the device group which share SIRK
*/
void GmapClientDeRegisterTaskReq(GmapClientProfileHandle profileHandle,
                                 ServiceHandle groupId);

#endif /* INSTALL_LEA_UNICAST_CLIENT */

#if defined(INSTALL_LEA_UNICAST_CLIENT) || defined(INSTALL_LEA_BROADCAST_SOURCE)
/*!
    @brief This API is used to overwrite standard CIG/BIG params with user defined values for a particular
           usecase before calling of GmapClientUnicastConnectReq() or GmapClientBroadcastSrcConfigReq().
           Application receives GMAP_CLIENT_SET_PARAMS_CFM when this API is called.

    @param profileHandle profile handle of GMAP Client for Unicast or GMAP Broadcast Source handle 
    @param groupId       the ID of the device group which share SIRK with new device. 
                         Incase of broadcast it can be profileHandle.
    @param sinkConfig    Sink CIG config params app wants to apply on usecase to be established
                         using GmapClientUnicastConnectReq()
    @param srcConfig     Source CIG config params app wants to apply on usecase to be established
                         using GmapClientUnicastConnectReq()
    @param type          GMAP_CLIENT_PARAMS_TYPE_UNICAST_CONNECT or GMAP_CLIENT_PARAMS_TYPE_BROADCAST_CONNECT
    @param param         Application supplied parameters corresponding to config. 
                         For Unicast use GmapClientUnicastConnectParams and
                         for broadcast use GmapClientBroadcastConfigParams

    @return None

    NOTE: In case of broadcast sinkConfig, srcConfig values will be ignored, it is advised to fill
          GMAP_CLIENT_STREAM_CAPABILITY_UNKNOWN in these parameters in case of Broadcast.

*/
void GmapClientSetParamsReq(GmapClientProfileHandle profileHandle,
                            ServiceHandle groupId,
                            GmapClientStreamCapability sinkConfig,
                            GmapClientStreamCapability srcConfig,
                            GmapClientParamsType type,
                            uint8 numOfParamsElems,
                            const void* paramsElems);
#endif /* #if defined(INSTALL_LEA_UNICAST_CLIENT) || defined(INSTALL_LEA_BROADCAST_SOURCE) */

#ifdef INSTALL_LEA_BROADCAST_SOURCE
/*!
    @brief Initialises the GMAP Broadcast Source.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_INIT_CFM when this API is called

    @param appTask The Task that will receive the messages sent from this library.
*/
void GmapClientBroadcastSrcInitReq(AppTask appTask);

/*!
    @brief Deinitialises the GMAP Broadcast Source.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_DEINIT_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
*/
void GmapClientBroadcastSrcDeinitReq(GmapClientProfileHandle brcstSrcProfileHandle);

/*!
    @brief Configures the GMAP Broadcast Source.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_CONFIG_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param ownAddrType Address type of the local device
    @param presentationDelay Presentation delay to synchronize the presentation
                             of multiple BISs in a BIG.
    @param numSubgroup Total number of sub groups in the broadcast group. 
                                  This field value should be non-zero and it should be atleast 0x01 
    @param subgroupInfo Sub group related information in CapBigSubgroups format.This field should not be NULL
                        and subgroupInfo->numBis value should be non-zero and it should be atleast 0x01.
                        Please note if common codec configurations parameters(config, targetLatency, lc3BlocksPerSdu) are
                        provided(i.e. non-zero) at both level-2 and level-3 then parameters provided at level-3 will be
                        overwritten by parameters provided at level-2. Recommended value for common codec configurations
                        parameters at level-3 is zero.
    @param brcstSrcNameLen   length of the broadcast source name
    @param brcstSrcName   Broadcast source name
    @param bigConfigMode   Default or QHS mode. In Default mode, CAP will use internal parameters
                           In QHS mode, application can configure some of BIG params by passing GmapClientQhsBigConfig.
                           If Joint Stereo mode is set then Broadcast Source will always configure for Audio Configuration 14
                           (Multiple Audio Channels, 1 BIS). Application needs to ensure that valid subgroupInfo for
                           Audio Configuration 14 is passed in this mode.
    @param qhsBigConfig       config param for BIG in QHS mode. Applicable only if cigConfigMode is set to QHS mode.

    Note : Recommended config GmapClientStreamCapability values are 
       GMAP_CLIENT_STREAM_CAPABILITY_48_1, GMAP_CLIENT_STREAM_CAPABILITY_48_2,
       GMAP_CLIENT_STREAM_CAPABILITY_48_3, GMAP_CLIENT_STREAM_CAPABILITY_48_4
*/
void GmapClientBroadcastSrcConfigReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                     uint8 ownAddrType,
                                     uint32 presentationDelay,
                                     uint8 numSubgroup,
                                     const GmapClientBigSubGroup* subgroupInfo,
                                     uint8 brcstSrcNameLen,
                                     const uint8* brcstSrcName,
                                     GmapClientBigConfigMode bigConfigMode,
                                     const GmapClientQhsBigConfig *qhsBigConfig);

/*!
    @brief Start the streaming of GMAP Broadcast Source.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_START_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param encryption   Whenther to encrypt BIG stream or not
    @param broadcastCode 16 octets code to encrypt the BIG stream if parameter encryption is TRUE else NULL.

*/
void GmapClientBroadcastSrcStartStreamReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                          bool  encryption,
                                          const uint8* broadcastCode);

/*!
    @brief Updates metadata of the GMAP  Broadcast Source when in streaming state.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_UPDATE_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param useCase  Streaming audio context value for broadcast usecase
    @param numSubgroup  number of subgroups requires updated for an existing broadcast source
    @param metadataLen  length of metadata
    @param metadata     metadata shall be in LTV format as per Generic Audio specification.

*/
void GmapClientBroadcastSrcUpdateStreamReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                           GmapClientContext useCase,
                                           uint8_t numSubgroup,
                                           uint8 metadataLen,
                                           const uint8* metadata);

/*!
    @brief Stop the streaming of GMAP Broadcast Source.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_STOP_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast SSourcerc Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.

*/
void GmapClientBroadcastSrcStopStreamReq(GmapClientProfileHandle brcstSrcProfileHandle);

/*!
    @brief Remove the GMAP Broadcast Source stream and audio announcements.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_REMOVE_STREAM_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.

*/
void GmapClientBroadcastSrcRemoveStreamReq(GmapClientProfileHandle brcstSrcProfileHandle);

/*!
    @brief Configures the GMAP Broadcast Source based on big test parameters.
    Application recieves GMAP_CLIENT_BROADCAST_SRC_BIG_TEST_CONFIG_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param useCase  Streaming audio context value for broadcast usecase
    @param bigTestConfigParam Configuration parameters for BIG Test
    @param numBis Number of Bis required in this BIG
    @param subgroupInfo Sub group related information in CapBigSubgroups format.
    @param bigConfigMode   Default or QHS mode. In Default mode, CAP will use internal parameters
                           In QHS mode, application can configure some of BIG params by passing GmapClientQhsBigConfig.
    @param qhsBigConfig       config param for BIG in QHS mode. Applicable only if cigConfigMode is set to QHS mode.

*/
#if 0
void GmapClientBroadcastSrcBigTestConfigReq(uint32 brcstSrcProfileHandle,
                                            GmapClientContext useCase,
                                            GmapClientBigTestConfigParam *bigTestConfigParam,
                                            uint8 numBis,
                                            GmapClientBigConfigMode bigConfigMode,
                                            const GmapClientQhsBigConfig *qhsBigConfig);
#endif
/*!
    @brief Sets the periodic advertisement parameters for GMAP Broadcast Source.
    It is synchronous API

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of GMAP_CLIENT_BROADCAST_SRC_INIT_CFM.
    @param srcAdvPaParams    Periodic Advertisement parameters to set.

*/
void GmapClientBroadcastSrcSetAdvParamsReq(GmapClientProfileHandle brcstSrcProfileHandle,
                                           const GmapClientBcastSrcAdvParams *srcAdvPaParams);
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/*!
*
*  NOTE: Before Calling broadcast Assistant Procedure make sure to Call
*        CapClientStreamAndContrilInitReq from Application. Only after
*        successful initiation of Stream and Control Procedure,
*        the CAP Broadcast Assistant Procedures can be excercised.
*/

#if 0
/*!
     @brief Sets the broadcast source scan parameters. This API is optional
            and if not invoked default parameters values for scan will be used	 

     @param scanningPhy Bit Field for LE PHYs to be used for Scanning
     @param phys   Scan Parameter Values to be used for each LE PHYs
     @param flags Extended scanning with a filter that only matches extended 
                advertising reports that contains sync info for a periodic train

                bits 0-1: Flags AD structure filter defines with 
                0 = Report all
                1 = Report advertising reports with flags AD type
                2 = Report advertising reports with flags AD type 
                    with LE Limited Discoverable Mode set or 
                    LE General Discoverable Mode set.
                3 = Report advertising reports with flags AD type
                    with LE Limited Discoverable Mode set

               bits 2-7: Reserved for future use always set to 0.
     @param scanDuration Scanning period in seconds.

     @return None
*/
void GmapClientBroadcastAsstSetSrcScanParams(uint16 scanningPhy,
                                             ScanningPhyType *phys,
                                             uint32 flags,
                                             uint16 scanDuration);
#endif
/*!
     @brief Broadcast Assistant starts scan for broadcast audio announcement that meet a 
            specified scan filter.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid btConnid for the device which is currently written to.
     @param bcastSrcType Broadcast Source is local device or not
     @param audioFilterContext Looks for particular SRC's meeting a specified filter(s) eg:media
     @param scanFlags Scanning flags
     @param ownAddressType Local address type that shall be used for scanning
     @param scanningFilterPolicy Scan filter policy to be used for scanning

     @return GMAP_CLIENT_BROADCASTST_ASST_START_SRC_SCAN_CFM confirmation message will be 
      sent to application when search for periodic train is started or failed.
      GMAP_CLIENT_BROADCAST_ASST_SRC_REPORT_IND will be sent to
      application on receiving reports from broadcast sources.
 */
void GmapClientBroadcastAsstStartSrcScanReq(GmapClientProfileHandle profileHandle,
                                            ServiceHandle groupId,
                                            uint32 cid,
                                            GmapClientBroadcastSrcType bcastSrcType,
                                            GmapClientContext audioFilterContext,
                                            uint8 scanFlags,
                                            uint8 ownAddressType,
                                            uint8 scanningFilterPolicy);

/*!
     @brief Broadcast Assistant stops scan for broadcast audio announcement of a given scan handle

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid btConnid for the device which is currently written to.
     @param scanHandle Scan handle passed as part of GMAP_CLIENT_BROADCASTST_ASST_START_SRC_SCAN_CFM

     @return GMAP_CLIENT_BROADCASTST_ASST_STOP_SRC_SCAN_CFM confirmation message will be 
      sent to application when search for periodic train is stopped or failed.
 */
void GmapClientBroadcastAsstStopSrcScanReq(GmapClientProfileHandle profileHandle,
                                           ServiceHandle groupId,
                                           uint32 cid,
                                           uint16 scanHandle);

/*!
     @brief Writes the Broadcast Sink Receiver State CCCDs in order to
            subscribe/unsubscribe GATT notifications.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID . If cid is zero notifications will be
                enabled for all sources.
     @parm sourceId Source identifier
     @param notificationEnable enable/disable the notifications on remote

     @return GMAP_CLIENT_BROADCAST_ASST_REGISTER_NOTIFICATION_CFM confirmation message
             will be sent to application.
*/

void GmapClientBroadcastAsstRegisterNotificationReq(GmapClientProfileHandle profileHandle,
                                                    ServiceHandle groupId,
                                                    uint32 cid,
                                                    uint8 sourceId,
                                                    bool noficationEnable);

/*!
     @brief Request to read the Broadcast Sink Receiver State for all sources.This API is optional
	        App anyway will receive BRS indication if registered for notifications.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID . If cid is zero BRS inidications will be
                sent for all sources.

     @return GMAP_CLIENT_BROADCAST_ASST_READ_BRS_IND(s) will be sent if cid is valid and also
             GMAP_CLIENT_BROADCAST_ASST_READ_RECEIVE_STATE_CFM confirmation message will be 
             sent to application about the status of operation.
*/
void GmapClientBroadcastAsstReadReceiverSinkStateReq(GmapClientProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32  cid);

/*!
     @brief Broadcast Assistant establishes sync to periodic train of given address.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param addrt BT address of the Broadcast Source along with type
     @param advSid Advertising SID

     @return GMAP_CLIENT_BROADCAST_ASST_START_SYNC_TO_SRC_CFM confirmation message
             will be sent to application

*/
void GmapClientBroadcastAsstStartSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              TYPED_BD_ADDR_T *addrt,
                                              uint8 advSid);

/*!
     @brief Broadcast Assistant is terminated to syncing to a periodic train.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param syncHandle sync handle of PA to be terminated

     @return GMAP_CLIENT_BROADCAST_ASST_TERMINATE_SYNC_TO_SRC_CFM confirmation
             message will be sent to application

*/
void GmapClientBroadcastAsstTerminateSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                                  ServiceHandle groupId,
												  uint16 syncHandle);

/*!
     @brief GMAP cancels establishing sync to a periodic train.
	         This API must be called for non-collocated cases only.

     @param profileHandle profile handle of GMAP
     @param groupId  CAP groupId received in CapInitCfm message on CAP Init.

     @return GMAP_CLIENT_BROADCAST_ASST_CANCEL_SYNC_TO_SRC_CFM confirmation message
             will be sent to application

*/
void GmapClientBroadcastAsstCancelSyncToSrcReq(GmapClientProfileHandle profileHandle,
                                               ServiceHandle groupId);

/*!
     @brief This API used to Add Broadcast Sink information to the source
            via remote connected Broadcast Assistant.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid handle of BAP instance(btConnId) of  connected device being
                      synced to Broadcast Source
     @param sourceAddrt Advertiser_Address for the Broadcast Source along with type
     @param bcastSrcType Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising SID
     @param paSyncState PA Synchronization state
     @param paInterval SyncInfo field Interval parameter value
     @param broadcastId Identifier of Broadcast Source's BIG, obtained as a part of
                        Extended Adv
     @param numbSubGroups Number of subgroups  present in the BIG as defined by
                          the Num_Subgroups parameter of the BASE.
     @param subgroupInfo Pointer to BIG first subgroupInfo pointed by
                         numbSubGroups in the BIG

     @return GMAP_CLIENT_BROADCAST_ASST_ADD_SRC_CFM message will be
             sent to application.

*/
void GmapClientBroadcastAsstAddSrcReq(GmapClientProfileHandle profileHandle,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      TYPED_BD_ADDR_T *sourceAddrt,
                                      GmapClientBroadcastSrcType bcastSrcType,
                                      uint16 syncHandle,
                                      uint8 sourceAdvSid,
                                      uint8 paSyncState,
                                      uint16 paInterval,
                                      uint32 broadcastId,
                                      uint8 numbSubGroups,
                                      GmapClientSubgroupInfo *subgroupInfo);

/*!
     @brief This API modifies Broadcast Sink information via remote connected Broadcast Assistant.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param bcastSrcType Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising SID
     @param paSyncState PA Synchronization state
     @param paInterval SyncInfo field Interval parameter value
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink
     @param numbSubGroups Number of subgroups  present in the BIG as defined by
                          the Num_Subgroups parameter of the BASE.
     @param subgroupInfo Pointer to BIG first subgroupInfo pointed by
                         numbSubGroups in the BIG.
                         App needs to free this pointer.

     @return GMAP_CLIENT_BROADCAST_ASST_MODIFY_SRC_CFM message will be
             sent to application.

*/
void GmapClientBroadcastAsstModifySrcReq(GmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         GmapClientBroadcastSrcType bcastSrcType,
                                         uint16 syncHandle,
                                         uint8 sourceAdvSid,
                                         uint8 paSyncState,
                                         uint16 paInterval,
                                         uint8 numBrcastSinkInfo,
                                         GmapClientBroadcastSinkInfo* brcastSinkInfo,
                                         uint8 numbSubGroups,
                                         GmapClientSubgroupInfo *subgroupInfo);

/*!
     @brief This API removes Broadcast Sink information associated with Broadcast Source 
            via remote connected Broadcast Assistant.
            This API should be called only when PA sync state is in NOT_SYNCHRONIZED state.

     @param profileHandle profile handle of GMAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink

     @return GMAP_CLIENT_BROADCAST_ASST_REMOVE_SRC_CFM confirmation message will be
             sent to application.
*/
void GmapClientBroadcastAsstRemoveSrcReq(GmapClientProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint8 numBrcastSinkInfo,
                                         GmapClientBroadcastSinkInfo* brcastSinkInfo);

/*!
     @brief This API sends broadcast code to Broadcast Sink associated with Broadcast Source via remote
            connected Broadcast Assistant as a response to TMAP_CLIENT_BROADCAST_ASST_SET_CODE_IND.
            Please note this indication is received only in case of non-collocated cases and also this
            API must be called for non-collocated cases only.

     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Connection ID
     @param broadcastcode Value of Broadcast Code to set for non-collocated case.
     @param numBrcastSinkInfo Number of broadcast sink info provided
     @param brcastSinkInfo Structure containing cid and sourceId of broadcast sink

     @return None.
*/
void GmapClientBroadcastAsstSetCodeRsp(ServiceHandle groupId,
                                       uint32  cid,
                                       uint8* broadcastcode,
                                       uint8 numBrcastSinkInfo,
                                       GmapClientBroadcastSinkInfo* brcastSinkInfo);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#endif /* GMAP_CLIENT_LIB_H */

