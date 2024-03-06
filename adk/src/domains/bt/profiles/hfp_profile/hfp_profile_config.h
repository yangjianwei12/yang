/*!
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for the HFP component.
*/

#ifndef HFP_PROFILE_CONFIG_H_
#define HFP_PROFILE_CONFIG_H_

/*! Number of volume steps to use per HFP UI volume event.
 The full volume range is 0-15 */
#define hfpConfigGetHfpVolumeStep() (1)

/*! The delay after a HFP device property is changed when the
    device state should be serialised */
#define hfpConfigSerialiseDeviceLaterDelay() D_SEC(5)

/*! Default speaker gain */
#define HFP_SPEAKER_GAIN    (10)

/*! Default microphone gain */
#define HFP_MICROPHONE_GAIN (15)

/*! Auto answer call on connect */
#ifndef HFP_CONNECT_AUTO_ANSWER_DISABLE
#define HFP_CONNECT_AUTO_ANSWER
#endif

/*! Disable - Auto transfer call on connect */
#undef HFP_CONNECT_AUTO_TRANSFER

/*! Enable HF battery indicator */
#define appConfigHfpBatteryIndicatorEnabled() (1)

/*! Enable Super-wideband SCO */
#define appConfigScoSwbEnabled() (TRUE)

/*! The time duration to boost the CPU clock after audio connects. The CPU is
    boosted to minimise time from eSCO connection to audio starting. */
#define appConfigAudioConnectedCpuBoostDuration() D_SEC(1)

#endif /* HFP_PROFILE_CONFIG_H_ */
