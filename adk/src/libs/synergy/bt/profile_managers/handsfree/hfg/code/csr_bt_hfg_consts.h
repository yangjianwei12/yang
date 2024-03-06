#ifndef CSR_BT_HFG_CONSTS_H__
#define CSR_BT_HFG_CONSTS_H__
/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/


#include "csr_synergy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Profile fundemental definitions */
#define CSR_BT_HFG_CLASS_OF_DEVICE                     (0x00000000)                     /* The COD for the HFG is not defined in the profile - set to 0 */
#define CSR_BT_HFG_NO_CONID                            0xFF                             /* No 'active' connection index */
#define CSR_BT_HF_DEFAULT_SUPPORTED_FEATURES           (0)                              /* Supported features bitmask for HF */
#define CSR_BT_HFG_SERVICE_NAME                        "Voice gateway"                  /* Default SDS service name for HFG */
#define CSR_BT_HFG_CONID_NONE                          (0xFFFFFFFEU)                    /* connection ID: none */

/* Server channels and service records */
#define CSR_BT_HFG_NUM_SERVERS                         (2)                              /* Hard limit on number of connections */
#define CSR_BT_HFG_NUM_RECORDS                         (2)                              /* Number of service records */
#define CSR_BT_HFG_NUM_CHANNELS                        (CSR_BT_HFG_NUM_SERVERS*CSR_BT_HFG_NUM_RECORDS)/* Number of server channels (shared between records) */
#define CSR_BT_HFG_IDX_AG                              (0)                              /* Index for AG server channels */
#define CSR_BT_HFG_IDX_HFG                             (1)                              /* Index for HFG server channels */

/* AT responses */
#define CSR_BT_HFG_STR_OK                              ((CsrUint8*)"OK")
#define CSR_BT_HFG_STR_RING                            ((CsrUint8*)"RING")
#define CSR_BT_HFG_STR_ERROR                           ((CsrUint8*)"ERROR")
#define CSR_BT_HFG_STR_HF_SPEAKER_GAIN                 ((CsrUint8*)"+VGS: ")
#define CSR_BT_HFG_STR_HS_SPEAKER_GAIN                 ((CsrUint8*)"+VGS=")
#define CSR_BT_HFG_STR_HF_MIC_GAIN                     ((CsrUint8*)"+VGM: ")
#define CSR_BT_HFG_STR_HS_MIC_GAIN                     ((CsrUint8*)"+VGM=")
#define CSR_BT_HFG_STR_BRSF                            ((CsrUint8*)"+BRSF: ")
#define CSR_BT_HFG_STR_CLIP                            ((CsrUint8*)"+CLIP: ")
#define CSR_BT_HFG_STR_COPS                            ((CsrUint8*)"+COPS: ")
#define CSR_BT_HFG_STR_CIEV                            ((CsrUint8*)"+CIEV: ")
#define CSR_BT_HFG_STR_CHLD                            ((CsrUint8*)"+CHLD: ")
#define CSR_BT_HFG_STR_BTRH                            ((CsrUint8*)"+BTRH: ")
#define CSR_BT_HFG_STR_BSIR                            ((CsrUint8*)"+BSIR: ")
#define CSR_BT_HFG_STR_BINP                            ((CsrUint8*)"+BINP: ")
#define CSR_BT_HFG_STR_CLCC                            ((CsrUint8*)"+CLCC: ")
#define CSR_BT_HFG_STR_CNUM                            ((CsrUint8*)"+CNUM: ")
#define CSR_BT_HFG_STR_CCWA                            ((CsrUint8*)"+CCWA: ")
#define CSR_BT_HFG_STR_CMEE_ERROR                      ((CsrUint8*)"+CME ERROR: ")
#define CSR_BT_HFG_STR_BVRA                            ((CsrUint8*)"+BVRA: ")
#define CSR_BT_HFG_STR_CIND                            ((CsrUint8*)"+CIND: ")
#define CSR_BT_HFG_STR_CIND_SUPPORT_1                  ((CsrUint8*)"(\"service\",(0-1)),(\"call\",(0-1))," \
                                                           "(\"callsetup\",(0-3)),")
#define CSR_BT_HFG_STR_CIND_SUPPORT_2                  ((CsrUint8*)"(\"callheld\",(0-2)),(\"signal\",(0-5))," \
                                                           "(\"roam\",(0-1)),(\"battchg\",(0-5))")

#define CSR_BT_HFG_STR_BCS                             ((CsrUint8*)"+BCS: ")
#define CSR_BT_HFG_STR_BIND                            ((CsrUint8*)"+BIND: ")

/* SDP search attributes for internal maintenance */
#define NETWORK_ATTRIBUTE                       0x01
#define SUPPORTED_FEATURES_ATTRIBUTE            0x02
#define SERVICE_NAME_ATTRIBUTE                  0x03

/* Misc settings */
#define CSR_BT_HFG_MAX_VGS                             (15)                             /* Max gain for speaker */
#define CSR_BT_HFG_MAX_VGM                             (15)                             /* Max gain for microphone */
#define CSR_BT_HFG_MISSING_SLC_AT_TIMER                (1000000)                        /* Timeout for missing AT+CHLD or AT+BIND in AT sequence */
#define CSR_BT_HFG_MISSING_CKPD_TIMER                  (1000000)                        /* Timeout for missing CKPD in AT sequence */
#define HFG_AT_RESPONSE_TIMEOUT                        (2 * CSR_SCHED_SECOND)           /* Time to wait for answer from HF after sending AT cmd or unsolicited response */

/* Non-indicator dynamic settings (ind.other) */
#define CSR_BT_HFG_SET_SPEAKER_VOL                     (0)                              /* Speaker volume */
#define CSR_BT_HFG_SET_MIC_VOL                         (1)                              /* Microphone volume */
#define CSR_BT_HFG_SET_CMEE                            (2)                              /* CMEE extended error codes enabled */
#define CSR_BT_HFG_SET_CLIP                            (3)                              /* CLIP notifications enabled */
#define CSR_BT_HFG_SET_CMER_CIEV                       (4)                              /* CIEV enabled by means of CMER <ind> */
#define CSR_BT_HFG_SET_CCWA                            (5)                              /* CCWA notifications enabled */
#define CSR_BT_HFG_INBAND_RINGING                      (6)                              /* In-band ringing enabled */
#define CSR_BT_HFG_NUM_OF_SETTINGS                    (7)

/* Audio definitions */
#define CSR_BT_HFG_SAMPLE_RATE_8KHZ                    ((CsrUint32) 8000)
#define CSR_BT_HFG_SAMPLE_RATE_16KHZ                   ((CsrUint32) 16000)

#define CSR_BT_HFG_WBS_AUDIO                     ((CsrUint16) 0x0000)
#define CSR_BT_HFG_CVSD_AUDIO                    ((CsrUint16) 0x0001)

#define CSR_BT_HFG_PCM_SYNC_RATE                 ((CsrUint16) 0x0100)
#define CSR_BT_HFG_PCM_MASTER_CLOCK_RATE         ((CsrUint16) 0x0101)
#define CSR_BT_HFG_PCM_MASTER_MODE               ((CsrUint16) 0x0102)
#define CSR_BT_HFG_PCM_SLOT_COUNT                ((CsrUint16) 0x0103)
#define CSR_BT_HFG_I2S_SYNC_RATE                 ((CsrUint16) 0x0200)
#define CSR_BT_HFG_I2S_MASTER_CLOCK_RATE         ((CsrUint16) 0x0201)
#define CSR_BT_HFG_I2S_MASTER_MODE               ((CsrUint16) 0x0202)
#define CSR_BT_HFG_CODEC_ADC_RATE                ((CsrUint16) 0x0300)
#define CSR_BT_HFG_CODEC_DAC_RATE                ((CsrUint16) 0x0301)
#define CSR_BT_HFG_CODEC_ADC_GAIN                ((CsrUint16) 0x0302)
#define CSR_BT_HFG_CODEC_DAC_GAIN                ((CsrUint16) 0x0303)

#define CSR_BT_HFG_NO_CONN_ID                    ((CsrUint16) 0xFFFF)

#ifdef __cplusplus
}
#endif

#endif
