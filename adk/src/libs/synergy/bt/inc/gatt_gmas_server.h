/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

/*!
@file    gatt_gmas_server.h
@brief   Header file for the GATT GMAS (Gaming Audio Service) Server library.

        This file provides documentation for the GATT GMAS server library
        API (library name: gatt_gmas_server).
*/

#ifndef GATT_GMAS_SERVER_H_
#define GATT_GMAS_SERVER_H_

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"

/* Maximum number of GATT connections */
#define GATT_GMAS_MAX_CONNECTIONS (3)


typedef uint8 GmasRole;
#define GMAS_ROLE_UNICAST_GAME_GATEWAY                ((GmasRole)0x01)
#define GMAS_ROLE_UNICAST_GAME_TERMINAL               ((GmasRole)0x02)
#define GMAS_ROLE_BROADCAST_GAME_SENDER               ((GmasRole)0x04)
#define GMAS_ROLE_BROADCAST_GAME_RECEIVER             ((GmasRole)0x08)

typedef uint8 GmasUggFeatures;
#define GMAS_UGG_MULTIPLEX_FEATURE_SUPPORT             ((GmasUggFeatures)0x01)
#define GMAS_UGG_96KBPS_SOURCE_FEATURE_SUPPORT         ((GmasUggFeatures)0x02)
#define GMAS_UGG_MULTISINK_FEATURE_SUPPORT             ((GmasUggFeatures)0x04)

typedef uint8 GmasUgtFeatures;
#define GMAS_UGT_SOURCE_FEATURE_SUPPORT                ((GmasUgtFeatures)0x01)
#define GMAS_UGT_80KBPS_SOURCE_FEATURE_SUPPORT         ((GmasUgtFeatures)0x02)
#define GMAS_UGT_SINK_FEATURE_SUPPORT                  ((GmasUgtFeatures)0x04)
#define GMAS_UGT_64KBPS_SINK_FEATURE_SUPPORT           ((GmasUgtFeatures)0x08)
#define GMAS_UGT_MULTIPLEX_FEATURE_SUPPORT             ((GmasUgtFeatures)0x10)
#define GMAS_UGT_MULTISINK_FEATURE_SUPPORT             ((GmasUgtFeatures)0x20)
#define GMAS_UGT_MULTISOURCE_FEATURE_SUPPORT           ((GmasUgtFeatures)0x40)

typedef uint8 GmasBgsFeatures;
#define GMAS_BGS_96KBPS_FEATURE_SUPPORT                ((GmasBgsFeatures)0x01)

typedef uint8 GmasBgrFeatures;
#define GMAS_BGR_MULTISINK_FEATURE_SUPPORT             ((GmasBgrFeatures)0x01)
#define GMAS_BGR_MULTIFRAME_FEATURE_SUPPORT            ((GmasBgrFeatures)0x02)


/*! @brief Definition of data required for the initialisation
 *         of the GMAS Server Library.
 */
typedef struct
{
    GmasRole             role;
#if defined(ENABLE_GMAP_UGG_BGS)
    GmasUggFeatures      uggFeatures;
    GmasBgsFeatures      bgsFeatures;
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
    GmasUgtFeatures      ugtFeatures;
    GmasBgrFeatures      bgrFeatures;
#endif
} GattGmasInitData;

/*! @brief Instantiate the GATT GMAS Server Service Library.

    The GATT Service Init function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param appTask The client task that will receive messages from this Service.
    @param startHandle The first handle in the ATT database for this Service instance.
    @param endHandle The last handle in the ATT database for this Service instance.
    @param initData The initialisation data to set

    @return ServiceHandle If the service handle returned is 0, this indicates a failure
                             during GATT Service initialisation.
*/
ServiceHandle GattGmasServerInit(AppTask appTask,
                                 uint16  startHandle,
                                 uint16  endHandle,
                                 GattGmasInitData* initData);

/*!
    \brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    \param srvcHndl Service handle of the GMAS server instance
    \param cid The Connection ID to the peer device.

    \return status_t status of the Add Configuration operation.
*/
status_t GattGmasServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t cid);

/*!
    \brief Remove the configuration for a peer device, identified by its
           Connection ID.

    \param srvcHndl Service handle of the GMAS server instance.
    \param cid  Connection ID to the peer device.

    \return bool boolean status of the Remove Configuration operation.
*/
bool GattGmasServerRemoveConfig(ServiceHandle srvcHndl,
                                connection_id_t  cid);

/*!
    @brief This API is used to set role, unicast and broadcast values.

    @param srvcHndl Service handle of the GMAS server instance.
    @role role value that needs to be set
    @param unicastFeatures Based on the role(i.e. UGG/UGT) corresponding unicast features will be set
    @param broadcastFeatures Based on the role(i.e. BGS/BGR) corresponding broadcast features will be set

    @return TRUE if successful, FALSE otherwise

*/
bool GattGmasServerSetRoleFeature(ServiceHandle srvcHndl, GmasRole role, uint8 unicastFeatures, uint8 broadcastFeatures);

/*!
    @brief This API is used to get the Unicast features for given unicast role.

    @param srvcHndl Service handle of the GMAS server instance.
    @unicastRole Unicast role(i.e. either UGT or UGG) for which features needs to be provided.

    @return uint8 Valid unicast features on success else 0
*/
uint8 GattGmasServerGetUnicastFeatures(ServiceHandle srvcHndl, GmasRole unicastRole);

/*!
    @brief This API is used to get the Broadcast features for given broadcast role.

    @param srvcHndl Service handle of the GMAS server instance.
    @broadcastRole Broadcast role(i.e. either BGS or BGR) for which features needs to be provided.

    @return uint8 Valid broadcast features on success else 0
*/
uint8 GattGmasServerGetBroadcastFeatures(ServiceHandle srvcHndl, GmasRole broadcastRole);
#endif /* GATT_GMAS_SERVER_H_ */
