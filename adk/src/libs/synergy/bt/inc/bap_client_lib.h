/*******************************************************************************

Copyright (C) 2022 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_LIB_H
#define BAP_CLIENT_LIB_H

#include "bap_client_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

 /*!
     @brief Initialize the BAP for the peer device.
            This APIs will trigger primary service discovery for ASCS and PACS services.
            On finding these services, it also triggers the characteristics discovery for each
            services.
            On completion, it will send  BapInitCfm message to application with the result.

     @param appHandle Application handle.
     @param initData  Initialization data. In the case of only Broadcast Source
                      role application can send cid value INVALID_CID

     @return BapInitCfm confirmation message will be sent to application

 */
void BapInitReq(phandle_t appHandle, BapInitData initData);

 /*!
     @brief De-Initialize the BAP role for the peer device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param role BAP role to be de-initialized.

     @return BapDeinitCfm confirmation message will be sent to application

 */
 void BapDeinitReq(BapProfileHandle handle, BapRole role);

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*!
    @brief Add Codec Record to BAP client.
           This API informs the BAP layer about the supported audio codec
           cabalitiesby the local host and not to expose to remote device.

    @param appHandle  Application handle.
    @param pacRecord  Pointer to BAP PAC record.

    @return BapAddPacRecordCfm confirmation message will be sent to application

*/
void BapAddCodecRecordReq(phandle_t appHandle, BapPacLocalRecord *pacRecord);

/*!
    @brief Remove Codec Record from BAP client.
           This API remove the Codec Record from the BAP layer based in codec record id.
           The codec record id is received in BapAddPacRecordCfm message on adding
           Codec record using BapAddCodecRecordReq API.

    @param appHandle Application handle.
    @param pacRecordId Audio codec record id.

    @return BapRemovePacRecordCfm confirmation message will be sent to application

*/
void BapRemoveCodecRecordReq(phandle_t appHandle, uint16 pacRecordId);


/*!
     @brief Discover Audio Role of the remote device.

     Note:- This function will locally checks the pac record type from the discovered PAC
            record of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param recordType Audio Sink or Source record.

     @return TRUE if passed audio record type is present in remote device else FALSE.

 */
void BapDiscoverAudioRoleReq(BapProfileHandle handle, BapPacRecordType recordType);

/*!
     @brief Discover Audio Sink or Source Capability record of remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param recordType Audio Sink or Source record.

     @return BapDiscoverRemoteAudioCapabilityCfm confirmation message will be sent to application.

 */
void BapDiscoverRemoteAudioCapabilityReq(BapProfileHandle handle, BapPacRecordType recordType);

/*!
     @brief Register for PACS notification from remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param notifyType Bitwise values of PACS Notifications to be enabled or disabled.
     @param notifyEnable PACS Notification enable (TRUE) or disable (FALSE).

     @return BapRegisterPacsNotificationCfm confirmation message will be sent to application.

 */
void BapRegisterPacsNotificationReq(BapProfileHandle handle, BapPacsNotificationType notifyType, bool notifyEnable);


/*!
     @brief Get Sink or Source Audio location of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param recordType Audio Sink or Source record.

     @return BapGetRemoteAudioLocationCfm confirmation message will be sent to application.

 */
void BapGetRemoteAudioLocationReq(BapProfileHandle handle, BapPacRecordType recordType);

/*!
     @brief Set Sink/Source Audio location of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param recordType  Audio Sink or Source record.
     @param location  Audio location to be set.

     @return BapSetRemoteAudioLocationCfm confirmation message will be sent to application.

 */
void BapSetRemoteAudioLocationReq(BapProfileHandle handle, BapPacRecordType recordType, BapAudioLocation location);

/*!
     @brief Discover the Available or Supported audio context of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param contextType Available or Supported audio context.

 */
void BapDiscoverAudioContextReq(BapProfileHandle handle, BapPacAudioContext contextType);


/*!
     @brief Discover the Available Audio Stream Endpoints from the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param aseId ASE Identifier. To get info about all the available ASE's
            from the server use value 'ASE_ID_ALL'.
            To get a specific info about an ASE use known ase id value.
     @param aseType ASE characteristics type

     @return BAP_UNICAST_CLIENT_READ_ASE_INFO_CFM confirmation message will be sent to application

 */
void BapUnicastClientReadAseInfoReq(BapProfileHandle handle, uint8 aseId, BapAseType aseType);


/*!
     @brief Register for ASCS notification from remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param aseId ASE Indentfier. To register for all availble ASES, use value
                  'ASE_ID_ALL'.
     @param notifyEnable PACS Notification enable (TRUE) or disable (FALSE).

     @return BAP_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_CFM confirmation message will be sent to application

 */
void BapUnicastRegisterAseNotificationReq(BapProfileHandle handle, uint8 aseId, bool notifyEnable);

/*!
     @brief Start Codec Configure procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseCodecConfigurations Total number of ASE Codec Configurations.
                    The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseCodecConfigurations Pointer to ASE Codec Configurations for each ASE.

     @return BAP_UNICAST_CLIENT_CODEC_CONFIGURE_IND for each ASE ID followed by a
             BAP_UNICAST_CLIENT_CODEC_CONFIGURE_CFM confirmation message will be sent to application

 */

void BapUnicastClientCodecConfigReq(BapProfileHandle handle,
                                        uint8 numAseCodecConfigurations,
                                        const BapAseCodecConfiguration * aseCodecConfigurations);

/*!
     @brief Start CIG Configure procedure to initilaised the CIG parameter in the controller.

     @param appHandle Application handle.
     @param cigParameters Pointer to CIG Parameters to be set in the controller.

     @return BAP_UNICAST_CLIENT_CIG_CONFIGURE_CFM  message will be sent to application

 */

void BapUnicastClientCigConfigReq(phandle_t appHandle,
                                    const BapUnicastClientCigParameters *cigParameters);

/*!
     @brief Start CIG Test Configure procedure to initilaised the CIG parameter in the controller.

     @param appHandle Application handle.
     @param cigParameters Pointer to CIG Parameters to be set in the controller.

     @return BAP_UNICAST_CLIENT_CIG_TEST_CONFIGURE_CFM  message will be sent to application

 */

void BapUnicastClientCigTestConfigReq(phandle_t appHandle,
                                      const BapUnicastClientCigTestParameters *cigTestParams);

/*!
     @brief Start CIG Remove procedure to remove an existing CIG parameter from the controller.

     @param appHandle Application handle.
     @param cigId A valid CIG Identifier generated by the controller .

     @return BAP_UNICAST_CLIENT_CIG_REMOVE_CFM  message will be sent to application

 */

void BapUnicastClientCigRemoveReq(phandle_t appHandle, uint8 cigId);

/*!
     @brief Start QOS Configure procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseQosConfigurations Total number of ASE QOS configuartions.
                   The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseQosConfigurations Pointer to ASE QOS Configurations for each ASE.

     @return BAP_UNICAST_CLIENT_QOS_CONFIGURE_IND for each ASE ID followed by a
             BAP_UNICAST_CLIENT_QOS_CONFIGURE_CFM confirmation message will be sent to application

 */

void BapUnicastClientQosConfigReq(BapProfileHandle handle,
                                    uint8 numAseQosConfigurations,
                                    const BapAseQosConfiguration * aseQosConfigurations);

/*!
     @brief Start Enable procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseEnableParameters Total number of ASE Enable Parameters.
                       The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseEnableParameters Pointer to ASE Enable Parameters for each ASE.

     @return BAP_UNICAST_CLIENT_ENABLE_IND for each ASE ID followed by a
             BAP_UNICAST_CLIENT_ENABLE_CFM confirmation message will be sent to application

 */

void BapUnicastClientEnableReq(BapProfileHandle handle, uint8 numAseEnableParameters,
                           const BapAseEnableParameters * aseEnableParameters);


/*!
     @brief Update Metadata procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseMetadataParameters Total number of ASE Metadata Parameters.
                       The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseMetadataParameters Pointer to ASE Metadata Parameters for each ASE.

     @return BAP_UNICAST_CLIENT_ENABLE_IND for each ASE ID followed by a
             BAP_UNICAST_CLIENT_ENABLE_CFM confirmation message will be sent to application

 */

void BapUnicastClientUpdateMetadataReq(BapProfileHandle handle, uint8 numAseMetadataParameters,
                           const BapAseMetadataParameters * aseMetadataParameters);



/*!
     @brief start Receiver Start/Stop ready procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param readyType type of readiness BAP_RECEIVER_START_READY or BAP_RECEIVER_STOP_READY

     @param numAse Total number of ASE Parameters.
                      The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseIds Pointer to ASE Identifiers.

     @return BAP_UNICAST_CLIENT_RECEIVER_READY_IND for each ASE ID followed by
             BAP_UNICAST_CLIENT_RECEIVER_READY_CFM confirmation message will be sent to application

 */

void BapUnicastClientReceiverReadyReq(BapProfileHandle handle, uint8 readyType,
                            uint8 numAse, const uint8 * aseIds);


/*!
     @brief Create one or more CIS connection with the peer device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param cisCount Total number of cis_conn_parameters.
                      The current supported max number is BAP_MAX_SUPPORTED_ASES.

     @param cisConnParameters Pointer to BapUnicastClientCisConnection.
     @return BAP_UNICAST_CLIENT_CIS_CONNECT_IND for each CISes followed by
             BAP_UNICAST_CLIENT_CIS_CONNECT_CFM confirmation message will be sent to application
 */

void BapUnicastClientCisConnectReq(BapProfileHandle handle,
                                                uint8  cisCount,
                  const BapUnicastClientCisConnection *cisConnParameters);


/*!
     @brief Disconnect a CIS connection with the peer device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param cisHandle handle of the existing CIS.
     @param disconReason Reason for disconnection.

     @param cis_conn_parameters Pointer to BapUnicastClientCisConnection.
     @return BAP_UNICAST_CLIENT_CIS_DISCONNECT_CFM confirmation message will be sent to application
 */

void BapUnicastClientCisDiconnectReq(BapProfileHandle handle,
                                                uint16  cisHandle,
                                                    uint8 disconReason);
#endif

/*!
     @brief Setup ISO data path for a CIS or BIS .

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param dataPathParameter Pointer to data_path_parameter.

     @return BAP_CLIENT_SETUP_DATA_PATH_CFM  message will be sent to application

 */

void BapClientSetupDataPathReq(BapProfileHandle handle,
                    const BapSetupDataPath *dataPathParameter);

/*!
     @brief Remove ISO data path for a CIS or BIS .

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param isoHandle Handle for CIS or BIS data path to be removed
     @param dataPathDirection Direction of the data path.

     @return BAP_CLIENT_REMOVE_DATA_PATH_CFM  message will be sent to application
 */

void BapClientRemoveDataPathReq(BapProfileHandle handle,
                        uint16 isoHandle, uint8 dataPathDirection);

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*!
     @brief Start Disable procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseDisableParameters Total number of ASE Disable Parameters.
                      The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseDisableParameters Pointer to ASE Disable Parameters for each ASE.

     @return BAP_UNICAST_CLIENT_DISABLE_IND for each ASE ID followed by
             BAP_UNICAST_CLIENT_DISABLE_CFM confirmation message will be sent to application

 */

void BapUnicastClientDisableReq(BapProfileHandle handle, uint8 numAseDisableParameters,
                                      const BapAseParameters * aseDisableParameters);

/*!
     @brief Start Release procedure of Audio Stream Endpoints of the remote device.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param numAseReleaseParameters Total number of ASE Enable Parameters.
                    The current supported max number is BAP_MAX_SUPPORTED_ASES.
     @param aseReleaseParameters Pointer to ASE Release Parameters for each ASE.

     @return BAP_UNICAST_CLIENT_RELEASE_IND for each ASE ID followed by
             BAP_UNICAST_CLIENT_RELEASE_CFM confirmation message will be sent to application

 */

void BapUnicastClientReleaseReq(BapProfileHandle handle, uint8 numAseReleaseParameters,
                                      const BapAseParameters * aseReleaseParameters);



/*!
     @brief BAP can switch to GATT write procedure with ASCS Server and Scan Delegator for
            Control point operations.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param controlOpResponse TRUE if Control Point operations has to be executed with the
            GATT Write procedure.
     @param longWrite TRUE if GATT long write procedure to be used

     NOTE:
       a) If this API is not used by application then by default GATT Write Without
          Response procedure is used
       b) This API can be exercised anytime to change the gatt operation procedure.

     @return BAP_SET_CONTROL_POINT_OP_CFM confirmation message will be sent to
      application.
*/
void BapSetControlPointOpReq(BapProfileHandle handle, bool controlOpResponse,bool longWrite);
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_SOURCE

/*!
     @brief Broadcast Source set Broadcast ID of the BIG.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param broadcastId generated Broadcast ID other than 0xFFFFFFFF.
            Only 24 bit value will be set as Broadcast id
.
            Always recommended to use new random broadcastId for each Broadcast session. 

     @return None
*/
void BapBroadcastSrcSetBroadcastId(BapProfileHandle handle, uint32 broadcastId);

/*!
     @brief Broadcast Source set Extended and periodic advertising parameters.
     

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param srcAdvPaParams Extended and periodic advertising parameters
      default parameter values: 
      advEventProperties = 0
      advIntervalMin = CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN
      advIntervalMax = CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN
      primaryAdvPhy = BAP_LE_1M_PHY
      primaryAdvChannelMap = HCI_ULP_ADVERT_CHANNEL_DEFAULT
      secondaryAdvMaxSkip = 0
      secondaryAdvPhy = BAP_LE_1M_PHY
      advSid = CM_EXT_ADV_SID_ASSIGNED_BY_STACK
      periodicAdvIntervalMin = 0x320/ 1Sec
      periodicAdvIntervalMax = 0x640/ 2Sec

     @return None
 */
void BapBroadcastSrcSetAdvParams(BapProfileHandle handle,
                          const BapBroadcastSrcAdvParams *srcAdvPaParams);

/*!
     @brief Broadcast Source configure and reconfigure procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)
     @param ownAddrType Address type of the local device
     @param presentationDelay Presentation delay to synchronize the presentation
            of multiple BISs in a BIG.
     @param numSubgroup Total number of sub groups in the broadcast group.
     @param subgroupInfo Sub group related information in BapBigSubgroups format.
     @param broadcastInfo Public Broadcast information to be published in extended advertisement
     @param bigNameLen  Length of bigName
     @param bigName Name of the broadcast source 

     @return BAP_BROADCAST_SRC_CONFIGURE_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcConfigureStreamReq(BapProfileHandle handle,uint8 bigId,
            uint8 ownAddrType, uint32 presentationDelay, uint8 numSubgroup,
            const BapBigSubgroups *subgroupInfo, const BapBroadcastInfo *broadcastInfo,
            uint8 bigNameLen, char* bigName);

/*!
     @brief Broadcast Source Enable procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)
     @param bigConfigParameters BIG configuration parameters.
     @param numBis Total number of bis in the broadcast group.
     @param encryption  Encryption mode of the BISes( 0- unencrypted 1-encrypted)
     @param broadcastCode Encryption key(size 16) for encrypting payloads of all BISes.

     @return BAP_BROADCAST_SRC_ENABLE_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcEnableStreamReq(BapProfileHandle handle, uint8 bigId,
       const BapBigConfigParam *bigConfigParameters, uint8 numBis,
       bool encryption, const uint8 *broadcastCode);

/*!
     @brief Broadcast Source Enable Test procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)
     @param bigTestConfigParameters BIG test configuration parameters.
     @param numBis Total number of bis in the broadcast group.
     @param encryption  Encryption mode of the BISes( 0- unencrypted 1-encrypted)
     @param broadcastCode Encryption key(size 16) for encrypting payloads of all BISes.

     @return BAP_BROADCAST_SRC_ENABLE_TEST_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcEnableStreamTestReq(BapProfileHandle handle, uint8 bigId,
          const BapBigTestConfigParam *bigTestConfigParameters, uint8 numBis,
          bool encryption, const uint8 *broadcastCode);

/*!
     @brief Broadcast Source Disable procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)

     @return BAP_BROADCAST_SRC_DISABLE_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcDisableStreamReq(BapProfileHandle handle, uint8 bigId);

/*!
     @brief Broadcast Source Release procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)

     @return BAP_BROADCAST_SRC_RELEASE_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcReleaseStreamReq(BapProfileHandle handle, uint8 bigId);

/*!
     @brief Broadcast Source update Metadat procedure of Broadcast Audio Stream Endpoints.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param bigId Host assigned identifier of the Broadcast group(0x00 - 0xEF)
     @param numSubgroup Total number of subgroup in the broadcast group.
     @param subgroupMetadata  Sub group level metadata

     @return BAP_BROADCAST_SRC_UPDATE_METADAT_CFM confirmation message will be sent to application
 */
void BapBroadcastSrcUpdateMetadataReq(BapProfileHandle handle, uint8 bigId,
    uint8 numSubgroup, const BapMetadata *subgroupMetadata);

/*!
     @brief Return LTV offset if present for the LTV type asked.

     @param ltvData Pointer to block containing LTV data
     @param ltvDataLength Length of the byte stream containing LTV data
     @param type metadata types value. Refer BAP_CLIENT_METADATA_LTV_TYPE_XXX defines in bap_client_prim.h
     @param offset Contains the LTV location offset if present

     @return TRUE if LTV type is found else FALSE.
             If TRUE then offset contains the offset at which LTV is present.
 */
bool BapLtvUtilitiesFindLtvOffset(uint8 *ltvData, uint8 ltvDataLength,
    uint8 type, uint8 *offset);


#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
/*!
     @brief Broadcast Assistant starts scan for periodic trains that meet a specified scan_filter.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param flags Scan SRC's collocated or remote or both.
     @param filterContext Looks for particular SRC's meeting a specified filter(s).
     @param scanFlags Scanning flags
     @param ownAddressType Local address type that shall be used for scanning
     @param scanningFilterPolicy Scan filter policy to be used for scanning

     @return BAP_BROADCAST_ASSISTANT_START_SCAN_CFM confirmation message will be sent to
      application when search for periodic train is started or failed.
      BAP_BROADCAST_ASSISTANT_SCAN_FILTERED_ADV_REPORT_IND will be sent to application
      on receiving reports from broadcast sources.
 */
void BapBroadcastAssistantStartScanReq(BapProfileHandle handle,
    uint8 flags,
    uint16 filterContext,
    uint8 scanFlags,
    uint8 ownAddressType,
    uint8 scanningFilterPolicy);

/*!
     @brief Broadcast Assistant stops scan for periodic trains.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param scanHandle returned in BAP_BROADCAST_ASSISTANT_START_SCAN_CFM

     @return BAP_BROADCAST_ASSISTANT_STOP_SCAN_CFM confirmation message will be sent to
      application.
 */
void BapBroadcastAssistantStopScanReq(BapProfileHandle handle, uint16 scanHandle);


/*!
     @brief Broadcast Assistant establishes sync to periodic train of device specified

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param addrt BT address of the Broadcast Source State
     @param advSid Advertising SID

     @return BAP_BROADCAST_ASSISTANT_START_SYNC_TO_SRC_CFM confirmation message will be sent to
      application
*/
void BapBroadcastAssistantSyncToSrcStartReq(BapProfileHandle handle,TYPED_BD_ADDR_T *addrt, uint8 advSid);

/*!
     @brief Broadcast Assistant cancels establishing sync to a periodic train.

     @param handle BAP handle received in BapInitCfm message on BAP Init.

     @return BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_CANCEL_CFM confirmation message will be sent to
      application
*/
void BapBroadcastAssistantSyncToSrcCancelReq(BapProfileHandle handle);


/*!
     @brief Broadcast Assistant stops syncing to a periodic train

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param syncHandle The sync train to terminate

     @return BAP_BROADCAST_ASSISTANT_SYNC_TO_SRC_TERMINATE_CFM confirmation message will be sent to
      application
*/
void BapBroadcastAssistantSyncToSrcTerminateReq(BapProfileHandle handle,
    uint16 syncHandle);


void BapBroadcastAssistantBRSRegisterForNotificationReq(BapProfileHandle  handle,
    uint8 sourceId,
    bool allSources,
    bool notificationsEnable);


void BapBroadcastAssistantReadBRSCccReq(BapProfileHandle  handle,
    uint8 sourceId,
    bool allSources);

/*!
     @brief Broadcast Assistant reads Broadcast receive state characteristics of remote server

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param sourceId Source id of the Broadcast Receive State characteristic that has to be read.
                     If NULL, all the Broadcast receive Stete Characteristics will be read.
     @param allSource  If TRUE all the Broadcast receive Stete Characteristics will be read and
                     source_id parameter will be ignored.


     @return BAP_BROADCAST_ASSISTANT_READ_BRS_IND indication(s) message will be sent to
      application as the remote responds with receiver state characteristics
      BAP_BROADCAST_ASSISTANT_READ_BRS_CFM confirmation message will be sent to  application
      when no more receive state information is available in case of success or
      when remote does not respond for the request resulting in failure.
*/
void BapBroadcastAssistantReadBRSReq(BapProfileHandle  handle,
    uint8 sourceId,
    bool allSources);


/*!
     @brief Broadcast Assistant writes the Broadcast Audio Scan Control point characteristic
           in order to execute the Add Source operation.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
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

     @return BAP_BROADCAST_ASSISTANT_ADD_SRC_CFM confirmation message will be sent to
      application.
*/

void BapBroadcastAssistantAddSrcReq(BapProfileHandle handle,
    BD_ADDR_T *sourceAddrt,
    uint8 advertiserAddressType,
    bool    srcCollocated,
    uint16 syncHandle,
    uint8 sourceAdvSid,
    uint8 paSyncState,
    uint16 paInterval,
    uint32 broadcastId,
    uint8 numbSubGroups,
    BapSubgroupInfo *subgroupInfo);

/*!
     @brief Broadcast Assistant writes the Broadcast Audio Scan Control point characteristic
           in order to execute the Modify Source operation.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param sourceAddrt BAP Broadcast Source BD address.
     @param sourceId Source id of the Broadcast Receive State characteristic
     @param srcCollocated Broadcast Source is local device or not
     @param syncHandle syncHandle of PA in case of non-collocated or
                       advHandle of collocated Broadcast src
     @param sourceAdvSid Advertising SID
     @param paSyncState PA Synchronization state
     @paInterval SyncInfo field Interval parameter value
     @numbSubGroups Number of subgroups  present in the BIG as defined by
                    the Num_Subgroups parameter of the BASE.
     @subgroupInfo Pointer to BIG first subgroupInfo pointed by
                   numbSubGroups in the BIG

     @return BAP_BROADCAST_ASSISTANT_ADD_SRC_CFM confirmation message will be sent to
      application.
*/
void BapBroadcastAssistantModifySrcReq(BapProfileHandle handle,
    uint8 sourceId,
    bool srcCollocated,
    uint16 syncHandle,
    uint8 sourceAdvSid,
    uint8 paSyncState,
    uint16 paInterval,
    uint8 numbSubGroups,
    BapSubgroupInfo *subgroupInfo);


/*!
     @brief Broadcast Assistant writes the Broadcast Audio Scan Control point characteristic
           in order to execute the Remove Source operation.

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param sourceId Source id of the Broadcast Receive State characteristic.

     @return BAP_BROADCAST_ASSISTANT_REMOVE_SRC_CFM confirmation message will be sent to
      application.
*/
void BapBroadcastAssistantRemoveSrcReq(BapProfileHandle handle,
    uint8 sourceId);

/*!
     @brief Broadcast Assistant writes the Broadcast Audio Scan Control point characteristic
           in order to send Broadcast codes when application receives
           BAP_BROADCAST_ASSISTANT_SET_CODE_IND

     @param handle BAP handle received in BapInitCfm message on BAP Init.
     @param sourceId Source id of the Broadcast Receive State characteristic.
     @param broadcastCode Value of Broadcast Code to set

     @return None.
*/
void BapBroadcastAssistantSetCodeRsp(BapProfileHandle handle,
    uint8 sourceId,
    uint8 *broadcastCode);

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */


/*!
     @brief This will return the device data for the respective profiles found during
            GATT service discovery procedure.

     @param profileHandle BAP handle received in BapInitCfm message on BAP Init.
     @param clntProfile Profile for which information need to retrieve.

     @return void *.
*/

void *BapGetAttributeHandles(BapProfileHandle profileHandle, uint8 clntProfile);


/*!
     @brief This function will return true if the audio role is supported.

     @param profileHandle BAP handle received in BapInitCfm message on BAP Init.
     @param recordType 

     @return bool.

     NOTE: This is not a message passing based API and will return the pacs role immediately from gatt pacs client library.
*/

bool BapDiscoverPacsAudioRoleReq(BapProfileHandle handle,
                                 BapPacRecordType recordType);

#ifdef __cplusplus
}
#endif

#endif /* ifnndef BAP_CLIENT_LIB_H */
