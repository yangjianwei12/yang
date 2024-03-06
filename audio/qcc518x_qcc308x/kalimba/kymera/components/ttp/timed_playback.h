/**
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
 *
 * \file  timed_playback.h
 *
 * \ingroup ttp
 *
 * Public header file for the timed playback module.
 */

#ifndef TIMED_PLAYBACK_H
#define TIMED_PLAYBACK_H

/*****************************************************************************
Include Files
*/

#include "cbops_mgr/cbops_mgr.h"
#include "stream/stream_common.h"

/****************************************************************************
Public Constant declarations
*/

/****************************************************************************
Public Type Declarations
*/
typedef struct TIMED_PLAYBACK_STRUCT TIMED_PLAYBACK;

/*
 * Type definition for unacheivable latency callback
 */
typedef void unachievable_latency_cback(ENDPOINT *ep,
                                        unsigned error);

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Creates a timed playback module.
 *
 * \return pointer to the timed playback instance, Null if insufficient resources.
 */
extern TIMED_PLAYBACK* timed_playback_create(void);

/**
 * \brief Initialise the timed playback instance for time aligning playback
 *        of audio.
 *
 * \param timed_pb      Pointer to the timed_pb playback instance
 * \param cbops_manager Cbops graph used for discarding, silence insertion and
 *                      rate adjust.
 * \param sample_rate   The sample rate in which the timed plalyback instance
 *                      should work.
 * \param period        The period in which the timed_playback_run is called.
 * \param callback      Function called if target latency can't be met.
 * \param delay         Delay due to endpoint hardware, in microseconds.
 * \param ep            Target endpoint for rate adjustment.
 * \param rate_adjust   Function to call with adjustment value.
 * \param need_sra      TRUE if software rate adjustment is required
 *
 * \return True if the initialisation was successful, false otherwise.
 */
extern bool timed_playback_init(TIMED_PLAYBACK *timed_pb,
                                cbops_mgr *cbops_manager,
                                unsigned sample_rate,
                                TIME_INTERVAL period,
                                unachievable_latency_cback *callback,
                                unsigned delay,
                                ENDPOINT *ep, 
                                void (*rate_adjust)(ENDPOINT *ep, int32 adjust_val),
                                bool need_sra);

/**
 * \brief Configure extra output delay for this timed playback instance
 *
 * \param timed_pb - Pointer to the timed_pb playback instance
 * \param delay - Delay value (microseconds)
 */
extern void timed_playback_set_delay(TIMED_PLAYBACK *timed_pb, unsigned delay);

/**
 * \brief Return amount of data in words left after processing above
 *        the threshold to insert silence
 * \param timed_pb - Pointer to the timed_pb playback instance
 * \return Amount in words
 */
extern unsigned timed_playback_get_latency_amount(TIMED_PLAYBACK *timed_pb);

/**
 * \brief Plays the next block of audio from in_buff to out_buff aiming for the
 *    target time specified in the metadata.
 *
 * \param timed_pb - Pointer to the timed_pb playback instance
 */
extern void timed_playback_run(TIMED_PLAYBACK *timed_pb);

/**
 * \brief Destroys the timed playback instance.
 *
 * \param timed_playback Pointer to the playback instance.
 */
extern void timed_playback_destroy(TIMED_PLAYBACK* timed_playback);

/**
 * \brief Return the time needed in us to use the samples with the given sample rate.
 *
 *      time [us] = (samples * 1 000 000)/ sample rate [samples/sec]
 *
 * \param samples Samples available to consume.
 * \param sample_rate The rate in which the samples are consumed in samples/sec.
 *
 * \return Time needed to use the samples
 */
extern TIME_INTERVAL convert_samples_to_time(unsigned samples, unsigned sample_rate);

/**
 * \brief Calculates the samples consumed within a given time.
 *
 *      samples = (time * sample_rate)/ 1 000 000
 *
 * \param time Time interval in which samples are consumed.
 * \param sample_rate - The data consumption sample rate at the buffer.
 *
 * \return samples
 */
extern unsigned convert_time_to_samples(TIME_INTERVAL time, unsigned sample_rate);

#endif /* TIMED_PLAYBACK_H */

