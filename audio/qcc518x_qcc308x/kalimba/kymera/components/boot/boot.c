/****************************************************************************
 * Copyright (c) 2010 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file boot.c
 * \ingroup boot
 *
 * Boot functionality executed just after the environment has been setup
 * to support functions written in C.
 */

/****************************************************************************
Include Files
*/

#include "boot/boot.h"
#include "sched_oxygen/sched_oxygen.h"
#include "patch/patch.h"
#include "pmalloc/pl_malloc.h"
#include "platform/pl_error.h"
#include "pl_timers/pl_timers.h"
#include "platform/stack_c.h"
#include "platform/profiler_c.h"
#include "hal/hal.h"
#include "hal/hal_dm_sections.h"
#include "id/id.h"
#if defined(INSTALL_CLK_MGR)
#include "clk_mgr/clk_mgr.h"
#endif
#include "fault/fault.h"
#include "platform/pl_interrupt.h"

#if defined(RUNNING_ON_KALSIM) && defined(FIRMWARE_TALKING_TO_KALCMD)
#include "kalsim_msgif/kalsim_msgif.h"
#endif
#include "submsg/submsg.h"
#include "hydra_sssm/sssm_ss.h"

#ifdef INSTALL_CAP_DOWNLOAD_MGR
#include "capability_database.h"
#include "cap_download_mgr/cap_download_mgr.h"
#endif

#ifdef INSTALL_AUDIO_DATA_SERVICE
#include "audio_data_service/audio_data_service.h"
#endif

#if defined(SUPPORTS_MULTI_CORE)
#include "kip_mgr/kip_mgr.h"
#endif

#if defined(SUPPORTS_LICENSING)
#include "../../lib_private/security/security_library_c.h"
#endif
#include "sys_events/sys_events.h"

#ifdef INSTALL_AOV
#include "aov/aov_task/aov_task.h"
#endif

/****************************************************************************
Private Variable Definitions
*/

/* Actually starts at a much smaller value until the first stack switch in main. */
static unsigned current_stack_size;
static char *current_stack = NULL;
static unsigned *requested_stack_size = NULL;
DM_SHARED static unsigned requested_stack_sizes[PROC_PROCESSOR_BUILD] =
{
    FORCE_STACK_SIZE_WORDS * sizeof(unsigned),
#if defined(SUPPORTS_MULTI_CORE)
    FORCE_STACK_SIZE_WORDS * sizeof(unsigned)
#endif
};

/****************************************************************************
Private Function Definitions
*/

#if defined(SUPPORTS_MULTI_CORE)
/**
 * \brief Initialization function specific to secondary cores.
 *
 * This is executed before starting the scheduler.
 */
static void cpux_main(PROC_ID_NUM proc_id)
{
    /* initialise KiP mgr. The kip mgr will set up communication with P0 */
    kip_init();

    /* Enable error handlers for Kalimba libs.
     * These all end up in panic so probably not useful to
     * do this any earlier. */
    error_enable_exception_handlers(TRUE);

    config_pmalloc();

#ifdef INSTALL_CAP_DOWNLOAD_MGR
    cap_download_mgr_init();
    capability_database_init_download_list();
#endif /* INSTALL_CAP_DOWNLOAD_MGR */

    /**
     * Initialise the fault system here because on P0 it is initialised by Hydra
     * SSSM which does not run other on P1.
     */
    init_fault();

#if defined(SUPPORTS_LICENSING)
    /* On Px, all security requests will be forwarded to P0 */
    security_init();
#endif
}
#else
#define cpux_main(x) 0
#endif /* defined(SUPPORTS_MULTI_CORE) */

/**
 * \brief Initialization function specific to primary core.
 *
 * This is executed before starting the scheduler.
 */
static void cpu0_main(void)
{
#ifdef INSTALL_PIO_SUPPORT
    /* Initialise PIO system - done early incase the
       rest of the boot depends on pio state */
    init_pio();
#endif

#if defined(SUPPORTS_LICENSING)
    security_init();
#endif

    init_system_event();

    /* Initialise the MMU hardware */
    mmu_buffer_initialise();

#if defined(RUNNING_ON_KALSIM) && defined(FIRMWARE_TALKING_TO_KALCMD)
    kalcmd_configure_communication();
#else
    /* Normal Hydra subsystem initialisation */
    submsg_init();
#endif

    /* This just queues a message to start the SSSM process off.
     * Nothing substantial happens (e.g., patch download) until the
     * scheduler actually starts running (sched() below). */
    sssm_init();

#ifdef INSTALL_AUDIO_DATA_SERVICE
    audio_data_service_init();
#endif

#ifdef INSTALL_AOV
    aov_init();
#endif

    /* Enable error handlers for Kalimba libs.
     * These all end up in panic so probably not useful to do this any earlier. */
    error_enable_exception_handlers(TRUE);
}

/****************************************************************************
Public function definitions
*/

/**
 * \brief Finish boot process
 *
 * The function is called once the system has been configured.
 *
 * \param sizes Pointer to an array of PROC_PROCESSOR_BUILD elements
 *              representing the desired size in octets of the stack.
 *              The sizes must be a multiple of 4 octets.
 */
void boot_configured(const BOOT_STACK_SIZES *sizes)
{
    memcpy(requested_stack_sizes, &sizes->array[0], sizeof(requested_stack_sizes));

    if (current_stack_size != *requested_stack_size)
    {
        sched_stop();
    }
}

/**
 * \brief  Startup entry function, which initialises & runs platform scheduler
 *
 * \return Ideally scheduler should run for ever(!).
 *
 * Initialises IRQ and scheduler and runs the scheduler.
 */
int main(void)
{
    PROC_ID_NUM proc_id;
    char *new_stack;
    size_t stack_size;

    /* Common hardware initialisation */
    hal_init();

#ifdef INSTALL_CLK_MGR
    clk_mgr_init();
#endif

    /* Initialise pmalloc module on all cores */
    init_pmalloc();

    /* This cannot be moved into a separate function as "set_stack_regs" can
       only be called from "main". */
    proc_id = proc_get_processor_id();
    requested_stack_size = &requested_stack_sizes[proc_id];
    current_stack_size = *requested_stack_size;
    current_stack = pmalloc(current_stack_size);
    set_stack_regs(current_stack, current_stack_size);

    /* Initialise IRQs and the scheduler */
    interrupt_initialise();

    /* Init scheduler */
    init_sched();
    init_pl_timers();

    if (proc_id != PROC_PROCESSOR_0)
    {
        cpux_main(proc_id);
    }
    else
    {
        cpu0_main();
    }

    PROFILER_INIT();

    /* Run the scheduler with the default large stack.
       The subsystem will get its configuration from the rest of the chip
       and apply it in function "sssm_init_operational". Once this is done,
       function "boot_configured" will be called that will check if the
       configuration has requested a different stack size. If so, the scheduler
       will return. */
    sched();

    patch_fn_shared(boot);

    /* This cannot be moved into a separate function as "set_stack_regs" can
       only be called from "main". */
    stack_size = *requested_stack_size;
    new_stack = pmalloc(stack_size);
    set_stack_regs(new_stack, stack_size);
    pfree(current_stack);
    current_stack_size = stack_size;
    current_stack = new_stack;

    /* This time, the scheduler should never return. */
    sched();

    return 0;
}

