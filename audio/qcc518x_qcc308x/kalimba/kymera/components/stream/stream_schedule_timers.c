/****************************************************************************
 * Copyright (c) 2011 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_schedule_timers.c
 * \ingroup stream
 *
 * stream timer scheduler. <br>
 * This file contains stream functions for setting up transform
 * timers. <br>
 *
 * \section sec1 Contains:
 *  <br>
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "platform/pl_trace.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Public Function Definitions
*/


/****************************************************************************
 *
 * stream_schedule_real_endpoint
 *
 */
void stream_schedule_real_endpoint(ENDPOINT *ep)
{
    KICK_OBJECT *ko;
    ENDPOINT_TIMING_INFORMATION connected_timing;

    /* Software patchpoint just in case */
    patch_fn_shared(stream_schedule_timers);

    /* Make sure the endpoint looks sensible */
    if (!ep || !ep->is_real)
    {
        panic(PANIC_AUDIO_INVALID_ENDPOINT_IN_SCHEDULE_CONTEXT);
    }

    /* In case of an audio endpoint only the head of sync gets kicked. */
    if (IS_AUDIO_ENDPOINT(ep) && !IS_ENDPOINT_HEAD_OF_SYNC(ep))
    {
        return;
    }

    ko = kick_obj_from_sched_endpoint(ep);
    if (ko == NULL)
    {
        /* Start by assuming that this endpoint is both responsible for
         * scheduling and the thing that gets kicked.
         */
        ko = kick_obj_create(ep, ep);
    }

    if (ep->connected_to != NULL)
    {
        /* Tell the endpoint the connected block size, in case it cares */
        ep->connected_to->functions->get_timing_info(ep->connected_to,
                                                    &connected_timing);
        ep->functions->configure(ep, EP_BLOCK_SIZE, connected_timing.block_size);
    }
}

/****************************************************************************
 *
 * stream_enable_endpoint
 *
 */
void stream_enable_endpoint(ENDPOINT *ep)
{
    ENDPOINT *connected = ep->connected_to;
    ENDPOINT *head_of_sync_ep;

    patch_fn_shared(stream_schedule_timers);
    /* Check that the transform exists, it is a valid case for an operator
       to signal a start to an unconnected operator terminal */
    if (connected != NULL)
    {
        KICK_OBJECT *ko;
        connected->is_enabled = TRUE;
        /* Previously, this was specially handled for audio & file with knowledge
         * that they could be synchronised and should only be started when all
         * synchronised endpoints are connected and only the endpoint scheduling
         * the kick needs to be started. But the endpoints should take care of
         * that. As long as a (group of) endpoint(s) have a kick object, all the
         * endpoints in the group should be started.
         */
        head_of_sync_ep = stream_get_head_of_sync(connected);
        ko = kick_obj_from_sched_endpoint(head_of_sync_ep);
        if (ko != NULL || connected != head_of_sync_ep)
        {
            connected->functions->start(connected, ko);
        }
    }

}

/****************************************************************************
 *
 * stream_disable_endpoint
 *
 */
void stream_disable_endpoint(ENDPOINT *ep)
{
    ENDPOINT *connected = ep->connected_to;

    patch_fn_shared(stream_schedule_timers);

    /* Check that the transform exists, it is a valid case for an operator
       to signal a stop to an unconnected operator */
    if (connected != NULL)
    {
        connected->functions->stop(connected);
        connected->is_enabled = FALSE;
    }
}

