/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief   Header file for the Unicast manager Peer Signal.
    @{
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#ifdef INCLUDE_MIRRORING

#ifndef LE_UNICAST_MANAGER_PEER_SIGNAL_H_
#define LE_UNICAST_MANAGER_PEER_SIGNAL_H_

#include "mirror_profile_typedef.h"
#include "multidevice.h"

/*! \brief Fills up the Qos and Codec information which are sent from Primary to Secondary.

    \return True if the information is filled successfully
            False if the information is not filled successfully.
 */
bool LeUnicastManagerPeerSignal_FillCodecAndQoSCfg(multidevice_side_t side, mirror_profile_lea_unicast_audio_conf_req_t *conf_data);

#endif /* LE_UNICAST_MANAGER_PEER_SIGNAL_H_ */

#endif /* INCLUDE_MIRRORING */
#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) */
/*! @} */