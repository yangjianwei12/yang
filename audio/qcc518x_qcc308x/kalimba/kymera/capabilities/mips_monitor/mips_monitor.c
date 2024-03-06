/****************************************************************************
 * Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd 
****************************************************************************/
/**
 * \file  mips_monitor.c
 * \ingroup  capabilities
 *
 *  A Stub implementation of a Capability that can be built and communicated
 *  with. This is provided to accelerate the development of new capabilities.
 *
 */

#include "capabilities.h"
#include "proc/proc.h"
#include "mips_monitor.h"


/****************************************************************************
Private Function Definitions
*/
static void mips_monitor_timer_task(void *kick_object);
static bool mips_monitor_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
static bool mips_monitor_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data);
/****************************************************************************
Private Constant Declarations
*/
#define NUM_RUN_CLKS_ADDR (0xFFFFE03CU)       // address of cycle counter
#define NUM_INSTRS_ADDR (0xFFFFE040U)         // address of instrcutions counter
#define NUM_CORE_STALLS_ADDR (0xFFFFE044U)    // address of core stall counter
#define NUM_MEM_ACCESS_STALLS (0xFFFFE048U)   // address of mem access stall counter
#define NUM_INSTR_EXPAND_STALLS (0xFFFFE04CU) // address of instruction expansion stall counter
#define NUM_TIMER_TIME (0xFFFFE0E0U)          // address of timer

#define DBG_COUNTERS_EN_ADDR (0xFFFFE050U) // address of register to enable/disable kpi counters

#define START_KALIMBA_KPI_COUNTERS (*((uint32 *)DBG_COUNTERS_EN_ADDR)) = 1
#define STOP_KALIMBA_KPI_COUNTERS (*((uint32 *)DBG_COUNTERS_EN_ADDR)) = 0

#define cycles() (*((volatile uint32 *)NUM_RUN_CLKS_ADDR))
#define inst() (*((volatile uint32 *)NUM_INSTRS_ADDR))
#define core_stalls() (*((volatile uint32 *)NUM_CORE_STALLS_ADDR))
#define mem_stalls() (*((volatile uint32 *)NUM_MEM_ACCESS_STALLS))
#define inst_exp_stalls() (*((volatile uint32 *)NUM_INSTR_EXPAND_STALLS))
#define timer() (*((volatile uint32 *)NUM_TIMER_TIME))

#define MIPS_MONITOR_ID  0x40DF /* CHANGE THIS VALUE TO THAT SELECTED */

#define MONITOR_PERIOD (2000*1000)

#define USE_STRICT_TIMER TRUE
/** The stub capability function handler table */
const handler_lookup_struct mips_monitor_handler_table =
{
    mips_monitor_create,           /* OPCMD_CREATE */
    mips_monitor_destroy,          /* OPCMD_DESTROY */
    base_op_start,            /* OPCMD_START */
    base_op_stop,             /* OPCMD_STOP */
    base_op_reset,            /* OPCMD_RESET */
    base_op_connect,          /* OPCMD_CONNECT */
    base_op_disconnect,       /* OPCMD_DISCONNECT */
    base_op_buffer_details,   /* OPCMD_BUFFER_DETAILS */
    base_op_get_data_format,  /* OPCMD_DATA_FORMAT */
    base_op_get_sched_info    /* OPCMD_GET_SCHED_INFO */
};

/* Null terminated operator message handler table - this is the set of operator
 * messages that the capability understands and will attempt to service. */
const opmsg_handler_lookup_table_entry mips_monitor_opmsg_handler_table[] =
{
    {OPMSG_COMMON_ID_GET_CAPABILITY_VERSION, base_op_opmsg_get_capability_version},
    {0, NULL}
};


/* Capability data - This is the definition of the capability that Opmgr uses to
 * create the capability from. */
const CAPABILITY_DATA mips_monitor_cap_data =
{
    MIPS_MONITOR_ID,             /* Capability ID */
    0, 1,                           /* Version information - hi and lo parts */
    0, 0,                           /* Max number of sinks/inputs and sources/outputs */
    &mips_monitor_handler_table, /* Pointer to message handler function table */
    mips_monitor_opmsg_handler_table,    /* Pointer to operator message handler function table */
    NULL,           /* Pointer to data processing function */
    0,                              /* TODO: this would hold processing time information */
    sizeof(MIPS_MONITOR_OP_DATA)       /* Size of capability-specific per-instance data */
};
MAP_INSTANCE_DATA(MIPS_MONITOR_ID, MIPS_MONITOR_OP_DATA)

/* Accessing the capability-specific per-instance data function */
static inline MIPS_MONITOR_OP_DATA *get_instance_data(OPERATOR_DATA *op_data)
{
    return (MIPS_MONITOR_OP_DATA *) base_op_get_instance_data(op_data);
}

extern uint32 hal_get_runclks(void);

static void mips_monitor_timer_task(void *kick_object)
{
    MIPS_MONITOR_OP_DATA *mips_monitor_data = get_instance_data((OPERATOR_DATA*)kick_object);
    uint32 mips_report_msg[5];
    uint32 run_clks;
    TIME time;
    uint32 clks_delta, time_delta;

    run_clks = cycles();
    time = time_get_time();

    time_delta = time - mips_monitor_data->last_time;
    clks_delta = run_clks - mips_monitor_data->last_run_clks;

    L0_DBG_MSG2("MIPS monitor, %d CPU clock cycles in last %d us", clks_delta, time_delta);
#ifdef USE_STRICT_TIMER
    mips_monitor_data->timer_id = timer_schedule_event_in(MONITOR_PERIOD, mips_monitor_timer_task, kick_object);
#else
    mips_monitor_data->timer_id = timer_schedule_bg_event_in(MONITOR_PERIOD, mips_monitor_timer_task, kick_object);
#endif

    mips_monitor_data->last_time = time;
    mips_monitor_data->last_run_clks = run_clks;

    mips_report_msg[0] = proc_get_processor_id();
    mips_report_msg[1] = time_delta >> 16;
    mips_report_msg[2] = time_delta & 0xFFFF;
    mips_report_msg[3] = clks_delta >> 16;
    mips_report_msg[4] = clks_delta & 0xFFFF;

    common_send_unsolicited_message(kick_object, 0x70, 4, mips_report_msg);
}


static bool mips_monitor_create(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    MIPS_MONITOR_OP_DATA *mips_monitor_data = get_instance_data(op_data);

    L0_DBG_MSG("MIPS monitor, create");

    START_KALIMBA_KPI_COUNTERS;

    mips_monitor_data->last_run_clks = cycles();
    mips_monitor_data->last_time = time_get_time();

    /* call base_op create, which also allocates and fills response message */
    if (!base_op_create_lite(op_data, response_data))
    {
        return FALSE;
    }

#ifdef USE_STRICT_TIMER
    mips_monitor_data->timer_id = timer_schedule_event_in(MONITOR_PERIOD, mips_monitor_timer_task,(void*)op_data);
#else
    mips_monitor_data->timer_id = timer_schedule_bg_event_in(MONITOR_PERIOD, mips_monitor_timer_task, (void*)op_data);
#endif

    return TRUE;
}

static bool mips_monitor_destroy(OPERATOR_DATA *op_data, void *message_data, unsigned *response_id, void **response_data)
{
    MIPS_MONITOR_OP_DATA *mips_monitor_data = get_instance_data(op_data);

    L0_DBG_MSG("MIPS monitor, destroy");

    STOP_KALIMBA_KPI_COUNTERS;

    timer_cancel_event_atomic(&mips_monitor_data->timer_id);
    /* call base_op destroy that creates and fills response message, too */
    return base_op_destroy_lite(op_data, response_data);
}
