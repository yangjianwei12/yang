
/*******************************************************************************

Copyright (C) 2021 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef CAP_CLIENT_LIB_H
#define CAP_CLIENT_LIB_H

#include "cap_client_prim.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*!
    @brief Initialises the CAP Client.
    Application recieves CAP_CLIENT_INIT_CFM with groupId which can be
    associated with devices(s). If the application wants to associate multiple
    devices in a group then it can call CapClientAddNewDevReq() API with
    same groupId received for the first device.

    @param appTask The Task that will receive the messages sent from the
                   CAP client  library.
    @param initData  As defined in CapClientInitData.
                     prefer application to free the pointer.
    @param role CAP role as CAP_CLIENT_INITIATOR or CAP_CLIENT_COMMANDER

    NOTE: 1. This API needs to be called only once per groupId
          2. CAP supports only 2 CAP groups
*/

void CapClientInitReq(AppTask appTask, CapClientInitData *initData,
                             CapClientRole role);

/*!
    @brief Removes a device from CAP Client group and
    Application receives CAP_REMOVE_DEV_CFM when this message is called

    @param groupId the ID of the device group which share SIRK with new device
    @param cid Gatt connection ID of device which is being disconnected. 
           If whole group information is to be removed then cid passed
           can be set 0

    NOTE: It does a deInit of CAP module if this API is called with cid 0 or
          if there are only 1 device in the group.
*/

void CapClientRemoveDevReq(ServiceHandle groupId, uint32 cid);

/*!
    @brief Adds the new device to the existing groupId.
    Application recieves CAP_ADD_DEV_CFM when this message is called

    @param groupId the ID of the device group which share SIRK with new device
    @param initData  As defined in CapClientInitData.
                     prefer application to free the pointer.
    @param discoveryComplete To indicate CAP layer if discovery is complete or
                     there are more devices to add in the existing group.

    NOTE: This API need not be called if there is only one device in Co-ordinated Set
*/

void CapClientAddNewDevReq(ServiceHandle groupId, CapClientInitData *initData,
                                     bool discoveryComplete);

/*!
    @brief Initialises the VCP/BAP unicast and/or broadcast role within CAP library.
    Application recieves CAP_INIT_STREAM_AND_CONTROL_CFM when this message is called

    Application should ensure that it adds all the devices in the group before calling
    this API.

    @param groupId the ID of the device group which share SIRK
*/


void CapClientInitStreamControlReq(ServiceHandle groupId);

/*!
    @brief Discover Audio capabilities on Remote device(s).
    Application recieves CAP_DISCOVER_STREAM_CAPABILITIES_CFM when this api is called.


    @param groupId the ID of the device group which share SIRK
    @param attribute, what attribute is being read sink/source PAC record/location or Context
*/

void CapClientDiscoverStreamCapabilitiesReq(ServiceHandle groupId,
                                             CapClientPublishedCapability attribute);
/*!
    @brief Discover Available Audio Context on Remote device(s).
    Application recieves CAP_DISCOVER_AVAILABLE_AUDIO_CONTEXT_CFM when this api is called.

    @param groupId the ID of the device group which share SIRK

    Note: Whenever the PAC record changes on remote side, CAP indicates the change
          to above layer by sending CAP_CLIENT_PAC_RECORD_CHANGED_IND.
          Application needs to trigger CapClientDiscoverStreamCapabilitiesReq() again to 
          rediscover the PAC records again.
*/

void CapClientDiscoverAvailableAudioContextReq(ServiceHandle groupId);

/*!
*
*  NOTE: CAP Unicast Connect, Start Stream, Update Audio, Stop Stream Req also
*        falls into group of procedures which needs to be excercised by upper
*        layer profiles (HAP, TMAP etc.) registered with CAP layer instead of
*        application. If no layer is registered the messages will be forwarded
*        to application. The APIs which follow the scheme are marked with a
*        note indicating this.
*/

/*!
    @brief Establishes Unicast connection from CAP Client to Remote Devices.
    Application recieves CAP_CLIENT_UNICAST_CONNECT_CFM when this api is called.

    @param groupId     The ID of the device group which share SIRK with new device
    @param sinkConfig  The Codec/QOS config of interest which is discovered using
                       CapClientDiscoverStreamCapabilitiesReq.
                       Set this to CAP_STREAM_CAPABILITY_UNKNOWN if SINK ASEs 
                       are not required.
    @param srcConfig   The Codec/QOS config of interest which is discovered using
                       CapClientDiscoverStreamCapabilitiesReq.
                       Set this to CAP_STREAM_CAPABILITY_UNKNOWN if SOURCE ASEs
                       are not required.
    @param targetLatency   Target latency required for audio usecase
    @param useCase         Whether we are configuring the Server for 
                           Voice/Media/Gaming
    @param sinkAudioLocations   bitmask audio locations to be associated with stream.
                                Number of bits set has to be equal to number of
                                Sink ASEs getting configured.
                                If 0, AudioLocation will be configured to MONO based on 
                                context.
    @param srcAudioLocations    bitmask audio locations to be associated with stream.
                                Number of bits set has to be equal to number of
                                Source ASEs getting configured.
                                If 0, AudioLocation will be configured to MONO based on
                                context.
    @param numOfMic        Number of Mic input desired by application, actual
                           value of mic used by CAP layer will be send by CAP
                           layer in CAP_CLIENT_UNICAST_CONNECT_CFM based on 
                           remote device stream capabilty.
                           This value needs to be set to 0 if no MIC path
                           is required by application

    @param cigConfigMode   Default or QHS mode and/ or Joint Stereo mode. In Default mode, CAP will
                           use internally use parameters for CIG configuration.
                           In QHS mode, application can configure some of CIG
                           params by passing CapClientQhsConfig. When joint stereo mode bit is set
                           CAP expects only 2 bits to be set in sinkAudioLocations/ srcAudioLocations
    @param cigConfig       config param for CIG in QHS mode. Applicable only
                           if cigConfigMode is set QHS mode.

     NOTE: 
         a) This API Prefereably needs to be called by Profiles(TMAP, HAP etc.)
         b) Please refer to below table for configuring different usecases

            |USECASE      | TARGET LATENCY                         |
            |------------------------------------------------------|
            | Game        | low latency                            |
            | Call        | Target balanced latency and reliability|
            | Music/SREC  | Target high reliability                |
            |------------------------------------------------------|
         c) If CAP Returns CAP_CLIENT_RESULT_INSUFFICIENT_RESOURCES when this particular
            api is called, then application needs to subsequently call CapClientUnicastDisconnectReq
            to free the resources and then excercise this api.
         d) Application or profile using this api must call CapClientSetParamReq prior calling
            CapClientUnicastConnectReq if they want to override default BAP specification values.
		 e) For dynamic addition of set member(s) in a group at later stage(where second device is catching the first device at a later stage but not immediately),
            then application needs to feed sink/src audio location and number of mic based on the set members which are currently connected in the group
            (When all the set members are connected in the group immediately, then these values have to be set for the group, otherwise
            if partial members are connected then application needs to feed these values accordingly only for the currently connected member(s) for the group)
*/

void CapClientUnicastConnectReq(AppTask profileTask,
                             ServiceHandle groupId,
                             CapClientSreamCapability sinkConfig,
                             CapClientSreamCapability srcConfig,
                             CapClientTargetLatency targetLatency,
                             CapClientContext useCase,
                             CapClientAudioLocation sinkAudioLocations,
                             CapClientAudioLocation srcAudioLocations,
                             uint8  numOfMic,
                             CapClientCigConfigMode cigConfigMode,
                             const CapClientQhsConfig *cigConfig);

/*!
    @brief Removes the configuration done for an Context.
    Application recieves a CAP_CLIENT_UNICAST_DISCONNECT_CFM when this API is called.

    @param appTask The Task that will receive the messages sent from this library
    @param groupId the ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media/Gaming

    NOTE: This API can be excercised by either Application or by profiles(TMAP, HAP etc.)

*/

void CapClientUnicastDisConnectReq(AppTask profileTask,
                                ServiceHandle groupId,
                                CapClientContext useCase);

/* Enable, CIS connect, Start ready */
/*!
    @brief Establishes Unicast Stream start from CAP Client to Remote Devices.
    Application recieves  one CAP_CLIENT_UNICAST_START_STREAM_IND for each connected
    devices in the group and will recieve one final CAP_CLIENT_UNICAST_START_STREAM_CFM.
    App shall perform creation of Audio graph only when CAP_CLIENT_UNICAST_START_STREAM_CFM
    is recieved with status as CAP_CLIENT_RESULT_SUCCESS and valid cis paramerters recieved
    as part of CAP_CLIENT_UNICAST_START_STREAM_IND.

    NOTE:In case of Bidirectional usecase CAP will configure bidirectional CIS's with Remote.

    @param profileTask The Task that will receive the messages sent from this library
    @param groupId  ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media/Gaming
    @param metadataParam  Metadata Params used to asscoiate a stream with Context
                          and CCID. Application/profile needs to free this pointer

    NOTE: This API Prefereably needs to be called by Profiles(TMAP, GMAP, AHP, HAP etc.)

*/


void CapClientUnicastStartStreamReq(AppTask profileTask,
                             ServiceHandle groupId,
                             CapClientContext useCase,
                             uint8 metadataLen,
                             uint8 *metadataParam);

/*!
    @brief Upadates Unicast Stream with new metadata.
    Application recieves CAP_CLIENT_UNICAST_UPDATE_AUDIO_CFM when this message is called

    Note:This procedure is only used for in use QOS config which supports multiple contexts.
         To change Config use stop procedure followed by start

    @param appTask       The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device
    @param metadataParam  Metadata Params used to asscoiate a stream with Context and CCID
                          App/profile needs to free this pointer.

    NOTE: This API Prefereably needs to be called by Profiles(TMAP, HAP etc.)
*/

void CapClientUnicastUpdateAudioReq(AppTask profileTask,
                             ServiceHandle groupId,
                             CapClientContext useCase,
                             uint8 metadataLen,
                             uint8* metadataParam);

/*!
    @brief Stops Established Unicast Stream.
    Application recieves CAP_CLIENT_UNICAST_STOP_STREAM_CFM when this API is called

    Note: Here all the ASE's associated with the groupId needs to be disabled/released

    @param appTask       The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device
    @param doRelease  Should all the ASEs to be released of the resources.

    NOTE: This api Prefereably needs to be called by Profiles(TMAP, HAP etc.)only when stream is active.

*/

void CapClientUnicastStopStreamReq(AppTask profileTask, ServiceHandle groupId,
                             bool doRelease);

/*!
    @brief Sets the VS config to be used when datapath is setup using
    CapClientUnicastConnectReq() API. Calling this API multiple times will
    overwrite the existing VS config. This API should be followed by
    CapClientUnicastConnectReq() API.

    Application recieves CAP_CLIENT_UNICAST_SET_VS_CONFIG_DATA_CFM when this API is called

    @param appTask The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device
    @param vsConfigLen VS config Params Length
    @param vsConfig  VS config Params
*/
void CapClientUnicastSetVsConfigDataReq (AppTask profileTask, ServiceHandle groupId,
                             uint8 vsConfigLen, const uint8 *vsConfig);

/* Volume Controller Procedures*/

/*!
    @brief Mutes the Device connected
    Application recieves CAP_MUTE_CFM when this API is called

    @param appTask  The Task that will receive the messages sent from this lib.
    @param groupId the ID of the device group which share SIRK with new device
    @param mute Mute state
*/

void CapClientMuteReq(AppTask profileTask, ServiceHandle groupId, bool mute);

/*!
    @brief Changes the volume state of Device connected
    Application recieves CAP_CLIENT_CHANGE_VOLUME_CFM when this API is called

    @param appTask       The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device
    @param volumeSetting new volume state of the connected devices
*/

void CapClientChangeVolumeReq(AppTask profileTask, ServiceHandle groupId,
                             uint8 volumeSetting);

/*!
    @brief Registers the upper layer profile with CAP
    Application receives CAP_CLIENT_REGISTER_TASK_CFM when this API is called

    @param profileTask The Task that will receive the messages sent from this library.
    @param groupId     The ID of the device group which share SIRK
*/

void CapClientRegisterTaskReq(AppTask profileTask, ServiceHandle groupId);


/*!
    @brief DeRegister a upper layer profile/application with CAP
    Application receives CAP_CLIENT_DEREGISTER_TASK_CFM when this message is called

    @param profileTask The Task that will receive the messages sent from this library.
    @param groupId     The ID of the device group which share SIRK
*/

void CapClientDeRegisterTaskReq(AppTask profileTask, ServiceHandle groupId);

/*!
    @brief Read the csip read characeristics
    Application receives CAP_CLIENT_CSIP_READ_CFM when this API is called

    @param groupId the ID of the device group which share SIRK with new device.
    @param cid btConnid for the device which is currently written to.
    @param csipCharType csip read characteristic type
*/
void CapClientCsipReadReq(ServiceHandle groupId, uint32  cid, CapClientCsipType csipCharType);

/*!
    @brief Read the Volume state characeristics
    Application receives CAP_CLIENT_READ_VOLUME_STATE_CFM when this API is called

    @param profileTask The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device.
    @param cid btConnid for the device which is currently been read.
*/

void CapClientReadVolumeStateReq(AppTask profileTask, ServiceHandle groupId, uint32 cid);

/*!
    @brief This API is used to retrieve the client profile characteristic and 
           descriptor handles stored by the profile library during discovery
           procedure.

    @param cid Connection ID
    @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
    @param clientProfile Interested Profile.

    @return void*
    NOTE: This is not a message passing based API, the handles, if found,
          will be returned immediately to the app.

*/
void *CapClientGetProfileAttributeHandles(ServiceHandle groupId, uint32 cid,
                                                           CapClientProfile clientProfile);


/*!
    @brief This API is used to overwrite standard CIG params with user defined
           values for test purpose for fine tuning CIG for a particular usecase
           established using CapClientUnicastConnectReq() API.
           Application receives CAP_CLIENT_UNICAST_CIG_TEST_CFM when this API
           is called.

    @param appTask The Task that will receive the messages sent from this library
    @param groupId the ID of the device group which share SIRK with new device
    @param useCase  Whether we are configuring the Server for Voice/Media/Gaming
    @param config CIG Test config params app wants to apply on usecase established
           using CapClientUnicastConnectReq()

    @return None
    NOTE: It is advised only to use standard CapClientUnicastConnectReq() for a
         particular useCase application is interested in. This API can be
         called to fine tune CIG with new params for the usecase which is 
         currently active or  configured. 
         If the API is called for inactive useCase then CAP_CLIENT_UNICAST_CIG_TEST_CFM
         with invalid parameter status shall be returned

*/
void CapClientUnicastCigTestConfigReq(AppTask profileTask,
                             ServiceHandle groupId,
                             CapClientContext useCase,
                             const CapClientCigTestConfig *config);

/*!
    @brief This API is used to retrieve the stream id (cis/bis) list

    @param groupId the ID of the device group which share SIRK with new device.
    @param id cig/big ID.
    @param flag : Unicast - CAP_CLIENT_CIG
                  Broadcast - CAP_CLIENT_BIG

    NOTE: This API is synchronous call. If cigId or bigId is valid then valid values of stream id are returned.
          If not valid then NULL is returned.
          Higher layer has to free the memory for CapClientGroupInfo and its members.

*/

CapClientGroupInfo *CapClientGetStreamInfo(ServiceHandle groupId, uint8 id, CapClientIsoGroupType flag);

/*!
    @brief Initialises the optional services e.g. MICP within CAP library.
    Application recieves CAP_CLIENT_INIT_OPTIONAL_SERVICES_CFM when this message is called

    Application should ensure that it adds all the devices in the group before calling
    this API.

    @param groupId the ID of the device group which share SIRK
    @param optServices(bit mask of the optional services which APP is intrested to use)
    refer CapClientOptionalServices bit mask in cap_client_prim.h for the defined optional services

    Note: This Interface shall be used by the Application once mandatory services dicosvery is
    completed as part of CapClientInitStreamControlReq procedure.
*/

void CapClientInitOptionalServicesReq(ServiceHandle groupId, CapClientOptionalServices optServices);

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
/*!
    @brief This API is used to set the client profile characteristic and 
           descriptor handles stored by the profile library during MICP discovery
           procedure.
           Application recieves CAP_CLIENT_SET_MICP_ATTBUTE_HANDLES_CFM 
           when this message is called

    @param profileTask The Task that will receive the messages sent from this library

    @param groupId  CAP groupId received in CapInitCfm message on CAP Init.

    @param cid Connection ID.

    @param micsHandles mics handles which got discovered as part of gatt service discovery.
    Memory deallocation for the pointer (GattMicsClientDeviceData*) shall be taken by CAP profile.

    @return None

    Note: This Interface shall be used by the Application before CapClientInitOptionalServicesReq procedure.
    The MICP handles value will set in CAP profile if the micsHandles are valid.

    1.It's not mandatory for App to wait for the confirmation and can continue other operations.
    2.If App is setting the micsHandles to NULL, CAP will perform the full discovery procedure.
    3.If the status is not successfull then CAP will perform the disocvery again as part of
    CapClientInitOptionalServicesReq procedure which will increase the connection latency during
    reconnection.

*/

void CapClientSetMicpProfileAttbuteHandlesReq(AppTask profileTask,
                                                   ServiceHandle groupId,
                                                   uint32 cid,
                                                   GattMicsClientDeviceData* micsHandles);

/*!
    @brief Mutes the Device connected
    Application recieves CAP_CLIENT_SET_MIC_STATE_CFM when this API is called

    @param appTask  The Task that will receive the messages sent from this lib.
    @param groupId the ID of the device group which share SIRK with new device.
    @param cid btConnid for the device which is currently being mute.
                Passing cid as 0 will mute all devices in the group.
    @param muteState Mute state.

    Note:For cid = 0, the  mic state setting will get applied to each device on a group on best effort and not guaranteed.
    The devices that failed to set MicState will be informed in the CFM msg
    And then its App call whether to retry for the failed device or not.

*/

void CapClientSetMicStateReq(AppTask profileTask, ServiceHandle groupId, uint32 cid, CapClientMicState micState);

/* !
    @brief Read the mic state characeristics
    Application receives CAP_CLIENT_READ_MIC_STATE_CFM when this API is called

    @param profileTask The Task that will receive the messages sent from this library.
    @param groupId the ID of the device group which share SIRK with new device.
    @param cid btConnid for the device which is currently been read.
*/

void CapClientReadMicStateReq(AppTask profileTask, ServiceHandle groupId, uint32 cid);
#endif /* EXCLUDE_CSR_BT_MICP_MODULE */

#endif /* INSTALL_LEA_UNICAST_CLIENT */

/*!
    @brief This API can be used to set custom CIG params for Unicast or BIG params for
           Broadcast with user defined values for a particular usecase before calling 
           CapClientUnicastConnectReq() for Unicast or CapClientBcastSrcConfigReq() for Broadcast.
           These values are used for for particular usecase as long as CIG or BIG is active.
           Once the CIG or BIG is destroyed then this API needs to be called again before
           CapClientUnicastConnectReq() or CapClientBcastSrcConfigReq() for a usecase is
           invoked.
           These parameters have precedence over QHS and standard CIG or BIG params.
           Application receives CAP_CLIENT_SET_PARAM_CFM when this API is called.

    @param appTask The Task that will receive the messages sent from this library
    @param profilehandle the ID of the device group which share SIRK with new device or
                         Bcast Profile handle
    @param sinkConfig  Sink CIG config params app wants to apply on usecase to be established
                       using CapClientUnicastConnectReq()
    @param srcConfig   Source CIG config params app wants to apply on usecase to be established
                       using CapClientUnicastConnectReq()
    @param type        CAP_CLIENT_PARAM_TYPE_UNI_CONNECT or CAP_CLIENT_PARAM_TYPE_BCAST_CONFIG
    @param numOfParamElem In case of Unicast this num has to be 1,
                          In case of Broadcast , this number is equal to number of subgroups.
    @param paramElems    Higher Layer supplied parameters corresponding to config type.

    @return None

    NOTE:a) In case of broadcast sinkConfig, srcConfig values will be ignored, it is advised to fill
            CAP_CLIENT_SREAM_CAPABILITY_UKNOWN in these parameters in case of Broadcast.
         b) In case of unicast for unidirectional use cases(Central to Peripheral or vice-versa),
            it is advised to set parameter values for CIG in the other direction(Peripheral to Central
            or vice-versa) same as Central to Peripheral(or vice-versa).
            sdu_size and rtn values for CIS must be set based on direction otherwise shall be set to 0.

*/

void  CapClientSetParamReq(AppTask appTask,
                           uint32 profileHandle,
                           CapClientSreamCapability sinkConfig,
                           CapClientSreamCapability srcConfig,
                           CapClientParamType type,
                           uint8 numOfParamElem,
                           const void* paramElems);

#ifdef INSTALL_LEA_BROADCAST_SOURCE

/*!
    @brief Initialises the CAP Broadcast Src.
    Application recieves CAP_CLIENT_BCAST_SRC_INIT_CFM when this API is called

    @param appTask The Task that will receive the messages sent from this library.
*/


void CapClientBcastSrcInitReq(AppTask appTask);

/*!
    @brief Destroys the CAP Broadcast Src Instance correspondig to profile Handle.
    Application recieves CAP_CLIENT_BCAST_SRC_DEINIT_CFM when this API is called

     @param bcastSrcProfileHandle  broadcast Src Profile handle returned
                                   as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
*/

void CapClientBcastSrcDeinitReq(uint32 bcastSrcProfileHandle);

/*!
     @brief Set Broadcast Source "Broadcast ID".

     @param bcastSrcProfileHandle  broadcast Src Profile handle returned 
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
     @param broadcastId generated Broadcast ID values based on random number 
            generation as defined in Volume 3, Part H, Section 2 in Core spec.
            Only 24 bit value will be set as Broadcast id
.
            Always recommended to use new random broadcastId for each Broadcast session. 

     @return None
*/
void CapClientBcastSrcSetBroadcastId(uint32 bcastSrcProfileHandle, uint32 broadcastId);

/*!
    @brief Configures the CAP Broadcast Src.
    Application recieves CAP_CLIENT_BCAST_SRC_CONFIG_CFM when this API is called

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned 
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
     @param ownAddrType Address type of the local device
     @param presentationDelay Presentation delay to synchronize the presentation
                              of multiple BISs in a BIG.
     @param numSubgroup Total number of sub groups in the broadcast group.
     @param subgroupInfo Sub group related information in CapBigSubgroups format.
     @param broadcastInfo Public Broadcast information to be published in 
            extended advertisement
     @param mode Default or QHS mode and/or Joint Stereo mode.
                 In Default mode, CAP will use parameters for BIG configuration
                 from BAP spec v1.0.1 3.7.1 
                 (Broadcast Source audio capability configuration support).

                 In QHS mode, application can configure some of BIG params by
                 passing CapClientBigQhsConfig.

                 If Joint Stereo mode is set then Broadcast Source will
                 always configure for Audio Configuration 14 (Multiple Audio
                 Channels, 1 BIS). Caller of the API needs to ensure that
                 valid subgroupInfo for Audio Configuration 14 is passed
                 in this mode..
     @param bigConfig config param for BIG in QHS mode. Applicable only
                           if mode is set QHS mode.

    NOTE: a) If the application configures both standard and HQ Broadcast in
             broadcast param then it needs to ensure that numSubgroup is correct
             as per BAP specification
          b) Application or profile using this api must call CapClientSetParamReq prior to calling
             CapClientBcastSrcConfigReq if they want to override default BAP specification values.
*/

void CapClientBcastSrcConfigReq(uint32 bcastSrcProfileHandle,
                             uint8 ownAddrType,
                             uint32 presentationDelay,
                             uint8 numSubGroup,
                             const CapClientBigSubGroup* subgroupInfo,
                             const CapClientBcastInfo* broadcastInfo,
                             CapClientBigConfigMode mode,
                             const CapClientQhsBigConfig* bigConfig);

/*!
    @brief Sets up Datapath for the CAP Broadcast SRC and starts SRC 
    Application recieves CAP_CLIENT_BCAST_SRC_START_STREAM_CFM when this API is called.

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned
           as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
    @param encryption   Encryption mode of the BISes.
                        TRUE :Encrypted
                        FALSE :Unencrypted
    @param broadcastCode :Encryption key(size 16) for encrypting payloads of all BISes.
                          App/profile needs to free this pointer.

*/

void CapClientBcastSrcStartStreamReq(uint32 bcastSrcProfileHandle,
                             bool  encryption,
                             uint8 *broadcastCode);

/*!
    @brief Stop the CAP Broadcast Src.
    Application recieves CAP_CLIENT_BCAST_SRC_STOP_STREAM_CFM when this API is called

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
*/

void CapClientBcastSrcStopStreamReq(uint32 bcastSrcProfileHandle);

/*!
    @brief Remove the CAP Broadcast Src.
    Application recieves CAP_CLIENT_BCAST_SRC_REMOVE_STREAM_CFM when this API is called

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.

*/

void CapClientBcastSrcRemoveStreamReq(uint32 bcastSrcProfileHandle);

/*!
    @brief Configures the CAP Updates metadata of Brcst Src.
    Application recieves CAP_CLIENT_BCAST_SRC_UPDATE_STREAM_CFM when this API is called

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned 
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
    @param useCase  Whether we are configuring the Server for Voice/Media/Gaming
    @param numSubgroup  number of subgroups required
    @param metadataLen  length of metadata
    @param metadata     Metadata can be ccid list or vendor specific.
                        App/profile needs to free this pointer.

*/

void CapClientBcastSrcUpdateStreamReq(uint32 bcastSrcProfileHandle,
                             CapClientContext useCase,
                             uint8 numSubgroup,
                             uint8 metadataLen,
                             const uint8* metadata);

/*!
    @brief Configures the Brcst Src Extended and periodic advertising parameters

    @param bcastSrcProfileHandle  broadcast Src Profile handle returned 
                                  as part of CAP_CLIENT_BCAST_SRC_INIT_CFM.
    @param srcAdvParams  Extended and periodic advertising parameters
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

     @return None
*/
void CapClientBcastSrcSetAdvParamsReq(uint32 bcastSrcProfileHandle,
                                   const CapClientBcastSrcAdvParams *srcAdvPaParams);
#endif /* INSTALL_LEA_BROADCAST_SOURCE*/

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/*!
*
*  NOTE: Before Calling broadcast Assistant Procedure make sure to Call
*        CapClientStreamAndContrilInitReq. Only after successful initiation of
*        Stream and Control Procedure, the CAP Broadcast Assistant Procedures
*        can be excercised.
*
*        CAP Broadcast assistant Procedures also falls into group of procedures
*        which needs to be excercised by upper layer profiles (HAP, TMAP etc.) 
*        registered with CAP layer instead of application. If no layer is
*        registered the messages will be forwarded to application.
*/


/*!
     @brief Broadcast Assistant starts scan for periodic trains that meet a
            specified scan_filter.  Application may choose to scan private or public
            broadcast or both.

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   GroupId
     @param cid btConnid for the device which is currently written to.
                Passing cid as 0 will start the scan and all devices in the 
                group will get notified if LE ACL is established with all.
     @param bcastSrcType Refer bcastSrcType definition for collocated only scan or 
                         non-collocated only scan or allow all.
     @param bcastType    Bit mask of all bcastType required to scan.
     @param filterContext Looks for particular StreamingAudioContext in BIG Info.
     @param scanFlags Scanning flags. Refer flags definition above.
                      bits 0-1:
                      Flags AD structure filter defination.
                      0 = Report all
                      1 = Report advertising reports with flags AD type
                      2 = Report advertising reports with flags AD type
                          with LE Limited Discoverable Mode set or
                          LE General Discoverable Mode set.
                      3 = Report advertising reports with flags AD type
                          with LE Limited Discoverable Mode set
                      bits 2-7:        Reserved for future use always set to 0. 
       @param ownAddressType Local address type that shall be used for scanning.Refer X_Address_Type in hci.h
       @param scanningFilterPolicy Scan filter policy to be used for scanning.Refer Scanning_Filter_Policy in hci.h


       @return CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM confirmation message will be
               sent to application when search for periodic train is started or failed.
               BAP_BROADCAST_ASSISTANT_SCAN_FILTERED_ADV_REPORT_IND will be sent to
               application on receiving reports from broadcast sources.

       NOTE: This API preferably needs to be called by Profiles(TMAP, HAP etc.)
*/

void CapClientBcastAsstStartSrcScanReq(AppTask profileTask,
                                      ServiceHandle groupId,
                                      uint32 cid,
                                      CapClientBcastSrcLocation bcastSrcType,
                                      CapClientBcastType    bcastType,
                                      CapClientContext filterContext,
                                      uint8 scanFlags,
                                      uint8 ownAddressType,
                                      uint8 scanningFilterPolicy);

/*!
     @brief Broadcast Assistant stops the current scan for periodic trains.

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   GroupId
     @param cid btConnid for the device which is currently written to.
                Passing cid as 0 will start the scan and all devices in the
                group will get notified if LE ACL is established with all.    
     @param scanHandle returned in CAP_CLIENT_BCAST_ASST_START_SRC_SCAN_CFM.

     @return CAP_CLIENT_BCAST_ASST_STOP_SRC_SCAN_CFM confirmation message will be
      sent to application when search for periodic train is started or failed.

      NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)

 */

void CapClientBcastAsstStopSrcScanReq(AppTask profileTask,
                                     ServiceHandle groupId,
                                     uint32 cid,
                                     uint16 scanHandle);
/*!
     @brief Broadcast Assistant sends request to sync to periodic train of
            Broadcast SRC address

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId CAP groupId received in CapInitCfm message on CAP Init.
     @param addrt Address of the Broadcast SRC device with which sync
            needs to be established.
     @param advSid Advertising SID identifier obtained in
            CAP_CLIENT_BCAST_ASST_SRC_REPORT_IND for the Broadcast SRC device.

     @return CAP_CLIENT_BCAST_ASST_START_SYNC_TO_SRC_CFM confirmation message
             will be sent to application

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
           There needs to be connection with Scan delagator before this API is
           invoked.

*/

void CapClientBcastAsstSyncToSrcStartReq(AppTask profileTask,
                             ServiceHandle groupId,
                             TYPED_BD_ADDR_T *addrt,
                             uint8 advSid);

/*!
     @brief Broadcast Assistant sends request to stop syncing to a periodic
            train

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param syncHandle Handle of sync train to terminate

     @return CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_TERMINATE_CFM confirmation
             message will be sent to application

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
           There needs to be connection with Scan delagator before this API is
           invoked.

*/

void CapClientBcastAsstTerminateSyncToSrcReq(AppTask profileTask,
                             ServiceHandle groupId,
                             uint16 syncHandle);

/*!
     @brief Broadcast Assistant cancels sync request to a periodic train
            in progress

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId  CAP groupId received in CapInitCfm message on CAP Init.
     @return CAP_CLIENT_BCAST_ASST_SYNC_TO_SRC_CANCEL_CFM confirmation message
             will be sent to application

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
     There needs to be connection with Scan delagator before this API is
     invoked.

*/

void CapClientBcastAsstSyncToSrcCancelReq(AppTask profileTask,
                             ServiceHandle groupId);

/*!
     @brief CAP writes the Broadcast Audio Scan Control point characteristic
            through BAP in order to execute the Add Source operation.

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid btConnid for the device which is currently written to.
                Passing cid as 0 will start the scan and all devices in the
                group will get notified if LE ACL is established with all.
     @param sourceAddrt Advertiser_Address for the Broadcast Source
     @param advertiserAddressType  Advertiser address type for the Broadcast Source.
                                   Refer hci_prim.h for peer_address_type values
     @param srcCollocated Broadcast Source is local device or not
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

     @return CAP_CLIENT_BCAST_ASST_ADD_SRC_CFM confirmation message will be
             sent to application.

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)

*/

void CapClientBcastAsstAddSrcReq(AppTask profileTask,
                             ServiceHandle groupId,
                             uint32 cid,
                             BD_ADDR_T *sourceAddrt,
                             uint8 advertiserAddressType,
                             bool srcCollocated,
                             uint16 syncHandle,
                             uint8 sourceAdvSid,
                             CapClientPaSyncState paSyncState,
                             uint16 paInterval,
                             uint32 broadcastId,
                             uint8 numbSubGroups,
                             const CapClientSubgroupInfo *subgroupInfo);

/*!
     @brief CAP writes the Broadcast Audio Scan Control point characteristic
            through BAP in order to execute the Modify Source operation.

     @param profileTask   application Handle of the upper layer application 
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param srcCollocated Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising SID
     @param paSyncState PA Synchronization state
     @param paInterval SyncInfo field Interval parameter value
     @param infoCount  array length of structure containing cid, sourceId set.
     @param info       array of structure containing cid, sourceId set.
     @param numbSubGroups Number of subgroups  present in the BIG as defined by
                          the Num_Subgroups parameter of the BASE.
     @param subgroupInfo Pointer to BIG first subgroupInfo pointed by
                         numbSubGroups in the BIG.
                         App/profile needs to free this pointer.

     @return CAP_CLIENT_BCAST_ASST_MODIFY_SRC_CFM confirmation message will be
             sent to application.

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)

*/

void CapClientBcastAsstModifySrcReq(AppTask profileTask,
                             ServiceHandle groupId,
                             bool srcCollocated,
                             uint16 syncHandle,
                             uint8 sourceAdvSid,
                             CapClientPaSyncState  paSyncState,
                             uint16 paInterval,
                             uint8 infoCount,
                             const CapClientDelegatorInfo* info,
                             uint8 numbSubGroups,
                             const CapClientSubgroupInfo *subgroupInfo);

/*!
     @brief CAP writes the Broadcast Audio Scan Control point characteristic
            through BAP in order to execute the Remove Source operation.

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param infoCount  array length of structure containing cid, sourceId set.
     @param info  array of structure containing cid, sourceId set.

     @return CAP_CLIENT_BCAST_ASST_REMOVE_SRC_CFM confirmation message will be
             sent to application.

     NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
*/

void CapClientBcastAsstRemoveSrcReq(AppTask profileTask,
                             ServiceHandle groupId, 
                             uint8 infoCount,
                             const CapClientDelegatorInfo* info);

/*!
     @brief CAP writes the Broadcast Receive State CCCDs through BAP
           in order to subscribe/unsubsccribe GATT noifications.

     @param profileTask   application Handle of the upper layer application 
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID .
     @param sourceId Source id of the Broadcast Receive State characteristic.
     @param allSource Setting this TRUE will enable notification on all the
            BRS states on remote Server
     @param notificationEnable enable/disable the notifications on remote
            server

     @return CAP_CLIENT_BCAST_ASST_REGISTER_NOTIFICATION_CFM confirmation message
             will be sent to application.

      NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
*/

void CapClientBcastAsstRegisterNotificationReq(AppTask profileTask,
                             ServiceHandle groupId,
                             uint32 cid,
                             uint8 sourceId,
                             bool allSources,
                             bool noficationEnable);
/*!
     @brief CAP reads the Broadcast Receive State through BAP.

     @param profileTask   application Handle of the upper layer application
            registered with CAP
     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.
     @param cid Device Connection ID .
     Note: This is not group based operation, hence a valid cid is required 
           for this operation.

     @return CAP_CLIENT_BCAST_ASST_READ_RECEIVE_STATE_CFM confirmation message
             will be sent to application.

      NOTE: This API prefereably needs to be called by Profiles(TMAP, HAP etc.)
*/

void CapClientBcastAsstReadReceiveStateReq(AppTask profileTask,
                             ServiceHandle groupId,
                             uint32  cid);


/*!
     @brief CAP Api to Unlock the CSIP set members.
     Returns CAP_CLIENT_UNLOCK_SET_CFM

     @param groupId   CAP groupId received in CapInitCfm message on CAP Init.

*/

void CapClientUnlockCoordinatedSetReq(ServiceHandle groupId);

/*!
     @brief CAP writes the Broadcast Audio Scan Control point characteristic
           in order to send Broadcast codes when application receives
           CAP_CLIENT_BCAST_ASST_SET_CODE_IND

     @param cid Connection ID .
     @param sourceId Source id of the Broadcast Receive State characteristic.
     @param broadcastcode Value of Broadcast Code to set

     @return None.
*/

void CapClientBcastAsstSetCodeRsp(ServiceHandle groupId,
                             uint32  cid,
                             uint8 sourceId,
                             uint8* broadcastcode);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */
#endif
