/**
* Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
 * \file cvc_send_cap_c.h
 */

#ifndef CVC_SEND_CAP_C_H
#define CVC_SEND_CAP_C_H

#include "cvc_send_gen_c.h"
#include "cvc_send.h"
#include "../../../lib_private/cvc_modules/mbdrc100/mbdrc_api_public.h"

#include "opmgr/opmgr_for_ops.h"
#include "volume/shared_volume_control.h"
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
   #include "mlfe100_library_c.h"
#endif

/* Capability   Version */
#define CVC_SEND_CAP_VERSION_MINOR                          0

#define CVC_SEND_MAX_NUM_INPUT                              5
#define CVC_SEND_MAX_NUM_OUTPUT                             5
#define CVCSEND_HS_FBDELAY_IN_FRAMES                        1
#define CVCSEND_SPKR_FBDELAY_IN_FRAMES                      3

/* switch to turn on/off the VA feature of the standard CVC capabilities */
#if !defined(CVC_VA_DISABLE)
    #define CVC_NUM_OUTPUTS     2
#else
    #define CVC_NUM_OUTPUTS     1
#endif

typedef struct
{
    unsigned *inst_alloc;                       /**< Pointer to allocated persistent memory */
    unsigned *scratch_alloc;                    /**< Pointer to allocated scratch memory */

    tCbuffer *input_stream[CVC_SEND_MAX_NUM_INPUT];     /**< Pointer to input Frame Sync Stream Maps */
    tCbuffer *output_stream[CVC_SEND_MAX_NUM_OUTPUT];   /**< Pointer to output Frame Sync Stream Map */
    unsigned *cvc_input_stream[CVC_SEND_MAX_NUM_INPUT];     /**< Pointer to internal cVc input stream */
    unsigned *cvc_output_stream[CVC_SEND_MAX_NUM_OUTPUT];   /**< Pointer to internal cVc output stream */

    unsigned *mode_table;                       /**< Pointer to mode table */
    unsigned *init_table;                       /**< Pointer to initialization table */
    unsigned *cvc_data;                         /**< Pointer to cvc data root table */
    unsigned *status_table;                     /**< Pointer to status table */
    CVC_SEND_PARAMETERS *params;                /**< Pointer to params */

    unsigned *cvclib_table;                     /**< Pointer to cvclib table */
    unsigned *fftsplit_table;                   /**< Pointer to fft split table */
    unsigned *oms_config_ptr;                   /**< Pointer to OMS config pointer */
    unsigned *dms200_mode;                      /**< Pointer to dms200 mode object */
    unsigned *asf_mode_table;                   /**< Pointer to asf100 mode object */
    unsigned *wnr_mode_table;                   /**< Pointer to asf100 wnr mode object */
    unsigned *vad_dc_coeffs;                    /**< Pointer to PEQ coeffs for VAD and dc blocker */

    unsigned *aec_mode;                         /**< Pointer to AEC nmode object */
    unsigned *filterbank_config;                /**< Pointer to filterbank config object */    

    /* Operator Shared Data for Volume */
    SHARED_VOLUME_CONTROL_OBJ *shared_volume_ptr;                   /**< pointer to NDVC noise level shared object*/

    /* State Information */
    unsigned mdgc_gain;
    unsigned mdgc_gain_ptr;

    /* additionally used fields */
    unsigned op_feature_requested;
    unsigned asf_wnr_available;    
    unsigned cap_id;
    unsigned *cap_root_ptr;
    unsigned Host_mode;
    unsigned Obpm_mode;
    unsigned Ovr_Control;
    unsigned ReInitFlag;

    unsigned *scratch_buffer;

    /* start consecutive block */
    unsigned Cur_mode;
    unsigned data_variant;
    unsigned major_config;
    unsigned num_mics;  
    unsigned mic_config;
    unsigned *cap_data;
    unsigned *dyn_main;
    unsigned *dyn_linker;
    /* end consecutive block */

    unsigned *mute_control_ptr;
    unsigned host_mute;
    unsigned obpm_mute;
    unsigned Cur_Mute;

    unsigned frame_size;
    unsigned sample_rate;
    unsigned num_va_outputs;
    unsigned host_omni;
    unsigned obpm_omni;
    unsigned *omni_mode_ptr;
    unsigned *tmm_obj;

    unsigned debug_reserve;

    unsigned secure_key[2];
    CPS_PARAM_DEF parms_def;
    unsigned* ptr_map_ratio;   
    unsigned mips_array;
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
    mlfe100_data_t *mlfe_obj;   
#else
    unsigned mlfe_reserve;
#endif
    unsigned *hr_filterbank_config;                /* Pointer to High Resolution filterbank config object */    
    unsigned *ref_downsampler;                     /* Pointer to ref-delay downsampler object */    
    unsigned *im_downsampler;                      /* Pointer to inear-mic downsampler object */    
    unsigned *im_upsampler;                        /* Pointer to inear-mic upsampler object */    
    mbdrc100_object_t *mbdrc_obj;
    tCbuffer *input_metadata_buffer;
    tCbuffer *output_metadata_buffer;
    unsigned enable_purge_till_inputs_synced;
    /* reserved for debugging or possible patching */
    unsigned reserved[3];
}CVC_SEND_OP_DATA;
#define DATA_VARIANT_NB_NBINS                65
#define DATA_VARIANT_WB_NBINS                129
#define DATA_VARIANT_SWB_NBINS               257
/* UCID to make a unique key for the EQMAP vector, this is needed because SBID is only 1 bit
   This leaves room for 31 unique UCIDs */
#define UCID_EQMAP                           32

#define DATA_VARIANT_NB                      0
#define DATA_VARIANT_WB                      1
#define DATA_VARIANT_UWB                     2
#define DATA_VARIANT_SWB                     3
#define DATA_VARIANT_FB                      4
#define NUM_DATA_VARIANTS                    5

#define CVC_SEND_CONFIG_HEADSET              0
#define CVC_SEND_CONFIG_SPEAKER              1
#define CVC_SEND_CONFIG_AUTO                 2

#define MIC_CONFIG_DEFAULT                   0
#define MIC_CONFIG_ENDFIRE                   1
#define MIC_CONFIG_BROADSIDE                 2
#define MIC_CONFIG_CIRCULAR                  4
#define MIC_CONFIG_INEAR                     8
#define MIC_CONFIG_AI                        16

#define CVC_REQUESTED_FEATURE_VOICE          0x1
#define CVC_REQUESTED_FEATURE_AEC            0x2
#define CVC_REQUESTED_FEATURE_VA             0x4

/* Bit position for consolidated CVC statistic bitflag */
#define SELFCLEAN_FLAG_POS                   0
#define MIC_MALFUNC_FLAG_POS                 1
#define LOOSE_FIT_FLAG_POS                   2
#define THREE_MIC_FLAG_POS                   3
#define REF_PWR_FLAG_POS                     4
#define VAD_FLAG_POS                         5

/****************************************************************************
Private Function Definitions
*/

/* ASM configuration functions per capability */
extern void CVC_SEND_CAP_Config_auto_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_auto_2mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_1mic_va(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_2mic_mono(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_2mic_wakeon(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_2mic_bargein(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_2mic_binaural(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_speaker_1mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_speaker_2mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_speaker_3mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_speaker_4mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_spkr_circ_3mic(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_spkr_circ_3mic_va(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_spkr_circ_3mic_va4b(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_earbud_2mic_ie(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_earbud_3mic_ie(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_earbud_3mic_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_2mic_mono_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
extern void CVC_SEND_CAP_Config_headset_1mic_hybrid(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);


/* ASM processing function */
extern unsigned CVC_SEND_CAP_Create(CVC_SEND_OP_DATA *op_extra_data);
extern void CVC_SEND_CAP_Destroy(CVC_SEND_OP_DATA *op_extra_data);
extern void CVC_SEND_CAP_Process(CVC_SEND_OP_DATA *op_extra_data);
extern unsigned compute_voice_quality_metric(CVC_SEND_OP_DATA *op_extra_data);

/* Send capability handler functions declarations */
extern bool cvc_send_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool cvc_send_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool cvc_send_connect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool cvc_send_disconnect(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool cvc_send_buffer_details(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
extern bool cvc_send_get_sched_info(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);

/* Data processing function */
extern void cvc_send_process_data(OPERATOR_DATA*, TOUCHED_TERMINALS*);

/* metadata transfer function */
extern void cvc_send_transport_metadata(CVC_SEND_OP_DATA   *op_extra_data);

/* Operator message handlers */
extern bool cvc_send_opmsg_obpm_set_control(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_obpm_get_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_obpm_get_defaults(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_obpm_set_params(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_obpm_get_status(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

extern bool cvc_send_opmsg_set_ucid(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_get_ps_id(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_send_opmsg_get_voice_quality(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#if defined(INSTALL_OPERATOR_CVC_EARBUD_3MIC_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_2MIC_MONO_HYBRID) || defined(INSTALL_OPERATOR_CVC_HEADSET_1MIC_HYBRID)
extern bool cvc_mlfe_opmsg_load_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_mlfe_opmsg_unload_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
extern bool cvc_mlfe_opmsg_activate_model(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
#endif

extern void set_input_latency(CVC_SEND_OP_DATA *op_extra_data);
extern bool cvc_send_ups_set_state(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, STATUS_KYMERA status, uint16 extra_status_info);
extern bool cvc_send_ups_set_state_eqmap(void* instance_data, PS_KEY_TYPE key, PERSISTENCE_RANK rank, STATUS_KYMERA status, uint16 extra_status_info);
extern bool ups_state_snd(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank, uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info);
extern bool ups_state_snd_eqmap(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank, uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info);
extern void cvc_send_set_requested_features(CVC_SEND_OP_DATA *op_extra_data);
extern bool ups_params_snd(void* instance_data,PS_KEY_TYPE key,PERSISTENCE_RANK rank, uint16 length, unsigned* data, STATUS_KYMERA status,uint16 extra_status_info);
extern void cvc_send_release_constants(OPERATOR_DATA *op_data);
bool cvc_send_opmsg_set_purge_till_inputs_synced(OPERATOR_DATA *op_data, void *message_data, unsigned *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

#if defined(INSTALL_OPERATOR_CREATE_PENDING) && defined(INSTALL_CAPABILITY_CONSTANT_EXPORT)
void cvc_send_create_pending_cb(OPERATOR_DATA *op_data, uint16 cmd_id,void *msg_body, pendingContext *context,unsigned cb_value);
#endif

/* cVc configuration */
typedef void (*cvc_send_config_function_type)(CVC_SEND_OP_DATA *op_extra_data, unsigned data_variant);
typedef struct cvc_send_config_data
{
    cvc_send_config_function_type config_func;
    unsigned cap_ids[NUM_DATA_VARIANTS];
} cvc_send_config_data;
extern bool cvc_send_config(CVC_SEND_OP_DATA *op_extra_data, const cvc_send_config_data *cvc_send_caps_table);

extern void cvc_send_metadata_transport_voice_and_va(CVC_SEND_OP_DATA *op_extra_data);

/* Consolidate cVc status flags into single bitflag */
extern unsigned cvc_send_combine_status_flags(unsigned **flags);

/* cVc send resampler utilities */
extern void cvc_send_resampler_create(CVC_SEND_OP_DATA *op_extra_data);
extern void cvc_send_resampler_destroy(CVC_SEND_OP_DATA *op_extra_data);

/* cVc send mbdrc utilities */
extern bool cvc_send_mbdrc_create(CVC_SEND_OP_DATA *op_extra_data);

#endif  /* CVC_SEND_CAP_C_H */
