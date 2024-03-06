/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  base_aud_cur_op.h
 * \defgroup base_aud_cur_op
 *
 * \ingroup capabilities
 *
 * Audio curation base operator public header file. <br>
 *
 */

#ifndef BASE_AUD_CUR_OP_H
#define BASE_AUD_CUR_OP_H

/****************************************************************************
Include Files
*/
#include "capabilities.h"
#include "opmgr/opmgr_for_ops.h"
#include "opmgr/opmgr.h"
#include "adaptor/connection_id.h"

#include "anc_hw_manager/anc_hw_manager_gain.h"

/****************************************************************************
Public Constant Declarations
*/

#define AUD_CUR_DEFAULT_BLOCK_SIZE  1

/* Playback terminal is 0 */
#define AUD_CUR_PLAYBACK_TERMINAL   0

/* Metadata channels for playback and microphone */
#define AUD_CUR_METADATA_PLAYBACK   0
#define AUD_CUR_METADATA_MIC        1
#define AUD_CUR_NUM_METADATA        2

/****************************************************************************
Public Macro Definitions
*/
#define AUD_CUR_GET_TERMINAL_POS(terminal_num) (1 << terminal_num)

/****************************************************************************
Public Type Declarations
*/


/* Constructor and destructor prototypes */
typedef bool (*AUD_CUR_START_FN)(OPERATOR_DATA* op_data);
typedef bool (*AUD_CUR_STOP_FN)(OPERATOR_DATA* op_data);
typedef bool (*AUD_CUR_CONNECT_FN)(OPERATOR_DATA* op_data,
                                   unsigned terminal_id);
typedef bool (*AUD_CUR_DISCONNECT_FN)(OPERATOR_DATA* op_data,
                                      unsigned terminal_id);
typedef bool (*AUD_CUR_PARAM_UPDATE_FN)(OPERATOR_DATA* op_data);

/* Audio curation class and supporting types */

/* Represent sink or source definitions for the class. */
typedef struct _AUD_CUR_TERMINAL
{
    uint16 max;                     /* Maximum number of terminal type */
    uint16 min_valid_mask;          /* Mask for minimum valid connections */
    uint16 max_valid_mask;          /* Mask for maximum valid connections */
    uint16 connected;               /* Connected terminals of this type */
    tCbuffer **p_buffer_list;       /* Pointer to the terminal buffer list */
} AUD_CUR_TERMINAL;

/* Base class definition for audio curation capabilities */
typedef struct _AUDIO_CURATION_DEF
{
    /* Capability flags */
    bool re_init_flag:8;
    bool in_place_flag:8;
    bool supports_metadata_flag:8;
    bool dynamic_buffer_size_flag:8;
    bool runtime_disconnect:8;       /* Will default to FALSE */

    AUD_CUR_TERMINAL sinks;          /* Capability sink definitions */
    AUD_CUR_TERMINAL sources;        /* Capability source definitions */

    unsigned buffer_size;            /* Capability desired buffer size */
    unsigned block_size;             /* Capability desired block size */

    CPS_PARAM_DEF param_def;         /* Capability parameter information */
    CAP_ID cap_id;                   /* Capability ID for the class instance */

    /* Capability-specific functions */
    AUD_CUR_START_FN start_fn;
    AUD_CUR_STOP_FN stop_fn;
    AUD_CUR_CONNECT_FN connect_fn;
    AUD_CUR_DISCONNECT_FN disconnect_fn;
    AUD_CUR_PARAM_UPDATE_FN param_update_fn;

    /* Metadata buffers */
    tCbuffer *metadata_ip[AUD_CUR_NUM_METADATA];
    tCbuffer *metadata_op[AUD_CUR_NUM_METADATA];

    /* Buffer pointers for input and output terminals */
    unsigned buffer_data[];
} AUDIO_CURATION_DEF;

/****************************************************************************
Private Function Declarations
*/

/**
 * \brief Get the class data
 *
 * \param  op_data    Pointer to operator structure
 *
 * \return - Pointer to the class data
 */

static inline AUDIO_CURATION_DEF *get_class_data(OPERATOR_DATA *op_data)
{
    return (AUDIO_CURATION_DEF *) base_op_get_class_ext(op_data);
}

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Create and initialize the audio curation class definition
 *
 * \param  op_data              Pointer to operator structure
 * \param  max_sources          Maximum number of sources
 * \param  max_sinks            Maximum number of sinks
 *
 * \return - result TRUE if success
 */

extern bool aud_cur_create(OPERATOR_DATA* op_data,
                           unsigned max_sources,
                           unsigned max_sinks);

/**
 * \brief Destroy audio curation class instance
 *
 * \param  op_data      Pointer to operator structure
 */
extern void aud_cur_destroy(OPERATOR_DATA *op_data);

/**
 * \brief Connect a terminal to an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_connect(OPERATOR_DATA *op_data,
                            void *message_data,
                            unsigned *response_id,
                            void **response_data);

/**
 * \brief Disconnect a terminal from an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_disconnect(OPERATOR_DATA *op_data,
                               void *message_data,
                               unsigned *response_id,
                               void **response_data);

/**
 * \brief Handle buffer details request to an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_buffer_details(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *response_id,
                                   void **response_data);

/**
 * \brief Start an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_start(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *response_id,
                          void **response_data);

/**
 * \brief Stop an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_stop(OPERATOR_DATA *op_data,
                         void *message_data,
                         unsigned *response_id,
                         void **response_data);

/**
 * \brief  Reset an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_reset(OPERATOR_DATA *op_data,
                          void *message_data,
                          unsigned *response_id,
                          void **response_data);

/**
 * \brief  Set the buffer size for an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_opmsg_set_buffer_size(OPERATOR_DATA *op_data,
                                          void *message_data,
                                          unsigned *response_id,
                                          OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Get the scheduler info for an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_get_sched_info(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *response_id,
                                   void **response_data);

/**
 * \brief Set audio curation callbacks
 *
 * \param  op_data          Pointer to operator structure
 * \param  start_fn         Pointer to start function
 * \param  stop_fn          Pointer to stop function
 * \param  connect_fn       Pointer to connect function
 * \param disconnect_fn     Pointer to disconnect function
 * \param param_update_fn   Pointer to param update function
 *
 * \return - NONE
 */
extern void aud_cur_set_callbacks(OPERATOR_DATA *op_data,
                                  AUD_CUR_START_FN start_fn,
                                  AUD_CUR_STOP_FN stop_fn,
                                  AUD_CUR_CONNECT_FN connect_fn,
                                  AUD_CUR_DISCONNECT_FN disconnect_fn,
                                  AUD_CUR_PARAM_UPDATE_FN param_update_fn);

/**
 * \brief Set audio curation flags
 *
 * \param  op_data              Pointer to operator structure
 * \param  in_place             Flag indicating in-place support
 * \param  supports_metadata    Flag indicating metadata support
 * \param  dynamic_buffer_size  Flag indicating dynamic buffer size support
 *
 * \return - NONE
 */
extern void aud_cur_set_flags(OPERATOR_DATA *op_data,
                              bool in_place,
                              bool supports_metadata,
                              bool dynamic_buffer_size);

/**
 * \brief Set audio curation flag for runtime disconnect
 *
 * \param  op_data              Pointer to operator structure
 * \param  allowed              Flag indicating whether runtime disconnect is
 *                              allowed (TRUE) or not (FALSE, default)
 *
 * \return - NONE
 */
extern void aud_cur_set_runtime_disconnect(OPERATOR_DATA *op_data,
                                           bool allowed);

/**
 * \brief Set minimum required terminals for start/stop
 *
 * \param  op_data          Pointer to operator structure
 * \param  source_mask      Mask for min valid source terminals
 * \param  sink_mask        Mask for min valid sink terminals
 *
 * \return - NONE
 */
extern void aud_cur_set_min_terminal_masks(OPERATOR_DATA *op_data,
                                           uint16 source_mask,
                                           uint16 sink_mask);

/**
 * \brief Set invalid terminals for connect
 *
 * \param  op_data          Pointer to operator structure
 * \param  source_mask      Mask for max valid source terminals
 * \param  sink_mask        Mask for max valid sink terminals
 *
 * \return - NONE
 */
extern void aud_cur_set_max_terminal_masks(OPERATOR_DATA *op_data,
                                           uint16 source_mask,
                                           uint16 sink_mask);

/**
 * \brief Get the state of the reinitialization flag.
 *
 * \param  op_data          Pointer to operator structure
 *
 * \return - Reinitialization flag state
 */
inline bool aud_cur_get_reinit(OPERATOR_DATA *op_data);

/**
 * \brief Set the state of the reinitialization flag.
 *
 * \param  op_data          Pointer to operator structure
 * \param  state            New state
 *
 * \return - NONE
 */
inline void aud_cur_set_reinit(OPERATOR_DATA *op_data, bool state);

/**
 * \brief Get CPS data.
 *
 * \param  op_data          Pointer to operator structure
 *
 * \return - Pointer to CPS data.
 */
extern CPS_PARAM_DEF *aud_cur_get_cps(OPERATOR_DATA *op_data);

/**
 * \brief Set buffer size for an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  buffer_size      Buffer size
 *
 */
extern void aud_cur_set_buffer_size(OPERATOR_DATA *op_data,
                                    unsigned buffer_size);

/**
 * \brief Set block size for an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  block_size       Block size
 *
 */
extern void aud_cur_set_block_size(OPERATOR_DATA *op_data, unsigned block_size);

/* TODO: copy/advance buffers */

/**
 * \brief Propagate metadata in an audio curation instance
 *
 * \param  op_data          Pointer to operator structure
 * \param  amount           Transfer amount
 *
 * \return - none
 */
extern void aud_cur_metadata_propagate(OPERATOR_DATA *op_data, unsigned amount);

/**
 * \brief Get a particular source terminal cbuffer pointer
 *
 * \param op_data           Pointer to operator structure
 * \param num               Terminal number to retrieve
 *
 * \return - pointer to the terminal cbuffer
 */
extern tCbuffer *aud_cur_get_source_terminal(OPERATOR_DATA *op_data,
                                             uint16 num);

/**
 * \brief Get a particular sink terminal cbuffer pointer
 *
 * \param op_data           Pointer to operator structure
 * \param num                Terminal number to retrieve
 *
 * \return - pointer to the terminal cbuffer
 */
extern tCbuffer *aud_cur_get_sink_terminal(OPERATOR_DATA *op_data,
                                           uint16 num);

/**
 * \brief  Get parameters from an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_opmsg_get_params(OPERATOR_DATA *op_data,
                                     void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Get default parameters from an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_opmsg_get_defaults(OPERATOR_DATA *op_data,
                                       void *message_data,
                                       unsigned *resp_length,
                                       OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Set parameters from an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 *
 * Note: this will set the re_init_flag in the class data.
 */
extern bool aud_cur_opmsg_set_params(OPERATOR_DATA *op_data,
                                     void *message_data,
                                     unsigned *resp_length,
                                     OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Handle reading parameters when setting a UCID
 *
 * \param instance_data         Pointer to instance data (OPERATOR_DATA*)
 * \param key                   PS Key type
 * \param rank                  Persistence rank
 * \param length                Length of the data
 * \param data                  Pointer to the data
 * \param STATUS_KYMERA         Status
 * \param extra_status_info     Additional status information
 *
 * Note: this will set the re_init_flag in the class data.
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_ups_params(void* instance_data,
                               PS_KEY_TYPE key,
                               PERSISTENCE_RANK rank,
                               uint16 length,
                               unsigned* data,
                               STATUS_KYMERA status,
                               uint16 extra_status_info);

/**
 * \brief  Set the UCID from an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 *
 * Note: this will set the re_init_flag in the class data.
 */
extern bool aud_cur_opmsg_set_ucid(OPERATOR_DATA *op_data,
                                   void *message_data,
                                   unsigned *resp_length,
                                   OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Get the PSID from an audio curation operator
 *
 * \param  op_data          Pointer to operator structure
 * \param  message_data     Pointer to message payload
 * \param  response_id      Pointer to response message ID
 * \param  response_data    Pointer to response payload
 *
 * \return - result TRUE if response generated
 */
extern bool aud_cur_opmsg_get_ps_id(OPERATOR_DATA *op_data,
                                    void *message_data,
                                    unsigned *resp_length,
                                    OP_OPMSG_RSP_PAYLOAD **response_data);

/**
 * \brief  Calculate the number of samples to process
 *
 * \param  op_data          Pointer to operator structure
 * \param  touched          Pointer to touched terminals
 *
 * \return - Number of samples to process
 */
extern unsigned aud_cur_calc_samples(OPERATOR_DATA *op_data,
                                     TOUCHED_TERMINALS *touched);

/**
 * \brief  Transfer data from input to output buffers
 *
 * \param  op_data             Pointer to operator structure
 * \param  amount              Amount of data to transfer
 * \param  terminal_skip_mask  Bitwise mask of terminals to skip transfer
 *
 * \return - Number of samples transferred
 */
extern unsigned aud_cur_mic_data_transfer(OPERATOR_DATA *op_data,
                                          unsigned amount,
                                          unsigned terminal_skip_mask);

/**
 * \brief  Transfer mic metadata from input to output buffers
 *
 * \param  op_data             Pointer to operator structure
 * \param  amount              Amount of data to transfer
 *
 * \return - Number of samples transferred
 */
extern unsigned aud_cur_mic_metadata_transfer(OPERATOR_DATA *op_data,
                                              unsigned amount);


/**
 * \brief  Transfer playback data from input to output buffers
 *
 * \param  op_data             Pointer to operator structure
 * \param  amount              Amount of data to transfer
 *
 * \return - Number of samples transferred
 */
extern unsigned aud_cur_playback_data_transfer(OPERATOR_DATA *op_data,
                                               unsigned amount);

/**
 * \brief  Transfer playback metadata from input to output buffers
 *
 * \param  op_data             Pointer to operator structure
 * \param  amount              Amount of data to transfer
 *
 * \return - Number of samples transferred
 */
extern unsigned aud_cur_playback_metadata_transfer(OPERATOR_DATA *op_data,
                                                   unsigned amount);

/**
 * \brief  Create cbuffers with a particular malloc preference.
 *
 * \param  pp_buffer            Pointer address for cbuffer
 * \param  size                 Size of the cbuffer
 * \param  malloc_pref          Malloc preference
 *
 * \return  boolean indicating success or failure.
 */
extern bool aud_cur_create_cbuffer(tCbuffer **pp_buffer,
                                   unsigned size,
                                   unsigned malloc_pref);

/**
 * \brief  ASM function to calculate adjusted target gain
 *
 * \param  fine_gain    Fine gain value
 * \param  db_gain      dB gain to adjust fine_gain (dB, log2)
 *
 * \return Adjusted fine gain in ANC HW steps
 *
 * This takes the static fine gain (in ANC HW steps) and applies a dB offset
 * specified by the db_gain to determine an adjusted fine gain.
 */
extern uint8 aud_cur_calc_adjusted_gain(uint16 fine_gain, int db_gain);

/****************************************************************************
Inline Function Declarations
*/
inline bool aud_cur_get_reinit(OPERATOR_DATA *op_data)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    return p_class_data->re_init_flag;
}

inline void aud_cur_set_reinit(OPERATOR_DATA *op_data, bool state)
{
    AUDIO_CURATION_DEF *p_class_data = get_class_data(op_data);
    p_class_data->re_init_flag = state;
}

/****************************************************************************
Inter-capability messaging
*/

/**
 * \brief  Callback from releasing a shared gain pointer.
 *
 * \param  con_id           Connection ID
 * \param  status           Message status
 * \param  op_id            Source operator ID
 * \param  num_resp_params  Number of parameters in the response
 * \param  resp_params      Pointer to response payload
 *
 * \return  boolean indicating success or failure.
 */
extern bool aud_cur_release_shared_gain_cback(CONNECTION_LINK con_id,
                                              STATUS_KYMERA status,
                                              EXT_OP_ID op_id,
                                              unsigned num_resp_params,
                                              unsigned *resp_params);

/**
 * \brief  Send a message to release a shared fine gain pointer
 *
 * \param  p_gain           Pointer to shared fine gain to release
 * \param  filter           ANC filter associated with the fine gain
 * \param  gain_type        Gain control type
 * \param  ahm_op_id        Operator ID for the ANC HW Manager
 *
 * \return  boolean indicating success or failure.
 */
extern void aud_cur_release_shared_fine_gain(AHM_SHARED_FINE_GAIN *p_gain,
                                             AHM_ANC_FILTER filter,
                                             AHM_GAIN_CONTROL_TYPE gain_type,
                                             uint16 ahm_op_id,
                                             AHM_ANC_INSTANCE anc_instance);

/**
 * \brief  Send a message to get a shared fine gain pointer
 *
 * \param  p_ext_data       Pointer to extra operator data structure
 * \param  filter           ANC filter associated with the fine gain
 * \param  op_id            Target operator ID
 * \param  gain_type        Gain control type
 * \param  callback         Callback to receive the response
 *
 * \return  boolean indicating success or failure.
 */
extern void aud_cur_get_shared_fine_gain(void *p_ext_data,
                                         AHM_ANC_FILTER filter,
                                         unsigned op_id,
                                         AHM_GAIN_CONTROL_TYPE gain_type,
                                         AHM_ANC_INSTANCE anc_instance,
                                         OP_MSG_CBACK callback);

#endif /* BASE_AUD_CUR_OP_H */
