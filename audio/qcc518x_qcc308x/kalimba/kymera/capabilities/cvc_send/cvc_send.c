/**
* Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
* \file  cvc_send.c
* \ingroup  capabilities
*
*  CVC send
*
*/

/****************************************************************************
Include Files
*/
#include "capabilities.h"
#include "cvc_send_cap_c.h"
#include "mem_utils/dynloader.h"
#include "mem_utils/exported_constants.h"
#include "mem_utils/exported_constant_files.h"
#include "audio_proc/iir_resamplev2_util.h"
#include "audio_proc/frame_iir_resamplerv2.h"
#include "cvc_processing_c.h"

#ifdef RUNNING_ON_KALSIM
#undef ACCESS_SHARE_MEMORY_THROUGH_PATCH
#endif
#ifdef ACCESS_SHARE_MEMORY_THROUGH_PATCH
#include "dls_vol_ctrl_lib_if.h"
#endif
/****************************************************************************

Local Definitions
*/

/* Reference inputs 0-2 */
#define CVC_SEND_NUM_INPUTS_MASK    0x7
#define CVC_SEND_NUM_OUTPUTS_MASK   0x7

/* CVC send terminals */
#define CVC_SEND_IN_TERMINAL_AECREF 0
#define CVC_SEND_IN_TERMINAL_MIC1   1

#define CVC_SEND_OUT_TERMINAL_VOICE 0
#define CVC_SEND_OUT_TERMINAL_VA    1

/* Voice quality metric error code */
#define CVC_SEND_VOICE_QUALITY_METRIC_ERROR_CODE 0xFF

/* bit field for kicking VA channels */
#define TOUCHED_CVC_VA_SOURCES     0x01E

/****************************************************************************
Private Constant Definitions
*/

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define CVCHS1MIC_SEND_NB_CAP_ID           CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_NB
#define CVCHS1MIC_SEND_WB_CAP_ID           CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_WB
#define CVCHS1MIC_SEND_UWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_UWB
#define CVCHS1MIC_SEND_SWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_SWB
#define CVCHS1MIC_SEND_WB_VA_CAP_ID        CAP_ID_DOWNLOAD_CVCHS1MIC_VA_WB

#define CVCHS1MIC_SEND_FB_CAP_ID           CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_FB
#define CVCHS2MIC_MONO_SEND_NB_CAP_ID      CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_NB
#define CVCHS2MIC_MONO_SEND_WB_CAP_ID      CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_WB
#define CVCHS2MIC_WAKEON_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCHS2MIC_WAKEON_WB
#define CVCHS2MIC_BARGEIN_WB_CAP_ID        CAP_ID_DOWNLOAD_CVCHS2MIC_BARGEIN_WB

#define CVCHS2MIC_MONO_SEND_UWB_CAP_ID     CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_UWB
#define CVCHS2MIC_MONO_SEND_SWB_CAP_ID     CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_SWB
#define CVCHS2MIC_MONO_SEND_FB_CAP_ID      CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_FB
#define CVCHS2MIC_BINAURAL_SEND_NB_CAP_ID  CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_NB
#define CVCHS2MIC_BINAURAL_SEND_WB_CAP_ID  CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_WB
#define CVCHS2MIC_BINAURAL_SEND_UWB_CAP_ID CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_UWB
#define CVCHS2MIC_BINAURAL_SEND_SWB_CAP_ID CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_SWB
#define CVCHS2MIC_BINAURAL_SEND_FB_CAP_ID  CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_FB
#define CVCHF1MIC_SEND_NB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_NB
#define CVCHF1MIC_SEND_WB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_WB
#define CVCHF1MIC_SEND_UWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_UWB
#define CVCHF1MIC_SEND_SWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_SWB
#define CVCHF1MIC_SEND_FB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_FB
#define CVCHF2MIC_SEND_NB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_NB
#define CVCHF2MIC_SEND_WB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_WB
#define CVCHF2MIC_SEND_UWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_UWB
#define CVCHF2MIC_SEND_SWB_CAP_ID          CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_SWB
#define CVCHF2MIC_SEND_FB_CAP_ID           CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_FB
#define CVCSPKR1MIC_SEND_NB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_NB
#define CVCSPKR1MIC_SEND_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_WB
#define CVCSPKR1MIC_SEND_UWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_UWB
#define CVCSPKR1MIC_SEND_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_SWB
#define CVCSPKR1MIC_SEND_FB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_FB
#define CVCSPKR2MIC_SEND_NB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_NB
#define CVCSPKR2MIC_SEND_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_WB
#define CVCSPKR2MIC_SEND_UWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_UWB
#define CVCSPKR2MIC_SEND_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_SWB
#define CVCSPKR2MIC_SEND_FB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_FB
#define CVCSPKR3MIC_SEND_NB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_NB
#define CVCSPKR3MIC_SEND_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_WB
#define CVCSPKR3MIC_SEND_UWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_UWB
#define CVCSPKR3MIC_SEND_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_SWB
#define CVCSPKR3MIC_SEND_FB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_FB
#define CVCSPKR3MIC_CIRC_SEND_NB_CAP_ID    CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_NB
#define CVCSPKR3MIC_CIRC_SEND_WB_CAP_ID    CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_WB
#define CVCSPKR3MIC_CIRC_SEND_UWB_CAP_ID   CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_UWB
#define CVCSPKR3MIC_CIRC_SEND_SWB_CAP_ID   CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_SWB
#define CVCSPKR3MIC_CIRC_SEND_FB_CAP_ID    CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_FB

#define CVCSPKR3MIC_CIRC_VA_SEND_WB_CAP_ID      CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_VA_SEND_WB
#define CVCSPKR3MIC_CIRC_VA4B_SEND_WB_CAP_ID    CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_VA4B_SEND_WB

#define CVCSPKR4MIC_SEND_NB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_NB
#define CVCSPKR4MIC_SEND_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_WB
#define CVCSPKR4MIC_SEND_UWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_UWB
#define CVCSPKR4MIC_SEND_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_SWB
#define CVCSPKR4MIC_SEND_FB_CAP_ID         CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_FB
#define CVCSPKR4MIC_CIRC_SEND_UWB_CAP_ID   CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_UWB
#define CVCSPKR4MIC_CIRC_SEND_SWB_CAP_ID   CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_SWB
#define CVCSPKR4MIC_CIRC_SEND_FB_CAP_ID    CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_FB

#define CVCEB2MIC_IE_NB_CAP_ID             CAP_ID_DOWNLOAD_CVCEB2MIC_IE_NB
#define CVCEB2MIC_IE_WB_CAP_ID             CAP_ID_DOWNLOAD_CVCEB2MIC_IE_WB
#define CVCEB3MIC_MONO_IE_NB_CAP_ID        CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_NB
#define CVCEB3MIC_MONO_IE_WB_CAP_ID        CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_WB
#define CVCEB3MIC_MONO_IE_SWB_CAP_ID       CAP_ID_DOWNLOAD_CVCEB3MIC_MONO_IE_SWB

#define CVCEB3MIC_HYBRID_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCEB3MIC_HYBRID_WB
#define CVCEB3MIC_HYBRID_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCEB3MIC_HYBRID_SWB
#define CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID    CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_HYBRID_WB
#define CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID   CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_HYBRID_SWB
#define CVCHS1MIC_SEND_HYBRID_WB_CAP_ID         CAP_ID_DOWNLOAD_CVCHS1MIC_HYBRID_WB
#define CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID        CAP_ID_DOWNLOAD_CVCHS1MIC_HYBRID_SWB

#else

#define CVCHS1MIC_SEND_NB_CAP_ID           CAP_ID_CVCHS1MIC_SEND_NB
#define CVCHS1MIC_SEND_WB_CAP_ID           CAP_ID_CVCHS1MIC_SEND_WB
#define CVCHS1MIC_SEND_UWB_CAP_ID          CAP_ID_CVCHS1MIC_SEND_UWB
#define CVCHS1MIC_SEND_SWB_CAP_ID          CAP_ID_CVCHS1MIC_SEND_SWB
#define CVCHS1MIC_SEND_FB_CAP_ID           CAP_ID_CVCHS1MIC_SEND_FB
#define CVCHS1MIC_SEND_WB_VA_CAP_ID        CAP_ID_CVCHS1MIC_VA_WB

#define CVCHS2MIC_MONO_SEND_NB_CAP_ID      CAP_ID_CVCHS2MIC_MONO_SEND_NB
#define CVCHS2MIC_MONO_SEND_WB_CAP_ID      CAP_ID_CVCHS2MIC_MONO_SEND_WB
#define CVCHS2MIC_WAKEON_WB_CAP_ID         CAP_ID_CVCHS2MIC_WAKEON_WB
#define CVCHS2MIC_BARGEIN_WB_CAP_ID        CAP_ID_CVCHS2MIC_BARGEIN_WB

#define CVCHS2MIC_MONO_SEND_UWB_CAP_ID     CAP_ID_CVCHS2MIC_MONO_SEND_UWB
#define CVCHS2MIC_MONO_SEND_SWB_CAP_ID     CAP_ID_CVCHS2MIC_MONO_SEND_SWB
#define CVCHS2MIC_MONO_SEND_FB_CAP_ID      CAP_ID_CVCHS2MIC_MONO_SEND_FB
#define CVCHS2MIC_BINAURAL_SEND_NB_CAP_ID  CAP_ID_CVCHS2MIC_BINAURAL_SEND_NB
#define CVCHS2MIC_BINAURAL_SEND_WB_CAP_ID  CAP_ID_CVCHS2MIC_BINAURAL_SEND_WB
#define CVCHS2MIC_BINAURAL_SEND_UWB_CAP_ID CAP_ID_CVCHS2MIC_BINAURAL_SEND_UWB
#define CVCHS2MIC_BINAURAL_SEND_SWB_CAP_ID CAP_ID_CVCHS2MIC_BINAURAL_SEND_SWB
#define CVCHS2MIC_BINAURAL_SEND_FB_CAP_ID  CAP_ID_CVCHS2MIC_BINAURAL_SEND_FB
#define CVCHF1MIC_SEND_NB_CAP_ID           CAP_ID_CVCHF1MIC_SEND_NB
#define CVCHF1MIC_SEND_WB_CAP_ID           CAP_ID_CVCHF1MIC_SEND_WB
#define CVCHF1MIC_SEND_UWB_CAP_ID          CAP_ID_CVCHF1MIC_SEND_UWB
#define CVCHF1MIC_SEND_SWB_CAP_ID          CAP_ID_CVCHF1MIC_SEND_SWB
#define CVCHF1MIC_SEND_FB_CAP_ID           CAP_ID_CVCHF1MIC_SEND_FB
#define CVCHF2MIC_SEND_NB_CAP_ID           CAP_ID_CVCHF2MIC_SEND_NB
#define CVCHF2MIC_SEND_WB_CAP_ID           CAP_ID_CVCHF2MIC_SEND_WB
#define CVCHF2MIC_SEND_UWB_CAP_ID          CAP_ID_CVCHF2MIC_SEND_UWB
#define CVCHF2MIC_SEND_SWB_CAP_ID          CAP_ID_CVCHF2MIC_SEND_SWB
#define CVCHF2MIC_SEND_FB_CAP_ID           CAP_ID_CVCHF2MIC_SEND_FB
#define CVCSPKR1MIC_SEND_NB_CAP_ID         CAP_ID_CVCSPKR1MIC_SEND_NB
#define CVCSPKR1MIC_SEND_WB_CAP_ID         CAP_ID_CVCSPKR1MIC_SEND_WB
#define CVCSPKR1MIC_SEND_UWB_CAP_ID        CAP_ID_CVCSPKR1MIC_SEND_UWB
#define CVCSPKR1MIC_SEND_SWB_CAP_ID        CAP_ID_CVCSPKR1MIC_SEND_SWB
#define CVCSPKR1MIC_SEND_FB_CAP_ID         CAP_ID_CVCSPKR1MIC_SEND_FB
#define CVCSPKR2MIC_SEND_NB_CAP_ID         CAP_ID_CVCSPKR2MIC_SEND_NB
#define CVCSPKR2MIC_SEND_WB_CAP_ID         CAP_ID_CVCSPKR2MIC_SEND_WB
#define CVCSPKR2MIC_SEND_UWB_CAP_ID        CAP_ID_CVCSPKR2MIC_SEND_UWB
#define CVCSPKR2MIC_SEND_SWB_CAP_ID        CAP_ID_CVCSPKR2MIC_SEND_SWB
#define CVCSPKR2MIC_SEND_FB_CAP_ID         CAP_ID_CVCSPKR2MIC_SEND_FB
#define CVCSPKR3MIC_SEND_NB_CAP_ID         CAP_ID_CVCSPKR3MIC_SEND_NB
#define CVCSPKR3MIC_SEND_WB_CAP_ID         CAP_ID_CVCSPKR3MIC_SEND_WB
#define CVCSPKR3MIC_SEND_UWB_CAP_ID        CAP_ID_CVCSPKR3MIC_SEND_UWB
#define CVCSPKR3MIC_SEND_SWB_CAP_ID        CAP_ID_CVCSPKR3MIC_SEND_SWB
#define CVCSPKR3MIC_SEND_FB_CAP_ID         CAP_ID_CVCSPKR3MIC_SEND_FB
#define CVCSPKR3MIC_CIRC_SEND_NB_CAP_ID    CAP_ID_CVCSPKR3MIC_CIRC_SEND_NB
#define CVCSPKR3MIC_CIRC_SEND_WB_CAP_ID    CAP_ID_CVCSPKR3MIC_CIRC_SEND_WB
#define CVCSPKR3MIC_CIRC_SEND_UWB_CAP_ID   CAP_ID_CVCSPKR3MIC_CIRC_SEND_UWB
#define CVCSPKR3MIC_CIRC_SEND_SWB_CAP_ID   CAP_ID_CVCSPKR3MIC_CIRC_SEND_SWB
#define CVCSPKR3MIC_CIRC_SEND_FB_CAP_ID    CAP_ID_CVCSPKR3MIC_CIRC_SEND_FB

#define CVCSPKR3MIC_CIRC_VA_SEND_WB_CAP_ID    CAP_ID_CVCSPKR3MIC_CIRC_VA_SEND_WB
#define CVCSPKR3MIC_CIRC_VA4B_SEND_WB_CAP_ID    CAP_ID_CVCSPKR3MIC_CIRC_VA4B_SEND_WB

#define CVCSPKR4MIC_SEND_NB_CAP_ID         CAP_ID_CVCSPKR4MIC_SEND_NB
#define CVCSPKR4MIC_SEND_WB_CAP_ID         CAP_ID_CVCSPKR4MIC_SEND_WB
#define CVCSPKR4MIC_SEND_UWB_CAP_ID        CAP_ID_CVCSPKR4MIC_SEND_UWB
#define CVCSPKR4MIC_SEND_SWB_CAP_ID        CAP_ID_CVCSPKR4MIC_SEND_SWB
#define CVCSPKR4MIC_SEND_FB_CAP_ID         CAP_ID_CVCSPKR4MIC_SEND_FB
#define CVCSPKR4MIC_CIRC_SEND_UWB_CAP_ID   CAP_ID_CVCSPKR4MIC_CIRC_SEND_UWB
#define CVCSPKR4MIC_CIRC_SEND_SWB_CAP_ID   CAP_ID_CVCSPKR4MIC_CIRC_SEND_SWB
#define CVCSPKR4MIC_CIRC_SEND_FB_CAP_ID    CAP_ID_CVCSPKR4MIC_CIRC_SEND_FB

#define CVCEB2MIC_IE_NB_CAP_ID             CAP_ID_CVCEB2MIC_IE_NB
#define CVCEB2MIC_IE_WB_CAP_ID             CAP_ID_CVCEB2MIC_IE_WB
#define CVCEB3MIC_MONO_IE_NB_CAP_ID        CAP_ID_CVCEB3MIC_MONO_IE_NB
#define CVCEB3MIC_MONO_IE_WB_CAP_ID        CAP_ID_CVCEB3MIC_MONO_IE_WB
#define CVCEB3MIC_MONO_IE_SWB_CAP_ID       CAP_ID_CVCEB3MIC_MONO_IE_SWB

#define CVCEB3MIC_HYBRID_WB_CAP_ID         CAP_ID_CVCEB3MIC_HYBRID_WB
#define CVCEB3MIC_HYBRID_SWB_CAP_ID        CAP_ID_CVCEB3MIC_HYBRID_SWB
#define CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID         CAP_ID_CVCHS2MIC_MONO_SEND_HYBRID_WB
#define CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID        CAP_ID_CVCHS2MIC_MONO_SEND_HYBRID_SWB
#define CVCHS1MIC_SEND_HYBRID_WB_CAP_ID              CAP_ID_CVCHS1MIC_HYBRID_WB
#define CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID             CAP_ID_CVCHS1MIC_HYBRID_SWB
#endif

/* Message handlers */

/** The cvc send capability function handler table */
const handler_lookup_struct cvc_send_handler_table =
{
    cvc_send_create,          /* OPCMD_CREATE */
    cvc_send_destroy,         /* OPCMD_DESTROY */
    base_op_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    cvc_send_connect,         /* OPCMD_CONNECT */
    cvc_send_disconnect,      /* OPCMD_DISCONNECT */
    cvc_send_buffer_details,  /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    cvc_send_get_sched_info   /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table */
const opmsg_handler_lookup_table_entry cvc_send_opmsg_handler_table[] =
    {{OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,           base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,                       cvc_send_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_PARAMS,                        cvc_send_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,                      cvc_send_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                        cvc_send_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_GET_STATUS,                        cvc_send_opmsg_obpm_get_status},

    {OPMSG_COMMON_ID_SET_UCID,                          cvc_send_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,                 cvc_send_opmsg_get_ps_id},
    {OPMSG_COMMON_GET_VOICE_QUALITY,                    cvc_send_opmsg_get_voice_quality},
    {OPMSG_CVC_SEND_ID_ENABLE_PURGE_TILL_INPUTS_SYNCED, cvc_send_opmsg_set_purge_till_inputs_synced},
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID) 
    {OPMSG_ML_ENGINE_ID_LOAD_MODEL,                     cvc_mlfe_opmsg_load_model},
    {OPMSG_ML_ENGINE_ID_UNLOAD_MODEL,                   cvc_mlfe_opmsg_unload_model},
    {OPMSG_ML_ENGINE_ID_ACTIVATE_MODEL,                 cvc_mlfe_opmsg_activate_model},
#endif    

    {0, NULL}};


/****************************************************************************
CVC send capability data declarations
*/
#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC
    const CAPABILITY_DATA cvc_send_1mic_nb_hs_cap_data =
        {
            CVCHS1MIC_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS1MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_1mic_wb_hs_cap_data =
        {
            CVCHS1MIC_SEND_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS1MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_1mic_uwb_hs_cap_data =
        {
            CVCHS1MIC_SEND_UWB_CAP_ID,      /* Capability ID */
            GEN_CVC_SEND_1M_HS_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_1mic_swb_hs_cap_data =
        {
            CVCHS1MIC_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_1mic_fb_hs_cap_data =
        {
            CVCHS1MIC_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS1MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS1MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_nb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_MONO_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_2mic_hs_mono_wb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_MONO_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_uwb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_UWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_MONO_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_swb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_MONO_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_fb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_MONO_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_MONO_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_BINAURAL
    const CAPABILITY_DATA cvc_send_2mic_hs_binaural_nb_cap_data =
        {
            CVCHS2MIC_BINAURAL_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSB_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BINAURAL_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_2mic_hs_binaural_wb_cap_data =
        {
            CVCHS2MIC_BINAURAL_SEND_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BINAURAL_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_2mic_hs_binaural_uwb_cap_data =
        {
            CVCHS2MIC_BINAURAL_SEND_UWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSB_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BINAURAL_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_2mic_hs_binaural_swb_cap_data =
        {
            CVCHS2MIC_BINAURAL_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BINAURAL_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_2mic_hs_binaural_fb_cap_data =
        {
            CVCHS2MIC_BINAURAL_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSB_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BINAURAL_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BINAURAL_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_AUTO_1MIC
    const CAPABILITY_DATA cvc_send_1mic_nb_auto_cap_data =
        {
            CVCHF1MIC_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_AUTO_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF1MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_1mic_wb_auto_cap_data =
        {
            CVCHF1MIC_SEND_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_AUTO_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF1MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_1mic_uwb_auto_cap_data =
        {
            CVCHF1MIC_SEND_UWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_AUTO_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_1mic_swb_auto_cap_data =
        {
            CVCHF1MIC_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_AUTO_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_1mic_fb_auto_cap_data =
        {
            CVCHF1MIC_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_AUTO_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF1MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF1MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_AUTO_2MIC
    const CAPABILITY_DATA cvc_send_2mic_nb_auto_cap_data =
        {
            CVCHF2MIC_SEND_NB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_2M_AUTO_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF2MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_2mic_wb_auto_cap_data =
        {
            CVCHF2MIC_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_2M_AUTO_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF2MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_2mic_uwb_auto_cap_data =
        {
            CVCHF2MIC_SEND_UWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_2M_AUTO_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF2MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_2mic_swb_auto_cap_data =
        {
            CVCHF2MIC_SEND_SWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_2M_AUTO_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF2MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_2mic_fb_auto_cap_data =
        {
            CVCHF2MIC_SEND_FB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_2M_AUTO_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHF2MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHF2MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_1MIC
    const CAPABILITY_DATA cvc_send_1mic_speaker_nb_cap_data =
        {
            CVCSPKR1MIC_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_SPKR_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR1MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_1mic_speaker_wb_cap_data =
        {
            CVCSPKR1MIC_SEND_WB_CAP_ID,          /* Capability ID */
            GEN_CVC_SEND_1M_SPKR_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR1MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_1mic_speaker_uwb_cap_data =
        {
            CVCSPKR1MIC_SEND_UWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_SPKR_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_1mic_speaker_swb_cap_data =
        {
            CVCSPKR1MIC_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_SPKR_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_1mic_speaker_fb_cap_data =
        {
            CVCSPKR1MIC_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_SPKR_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR1MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR1MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_2MIC
    const CAPABILITY_DATA cvc_send_2mic_speaker_nb_cap_data =
        {
            CVCSPKR2MIC_SEND_NB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_SPKRB_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR2MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_2mic_speaker_wb_cap_data =
        {
            CVCSPKR2MIC_SEND_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_SPKRB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR2MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_2mic_speaker_uwb_cap_data =
        {
            CVCSPKR2MIC_SEND_UWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_SPKRB_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR2MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_2mic_speaker_swb_cap_data =
        {
            CVCSPKR2MIC_SEND_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_SPKRB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR2MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_2mic_speaker_fb_cap_data =
        {
            CVCSPKR2MIC_SEND_FB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_SPKRB_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR2MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR2MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_3MIC
    const CAPABILITY_DATA cvc_send_3mic_speaker_nb_cap_data =
        {
            CVCSPKR3MIC_SEND_NB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRB_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_3mic_speaker_wb_cap_data =
        {
            CVCSPKR3MIC_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_3mic_speaker_uwb_cap_data =
        {
            CVCSPKR3MIC_SEND_UWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRB_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_3mic_speaker_swb_cap_data =
        {
            CVCSPKR3MIC_SEND_SWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_3mic_speaker_fb_cap_data =
        {
            CVCSPKR3MIC_SEND_FB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRB_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif


#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_4MIC
    const CAPABILITY_DATA cvc_send_4mic_speaker_nb_cap_data =
        {
            CVCSPKR4MIC_SEND_NB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRB_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_SEND_NB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_NB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

    const CAPABILITY_DATA cvc_send_4mic_speaker_wb_cap_data =
        {
            CVCSPKR4MIC_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_4mic_speaker_uwb_cap_data =
        {
            CVCSPKR4MIC_SEND_UWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRB_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_4mic_speaker_swb_cap_data =
        {
            CVCSPKR4MIC_SEND_SWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#endif
#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_4mic_speaker_fb_cap_data =
        {
            CVCSPKR4MIC_SEND_FB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRB_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif


#endif

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC
    const CAPABILITY_DATA cvc_send_3mic_circ_speaker_wb_cap_data =
        {
            CVCSPKR3MIC_CIRC_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_3mic_circ_speaker_uwb_cap_data =
        {
            CVCSPKR3MIC_CIRC_SEND_UWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_3mic_circ_speaker_swb_cap_data =
        {
            CVCSPKR3MIC_CIRC_SEND_SWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_3mic_circ_speaker_fb_cap_data =
        {
            CVCSPKR3MIC_CIRC_SEND_FB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif /* INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC */

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_4MIC
#ifdef INSTALL_OPERATOR_CVC_24K
    const CAPABILITY_DATA cvc_send_4mic_circ_speaker_uwb_cap_data =
        {
            CVCSPKR4MIC_CIRC_SEND_UWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRCIRC_UWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_CIRC_SEND_UWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_UWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_4mic_circ_speaker_swb_cap_data =
        {
            CVCSPKR4MIC_CIRC_SEND_SWB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRCIRC_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_CIRC_SEND_SWB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_SWB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_48K
    const CAPABILITY_DATA cvc_send_4mic_circ_speaker_fb_cap_data =
        {
            CVCSPKR4MIC_CIRC_SEND_FB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_4M_SPKRCIRC_FB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            5, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR4MIC_CIRC_SEND_FB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR4MIC_CIRC_SEND_FB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif
#endif /* INSTALL_OPERATOR_CVC_SPKR_CIRC_4MIC */

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC_VA
    const CAPABILITY_DATA cvc_send_3mic_circ_speaker_va_wb_cap_data =
        {
            CVCSPKR3MIC_CIRC_VA_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_VA_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, 5,                            /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_VA_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_VA_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC_VA4B
     const CAPABILITY_DATA cvc_send_3mic_circ_speaker_va4b_wb_cap_data =
        {
            CVCSPKR3MIC_CIRC_VA4B_SEND_WB_CAP_ID,                                                       /* Capability ID */
            GEN_CVC_SEND_3M_SPKRCIRC_VA4B_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, 5,                            /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCSPKR3MIC_CIRC_VA4B_SEND_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCSPKR3MIC_CIRC_VA4B_SEND_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_VA_WAKEON
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_wb_va_wakeon_cap_data =
        {
            CVCHS2MIC_WAKEON_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_WAKEON_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_WAKEON_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_VA_BARGEIN
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_wb_va_bargein_cap_data =
        {
            CVCHS2MIC_BARGEIN_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_CVCHS2MIC_BARGEIN_WB, CVC_SEND_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_CVCHS2MIC_BARGEIN_WB, CVC_SEND_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC_VA
    const CAPABILITY_DATA cvc_send_1mic_wb_hs_va_cap_data =
        {
            CVCHS1MIC_SEND_WB_VA_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_1M_HS_VA_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

    MAP_INSTANCE_DATA(CVCHS1MIC_SEND_WB_VA_CAP_ID, CVC_SEND_OP_DATA)
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_2MIC_IE
    const CAPABILITY_DATA cvc_send_earbud_2mic_wb_ie_cap_data =
        {
            CVCEB2MIC_IE_WB_CAP_ID,         /* Capability ID */
            GEN_CVC_SEND_2M_EB_IE_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,             /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,        /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table,   /* Pointer to operator message handler function table */
            cvc_send_process_data,          /* Pointer to data processing function */
            0,                              /* Reserved */
            sizeof(CVC_SEND_OP_DATA)        /* Size of capability-specific per-instance data */
        };

   MAP_INSTANCE_DATA(CVCEB2MIC_IE_WB_CAP_ID, CVC_SEND_OP_DATA)
    
    const CAPABILITY_DATA cvc_send_earbud_2mic_nb_ie_cap_data =
        {
            CVCEB2MIC_IE_NB_CAP_ID,         /* Capability ID */
            GEN_CVC_SEND_2M_EB_IE_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,             /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,        /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table,   /* Pointer to operator message handler function table */
            cvc_send_process_data,          /* Pointer to data processing function */
            0,                              /* Reserved */
            sizeof(CVC_SEND_OP_DATA)        /* Size of capability-specific per-instance data */
        };
     MAP_INSTANCE_DATA(CVCEB2MIC_IE_NB_CAP_ID, CVC_SEND_OP_DATA)
     
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_3MIC_MONO_IE
    const CAPABILITY_DATA cvc_send_earbud_3mic_mono_ie_cap_data =
        {
            CVCEB3MIC_MONO_IE_WB_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_3M_EBE_IE_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

    MAP_INSTANCE_DATA(CVCEB3MIC_MONO_IE_WB_CAP_ID, CVC_SEND_OP_DATA)
    
    const CAPABILITY_DATA cvc_send_earbud_3mic_mono_nb_ie_cap_data =
        {
            CVCEB3MIC_MONO_IE_NB_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_3M_EBE_IE_NB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

    MAP_INSTANCE_DATA(CVCEB3MIC_MONO_IE_NB_CAP_ID, CVC_SEND_OP_DATA)

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_earbud_3mic_mono_ie_swb_cap_data =
        {
            CVCEB3MIC_MONO_IE_SWB_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_3M_EBE_IE_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

    MAP_INSTANCE_DATA(CVCEB3MIC_MONO_IE_SWB_CAP_ID, CVC_SEND_OP_DATA)
#endif
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID
    const CAPABILITY_DATA cvc_send_earbud_3mic_hybrid_wb_cap_data =
        {
            CVCEB3MIC_HYBRID_WB_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_3M_EB_HYB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
    MAP_INSTANCE_DATA(CVCEB3MIC_HYBRID_WB_CAP_ID, CVC_SEND_OP_DATA)

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_earbud_3mic_hybrid_swb_cap_data =
        {
            CVCEB3MIC_HYBRID_SWB_CAP_ID,  /* Capability ID */
            GEN_CVC_SEND_3M_EB_HYB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            4, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };

    MAP_INSTANCE_DATA(CVCEB3MIC_HYBRID_SWB_CAP_ID, CVC_SEND_OP_DATA)
#endif
#endif // INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_hybrid_wb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_HYB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
    MAP_INSTANCE_DATA(CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID, CVC_SEND_OP_DATA)

#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_2mic_hs_mono_hybrid_swb_cap_data =
        {
            CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_2M_HSE_HYB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            3, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
    MAP_INSTANCE_DATA(CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID, CVC_SEND_OP_DATA)
#endif
#endif // INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID

#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID
    const CAPABILITY_DATA cvc_send_1mic_hs_hybrid_wb_cap_data =
        {
            CVCHS1MIC_SEND_HYBRID_WB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_HYB_WB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
    MAP_INSTANCE_DATA(CVCHS1MIC_SEND_HYBRID_WB_CAP_ID, CVC_SEND_OP_DATA)
#ifdef INSTALL_OPERATOR_CVC_32K
    const CAPABILITY_DATA cvc_send_1mic_hs_hybrid_swb_cap_data =
        {
            CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID,       /* Capability ID */
            GEN_CVC_SEND_1M_HS_HYB_SWB_VERSION_MAJOR, CVC_SEND_CAP_VERSION_MINOR, /* Version information - hi and lo parts */
            2, CVC_NUM_OUTPUTS,           /* Max number of sinks/inputs and sources/outputs */
            &cvc_send_handler_table,      /* Pointer to message handler function table */
            cvc_send_opmsg_handler_table, /* Pointer to operator message handler function table */
            cvc_send_process_data,        /* Pointer to data processing function */
            0,                               /* Reserved */
            sizeof(CVC_SEND_OP_DATA)      /* Size of capability-specific per-instance data */
        };
    MAP_INSTANCE_DATA(CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID, CVC_SEND_OP_DATA)
#endif
#endif // INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID
/*
 *   cVc send capability configuration table:
 *       cap1_config_func(), cap1_id_nb, cap1_id_wb, cap1_id_uwb, cap1_id_swb, cap1_id_fb,
 *       cap2_config_func(), cap2_id_nb, cap2_id_wb, cap2_id_uwb, cap2_id_swb, cap2_id_fb,
 *       ...
 *       0;
 */
const cvc_send_config_data cvc_send_caps[] = {
#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC
    {
        &CVC_SEND_CAP_Config_headset_1mic,
        {
            CVCHS1MIC_SEND_NB_CAP_ID,  CVCHS1MIC_SEND_WB_CAP_ID,
            CVCHS1MIC_SEND_UWB_CAP_ID, CVCHS1MIC_SEND_SWB_CAP_ID, CVCHS1MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO
    {
        &CVC_SEND_CAP_Config_headset_2mic_mono,
        {
            CVCHS2MIC_MONO_SEND_NB_CAP_ID,   CVCHS2MIC_MONO_SEND_WB_CAP_ID,
            CVCHS2MIC_MONO_SEND_UWB_CAP_ID,  CVCHS2MIC_MONO_SEND_SWB_CAP_ID,  CVCHS2MIC_MONO_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_BINAURAL
    {
        &CVC_SEND_CAP_Config_headset_2mic_binaural,
        {
            CVCHS2MIC_BINAURAL_SEND_NB_CAP_ID,  CVCHS2MIC_BINAURAL_SEND_WB_CAP_ID,
            CVCHS2MIC_BINAURAL_SEND_UWB_CAP_ID, CVCHS2MIC_BINAURAL_SEND_SWB_CAP_ID, CVCHS2MIC_BINAURAL_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_1MIC
    {
        &CVC_SEND_CAP_Config_speaker_1mic,
        {
            CVCSPKR1MIC_SEND_NB_CAP_ID,   CVCSPKR1MIC_SEND_WB_CAP_ID,
            CVCSPKR1MIC_SEND_UWB_CAP_ID,  CVCSPKR1MIC_SEND_SWB_CAP_ID,  CVCSPKR1MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_2MIC
    {
        &CVC_SEND_CAP_Config_speaker_2mic,
        {
            CVCSPKR2MIC_SEND_NB_CAP_ID,   CVCSPKR2MIC_SEND_WB_CAP_ID,
            CVCSPKR2MIC_SEND_UWB_CAP_ID,  CVCSPKR2MIC_SEND_SWB_CAP_ID,  CVCSPKR2MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_3MIC
    {
        &CVC_SEND_CAP_Config_speaker_3mic,
        {
            CVCSPKR3MIC_SEND_NB_CAP_ID,   CVCSPKR3MIC_SEND_WB_CAP_ID,
            CVCSPKR3MIC_SEND_UWB_CAP_ID,  CVCSPKR3MIC_SEND_SWB_CAP_ID,  CVCSPKR3MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPEAKER_4MIC
    {
        &CVC_SEND_CAP_Config_speaker_4mic,
        {
            CVCSPKR4MIC_SEND_NB_CAP_ID,   CVCSPKR4MIC_SEND_WB_CAP_ID,
            CVCSPKR4MIC_SEND_UWB_CAP_ID,  CVCSPKR4MIC_SEND_SWB_CAP_ID,  CVCSPKR4MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_AUTO_1MIC
    {
        &CVC_SEND_CAP_Config_auto_1mic,
        {
            CVCHF1MIC_SEND_NB_CAP_ID,  CVCHF1MIC_SEND_WB_CAP_ID,
            CVCHF1MIC_SEND_UWB_CAP_ID, CVCHF1MIC_SEND_SWB_CAP_ID, CVCHF1MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_AUTO_2MIC
    {
        &CVC_SEND_CAP_Config_auto_2mic,
        {
            CVCHF2MIC_SEND_NB_CAP_ID,  CVCHF2MIC_SEND_WB_CAP_ID,
            CVCHF2MIC_SEND_UWB_CAP_ID, CVCHF2MIC_SEND_SWB_CAP_ID, CVCHF2MIC_SEND_FB_CAP_ID
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC
    {
        &CVC_SEND_CAP_Config_spkr_circ_3mic,
        {
            0,  CVCSPKR3MIC_CIRC_SEND_WB_CAP_ID,
            0, 0, 0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC_VA
    {
        &CVC_SEND_CAP_Config_spkr_circ_3mic_va,
        {
            0,  CVCSPKR3MIC_CIRC_VA_SEND_WB_CAP_ID,
            0, 0, 0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_SPKR_CIRC_3MIC_VA4B
    {
        &CVC_SEND_CAP_Config_spkr_circ_3mic_va4b,
        {
            0,  CVCSPKR3MIC_CIRC_VA4B_SEND_WB_CAP_ID,
            0, 0, 0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_VA_WAKEON
    {
        &CVC_SEND_CAP_Config_headset_2mic_wakeon,
        {
            0,   CVCHS2MIC_WAKEON_WB_CAP_ID,
            0,  0,  0
        }
     },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_VA_BARGEIN
    {
        &CVC_SEND_CAP_Config_headset_2mic_bargein,
        {
            0,   CVCHS2MIC_BARGEIN_WB_CAP_ID,
            0,  0,  0
        }
     },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC_VA
    {
        &CVC_SEND_CAP_Config_headset_1mic_va,
        {
            0,  CVCHS1MIC_SEND_WB_VA_CAP_ID,
            0,  0,  0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_2MIC_IE
    {
        &CVC_SEND_CAP_Config_earbud_2mic_ie,
        {
            CVCEB2MIC_IE_NB_CAP_ID,  CVCEB2MIC_IE_WB_CAP_ID,
            0,  0,  0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_3MIC_MONO_IE
    {
        &CVC_SEND_CAP_Config_earbud_3mic_ie,
        {
            CVCEB3MIC_MONO_IE_NB_CAP_ID,  CVCEB3MIC_MONO_IE_WB_CAP_ID,
            0,  CVCEB3MIC_MONO_IE_SWB_CAP_ID,  0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID
    {
        &CVC_SEND_CAP_Config_earbud_3mic_hybrid,
        {
            0,  CVCEB3MIC_HYBRID_WB_CAP_ID,
            0,  CVCEB3MIC_HYBRID_SWB_CAP_ID,  0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID
    {
        &CVC_SEND_CAP_Config_headset_2mic_mono_hybrid,
        {
            0,  CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID,
            0,  CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID,  0
        }
    },
#endif

#ifdef INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID
    {
        &CVC_SEND_CAP_Config_headset_1mic_hybrid,
        {
            0,  CVCHS1MIC_SEND_HYBRID_WB_CAP_ID,
            0,  CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID,  0
        }
    },
#endif

    {
        /* end of table */
        NULL, {0, 0, 0, 0, 0}
    }
};


/*
 * cvc_send_config()
 *    Search for op_extra_data->cap_id from cvc_send_caps[] table, if found
 *
 *       set the following field based on specified cap_id:
 *          op_extra_data->data_variant
 *          op_extra_data->major_config
 *          op_extra_data->num_mics
 *          op_extra_data->mic_config
 *          op_extra_data->frame_size
 *          op_extra_data->sample_rate
 *
 *       and initialize other internal fields
 */
bool cvc_send_config(CVC_SEND_OP_DATA *op_extra_data, const cvc_send_config_data *cvc_send_caps_table)
{
    const cvc_send_config_data *caps;
    unsigned cap_id = op_extra_data->cap_id;
    unsigned variant = 0;

    for (caps = cvc_send_caps_table; caps->config_func != NULL; caps++)
    {
        for (variant = DATA_VARIANT_NB; variant <= DATA_VARIANT_FB; variant++)
        {
            if (caps->cap_ids[variant] == cap_id)
            {
                caps->config_func(op_extra_data, variant);

                switch(op_extra_data->data_variant)
                {
                case DATA_VARIANT_WB:  // 16 kHz
                    op_extra_data->frame_size = 120;
                    op_extra_data->sample_rate = 16000;
                    break;
                case DATA_VARIANT_UWB: // 24 kHz
                    op_extra_data->frame_size = 120;
                    op_extra_data->sample_rate = 24000;
                    break;
                case DATA_VARIANT_SWB: // 32 kHz
                    op_extra_data->frame_size = 240;
                    op_extra_data->sample_rate = 32000;
                    break;
                case DATA_VARIANT_FB:  // 48 kHz
                    op_extra_data->frame_size = 240;
                    op_extra_data->sample_rate = 48000;
                    break;
                case DATA_VARIANT_NB:  // 8 kHz
                default:
                    op_extra_data->frame_size = 60;
                    op_extra_data->sample_rate = 8000;
                    break;
                }
                op_extra_data->ReInitFlag = 1;
                op_extra_data->Host_mode = GEN_CVC_SEND_SYSMODE_FULL;
                op_extra_data->Cur_mode = GEN_CVC_SEND_SYSMODE_STANDBY;
                op_extra_data->mute_control_ptr = &op_extra_data->Cur_Mute;

                return TRUE;
            }
        }
    }

   return FALSE;
}

static inline CVC_SEND_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (CVC_SEND_OP_DATA *) base_op_get_instance_data(op_data);
}

/****************************************************************************
Public Function Declarations
*/


/* ********************************** API functions ************************************* */

bool ups_state_snd(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank,
                 uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info)
{
    CVC_SEND_OP_DATA   *op_extra_data = get_instance_data((OPERATOR_DATA*)instance_data);

    L2_DBG_MSG4("ups_state_snd len=%d rank=%d status=%d extra_info=%d \n", length, (unsigned)rank, (unsigned)status, extra_status_info );
    if((length==2)&&(status==STATUS_OK))
    {
        op_extra_data->mdgc_gain = ((data[0]&0xFFFF)<<16) | (data[1]&0xFFFF);
        /* Set the Reinit flag after setting the paramters */
        op_extra_data->ReInitFlag = 1;
    }

    return(TRUE);
}

bool ups_state_snd_eqmap(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank,
                 uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info)
{
    unsigned i;
    CVC_SEND_OP_DATA   *op_extra_data = get_instance_data((OPERATOR_DATA*)instance_data);
    L2_DBG_MSG4("ups_state_snd_eqmap len=%d rank=%d status=%d extra_info=%d \n", length, (unsigned)rank, (unsigned)status, extra_status_info );
    if((length==DATA_VARIANT_NB_NBINS*2 || length==DATA_VARIANT_WB_NBINS*2 || length==DATA_VARIANT_SWB_NBINS*2)&&(status==STATUS_OK))
    {
         /* unpack 2*nbins half-words (small endian format) */
         for (i=0; i<length/2; i++)
		 {
             op_extra_data->ptr_map_ratio[i] = (data[2*i] & 0xFFFF)  | ( (data[2*i+1] & 0xFFFF)<<16) ;
         }
         op_extra_data->ReInitFlag = 1;
    }
    return(TRUE);
}

#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
void cvc_send_create_pending_cb(OPERATOR_DATA *op_data,
                                uint16 cmd_id, void *msg_body,
                                pendingContext *context, unsigned cb_value)
{
    external_constant_callback_when_available(op_data, (void*)cb_value, cmd_id,
                                              msg_body, context);
}

#endif

void cvc_send_release_constants(OPERATOR_DATA *op_data)
{
#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    INT_OP_ID int_id = base_op_get_int_op_id(op_data);

    external_constant_release(cvclib_dataDynTable_Main, int_id);
    external_constant_release(aec530_DynamicMemDynTable_Main, int_id);
    external_constant_release(ASF100_DynamicMemDynTable_Main, int_id);
    external_constant_release(oms280_DynamicMemDynTable_Main, int_id);
    external_constant_release(filter_bank_DynamicMemDynTable_Main, int_id);
    external_constant_release(vad410_DynamicMemDynTable_Main, int_id);
    external_constant_release(op_extra_data->dyn_main, int_id);
#else
    NOT_USED(op_data);
#endif
}

#if defined(DEBUG_MIPS)
   unsigned *mips_ptr;
#endif
bool cvc_send_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    PS_KEY_TYPE key;

    patch_fn_shared(cvc_send_wrapper);
    /* Setup Response to Creation Request.   Assume Failure*/
    if (!base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data))
    {
        /* We call cvc_send_release_constants as there is a slim chance we fail on
         * the second pass through */
        cvc_send_release_constants(op_data);
        return(FALSE);
    }

    /* Initialize extended data for operator.  Assume intialized to zero*/
    op_extra_data->cap_id = base_op_get_cap_id(op_data);

    /* Capability Specific Configuration */
    if (FALSE == cvc_send_config(op_extra_data, cvc_send_caps)) {
        return(TRUE);
    }

#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
    /* Reserve (and request) any dynamic memory tables that may be in external
     * file system.
     * A negative return value indicates a fatal error */
    INT_OP_ID int_id = base_op_get_int_op_id(op_data);
    if (   !external_constant_reserve(cvclib_dataDynTable_Main, int_id)
        || !external_constant_reserve(aec530_DynamicMemDynTable_Main, int_id)
        || !external_constant_reserve(ASF100_DynamicMemDynTable_Main, int_id)
        || !external_constant_reserve(oms280_DynamicMemDynTable_Main, int_id)
        || !external_constant_reserve(filter_bank_DynamicMemDynTable_Main, int_id)
        || !external_constant_reserve(vad410_DynamicMemDynTable_Main, int_id)
        || !external_constant_reserve(op_extra_data->dyn_main, int_id))
    {
        L2_DBG_MSG("cvc_send_create failed reserving constants");
        cvc_send_release_constants(op_data);
        return TRUE;
    }

    /* Now see if these tables are available yet */
    if (   !is_external_constant_available(cvclib_dataDynTable_Main, int_id)
        || !is_external_constant_available(aec530_DynamicMemDynTable_Main, int_id)
        || !is_external_constant_available(ASF100_DynamicMemDynTable_Main, int_id)
        || !is_external_constant_available(oms280_DynamicMemDynTable_Main, int_id)
        || !is_external_constant_available(filter_bank_DynamicMemDynTable_Main, int_id)
        || !is_external_constant_available(vad410_DynamicMemDynTable_Main, int_id)
        || !is_external_constant_available(op_extra_data->dyn_main, int_id))
    {
        /* Free the response created above, before it gets overwritten with the pending data */
        pdelete(*response_data);

        /* Database isn't available yet. Arrange for a callback
         * Only need to check on one table */
        *response_id = (unsigned)op_extra_data->dyn_main;
        *response_data = (void*)(pending_operator_cb)cvc_send_create_pending_cb;

        L4_DBG_MSG("cvc_send_create - requesting callback when constants available");
        return (bool)HANDLER_INCOMPLETE;
    }
#endif

    patch_fn_shared(cvc_send_wrapper);


    /*allocate the volume control shared memory*/
    op_extra_data->shared_volume_ptr = allocate_shared_volume_cntrl();
    if(!op_extra_data->shared_volume_ptr)
    {
        cvc_send_release_constants(op_data);
        return(TRUE);
    }

    cvc_send_resampler_create(op_extra_data);

    /* call the "create" assembly function */
    if(CVC_SEND_CAP_Create(op_extra_data))
    {
        /* Free all the scratch memory we reserved */
        CVC_SEND_CAP_Destroy(op_extra_data);
        release_shared_volume_cntrl(op_extra_data->shared_volume_ptr);
        op_extra_data->shared_volume_ptr = NULL;
        cvc_send_release_constants(op_data);
        return(TRUE);
    }

    if (op_extra_data->mbdrc_obj) {
        if (!cvc_send_mbdrc_create(op_extra_data)) {
            CVC_SEND_CAP_Destroy(op_extra_data);
            release_shared_volume_cntrl(op_extra_data->shared_volume_ptr);
            op_extra_data->shared_volume_ptr = NULL;
            cvc_send_release_constants(op_data);
            return(TRUE);
        }
    }

    #if defined(DEBUG_MIPS)
       mips_ptr = (unsigned*)op_extra_data->mips_array;
    #endif

    if(!cvc_send_register_component((void*)op_extra_data))
    {
        /* Free all the scratch memory we reserved, exit with fail response msg. Even if it had failed
         * silently, subsequent security checks will fail in lack of a successful registration.
         */
        CVC_SEND_CAP_Destroy(op_extra_data);
        release_shared_volume_cntrl(op_extra_data->shared_volume_ptr);
        op_extra_data->shared_volume_ptr = NULL;
        cvc_send_release_constants(op_data);
        return(TRUE);
    }


    if(!cpsInitParameters(&op_extra_data->parms_def,(unsigned*)CVC_SEND_GetDefaults(op_extra_data->cap_id),(unsigned*)op_extra_data->params,sizeof(CVC_SEND_PARAMETERS)))
    {
        /* Free all the scratch memory we reserved, exit with fail response msg. Even if it had failed
         * silently, subsequent security checks will fail in lack of a successful registration.
         */
        CVC_SEND_CAP_Destroy(op_extra_data);
        release_shared_volume_cntrl(op_extra_data->shared_volume_ptr);
        op_extra_data->shared_volume_ptr = NULL;
        cvc_send_release_constants(op_data);
        return(TRUE);
    }

     /* Read state info for MDGC */
    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(op_extra_data->cap_id,0,OPMSG_P_STORE_STATE_VARIABLE_SUB_ID);
    ps_entry_read((void*)op_data,key,PERSIST_ANY,ups_state_snd);
    L4_DBG_MSG1("ps req MGDC: key=%d \n", key );

     /* Read EQmap vector */
    if (op_extra_data->ptr_map_ratio)
    {
        /* make the p-store request */
        key = MAP_CAPID_UCID_SBID_TO_PSKEYID(op_extra_data->cap_id, UCID_EQMAP, OPMSG_P_STORE_STATE_VARIABLE_SUB_ID);
        ps_entry_read((void*)op_data,key,PERSIST_ANY,ups_state_snd_eqmap);
        L4_DBG_MSG1("ps req EQMAP: key=%d \n", key );
    }
    
    /* allocate the ML Engine data object  */
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
    if ((op_extra_data->mic_config & MIC_CONFIG_AI) && (op_extra_data->mlfe_obj != NULL))
    {
   
        op_extra_data->mlfe_obj->ml_engine_container  = (ML_ENGINE_OP_DATA*)xzpmalloc(sizeof(ML_ENGINE_OP_DATA));
        if (op_extra_data->mlfe_obj->ml_engine_container  == NULL)
        {
            return FALSE; 
        }
         L2_DBG_MSG("ml_engine container created");
#if 0 // removed to accomodale ml_engine changes         
        /* Initialise the frames_processed */
        op_extra_data->mlfe_obj->ml_engine_container->frames_processed = 0;
        /* Initialise the input and output data formats */
        op_extra_data->mlfe_obj->ml_engine_container->ip_format = AUDIO_DATA_FORMAT_FIXP;
        op_extra_data->mlfe_obj->ml_engine_container->op_format = AUDIO_DATA_FORMAT_FIXP;
            /* Initialize the model load status */
        op_extra_data->mlfe_obj->ml_engine_container->model_load_status = 0;
#endif


    }
#endif


    op_extra_data->input_metadata_buffer = NULL;
    op_extra_data->output_metadata_buffer = NULL;

    /* This feature is enabled by default */
    op_extra_data->enable_purge_till_inputs_synced = 1;

    base_op_change_response_status(response_data,STATUS_OK);
    return TRUE;
}

bool cvc_send_ups_set_state(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, STATUS_KYMERA status,
                                     uint16 extra_status_info)
{
    L2_DBG_MSG3("ups_set_state  rank=%d status=%d extra_info=%d \n", (unsigned)rank, (unsigned)status, extra_status_info );
    return TRUE;
}

bool cvc_send_ups_set_state_eqmap(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, STATUS_KYMERA status,
                                     uint16 extra_status_info)
{
    L2_DBG_MSG3("ups_set_state_eqmap  rank=%d status=%d extra_info=%d \n", (unsigned)rank, (unsigned)status, extra_status_info );
    return TRUE;
}

bool cvc_send_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

     patch_fn_shared(cvc_send_wrapper);

    /* Setup Response to Destroy Request.*/
    if(!base_op_destroy_lite(op_data, response_data))
    {
        cvc_send_release_constants(op_data);
        return(FALSE);
    }

    /* free MBDRC */
    if (op_extra_data->mbdrc_obj) {
        pfree(op_extra_data->mbdrc_obj->mbdrc_lib.lib_mem_ptr);
    }

    /* calling the "destroy" assembly function - this frees up all the capability-internal memory */
    CVC_SEND_CAP_Destroy(op_extra_data);

    /*free volume control shared memory*/
    release_shared_volume_cntrl(op_extra_data->shared_volume_ptr);
    op_extra_data->shared_volume_ptr = NULL;

#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
    if (op_extra_data->mic_config & MIC_CONFIG_AI)
    {
        
        ml_engine_delete_all_node(&op_extra_data->mlfe_obj->ml_engine_container->use_cases, ml_engine_free_usecase_node);
        L2_DBG_MSG("ml_engine node deleted");
        /*free ML engine data object*/
        pfree(op_extra_data->mlfe_obj->ml_engine_container);
        op_extra_data->mlfe_obj->ml_engine_container = NULL;
         L2_DBG_MSG("ml_engine container freed");
        
    }
#endif
    base_op_change_response_status(response_data,STATUS_OK);

    /* Save state info to ucid 0 */
      {
          unsigned key = MAP_CAPID_UCID_SBID_TO_PSKEYID(op_extra_data->cap_id,0,OPMSG_P_STORE_STATE_VARIABLE_SUB_ID);
          uint16 state_data[2];

          state_data[0] = (op_extra_data->mdgc_gain>>16)&0xFFFF;
            state_data[1] = op_extra_data->mdgc_gain&0xFFFF;
          ps_entry_write((void*)op_data,key,PERSIST_ANY,2,state_data,cvc_send_ups_set_state);
          L4_DBG_MSG1("pswrite MGDC: key=%d \n", key );

      }
    /* Save state from map ratio vector info to ucid */
    if ( op_extra_data->ptr_map_ratio )
    {
        unsigned key = MAP_CAPID_UCID_SBID_TO_PSKEYID(op_extra_data->cap_id, UCID_EQMAP, OPMSG_P_STORE_STATE_VARIABLE_SUB_ID);
        uint16 bin_count;
        switch(op_extra_data->data_variant)
        {
              case DATA_VARIANT_SWB:  // 32 kHz
                  bin_count = DATA_VARIANT_SWB_NBINS; //257
                  break;
              case DATA_VARIANT_WB:  // 16 kHz
                  bin_count = DATA_VARIANT_WB_NBINS; //129
                  break;
              default: // only 32k, 16K and 8K are supported
                  bin_count = DATA_VARIANT_NB_NBINS; //65
                  break;
        }

        /* save bin_count*2 half-words */
        ps_entry_write((void*)op_data,key,PERSIST_ANY, (uint16)(bin_count*2), (uint16*)op_extra_data->ptr_map_ratio,cvc_send_ups_set_state_eqmap);
        L4_DBG_MSG1("pswrite EQMAP: key=%d \n", key );
    }                                                                                       

    cvc_send_release_constants(op_data);

    cvc_send_resampler_destroy(op_extra_data);

    return(TRUE);
}




void cvc_send_set_requested_features(CVC_SEND_OP_DATA *op_extra_data)
{
    op_extra_data->op_feature_requested = 0;
    int num_va_outputs = op_extra_data->num_va_outputs;
    int i;

    /* Check microphone connection */
    /* If microphone is not all connected, fail */
    for (i=1; i<=op_extra_data->num_mics; i++) {
        if(!op_extra_data->input_stream[i]) {
            return;
        }
    }

    /* Check VA channel connections */
    if(num_va_outputs)
    {
        op_extra_data->op_feature_requested = CVC_REQUESTED_FEATURE_VA;
        for (i=1; i<=num_va_outputs; i++) {
            if(!op_extra_data->output_stream[i]) {
                op_extra_data->op_feature_requested = 0;
                break;
            }
        }
    }

    /* Check VOICE channel connection */
    if(op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VOICE])
    {
        op_extra_data->op_feature_requested |= CVC_REQUESTED_FEATURE_VOICE;
    }

    /* If neither VOICE nor VA is requested, fail */
    if (!op_extra_data->op_feature_requested)
    {
        return;
    }

    /* Check AEC channel connection */
    if(op_extra_data->input_stream[0])
    {
        op_extra_data->op_feature_requested |= CVC_REQUESTED_FEATURE_AEC;
    }
}

bool cvc_send_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_CONNECT_TERMINAL_ID(message_data);    /* extract the terminal_id */
    tCbuffer* pterminal_buf = OPMGR_GET_OP_CONNECT_BUFFER(message_data);

    patch_fn_shared(cvc_send_wrapper);

    /* Setup Response to Connection Request.   Assume Failure*/
    if (!base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data))
    {
        return(FALSE);
    }

    /* (i)  check if the terminal ID is valid . The number has to be less than the maximum number of sinks or sources .  */
    /* (ii) check if we are connecting to the right type . It has to be a buffer pointer and not endpoint connection */
    if( !base_op_is_terminal_valid(op_data, terminal_id) || !pterminal_buf)
    {
        base_op_change_response_status(response_data,STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    /* Connect the appropriate stream map */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        op_extra_data->input_stream[terminal_id&CVC_SEND_NUM_INPUTS_MASK] = pterminal_buf;

        if(0 != (terminal_id & ~TERMINAL_SINK_MASK))
        {
            /* if this is a mic input terminal, then
             * the first connected buffer with metadata
             * will be used as metadata buffer
             */
            if(NULL == op_extra_data->input_metadata_buffer &&
               buff_has_metadata(pterminal_buf))
            {
                op_extra_data->input_metadata_buffer = pterminal_buf;
            }
        }
    }
    else
    {
        op_extra_data->output_stream[terminal_id&CVC_SEND_NUM_OUTPUTS_MASK] = pterminal_buf;
        if(NULL == op_extra_data->output_metadata_buffer &&
               buff_has_metadata(pterminal_buf))
        {
            op_extra_data->output_metadata_buffer = pterminal_buf;
        }
    }

    /* Allow Connect while running.  Re-enable processing alter all connections are completed */
    cvc_send_set_requested_features(op_extra_data);

    base_op_change_response_status(response_data,STATUS_OK);
    return TRUE;
}



bool cvc_send_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_DISCONNECT_TERMINAL_ID(message_data);

     patch_fn_shared(cvc_send_wrapper);

    /* Setup Response to Disconnection Request.   Assume Failure*/
    if (!base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data))
    {
        return(FALSE);
    }

    /* check if the terminal ID is valid . The number has to be less than the maximum number of sinks or sources.  */
    if(!base_op_is_terminal_valid(op_data, terminal_id))
    {
        base_op_change_response_status(response_data,STATUS_INVALID_CMD_PARAMS);
        return TRUE;
    }

    /* Disconnect the relevant terminal */
    if (terminal_id & TERMINAL_SINK_MASK)
    {
        if(terminal_id&CVC_SEND_NUM_INPUTS_MASK)
        {
            tCbuffer *this_buf = op_extra_data->input_stream[terminal_id&CVC_SEND_NUM_INPUTS_MASK];
            if(this_buf == op_extra_data->input_metadata_buffer)
            {
                /* disconnecting buffer is the metadata buffer,
                 * change the metadata buffer to another connected
                 * buffer with metadata, if there is any.
                 */
                tCbuffer *new_metadata_buf = NULL;
                int mic_idx;
                for(mic_idx=1; mic_idx <= op_extra_data->num_mics; mic_idx++)
                {
                    tCbuffer *mic_buffer = op_extra_data->input_stream[mic_idx];

                    if(mic_buffer != NULL &&
                       mic_buffer != this_buf &&
                       buff_has_metadata(mic_buffer))
                    {
                        new_metadata_buf = mic_buffer;
                        break;
                    }
                }
                op_extra_data->input_metadata_buffer = new_metadata_buf;
            }
        }

        op_extra_data->input_stream[terminal_id&CVC_SEND_NUM_INPUTS_MASK] = NULL;

    }
    else
    {
        tCbuffer *this_buf = op_extra_data->output_stream[terminal_id&CVC_SEND_NUM_OUTPUTS_MASK];
        if(this_buf == op_extra_data->output_metadata_buffer)
        {
            tCbuffer *new_metadata_buf = NULL;
            int op;
            for(op=0; op < CVC_SEND_MAX_NUM_OUTPUT; op++)
            {
               if(NULL != op_extra_data->output_stream[op])
               {
                   tCbuffer *op_buffer = op_extra_data->output_stream[op];

                   if(op_buffer != NULL &&
                         op_buffer != this_buf &&
                         buff_has_metadata(op_buffer))
                   {
                      new_metadata_buf = op_buffer;
                      break;
                   }
               }
            }
            op_extra_data->output_metadata_buffer = new_metadata_buf;
        }
        op_extra_data->output_stream[terminal_id&CVC_SEND_NUM_OUTPUTS_MASK] = NULL;
    }

    cvc_send_set_requested_features(op_extra_data);

    base_op_change_response_status(response_data,STATUS_OK);
    return TRUE;
}


bool cvc_send_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *opx_data = get_instance_data(op_data);
    unsigned terminal_id = OPMGR_GET_OP_BUF_DETAILS_TERMINAL_ID(message_data);

    patch_fn_shared(cvc_send_wrapper);

    if (!base_op_buffer_details_lite(op_data, response_data))
    {
        return FALSE;
    }
    /* No inplace support input samples may be dropped */
    ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size= opx_data->frame_size<<1;

    if(terminal_id == (TERMINAL_SINK_MASK|CVC_SEND_IN_TERMINAL_AECREF))
    {
        /* Reference buffer needs more space */
        ((OP_BUF_DETAILS_RSP*)*response_data)->b.buffer_size += (opx_data->frame_size>>1);
    }
    else
    {
        CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

        /* metadata is supported in all terminals except the reference one */
        ((OP_BUF_DETAILS_RSP*)*response_data)->supports_metadata = TRUE;
        if(terminal_id & TERMINAL_SINK_MASK)
        {
            /* A mic terminal, if we already have a connected buffer with metadata pass that one */
            ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = op_extra_data->input_metadata_buffer;
        }
        else
        {
            /* Output terminal */
            ((OP_BUF_DETAILS_RSP*)*response_data)->metadata_buffer = op_extra_data->output_metadata_buffer;
        }
    }

    return TRUE;
}


bool cvc_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    OP_SCHED_INFO_RSP* resp;

    resp = base_op_get_sched_info_ex(op_data, message_data, response_id);
    if (resp == NULL)
    {
        return base_op_build_std_response_ex(op_data, STATUS_CMD_FAILED, response_data);
    }
    *response_data = resp;

    /* Same buffer size for sink and source.
     * No additional verification needed.*/
    resp->block_size = op_extra_data->frame_size;

    return TRUE;
}

/**
 * cvc_send_transport_metadata
 * \brief transfers metadata from input to output buffer
 * \param op_extra_data pointer to CVC_SEND_OP_DATA
 */
void cvc_send_transport_metadata(CVC_SEND_OP_DATA *op_extra_data)
{
    metadata_tag *mtag;
    unsigned b4idx, afteridx;
    int delay_in_samples;
    TIME reference_time,latency_time;
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
    int mldelay_frames;
#endif    
    if (op_extra_data->input_metadata_buffer != NULL)
    {
       if(op_extra_data->Cur_mode == GEN_CVC_SEND_SYSMODE_FULL)
       {
           /* Extract metadata tag from input */
           mtag = buff_metadata_remove(op_extra_data->input_metadata_buffer, op_extra_data->frame_size * OCTETS_PER_SAMPLE, &b4idx, &afteridx);
       }
       else
       {
          /* Propagate any metadata to ALL the output. Null output_metadata_buffer can also be handled by the metadata library */
           metadata_strict_transport(op_extra_data->input_metadata_buffer,op_extra_data->output_metadata_buffer, op_extra_data->frame_size*OCTETS_PER_SAMPLE);
           return;
       }
    }
    else
    {
        /* Create a new tag for the output */
        b4idx = 0;
        afteridx = op_extra_data->frame_size * OCTETS_PER_SAMPLE;
        mtag = buff_metadata_new_tag();
        if (mtag != NULL)
        {
            mtag->length = op_extra_data->frame_size * OCTETS_PER_SAMPLE;
        }
        else
        {
            L1_DBG_MSG("CVC_SEND : Failed to allocate metadata tag");
        }
    }
    
    if (mtag != NULL)
    {
       if(op_extra_data->Cur_mode == GEN_CVC_SEND_SYSMODE_FULL)
       {
           /* Extract reference timestamp */
           reference_time = mtag->timestamp;
           
           /* set delay for headset usecases */
           delay_in_samples = CVCSEND_HS_FBDELAY_IN_FRAMES*op_extra_data->frame_size;
            
           /* adjust the delay for speaker/auto usecases */
           if(op_extra_data->major_config!=CVC_SEND_CONFIG_HEADSET)
           {
              delay_in_samples = CVCSEND_SPKR_FBDELAY_IN_FRAMES*op_extra_data->frame_size;
           }
           
      #if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)  
           /* adjust the delay for cvc-hybrid usecases */
           if(op_extra_data->cap_id==CVCEB3MIC_HYBRID_WB_CAP_ID || op_extra_data->cap_id==CVCEB3MIC_HYBRID_SWB_CAP_ID ||
              op_extra_data->cap_id==CVCHS2MIC_MONO_SEND_HYBRID_WB_CAP_ID || op_extra_data->cap_id==CVCHS2MIC_MONO_SEND_HYBRID_SWB_CAP_ID ||
              op_extra_data->cap_id==CVCHS1MIC_SEND_HYBRID_WB_CAP_ID || op_extra_data->cap_id==CVCHS1MIC_SEND_HYBRID_SWB_CAP_ID)
           {
              mlfe100_data_t *mlfe_data = op_extra_data->mlfe_obj;
              model_5_pre_proc_data_t *pre_proc_data = (model_5_pre_proc_data_t*) mlfe_data->pre_proc_data;
              mldelay_frames = pre_proc_data->delay_struct->delay_in_frames;
              delay_in_samples = delay_in_samples + mldelay_frames*op_extra_data->frame_size;
           }
      #endif
           
           /* convert delay_in_samples to time */
           latency_time = (TIME)((delay_in_samples * 1000000ul) / op_extra_data->sample_rate);
           
           /* update timestamp */
           mtag->timestamp = time_add(reference_time,latency_time);
       }
         
       /* transfer tag to output buffer */
       buff_metadata_append(op_extra_data->output_metadata_buffer, mtag, b4idx, afteridx);
    }
}

/* ************************************* Data processing-related functions and wrappers **********************************/
void cvc_send_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    int samples_to_process, stream_amount_data;
    int amount_input_data[op_extra_data->num_mics + 1];
    int mic_index;
    unsigned frame_size = op_extra_data->frame_size;
    unsigned doubled_frame_size = frame_size * 2;
    unsigned sinks_not_having_enough_data_for_current_frame = 0;
    unsigned sinks_not_having_enough_data_for_next_frame = 0;

    patch_fn(cvc_send_process_data_patch);

    unsigned op_feature_requested = op_extra_data->op_feature_requested;

    /* Bypass processing until all streams are connected */
    if(op_feature_requested == 0)
    {
       return;
    }

    /* number of samples to process at the reference */
    if(op_feature_requested & CVC_REQUESTED_FEATURE_AEC)
    {
        samples_to_process = cbuffer_calc_amount_data_in_words(op_extra_data->input_stream[0]);   /* Reference */

        /* Issue backward kick from this sink to request more data if, at the exit point of this 
        * function, the data in the input buffer is insufficient for frame processing.
        */
        if(samples_to_process < frame_size)
        {
            sinks_not_having_enough_data_for_current_frame = TOUCHED_SINK_0;
        }
        else if (samples_to_process < doubled_frame_size)
        {
            sinks_not_having_enough_data_for_next_frame = TOUCHED_SINK_0;
        }

    }
    else
    {
       samples_to_process = frame_size;
    }
    /* number of samples to process at the mics */
    for(mic_index=1; mic_index <= op_extra_data->num_mics; mic_index++)
    {
        stream_amount_data = cbuffer_calc_amount_data_in_words(op_extra_data->input_stream[mic_index]);
        amount_input_data[mic_index] = stream_amount_data;
        if (stream_amount_data < samples_to_process)
        {
            samples_to_process = stream_amount_data;
        }
        
        /* Issue backward kick from this sink to request more data if, at the exit point of this 
        * function, the data in the input buffer is insufficient for frame processing.
        */
        if(stream_amount_data < frame_size)
        {
            sinks_not_having_enough_data_for_current_frame |= TOUCHED_SINK_0 << mic_index;
        }
        else if(stream_amount_data < doubled_frame_size)
        {
            sinks_not_having_enough_data_for_next_frame |= TOUCHED_SINK_0 << mic_index;
        }
    }

    patch_fn_shared(cvc_send_wrapper);

    if(op_extra_data->input_metadata_buffer!= NULL)
    {
        /* if mic inputs have metadata, then limit the amount of
        * consuming to the amount of metadata available.
        */
        unsigned meta_data_available =
           buff_metadata_available_octets(op_extra_data->input_metadata_buffer)/OCTETS_PER_SAMPLE;

        samples_to_process = MIN(samples_to_process, meta_data_available);
    }

    if(samples_to_process < frame_size)
    {
        /* Unable to trigger processing due to insufficient input data, 
         * kick backward to request more data */
        touched->sinks = sinks_not_having_enough_data_for_current_frame;
        return;
    }

     /* if outputs have metadata, also check the space available */
     /* Voice channel */
     if (NULL != op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VOICE] && buff_has_metadata(op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VOICE]))
     {
         unsigned space_available = cbuffer_calc_amount_space_in_words(op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VOICE]);
         samples_to_process = MIN(samples_to_process, space_available);
     }
     /* VA channel */
     if (NULL != op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VA] && buff_has_metadata(op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VA]))
     {
         unsigned space_available = cbuffer_calc_amount_space_in_words(op_extra_data->output_stream[CVC_SEND_OUT_TERMINAL_VA]);
         samples_to_process = MIN(samples_to_process, space_available);
     }

    /* Check for sufficient data and space */
    if(samples_to_process < frame_size)
    {
        /* Unable to trigger processing due to insufficient output space, 
         * return without kicking backward */
        return;
    }

    /* At this point, the data & space requirements of frame processing are satisfied, and
     * once the frame processing is done, request more data for next frame by kicking back */
    touched->sinks = sinks_not_having_enough_data_for_next_frame;

    /* Sometimes the data for one microphone can follow a different chain than the data for another microphone.
     * If the two chains aren't started at the exact same time then the data at the inputs of cvc will be out of sync.
     * In such cases we need to purge the input buffers and kick backwards
    */
    if(op_extra_data->enable_purge_till_inputs_synced == 1)
    {
        /* In 2-mic-cvc-ie-eb and 3-mic-cvc-ie-eb the in-ear mic is connected at terminal 2 and 3 respectively.
         * Therefore we can treat the "number of mics" as also being the terminal number of the in-ear mic.
         * In 1-mic-cvc diff_data_levels will always be zero (because your subtracting the mic buffer level from itself) so this
         * feature is efectivel bypassed.
        */
        unsigned in_ear_mic_terminal_number = op_extra_data->num_mics;

        int diff_data_levels;
        diff_data_levels = ABS(amount_input_data[CVC_SEND_IN_TERMINAL_MIC1] - amount_input_data[in_ear_mic_terminal_number]);

        /* If there's more than a frame of data between the Primary and in-ear mic then we purge all the input buffers
         * so that the lagging input chain has a chance to catch up.
        */
        if(diff_data_levels > op_extra_data->frame_size)
        {
            int i;
            for(i=0; i <= op_extra_data->num_mics; i++)
            {
                cbuffer_empty_buffer_and_metadata(op_extra_data->input_stream[i]);
                /* Kick all input terminals after emptying them */
                touched->sinks |= TOUCHED_SINK_0 << i;
            }
            return;
        }
    }

    if (op_extra_data->Ovr_Control & GEN_CVC_SEND_CONTROL_MODE_OVERRIDE)
    {
        op_extra_data->Cur_mode = op_extra_data->Obpm_mode;
    }
    else if (op_extra_data->major_config!=CVC_SEND_CONFIG_HEADSET)
    {
        op_extra_data->Cur_mode = op_extra_data->Host_mode;
    }
    else if (op_extra_data->Host_mode != GEN_CVC_SEND_SYSMODE_FULL)
    {
        op_extra_data->Cur_mode = op_extra_data->Host_mode;
    }
    else
    {
        unsigned temp = op_extra_data->Cur_mode;
        if ((temp == GEN_CVC_SEND_SYSMODE_FULL) || (temp == GEN_CVC_SEND_SYSMODE_LOWVOLUME) )
        {
#ifndef ACCESS_SHARE_MEMORY_THROUGH_PATCH
           /* TODO - need to redefine OFFSET_LVMODE_THRES to dB/60 */
            unsigned vol_level = 15 - (((int)op_extra_data->shared_volume_ptr->current_volume_level)/(-360));
#else
           unsigned vol_level = 15 - (((int)vol_ctrl_low_event_get())/(-360));
           L3_DBG_MSG1("Current Volume level inside cvc_send %d",vol_ctrl_low_event_get());


#endif
            if (vol_level < op_extra_data->params->OFFSET_LVMODE_THRES)
            {
               op_extra_data->Cur_mode = GEN_CVC_SEND_SYSMODE_LOWVOLUME;
            }
            else
            {
               op_extra_data->Cur_mode = GEN_CVC_SEND_SYSMODE_FULL;
            }

            if (temp != op_extra_data->Cur_mode)
            {
               op_extra_data->ReInitFlag = 1;
            }
        }
        else
        {
            op_extra_data->Cur_mode = op_extra_data->Host_mode;
        }
    }

    patch_fn_shared(cvc_send_wrapper);
    
    cvc_send_transport_metadata(op_extra_data);

    /* call the "process" assembly function */
    CVC_SEND_CAP_Process(op_extra_data);

    /* Set touched->sources for forward kicking */
    if(op_feature_requested & CVC_REQUESTED_FEATURE_VOICE)
    {
        touched->sources = TOUCHED_SOURCE_0;
    }

    if(op_feature_requested & CVC_REQUESTED_FEATURE_VA)
    {
        /* in order to kick all the connected VA channels, we need to assign a binary number
        such as 11110 (for 4 VA channels) or 10 (for 1 VA channel) to the touched->sources.

        The 0 on the last bit (least significant bit) of the binary number represents the voice channel
        (1st output channel) whose kicking logic should be determined separately. */

        touched->sources |= (TOUCHED_SOURCE_1 << (op_extra_data->num_va_outputs)) - TOUCHED_SOURCE_1;
    }
}


/* **************************** Operator message handlers ******************************** */



bool cvc_send_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned           i,num_controls,cntrl_value;
    CPS_CONTROL_SOURCE cntrl_src;
    OPMSG_RESULT_STATES result = OPMSG_RESULT_STATES_NORMAL_STATE;

    patch_fn(cvc_send_opmsg_obpm_set_control_patch);

    if(!cps_control_setup(message_data, resp_length, resp_data,&num_controls))
    {
       return FALSE;
    }

    for(i=0;i<num_controls;i++)
    {
        unsigned  cntrl_id=cps_control_get(message_data,i,&cntrl_value,&cntrl_src);
        /* Only interested in lower 8-bits of value */
        cntrl_value &= 0xFF;

        if (cntrl_id == OPMSG_CONTROL_MODE_ID)
        {
            /* Control is Mode */
            if (cntrl_value >= GEN_CVC_SEND_SYSMODE_MAX_MODES)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }

            if(cntrl_src == CPS_SOURCE_HOST)
            {
               op_extra_data->Host_mode = cntrl_value;
            }
            else
            {
                op_extra_data->Obpm_mode = cntrl_value;

                /* When the override bit in the control id is high, then we override the
                 * OBPM's ability to override the control value. In other words we let the control
                 * value be reset to the host's control value when the OPMSG_CONTROL_OBPM_OVERRIDE
                 * bit is high in the control id.*/
                if (cntrl_src == CPS_SOURCE_OBPM_DISABLE)
                {
                    op_extra_data->Ovr_Control &= ~GEN_CVC_SEND_CONTROL_MODE_OVERRIDE;
                }
                else
                {
                    op_extra_data->Ovr_Control |= GEN_CVC_SEND_CONTROL_MODE_OVERRIDE;
                }
            }
            op_extra_data->ReInitFlag = 1;
        }
        else if (cntrl_id == OPMSG_CONTROL_MUTE_ID)
        {
            if (cntrl_value > 1)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }
            /* Control is Mute */
            if(cntrl_src == CPS_SOURCE_HOST)
            {
               op_extra_data->host_mute = cntrl_value;
            }
            else
            {
                op_extra_data->obpm_mute = cntrl_value;

                /* When the override bit in the control id is high, then we override the
                 * OBPM's ability to override the control value. In other words we let the control
                 * value be reset to the host's control value when the OPMSG_CONTROL_OBPM_OVERRIDE
                 * bit is high in the control id.*/
                if(cntrl_src == CPS_SOURCE_OBPM_DISABLE)
                {
                    op_extra_data->Ovr_Control &= ~GEN_CVC_SEND_CONTROL_MUTE_OVERRIDE;
                }
                else
                {
                    op_extra_data->Ovr_Control |= GEN_CVC_SEND_CONTROL_MUTE_OVERRIDE;
                }
            }
            op_extra_data->Cur_Mute = ( op_extra_data->Ovr_Control & GEN_CVC_SEND_CONTROL_MUTE_OVERRIDE) ? op_extra_data->obpm_mute : op_extra_data->host_mute;
        }
        else if (cntrl_id == OPMSG_CONTROL_OMNI_ID)
        {
            if(op_extra_data->omni_mode_ptr == NULL)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }
            if (cntrl_value > 1)
            {
                result = OPMSG_RESULT_STATES_INVALID_CONTROL_VALUE;
                break;
            }
            /* Control is Mute */
            if(cntrl_src == CPS_SOURCE_HOST)
            {
                op_extra_data->host_omni = cntrl_value;
            }
            else
            {
                op_extra_data->obpm_omni = cntrl_value;

                /* When the override bit in the control id is high, then we override the
                 * OBPM's ability to override the control value. In other words we let the control
                 * value be reset to the host's control value when the OPMSG_CONTROL_OBPM_OVERRIDE
                 * bit is high in the control id.*/
                if(cntrl_src == CPS_SOURCE_OBPM_DISABLE)
                {
                    op_extra_data->Ovr_Control &= ~GEN_CVC_SEND_CONTROL_OMNI_OVERRIDE;
                }
                else
                {
                    op_extra_data->Ovr_Control |= GEN_CVC_SEND_CONTROL_OMNI_OVERRIDE;
                }
            }
            *(op_extra_data->omni_mode_ptr) = ( op_extra_data->Ovr_Control & GEN_CVC_SEND_CONTROL_OMNI_OVERRIDE) ? op_extra_data->obpm_omni : op_extra_data->host_omni;
        }
        else
        {
            result = OPMSG_RESULT_STATES_UNSUPPORTED_CONTROL;
            break;
        }
    }

    cps_response_set_result(resp_data,result);

    return TRUE;
}

bool cvc_send_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

    return cpsGetParameterMsgHandler(&op_extra_data->parms_def ,message_data, resp_length,resp_data);
}

bool cvc_send_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

    return cpsGetDefaultsMsgHandler(&op_extra_data->parms_def ,message_data, resp_length,resp_data);
}

bool cvc_send_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned int_mode_saved;
    int_mode_saved = op_extra_data->params->OFFSET_INT_MODE;
    bool retval;
    OPMSG_SET_PARAM_MSG *obpm_msg = (OPMSG_SET_PARAM_MSG *) message_data;
    

    patch_fn(cvc_send_opmsg_obpm_set_params_patch);

    retval = cpsSetParameterMsgHandler(&op_extra_data->parms_def ,message_data, resp_length,resp_data);

    if ((obpm_msg->data_block->num_params == 1) && (obpm_msg->num_blocks == 1) && (int_mode_saved != op_extra_data->params->OFFSET_INT_MODE))
    {
        /* Set the Reinit flag after setting the paramters */
        op_extra_data->ReInitFlag = 0;
    }
    else
    {
       /* Set the Reinit flag after setting the paramters */
        op_extra_data->ReInitFlag = 1;
    }   

    return retval;
}

unsigned cvc_send_combine_status_flags(unsigned **flags)
{
    unsigned combined_bitflag = 0;

    // Self Clean Flag 
    if (*flags[SELFCLEAN_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_SELFCLEAN_DETECTED;
    // Mic Malfunction Flag 
    if (*flags[MIC_MALFUNC_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_MIC_MALFUNC_FLAG;
    // Loose Fit Flag 
    if (*flags[LOOSE_FIT_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_LOOSE_FIT_FLAG;
    // Three Mic Flag 
    if (*flags[THREE_MIC_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_THREE_MIC_FLAG;
    // Ref Power Flag 
    if (*flags[REF_PWR_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_REF_POWER_FLAG;
    // VAD Flag 
    if (*flags[VAD_FLAG_POS]) combined_bitflag |= GEN_CVC_SEND_CVC_STATUS_BITFLAG_SND_VAD_FLAG;

    return combined_bitflag;
}

bool cvc_send_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    unsigned  *resp;
    unsigned **stats = (unsigned**)op_extra_data->status_table;


    patch_fn(cvc_send_opmsg_obpm_get_status_patch);

    if(!common_obpm_status_helper(message_data,resp_length,resp_data,sizeof(CVC_SEND_STATISTICS),&resp))
    {
         return FALSE;
    }

    if(resp)
    {
        resp = cpsPack2Words(op_extra_data->Cur_mode, op_extra_data->Ovr_Control, resp);
        resp = cpsPack2Words(*stats[0], *stats[1], resp);
        resp = cpsPack2Words(*stats[2], *stats[3], resp);
        /* Reset IN/OUT Peak Detectors*/
        *(stats[1])=0;
        *(stats[2])=0;
        *(stats[3])=0;
        resp = cpsPack2Words(*stats[4], *stats[5], resp);
        resp = cpsPack2Words(*stats[6], *stats[7], resp);
        resp = cpsPack2Words(*stats[8], *stats[9], resp);
        resp = cpsPack2Words(*stats[10], *stats[11], resp);
        resp = cpsPack2Words(*op_extra_data->mute_control_ptr, *stats[13], resp);
        resp = cpsPack2Words(*stats[14], *stats[15], resp);
        resp = cpsPack2Words(*stats[16], *stats[17], resp);
        resp = cpsPack2Words(*stats[18], *stats[19], resp);
        resp = cpsPack2Words(*stats[20] , *stats[21], resp);
        resp = cpsPack2Words(*stats[22], cvc_send_combine_status_flags((unsigned**)stats[23]), resp);
        resp = cpsPack2Words(*stats[24] , *stats[25], resp);
        resp = cpsPack1Word(*stats[26], resp);

        /* Reset Peak Detectors AEC_REF/MIC3/MIC4/VA1 */
        *(stats[14])=0;
        *(stats[15])=0;
        *(stats[16])=0;
        *(stats[19])=0;
    }

    return TRUE;
}

bool ups_params_snd(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank,
                 uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data((OPERATOR_DATA*)instance_data);

    cpsSetParameterFromPsStore(&op_extra_data->parms_def,length,data,status);

    /* Set the Reinit flag after setting the paramters */
    op_extra_data->ReInitFlag = 1;

    return(TRUE);
}

bool cvc_send_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    PS_KEY_TYPE key;
    bool retval;

    retval = cpsSetUcidMsgHandler(&op_extra_data->parms_def,message_data,resp_length,resp_data);

    key = MAP_CAPID_UCID_SBID_TO_PSKEYID(op_extra_data->cap_id,op_extra_data->parms_def.ucid,OPMSG_P_STORE_PARAMETER_SUB_ID);
    ps_entry_read((void*)op_data,key,PERSIST_ANY,ups_params_snd);

    return retval;
}

bool cvc_send_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

    return cpsGetUcidMsgHandler(&op_extra_data->parms_def,op_extra_data->cap_id,message_data,resp_length,resp_data);
}

bool cvc_send_opmsg_get_voice_quality(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

    /* call the assembly function */
    unsigned metric = compute_voice_quality_metric(op_extra_data);

    /* CVC_SEND_VOICE_QUALITY_METRIC_ERROR_CODE is the error code indicating
    either mode is not supported or the DMS object is bypassed */
    if(metric != CVC_SEND_VOICE_QUALITY_METRIC_ERROR_CODE){
       *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(1);

      /* raw data pointer */
       unsigned* resp = xzppmalloc((*resp_length)* sizeof(unsigned), MALLOC_PREFERENCE_NONE);
       if (!resp){
          /* return if memory allocation fails */
          return FALSE;
       }

       /* set operator message ID in the raw data */
       resp[0] = OPMSG_COMMON_GET_VOICE_QUALITY;
       resp[1] = metric;
       /* assign the raw data pointer *resp to the **resp_data */
       *resp_data = (OP_OPMSG_RSP_PAYLOAD*)resp;
    }

    return TRUE;
}

#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID) 
/**
 * \brief Loads the MLE model file
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool cvc_mlfe_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    /* Model cannot be loaded if the operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("MLE: Cannot load model while operator is running");
        return FALSE;
    }

    L2_DBG_MSG("CVC_HYBRID: Load model");
    unsigned usecase_id  = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, USECASE_ID);
    unsigned batch_reset_count = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, BATCH_RESET_COUNT);
    unsigned file_handle = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, FILE_HANDLE);
    unsigned access_method = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_LOAD_MODEL, ACCESS_METHOD);

    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = (ML_ENGINE_OP_DATA*)op_extra_data->mlfe_obj->ml_engine_container;

    L2_DBG_MSG4("CVC_HYBRID: ID:%d, batch_cnt:%d, fhandle:%d, access:%d", usecase_id, batch_reset_count, file_handle, access_method);
    L2_DBG_MSG1("CVC_HYBRID: ml_engine_container_ptr:%d",ml_engine_data);
    /* Call helper function to load the model */
    return ml_engine_load(ml_engine_data, usecase_id, batch_reset_count, file_handle, access_method);
    }

/**
 * \brief Unloads the MLE model file and releases associated memory
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool cvc_mlfe_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    L2_DBG_MSG("CVC_HYBRID: Unload model");
    /* Model cannot be unloaded if the operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("CVC_HYBRID: Cannot unload model while operator is running");
        return FALSE;
    }

    /* get the usecase_id of the model to be unloaded from the opmsg */
    unsigned usecase_id = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_UNLOAD_MODEL, USECASE_ID);
    unsigned file_id    = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_UNLOAD_MODEL, FILE_ID);
    /* unload the model file - this will direct the file_mgr to release the file handle */
    return ml_unload(usecase_id, file_id);
}

/**
 * \brief Activates the MLE model file associated with the usecase_id
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the destroy request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
bool cvc_mlfe_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
     L2_DBG_MSG("CVC_HYBRID: activate model");
    /* currently not supported while operator is running */
    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("CVC_HYBRID: Cannot activate model while operator is running  \n");
        return FALSE;
    }
    
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);
    ML_ENGINE_OP_DATA *ml_engine_data = (ML_ENGINE_OP_DATA*)op_extra_data->mlfe_obj->ml_engine_container;
    /* get the usecase_id from the operator message */
    unsigned usecase_id = OPMSG_FIELD_GET(message_data, OPMSG_ML_ENGINE_ACTIVATE_MODEL, USECASE_ID);

    L2_DBG_MSG1("CVC_HYBRID: Activate Usecase %d", usecase_id);
    /* Call helper function to activate the model */
    if (!ml_engine_activate(ml_engine_data, usecase_id))
    {  
         L2_DBG_MSG("CVC_HYBRID: model Activation failed!!!");

        return FALSE;
    }
    L2_DBG_MSG("CVC_HYBRID: model activated sucessfully");

#if 0 //debug messages
    USECASE_INFO *usecase_info = (USECASE_INFO *)ml_engine_list_find(ml_engine_data->use_cases,(uint16)ml_engine_data->uc_id);
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->input_tensor.tensors[i];
        L2_DBG_MSG2("Input Tensor[%d] Data  pts: %d",i , tensor->data);
        
    }
    for(int i=0; i<usecase_info->input_tensor.num_tensors; i++)
    {
        MODEL_TENSOR_INFO *tensor = &usecase_info->output_tensor.tensors[i];
        L2_DBG_MSG2("Output Tensor[%d] Data  pts: %d",i , tensor->data);
        
    }
#endif
   /* Connect the pre and post processing directly to the input and output tensors */
    L2_DBG_MSG("CVC_HYBRID: init mlfe");
    mlfe_initialize(op_extra_data->mlfe_obj);
     L2_DBG_MSG("CVC_HYBRID:end activate message handler");
    return TRUE;
}

#endif


/*******************************************************************************
   cVc send resampler utilities
*******************************************************************************/
void* cvc_resampler_instance_open(unsigned *sampler);
void* cvc_resampler_instance_open(unsigned *sampler)
{
   frame_iir_resampler_op *resampler = xzppmalloc(sizeof(frame_iir_resampler_op) + (IIR_RESAMPLEV2_IIR_BUFFER_SIZE+IIR_RESAMPLEV2_FIR_BUFFER_SIZE)*sizeof(unsigned), MALLOC_PREFERENCE_NONE);
   resampler->common.filter = sampler;
   resampler->common.input_scale = -IIR_RESAMPLEV2_IO_SCALE_FACTOR;
   resampler->common.output_scale = IIR_RESAMPLEV2_IO_SCALE_FACTOR;
   return resampler;
}

void cvc_resampler_instance_close(void *resampler_instance, unsigned release_config);
void cvc_resampler_instance_close(void *resampler_instance, unsigned release_config)
{
    frame_iir_resampler_op *resampler = resampler_instance;

    if (release_config) {
       iir_resamplerv2_release_config(resampler->common.filter);
    }

    pfree(resampler);
}

void cvc_resampler_config_io(frame_iir_resampler_op *resampler, unsigned *input_frame, unsigned *output_frame);
void cvc_resampler_config_io(frame_iir_resampler_op *resampler, unsigned *input_frame, unsigned *output_frame)
{
   resampler->input_frame_ptr = input_frame;
   resampler->output_frame_ptr = output_frame;
}

void cvc_send_resampler_create(CVC_SEND_OP_DATA *op_extra_data)
{
    unsigned *downsampler, *upsampler;

    if ((op_extra_data->mic_config & MIC_CONFIG_INEAR) && (op_extra_data->sample_rate >= 32000))
    {
       iir_resamplerv2_add_config_to_list(NULL);

       downsampler = iir_resamplerv2_allocate_config_by_rate(op_extra_data->sample_rate, op_extra_data->sample_rate/2, 1);
       upsampler   = iir_resamplerv2_allocate_config_by_rate(op_extra_data->sample_rate/2, op_extra_data->sample_rate, 1);

       op_extra_data->ref_downsampler = cvc_resampler_instance_open(downsampler);
       op_extra_data->im_downsampler  = cvc_resampler_instance_open(downsampler);
       op_extra_data->im_upsampler    = cvc_resampler_instance_open(upsampler);
    }                                                                                       
}

void cvc_send_resampler_destroy(CVC_SEND_OP_DATA *op_extra_data)
{
    if ((op_extra_data->mic_config & MIC_CONFIG_INEAR) && (op_extra_data->sample_rate >= 32000))
    {
       cvc_resampler_instance_close(op_extra_data->im_upsampler, 1);
       cvc_resampler_instance_close(op_extra_data->im_downsampler, 1);
       cvc_resampler_instance_close(op_extra_data->ref_downsampler, 0);
       iir_resamplerv2_delete_config_list();
    }                                                                                       
}

/*******************************************************************************
   cVc send mbdrc utilities
*******************************************************************************/
bool cvc_send_mbdrc_create(CVC_SEND_OP_DATA *op_extra_data)
{
    mbdrc_static_struct_t mbdrc_cfg;
    int8_t *mbdrc_mem = NULL;

    mbdrc_cfg.sample_rate = op_extra_data->sample_rate;
    mbdrc_cfg.framesize = op_extra_data->frame_size;

    switch(op_extra_data->data_variant)
    {
            case DATA_VARIANT_SWB:  // 32 kHz
                mbdrc_cfg.nfft  = (DATA_VARIANT_SWB_NBINS - 1)<<1; //(257-1)*2
                break;
            case DATA_VARIANT_WB:  // 16 kHz
                mbdrc_cfg.nfft  = (DATA_VARIANT_WB_NBINS - 1)<<1; //(129-1)*2
                break;
            default: // only 32k, 16K and 8K are supported
                mbdrc_cfg.nfft  = (DATA_VARIANT_NB_NBINS - 1)<<1; //(65-1)*2
                break;
    }

    L2_DBG_MSG3("sample_rate: %d, framesize: %d, nfft: %d", mbdrc_cfg.sample_rate,mbdrc_cfg.framesize,mbdrc_cfg.nfft);
    /* MBDRC memory requirements */
    MBDRC_RESULT rc = mbdrc_get_mem_req(&(op_extra_data->mbdrc_obj->mbdrc_mem_req), &mbdrc_cfg);
    if(MBDRC_SUCCESS != rc) return (FALSE);

    /* allocate a block on the heap for library memory */
    mbdrc_mem = (int8_t*)xzpmalloc(op_extra_data->mbdrc_obj->mbdrc_mem_req.lib_memory_size);
    if (mbdrc_mem == NULL)
    {
        return(FALSE);
    }
    op_extra_data->mbdrc_obj->mbdrc_lib.lib_mem_ptr = mbdrc_mem;

    L2_DBG_MSG1("mbdrc_init_memory: op_extra_data->mbdrc_lib.lib_mem_ptr at %p", op_extra_data->mbdrc_obj->mbdrc_lib.lib_mem_ptr);

    rc = mbdrc_init_memory(&op_extra_data->mbdrc_obj->mbdrc_lib,
        &mbdrc_cfg,
        mbdrc_mem,
        op_extra_data->mbdrc_obj->mbdrc_mem_req.lib_memory_size
    );

    return (TRUE);
}

bool cvc_send_opmsg_set_purge_till_inputs_synced(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    CVC_SEND_OP_DATA *op_extra_data = get_instance_data(op_data);

    if (opmgr_op_is_running(op_data))
    {
        L0_DBG_MSG("CVC_SEND: Cannot change enable_purge_till_inputs_synced because the operator is running!");
        return FALSE;
    }

    op_extra_data->enable_purge_till_inputs_synced = OPMSG_FIELD_GET(message_data,
                                        OPMSG_CVC_SEND_ENABLE_PURGE_TILL_INPUTS_SYNCED, ENABLE_PURGE);
    return TRUE;
}
