/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile HFP Profile
\ingroup    profiles
\brief      HFP Profile instances functionality.
*/

#ifndef HFP_PROFILE_INSTANCE_H_
#define HFP_PROFILE_INSTANCE_H_

#include <hfp.h>
#include <task_list.h>

#include "hfp_profile.h"
#include "hfp_profile_typedef.h"
#include "voice_sources.h"

#define HFP_MAX_NUM_INSTANCES 2

#define for_all_hfp_instances(hfp_instance, iterator) for(hfp_instance = HfpInstance_GetFirst(iterator); hfp_instance != NULL; hfp_instance = HfpInstance_GetNext(iterator))

typedef struct
{
    hfpInstanceTaskData* instances[HFP_MAX_NUM_INSTANCES];
    unsigned index;
} hfp_instance_iterator_t;

hfpInstanceTaskData * HfpProfileInstance_Create(const bdaddr *bd_addr, bool allocate_source);

void HfpProfileInstance_Destroy(hfpInstanceTaskData *instance);

device_t HfpProfileInstance_FindDeviceFromInstance(hfpInstanceTaskData* instance);

device_t HfpProfileInstance_FindDeviceFromVoiceSource(voice_source_t source);

hfpInstanceTaskData * HfpInstance_GetFirst(hfp_instance_iterator_t * iterator);

hfpInstanceTaskData * HfpInstance_GetNext(hfp_instance_iterator_t * iterator);

hfpInstanceTaskData* HfpProfileInstance_GetInstanceForDevice(device_t device);

hfpInstanceTaskData * HfpProfileInstance_GetInstanceForBdaddr(const bdaddr *bd_addr);

hfpInstanceTaskData * HfpProfileInstance_GetInstanceForSource(voice_source_t source);

voice_source_t HfpProfileInstance_GetVoiceSourceForInstance(hfpInstanceTaskData * instance);

void HfpProfileInstance_RegisterVoiceSourceInterfaces(voice_source_t voice_source);

void HfpProfileInstance_DeregisterVoiceSourceInterfaces(voice_source_t voice_source);

uint16 * HfpProfileInstance_GetLock(hfpInstanceTaskData *instance);
void HfpProfileInstance_SetLock(hfpInstanceTaskData *instance, uint16 lock);

/*! \brief Get HFP Audio lock */
uint16 * HfpProfileInstance_GetAudioLock(hfpInstanceTaskData* instance);

/*! \brief Set HFP Audio lock */
void HfpProfileInstance_SetAudioLock(hfpInstanceTaskData* instance);

/*! \brief Clear HFP Audio lock */
void HfpProfileInstance_ClearAudioLock(hfpInstanceTaskData* instance);

/*! \brief Is HFP SCO/ACL encrypted */
bool HfpProfileInstance_IsEncrypted(hfpInstanceTaskData *instance);

/*! \brief Resets aptx voice good and error frame counts after hfp call audio
           disconnected*/
void HfpProfileInstance_ResetAptxVoiceFrameCounts(void);

/*! \brief Starts checking aptX voice packets counters without any delay if super wide
           band (swb) call is active. */
void HfpProfileInstance_StartCheckingAptxVoicePacketsCounterImmediatelyIfSwbCallActive(hfpInstanceTaskData * instance);

/*! \brief Disable the aptX Black List timer in the instance */
bool HfpProfileInstance_DisableAptxBlacklistTimer(const bdaddr* handset_bd_addr);

/*! \brief Cancel checking aptX voice packets counters to stop blacklisting */
void HfpProfileInstance_CancelAptxVoicePacketCouter(hfpInstanceTaskData* instance);

/*! \brief Request the creation of an audio connection (SCO) */
void HfpProfileInstance_RequestAudioConnection(hfpInstanceTaskData *instance);

/*! \brief Request the disconnection of any existing audio connection (SCO) */
void HfpProfileInstance_RequestAudioDisconnection(hfpInstanceTaskData *instance);

#endif /* HFP_PROFILE_INSTANCE_H_ */
