/****************************************************************************
Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
%%version

FILE NAME
    bap_server_lib.h
    
DESCRIPTION
    Header file for the Basic Audio Profile library.
*****************************************************************************/
#ifndef BAP_SERVER_LIB_H__
#define BAP_SERVER_LIB_H__

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_tasks.h"
#include "service_handle.h"
#include "bap_server_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
    @brief This API is used to Get the maximum number of supported ASEs by BAP

    ASCS server
    @param void 

    @return Maximum number of supported ASEs
*/
uint8 BapServerUnicastGetMaxSupportedAses(void);

/*!
    @brief This API is used to Initialise the Basic Audio Profile Library for
    unicast Role.

    @param theAppTask The AppTask that will receive the messages sent from this BAP library.
    @param numAses The number of ASEs to allow on BAP Unicast server instance. 
                    The value shall not exceed the value returned by 
                    BapServerUnicastGetMaxSupportedAses().
    @param pacsHandles pointer to Start handle and End handle of PACS service
    @param bassHandles pointer to Start handle and End handle of ASCS service

    @return The Profile handle for this BAP profile instance. On Success it 
            non-zero value otherwise it returns BAP_INVALID_HANDLE
*/
bapProfileHandle BapServerUnicastInit(AppTask appTask,
                                      uint8 numAses,
                                      const BapServerHandleRange *pacsHandles,
                                      const BapServerHandleRange *ascsHandles);

/*! \brief This API is used in response to BAP_SERVER_ASE_RECEIVER_START_READY_IND
    to signal that source ASE is ready to transmitting audio.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds
 */
void BapServerUnicastAseReceiveStartReadyResponse(bapProfileHandle profileHandle,
                                                  ConnId connectionId,
                                                  uint8 numAses,
                                                  const uint8 *aseIds);

/*! \brief This API is used to release the ASE after CIS is disconnected.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds
    @param cacheCodecEnable whether cacheCodecConfiguration is enabled or not
 */
void BapServerUnicastAseReleased(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 uint8 numAses,
                                 const uint8 *aseIds,
                                 bool cacheCodecEnable);

/*!
    @brief This API is used to initiate the ASE Configure Codec procedure
    BAP_SERVER_ASE_CODEC_CONFIGURED_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param aseCodecInfo    A pointer to a BapServerAseConfigCodecReq structure
                           that describes the codec parameters for ASEs to be taken to the codec configured state
                           and the codec configuration data for each of those ASEs.
                           The Profile/Application is responsible for all memory associated
                           with BapServerAseConfigCodecReq, e.g. the Application/Profile
                           must free any codec_configuration data referenced in the
                           BapServerAseConfigCodecReq
    @return TRUE if success, FALSE otherwise
*/
bool BapServerUnicastAseConfigureCodecReq(bapProfileHandle profileHandle, 
                                          BapServerAseConfigCodecReq *aseCodecInfo);

/*! \brief This API is used when sink ASE is ready to receive audio.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response
    @param aseIds pointer of aseIds

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseReceiveStartReadyReq(bapProfileHandle profileHandle,
                                             ConnId connectionId,
                                             uint8 numAses,
                                             const uint8 *aseIds);

/*! \brief This API is used update metadata of the ASES.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param updateMetadata pointer of BapServerUpdateMetadataInfo.

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseUpdateMetadataRequest(bapProfileHandle profileHandle,
                                              BapServerAseUpdateMetadataReq *updateMetadataReq);

/*! \brief This API is used to initiate Server autonomous disable ASEs procedure
    BAP_SERVER_ASE_DISABLED_IND message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    \param connectionId connection id of the LE audio source
    \param numAses the number of ases in the request
    \param aseIds pointer of aseIds
    @param cisLoss This flag should be set to true when the CIS is lost. When
    the CIS is lost and the ASE is in the Streaming state or the Disabling
    state, the server shall immediately transition that ASE to the QoS
    Configured state

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseDisableReq(bapProfileHandle profileHandle,
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds,
                                   bool cisLoss);

/*! \brief This API is used to initiate Server autonomous release ASEs procedure
    BAP_SERVER_ASE_RELEASED_IND message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    \param connectionId connection id of the LE audio source
    \param numAses the number of ases in the request
    @param aseIds pointer of aseIds

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseReleaseReq(bapProfileHandle profileHandle, 
                                   ConnId connectionId,
                                   uint8 numAses,
                                   const uint8 *aseIds);

/*!
    @brief This API is used to retrieve the Codec Configuration of an ASE.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from
                         which the codec configuration is to be retrieved.
    @param aseId  The id of the ASE from which the codec configuration is to be retrieved.
*/
BapServerAseCodecInfo * BapServerUnicastReadAseCodecConfiguration(bapProfileHandle profileHandle,
                                                                  ConnId connectionId,
                                                                  uint8 aseId);

/*!
    @brief This API is used to retrieve the QOS Configuration of an ASE.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE
                         from which the QOS configuration is to be retrieved.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
*/
BapServerAseQosInfo * BapServerUnicastReadAseQosConfiguration(bapProfileHandle profileHandle,
                                                              ConnId connectionId,
                                                              uint8 aseId);

/*!
    @brief This API is used to retrieve the ASE direction of given ASE ID.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from which the QOS configuration is to be retrieved.
    @param aseId  The id of the ASE from which the QOS configuration is to be retrieved.
*/
AseDirectionType  BapServerUnicastReadAseDirection(bapProfileHandle profileHandle,
                                                   ConnId connectionId,
                                                   uint8 aseId);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
bapStatus BapServerUnicastAddAscsConfig(bapProfileHandle profileHandle,
                                        ConnId connectionId,
                                        const BapAscsConfig * config);

/*! \brief This API is used to remove the configuration for a peer device,
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapAscsConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapAscsConfig * BapServerUnicastRemoveAscsConfig(bapProfileHandle profileHandle,
                                                 ConnId connectionId);

/*! \brief This API is used to create an ISO data path between Host and
    the controller for the CIS or BIS identfied by isoHandle. 
    BAP_SERVER_SETUP_DATA_PATH_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param isoDataType ISO data for this datapath.
    @param dataPathParameters Iso data path parameters
 */
void BapServerSetupIsoDataPathReq(bapProfileHandle profileHandle,
                                  BapIsoDataType isoDataType,
                                  const BapServerSetupDataPathReq *dataPathParameters);

/*! \brief This API is used to remove an ISO data path between Host and
    the controller for the CIS or BIS identfied by isoHandle. 
    BAP_SERVER_REMOVE_DATA_PATH_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param isoHandle     Iso handle of the CIS or BIS.
    @param isoDataType ISO data for this datapath.
    @param dataPathDirection Direction of the audio datapath.
 */
void BapServerRemoveIsoDataPathReq(bapProfileHandle profileHandle,
                                   uint16 isoHandle,
                                   BapIsoDataType isoDataType,
                                   uint8 dataPathDirection);

/*! \brief This API is use disconnect the CIS identfied by cisHandle. 
    BAP_SERVER_CIS_DISCONNECTED_CFM message will be sent to the registered app Task.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param cisHandle     cisHandle handle of the CIS.
    @param disconnectReason Reason for disconnection.

 */
void BapServerUnicastCisDisconnectReq(bapProfileHandle profileHandle,
                                      uint16 cisHandle,
                                      uint8 disconnectReason);


/*! \brief This API is for sending ASE Enable operation response.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ases in the response of bapServerAseResult
    @param bapServerAseResult pointer of BapServerAseResult has 'numAses' elements

    @return TRUE if success, FALSE otherwise
 */
bool BapServerUnicastAseEnableRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult);

/* PACS API */

/*!
    @brief This API is used to add a PAC record for SINK/SOURCE. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE PAC record.
    @param pacRecord A pointer to gatt_pacs_server_record_t of SINK/SOURCE PAC

    @return pac_record_handle if successful, errors mentioned in pacs_record_error_t otherwise.

*/
PacsRecordHandle BapServerAddPacRecord(bapProfileHandle profileHandle, 
                                       PacsDirectionType pacsDirection,
                                       const BapServerPacsRecord * pacRecord);


/*!
    @brief This API is used to add a vendor specific PAC record for SINK/SOURCE. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE PAC record.
    @param pacVSRecord A pointer to BapServerVSPacsRecord of SINK/SOURCE PAC

    @return pac_record_handle if successful, errors mentioned in pacs_record_error_t otherwise.

*/

PacsRecordHandle BapServerAddVSPacRecord(bapProfileHandle profileHandle,
                               PacsDirectionType pacsDirection,
                               const BapServerVSPacsRecord *pacVSRecord);

/*!
    @brief This API is used to get a registered PAC record corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return A const pointer to BapServerPacsRecord if success, NULL otherwise

*/
const BapServerPacsRecord * BapServerGetPacRecord(bapProfileHandle profileHandle,
                                                  uint16 pacRecordHandle);

/*!
    @brief This API is used to get a registered Vendor specific PAC record
    corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return A const pointer to BapServerVSPacsRecord if success, NULL otherwise

*/
const BapServerVSPacsRecord * BapServerGetVSPacRecord(bapProfileHandle profileHandle,
                                                  uint16 pacRecordHandle);

/*!
    @brief This API is used to remove a registered PAC record corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemovePacRecord(bapProfileHandle profileHandle,
                              PacsRecordHandle pacRecordHandle);

/*!
    @brief This API is used to remove a registered Vendor specific PAC record
           corresponds to the pacRecordHandle. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacRecordHandle Returned as part of BapServerAddPacRecord() for PAC record

    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemoveVSPacRecord(bapProfileHandle profileHandle,
                              PacsRecordHandle pacRecordHandle);

/*!
    @brief This API is used to add Audio Location of SINK/SOURCE PAC.
    A device may choose to have multiple bitmask of AudioLocationType or
    single AudioLocationType. Application may send complete bitmask by
    calling this API once or by sending different single AudioLocationType
    by calling this API multiple times.

    This API essentially OR's the single or multiple bit mask with previously
    set Audio Location of SINK/SOURCE PAC.

    For example: 
    If Application wants to set AudioLocation as both Left & Right, it can do
    so by below ways:
    a) Application may choose to send 'audioLocations' field as
    AUDIO_LOCATION_FRONT_LEFT | AUDIO_LOCATION_FRONT_RIGHT by calling this
    API once.
    b) Or application may chose to send 'audioLocation' field as
    AUDIO_LOCATION_FRONT_LEFT first and then by again calling this API 
    with 'audioLocation' fiedl as AUDIO_LOCATION_FRONT_RIGHT.
    In first case (a) there will be single Audio Location change notification
    send to connected client(s). 
    In second case (b), there will be mulitple Audio Location change
    notification send to connected client(s), one for Left and another
    for Left | Right.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data

    @return TRUE if success, FALSE otherwise
*/
bool BapServerAddPacAudioLocation(bapProfileHandle profileHandle,
                                  PacsDirectionType pacsDirection,
                                  AudioLocationType audioLocations);

/*!
    @brief This API is used to get Audio Location set for SINK or SOURCE configuration. 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param direction Direction specifies the SINK or SOURCE audio locations.

    @return Audio Location if success, 0 otherwise

*/
AudioLocationType BapServerGetPacAudioLocation(bapProfileHandle profileHandle,
                                               PacsDirectionType pacsDirection);

/*!
    @brief This API is used to remove Audio Location of SINK/SOURCE PAC

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioLocations Device wide bitmap of supported Audio Location values
           for all PAC records where the server supports reception/transmission
           of audio data

    NOTE:
    1)If Application passes a subset of Device Audio Location to be removed,then
      subset of audiolocation shall be removed from Device Audio Location set
      during BapServerAddPacAudioLocation() API and a notification to the 
      connected client(s) with the new Device Audio Location for SINK/SOURCE
      PAC shall be triggerred.

    2)If Application wants to clear the Device Audio Location for SINK/SOURCE
      PAC, then it may pass 'audioLocations' field with value AUDIO_LOCATION_CLEAR. 
      This will remove Device Audio Location set for 'direction' (SINK or
      SOURCE) passed and will not trigger any notification change to connected
      client(s). The Application has to then call BapServerAddPacAudioLocation()
      with proper bitmask of AudioLocationType to be set for the 'direction' 
      PAC immediately after BapServerRemovePacAudioLocation() API.

    @return TRUE if success, FALSE otherwise

*/
bool BapServerRemovePacAudioLocation(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioLocationType audioLocations);

/*!
    @brief This API is used to add Supported Audio Contexts for SINK and SOURCE contexts.
           The API is also used to add Available Audio Contexts for SINK and SOURCE contexts
           at any point from Supported Audio Contexts list.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies the SINK or SOURCE audio contexts.
    @param audioContext Bitmask of audio data Context Type values for transmission/reception.
    @param contexts Supported or Available audio context defined by pacs_server_audio_context_t

    @return TRUE if success, FALSE otherwise

    NOTE: This API will return failure for Available Audio Context addition, 
    if Available Audio Context Control is with the Higher layer.
    Higher layer needs to call BapServerEnableAvailableAudioContextControl() API  after 
    BapServerUnicastInit() API to get the Avaiable Audio Context Control.
*/
bool BapServerAddPacAudioContexts(bapProfileHandle profileHandle,
                                  PacsDirectionType pacsDirection,
                                  AudioContextType audioContext,
                                  PacAudioContextType contexts);

/*!
    @brief This API is used to get Available Audio Contexts for SINK or SOURCE configuration 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies the SINK or SOURCE supported audio contexts.

    @return Available Audio Contexts if success, 0 otherwise

    NOTE: This API will return AUDIO_CONTEXT_TYPE_UNKNOWN, if Available Audio
    Context Control is with the higher layer.
*/
AudioContextType BapServerGetPacAvailableContexts(bapProfileHandle profileHandle,
                                                  PacsDirectionType pacsDirection);

/*!
    @brief This API is used to remove Supported or Available Audio Contexts for SINK or SOURCE
           configuration 

    @param profileHandle The Profile handle of this BAP Server instance.
    @param pacsDirection Direction specifies SINK/SOURCE Audio location.
    @param audioContext Bitmask of audio data Context Type values for transmission/reception.
    @param contexts Supported or Available audio context defined by pacs_server_audio_context_t


    @return TRUE if success, FALSE otherwise

    NOTE: This API will return FALSE, if Available Audio Context Control is with the higher
    layer and higher layer tries to call this API for Avaiable Audio Context Removal.
*/
bool BapServerRemovePacAudioContexts(bapProfileHandle profileHandle,
                                     PacsDirectionType pacsDirection,
                                     AudioContextType audioContext,
                                     PacAudioContextType contexts);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
 
bapStatus BapServerAddPacsConfig(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 BapPacsConfig * config);

/*! \brief Remove the configuration for a peer device, identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapPacsConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapPacsConfig * BapServerRemovePacsConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId);

/*! \brief This API is used to Add configuration for a paired peer device, 
    identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @config config Pointer to Connected Clinet characteristics configurations

    @return bapStatus status of the Add Configuration operation.

 */
bapStatus BapServerAddBassConfig(bapProfileHandle profileHandle,
                                 ConnId connectionId,
                                 BapBassConfig * config);

/*! \brief Remove the configuration for a peer device, identified by its connectionId

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId connection id of the LE audio source

    @return BapBassConfig  Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
 */
BapBassConfig * BapServerRemoveBassConfig(bapProfileHandle profileHandle,
                                          ConnId connectionId);


/*! \brief This API is used to Get Service configuration for a paired peer device, 
    identified by its connectionId.

    It is recommnded to call this API after BAP_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param configType Service config required for defined BapServerConfigType

    @return config pointer for the configType passed.
           For PACS the return pointer will be BapPacsConfig if valid else NULL
           For ASCS the return pointer will be BapAscsConfig if valid else NULL
           For BASS the return pointer will be BapBassConfig if valid else NULL
           It is the applications responsibility to free the pointer memory.

 */
void* BapServerGetServiceConfig(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  BapServerConfigType configType);


/* Advertisement API */

/*! \brief Request to get number of Advertising elements

    @param ServerRole  Role of the Server for Advertising

    @return uint16  Number of Advertising data items.
 */
uint16 BapServerGetNumberOfAdvertisingItems( BapServerRole ServerRole);

/*! \brief Request to get Advertising data

    @param ServerRole  Role of the Server for Advertising

    @return leAdvDataItem  Pointer to LE Advertising data.

    NOTE: This API should not be called for ServerRole = BAP_SERVER_UNICAST_ROLE,
    if Available Audio Context control is with higher layer.
 */
leAdvDataItem *BapServerGetAdvertisingData( BapServerRole ServerRole);

/*! \brief Request to get Bap Unicast Server Targeted Advertising data

    @return leAdvDataItem  Pointer to LE Advertising data.

    NOTE: This API should not be called if Available Audio Context control
    is with higher layer.
 */
leAdvDataItem *BapServerGetTargetedAdvertisingData(void);

/*! \brief Request to free the Advertising data

    @param ServerRole  Role of the Server for Advertising

    NOTE: This API should not be called for ServerRole = BAP_SERVER_UNICAST_ROLE,
    if Available Audio Context control is with higher layer.
 */
void BapServerReleaseAdvertisingItems( BapServerRole ServerRole);

/*! \brief Request to get Advertising data size

    \param server_role  Role of the Server for Advertising

    \return The size of the advertising data.
 */
unsigned BapServerGetAdvertisingDataSize(BapServerRole server_role);

/*!
    @brief Initialises the Basic Audio Profile Library for Broadcast Role.

    @param theAppTask The AppTask that will receive the messages sent from this BAP library.
    @param numberBroadcastSources The number of broadcast sources to allow on BAP broadcast server instance
    @param pacsHandles pointer to Start handle and End handle of PACS service
    @param bassHandles pointer to Start handle and End handle of BASS service

    @return The Profile handle for this BAP profile instance.

*/
bapProfileHandle BapServerBroadcastInit(AppTask appTask,
                                        uint8 numberBroadcastSources,
                                        const BapServerHandleRange *pacsHandles,
                                        const BapServerHandleRange *bassHandles);

/*!
    @brief Get the number and the values of the source IDs of all the Broadcast Receive State characteristics.

    @param profileHandle The BAP instance profile handle.
    @param sourceIdNum Pointer to the variable in which the library will put the number of Source IDs.

    @return uint8 * pointer to the list of values of the Source IDs. It's responsibility of the application
                    to free the memory associated to this pointer.
*/
uint8 * BapServerGetSourceIdsReq(bapProfileHandle profileHandle,
                                 uint16 *sourceIdNum);

/*!
    @brief Add a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.
    @param sourceInfo   Pointer to the structure containing the info of the broadcast source to set
                         in the Receive State characteristic.
                         It is the application's responsibility to free the memory of this pointer.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerAddBroadcastSourceReq(bapProfileHandle profileHandle,
                                          uint8 *sourceId,
                                          BapServerBassReceiveState *sourceInfo);


/*!
    @brief Get the info of a specific broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.
    @param state         Pointer to the structure containing all the info.
                         The gatt_bass_server_receive_state_t structure contains a pointer (metadata):
                         it is the application's responsibility to free the memory of the this pointer.

    @return gatt_bass_server_status_tbapStatus Result of the operation.

*/
bapStatus BapServerGetBroadcastReceiveStateReq(bapProfileHandle profileHandle,
                                               uint8 sourceId,
                                               BapServerBassReceiveState *state);

/*!
    @brief Modify a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source to modify.
    @param sourceInfo   Pointer to the structure containing the info of the broadcast source to modify
                         in the Receive State characteristic.
                         It is the application responsibility to free the memory of this pointer.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerModifyBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId,
                                             BapServerBassReceiveState *sourceInfo);

/*!
    @brief Remove a broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source to remove.

    @return bapStatus Result of the operation.

*/
bapStatus  BapServerRemoveBroadcastSourceReq(bapProfileHandle profileHandle,
                                             uint8 sourceId);

/*!
    @brief Get the Broadcast Code of a specific broadcast source.

    @param profileHandle The BAP instance profile handle.
    @param sourceId     ID of the specific broadcast source.

    @return Broadcast Code for that Broadcast Source.

*/
uint8* BapServerGetBroadcastCodeReq(bapProfileHandle profileHandle,
                                    uint8 sourceId);

/*!
    @brief Set the Broadcast Code of a specific broadcast source.

    @param profileHandle  The BAP instance profile handle.
    @param sourceId      ID of the specific broadcast source.
    @param broadcastCode The broadcast code to set.

    @return bapStatus Result of the operation.

*/
bapStatus BapServerSetBroadcastCodeReq(bapProfileHandle profileHandle,
                                       uint8 sourceId,
                                       uint8 *broadcastCode);

/*! \brief Query if the server has any client connections.
    @param srvcHndl The GATT service instance handle
    .
    @param profileHandle  The BAP instance profile handle.
    @return TRUE if any clients are connected to the server.
*/
bool BapServerIsAnyClientConnected(bapProfileHandle profileHandle);

/*!
    @brief This API is called to synchronize to  BIG described in the periodic advertising
          train specified by the SyncHandle parameter.
    BAP_SERVER_ISOC_BIG_CREATE_SYNC_CFM message will be sent to the registered app Task.

    @param appTask The AppTask that will receive the BIS messages sent from this BAP library.
                   This can be set to CSR_SCHED_QID_INVALID if appTask is same used in BapServerBroadcastInit()
    @param profileHandle  The BAP instance profile handle.
    @param syncHandle:         Identifies the associated periodic advertising train of the BIG
    @param bigSyncTimeout     Maximum permitted time between successful receptions of BIS PDUs
    @param bigHandle:          Identifier of the BIG
    @param mse                  maximum number of subevents that a Controller should use to receive
                                data payloads in each interval for a BIS.
    @param encryption:          Encryption mode of the BISes( 0- unencrypted 1-encrypted)
    @param broadcastCode:      Encryption key(size 16) for encrypting payloads of all BISes.
    @param numBis              Total number of BISes indices specified in the BIS[i] parameter
    @param bis                  List of indices corresponding to BIS(es) in the synchronized BIG

*/
void BapServerBroadcastBigCreateSyncReq(AppTask appTask,
                                        bapProfileHandle profileHandle,
                                        uint16 syncHandle,
                                        uint16 bigSyncTimeout,
                                        uint8 bigHandle,
                                        uint8 mse,
                                        uint8 encryption,
                                        uint8 *broadcastCode,
                                        uint8 numBis,
                                        uint8 *bis);

/*!
    @brief This API is called to terminate or cancel the syncronised BIS identified by BigHandle.
    BAP_SERVER_ISOC_BIG_TERMINATE_SYNC_IND message will be sent to the registered app Task.

    @param profileHandle  The BAP instance profile handle.
    @param bigHandle:          Identifier of the BIG

*/
void BapServerBroadcastBigTerminateSyncReq(bapProfileHandle profileHandle,
                                           uint8 bigHandle);
/*!
    @brief This API is called to populate GattAscsServerConfigureCodecServerReqInfo data for.
    Codec config response by the lower services.

    @param cid  Connection Identifier of the Client.
    @param aseId Audio stream Identifier of the associated codec config response.
    @param configureCodecData  codec config response data.

*/
void BapServerUnicastPopulateConfigureCodecData(ConnectionId cid, uint8 aseId, GattAscsServerConfigureCodecServerReqInfo * configureCodecData);


/*! \brief This API is used to Register presentation delay lookup table by the 
     higher layer.

    @param profileHandle  The BAP instance profile handle.
    @param bapServerCodecPdMin A pointer to a BapServerCodecPdMin structure
                           that describes PD min table
    @param numEntries Number of rows in the bapServerCodecPdMin

    @return TRUE if success, FALSE otherwise

 */
bool BapServerUnicastRegisterCodecPdMin(bapProfileHandle profileHandle,
                                        const BapServerCodecPdMin *bapServerCodecPdMin,
                                        uint8 numEntries);

/*! \brief This API is used to find values of specific Types from the Audio Stream
     Endpoint LTV stucture.

    @param ltvData  Pointer to LTV data.
    @param ltvDataLength length of ltvData pointer
    @param type Type field of LTV stucture
    @param value Pointer to Value filed to be returned from LTV stucture
    @param valueLength length of value pointer

    @return TRUE if success, FALSE otherwise

 */
bool BapServerLtvUtilitiesFindLtvValue(uint8 * ltvData, uint8 ltvDataLength,
                                              uint8 type, uint8 * value,
                                              uint8 valueLength);

/*! \brief This API is used to find values of specific Types from within the vendor specific LTV data
    @param ltvData  Pointer to LTV data.
    @param ltvDataLength length of ltvData pointer
    @param type Type field of LTV stucture
    @param value Pointer to Value filed to be returned from LTV stucture
    @param valueLength length of value pointer

    @return TRUE if success, FALSE otherwise

 */
bool BapServerLtvUtilitiesFindLtvValueFromVsMetadata(uint8 * ltvData, uint8 ltvDataLength,
                                                     uint8 type ,uint8 * value, uint8 valueLength);


/*! \brief This API is used to get Sampling frequency value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint32  sampling frequency value.

 */
uint32 BapServerLtvUtilitiesGetSampleRate(uint8 * config, uint8 configLength);

/*! \brief This API is used to get Frame duration value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint16  Frame duration value.

 */
uint16 BapServerLtvUtilitiesGetFrameDuration(uint8 * config, uint8 configLength);

/*! \brief This API is used to get Streaming audio context from LTV structure.

    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata

    @return AudioContextType  Streaming Audio context.

 */
AudioContextType BapServerLtvUtilitiesGetStreamingAudioContext(uint8 * metadata,
                                                         uint8 metadataLength);

/*! \brief This API is used to get Audio channel allocation value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint32  Audio channel allocation

 */
uint32 BapServerLtvUtilitiesGetAudioChannelAllocation(uint8 * config,
                                                      uint8 configLength);

/*! \brief This API is used to get Number of Octets per Frame value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint16  Number of Octets per Frame

 */
uint16 BapServerLtvUtilitiesGetOctetsPerCodecFrame(uint8 * config,
                                                    uint8 configLength);


/*! \brief This API is used to get Codec frame blocks per SDU value from LTV structure.

    @param config  Pointer to LTV data.
    @param configLength Length of config

    @return uint8  Number of Codec frame blocks per SDU value

 */
uint8 BapServerLtvUtilitiesGetCodecFrameBlocksPerSdu(uint8 * config,
                                                        uint8 configLength);

/*! \brief This API is used validate values range from LTV structure.

    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata
    @param invalidMetadataType Pointer to Type value of metadata

    @return bapServerResponse  Result of validation

 */
bapServerResponse BapServerValidateMetadataLtvs(uint8* metadata,
                                                   uint8 metadataLength,
                                                   uint8* invalidMetadataType);

/*! \brief This API is used to validate Streaming Audio context from PACS available
     Audio context(Non Selective audio context).

    Note: This utility function shall not be called if selective available audio context
    is set using BapServerSetSelectiveAvailableAudioContexts().
    Parameter 'metadata'and 'metadataLength' will not be modified.

    @param profileHandle  The BAP instance profile handle.
    @param connectionId connection id of the LE audio source
    @param aseId ASE Id of Sink or Source ASE
    @param metadata  Pointer to LTV data.
    @param metadataLength Length of metadata

    @return TRUE if success, FALSE otherwise

 */
bool BapServerValidateStreamingContext(bapProfileHandle profileHandle, 
                                      ConnId connectionId,uint8 aseId,
                                      uint8 * metadata, uint8 *metadataLength);

/*! \brief This API is used to get offset of the LTV type from the ltvData.

    @param ltvData  Pointer to LTV data.
    @param ltvDataLength Length of config
    @param type Type field of LTV
    @param offset offset of the LTV type

    @return bool  True if LTV type present else false

 */
bool BapServerLtvUtilitiesFindLtvOffset(uint8 * ltvData, uint8 ltvDataLength,
                                                    uint8 type, uint8 * offset);


/*! \brief This API is used to set Unicast Server preferred Max transport latancey 
    and retransmission number value.
    This API shall be called once just after BAP unicast server is initialised.

    @param profileHandle  The BAP instance profile handle.
    @param bapServerQosParams A pointer to a BapServerQosParams structure
           that populate Unicast Server supported Max Transport latency and RTN value.

    @return TRUE if success, FALSE otherwise

 */
bool BapServerUnicastSetQosParams(bapProfileHandle profileHandle,
                             const BapServerQosParams *bapServerQosParams);


/*! \brief An API to retrieve the ASE Ids which are in Releasing state associated with a CIS.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param cisId  The id of the CIS for which ASE ID's is to be retrieved.

    @return A pointer to a BapServerReleasingAseInfo structure of matching ASE Id's.
            This structure contains the number of ASE Id's matched for the CIS ID.
            This pointer to be freed by the caller.
 */
BapServerReleasingAseInfo* BapServerUnicastReadReleasingAseIdsByCisId(bapProfileHandle profileHandle,
                                 ConnId connectionId, uint8 cisId );

/*!
    @brief This API is used to set selective "Available Audio Contexts" values 
    for SINK and SOURCE context to notify a remote connected client identified 
    by 'connectionId'. The values of Sink/Source Available Audio Contexts selected  
    from the list of Supported Audio Contexts values.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE ACL connection id of the remote connected client
    @param sinkAudioContexts Bitmask of audio data Context Type values for Sink
    @param sourceAudioContexts Bitmask of audio data Context Type values for Source

    @return TRUE if success, FALSE otherwise

    NOTE: Once the link with remote device is disconnected then this information
    is flushed in PACS Server. During subsequent re-connection with the remote
    device, the application needs to set selective Audio Context Availability
    again for the connected client.

    This API is useful in case of Multidevice setup where Server is connected
    to multiple clients and may want to allow availabilty of Selective Available
    Audio context to client(s) for certain use cases as desired.

    An example could be:User listening music from their PC on their earbuds is 
    interrupted by an incoming call from their Phone ( as mentioned in BT SIG 
    white paper titled "Low Energy Audio - Incoming Call over Media (Multi-Device))
    v1 14-02-2023. 

    In the use case mentioned in the white paper, User is listening to music from
    PC on their earbud. Phone is paired and is not connected when user is listening
    music. When Phone call arrives the phone establishes a connection with the 
    earbuds and sends an in-band ringtone to both earbuds. The Earbud decides to 
    release the music audio from the PC to free the necessary resources to support
    the in-band ringtone from the phone.

    After the user accepts the call using the earbuds’ user interface (UI), the phone
    changes the Streaming Audio Contexts values of the <<Ringtone>> Audio Streams
    (which carry the in-band ringtone). The phone switches the value from <<Ringtone>>
    to <<Conversational>>. In addition, the phone starts a <<Conversational>> Audio 
    Stream from one earbud to the phone to carry the voice signal from the earbud’s
    microphone. When the user ends the call, the PC restarts the original <<Media>>
    Audio Streams and resumes sending music.

    To achieve this use case mentioned in white paper, Higher layer can take
    complete control of Available Audio Context by calling 
    BapServerEnableAvailableAudioContextControl() after BapServerUnicastInit().

    After the PC is connected, it can call this API to set
    Available Audio Context for this connected client. The Application can decide
    what Available Audio Context it wants to set for this connected client
    anytime.

    When music is streaming on Earbud, the <<Media>> Audio context is taken by PC.
    If Phone connects now for an incoming call, the Application can chose what
    available audio context it wants to share with Phone.

    If it wants to change Available Audio Context with PC, then it may do so
    by calling this API again and set whatever it wants to set for 
    PC. For example, if Application wants only <<Conversational>> Audio
    context allowed for PC during ongoing call with Phone, then it can
    set Available Audio Context as <<Conversational>> using this API.

    Once the call is finished with Phone, the Earbud may choose to
    set Available Audio context value as desired with PC and Phone.
*/
bool BapServerSetSelectiveAvailableAudioContexts(
                               bapProfileHandle profileHandle,
                               ConnId connectionId,
                               AudioContextType sinkAudioContexts,
                               AudioContextType sourceAudioContexts);

/*!
    @brief This API is used to clear Sink and Source "Available Audio Contexts"
    values set previously for a  remote connected client.
    This API triggers the notification to the connected client with the list
    of Available Audio Contexts in Earbud.
    
    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE ACL connection id of the remote connected client

    @return TRUE if success, FALSE otherwise

    NOTE: This API should not be called if Available Audio Context control
    is with higher layer.
*/
bool BapServerClearSelectiveAvailableAudioContexts(
                               bapProfileHandle profileHandle,
                               ConnId connectionId);

/*! \brief This API is for sending ASE Update Metadata operation response.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId LE audio connection id
    @param numAses the number of ASES in the response of bapServerAseResult
    @param bapServerAseResult pointer of BapServerAseResult has 'numAses' elements

    @return TRUE if success, FALSE otherwise

 */
bool BapServerUnicastAseUpdateMetadataRsp(bapProfileHandle profileHandle,
                                  ConnId connectionId,
                                  uint8 numAses,
                                  const BapServerAseResult *bapServerAseResult);


/*!
    @brief This API is used to get the ASE data of given ASE ID as per the current ASE state.

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from
                         which the ASE data is to be retrieved.
    @param aseId  The id of the ASE from which the data is to be retrieved.

    @return BapServerAseData if ASE ID is valid else NULL.
            Caller shall free return pointer BapServerAseData along with its member pointer.
*/
BapServerAseData* BapServerUnicastGetAseData(bapProfileHandle profileHandle,
                                             ConnId connectionId,                                
                                             uint8 aseId);

/*!
    @brief This API is used to enable Available Audio Context control in higher layer.
    This API also enables a new indication message
    BAP_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND to be sent to higher layer when 
    remote connected client does a GATT Read procedure for PACS Available Audio
    Context attribute. The higher layer shall have to now respond with a 
    BapServerAvailableAudioContextReadResponse() API for read response to
    that connected client.

    Higher layer shall call BapServerSetSelectiveAvailableAudioContexts() to set
    Available Audio context for connected client. Refer documentation for
    BapServerSetSelectiveAvailableAudioContexts() API for details.

    NOTE: This API has to be called only once after BapServerUnicastInit()

    @param profileHandle The Profile handle of this BAP Server instance.
*/
void BapServerEnableAvailableAudioContextControl(bapProfileHandle profileHandle);

/*!
   @brief This API is called by higher layer when it receives 
          BAP_SERVER_AVAILABLE_AUDIO_CONTEXT_READ_IND for a connected client.
          The reponse by higher layer will be send as it is as a response to
          connected client.

    NOTE: This API has to be called if higher layer has taken Available Audio Context control by
          setting BapServerEnableAvailableAudioContextControl() API after BapServerUnicastInit()

    @param profileHandle The Profile handle of this BAP Server instance.
    @param connectionId  The id of the connection that contains the ASE from
                         which the ASE data is to be retrieved.
    @param sinkAudioContexts Bitmask of audio data Context Type values for Sink
    @param sourceAudioContexts Bitmask of audio data Context Type values for Source
*/
bool BapServerAvailableAudioContextReadResponse(bapProfileHandle profileHandle,
                               ConnId connectionId,
                               AudioContextType sinkAudioContexts,
                               AudioContextType sourceAudioContexts);


#ifdef __cplusplus 
}
#endif

#endif

