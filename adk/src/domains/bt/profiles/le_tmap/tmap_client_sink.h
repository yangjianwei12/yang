/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup tmap_profile
    \brief      Header file for TMAP Client sink
    @{ 
*/

#ifndef TMAP_CLIENT_SINK_H_
#define TMAP_CLIENT_SINK_H_

#if defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST)
#ifdef USE_SYNERGY
#include "bt_types.h"

/*! \brief Check if the given TMAP role supports unicast media sender role */
#define TmapClientSink_IsRoleSupportsUnicastMediaReceiver(role) (role & TMAP_ROLE_UNICAST_MEDIA_RECEIVER)
/*! \brief Check if the given TMAP role supports unicast call terminal role */
#define TmapClientSink_IsRoleSupportsCallTerminal(role)         (role & TMAP_ROLE_CALL_TERMINAL)

/*! \brief Initialise the TMAP client component
 */
bool TmapClientSink_Init(Task init_task);

/*! \brief Register as Persistant Device user */
void TmapClientSink_RegisterAsPersistentDeviceDataUser(void);

/*! \brief Read the Tmap role characteristics
    \param[in] cid    GATT Connection id to which the Tmap role is to be read
*/
void TmapClientSink_ReadTmapRole(gatt_cid_t cid);

/*! \brief Used to check if TMAP is connected or not

    \return TRUE if TMAP is connected, FALSE otherwise
*/
bool TmapClientSink_IsTmapConnected(void);

#else /* USE_SYNERGY */
#define TmapClientSink_Init()
#endif /* USE_SYNERGY */
#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) || defined(INCLUDE_LE_AUDIO_BROADCAST) */
#endif /* TMAP_CLIENT_SINK_H_ */
/*! @} */