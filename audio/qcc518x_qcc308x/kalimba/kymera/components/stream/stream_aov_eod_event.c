/****************************************************************************
 * Copyright (c) 2012 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_aov_eod_event.c
 * \ingroup stream
 *
 * Code for setting up and handling the AoV Empty on Demand events.
 */

/****************************************************************************
Include Files
*/
#include "stream_private.h"
#include "platform/pl_trace.h"
#include "audio/audio.h"
#include "hal/hal_audio.h"

/****************************************************************************
Private Variable Definitions
*/
static void stream_aov_eod_service_routine(void* kick_object);

/****************************************************************************
Private Function Definitions
*/
/**
 * \brief AoV EoD interrupt call back routine that gets called from
 * the hal_audio_aov_int_handler
 *
 * \param kick_object kick object associated with the AoV EoD instance
 *
 */
void stream_aov_eod_service_routine(void* kick_object)
{
    KICK_OBJECT* ko = (KICK_OBJECT*)kick_object;
    PL_PRINT_P0(TR_STREAM, "stream_aov_eod_service_routine\n");

    /* Need to check whether our kick object is NULL, because unfortunate
     * timing could see an interrupt being handled just after we disabled
     * the EoD event handling. */
    if (ko != NULL)
    {
        /* We need to kick the kick object, not the endpoint, because it's
         * the kick object which knows who has responsibility for the chain.
         */
        kick_obj_kick(ko);
    }
}

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
 *
 * stream_aov_eod_event_enable
 *
 */
bool stream_aov_eod_event_enable(ENDPOINT* ep, KICK_OBJECT *ko)
{
    unsigned int instance;
    SID sid;

    /* Software patchpoint just in case */
    patch_fn_shared(stream);

    sid = stream_external_id_from_endpoint(ep);
    instance = audio_vsm_get_aov_instance_from_sid(sid);

    PL_PRINT_P2(TR_STREAM, "stream_aov_eod_event_enable: endpoint 0x%x AoV instance %u \n",
                sid, instance);

    return hal_aov_eod_enable(instance,
                              stream_aov_eod_service_routine,
                              (void*)ko);
}

/****************************************************************************
 *
 * stream_aov_eod_event_disable
 *
 */
bool stream_aov_eod_event_disable(ENDPOINT* ep)
{
    unsigned int instance;
    SID sid;

    /* Software patchpoint just in case */
    patch_fn_shared(stream);

    sid = stream_external_id_from_endpoint(ep);
    instance = audio_vsm_get_aov_instance_from_sid(sid);

    PL_PRINT_P2(TR_STREAM, "stream_aov_eod_event_disable: endpoint 0x%x AoV instance %u \n",
                sid, instance);

    return hal_aov_eod_disable(instance);
}
