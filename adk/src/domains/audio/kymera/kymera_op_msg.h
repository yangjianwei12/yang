/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera private header with operator message related definitions
*/

#ifndef KYMERA_OP_MSG_H_
#define KYMERA_OP_MSG_H_

/*! Kymera operator messages are 3 words long, with the ID in the 2nd word */
#define KYMERA_OP_MSG_LEN                   (3)
#define KYMERA_OP_MSG_WORD_MSG_ID           (1)
#define KYMERA_OP_MSG_WORD_EVENT_ID         (3)
#define KYMERA_OP_MSG_WORD_PAYLOAD_0        (4)

#define KYMERA_OP_MSG_WORD_PAYLOAD_NA       (0xFFFF)

/*! \brief The kymera operator unsolicited message ids. */
typedef enum
{
    /*! Kymera ringtone generator TONE_END message */
    KYMERA_OP_MSG_ID_TONE_END = 0x0001U,
    /*! AANC Capability  INFO message Kalsim testing only */
    KYMERA_OP_MSG_ID_AANC_INFO = 0x0007U,
    /*! AANC Capability EVENT_TRIGGER message  */
    KYMERA_OP_MSG_ID_AANC_EVENT_TRIGGER = 0x0008U,
    /*! AANC Capability EVENT_CLEAR aka NEGATIVE message  */
    KYMERA_OP_MSG_ID_AANC_EVENT_CLEAR = 0x0009U,
    /*! Earbud Fit Test result message */
    KYMERA_OP_MSG_ID_FIT_TEST = 0x000BU,
    /*! AHM/Wind noise reduction/ATR VAD Capability event message */
    KYMERA_OP_MSG_ID_AHM_EVENT = 0x000DU,
} kymera_op_unsolicited_message_ids_t;

/*! \brief The kymera AANC operator event ids. */
typedef enum
{
    /*! Gain unchanged for 5 seconds when EDs inactive */
    KYMERA_AANC_EVENT_ED_INACTIVE_GAIN_UNCHANGED = 0x0000U,
    /*! Either ED active for more than 5 seconds */
    KYMERA_AANC_EVENT_ED_ACTIVE = 0x0001U,
    /*! Quiet mode  */
    KYMERA_AANC_EVENT_QUIET_MODE = 0x0002U,
    /*! Bad environment updated for x secs above spl threshold and cleared immediately */
    KYMERA_AANC_EVENT_BAD_ENVIRONMENT = 0x0006U,
} kymera_aanc_op_event_ids_t;

#define KYMERA_FIT_TEST_EVENT_ID (0x0U)
#define KYMERA_FIT_TEST_RESULT_BAD (0x0U)
#define KYMERA_FIT_TEST_RESULT_GOOD (0x1U)
#define KYMERA_FIT_TEST_DATA_AVAILABLE (65535U)

/*! \brief The kymera AHM/Wind Detect operator event ids. */
typedef enum
{
    KYMERA_AHM_EVENT_ID_FF_RAMP = 0x0000U,
    KYMERA_AHM_EVENT_ID_FB_RAMP = 0x0001U,
    KYMERA_WIND_DETECT_ID_1MIC = 0x0002U,
    KYMERA_WIND_DETECT_ID_2MIC = 0x0003U,
    KYMERA_ATR_VAD_ID_1MIC = 0x0004U,
    KYMERA_ATR_VAD_ID_2MIC = 0x0005U,
    KYMERA_AHM_EVENT_ID_TRANSITION = 0x0006U,
    KYMERA_NOISE_ID      = 0x0008U
} kymera_ahm_op_event_id_t;

/*! \brief The kymera AHM operator event types. */
typedef enum
{
    KYMERA_AHM_EVENT_TYPE_TRIGGER = 0x0000U,
    KYMERA_AHM_EVENT_TYPE_CLEAR = 0x0001U
} kymera_ahm_op_event_type_t;

/*! \brief The kymera Noise ID category */
typedef enum
{
    KYMERA_NOISE_ID_CATEGORY_0 = 0x0000U,
    KYMERA_NOISE_ID_CATEGORY_1 = 0x0001U
} kymera_noise_id_event_type_t;


#define KYMERA_AHM_EVENT_ID_INDEX   (3)
#define KYMERA_AHM_EVENT_TYPE_INDEX (4)
#define KYMERA_AHM_EVENT_PAYLOAD_A_INDEX (5)
#define KYMERA_AHM_EVENT_PAYLOAD_B_INDEX (6)

#endif /* KYMERA_OP_MSG_H_ */
