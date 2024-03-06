/*!
\copyright  Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for the HFP component.
*/

#ifndef AGHFP_PROFILE_CONFIG_H_
#define AGHFP_PROFILE_CONFIG_H_

#define AGHFP_SCO_RECONNECT_ATTEMPT_LIMIT 3

#ifdef USE_SYNERGY
/*! Max no of allowed AG HF connections */
#define AGHFP_MAX_HF_CONNECTION             (2)

#define QCE_CODEC_MODE_ID_MASK (CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3 | CSR_BT_HF_QCE_CODEC_MASK_64_2_EV3_QHS3 | CSR_BT_HF_QCE_CODEC_MASK_128_QHS3 | CSR_BT_HF_QCE_CODEC_MASK_64_QHS3 )


/*! AGHFP local supported feature  */
#define AGHFP_LOCAL_SUPPORTED_FEATURES          (CSR_BT_HFG_SUPPORT_ABILITY_TO_REJECT_CALL |    \
                                                    CSR_BT_HFG_SUPPORT_ESCO_S4_T2_SETTINGS |    \
                                                    CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION |      \
                                                    CSR_BT_HFG_SUPPORT_ENHANCED_CALL_STATUS)

/*! PCM channels to use*/
#define PCM_SLOT                            CSR_BT_PCM_DONT_CARE
#define PCM_SLOT_REALLOC                    TRUE


#define AGHFP_CALL_CONFIG                   (0)

#define AGHFG_PROFILE_SETUP                 (CSR_BT_HFG_CNF_AUDIO_STATUS)

#define AGHFP_SUPPORTED_HF_INDICATORS       ()

#endif /* USE_SYNERGY */

#endif /* HFP_PROFILE_CONFIG_H_ */
