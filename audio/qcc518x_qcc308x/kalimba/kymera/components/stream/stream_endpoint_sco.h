/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_endpoint_sco.h
 * \ingroup endpoints
 */

#ifndef _STREAM_ENDPOINT_SCO_H_
#define _STREAM_ENDPOINT_SCO_H_

/****************************************************************************
Include Files
*/
#include "stream/stream.h"


/****************************************************************************
 * Private Macro Declarations
 */

#ifdef INSTALL_MIB
/* Get sink endpoint processing time from MIB key, if available */
#define SCOISO_SINK_PROCESSING_TIME() mibgetrequ16(SCOISOSINKPROCESSINGTIME)
#else
/* Otherwise just use a fixed value
 * This could probably be zero as only unit tests will use it 
 */
#define SCOISO_SINK_PROCESSING_TIME() 500
#endif

/****************************************************************************
 * Private function declarations: Sco common functions.
 */

/**
 * \brief Gets the unique identifier of a sco link's clock source, it's
 * wallclock for comparing with other sco endpoints.
 *
 * \param ep The sco endpoint to get the wallclock id of
 *
 * \return A unique identifier representing the wallclock for this link
 */
unsigned stream_sco_get_wallclock_id(ENDPOINT *ep);

/**
 * \brief get the audio data format of the underlying hardware associated with
 * the endpoint
 *
 * \param endpoint pointer to the endpoint to get the data format of.
 *
 * \return the data format of the underlying hardware
 */
AUDIO_DATA_FORMAT sco_get_data_format (ENDPOINT *endpoint);

/**
 * \brief set the audio data format that the endpoint will place in/consume from
 * the buffer
 *
 * \param endpoint pointer to the endpoint to set the data format of.
 * \param format AUDIO_DATA_FORMAT requested to be produced/consumed by the endpoint
 *
 * \return whether the set operation was successful
 */
bool sco_set_data_format (ENDPOINT *endpoint, AUDIO_DATA_FORMAT format);

/**
 * \brief Get the hci_handle from a sco endpoint
 *
 * \param endpoint pointer to the sco endpoint
 *
 * \return hci_handle
 *
 * \* note This is a function in the SCO endpoint file so that
 *         if the key is ever changed then this function should
 *         be changed along side.
 */
unsigned int stream_sco_get_hci_handle(ENDPOINT *endpoint);

/**
 * \brief Common function to obtain the timing information from the endpoint.
 *
 * \param endpoint  Pointer to the sco endpoint.
 * \param time_info Pointer to timing data structure.
 */
void sco_common_get_timing(ENDPOINT *endpoint,
                           ENDPOINT_TIMING_INFORMATION *time_info);

/****************************************************************************
 * Protected function declaration: Endpoint functions.
 */

/**
 * \brief Close the endpoint
 */
bool sco_close(ENDPOINT *endpoint);

/**
 * \brief Connects the SCO source/sink endpoint.
 *
 * \param  endpoint          Pointer to the endpoint that is being connected.
 * \param  Cbuffer_ptr       Pointer to the buffer created by the transform.
 * \param  ep_to_kick        Pointer to the endpoint which will be kicked after
 *                           a successful run.
 *                           Note: this can be different from the connected to
 *                           endpoint when in-place running is enabled.
 * \param  start_on_connect  Return flag which indicates if the endpoint wants
 *                           to be started on connect.
 *                           Note: The endpoint will only be started if the
 *                           connected to endpoint wants to be started too.
 *
 * \return TRUE on success.
 */
extern bool sco_connect(ENDPOINT *endpoint,
                        tCbuffer *Cbuffer_ptr,
                        ENDPOINT *ep_to_kick,
                        bool *start_on_connect);

/**
 * \brief Disconnect from the endpoint
 */
bool sco_disconnect(ENDPOINT *endpoint);

/**
 * \brief Retrieve the buffer details from the endpoint
 */
bool sco_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details);

/**
 * \brief Make the endpoint run any data processing and propagate kick
 */
void sco_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief The sco endpoint is responsible for scheduling chain kicks:
 *        this function is called to perform any real-time scheduling
 *        that needs to occur per kick
 */
void sco_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief Perform setups for the from-air endpoint.
 */
void sco_start_from_air(ENDPOINT *endpoint);

/**
 * \brief Perform setups for the to-air endpoint.
 */
void sco_start_to_air(ENDPOINT *endpoint);

/**
 * \brief Perform timing calculations and setups, and start timers.
 *
 * \return TRUE if the timer was not active,
 *         FALSE if it was already active (and nothing needs doing)
 */
bool sco_start_timers(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function initiates a kick interrupt source to start producing kicks.
 */
bool sco_start(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief The endpoint is responsible for scheduling chain kicks:
 *        this function cancels the associated kick interrupt source.
 */
bool sco_stop(ENDPOINT *endpoint);

/**
 * \brief Configure the sco endpoint
 */
bool sco_configure(ENDPOINT *endpoint, unsigned int key, uint32 value);

/**
 * \brief Get endpoint configuration
 */
bool sco_get_config(ENDPOINT *endpoint, unsigned int key, ENDPOINT_GET_CONFIG_RESULT* result);

/**
 * \brief Obtain the timing information from the endpoint
 */
void sco_get_timing (ENDPOINT *endpoint, ENDPOINT_TIMING_INFORMATION *time_info);

/**
 * \brief Report whether two endpoints have the same clock source
 */
bool sco_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local);

/**
 * \brief Get the sco source driver instance associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 *
 * \return SCO_SRC_DRV_DATA pointer to the instance.
 */
SCO_SRC_DRV_DATA* sco_get_sco_src_drv(ENDPOINT *ep);

/**
 * \brief Get the sco sink driver instance associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 *
 * \return SCO_SINK_DRV_DATA pointer to the instance.
 */
SCO_SINK_DRV_DATA* sco_get_sco_sink_drv(ENDPOINT *ep);

/**
 * \brief Destroy the sco (source or sink) driver instance
 *        associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 */
void sco_clean_up_sco_drv(ENDPOINT *ep);

/**
 * \brief Connects the SCO/ISO source/sink endpoint.
 *        Called from both sco_connect and iso_connect.
 *
 * \param  endpoint          Pointer to the endpoint that is being connected.
 * \param  Cbuffer_ptr       Pointer to the buffer created by the transform.
 * \param  ep_to_kick        Pointer to the endpoint which will be kicked after
 *                           a successful run.
 *                           Note: this can be different from the connected to
 *                           endpoint when in-place running is enabled.
 * \param  start_on_connect  Return flag which indicates if the endpoint wants
 *                           to be started on connect.
 *                           Note: The endpoint will only be started if the
 *                           connected to endpoint wants to be started too.
 * \param  format            Endpoint data format needed to set the usable
 *                           octets for the transform buffer.
 *
 * \return TRUE on success.
 */
extern bool sco_iso_connect(ENDPOINT *endpoint,
                            tCbuffer *Cbuffer_ptr,
                            ENDPOINT *ep_to_kick,
                            bool *start_on_connect,
                            AUDIO_DATA_FORMAT format);

/**
 * \brief Obtain the usable octets to be further set for the transform buffer
 *        for SCO/ISO endpoints according to the endpoint data format. SCO
 *        encoders and decoders output data in 16-bit format, as opposed to
 *        LC3 which outputs data in 32-bit format.
 *
 * \param  format  Format of the endpoint.
 *
 * \return The number of usable octets to be set for the transform buffer.
 */
extern unsigned sco_iso_get_transf_buf_usable_octets(AUDIO_DATA_FORMAT format);

/**
 * \brief Free the memory associated to the endpoint.
 *
 * \param ep  Pointer to the ENDPOINT structure.
 */
extern void sco_iso_clean_up_endpoint(ENDPOINT *ep);

#endif /* !_STREAM_ENDPOINT_SCO_H_ */
