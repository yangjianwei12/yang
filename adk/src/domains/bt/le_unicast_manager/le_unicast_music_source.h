/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief
    @{
*/

#ifndef LE_UNICAST_MUSIC_SOURCE_H_
#define LE_UNICAST_MUSIC_SOURCE_H_

#include "le_unicast_manager_instance.h"


void LeUnicastMusicSource_Init(void);

/*! \brief Reconfigure the LE Audio kymera chain.

    \param inst  The unicast manager instance to reconfigure the audio chain for.
*/
void LeUnicastMusicSource_Reconfig(le_um_instance_t *inst);

#endif /* LE_UNICAST_MUSIC_SOURCE_H_ */
/*! @} */