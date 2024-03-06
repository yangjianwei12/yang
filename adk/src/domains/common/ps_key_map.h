/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       ps_key_map.h
\defgroup   ps_key_map PS Key Map
\ingroup    common_domain
\brief      Map of user PS Keys.
 
Add a PS Key here if it is going to be used.
If there is possibility that a PS Key would need to be populated during deployment
then it should also be added to subsys7_psflash.htf.
 
*/

#ifndef PS_KEY_MAP_H_
#define PS_KEY_MAP_H_

#include <ps.h>

/*! @{ */

/*!  \brief Macro that applies offset of 100 to user PS Keys 50 to 99.
    
    The second group of user PS Key is not located
    immediately after the first group, hence the offset.
*/
#define PS_MAPPED_USER_KEY(key) (((key)<50)?(key):100+(key))

typedef enum
{
    /*! \brief Reserved PS Key, don't use*/
    PS_KEY_RESERVED = PS_MAPPED_USER_KEY(0),

    /*! \brief HFP volumes
       HFP won't use this key anymore,
       but don't use it for other purposes to not affect upgrade
       from the previous ADKs.
    */
    PS_KEY_HFP_CONFIG = PS_MAPPED_USER_KEY(1),

    /*! \brief Fixed role setting */
    PS_KEY_FIXED_ROLE = PS_MAPPED_USER_KEY(2),

    /*! \brief Device Test Service enable */
    PS_KEY_DTS_ENABLE = PS_MAPPED_USER_KEY(3),

    /*! \brief Reboot action PS Key */
    PS_KEY_REBOOT_ACTION = PS_MAPPED_USER_KEY(4),

    /*! \brief legacy Fast Pair Model Id, this has now been moved to read only keys, 
        this legacy key remains for upgrading devices already using it in the field */
    PS_KEY_LEGACY_FAST_PAIR_MODEL_ID = PS_MAPPED_USER_KEY(5),

    /*! \brief legacy Fast Pair Scrambled ASPK, this has now been moved to read only keys 
        (and stored unscrambled), this legacy scrambled key remains for upgrading devices 
        already using it in the field */
    PS_KEY_LEGACY_FAST_PAIR_SCRAMBLED_ASPK = PS_MAPPED_USER_KEY(6),

    /*! \brief Upgrade PS Key */
    PS_KEY_UPGRADE = PS_MAPPED_USER_KEY(7),

    /*! \brief legacy GAA Model Id, this has now been moved to read only keys, 
        this legacy key remains for upgrading devices already using it in the field */
    PS_KEY_LEGACY_GAA_MODEL_ID = PS_MAPPED_USER_KEY(8),

    /*! \brief User EQ selected bank and gains */
    PS_KEY_USER_EQ = PS_MAPPED_USER_KEY(9),

    /*! \brief legacy GAA OTA control, this has now been moved to read only keys, 
        this legacy key remains for upgrading devices already using it in the field */
    PS_KEY_LEGACY_GAA_OTA_CONTROL = PS_MAPPED_USER_KEY(10),

    PS_KEY_BATTERY_STATE_OF_CHARGE = PS_MAPPED_USER_KEY(11),

    /*! \brief ANC session data */
    PS_KEY_ANC_SESSION_DATA = PS_MAPPED_USER_KEY(12),

    /*! \brief PS Key used to store ANC delta gain (in DB) between
     * ANC golden gain configuration and calibrated gain during
     * production test: FFA, FFB and FB
     */
    PS_KEY_ANC_FINE_GAIN_TUNE_KEY = PS_MAPPED_USER_KEY(13),

    PS_KEY_EARBUD_DEVICES_BACKUP = PS_MAPPED_USER_KEY(14),

    /*! \brief PS Key used for various charger comms controls.
     * This key can be extended for any debug, test or general
     * configurations are exposed.
     * Current format is:
     * Index [0]: If set, will enable debug-over-charger-comms and
     *            disable entering dormant mode when in the charging
     *            case. This is only applicable to SchemeB.
     */
    PS_KEY_CHARGER_COMMS_CONTROL = PS_MAPPED_USER_KEY(15),

    /*! \brief Reserved for gaming headset addon
     * This PS key is used by an Addon for a gaming headset
     */
    PS_KEY_GAMING_HEADSET_ADDON = PS_MAPPED_USER_KEY(16),

    /*! \brief PS Keys used to store usb dongle mode */
    PS_KEY_USB_DONGLE_MODE = PS_MAPPED_USER_KEY(17),

    /*! \brief PS Keys used to store per device data */
    PS_KEY_DEVICE_PS_KEY_FIRST = PS_MAPPED_USER_KEY(20),
    PS_KEY_DEVICE_PS_KEY_LAST = PS_MAPPED_USER_KEY(29),

    /*! \brief Version of PS Key layout */
    PS_KEY_DATA_VERSION = PS_MAPPED_USER_KEY(50),

    /*! \brief Setting for testing AV Codec in test mode */
    PS_KEY_TEST_AV_CODEC = PS_MAPPED_USER_KEY(80),

    /*! \brief Setting for testing HFP Codec in test mode */
    PS_KEY_TEST_HFP_CODEC = PS_MAPPED_USER_KEY(81),

    /*! \brief PS Keys used to store Sirk Key for CSIP */
    PS_KEY_CSIP_SIRK_KEY = PS_MAPPED_USER_KEY(82),

    /*! \brief PS Keys used to store broadcast source name */
    PS_KEY_BROADCAST_SRC_NAME = PS_MAPPED_USER_KEY(83),

    /*! \brief PS Keys used to store broadcast source encryption code */
    PS_KEY_BROADCAST_SRC_CODE = PS_MAPPED_USER_KEY(84),

    /*! \brief PS Keys used to store broadcast source advertising settings */
    PS_KEY_BROADCAST_SRC_ADV_CONFIG = PS_MAPPED_USER_KEY(85),

    /*! \brief PS Keys used to store broadcast source audio config settings */
    PS_KEY_BROADCAST_SRC_AUDIO_CONFIG = PS_MAPPED_USER_KEY(86),

    /*! \brief PS Keys used to store broadcast ID */
    PS_KEY_BROADCAST_ID = PS_MAPPED_USER_KEY(87),

    /*! \brief PS Keys used to store per device data */
    PS_KEY_FINDMY_PS_KEY_FIRST = PS_MAPPED_USER_KEY(200),
    PS_KEY_FINDMY_PS_KEY_LAST = PS_MAPPED_USER_KEY(203),
    /*! \brief PS Key used to store the preserved role of an EB */
    PS_KEY_EARBUD_PRESERVED_ROLE = PS_MAPPED_USER_KEY(204),

} ps_key_map_t;


/*! \brief Application read-only keys, provide storage for critical or sensitive application data
    
    Total of 10 keys, these can only be accessed through PsFullRetrieve(..) using 
    the PS Key indexes PSKEY_APPLICATION_READONLY0 - PSKEY_APPLICATION_READONLY9, and not through the 
    apps subsytem PS key indexing used by PsRetrieve(..)
*/
typedef enum
{
    PS_KEY_READ_ONLY_FAST_PAIR_MODEL_ID,
    PS_KEY_READ_ONLY_FAST_PAIR_ASPK,
    PS_KEY_READ_ONLY_GAA_MODEL_ID,
    PS_KEY_READ_ONLY_UNUSED_3,
    PS_KEY_READ_ONLY_UNUSED_4,
    PS_KEY_READ_ONLY_UNUSED_5,
    PS_KEY_READ_ONLY_UNUSED_6,
    PS_KEY_READ_ONLY_UNUSED_7,
    PS_KEY_READ_ONLY_UNUSED_8,
    PS_KEY_READ_ONLY_UNUSED_9,
} ps_read_only_key_t;

#define PsRetrieveReadOnlyKey(ps_read_only_key, buff, words)    PsFullRetrieve((PSKEY_APPLICATION_READONLY0 + ps_read_only_key), buff, words)

/*! @} */

/* allow pre-processor to check if this PS key has been defined */
#define _PS_KEY_GAMING_HEADSET_ADDON PS_KEY_GAMING_HEADSET_ADDON

/* Bitfields used for PS_KEY_TEST_HFP_CODEC */
#define HFP_CODEC_PS_BIT_NB             (1<<0)
#define HFP_CODEC_PS_BIT_WB             (1<<1)
#define HFP_CODEC_PS_BIT_SWB            (1<<2)

#endif /* PS_KEY_MAP_H_ */
