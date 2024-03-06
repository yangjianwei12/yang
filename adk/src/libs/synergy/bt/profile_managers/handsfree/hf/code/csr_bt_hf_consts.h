#ifndef CSR_BT_HF_CONSTS_H__
#define CSR_BT_HF_CONSTS_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #3 $
******************************************************************************/


#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Internal defines which may NOT be altered */
#define HF_CLASS_OF_DEVICE                      (CSR_BT_HANDS_FREE_PROFILE)
#define HS_CLASS_OF_DEVICE                      (CSR_BT_HEADSET_PROFILE)
#define HF_SCO_UNUSED                           (0xFFFF)
#define HF_TIMER_UNUSED                         (0xFFFF)
#define HF_HFG_DEFAULT_SUPPORTED_FEATURES       (CSR_BT_HFG_SUPPORT_THREE_WAY_CALLING | CSR_BT_HFG_SUPPORT_INBAND_RINGING)
#define HF_HFG_DEFAULT_NETWORK_FEATURES         (0x00)

#define HF_NUMBER_OF_UNREGISTER                 (3)
#define MAX_NUMBER_OF_SDC_REQUEST_RETRIES       (10)

#define HF_NUM_SERVERS                          (2)
#define HF_NUM_RECORDS                          (2)                              /* Number of service records */
#define HF_NUM_CHANNELS                         (HF_NUM_SERVERS*HF_NUM_RECORDS)  /* Number of server channels (shared between records) */
#define HF_IDX_HF                               (0)                              /* Index for HF server channels */
#define HF_IDX_HS                               (1)                              /* Index for HS server channels */

/* SDP search attributes for internal maintenance */
#define NETWORK_ATTRIBUTE                       0x01
#define SUPPORTED_FEATURES_ATTRIBUTE            0x02
#define SERVICE_NAME_ATTRIBUTE                  0x03

#define CLIP_COMMAND                            "AT+CLIP=?\r"
#define CLIP_VALUE_INDEX                        (8)
#define SPEAKER_GAIN                            "AT+VGS=??\r"
#define SPEAKER_GAIN_VALUE_INDEX                (7)
#define MAX_SPEAKER_GAIN                        (15)
#define MICROPHONE_GAIN                         "AT+VGM=??\r"
#define MICROPHONE_GAIN_VALUE_INDEX             (7)
#define MAX_MICROPHONE_GAIN                     (15)
#define ERROR                                   "ERROR\r"
#define ANSWER                                  "ATA\r"
#define CKPD200                                 "AT+CKPD=200\r"
#define CKPD200_LENGTH                          (12)
#define CKPD                                    "AT+CKPD\r"
#define REJECT                                  "AT+CHUP\r"
#define CR_NEWLINE                              "\r"
#define CR_NEWLINE_LEN                          (1)
#define SUPPORT_FEATURES                        "AT+BRSF="
#define SUPPORT_FEATURES_LENGTH                 (8)
#define CODEC_SUPPORT                           "AT+BAC="
#define CODEC_SUPPORT_LENGTH                    (7)
#define CODEC_SUPPORT_FIRST_XTRA_CODEC_INDEX    (8) /* Index where the first extra codec supported shall be added */
#define CODEC_SUPPORT_LENGTH_PER_XTRA_CODEC     (2)  /* For each extra codec, we need to add a ',' and the new codec ID */
#define CIND_SUPPORT                            "AT+CIND=?\r"
#define CIND_SUPPORT_LENGTH                     (10)
#define CIND_STATUS                             "AT+CIND?\r"
#define CIND_STATUS_LENGTH                      (9)
#define RESET_CMER                              "AT+CMER=3,0,0,0\r"
#define SET_CMER                                "AT+CMER=3,0,0,1\r"
#define SET_CMER_LENGTH                         (16)
#define CALL_HOLD_STATUS                        "AT+CHLD=?\r"
#define CALL_HOLD_STATUS_LENGTH                 (10)
#define CHLD_COMMAND                            "AT+CHLD=" /* Example: AT+CHLD=2x,10\r */
#define CHLD_COMMAND_LENGTH                     (14)
#define CHLD_VALUE_INDEX                        (8)
#define BTRH_COMMAND                            "AT+BTRH=" /* Example: AT+BTRH=1\r */
#define BTRH_COMMAND_LENGTH                     (11)
#define BTRH_VALUE_INDEX                        (8)

#define COPS_QUERY                              "AT+COPS?\r"
#define COPS_QUERY_LENGTH                       (9)

#define COPS_SET_FORMAT                         "AT+COPS=X,X\r"
#define COPS_SET_MODE_INDEX                     (8)
#define COPS_SET_FORMAT_INDEX                   (10)

#define COPS_QUERY_RESPONSE                     "AT+COPS=X,X,"
#define COPS_QUERY_RESPONSE_LENGTH              (10)

#define SUBSCRIBER_QUERY                        "AT+CNUM\r"
#define CALL_LIST_QUERY                         "AT+CLCC\r"
#define CALL_WAITING_CMD                        "AT+CCWA=?\r"
#define CCWA_VALUE_INDEX                        (8)
#define EXTENDED_ERROR                          "AT+CMEE=?\r"
#define EXTENDED_ERROR_INDEX                    (8)
#define ECHO_NOISE_REDUCTION_CMD                "AT+NREC=?\r"
#define NREC_VALUE_INDEX                        (8)
#define VOICE_RECOGNITION_CMD                   "AT+BVRA=?\r"
#define BVRA_VALUE_INDEX                        (8)
#define BT_INPUT_CMD                            "AT+BINP=?\r"
#define BINP_VALUE_INDEX                        (8)
#define DTMF_SEND_CMD                           "AT+VTS=?\r"
#define DTMF_VALUE_INDEX                        (7)
#define DIAL_CMD                                "ATD"
#define DIAL_CMD_INDEX                          (3)
#define DIAL_MEM_CMD                            "ATD>"
#define DIAL_MEM_CMD_INDEX                      (4)
#define REDIAL_CMD                              "AT+BLDN\r"
#define BIA_CMD                                 "AT+BIA="
#define BIA_CMD_INDEX                           (7)
#define BCS_CMD                                 "AT+BCS="
#define BCS_CMD_LENGTH                          (7)
#define BCC_CMD                                 "AT+BCC\r"
#define BCC_CMD_LENGTH                          (7)
#define BIND_SET_CMD                            "AT+BIND="
#define BIND_SET_CMD_LENGTH                     (8)
#define BIND_TEST_CMD                           "AT+BIND=?\r"
#define BIND_TEST_CMD_LENGTH                    (10)
#define BIND_READ_CMD                           "AT+BIND?\r"
#define BIND_READ_CMD_LENGTH                    (9)
#define BIEV_SET_CMD                            "AT+BIEV="
#define BIEV_SET_CMD_LENGTH                     (8)
#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
#define QAC_CMD                                 "AT+%QAC="
#define QAC_CMD_LENGTH                          (8)
#define QCS_CMD                                 "AT+%QCS=X"
#define QCS_CMD_INDEX                           (8)
#define QCS_CMD_LENGTH                          (9)
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */
#define CSR_BT_HF_AUDIO_ON                      (TRUE)
#define CSR_BT_HF_AUDIO_OFF                     (FALSE)

#define PCM_SLOT_UNKNOWN                        (0)

#ifdef __cplusplus
}
#endif

#endif
