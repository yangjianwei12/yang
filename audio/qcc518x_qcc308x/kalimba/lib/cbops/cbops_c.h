/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file cbops_c.h
 * \ingroup cbops
 *
 */

#ifndef _CBOPS_C_H_
#define _CBOPS_C_H_

#include "operators/cbops_copy_op_c.h"
#include "operators/cbops_shift_c.h"
#include "operators/cbops_dc_remove_c.h"
#include "operators/cbops_mute_c.h"
#include "operators/cbops_rate_adjustment_and_shift_c.h"
#include "log_linear_cbops/log_linear_cbops_c.h"
#include "operators/cbops_iir_resamplerv2_op.h"
#include "operators/cbops_sidetone_filter.h"
#include "operators/cbops_underrun_comp_op.h"
#include "operators/cbops_discard_op.h"
#include "operators/cbops_sink_overflow_disgard_op.h"
#include "operators/cbops_sidetone_mix_op.h"
#include "operators/cbops_howling_limiter_c.h"
#include "hl_limiter_struct.h"
#include "buffer.h"
#include "patch/patch.h"

/****************************************************************************
Public Constant Declarations
*/
#define CBOPS_NO_MORE_OPERATORS_PTR           ((cbops_op*)NULL)
#define CBOPS_BUFFER_NOT_SUPPLIED   (-1)

/* Maximum number of channels - for now until ever needing it differently,
     same number of max in/out channels are considered */
#define CBOPS_MAX_NR_CHANNELS       8
#define CBOPS_MAX_COPY_SIZE         1920 /* 20ms kick at 96kHz */

/* default max number of CBOPS operators in a graph/chain, will panic if exceeded.
   NB. use cbops_set_max_ops if necessary. */
#define CBOPS_DEFAULT_MAX_OPERATORS     12

/****************************************************************************
Public Type Declarations
*/
typedef enum cbops_buffer_type
{
   CBOPS_IO_SINK     = 0x0001,
   CBOPS_IO_SOURCE   = 0x0002,
   CBOPS_IO_INTERNAL = 0x0004,
   CBOPS_IO_SHARED   = 0x8000
}cbops_buffer_type;

typedef struct cbops_buffer
{
   cbops_buffer_type  type;
   tCbuffer          *buffer;
   void              *base;
   unsigned          size;
   void              *rw_ptr;
   unsigned          *transfer_ptr;       /* Pointer to shared amount to transfer - multi-channel optimisation */
   unsigned          transfer_amount;     /* Amount to transfer */
}cbops_buffer;

typedef struct cbops_functions
{
   void *reset;
   void *amount_to_use;
   void *process;
}cbops_functions;

/** The "header" of cbop parameter structure - this is followed by cbop-specific parameters */
typedef struct cbops_param_hdr
{
    /** Pointer to operator parameters */
    void *operator_data_ptr;
    /** Number of input channels at creation time */
    unsigned nr_inputs;
    /** Number of output channels at creation time */
    unsigned nr_outputs;
    /** Table of input, and then output channel indexes in buffer info table.
     *  This table has always nr_inputs + nr_outputs entries.
     *  This table may have zero length if there are zero in & out channels (see certain
     *  ops like HW rate monitor).
     */
    unsigned index_table[];
} cbops_param_hdr;


typedef struct cbops_op
{
   /** The next operator in the chain */
    struct cbops_op *prev_operator_addr;
    /** The next operator in the chain */
    struct cbops_op *next_operator_addr;
    /** The function table of this cbop operator */
    void *function_vector;

    /** cbop operator specific data fields - it begins with a cbops_param_hdr structure */
    unsigned parameter_area_start[];
} cbops_op;


/**
 * cbops_graph structure
 *
 * The cbops_graph structure describes a CBOPs graph

 */
typedef struct cbops_graph
{
    /** linked list of operators */
    cbops_op   *first;
    cbops_op   *last;
    /** Graph Support data*/
    unsigned        num_io;             /* number of buffers */
    unsigned        *override_data;     /* cbops override operator data*/
    cbops_functions override_funcs;     /* cbops override functions */
    unsigned        refresh_buffers;    /* Signal buffer reset required */
    unsigned        force_update;       /* Signal processing must occur */
    unsigned        max_ops;            /* max number of cbops operators*/
    /** Graph IO Table */
    cbops_buffer   buffers[];
}cbops_graph;

/****************************************************************************
Public Macro Declarations
*/

/* Pointer to where cbop parameters begin in cbops_op - this is where
 * cbops_param_hdr begins.
 */
#define CBOPS_OPERATOR_DATA_PTR(op)           (&((op)->parameter_area_start[0]))

/* Size of cbop parameter structure header: nr ins, nr outs, indexes for ins & outs */
#define CBOPS_PARAM_HDR_SIZE(ni, no)          (sizeof(cbops_param_hdr) + (ni + no)*sizeof(unsigned))


/** Used for getting the memory allocation size required for the specified multi-channel cbop operator.
 *  NOTE: the operator's create() function is to then take into account what extras are needed to be allocated
 *  on top of what below macro calculates.
 *  Example: a single channel dc removal cbop will arrive at the right size with below macro as there is one
 *  channel-specific algorithm parameter in its param struct definition. If it was created with more than one
 *  channel, then it has to add the size of the extra channels' parameters (in dc removal case, N-1 extra
 *  integers, as one channel is accounted for already).
 */
#define sizeof_cbops_op(op_name, ni, no) \
             (sizeof(cbops_op) + sizeof(op_name) + CBOPS_PARAM_HDR_SIZE(ni, no))

/* Pointer to cbop-specific parameter start (this is after cbops_param_hdr structure) */
#define CBOPS_PARAM_PTR(op, op_name) \
             ((op_name*)(((cbops_param_hdr*)CBOPS_OPERATOR_DATA_PTR(op))->operator_data_ptr))


/****************************************************************************
Public Function Declarations
*/
/**
 * Initialise buffer tables.
 *
 * Initialises the buffer tables when a chain is created.
 *
 * \param head pointer to the head of the cbops chain structure.
 * \param fw_obj framework internal object pointer.
 * \return none.
 */
extern void cbops_reinit_buffers(cbops_graph *graph);

/**
 * Executes cbops chain / graph.
 *
 * This function executes the cbops graph.
 *
 * \param graph pointer to cbops_graph object describing the graph.
 * \param max limit on the amount of data in samples to process.
 */
extern void cbops_process_data(cbops_graph* graph, unsigned max_amount);

/**
 * \brief Retrieves the amount of data written by a cbops graph run.
 *
 * \param index The index of a buffer for the graph
 *
 * \return The amount of data consumed/produced in buffer
 * cbops_process_data was called
 */
extern unsigned cbops_get_amount(cbops_graph* graph, unsigned index);

/**
 * Accessor functions
 *
 * Get or Set one of next items of private data obfuscated in io[] area of the head
 *      number of inputs / outputs
 *      pointer to I/O or scratch buffer
 *    for a chain
 *
 *  Caution. The accessors are low-level, fast access functions, assuming the head
 *          structure is valid, there are no checks.
 *
 * \param head pointer to the head of the cbops chain structure.
 * \return none or requested item for this chain
 */
extern unsigned cbops_get_num_inputs(cbops_graph *head);
extern unsigned cbops_get_num_outputs(cbops_graph *head);

/**
 * dynamically create a cbops graph
 *
 * \param number of io buffers in the graph
 * \return pointer to allocated graph
 */
cbops_graph* cbops_alloc_graph(unsigned num_io);

/**
 *  Set the maximum number of cbops operators in a cbops graph
 *
 * \param pointer to allocated graph
 * \param max number of cbops operators in the graph
 * \return none
 * Note. Could alter the default MAX_OPERATORS set at graph creation (cbops_alloc_graph).
 */
void cbops_set_max_ops(cbops_graph *graph, unsigned max_ops);

/**
 * destroy a cbops graph that was dynamically created
 *
 * \param pointer to graph
 * \return none
 */
void destroy_graph(cbops_graph *graph);

/**
 * destroy all operators in a graph
 *
 * \param pointer to graph
 * \return none
 */
void cbops_free_operators(cbops_graph *graph);

/**
 * associate an input buffer with a graph
 *
 * \param pointer to graph
 * \param buffer index in graph
 * \param linked buffer index in graph
 * \param pointer to buffer
 * \return NONE
 */
void cbops_set_input_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf);

/**
 * associate an output buffer with a graph
 *
 * \param pointer to graph
 * \param buffer index in graph
 * \param linked buffer index in graph
 * \param pointer to buffer
 * \return NONE
 */

void cbops_set_output_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf);

/**
 * associate an internal buffer with a graph
 *
 * \param pointer to graph
 * \param buffer index in graph
 * \param linked buffer index in graph
 * \param pointer to buffer
 * \return NONE
 */
void cbops_set_internal_io_buffer(cbops_graph *graph,unsigned index,unsigned share_index,tCbuffer *buf);


/**
 * append an operator to a graph
 *
 * \param pointer to graph
 * \param pointer to operator
 * \return NONE
 */
void cbops_append_operator_to_graph(cbops_graph *graph,cbops_op *op);

/**
 * pre-pend an operator to a graph
 *
 * \param pointer to graph
 * \param pointer to operator
 * \return NONE
 */
void cbops_prepend_operator_to_graph(cbops_graph *graph,cbops_op *op);


/**
 * set the override operator for the graph
 *
 * \param pointer to graph
 * \param pointer to operator
 * \return NONE
 */
void cbops_set_override_operator(cbops_graph *graph,cbops_op *op);

/**
 * removes a cbops operator from a cbops graph
 *
 * \param graph pointer to cbops graph (can't be NULL)
 * \param op pointer to cbops operator (can't be NULL)
 * \return NONE
 * NOTE: this will only remove the operator from the graph,
 *       any buffer index management that needed after removing
 *       the operator needs to be done by the user, e.g. when
 *       the removed op isn't an in-place op.
 */
extern void cbops_remove_operator_from_graph(cbops_graph *graph,cbops_op *op);

/**
 * inserts an operator into an existing cbops graph
 *
 * \param graph Pointer to cbops graph
 * \param op The operator to be inserted into the graph (can't be NULL)
 * \param after The insertion point operator, op will be inserted after this
 *        operator in the graph. It must be already existing in the graph and
 *        cannot be NULL.
 */
extern void cbops_insert_operator_into_graph(cbops_graph *graph,cbops_op *op, cbops_op *after);

/**
 * Makes a cbops buffer unused
 *
 * \param graph Pointer to cbops graph
 * \param index The index of the cbops buffer
 */
extern void cbops_unset_buffer(cbops_graph *graph,unsigned index);

/**
 * Set input / output channel buffer indexes
 *
 * \param op pointer to cbop
 * \param channel number of the input/output channel (0,...)
 * \param idx  index in buffer table
 * \return TRUE/FALSE depending on success
 */
extern bool cbops_set_input_idx(cbops_op* op, unsigned channel, unsigned idx);
extern bool cbops_set_output_idx(cbops_op* op, unsigned channel, unsigned idx);


/**
 * Signal buffer change
 *
 * \param op pointer to cbop
 */
static inline void cbops_refresh_buffers(cbops_graph *graph)
{
   graph->refresh_buffers = TRUE;
}


/**
 * Create and fill an index table with default indexes.
 *
 * \param nr_ins   Number of input channels at creation time.
 * \param nr_outs  Number of output channels at creation time.
 */
extern unsigned* create_default_indexes(unsigned nr_io);

/**
 * Fill the channel number & channel index-related header part of cbop param struct (multi-channel)
 *
 * \param op       Pointer to cbop param structure
 * \param nr_ins   Number of input channels at creation time.
 * \param nr_outs  Number of output channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 */
extern void* cbops_populate_param_hdr(cbops_op* op,
                                     unsigned nr_ins, unsigned nr_outs,
                                     unsigned* input_idx, unsigned* output_idx);

/* cbops operator specific support */

/**
 * create a G.711 loglinear operator (multi-channel)
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \param mapping_func  Address of the conversion function (u-law or A-law).
 * \return pointer to operator
 */
cbops_op* create_g711_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx, void* mapping_func);

/**
 * create a copy operator (multi-channel)
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \return pointer to operator
 */
cbops_op* create_copy_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx);

/**
 * create a DC offset removal operator (multi-channel)
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \return pointer to operator
 */
cbops_op* create_dc_remove_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx);


/**
 * create a cbops mute operator (multi-channel)
 *
 * \param nr_channels   Number of channels at creation time.
 * \param io_idx     Pointer to input/output channel indexes.
 * \return pointer to operator
 */
extern cbops_op* create_mute_op(unsigned nr_channels, unsigned* io_idx);

/**
 * configure cbops_mute for muting/unmuting
 *
 * \param op Pointer to cbops_mute operator structure
 * \param enable Any non-zero value will mute
 * \param no_ramp if enabled it will be immediate mute/unmute
 *        else a ramping will apply during first run.
 */
extern void cbops_mute_enable(cbops_op *op, bool enable, bool no_ramp);

/**
 * reset cbops_mute (to unmute state)
 *
 * \param op Pointer to cbops_mute operator structure
 */
extern void cbops_mute_reset(cbops_op *op);

/**
 * Create underrun compensation cbop (multi-channel). It must be at end of the chain,
 * it works only on "final" output buffers. It uses and tweaks endpoint-level parameters, too,
 * that are produced / used by RM (rate matching) calculations.
 *
 * \param nr_ins   Number of input channels at creation time, must match entire chain inputs.
 * \param nr_outs  Number of output channels at creation time.
 * \param input_idx       Pointer to input channel indexes (for consistency and future proofing - values are not used)
 * \param output_idx      Pointer to output channel indexes.
 * \param rm_diff_ptr     Pointer to rm_diff adjustment value used by owner entity, too.
 * \param block_size      Block size of owner endpoint (sink). Equates to data amount per kick period.
 * \param total_inserts_ptr   Pointer to sum total of inserted samples (discards will try to reduce this)
 * \param data_block_size_ptr Pointer to estimated data blockiness arriving to the chain
 * \param delta_samples_ptr   Pointer to sample amount delta, calculated by endpoint-level RM functions
 * \param insertion_vals_ptr  Pointer to a vector of nr_outs values used for insertion.
 *                            These may be last copied values on those channels, or just some constant values.
 *                            If NULL, then insertions are done with zero value.
 * \param sync_start_ptr      Pointer to data started state's flag.
 *
 * \return pointer to operator
 */
cbops_op* create_underrun_comp_op(unsigned nr_ins, unsigned nr_outs,
                                  unsigned* input_idx, unsigned* output_idx,
                                  int* rm_diff_ptr, unsigned *block_size_ptr,
                                  unsigned* total_inserts_ptr, unsigned *data_block_size_ptr,
                                  unsigned *delta_samples_ptr, unsigned* insertion_vals_ptr,
                                  bool *sync_start_ptr);

/**
 * Get key parameters from the cbop instance, which are used for inter-component communication, too.
 *
 * \param op                  Pointer to cbop param structure
 * \param rm_diff_ptr         Pointer to pointer to rm_diff adjustment value used by owner entity, too.
 * \param block_size_ptr      Pointer to block size of owner endpoint (sink). Equates to data amount per kick period.
 * \param total_inserts_ptr   Pointer to pointer to sum total of inserted samples (discards will try to reduce this)
 * \param data_block_size_ptr Pointer to pointer to estimated data blockiness arriving to the chain
 * \param delta_samples_ptr   Pointer to pointer to sample amount delta, calculated by endpoint-level RM functions
 * \param insertion_vals_ptr  Pointer to pointer to a vector of nr_outs values used for insertion.
 *                            These may be last copied values on those channels, or just some constant values.
 *                            If NULL, then insertions are done with zero value.
 * \param sync_start_ptr      Pointer to data started state's flag.
 *
 * \return TRUE/FALSE depending on success
 */
bool get_underrun_comp_op_vals(cbops_op *op, int **rm_diff_ptr, unsigned** block_size_ptr,
                               unsigned **total_inserts_ptr, unsigned **data_block_size_ptr,
                               unsigned **delta_samples_ptr, unsigned **insertion_vals_ptr,
                               bool **sync_start_ptr);

/**
 * Create input data discard cbop (multi-channel). It must be at start of the chain,
 * it works only on input buffers. It uses and tweaks endpoint-level parameters, too,
 * that are produced / used by RM (rate matching) calculations and the underrun compensator cbop.
 * It essentially has zero outputs, only ever works on the input buffers located at the input of the
 * owner entity (sink endpoint).
 *
 * \param nr_ins              Number of input channels at creation time, must match entire chain's inputs.
 * \param input_idx           Pointer to input channel indexes (for consistency and future proofing - values are not used)
 * \param block_size_ptr      Pointer to block size of owner endpoint (sink). Equates to data amount per kick period.
 * \param rm_headroom         RM headroom amount
 * \param total_inserts_ptr   Pointer to sum total of inserted samples (discards will try to reduce this)
 * \param data_block_size_ptr Pointer to estimated data blockiness arriving to the chain
 * \param sync_start_ptr      Pointer to data started state's flag.
 *
 * \return pointer to operator
 */
cbops_op* create_discard_op(unsigned nr_ins, unsigned* input_idx,
                            unsigned *block_size_ptr, unsigned rm_headroom,
                            unsigned* total_inserts_ptr, unsigned *data_block_size_ptr,
                            bool *sync_start_ptr);

/**
 * Get key parameters from the cbop instance, which are used for inter-component communication, too.
 *
 * \param op                  Pointer to cbop param structure
 * \param block_size_ptr      Pointer to pointer to block size of owner endpoint (sink).
 *                            Equates to data amount per kick period.
 * \param rm_headroom         Pointer to RM headroom amount
 * \param total_inserts_ptr   Pointer to pointer to sum total of inserted samples (discards will try to reduce this)
 * \param data_block_size_ptr Pointer to pointer to  estimated data blockiness arriving to the chain
 * \param sync_start_ptr      Pointer to data started state's flag.
 *
 * \return TRUE/FALSE depending on success
 */
bool get_discard_op_vals(cbops_op *op,
                         unsigned **block_size_ptr, unsigned *rm_headroom,
                         unsigned **total_inserts_ptr, unsigned **data_block_size_ptr,
                         bool **sync_start_ptr);

/**
 * create a SW rate adjustment operator (multi-channel)
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \param quality       ID for quality level (across all channels)
 * \param rate_val_addr pointer to rate adjustment variable (across all channels)
 * \param shift_amt     pow2 shift amount applied to output (across all channels)
 * \return pointer to operator
 */
cbops_op* create_sw_rate_adj_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx,
                                      unsigned quality, unsigned *rate_val_addr, int shift_amt);

/**
 * destroy a SW rate adjustment operator (multi-channel)
 *
 * \param op   pointer to operator
 */
void destroy_sw_rate_adj_op(cbops_op *op);


/**
 * \brief Enable/Disable passthrough mode of a rateadjust and shift cbop
 *
 * \param op The rateadjust and shift cbop
 * \param enable boolean indicating whether to enable or disable passthrough mode
 */
extern void cbops_rateadjust_passthrough_mode(cbops_op *op, bool enable);

/**
 * \brief Set the rate of a rate_adjust cbop.
 *
 * \param pointer to operator
 * \param rate to adjust to (fraction of nominal rate)
 * \return NONE
 */
extern void cbops_sra_set_rate_adjust(cbops_op *op, unsigned rate);

/**
 * \brief Get the current rate of a rate_adjust cbops op.
 *
 * \param pointer to operator
 * \return current rate for the cbops rate adjust operator
 */
extern unsigned cbops_sra_get_current_rate_adjust(const cbops_op *op);

/**
 * \brief Resets the sw rate_adjust cbop.
 *
 * \param pointer to operator
 * \return NONE
 */
extern void cbops_sra_reset(cbops_op *op, unsigned nr_channels);

/**
 * \brief Retrieve the phase of the next output sample
 * relative to the input samples
 *
 * \param pointer to operator
 * \return A fractional value from -1 to 0
 */
extern int cbops_sra_get_phase(const cbops_op *op);

/**
 * create resampler operator (multi-channel).
 * It acts on a chorus of channels in tandem, applying same resampling to all of them.
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \param input sample rate
 * \param output sample rate
 * \param size of interstage buffer
 * \param pointer to interstage buffer
 * \param pow2 scale of output
 * \param double precision flag   1:on, 0:off
 * \param low mips flag           1:on, 0:off
 * \return pointer to operator
 */
cbops_op* create_iir_resamplerv2_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx,
                                          unsigned in_rate, unsigned out_rate,
                                          unsigned inter_stage_size, unsigned *inter_stage,
                                          int shift, unsigned dbl_precision, unsigned low_mips);
/**
 * destroy resampler operator
 *
 * \param pointer to operator
 * \return NONE
 */
void      destroy_iir_resamplerv2_op(cbops_op *op);

/**
 * create a sample insert  HW rate adjustment rate monitor operator
 *
 * \param nr_channels   Number of channels at creation time.
 * \param input_idx     Pointer to input channel indexes.
 * \param output_idx    Pointer to output channel indexes.
 * \param threshold     Comparison threshold amount.
 * \return pointer to operator
 */
cbops_op* create_insert_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx, unsigned threshold);

/**
 * create mix operator
 * NOTE: As it stands, it specifically does 2-to-1 downmixing. This can be generalised as/when needed to N-to-1 downmixing,
 * at which point it would take arrays of indexes.
 *
 * \param input1 index
 * \param input2 index
 * \param output index
 * \param shift pow2 scalling applied to output
 * \param mix_ratio  fraction mix ratio for channels
 * \return pointer to operator
 */
cbops_op* create_mixer_op(unsigned input1_idx,unsigned input2_idx,unsigned output_idx,unsigned shift,unsigned mix_ratio);

/**
 * create external IO latency control and wrap protection operation.
 * NOTE: Due to its particularities, it can be modelled as a cbop with no channels or indexes, it just receives
 * some buffer pointers that may not be at all in framework object's buffer info table.
 *
 * \param nr_channels Number of output buffer pointers it receives.
 * \param out_bufs    Pointer to some output buffers
 * \param latency threshold
 * \return pointer to operator
 */
cbops_op* create_port_wrap_op(unsigned nr_out_bufs, unsigned out_bufs[], unsigned threshold);

/**
 * create copy shift operator (multi-channel)
 *
 * \param nr_channels Number of channels at creation time.
 * \param input_idx   Pointer to input channel indexes.
 * \param output_idx  Pointer to output channel indexes.
 * \param shift_amt   Shift amount
 * \return pointer to operator
 */
cbops_op* create_shift_op(unsigned nr_channels, unsigned* input_idx, unsigned* output_idx, int shift_amt);

/**
 * create sidetone filter operator (fits into multi-channel model, but works on single channel always)
 *
 * \param input index (it only ever works on one channel)
 * \param output index (it only ever works on one channel)
 * \param maximum support filter stages
 * \param pointer to sidetone parameters
 * \param pointer to filter parameters
 * \return pointer to operator
 */
cbops_op* create_sidetone_filter_op(unsigned input_idx, unsigned output_idx,
                                          unsigned max_stages, cbops_sidetone_params *st_params,
                                          unsigned *peq_params);
/**
 * set step for ramping of sidetone gain
 *
 * \param op sidetone filter cbops operator
 * \param exp_ramp exponential ramping parameter, it is the desired growth of sidetone gain in 10ms,
 *     pass FRACTIONAL(10**(dB_per_sec/2000.0) - 1.0), or 0 if no exponential ramping required.
 * \param lin_ramp linear ramping parameter, it is desired sidetone linear gain increase in 10ms,
 *     pass FRACTIONAL(full_scale_percent_per_sec/10000.0), or 0 if no linear ramping required.
 * \param run_period running period for filter in microseconds,
 *     Note: operator is expected to run at regular intervals and for accuracy of ramping speed
 *     this is not expected to exceed 10ms.
 */
void cbops_sidetone_filter_set_ramping_step(cbops_op *op, unsigned exp_ramp, unsigned lin_ramp, unsigned run_period);

/**
 * reset the sidetone filter
 *
 * \param pointer to operator
 * \return NONE
 */
void      initialize_sidetone_filter_op(cbops_op *op);

/**
 * update the sidetone mode
 *
 * \param pointer to operator
 * \param mode flags
 * \param current measured noise level for noise switch of mode
 * \return NONE
 */
void      update_sidetone_filter_op(cbops_op *op,unsigned enable,unsigned noise_level);

/**
 * create_sidetone_mix_op
 * \brief creates single channel sidetone mix operator
 * \param input_idx cbops buffer index for main input channel
 * \param output_idx cbops buffer index for main output channel
 * \param st_in_idx cbops buffer index for sidetone channel
 * \param threshold threshold for sidetone latency
 * \return pointer to operator
 */
cbops_op* create_sidetone_mix_op(unsigned input_idx,
                                 unsigned output_idx,
                                 unsigned st_in_idx,
                                 unsigned threshold);

/**
 * create_multichan_sidetone_mix_op_base
 * \brief
 *    create multi channel sidetone mix operator
 *    NOTE: This function however wont map any main channel to a particular
 *          sidetone input, after creation use cbops_sidetone_mix_map_channel
 *          to tell the operator which sidetone buffer should mix into a main
 *          channel.
 *
 * \param nr_channels number of main channels
 *
 * \param input_idxs array of cbops buffer indexes for main input channels
 *
 * \param output_idxs array of cbops buffer indexes for main output channels
 *   NOTES:
 *        1- This operator can work in-place, i.e. same input_idxs and output_idxs
 *        2- each input channel is copied to its corresponding output,
 *           after mixing with a sidetone input if configured.
 *
 * \param nr_st_channels number of sidetone channels, practically this shall not
 *   be needed to exceed nr_channels.
 *
 * \param st_in_idxs array of cbops buffer indexes for sidetone channels
 *   NOTES:
 *        1- each sidetone channel can be mixed into any number of main channels.
 *        2- each main channel can be mixed into only one or none sidetone channel.
 *
 * \param threshold threshold for sidetone latency
 * \param adjust_level for latency control, if sidetone buffer under-runs up to
 *        this value will be inserted to adjust latency working point. 0 will
 *        insert minimum needed.
 * \return pointer to operator
 */
cbops_op* create_multichan_sidetone_mix_op_base(unsigned nr_channels,
                                                unsigned *input_idxs,
                                                unsigned *output_idxs,
                                                unsigned nr_st_channels,
                                                unsigned *st_in_idxs,
                                                unsigned threshold,
                                                unsigned adjust_level);

/**
 * cbops_sidetone_mix_map_channel
 * \brief configures a main channel of the operator to use a sidetone channel for mixing
 * \param op cbops multichannel sidetone mix operator (input)
 * \param input_channel main channel number (0 to (nr_channels-1))
 * \param use_st_channel use this sidetone channel (0 to (nr_st_channels-1))
 */
void cbops_sidetone_mix_map_channel(cbops_op *op, unsigned input_channel, unsigned use_st_channel);

/**
 * cbops_sidetone_mix_map_one_to_all
 * \brief configures a sidetone channel to be mixed into all main channels
 * \param op cbops multichannel sidetone mix operator (input)
 * \param use_st_channel use this sidetone channel to mix into all channels
 */
void cbops_sidetone_mix_map_one_to_all(cbops_op *op, unsigned use_st_channel);

/**
 * cbops_sidetone_mix_map_one_to_one
 * \brief configures one to one sidetone mixing, i.e each main channel
 *        will be mixed into same-number sidetone channel.
 * \param op cbops multichannel sidetone mix operator (input)
 */
void cbops_sidetone_mix_map_one_to_one(cbops_op *op);

/**
 * create_multichan_sidetone_mix_op
 * \brief create multi channel sidetone mix operator
 *  Note:  This is same as create_multichan_sidetone_mix_op_base just needs index
 *    of first channel.
 *
 * \param nr_channels number of main channels *
 * \param first_input_idx buffer index for first main input channel, others
 *        are expected to be consecutive.
 * \param first_output_idx buffer indexer for first main output channel, others
 *        are expected to be consecutive.
 * \param nr_st_channels number of sidetone channels *
 * \param first_st_in_idx buffer index for first sidetone channel, others
 *        are expected to be consecutive.
 *
 * \param threshold threshold for sidetone latency
 * \param adjust_level for latency control, if sidetone buffer under-runs up to
 *        this value will be inserted to adjust latency working point. 0 will
 *        insert minimum needed.
 *
 * \return pointer to operator
 * \return pointer to operator
 */
cbops_op* create_multichan_sidetone_mix_op(unsigned nr_channels,
                                           unsigned first_input_idx,
                                           unsigned first_output_idx,
                                           unsigned nr_st_channels,
                                           unsigned first_st_in_idx,
                                           unsigned threshold,
                                           unsigned adjust_level);

/**
 * create rate monitor operator (multi-channel model, but it acts as a cbop with zero in & out channels)
 *
 * \param clocks per second
 * \return pointer to operator
 */
cbops_op* create_rate_monitor_operator(unsigned clk_per_sec,unsigned idx);

/**
 * reset SW rate monitor operator
 *
 * \param pointer to operator
 * \param expected sample rate
 * \param HW=true/SW=false select
 * \param measurement period in msec
 * \return NONE
 */
void rate_monitor_op_initialise(cbops_op *op, unsigned target,bool bHwSw,unsigned meas_period_msec);

/**
 * get rate monitor operator computed rate adjustment
 *
 * \param pointer to operator
 * \param direction 0=source,1=sink
 * \return rate adjustment
 */
int rate_monitor_op_get_rate(cbops_op *op,unsigned dir);
/**
 * rate_monitor_op_restart
 * \param pointer to operator
 * \return NONE
 */

void rate_monitor_op_restart (cbops_op *op);

/**
 * rate_monitor_op_is_complete
 * \param pointer to operator
 * \return measurement complete indication
 */

int rate_monitor_op_is_complete (cbops_op *op);

/**
 * destroy an interleave operator (multi-channel)
 *
 * \param op   pointer to operator
 */
void      destroy_interleave_op(cbops_op *op);


/**
 * create operator to drop data at sink if insufficient space
 *
 * \param nr_channels   Number of channels at creation time.
 * \param output_idx    Pointer to output channel indexes.
 * \param min_space     Comparison threshold amount.
 * \return pointer to operator
 */
cbops_op* create_sink_overflow_disgard_op(unsigned nr_channels,unsigned* output_idx,unsigned min_space);

/**
 * return number of samples dropped by operator
 *
 * \param pointer to operator
 * \return number of drops
 */
unsigned get_sink_overflow_disgard_drops(cbops_op *op);

void rate_monitor_simple_reset(cbops_op *op);

unsigned get_cbops_insert_op_inserts(cbops_op *op);

unsigned get_rate_monitor_last_acc(cbops_op *op);

/**
 * configure rate monitor operator to receive new amount
 * directly.
 *
 * \param op cbops rate monitor operator
 * \param new_amount Pointer containing new amount for the operator
 *  if not NULL, the operator will read new amount from this address
 *  rather than using the cbops transfer amount.
 */
void set_rate_monitor_new_amount_ptr(cbops_op *op, const void *new_amount);

/**
 * create operator to minimise/prevent "howling"
 *
 * \param input_idx             Pointer to the input channel index.
 * \param mic_sampling_rate     Microphone sampling rate.
 * \param hl_ui                 Howling Limiter user interface parameter structure
 *
 * \return pointer to operator
 */
cbops_op *create_howling_limiter_op(
    unsigned input_idx,
    unsigned mic_sampling_rate,
    HL_LIMITER_UI *hl_ui);

/**
 * Initialize the howling limiter operator
 *
 * \param op                    Pointer to the operator structure.
 * \param mic_sampling_rate     Microphone sampling rate.
 * \param hl_ui                 Howling Limiter user interface parameter structure
 *
 * \return none
 */
void initialize_howling_limiter_op(
    cbops_op *op,
    unsigned mic_sampling_rate,
    HL_LIMITER_UI *hl_ui);

/**
 * Configure the howling limiter operator
 *
 * \param op                    Pointer to the operator structure.
 * \param mic_sampling_rate     Microphone sampling rate.
 * \param hl_ui                 Howling Limiter user interface parameter structure
 *
 * \return none
 */
void configure_howling_limiter_op(
    cbops_op *op,
    unsigned mic_sampling_rate,
    HL_LIMITER_UI *hl_ui);

#endif      /* _CBOPS_C_H_ */
