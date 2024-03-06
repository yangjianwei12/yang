#ifndef HFP_ABSTRACTION_H
#define HFP_ABSTRACTION_H
#include "hfp.h"

#ifdef USE_SYNERGY
#include "hf_lib.h"
#define HF_PROFILE_WBS_MSBC_CODEC CSR_BT_WBS_MSBC_CODEC
#ifdef INCLUDE_SWB_LC3
#define HF_PROFILE_WBS_LC3SWB_CODEC CSR_BT_WBS_LC3SWB_CODEC
#endif
#define HF_PROFILE_HF_BATTERY_LEVEL CSR_BT_HFP_BATTERY_LEVEL_HF_IND
#define HF_PROFILE_HF_INDICATORS_INVALID 0x00
#else //USE_SYNERGY
/*Mapping to CAA types*/
#define HF_PROFILE_WBS_CODEC_MASK hfp_wbs_codec_mask
#define HF_PROFILE_WBS_MSBC_CODEC hfp_wbs_codec_mask_msbc
#define HF_PROFILE_HF_BATTERY_LEVEL hf_battery_level
#define HF_PROFILE_HF_INDICATORS_INVALID hf_indicators_invalid
#define HF_PROFILE_HFP_PRIMARY_LINK hfp_primary_link
#define HF_PROFILE_HFP_INVALID_LINK hfp_invalid_link
#define hfp_link_priority_t hfp_link_priority
#endif //USE_SYNERGY

#define hfp_call_state_t hfp_call_state
#define hfp_connection_type_t hfp_profile

#endif // HFP_ABSTRACTION_H
