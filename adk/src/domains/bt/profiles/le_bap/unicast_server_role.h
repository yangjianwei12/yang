/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   le_bap  LE BAP
    @{
    \ingroup    profiles
    \brief      Header file for LE Bap Unicast Server Interfaces
*/

#ifndef UNICAST_SERVER_ROLE_H_
#define UNICAST_SERVER_ROLE_H_

#include "bt_types.h"
#include "gatt_ascs_server.h"
#include "connection_no_ble.h"
#ifdef USE_SYNERGY
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif

/*! \brief BAP unicast profile handle */
extern ServiceHandle bapUnicastServiceHandle;

/*! \brief Initialises the BAP Unicast Server role

    Initialises the ASCS, adds LE advertising data and adds mandatory unicast PACS records

    \param number_of_ases max number of supported ases
    \param bap_handler_task Task that handles BAP unicast messages
 */
void LeBapUnicastServer_Init(uint8 number_of_ases, Task bap_handler_task);

/*! \brief This API is used to initiate the ASE Configure Codec procedure

    \param aseCodecInfo    A pointer to a BapServerAseConfigCodecReq structure

    \return TRUE if success, FALSE otherwise
 */
#define LeBapUnicastServer_AseConfigureCodecReq(aseCodecInfo) \
    BapServerUnicastAseConfigureCodecReq(bapUnicastServiceHandle, aseCodecInfo)

/*! \brief To be called in response to LeBapUnicastServer_AseReceiveReady callback to signal that source ASE is
     transmitting audio.

    \param connection_id LE audio connection id
    \param numAses the number of ases in the response
    \param aseIds[] array of ase ids
 */
#define LeBapUnicastServer_AseReceiveReadyResponse(connection_id, numAses, aseIds) \
    BapServerUnicastAseReceiveStartReadyResponse(bapUnicastServiceHandle, connection_id, numAses, aseIds)

/*! \brief To be called in response to LeBapUnicastServer_AseRelease callback.

    \param connection_id LE audio connection id
    \param numAses the number of ases in the response
    \param aseIds[] array of ase ids
    \param cacheCodecEnable Flag to enable use of config caching
 */
#define LeBapUnicastServer_AseReleasedRequest(connection_id, numAses, aseIds, cacheCodecEnable) \
    BapServerUnicastAseReleased(bapUnicastServiceHandle, connection_id, numAses, aseIds, cacheCodecEnable)

#if defined(INCLUDE_LE_AUDIO_UNICAST)
/*! \brief This API is used update metadata of the ASES.

    \param updateMetadata pointer of GattAscsServerUpdateMetadataReq.

    \return TRUE if success, FALSE otherwise
 */
#define LeBapUnicastServer_AseUpdateMetadataRequest(updateMetadataReq) \
    BapServerUnicastAseUpdateMetadataRequest(bapUnicastServiceHandle, updateMetadataReq)
#endif

/*! \brief To be called when sink ASE is ready to receive audio.

    \param connection_id LE audio connection id
    \param numAses the number of ases in the response
    \param aseIds[] array of ase ids
 */
#define LeBapUnicastServer_AseReceiveReadyRequest(connection_id, numAses, aseIds) \
    BapServerUnicastAseReceiveStartReadyReq(bapUnicastServiceHandle, connection_id, numAses, aseIds)

/*! \brief Request to disable ASEs

    \param connection_id connection id of the LE audio source
    \param numAses the number of ases in the request
    \param aseIds[] array of ase ids
 */
#define LeBapUnicastServer_AseDisableRequest(connection_id, numAses, aseIds, cisLoss) \
    BapServerUnicastAseDisableReq(bapUnicastServiceHandle, connection_id, numAses, aseIds, cisLoss)

/*! \brief Request to release ASEs

    \param connection_id connection id of the LE audio source
    \param numAses the number of ases in the request
    \param aseIds[] array of ase ids
 */
#define LeBapUnicastServer_AseReleaseRequest(connection_id, numAses, aseIds) \
    BapServerUnicastAseReleaseReq(bapUnicastServiceHandle, connection_id, numAses, aseIds)

/*! \brief Read the Releasing Ase Ids by Cis Id

    \param connection_id connection id of the LE audio source
    \param cisId Cis Id for which the ase with releasing state is required
    \return A pointer to a BapServerReleasingAseInfo structure of matching ASE Id's.
            This structure contains the number of ASE Id's matched for the CIS ID.
            This pointer to be freed by the caller.
 */
#define LeBapUnicastServer_ReadReleasingAseIdsByCisId(connection_id, cisId) \
    BapServerUnicastReadReleasingAseIdsByCisId(bapUnicastServiceHandle, connection_id, cisId)

/*! \brief Get Codec Configuration for an ASE

    \param connection_id connection id of the LE audio source
    \param aseId from which the codec configuration has to be retrieved
    \return returns a pointer to a constant codec data.
 */
#define LeBapUnicastServer_GetCodecParameters(connection_id, ase_id) \
    BapServerUnicastReadAseCodecConfiguration(bapUnicastServiceHandle, connection_id, ase_id)

/*! \brief Request to get QoS Parameters for an ASE

    \param connection_id connection id of the LE audio source
    \param aseId from which the qos configuration has to be retrieved
    \return returns a pointer to a constant qos data.
 */
#define LeBapUnicastServer_GetQoSParameters(connection_id, ase_id) \
    BapServerUnicastReadAseQosConfiguration(bapUnicastServiceHandle, connection_id, ase_id)

/*! \brief Request to get direction for an ASE

    \param connection_id connection id of the LE audio source
    \param aseId from which the qos configuration has to be retrieved
    \return returns a ASE direction.
 */
#define LeBapUnicastServer_GetAseDirection(connection_id, ase_id) \
    BapServerUnicastReadAseDirection(bapUnicastServiceHandle, connection_id, ase_id)

/*! \brief Creates data path for a Isochronous CIS Link.

    \param bap_handler_task Task that handles BAP unicast messages
    \param cis_handle The handle of the CIS Link
    \param host_to_controller Setup host to controller data path or not.
    \param controller_to_host Setup controller to host data path or not.
    \param is_cis_delegated Indicates if CIS is delegated or not.
*/
void LeBapUnicastServer_CreateDataPath(Task bap_handler_task, uint16 cis_handle, bool host_to_controller, bool controller_to_host,
                                       bool is_cis_delegated);

/*! \brief Removes data paths for an isochronous CIS.

    \param cis_handle The handle of the CIS.
    \param direction_mask Bit mask specifying the data path direction(s) to be removed as described
                          in the Bluetooth Core Specification "LE Remove ISO Data Path command".
*/
void LeBapUnicastServer_RemoveDataPath(uint16 cis_handle, uint8 direction_mask);

/*! \brief Get a pointer to the FT Infomation Pointer for the current connection and ase

    \param connection_id connection id of the LE audio source
    \param ase_id from which the FT information has to be retrieved
*/
#define LeBapUnicastServer_GetFTParametersPTR(connection_id, ase_id) \
    BapServerUnicastReadAseFTsConfiguration(bapUnicastServiceHandle, connection_id, ase_id)

/*! \brief This API is used to Get Service configuration for a paired peer device, 
    identified by its connectionId.

    It is recommnded to call this API after BAP_SERVER_CONFIG_CHANGE_IND
    is sent by library with configChangeComplete set to TRUE

    \param connectionId LE audio connection id
    \param configType Service config required for defined BapServerConfigType

    \return config pointer for the configType passed.
           For PACS the return pointer will be BapPacsConfig if valid else NULL
           For ASCS the return pointer will be BapAscsConfig if valid else NULL
           For BASS the return pointer will be BapBassConfig if valid else NULL
           It is the applications responsibility to free the pointer memory.
 */
#define LeBapUnicastServer_GetServiceConfig(connection_id, configType) \
    BapServerGetServiceConfig(bapUnicastServiceHandle, connection_id, configType);

#endif /* UNICAST_SERVER_ROLE_H_ */
/*! @} */
