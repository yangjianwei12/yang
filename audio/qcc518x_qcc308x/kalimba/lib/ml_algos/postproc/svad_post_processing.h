/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef SVAD_POST_PROCESSING_H
#define SVAD_POST_PROCESSING_H

#define VAD_RESULT_COUNT 7

/**
 * \brief Function to process the last n model outputs and interprate if the VAD is detected
 * \param current state
 * \param vad_results
 * \return none
 */
void svad_detect_state_change(int *current_state, int *vad_results);

#endif /* SVAD_POST_PROCESSING_H */
