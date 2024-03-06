/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #8 $
******************************************************************************

FILE NAME
    pbp.h
    
DESCRIPTION
    Header file for the Public Broadcast Profile (PBP) library.
*/

#ifndef PBP_H
#define PBP_H

#include "csr_bt_gatt_prim.h"
#include "service_handle.h"

#include "cap_client_prim.h"
#include "cap_client_lib.h"

#include "pbp_prim.h"

#define PBP_STANDARD_BROADCAST           (PbpBroadcastType)(STANDARD_BROADCAST)
#define PBP_HQ_BROADCAST                 (PbpBroadcastType)(HQ_BROADCAST)

/*!
    @brief Initialises the PBP Source Library.

    @param appTask           The Task that will receive the messages sent from this profile library

    NOTE: A PBP_INIT_CFM message will be received with a PbpStatus that indicates the result of the initialisation.
*/
void PbpInitReq(AppTask appTask);

/*!
    @brief This is the cleanup routine as a result of calling the PbpInitReq API.

    @param profileHandle The Profile handle.

    NOTE: A PBP_DESTROY_CFM message will be received with a PbpStatus that indicates the
          result of the destroy.
*/
void PbpDestroyReq(PbpProfileHandle profileHandle);

#ifdef INSTALL_LEA_BROADCAST_SOURCE

/* Public Broadcast Source API*/

/*!
    @brief Initialises the PBP Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_INIT_CFM when this API is called

    @param profileHandle  Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
*/
void PbpBroadcastSrcInitReq(PbpProfileHandle profileHandle);

/*!
    @brief Deinitialises the PBP Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_DEINIT_CFM when this API is called

    @param brcstSrcProfileHandle  broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
*/
void PbpBroadcastSrcDeinitReq(PbpProfileHandle brcstSrcProfileHandle);

/*!
    @brief Configures the Broadcast Source Extended and periodic advertising parameters

    @param brcstSrcProfileHandle  Public Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
    @param srcAdvPaParams    Set extended and Periodic Advertisement parameters.
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
void PbpBroadcastSrcSetAdvParamsReq(PbpProfileHandle brcstSrcProfileHandle, const PbpBcastSrcAdvParams *srcAdvPaParams);

/*!
    @brief This API is used to overwrite standard CIG params with user defined
           values for a particular usecase before calling PbpBroadcastSrcConfigReq(). 
           Application receives PBP_BROADCAST_SRC_SET_PARAM_CFM when this API is called.

    @param brcstSrcProfileHandle Public Broastcast Source profile handle
    @param numOfSubGroups Number of subgroups of the broadcast source.
    @param param          Broadcast configuration parameters for each subgroup of the
                          broadcast source.

    @return None
*/

void  PbpBroadcastSrcSetParamReq(PbpProfileHandle brcstSrcProfileHandle,
                                 uint8 numOfSubGroups,
                                 const PbpBcastConfigParam* param);

/*!
     @brief Set Broadcast Source "Broadcast ID".
 
     This optional API allows application to set unique Broadcast ID.
     This API shall be called before PbpBroadcastSrcConfigReq() once for the
     life time of the Broadcast session.
     If a Broadcast ID is set from the application, it shall not set it again
     until PbpBroadcastSrcRemoveStreamReq() called.

     @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as 
                                   part of PBP_BROADCAST_SRC_INIT_CFM
     @param broadcastId    Generated Broadcast ID values based on random number 
            generation as defined in Volume 3, Part H, Section 2 in Core specification.
            If value PBP_BROADCAST_ID_DONT_CARE is set, profile will generate it internally. 
            Only 24 bit value will be set as Broadcast id.
.
     Note:  Always recommended to use new random broadcastId for each Broadcast session. 

     @return None
*/
void PbpBroadcastSrcSetBroadcastId(PbpProfileHandle brcstSrcProfileHandle, uint32 broadcastId);

/*!
    @brief Configures the PBP Broadcast Source identified by.brcstSrcProfileHandle
    Application recieves PBP_BROADCAST_SRC_CONFIG_CFM when this function is called.

    @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
    @param ownAddrType            Address type of the local device
    @param presentationDelay      Presentation delay to synchronize the presentation
                                  of multiple BISs in a BIG.
    @param numSubgroup            Total number of sub groups in the broadcast group.
    @param subgroupInfo           Sub group related information
    @param broadcastInfo          Public Broadcast information to be published in extended advertisement
    @param mode                   Default or QHS mode and/or Joint Stereo mode. 
                                  In Default mode, CAP will use parameters for BIG configuration from
                                  BAP spec v1.0.1 3.7.1 (Broadcast Source audio capability configuration support).
                                  In QHS mode, application can configure some of BIG
                                  params by passing CapClientBigQhsConfig.
                                  If Joint Stereo mode is set then Broadcast Source will always configure for 
                                  Audio Configuration 14 (Multiple Audio Channels, 1 BIS).
                                  Application needs to ensure that valid subgroupInfo for Audio Configuration 14
                                  is passed in this mode.
    @param bigConfig              Configuraration of the BIG parameters in QHS mode. Applicable only
                                  if mode is set QHS mode.

    NOTE: It's application/upper layer responsibility to free all the eventual memory associated 
          with the subgroupInfo, broadcastInfo and bigConfig pointers.
*/
void PbpBroadcastSrcConfigReq(PbpProfileHandle brcstSrcProfileHandle,
                              uint8 ownAddrType,
                              uint32 presentationDelay,
                              uint8 numSubGroup,
                              const PbpBigSubGroups* subgroupInfo,
                              PbpBroadcastInfo *broadcastInfo,
                              PbpBigConfigMode mode,
                              const PbpQhsBigConfig *bigConfig);

/*!
    @brief Starts the streaming of the PBP Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_START_STREAM_CFM when this function is called

    @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
    @param encryption             TRUE if BIS is encrypted, otherwise FALSE
    @param broadcastCode          Code to decrypt the BIS/es if encrypted (encryption set to TRUE). 
                                  It should be set to NULL, if the BIS/es is/are not encrypted (encryption set to FALSE).
                                  NOTE: App/profile needs to free this pointer.

    NOTE: App/upper layer needs to free all the memory associates to the pointer in PBP_BROADCAST_SRC_START_STREAM_CFM.
*/
void PbpBroadcastSrcStartStreamReq(PbpProfileHandle brcstSrcProfileHandle, bool encryption, uint8 *broadcastCode);

/*!
    @brief Updates metadata of the PBP  Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_UPDATE_AUDIO_CFM when this function is called

    @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
    @param useCase                The use case we are configuring the stream
    @param numSubgroup            Number of subgroups required
                                  NOTE: It should be equal to the one used to configure the Public Broadcast Source.
    @param metadataLen            Length of metadata
    @param metadata               Metadata can be ccid list or vendor specific. 
                                  NOTE: App/profile needs to free this pointer.
*/
void PbpBroadcastSrcUpdateAudioReq(PbpProfileHandle brcstSrcProfileHandle,
                                   PbpContext useCase,
                                   uint8_t numSubgroup,
                                   uint8 metadataLen,
                                   uint8* metadata);

/*!
    @brief Stop the streaming of the PBP Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_STOP_STREAM_CFM when this function is called

    @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
*/
void PbpBroadcastSrcStopStreamReq(PbpProfileHandle brcstSrcProfileHandle);

/*!
    @brief Release the PBP Broadcast Source.
    Application recieves PBP_BROADCAST_SRC_REMOVE_STREAM_CFM when this function is called

    @param brcstSrcProfileHandle  Broadcast Source Profile handle returned as part of PBP_BROADCAST_SRC_INIT_CFM.
*/
void PbpBroadcastSrcRemoveStreamReq(PbpProfileHandle brcstSrcProfileHandle);
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

/*Public Broadcast Assistant procedure*/

/*!
     @brief Broadcast Assistant starts scan for periodic trains that meet a
            specified scan_filter.  Application may choose to scan SQ quality or
            HQ quality broadcaster or both.

     @param profileHandle        profile handle of PBP
     @param groupId              CAP groupId received in CapInitCfm message on CAP Init.
     @param cid                  btConnid for the device which is currently written to.
                                 Passing cid as 0 will start the scan and all devices in the
                                 group will get notified if LE ACL is established with all.
     @param bcastSrcType         Refer bcastSrcType definition for collocated only scan or
                                 non-collocated only scan or allow all.
     @param bcastType            Bit mask of all bcastType required to scan.
     @param filterContext        Looks for particular StreamingAudioContext in BIG Info.
     @param scanFlags            Scanning flags. Refer flags definition above.
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
     @param ownAddressType       Local address type that shall be used for scanning.Refer X_Address_Type in hci.h
     @param scanningFilterPolicy Scan filter policy to be used for scanning.Refer Scanning_Filter_Policy in hci.h


     @return PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM confirmation message will be
             sent to the application when search for periodic train is started or failed.
             BAP_BROADCAST_ASSISTANT_SCAN_FILTERED_ADV_REPORT_IND will be sent to
             application on receiving reports from broadcast sources.
*/

void PbpBroadcastAssistantScansSrcStartSrcReq(PbpProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              uint32 cid,
                                              PbpBcastSrcLocation bcastSrcType,
                                              PbpBcastType bcastType,
                                              PbpContext filterContext,
                                              uint8 scanFlags,
                                              uint8 ownAddressType,
                                              uint8 scanningFilterPolicy);

/*!
     @brief Broadcast Assistant stops the current scan for periodic trains.

     @param profileHandle profile handle of PBP
     @param groupId       CAP groupId received in CapInitCfm message on CAP Init.
     @param cid           btConnid for the device which is currently written to.
                          Passing cid as 0 will start the scan and all devices in the
                          group will get notified if LE ACL is established with all.
     @param scanHandle    returned in PBP_BROADCAST_ASSISTANT_SCAN_SRC_START_CFM.

     @return PBP_BROADCAST_ASSISTANT_SCAN_SRC_STOP_CFM confirmation message will be
             sent to application when search for periodic train is stoped.
 */

void PbpBroadcastAssistantStopSrcScanReq(PbpProfileHandle profileHandle,
                                         ServiceHandle groupId,
                                         uint32 cid,
                                         uint16 scanHandle);

/*!
     @brief Broadcast Assistant sends request to sync to periodic train of
            Broadcast SRC address

     @param profileHandle profile handle of PBP
     @param groupId       CAP groupId received in CapInitCfm message on CAP Init.
     @param addrt         Address of the Broadcast SRC device with which sync
                          needs to be established.
     @param advSid        Advertising Set ID identifier obtained in
                          PBP_BROADCAST_SRC_REPORT_IND for the Broadcast SRC device.

     @return PBP_BROADCAST_ASSISTANT_START_SYNC_TO_SRC_CFM confirmation message
             will be sent to application.

     NOTE: This API must be called for non-collocated cases only.
*/

void PbpBroadcastAssistantStartSyncToSrcReq(PbpProfileHandle profileHandle,
                                            ServiceHandle groupId,
                                            TYPED_BD_ADDR_T* addrt,
                                            uint8 advSid);

/*!
     @brief Broadcast Assistant sends request to stop syncing to a periodic
            train

     @param profileHandle profile handle of PBP
     @param groupId       CAP groupId received in CapInitCfm message on CAP Init.
     @param syncHandle    Handle of sync train to terminate

     @return PBP_BROADCAST_ASSISTANT_TERMINATE_SYNC_TO_SRC_CFM confirmation
             message will be sent to application.
*/

void PbpBroadcastAssistantTerminateSyncToSrcReq(PbpProfileHandle profileHandle,
                                                ServiceHandle groupId,
                                                uint16 syncHandle);

/*!
     @brief Broadcast Assistant cancels sync request to a periodic train
            in progress

     @param profileHandle profile handle of PBP
     @param groupId       CAP groupId received in CapInitCfm message on CAP Init.

     @return PBP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM confirmation message
             will be sent to application.
*/

void PbpBroadcastAssistantSyncToSrcCancelReq(PbpProfileHandle profileHandle, ServiceHandle groupId);

/*!
     @brief CAP writes the Broadcast Audio Scan Control point characteristic
            through BAP in order to execute the Add Source operation.

     @param profileHandle  Pprofile handle of PBP
     @param groupId        CAP groupId received in CapInitCfm message on CAP Init.
     @param cid            btConnid for the device which is currently written to.
                           Passing cid as 0 will start the scan and all devices in the
                           group will get notified if LE ACL is established with all.
     @param sourceAddrt    Advertiser_Address for the Broadcast Source along with type
     @param srcLocation    Broadcast Source is local device or not
     @param syncHandle     Sync Handle of PA in case of non-collocated or
                           advHandle of collocated Broadcast src
     @param sourceAdvSid   Advertising SID
     @param paSyncState    PA Synchronization state
     @param paInterval     SyncInfo field Interval parameter value
     @param broadcastId    Identifier of Broadcast Source's BIG, obtained as a part of
                           Extended Adv
     @param numbSubGroups  Number of subgroups  present in the BIG as defined by
                           the Num_Subgroups parameter of the BASE.
     @param subgroupInfo   Pointer to BIG first subgroupInfo pointed by
                           numbSubGroups in the BIG
                           App/profile needs to free this pointer.

     @return PBP_BROADCAST_ASSISTANT_ADD_SRC_CFM confirmation message will be
             sent to application.
*/

void PbpBroadcastAssistantAddSrcReq(PbpProfileHandle profileHandle,
                                    ServiceHandle groupId,
                                    uint32 cid,
                                    TYPED_BD_ADDR_T* sourceAddrt,
                                    PbpBcastSrcLocation srcLocation,
                                    uint16 syncHandle,
                                    uint8 sourceAdvSid,
                                    PbpPaSyncState paSyncState,
                                    uint16 paInterval,
                                    uint32 broadcastId,
                                    uint8 numbSubGroups,
                                    const PbpSubgroupInfo *subgroupInfo);

/*!
     @brief The Broadcast Audio Scan Control point characteristic is written
            through CAP/BAP in order to execute the Modify Source operation.

     @param profileHandle           Profile handle of PBP
     @param groupId                 CAP groupId received in CapInitCfm message on CAP Init.
     @param srcLocation             Broadcast Source is local device or not
     @param syncHandle              syncHandle of PA in case of non-collocated or
                                    advHandle of collocated Broadcast src
     @param sourceAdvSid            Advertising SID
     @param paSyncState             PA Synchronization state
     @param paInterval              SyncInfo field Interval parameter value
     @param numBroadcastDelegator   Length of broadcastDelegatorInfo.
     @param broadcastDelegatorInfo  Array of structure containing connected
                                    broadcast delegator info.
                                    App/profile needs to free this pointer.
     @param numbSubGroups           Number of subgroups  present in the BIG as defined by
                                    the Num_Subgroups parameter of the BASE.
     @param subgroupInfo            Pointer to BIG first subgroupInfo pointed by
                                    numbSubGroups in the BIG.
                                    App/profile needs to free this pointer.

     @return PBP_BROADCAST_ASSISTANT_MODIFY_SRC_CFM confirmation message will be
             sent to application.
*/

void PbpBroadcastAssistantModifySrcReq(PbpProfileHandle profileHandle,
                                       ServiceHandle groupId,
                                       PbpBcastSrcLocation srcLocation,
                                       uint16 syncHandle,
                                       uint8 sourceAdvSid,
                                       PbpPaSyncState paSyncState,
                                       uint16 paInterval,
                                       uint8 numBroadcastDelegator,
                                       const PbpBroadcastDelegatorInfo* broadcastDelegatorInfo,
                                       uint8 numbSubGroups,
                                       const PbpSubgroupInfo* subgroupInfo);

/*!
     @brief The Broadcast Audio Scan Control point characteristic is
            written through BAP in order to execute the Remove Source operation.

     @param profileHandle          Profile handle of PBP
     @param groupId                CAP groupId received in CapInitCfm message on CAP Init.
     @param numBroadcastDelegator  Length of broadcastDelegatorInfo.
     @param broadcastDelegatorInfo Array of structure containing connected
                                   broadcast delegator info.
                                   App/profile needs to free this pointer.

     @return PBP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM confirmation message will be
             sent to application.
*/

void PbpBroadcastAssistantRemoveSrcReq(PbpProfileHandle profileHandle,
                                       ServiceHandle groupId,
                                       uint8 numBroadcastDelegator,
                                       const PbpBroadcastDelegatorInfo* broadcastDelegatorInfo);

/*!
     @brief The Broadcast assistant registered for any changes in Broadcast Sink
            Receive audio streams state changes.

     @param profileHandle       Profile handle of PBP
     @param groupId             CAP groupId received in CapInitCfm message on CAP Init.
     @param cid                 Device Connection ID.
     @param sourceId            Source id of the Broadcast Receive State characteristic.
     @param allSource           Setting this TRUE will enable notifications on all the
                                BRS states on remote Server
     @param notificationEnable  Enable/disable the notifications on remote
                                server

     @return PBP_BROADCAST_ASSISTANT_REG_FOR_NOTIFICATION_CFM confirmation message
             will be sent to application.
*/

void PbpBroadcastAssistantRegisterForNotificationReq(PbpProfileHandle profileHandle,
                                                     ServiceHandle groupId,
                                                     uint32 cid,
                                                     uint8 sourceId,
                                                     bool allSources,
                                                     bool noficationEnable);

/*!
     @brief The Broadcast assistant read current state of Broadcast Sink Receive
            audio streams.


     @param profileHandle  Profile handle of PBP
     @param groupId        CAP groupId received in CapInitCfm message on CAP Init.
     @param cid            Device Connection ID .
     Note: This is not group based operation, hence a valid cid is required
           for this operation.

     @return PBP_BROADCAST_ASSISTANT_READ_RECEIVE_STATE_CFM confirmation message
             will be sent to application.
*/

void PbpBroadcastAssistantReadReceiveStateReq(PbpProfileHandle profileHandle,
                                              ServiceHandle groupId,
                                              uint32  cid);

/*!
     @brief The Broadcast Audio Scan Control point characteristic is
            written in order to send Broadcast codes when application receives
            PBP_BROADCAST_ASSISTANT_SET_CODE_IND

     @param groupId        CAP groupId received in CapInitCfm message on CAP Init.
     @param cid            Connection ID.
     @param sourceId       Source id of the Broadcast Receive State characteristic.
     @param broadcastcode  Value of Broadcast Code to set

     @return None.
*/

void PbpBroadcastAssistantSetCodeRsp(ServiceHandle groupId,
                                     uint32  cid,
                                     uint8 sourceId,
                                     uint8 *broadcastcode);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*!
    @brief Registers profiles with CAP for indications/notification that
           profile is interested in handling

    @param groupId       The ID of the device group which share SIRK with new device
    @param profileHandle Profile handle of PBP
*/

void PbpRegisterTaskReq(ServiceHandle groupId, PbpProfileHandle profileHandle);

/*!
    @brief DeRegister with CAP for msgs/indications
    Application receives PBP_DEREGISTER_TASK_CFM when this message is called

    @param profileHandle  Profile handle of PBP.
    @param groupId        The ID of the device group which share SIRK
*/
void PbpDeRegisterTaskReq(PbpProfileHandle profileHandle, ServiceHandle groupId);
#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif /* PBP_H */

