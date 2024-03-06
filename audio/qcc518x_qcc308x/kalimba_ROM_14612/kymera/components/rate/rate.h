/****************************************************************************
 * Copyright 2017 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup rate_lib Rate Matching Library
 */

/**
 * \file rate.h
 * \ingroup rate_lib
 *
 */

#ifndef RATE_RATE_H
#define RATE_RATE_H

#include "rate_types.h"

/* Public sub-module declarations */
#include "rate_measure.h"
#include "rate_measure_metadata.h"
#include "rate_compare.h"
#include "rate_ts_filter.h"
#include "rate_pid.h"
#include "rate_match.h"

/* Types referenced in the declarations in this file */
#include "buffer/cbuffer_c.h"

/* Definitions used in inline functions defined here */
#include "platform/pl_fractional.h"

/****************************************************************************
 * Public Type Definitions
 */

/****************************************************************************
 * Public Function Declarations And Inline Definitions
 */

/**********************************************
 * Conversion functions
 */

/**
 * \param t Non-negative time in seconds represented as Q1.N
 * \param sr Sample rate in Hz
 * \return Number of samples in t at sr
 */
static inline unsigned rate_time_to_samples(unsigned t, unsigned sr)
{
    return (unsigned)frac_mult(t, sr);
}

/**
 * \brief Calculate whole sample periods in time t, i.e.
 *        version of rate_time_to_samples which rounds towards zero
 * \param t Non-negative time in seconds represented as Q1.N
 * \param sr Sample rate in Hz
 * \return Number of samples in t at sr
 */
static inline unsigned rate_time_to_samples_trunc(unsigned t, unsigned sr)
{
    return (unsigned)(((uint48)t * sr) >> (DAWTH-1));
}

/**
 * \brief Convert a Q1.N fractional seconds value to a RATE_SECOND_INTERVAL
 * \note TODO unit test
 */
static inline RATE_SECOND_INTERVAL rate_sec_frac_to_second_interval(int sec_frac)
{
    return frac_mult(sec_frac, RATE_SECOND_INTERVAL_SECOND);
}

/**
 * \brief Convert a RATE_SECOND_INTERVAL to Q1.N fractional seconds
 * \note TODO unit test
 */
extern int rate_second_interval_to_sec_frac(RATE_SECOND_INTERVAL interval);

/**
 * \brief Convert signed microseconds to RATE_SECOND_INTERVAL.
 * \param usec The argument should be <= 1000000, this is not checked.
 */
static inline RATE_SECOND_INTERVAL rate_signed_usec_to_second_interval(int usec)
{
    return usec << RATE_SECOND_INTERVAL_EXTRA_RESOLUTION;
}

/**
 * \brief Convert RATE_SECOND_INTERVAL to signed seconds with rounding
 * \note TODO unit test
 */
static inline int rate_second_interval_to_signed_usec_round(RATE_SECOND_INTERVAL rsi)
{
#ifdef __KCC__
    return frac_mult(rsi, (1 << (DAWTH-1-RATE_SECOND_INTERVAL_EXTRA_RESOLUTION)));
#else
    return (rsi + (1 << (RATE_SECOND_INTERVAL_EXTRA_RESOLUTION-1))) >> RATE_SECOND_INTERVAL_EXTRA_RESOLUTION;
#endif
}

/* rate_conv.c */

/** Convert from a sample rate in Hz to sample period in the RATE_SAMPLE_PERIOD
 * format.
 * \param sample_rate Sample rate in Hz
 * \return Sample period in microseconds, Qu16.m
 */
extern RATE_SAMPLE_PERIOD rate_sample_rate_to_sample_period(unsigned sample_rate);

/** Calculate length of a number of sample periods in microseconds.
 * \param num_samples Number of samples
 * \param sample_period in microseconds, Qu16.m
 * \return Rounded duration of num_samples samples, in microseconds
 */
extern TIME rate_samples_to_usec(unsigned num_samples, RATE_SAMPLE_PERIOD sample_period);

/** Calculate length of a number of sample periods in microseconds, when the
 * sample count is specified with some fractional bits.
 * \param num_samples_frac Number of samples, Qn.x where x is given by the
 *                         sample_shift argument
 * \param sample_shift Number of fractional bits in num_samples_frac.
 *                     The useful and tested range is 0..8.
 * \param sample_period in microseconds, Qu16.m
 * \return Rounded duration of num_samples samples, in microseconds
 */
extern TIME rate_samples_frac_to_usec(unsigned num_samples_frac,
                                      unsigned sample_shift,
                                      RATE_SAMPLE_PERIOD sample_period);

/** Calculate length of a number of sample periods as a RATE_SECOND_INTERVAL.
 * \param num_samples Number of samples
 * \param sample_period in RATE_SAMPLE_PERIOD representation
 * \return Rounded duration of num_samples samples as RATE_SECOND_INTERVAL.
 */
extern RATE_SECOND_INTERVAL rate_samples_to_second_interval(
        unsigned num_samples,
        RATE_SAMPLE_PERIOD sample_period);

/** Calculate rounded number of samples from duration in RATE_SECOND_INTERVAL
 * and sample rate.
 * \param interval non-negative interval
 * \param sample_rate
 * \return Number of samples
 * \note TODO unit test
 */
extern unsigned rate_second_interval_to_samples(
        RATE_SECOND_INTERVAL interval,
        unsigned sample_rate);

/** Calculate number of samples from duration in RATE_SECOND_INTERVAL
 * and sample rate, rounded down.
 * \param interval non-negative interval
 * \param sample_rate
 * \return Number of samples
 * \note TODO unit test
 */
extern unsigned rate_second_interval_to_samples_trunc(
        RATE_SECOND_INTERVAL interval,
        unsigned sample_rate);

/**********************************************
 * Utility functions
 */

/** Reduce two sample rates. The purpose is to minimize the number of bits
 * needed for calculations involving ratios of sample rates; the result
 * is not an irreducible pair. With rates up to 192000Hz, the output
 * values are <= 2560.
 * Either both fs1 and fs2 are in Hz, or both are in Hz divided by 25,
 * but not mixed.
 * \param fs1 First sample rate in Hz or in Hz divided by 25.
 * \param fs2 Second sample rate in Hz or in Hz divided by 25.
 * \param reduced1 Pointer to variable for first reduced sample rate [out]
 * \param reduced2 Pointer to variable for second reduced sample rate [out]
 * \return False if any invalid inputs were detected. True does not guarantee validity.
 */
extern bool rate_reduce_sample_rates(unsigned fs1, unsigned fs2, unsigned* reduced1, unsigned* reduced2);

/** Given a supported sample rate, produce the rate divided by 25.
 * This function is not guaranteed to be correct for other arguments.
 * \param fs Full sample rate
 * \return fs/25 when fs is a supported sample rate, undefined otherwise.
 */
static inline unsigned rate_sample_rate_div_25(unsigned fs)
{
#ifdef __KCC__
    return frac_mult(fs, FRACTIONAL(0.04));
#else /* __KCC__ */
    return fs/25;
#endif /* __KCC__ */
}

/* rate_metadata.c */

/** Determine the timestamp type, if any, contained in a metadata tag.
 * \param tag Metadata tag [in]
 * \return Type of timestamp in the tag
 */
extern RATE_TIMESTAMP_TYPE rate_metadata_get_timestamp_type(const metadata_tag* tag);

#endif /* RATE_RATE_H */
