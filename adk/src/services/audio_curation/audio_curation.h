/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       audio_curation.h
    \defgroup   audio_curation Audio Curation
    @{
        \ingroup    services
        \brief      A component responsible for controlling audio curation services.

        Responsibilities:
        - Handles audio curation use cases e.g ANC/Transparency etc.
        - INCLUDE_ANC_V2 - Based on ANC IP Gen2
*/

#ifndef AUDIO_CURATION_H_
#define AUDIO_CURATION_H_

#include "domain_message.h"

#include "audio_curation_v2.h"

/*! \brief Initialise the audio curation service

    \param init_task Unused
 */
bool AudioCuration_Init(Task init_task);

#endif /* AUDIO_CURATION_H_ */

/*! @} */