/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */

/**
 * \file
 *
 * For registering "listener" tasks for the different IPC message types.
 */

#include "assert.h"
#include "hydra/hydra_macros.h"
#include "trap_api/task_registry.h"

/**
 * Look up table for the Task associated with an IPC_MSG_TYPE.
 */
Task registered_hdlrs[NUM_IPC_MSG_TYPES];

Task registered_pio_hdlrs[PIODEBOUNCE_NUMBER_OF_GROUPS];

Task task_registry_register(IPC_MSG_TYPE id, Task task)
{
    Task old;

    assert(id < ARRAY_DIM(registered_hdlrs));

    old = registered_hdlrs[id];
    registered_hdlrs[id] = task;

    return old;
}

Task pio_task_registry_register(Task task, uint16 group)
{
    Task old;
    uint32 i;

    if (group >= PIODEBOUNCE_NUMBER_OF_GROUPS)
    {
        old = registered_pio_hdlrs[0];
        for(i = 0; i < PIODEBOUNCE_NUMBER_OF_GROUPS; ++i)
        {
            registered_pio_hdlrs[i] = task;
        } 
    }
    else
    {
        old = registered_pio_hdlrs[group];
        registered_pio_hdlrs[group] = task;
    }
    return old;
}

void task_registry_remove(Task task)
{
    uint32 i;

    /* Interrupts must be blocked as we would want a new task being registered
       for a message to always succeed, even if the old task was being removed
       at the same time. */
    block_interrupts();
    {
        for(i = 0; i < NUM_IPC_MSG_TYPES; ++i)
        {
            if(task == registered_hdlrs[i])
            {
                /* If interrupts aren't blocked between the comparison and
                   setting to NULL then it's possible a new task could be
                   registered then removed by this next line. */
                registered_hdlrs[i] = NULL;
            }
        }
    }
    unblock_interrupts();
}
