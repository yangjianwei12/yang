/****************************************************************************
 * Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_iso_interface.c
 * \ingroup stream
 *
 * stream iso type file. <br>
 * This file contains stream functions for iso endpoints. <br>
 *
 * \section public Contains:
 * stream_iso_get_endpoint <br>
 * stream_create_iso_endpoints_and_cbuffers <br>
 * stream_delete_iso_endpoints_and_cbuffers <br>
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_iso.h"
#include "sco_drv/sco_src_drv.h"
#include "sco_drv/sco_sink_drv.h"


/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/
/** The location of the hci handle in the iso_get_endpoint params */
#define HCI_HANDLE  0

#ifdef INSTALL_ISO_CHANNELS
/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static bool iso_set_up_endpoint(ENDPOINT * ep, ENDPOINT_DIRECTION dir, unsigned int buf_size);
static void clean_up_endpoint(ENDPOINT * ep);
static inline void iso_rx_reset_toa_packet_offset(endpoint_iso_state *iso);
static inline void clean_up_iso(ENDPOINT * source_ep, ENDPOINT * sink_ep);

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
 *
 * stream_iso_get_endpoint
 *
 */
ENDPOINT *stream_iso_get_endpoint(CONNECTION_LINK con_id,
                                  ENDPOINT_DIRECTION dir,
                                  unsigned num_params,
                                  unsigned *params)
{
    ENDPOINT *ep;
    unsigned key;

    patch_fn_shared(stream_iso_interface);

    /* Expect an hci handle and potentially some padding */
    if (num_params < 1)
    {
        L3_DBG_MSG("hydra stream_iso_get_endpoint (num_params < 1) return NULL");
        return NULL;
    }
    /* The hci handle forms the key (unique for the type and direction) */
    key = params[HCI_HANDLE];

    L3_DBG_MSG1("hydra stream_iso_get_endpoint hci handle: %d", key);

    /* Return the requested endpoint (NULL if not found) */
    ep = iso_get_endpoint(key, dir);
    if (ep)
    {
        /* The endpoint has been created, however we now need to check
           the ID */
        if (ep->con_id == INVALID_CON_ID)
        {
            ep->con_id = con_id;
        }
        /* If the client does not own the endpoint they can't access it. */
        else if (ep->con_id != con_id)
        {
            L3_DBG_MSG("hydra stream_iso_get_endpoint (ep->con_id != con_id) return NULL");
            return NULL;
        }
    }
    L3_DBG_MSG1("hydra stream_iso_get_endpoint normal execution ep: %d", ep);
    return ep;
}


/****************************************************************************
 *
 * stream_create_iso_endpoints_and_cbuffers
 *
 */
bool stream_create_iso_endpoints_and_cbuffers(unsigned int hci_handle,
                                              unsigned int source_buf_size,
                                              unsigned int sink_buf_size,
                                              tCbuffer **source_cbuffer,
                                              tCbuffer **sink_cbuffer)
{
    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;
    ENDPOINT *source_ep, *sink_ep;

    source_ep = sink_ep = NULL;

    patch_fn_shared(stream_iso_interface);

    /* Clear contents of source and sink cbuffer pointer parameters as a
     * precaution to minimise the risk of a caller treating them as valid
     * pointers in the event of the function failing and returning FALSE.
     */
    *source_cbuffer = NULL;
    *sink_cbuffer = NULL;

    L3_DBG_MSG1("hydra stream_create_iso_endpoints_and_cbuffers hci handle: %d", key);

    /* Check that we don't already have a source endpoint for the
     * specified hci handle.
     */
    if (iso_get_endpoint(key, SOURCE) != NULL
            || iso_get_endpoint(key, SINK) != NULL)
    {
        /* Caller should not have called us for a second time without
         * deleting the existing buffers first.
         */
        panic(PANIC_AUDIO_SCO_BUFFERS_ALREADY_EXIST);
    }

    /* Check if the channel is active in the source direction */
    if (source_buf_size > 0)
    {
        SCO_SRC_DRV_DATA *sco_src_drv;

        /* Create and initialise a source endpoint
         * ---------------------------------------
         */
        if ((source_ep = iso_create_endpoint(key, SOURCE)) == NULL)
        {
            return FALSE;
        }

        if (!iso_set_up_endpoint(source_ep, SOURCE, source_buf_size))
        {
            /* we are failing so free up created source endpoint*/
            clean_up_endpoint(source_ep);
            return FALSE;
        }

        /* Create the sco_src_drv as part of the endpoint. */
        sco_src_drv = sco_src_drv_data_create();
        if (sco_src_drv == NULL)
        {
            clean_up_endpoint(source_ep);
            return FALSE;
        }

       /* Connecting the sco_src_drv output buffer to the MMU buffer will happen
        * when the transform creates that output buffer.
        */

        /* Install the sco drv. */
        source_ep->state.sco.sco_drv.sco_src_drv = sco_src_drv;
    }

    /* Check if the channel is active in the sink direction */
    if (sink_buf_size > 0)
    {
        SCO_SINK_DRV_DATA *sco_sink_drv;

        /* Create and initialise a sink endpoint
         * -------------------------------------
         */
        if ((sink_ep = iso_create_endpoint(key, SINK)) == NULL)
        {
            /* we are failing so free up any resources
             */
            clean_up_iso(source_ep, sink_ep);
            return FALSE;
        }

        if (!iso_set_up_endpoint(sink_ep, SINK, sink_buf_size))
        {
            /* we are failing so free up any resources
             */
            clean_up_iso(source_ep, sink_ep);
            return FALSE;
        }
        /* Create the sco_sink_drv as part of the endpoint. */

#ifdef ISO_TO_AIR_FRAMING_ENABLED
        sco_sink_drv = sco_sink_drv_data_create(TRUE);
#else
        sco_sink_drv = sco_sink_drv_data_create(FALSE);
#endif
        if (sco_sink_drv == NULL)
        {
            clean_up_iso(source_ep, sink_ep);
            return FALSE;
        }

        /* Connecting the sco_sink_drv input buffer to the MMU buffer will happen
         * when the transform creates that input buffer.
         */

        /* Install the sco drv. */
        sink_ep->state.sco.sco_drv.sco_sink_drv = sco_sink_drv;
    }

    /* Update incoming pointer parameters to give caller access to the
     * created source and sink cbuffer structures.
     */
    if (source_ep != NULL)
    {
        *source_cbuffer = source_ep->state.iso.cbuffer;
    }
    if(sink_ep != NULL)
    {
        *sink_cbuffer = sink_ep->state.iso.cbuffer;
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * stream_delete_iso_endpoints_and_cbuffers
 *
 */
void stream_delete_iso_endpoints_and_cbuffers(unsigned int hci_handle)
{
    ENDPOINT *ep;

    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;

    patch_fn_shared(stream_iso_interface);

    /* Get and close the source endpoint associated with the hci handle */
    if ((ep = iso_get_endpoint(key, SOURCE)) != NULL)
    {
        sco_iso_clean_up_endpoint(ep);
    }

    /* Get and close the sink endpoint associated with the hci handle */
    if ((ep = iso_get_endpoint(key, SINK)) != NULL)
    {
        sco_iso_clean_up_endpoint(ep);
    }
}

/****************************************************************************
Private Function Definitions
*/

/*
 * Set up a newly created endpoint and create/wrap mmu buffers
 */
static bool iso_set_up_endpoint(ENDPOINT * ep, ENDPOINT_DIRECTION dir, unsigned int buf_size)
{
    unsigned flags_aux;

    patch_fn_shared(stream_iso_interface);

    if (dir == SOURCE)
    {
        flags_aux = BUF_DESC_MMU_BUFFER_AUX_WR;
    }
    else
    {
        flags_aux = BUF_DESC_MMU_BUFFER_AUX_RD;
    }

    ep->can_be_closed = FALSE;
    ep->can_be_destroyed = FALSE;
    /* ISO endpoints are always at the end of a chain */
    ep->is_real = TRUE;

    /* We're hosting the buffer so create the source buffer and handles.
     * This includes a third auxiliary handle which is a write handle.
     */
    ep->state.iso.cbuffer = cbuffer_create_mmu_buffer(
                                      MMU_UNPACKED_16BIT_MASK | flags_aux,
                                      buf_size);

    if (ep->state.iso.cbuffer == NULL)
    {
        return FALSE;
    }

    /* Set 16-bit shift for unpacked buffers */
    if (dir == SOURCE)
    {
        cbuffer_set_write_shift(ep->state.iso.cbuffer, 16);
    }
    else
    {
        cbuffer_set_read_shift(ep->state.iso.cbuffer, 16);
    }

    /* Defer endpoint kicks to high-priority bgint */
    ep->deferred.config_deferred_kick = TRUE;

    /* initialise measured rate */
    ep->state.iso.rate_measurement = 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;
#ifdef INSTALL_SCO_EP_CLRM
    ep->state.iso.rm_enable_clrm_measurement = TRUE;
    ep->state.iso.rm_enable_clrm_trace = FALSE;
#endif
    return TRUE;
}

/*
 * Reset the toa packet offset procedure
 */
static inline void  iso_rx_reset_toa_packet_offset(endpoint_iso_state *iso)
{
    iso->packet_offset = 0;
    iso->packet_offset_counter = 0;
    iso->packet_offset_stable = FALSE;
}

/*
 * Cleanup endpoint and cbuffer
 */
static void clean_up_endpoint(ENDPOINT * ep)
{
    patch_fn_shared(stream_iso_interface);
    if (ep != NULL)
    {
        if (ep->state.iso.cbuffer != NULL)
        {
            /* Free up the buffer and associated data space */
            cbuffer_destroy(ep->state.iso.cbuffer);
        }

        /* Destroy SCO driver instance */
        sco_clean_up_sco_drv(ep);

        ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(ep);
    }
}

/*
 * Cleanup source and sink endpoints and cbuffers if they exist
 */
static inline void clean_up_iso(ENDPOINT * source_ep, ENDPOINT * sink_ep)
{
    if (source_ep != NULL)
    {
        clean_up_endpoint(source_ep);
    }
    if (sink_ep != NULL)
    {
        clean_up_endpoint(sink_ep);
    }
}


#endif /* INSTALL_ISO_CHANNELS */
