/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  va_pryon_lite.c
 * \ingroup  capabilities
 *
 *  A Stub implementation of a Capability that can be built and communicated
 *  with. This is provided to accelerate the development of new capabilities.
 *
 */

#include <string.h>
#include "capabilities.h"
#include "op_msg_helpers.h"
#include "obpm_prim.h"
#include "accmd_prim.h"
#include "apva.h"
#include "apva_struct.h"
#include "ttp/ttp.h"
#include "ttp_utilities.h"
#include "file_mgr_for_ops.h"
#include "opmgr/opmgr_for_ops.h"
#include "cap_id_prim.h"
#include "aov_interface/aov_interface.h"
#include "pryon_lite_PRL1000.h"

#include "wwe/wwe_cap.h"

#if defined(RUNNING_ON_KALSIM)
#include "apva_default_model.h"
#endif

#if (APVA_SYSMODE_STATIC != WWE_SYSMODE_STATIC)
#error APVA SYSMODE STATIC != WWE SYSMODE STATIC
#endif

#if (APVA_SYSMODE_FULL_PROC != WWE_SYSMODE_FULL_PROC)
#error APVA SYSMODE FULL PROC != WWE SYSMODE FULL PROC
#endif

#if (APVA_SYSMODE_PASS_THRU!= WWE_SYSMODE_PASS_THRU)
#error APVA SYSMODE PASS THRU != WWE SYSMODE PASS THRU
#endif

#if (APVA_CONTROL_MODE_OVERRIDE != WWE_CONTROL_MODE_OVERRIDE)
#error APVA CONTROL MODE OVERRIDE != WWE CONTROL MODE OVERRIDE
#endif

#define USE_VADx
#define LOW_LATENCYx

#ifdef CAPABILITY_DOWNLOAD_BUILD
#define APVA_CAP_ID CAP_ID_DOWNLOAD_APVA
#else
#define APVA_CAP_ID CAP_ID_APVA
#endif /* CAPABILITY_DOWNLOAD_BUILD */

#ifndef RUNNING_ON_KALSIM
    #pragma unitsuppress ExplicitQualifier
#endif

#define APVA_METADATA_MAX_PART_SIZE 100 // In bytes
#define APVA_GET_METADATA_MESSAGE_RESP_LENGTH ((APVA_METADATA_MAX_PART_SIZE/2) + 2)

/****************************************************************************
Private Function Definitions
*/
static void va_pryon_lite_result_callback(PryonLiteWakewordHandle handle,
                                          const PryonLiteWakewordResult* result);
#ifdef USE_VAD
static void va_pryon_lite_vad_callback(PryonLiteV2Handle* handle,
                                       const PryonLiteVadEvent* event);
#endif

static bool va_pryon_lite_create(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *response_id, void **resp_data);
static bool va_pryon_lite_destroy(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *response_id, void **resp_data);
static bool va_pryon_lite_process_data(OPERATOR_DATA *op_data,
                                       TOUCHED_TERMINALS *touched);

#ifndef RUNNING_ON_KALSIM
static bool va_pryon_lite_trigger_phrase_load(OPERATOR_DATA *op_data,
                                              void *message_data,
                                              unsigned *resp_length,
                                              OP_OPMSG_RSP_PAYLOAD **resp_data,
                                              DATA_FILE *trigger_file);
static bool va_pryon_lite_trigger_phrase_check(OPERATOR_DATA *op_data,
                                              void *message_data,
                                              unsigned *resp_length,
                                              OP_OPMSG_RSP_PAYLOAD **resp_data,
                                              DATA_FILE *trigger_file);
static bool va_pryon_lite_trigger_phrase_unload(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data,
                                                uint16 file_id);
#else
static bool va_pryon_lite_static_load(OPERATOR_DATA *op_data,
                                      void *message_data, unsigned *response_id,
                                      void **response_data);
#endif

static bool va_pryon_lite_opmsg_obpm_get_status(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data);
static bool va_pryon_lite_opmsg_obpm_set_params(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data);


static bool apva_start_engine(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **response_data);
static bool apva_stop_engine(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data);
static bool apva_reset_engine(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);

static inline bool initialize_model(VA_PRYON_LITE_OP_DATA *p_ext_data,
                                    WWE_COMMON_DATA *p_wwe_data);
static bool apva_send_notification(OPERATOR_DATA *op_data);
bool apva_get_metadata(OPERATOR_DATA *op_data, void *message_data,
                 unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);
bool apva_set_client_property(OPERATOR_DATA *op_data, void *message_data,
                 unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data);

/****************************************************************************
Private Constant Declarations
*/

#ifdef LOW_LATENCY
#define VA_PRYON_LITE_BUFFER_SIZE   320
#define VA_PRYON_LITE_FRAME_SIZE    160
#define VA_PRYON_LITE_BLOCK_SIZE    80
#else
#define VA_PRYON_LITE_BUFFER_SIZE   320
#define VA_PRYON_LITE_FRAME_SIZE    160
#define VA_PRYON_LITE_BLOCK_SIZE    80
#endif

#define N_STAT ( sizeof(APVA_STATISTICS) / sizeof(ParamType) )

/** The stub capability function handler table */
const handler_lookup_struct va_pryon_lite_handler_table =
{
    va_pryon_lite_create,           /* OPCMD_CREATE */
    va_pryon_lite_destroy,          /* OPCMD_DESTROY */
    wwe_start,                      /* OPCMD_START */
    base_op_stop,                   /* OPCMD_STOP */
    base_op_reset,                  /* OPCMD_RESET */
    wwe_connect,                    /* OPCMD_CONNECT */
    wwe_disconnect,                 /* OPCMD_DISCONNECT */
    wwe_buffer_details,             /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,        /* OPCMD_DATA_FORMAT */
    wwe_sched_info                  /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry va_pryon_lite_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION,    base_op_opmsg_get_capability_version},
    {OPMSG_COMMON_ID_SET_CONTROL,               wwe_opmsg_obpm_set_control},
    {OPMSG_COMMON_ID_GET_STATUS,                va_pryon_lite_opmsg_obpm_get_status},
    {OPMSG_COMMON_ID_GET_PARAMS,                wwe_opmsg_obpm_get_params},
    {OPMSG_COMMON_ID_GET_DEFAULTS,              wwe_opmsg_obpm_get_defaults},
    {OPMSG_COMMON_ID_SET_PARAMS,                va_pryon_lite_opmsg_obpm_set_params},
    {OPMSG_COMMON_ID_SET_UCID,                  wwe_opmsg_set_ucid},
    {OPMSG_COMMON_ID_GET_LOGICAL_PS_ID,         wwe_opmsg_get_ps_id},
    {OPMSG_QVA_ID_RESET_STATUS,                 wwe_reset_status},
    {OPMSG_QVA_ID_MODE_CHANGE,                  wwe_mode_change},
    {OPMSG_QVA_ID_TRIGGER_PHRASE_LOAD,          wwe_trigger_phrase_load},
    {OPMSG_QVA_ID_TRIGGER_PHRASE_UNLOAD,        wwe_trigger_phrase_unload},
    {OPMSG_COMMON_SET_SAMPLE_RATE,              wwe_set_sample_rate},
    {OPMSG_COMMON_REINIT_ALGORITHM,             wwe_reinit_algorithm},
    {OPMSG_APVA_ID_GET_METADATA,                apva_get_metadata},
    {OPMSG_APVA_ID_SET_CLIENT_PROPERTY,         apva_set_client_property},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA apva_cap_data =
{
    APVA_CAP_ID,                        /* Capability ID */
    APVA_APVA_VERSION_MAJOR, 1,         /* Version information - hi and lo parts */
    1, 1,                               /* Max number of sinks/inputs and sources/outputs */
    &va_pryon_lite_handler_table,       /* Pointer to message handler function table */
    va_pryon_lite_opmsg_handler_table,  /* Pointer to operator message handler function table */
    wwe_process_data,                   /* Pointer to data processing function */
    0,                                  /* This would hold processing time information */
    sizeof(VA_PRYON_LITE_OP_DATA)       /* Size of capability-specific per-instance data */
};

#if !defined(CAPABILITY_DOWNLOAD_BUILD)
MAP_INSTANCE_DATA(CAP_ID_APVA, VA_PRYON_LITE_OP_DATA)
#else
MAP_INSTANCE_DATA(CAP_ID_DOWNLOAD_APVA, VA_PRYON_LITE_OP_DATA)
#endif /* CAPABILITY_DOWNLOAD_BUILD */

const WWE_CAP_VIRTUAL_TABLE apva_vt =
{
    apva_start_engine,                        /* Start */
    NULL,                                     /* Stop */
    apva_reset_engine,                        /* Reset */
    va_pryon_lite_process_data,          /* Process */
#ifndef RUNNING_ON_KALSIM
    va_pryon_lite_trigger_phrase_load,   /* Load */
    va_pryon_lite_trigger_phrase_check,  /* Check */
    va_pryon_lite_trigger_phrase_unload, /* Unload */
#else
    va_pryon_lite_static_load,            /* Static Load */
#endif
    apva_send_notification                /* Send trigger notification */
};

/* Accessing the capability-specific per-instance data function */
static inline VA_PRYON_LITE_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (VA_PRYON_LITE_OP_DATA *) base_op_get_instance_data(op_data);
}

static inline WWE_COMMON_DATA *get_wwe_data(OPERATOR_DATA *op_data)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = base_op_get_instance_data(op_data);
    return &p_ext_data->wwe_class.wwe;
}

#ifndef RUNNING_ON_KALSIM
static bool ext_buffer_read(uint8 *dest_buffer, EXT_BUFFER *ext_src,
    unsigned int read_offset, unsigned int num_octets)
{
    tCbuffer *dest_cbuffer = cbuffer_create(dest_buffer, num_octets/4 + 1, BUF_DESC_SW_BUFFER);
    dest_cbuffer->read_ptr = (int *)dest_buffer;
    dest_cbuffer->write_ptr = (int *)dest_buffer;
    ext_buffer_set_read_offset(ext_src, read_offset);
    bool result = (ext_buffer_circ_read(dest_cbuffer, ext_src, num_octets) == num_octets);
    cbuffer_destroy_struct(dest_cbuffer);
    if (result != TRUE)
        panic(0x3002);
    return result;
}
#endif

static bool va_pryon_lite_create(OPERATOR_DATA *op_data, void *message_data,
                                 unsigned *response_id, void **resp_data)
{

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    if (!wwe_base_class_init(op_data, &p_ext_data->wwe_class, &apva_vt))
    {
        return FALSE;
    }

    if (!wwe_base_create(op_data, message_data, response_id, resp_data,
                         VA_PRYON_LITE_BLOCK_SIZE, VA_PRYON_LITE_BUFFER_SIZE,
                         VA_PRYON_LITE_FRAME_SIZE))
    {
        return FALSE;
    }

    if (!cpsInitParameters(&p_wwe_data->parms_def,
                           (unsigned *)APVA_GetDefaults(APVA_APVA_CAP_ID),
                           (unsigned *)&p_ext_data->apva_cap_params,
                           sizeof(APVA_PARAMETERS)))
    {
        return FALSE;
    }

    if (load_apva_handle(&p_ext_data->f_handle) == FALSE)
    {
        return TRUE;
    }

    /* initialize input metadata buffers */
    p_ext_data->handle.ww = NULL;

    p_ext_data->wakewordConfig.detectThreshold = \
    p_ext_data->apva_cap_params.OFFSET_APVA_THRESHOLD;
#ifdef LOW_LATENCY
    p_ext_data->wakewordConfig.lowLatency = 1;
#else
    p_ext_data->wakewordConfig.lowLatency = 0;
#endif
    p_ext_data->wakewordConfig.model = NULL;
    p_ext_data->wakewordConfig.sizeofModel = 0; // in bytes
    p_ext_data->engineBuffer = NULL;
    p_ext_data->sizeofengineBuffer = 0;
    p_ext_data->wakewordConfig.dnnAccel = NULL;
    p_ext_data->wakewordConfig.reserved = NULL;
    p_ext_data->wakewordConfig.apiVersion = PRYON_LITE_API_VERSION;
    p_ext_data->wakewordConfig.userData = op_data;

#ifdef USE_VAD
    p_ext_data->engineEventConfig.enableVadEvent = TRUE;
#else
    p_ext_data->engineEventConfig.enableVadEvent = FALSE;
#endif
    p_ext_data->engineEventConfig.enableWwEvent = TRUE;

    p_ext_data->engineConfig.ww = &p_ext_data->wakewordConfig;

    p_ext_data->num_client_properties_preserved = 0;

    L0_DBG_MSG1("VA_PRYON_LITE: created engine=%s\0",
                p_ext_data->configAttributes.engineVersion);

    return TRUE;
}

static bool va_pryon_lite_destroy(OPERATOR_DATA *op_data, void *message_data,
                                  unsigned *response_id, void **resp_data)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    /* call base_op destroy that creates and fills response message, too */
    if(!base_op_destroy_lite(op_data, resp_data))
    {
        return FALSE;
    }

    unload_wwe_handle(p_ext_data->f_handle);
    p_ext_data->f_handle = NULL;

    apva_stop_engine(op_data, message_data, response_id, resp_data);

#ifndef RUNNING_ON_KALSIM
    if (p_wwe_data->file_id)
    {
        DATA_FILE *trigger_file = file_mgr_get_file(p_wwe_data->file_id);
        if (trigger_file->type == ACCMD_TYPE_OF_FILE_EDF)
        {
            pfree(p_wwe_data->p_model);
            p_wwe_data->p_model = NULL;
        }
        file_mgr_release_file(p_wwe_data->file_id);
    }

#else /*ifndef RUNNING_ON_KALSIM*/
    if (p_wwe_data->file_id == MODEL_ID_DEFAULT)
    {
        p_wwe_data->p_model = NULL;
    }
#endif /* ifndef RUNNING_ON_KALSIM */

    pfree(p_ext_data->engineBuffer);
    p_ext_data->wakewordConfig.model = NULL;
    p_ext_data->engineBuffer = NULL;

    pdelete(p_ext_data->p_buffer);
    p_ext_data->p_buffer = NULL;

    for(int i=0; i<p_ext_data->num_client_properties_preserved; i++)
    {
        pfree(p_ext_data->cpd[i]);
    }

    L0_DBG_MSG("VA_PRYON_LITE: destroyed\0");

    return TRUE;
}

/* Data processing function */
static bool va_pryon_lite_process_data(OPERATOR_DATA *op_data,
                                       TOUCHED_TERMINALS *touched)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    cbuffer_copy_and_shift(p_wwe_data->ip_buffer, p_ext_data->p_buffer,
                           p_wwe_data->frame_size);

    /* Process */
    if (p_wwe_data->cur_mode == APVA_SYSMODE_FULL_PROC)
    {
        PryonLiteStatus status = PryonLite_ProcessData(p_ext_data->f_handle, &p_ext_data->handle, 
                                        (const short *)p_ext_data->p_buffer, p_wwe_data->frame_size);
        if (status.publicCode != PRYON_LITE_ERROR_OK)
        {
            L0_DBG_MSG("VA_PRYON_LITE: failed to push audio samples\0");
            return FALSE;
        }
    }

    return TRUE;
}

/* *************** Operator Message Handle functions ************************ */

static bool va_pryon_lite_opmsg_obpm_get_status(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);
    unsigned *resp = NULL;
    int i;

    if(!common_obpm_status_helper(message_data, resp_length, resp_data,
                                  sizeof(APVA_STATISTICS), &resp))
    {
         return FALSE;
    }

    if (resp)
    {
        APVA_STATISTICS stats;
        APVA_STATISTICS *pstats = &stats;
        ParamType* pparam = (ParamType*)&stats;

        // ch1 statistics
        pstats->OFFSET_APVA_DETECTED = p_wwe_data->keyword_detected;
        pstats->OFFSET_APVA_RESULT_CONFIDENCE = p_wwe_data->result_confidence;
        pstats->OFFSET_APVA_KEYWORD_LEN = p_wwe_data->keyword_len;

        // other statistics
        pstats->OFFSET_CUR_MODE = p_wwe_data->cur_mode;
        pstats->OFFSET_OVR_CONTROL = p_wwe_data->ovr_control;

        for (i=0; i<N_STAT/2; i++)
        {
            resp = cpsPack2Words(pparam[2*i], pparam[2*i+1], resp);
        }
        if (N_STAT % 2) // last one
        {
            cpsPack1Word(pparam[N_STAT-1], resp);
        }
    }

    return TRUE;
}

static bool va_pryon_lite_opmsg_obpm_set_params(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    bool retval = cpsSetParameterMsgHandler(&p_wwe_data->parms_def,
                                            message_data, resp_length,
                                            resp_data);

    p_wwe_data->init_flag = TRUE;

    p_ext_data->wakewordConfig.detectThreshold = \
        p_ext_data->apva_cap_params.OFFSET_APVA_THRESHOLD;
    L0_DBG_MSG1("VA_PRYON_LITE: set threshold to %d\0",
                p_ext_data->wakewordConfig.detectThreshold);

    return retval;
}

static void va_pryon_public_event_callback(PryonLiteV2Handle *handle, const PryonLiteV2Event* event)
{
#ifdef USE_VAD
    if (event->vadEvent != NULL)
    {
        va_pryon_lite_vad_callback(handle->ww, event->vadEvent);
    }
#endif

    if (event->wwEvent != NULL)
    {
        va_pryon_lite_result_callback(handle->ww, event->wwEvent);
    }
}

static inline bool initialize_model(VA_PRYON_LITE_OP_DATA *p_ext_data,
                                    WWE_COMMON_DATA *p_wwe_data)
{
    if(p_ext_data->handle.ww == NULL)
    {
        PryonLiteStatus status = PryonLite_Initialize(&p_ext_data->engineConfig, &p_ext_data->handle,
                                 va_pryon_public_event_callback, 
                                 &p_ext_data->engineEventConfig, p_ext_data->engineBuffer, 
                                 p_ext_data->sizeofengineBuffer);
        if (status.publicCode != PRYON_LITE_ERROR_OK)
        {
            L0_DBG_MSG("VA_PRYON_LITE: failed to initialize engine\0");
            return FALSE;
        }
        p_wwe_data->frame_size = VA_PRYON_LITE_FRAME_SIZE;
    }
    
    PryonLiteWakewordConfig *pconfig = &p_ext_data->wakewordConfig;
    
    /* This sets the detection thresholds for all keywords including Alexa */
    if (PryonLiteWakeword_SetDetectionThreshold(p_ext_data->handle.ww, 
                                               NULL, pconfig->detectThreshold).publicCode
        != PRYON_LITE_ERROR_OK)
    {
        L0_DBG_MSG("VA_PRYON_LITE: failed to set threshold\0");
        return FALSE;
    }

    L2_DBG_MSG4("VA_PRYON_LITE: engine initialized with threshold=%d, "
               "lowLatency=%d, modelsize=%d, engineBuffersize=%d",
               pconfig->detectThreshold,
               pconfig->lowLatency, pconfig->sizeofModel,
               p_ext_data->sizeofengineBuffer);

    return TRUE;
}

static bool apva_start_engine(OPERATOR_DATA *op_data, void *message_data,
                         unsigned *response_id, void **response_data)
{
    L0_DBG_MSG("VA_PRYON_LITE: start engine\0");

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    /* Prevent double allocation of working memory buffer */
    if (p_ext_data->engineBuffer == NULL)
    {
        p_ext_data->engineBuffer = xzpmalloc(
            p_ext_data->sizeofengineBuffer);

        if (p_ext_data->engineBuffer == NULL)
        {
            L0_DBG_MSG("VA_PRYON_LITE: failed to allocate working memory\0");
            return FALSE;
        }
    }

    if (!initialize_model(p_ext_data, p_wwe_data))
    {
        return FALSE;
    }

    /* initialize_model sets up the buffer size */
    if (p_ext_data->p_buffer == NULL)
    {
        p_ext_data->p_buffer = xzpnewn(p_wwe_data->frame_size, int16);
        if (p_ext_data->p_buffer == NULL)
        {
            L0_DBG_MSG("VA_PRYON_LITE: failed to allocate buffer memory\0");
            return FALSE;
        }
    }

    return TRUE;
}

static bool apva_stop_engine(OPERATOR_DATA *op_data, void *message_data,
                        unsigned *response_id, void **response_data)
{
    L0_DBG_MSG("VA_PRYON_LITE: stop engine\0");

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);

    PryonLite_Destroy(&p_ext_data->handle);

    return TRUE;
}

static bool apva_reset_engine(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched)
{
    L0_DBG_MSG("VA_PRYON_LITE: reset engine\0");

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    if (p_ext_data->wakewordConfig.model == NULL ||
        p_ext_data->engineBuffer == NULL)
    {
        return FALSE;
    }

    PryonLite_Destroy(&p_ext_data->handle);

    if (!initialize_model(p_ext_data, p_wwe_data))
    {
        return FALSE;
    }

    // re-intialize any previously preserved client properties
    client_property_data* cpd_i;
    for(int i=0; i<p_ext_data->num_client_properties_preserved; i++)
    {
        cpd_i = p_ext_data->cpd[i];
        PryonLite_SetClientProperty(&p_ext_data->handle, cpd_i->group_id,
                                     cpd_i->property_id, cpd_i->property_value);
    }

    return TRUE;
}

#ifndef RUNNING_ON_KALSIM

static bool va_pryon_lite_trigger_phrase_load(OPERATOR_DATA *op_data,
                                              void *message_data,
                                              unsigned *resp_length,
                                              OP_OPMSG_RSP_PAYLOAD **resp_data,
                                              DATA_FILE *trigger_file)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    if (p_wwe_data->p_model != NULL ||
        p_ext_data->wakewordConfig.model != NULL ||
        p_ext_data->engineBuffer != NULL)
    {
        return FALSE;
    }

    if (trigger_file->type == ACCMD_TYPE_OF_FILE_EDF)
    {
        /* External buffer */
        p_ext_data->wakewordConfig.sizeofModel = ext_buffer_amount_data(
            trigger_file->u.ext_file_data);
        p_ext_data->wakewordConfig.model = (uint8 *) pmalloc(p_ext_data->wakewordConfig.sizeofModel);
        ext_buffer_read((uint8 *)p_ext_data->wakewordConfig.model, trigger_file->u.ext_file_data, 0,
                        p_ext_data->wakewordConfig.sizeofModel);

        L0_DBG_MSG1(
            "VA_PRYON_LITE: model data copied from external buffer size=%d\0",
            p_ext_data->wakewordConfig.sizeofModel);
    }
    else
    {
        /* Internal buffer */
        p_ext_data->wakewordConfig.sizeofModel = cbuffer_calc_amount_data_in_words(
            trigger_file->u.file_data) * 4; // in bytes
        p_ext_data->wakewordConfig.model = \
            (uint8 *)trigger_file->u.file_data->base_addr;
        L0_DBG_MSG1("VA_PRYON_LITE: model data in internal buffer size=%d\0",
                    p_ext_data->wakewordConfig.sizeofModel);
    }

    p_wwe_data->p_model = (uint8 *) p_ext_data->wakewordConfig.model;
    p_wwe_data->n_model_size = p_ext_data->wakewordConfig.sizeofModel;

    return TRUE;
}

static bool va_pryon_lite_trigger_phrase_check(OPERATOR_DATA *op_data,
                                              void *message_data,
                                              unsigned *resp_length,
                                              OP_OPMSG_RSP_PAYLOAD **resp_data,
                                              DATA_FILE *trigger_file)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    /* Get config attributes */
    PryonLiteStatus status = PryonLite_GetConfigAttributes(&p_ext_data->engineConfig, 
                                                           &p_ext_data->engineEventConfig, 
                                                           &p_ext_data->configAttributes);

    if (status.publicCode != PRYON_LITE_ERROR_OK)
    {
        /* If we fail to load data from the model then reset these fields */
        file_mgr_release_file(p_wwe_data->file_id);
        p_ext_data->wakewordConfig.sizeofModel = 0;
        p_wwe_data->n_model_size = 0;
        p_wwe_data->file_id = 0;
        return FALSE;
    }

    p_ext_data->sizeofengineBuffer = p_ext_data->configAttributes.requiredMem;
    p_wwe_data->n_model_size = p_ext_data->wakewordConfig.sizeofModel;
    L0_DBG_MSG2("VA_PRYON_LITE: model loaded size=%d decMem=%d\0",
                p_ext_data->wakewordConfig.sizeofModel,
                p_ext_data->configAttributes.requiredMem);

    return TRUE;
}

static bool va_pryon_lite_trigger_phrase_unload(OPERATOR_DATA *op_data,
                                                void *message_data,
                                                unsigned *resp_length,
                                                OP_OPMSG_RSP_PAYLOAD **resp_data,
                                                uint16 file_id)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    DATA_FILE *trigger_file = file_mgr_get_file(p_wwe_data->file_id);

    if (trigger_file->type == ACCMD_TYPE_OF_FILE_EDF)
    {
        pfree(p_wwe_data->p_model);
    }

    file_mgr_release_file(p_wwe_data->file_id);

    p_wwe_data->file_id = 0;
    p_ext_data->wakewordConfig.model = NULL;
    p_ext_data->wakewordConfig.sizeofModel = 0;
    p_ext_data->sizeofengineBuffer = 0;

    p_wwe_data->n_model_size = 0;
    p_wwe_data->p_model = NULL;

    pfree(p_ext_data->engineBuffer);
    p_ext_data->engineBuffer = NULL;

    L0_DBG_MSG("VA_PRYON_LITE: model unloaded\0");

    return TRUE;
}

#else /* ifndef RUNNING_ON_KALSIM */

static bool va_pryon_lite_static_load(OPERATOR_DATA *op_data,
                                      void *message_data, unsigned *response_id,
                                      void **response_data)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);

    p_wwe_data->file_id = MODEL_ID_DEFAULT;
    p_wwe_data->p_model = (uint8 *) &model_raw_apva;
    p_ext_data->wakewordConfig.sizeofModel = APVA_DEFAULT_MODEL_SIZE * 4 ; // in bytes
    p_ext_data->wakewordConfig.model = p_wwe_data->p_model;

    /* Check configuration attributes */
    PryonLiteStatus status = PryonLite_GetConfigAttributes(&p_ext_data->engineConfig, 
                                                           &p_ext_data->engineEventConfig, 
                                                           &p_ext_data->configAttributes);

    if (status.publicCode != PRYON_LITE_ERROR_OK)
    {
        /* If we fail to load data from the model then reset these fields */
        p_ext_data->wakewordConfig.sizeofModel = 0;
        p_ext_data->wakewordConfig.model = NULL;
        p_wwe_data->p_model = NULL;
        p_wwe_data->n_model_size = 0;
        p_wwe_data->file_id = 0;
        return FALSE;
    }
 

    p_ext_data->sizeofengineBuffer = p_ext_data->configAttributes.requiredMem;
    p_wwe_data->n_model_size = p_ext_data->wakewordConfig.sizeofModel;

    L0_DBG_MSG2("VA_PRYON_LITE: model loaded size=%d decMem=%d\0",
                p_ext_data->wakewordConfig.sizeofModel,
                p_ext_data->configAttributes.requiredMem);

    return TRUE;
}
#endif

/* ********************* callbacks ******************************** */
static void va_pryon_lite_result_callback(PryonLiteWakewordHandle handle,
                                          const PryonLiteWakewordResult* result)
{
    OPERATOR_DATA *op_data = (OPERATOR_DATA *)result->userData;
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);

    L2_DBG_MSG("VA_PRYON_LITE: Result Callback\0");

    p_wwe_data->keyword_detected++;
    p_wwe_data->result_confidence = result->confidence;
    p_wwe_data->keyword_len = (int) (
        result->endSampleIndex - result->beginSampleIndex);

    TIME keyword_duration = (TIME)(
        (p_wwe_data->keyword_len) * 1000000ULL /
        (unsigned long long)p_wwe_data->sample_rate);

    TIME end_kw_ts = time_sub(p_wwe_data->last_timestamp,
        (TIME)((p_wwe_data->num_samples_fully_processed - result->endSampleIndex) * 1000000ULL /
        (unsigned long long)p_wwe_data->sample_rate));

    p_wwe_data->ts.trigger_start_timestamp = time_sub(end_kw_ts, keyword_duration);

    p_wwe_data->ts.trigger_end_timestamp = end_kw_ts;
    p_wwe_data->ts.end_timestamp_computed_flag = TRUE;
    p_wwe_data->ts.trigger_offset = 0;
    p_wwe_data->trigger = TRUE;
    
    p_ext_data->pryon_metadata_blob_size = result->metadataBlob.blobSize;
    p_ext_data->pryon_metadata_blob = result->metadataBlob.blob;

    L0_DBG_MSG4("VA_PRYON_LITE: Logging endSampleIndex, beginSampleIndex, start_timestamp, \
         trigger_end_timestamp %d %d %d %d \0", result->endSampleIndex, result->beginSampleIndex,
         p_wwe_data->ts.trigger_start_timestamp, p_wwe_data->ts.trigger_end_timestamp);

}


static bool apva_send_notification(OPERATOR_DATA *op_data)
{
    WWE_COMMON_DATA *p_wwe_data = get_wwe_data(op_data);
    unsigned msg_size = \
        OPMSG_UNSOLICITED_WWE_TRIGGER_CH1_TRIGGER_DETAILS_WORD_OFFSET + 1;
    OPMSG_REPLY_ID message_id = OPMSG_REPLY_ID_VA_NEGATIVE_TRIGGER;

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);

    unsigned *trigger_message = xzpnewn(msg_size, unsigned);
    if (trigger_message == NULL)
    {
        L2_DBG_MSG("APVA: NO MEMORY FOR TRIGGER MESSAGE");
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, msg_size);
        return FALSE;
    }

    OPMSG_CREATION_FIELD_SET(trigger_message, OPMSG_UNSOLICITED_WWE_TRIGGER,
                             NUMBER_OF_CHANNELS, 1);

    unsigned start_ts_upper = 0;
    unsigned start_ts_lower = 0;
    unsigned end_ts_upper = 0;
    unsigned end_ts_lower = 0;

    unsigned *submessage = OPMSG_CREATION_FIELD_POINTER_GET_FROM_OFFSET(
        trigger_message, OPMSG_UNSOLICITED_WWE_TRIGGER, CH0_TRIGGER_DETAILS, 0);

    if (p_wwe_data->trigger)
    {
        OPMSG_CREATION_FIELD_SET(submessage,
                                 OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                                 TRIGGER_CONFIDENCE,
                                 p_wwe_data->result_confidence);

        message_id = OPMSG_REPLY_ID_VA_TRIGGER;
        start_ts_upper = time_sub(p_wwe_data->ts.trigger_start_timestamp,
                                  p_wwe_data->ts.trigger_offset) >> 16;
        start_ts_lower = time_sub(p_wwe_data->ts.trigger_start_timestamp,
                                  p_wwe_data->ts.trigger_offset) & 0xffff;
        end_ts_upper = time_sub(p_wwe_data->ts.trigger_end_timestamp,
                                p_wwe_data->ts.trigger_offset) >> 16;
        end_ts_lower = time_sub(p_wwe_data->ts.trigger_end_timestamp,
                                p_wwe_data->ts.trigger_offset) & 0xffff;
    }
    else
    {
        OPMSG_CREATION_FIELD_SET(submessage,
                                 OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                                 TRIGGER_CONFIDENCE,
                                 0);
    }

    OPMSG_CREATION_FIELD_SET(submessage, OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                             START_TIMESTAMP_UPPER, start_ts_upper);
    OPMSG_CREATION_FIELD_SET(submessage, OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                             START_TIMESTAMP_LOWER, start_ts_lower);
    OPMSG_CREATION_FIELD_SET(submessage, OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                             END_TIMESTAMP_UPPER, end_ts_upper);
    OPMSG_CREATION_FIELD_SET(submessage, OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                             END_TIMESTAMP_LOWER, end_ts_lower);

    OPMSG_CREATION_FIELD_SET(submessage, OPMSG_WWE_TRIGGER_CHANNEL_INFO,
                             METADATA_SIZE, p_ext_data->pryon_metadata_blob_size);

    common_send_unsolicited_message(op_data, (unsigned)message_id, msg_size,
                                    trigger_message);

    pdelete(trigger_message);

    return TRUE;
}

bool apva_get_metadata(OPERATOR_DATA *op_data, void *message_data,
        unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{

    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);
    unsigned requested_part_size = APVA_METADATA_MAX_PART_SIZE; // In bytes

    /* extract the metadata part number */
    unsigned metadata_part_num = OPMSG_FIELD_GET(message_data,
                                        OPMSG_APVA_GET_METADATA, METADATA_PART_NUMBER);    

    unsigned total_num_parts = p_ext_data->pryon_metadata_blob_size/requested_part_size;
    if(p_ext_data->pryon_metadata_blob_size % requested_part_size)
    {
        // We have an extra part that needs to be accounted for
        total_num_parts = total_num_parts + 1; 
    }

    if (metadata_part_num == total_num_parts)
    {
        requested_part_size = p_ext_data->pryon_metadata_blob_size % APVA_METADATA_MAX_PART_SIZE;
    }
    else if (metadata_part_num > total_num_parts)
    {
        L0_DBG_MSG("Invalid metadata part number requested");
        return FALSE;
    }

    unsigned metadata_start_index = (metadata_part_num-1)*APVA_METADATA_MAX_PART_SIZE;

    L2_DBG_MSG4("apva_get_metadata: First byte of metadata part is 0x%04X and part size is %d \
        and metadata_part_num is %d and metadata_start_index is %d",
        *(p_ext_data->pryon_metadata_blob + metadata_start_index), requested_part_size,
        metadata_part_num, metadata_start_index);

    *resp_length = OPMSG_RSP_PAYLOAD_SIZE_RAW_DATA(APVA_GET_METADATA_MESSAGE_RESP_LENGTH);
    *resp_data = (OP_OPMSG_RSP_PAYLOAD *)xzpmalloc((*resp_length)*sizeof(unsigned));
    if (*resp_data == NULL)
    {
        return FALSE;
    }

    /* echo the opmsgID/keyID */
    (*resp_data)->msg_id = OPMGR_GET_OPCMD_MESSAGE_MSG_ID((OPMSG_HEADER*)message_data);

    /* echo the requested metadata part number */
    (*resp_data)->u.raw_data[0] = metadata_part_num;
    /* inform Apps about the size (in bytes) of the metdata part being sent back */
    (*resp_data)->u.raw_data[1] = requested_part_size;

    /* Every 2 bytes of the blob is packed into a 16 bit word in the message. 
     * For example blob[0] is the LSB of the first 16-bit word,
     * and blob[1] is the MSB of the first word */
    int j = 2;
    for(int i = metadata_start_index; i < (metadata_start_index + requested_part_size); i = i + 2)
    {
        /*
            The code inside this for loop implements the following:
            
            // Left shifting by half a word for bitwise OR later on
            int upper_16_bit = p_ext_data->pryon_metadata_blob[i+1] << 8;
            // 0x00FF masks out the upper bits
            int lower_16_bit = 0x00FF & p_ext_data->pryon_metadata_blob[i]; 
            (*resp_data)->u.raw_data[j] = upper_16_bit | lower_16_bit;
        */
        (*resp_data)->u.raw_data[j] = (*(p_ext_data->pryon_metadata_blob + (i+1)) << 8) |
                                         (0x00FF & *(p_ext_data->pryon_metadata_blob + i));
        j = j + 1;
    }

    return TRUE;
}

bool apva_set_client_property(OPERATOR_DATA *op_data, void *message_data,
         unsigned int *resp_length, OP_OPMSG_RSP_PAYLOAD **resp_data)
{
    VA_PRYON_LITE_OP_DATA *p_ext_data = get_instance_data(op_data);

    if(!PryonLite_IsInitialized(&p_ext_data->handle))
    {
        L0_DBG_MSG("VA_PRYON_LITE: Cannot set client property before initializing the pryon engine!");
        return FALSE;
    }

    int num_client_properties_preserved = p_ext_data->num_client_properties_preserved;
    if(num_client_properties_preserved == MAX_NUM_CLIENT_PROPERTIES)
    {
        L0_DBG_MSG("VA_PRYON_LITE: Cannot set client property because we've reached \
                the maximum number of client properties that we can preserve");
        return FALSE;
    }

    int group = OPMSG_FIELD_GET(message_data, OPMSG_APVA_SET_CLIENT_PROPERTY, GROUP_ID);
    int property = OPMSG_FIELD_GET(message_data, OPMSG_APVA_SET_CLIENT_PROPERTY, CLIENT_PROPERTY_ID);
    int data = OPMSG_FIELD_GET(message_data, OPMSG_APVA_SET_CLIENT_PROPERTY, CLIENT_PROPERTY_VALUE);
    PryonLiteStatus result_status;

    result_status = PryonLite_SetClientProperty(&p_ext_data->handle, group, property, data);
    if(result_status.publicCode != PRYON_LITE_ERROR_OK)
    {
        L0_DBG_MSG1("VA_PRYON_LITE: SetClientProperty failed with pryon_lite error code %d ",
                 result_status.publicCode);
        return FALSE;
    }

    // preserve client property in case APVA gets reinitailized in the future
    client_property_data* cpd_i;
    for(int i=0; i<MAX_NUM_CLIENT_PROPERTIES; i++)
    {

        if(p_ext_data->cpd[i] == NULL)
        {
            /* Create new entry in preserved properties list for this new
             * property */  
            cpd_i = xzpnew(client_property_data);
            cpd_i->group_id = group;
            cpd_i->property_id = property;
            cpd_i->property_value = data;
            p_ext_data->cpd[i] = cpd_i;
            p_ext_data->num_client_properties_preserved = num_client_properties_preserved + 1;
            break;
        }
        else
        {
            /* Check to see if this property already exists in the  
             * preserved properties list */  
            cpd_i = p_ext_data->cpd[i];
            if(cpd_i->group_id==group && cpd_i->property_id==property)
            {
               /* Update preserved properties list with new value for
                * the existing property */       
               cpd_i->property_value = data;
               break; 
            }
         }
    }

    return TRUE;

}

#ifdef USE_VAD
static void va_pryon_lite_vad_callback(PryonLiteV2Handle* handle,
                                       const PryonLiteVadEvent* event)
{

}
#endif

