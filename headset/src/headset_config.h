/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_config.h
\brief      Application configuration file
*/

#ifndef HEADSET_CONFIG_H_
#define HEADSET_CONFIG_H_

/*! The number of battery servers that are initialized for headset application */
#define NUMBER_BATTERY_SERVERS_HEADSET (1)

/*! The number of Trusted Devices supported by Headset */
#ifdef INCLUDE_EXTENDED_TDL
#define appConfigMaxDevicesSupported()  (12)
#else
#define appConfigMaxDevicesSupported()  (7)
#endif

#ifdef INCLUDE_FAST_PAIR
/*! User will need to change this in Project DEFS (default is BOARD_TX_POWER_PATH_LOSS=236)as per the hardware used. */
#define appConfigBoardTxPowerPathLoss        (BOARD_TX_POWER_PATH_LOSS)
#endif

/*! place the audio types in the order you wish the audio_router to prioritise their routing,
    with the highest priority first */
#define AUDIO_TYPE_PRIORITIES {source_type_voice, source_type_audio}

/*! Initialize major and minor upgrade version information*/
#define UPGRADE_INIT_VERSION_MAJOR (1)
#define UPGRADE_INIT_VERSION_MINOR (0)

/*! The factory-set PS config version. After a successful upgrade the values from 
    the upgrade header will be written to the upgrade PS key and used in future.*/
#define UPGRADE_INIT_CONFIG_VERSION (1)

/*! Initialize silent commit supported information*/
#define UPGRADE_SILENT_COMMIT_SUPPORTED (1)

/*! Peer pair timeout for Speaker config */
#define appConfigSpeakerPeerPairTimeout() (10)

/*! Peer find role timeout for Speaker config */
#define appConfigSpeakerPeerFindRoleTimeout() (3)

#ifdef DELETE_PEER_INFO_ON_PEER_PAIRING_TRIGGER
/*! On peer-pair trigger, speaker can remove peer info to allow fresh pair */
#define appConfigSpeakerRemovePeerInfoOnTrigger()   TRUE
#else
#define appConfigSpeakerRemovePeerInfoOnTrigger() FALSE
#endif

#ifdef INCLUDE_WATCHDOG
/*! Watchdog timeout in seconds */
#define appConfigWatchdogTimeout()      (5)
/*! Watchdog kick interval in ms */
#define appConfigWatchdogKickTimeMs()   (2000)
#endif /* INCLUDE_WATCHDOG */

#endif /* HEADSET_CONFIG_H_ */
