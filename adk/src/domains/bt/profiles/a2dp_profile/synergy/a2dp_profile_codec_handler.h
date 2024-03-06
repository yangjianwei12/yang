/****************************************************************************
Copyright (c) 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_codec_handler.h
    
DESCRIPTION
	
*/

#include "a2dp_typedef.h"
#include "av_typedef.h"
#include <a2dp.h>

#ifndef A2DP_CODEC_HANDLER_H_
#define A2DP_CODEC_HANDLER_H_

/****************************************************************************
NAME	
	appA2dpSelectOptimalCodecSettings

DESCRIPTION
	Select the correct capabilities depending on which codec is selected.
		
*/
bool appA2dpSelectOptimalCodecSettings(const avInstanceTaskData *theInst, uint8 *remote_codec_caps);

/****************************************************************************
NAME	
	a2dpSendCodecAudioParams

DESCRIPTION
	Choose configured codec parameterss and send them to the application.
		
*/
a2dp_codec_settings * appA2dpGetCodecAudioParams(const avInstanceTaskData *theInst);

bool isCodecAptxAdaptive(const uint8 *codec_caps);

#endif /* A2DP_CODEC_HANDLER_H_ */
