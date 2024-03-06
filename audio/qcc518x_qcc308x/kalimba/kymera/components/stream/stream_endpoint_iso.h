/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stream_endpoint_iso.h
 * \ingroup endpoints 
 */

#ifndef _STREAM_ENDPOINT_ISO_H_
#define _STREAM_ENDPOINT_ISO_H_

/****************************************************************************
Include Files
*/
#include "stream/stream.h"
#include "stream_endpoint_sco.h"

/****************************************************************************
 * Private Macro Declarations
 */
#ifdef INSTALL_ISO_CHANNELS

/****************************************************************************
 * Protected function declaration: Endpoint functions.
 */

/**
 * \brief Close the endpoint.
 *
 * \param endpoint Pointer to the endpoint that is being closed.
 *
 * \return True if successful.
 */
extern bool iso_close(ENDPOINT *endpoint);

/**
 * \brief Connects the ISO source/sink endpoint.
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
extern bool iso_connect(ENDPOINT *endpoint,
                        tCbuffer *Cbuffer_ptr,
                        ENDPOINT *ep_to_kick,
                        bool *start_on_connect);

/**
 * \brief Disconnect from the endpoint.
 *
 * \param endpoint Pointer to the endpoint that is being disconnected.
 *
 * \return True if successful.
 */
extern bool iso_disconnect(ENDPOINT *endpoint);

/**
 * \brief Retrieve the buffer details from the endpoint.
 *
 * \param endpoint Pointer to the endpoint that is being queried.
 * \param details  Pointer to the buffer_detailes structure.
 *
 * \return True if successful.
 */
extern bool iso_buffer_details(ENDPOINT *endpoint, BUFFER_DETAILS *details);

/**
 * \brief Make the endpoint run any data processing and propagate kick.
 *
 * \param endpoint Pointer to the endpoint that is being kicked.
 * \param kick_dir Direction to kick.
 */
extern void iso_kick(ENDPOINT *endpoint, ENDPOINT_KICK_DIRECTION kick_dir);

/**
 * \brief The iso endpoint is responsible for scheduling chain kicks.
 *
 * This function is called to perform any real-time scheduling
 * that needs to occur per kick.
 *
 * \param endpoint Pointer to the endpoint that is being scheduled.
 * \param ko       Pointer to the kick object to be kicked.
 */
extern void iso_sched_kick(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief Initiates a kick interrupt source to start producing kicks.
 *
 * \param endpoint Pointer to the endpoint that is being started.
 * \param ko       Pointer to the kick object to be kicked first.
 *
 * \return True if successful.
 */
extern bool iso_start(ENDPOINT *endpoint, KICK_OBJECT *ko);

/**
 * \brief Cancel the associated kick interrupt source.
 *
 * \param endpoint Pointer to the endpoint that is being stopped.
 *
 * \return True if successful.
 */
extern bool iso_stop(ENDPOINT *endpoint);

/**
 * \brief Configure the endpoint.
 *
 * \param endpoint Pointer to the endpoint that is being configured.
 * \param key      Key to the setting being configured.
 * \param value    New value for the setting.
 *
 * \return True if successful.
 */
extern bool iso_configure(ENDPOINT *endpoint,
                          unsigned int key,
						  uint32 value);

/**
 * \brief Get endpoint configuration
 *
 * \param endpoint Pointer to the endpoint that is being queried.
 * \param key      Key to the setting being queried.
 * \param result   Pointer to where to store the value associated
 *                 with the key.
 *
 * \return True if successful
 */
extern bool iso_get_config(ENDPOINT *endpoint,
                           unsigned int key,
						   ENDPOINT_GET_CONFIG_RESULT *result);

/**
 * \brief Obtain the timing information from the endpoint.
 *
 * \param endpoint  Pointer to the endpoint that is being queried.
 * \param time_info Pointer to timing information structure.
 */
extern void iso_get_timing(ENDPOINT *endpoint,
                           ENDPOINT_TIMING_INFORMATION *time_info);

/**
 * \brief Report whether two endpoints have the same clock source
 *
 * \param ep1        Endpoint to compare with ep2.
 * \param ep2        Endpoint to compare with ep1.
 * \param both_local True if both endpoints are locally clocked.
 *
 * \return TRUE if ep1 and ep2 share a clock source, otherwise FALSE.
 */
extern bool iso_have_same_clock(ENDPOINT *ep1, ENDPOINT *ep2, bool both_local);

/****************************************************************************
 * Private function declaration: ISO private functions.
 */

/**
 * \brief Create a new ISO endpoint
 *
 * \param key unique key for ISO endpoints
 * \param dir endpooint direction (source/sink)
 *
 * \return pointer to created endpoint
 */
extern ENDPOINT *iso_create_endpoint(unsigned key, ENDPOINT_DIRECTION dir);

/**
 * \brief Get pointer to an existing ISO endpoint
 *
 * \param key unique key for ISO endpoints
 * \param dir endpooint direction (source/sink)
 *
 * \return pointer to endpoint
 */
extern ENDPOINT *iso_get_endpoint(unsigned int key, ENDPOINT_DIRECTION dir);


#endif /* INSTALL_ISO_CHANNELS */

#endif /* !_STREAM_ENDPOINT_SCO_H_ */
