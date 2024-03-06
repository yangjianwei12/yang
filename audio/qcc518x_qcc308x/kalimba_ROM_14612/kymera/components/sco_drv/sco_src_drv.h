/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_src_drv Sco
 * \file  sco_src_drv.h
 *
 * ScoDrv header
 *
 */

#ifndef SCO_SRC_DRV_H
#define SCO_SRC_DRV_H

/****************************************************************************
Include Files
*/
#include "sco_drv/sco_global.h"
#include "stream/stream_audio_data_format.h"

/****************************************************************************
Public Type Declarations
*/

/** Data structure used by the Sco Drv module.
 */
typedef struct SCO_SRC_DRV_DATA SCO_SRC_DRV_DATA;


/** Data structure used to pass timing information 
 *  between the endpoint and SCO driver
 */
typedef struct sco_src_timing_info
{
    TIME toa_ep;    /* ToA assigned by the endpoint */
    int sp_adjust;  /* TTP Sample period adjustment */
} sco_src_timing_info;

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Processes packets from the SCO driver input buffer.
 *        SCO/unframed ISO case
 *
 * \param sco_data  Pointer to the sco src drv data
 * \param timing_info  Pointer to timing information, NULL if not valid
 * 
 */
extern void sco_src_drv_processing(SCO_SRC_DRV_DATA *sco_data,
                                   sco_src_timing_info *timing_info);

#ifdef INSTALL_ISO_CHANNELS
/**
 * \brief Processes packets from the SCO driver input buffer.
 *        Framed ISO case
 *
 * \param sco_data  Pointer to the sco src drv data
 * \param sdu_interval  Nominal SDU interval in microseconds
 * \param iso_interval  ISO interval in microseconds
 * \param kick_time  Reference time for the current kick
 */
extern void sco_src_drv_processing_iso_framed(SCO_SRC_DRV_DATA *sco_data,
                                              unsigned sdu_interval,
                                              unsigned iso_interval,
                                              TIME kick_time);
#endif /* INSTALL_ISO_CHANNELS */

/**
 * \brief Create and initialise the SCO_SRC_DRV_DATA data structure.
 *
 * \returns Pointer to the ScoDrv data structure
 */
extern SCO_SRC_DRV_DATA* sco_src_drv_data_create(void);

/**
 * \brief Sets the data format. This is needed for SCO src drv when reading
 *        data from the MMU buffer.
 *
 * \param  sco_drv_data  Pointer to the SCO drv data structure.
 * \param  input_format  The data format of the capability input terminal.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_src_drv_set_sco_data_format(SCO_SRC_DRV_DATA *sco_drv_data,
                                            AUDIO_DATA_FORMAT input_format);

/**
 * \brief Set some information about what happens over the air.
 *        Specifically, the sco_drv needs to know the size of the packets,
 *        the first value to expect in the timestamp field of the record header
 *        and how will that value increment with each packet.
 *
 * \param  sco_drv_data     Pointer to the SCO src drv data structure.
 * \param  from_air_length  Length of the packets in input.
 * \param  expected_ts      Sco metadata timestamp to expect in the first
 *                          packet.
 * \param  ts_step          Step between the sco metadata timstamps of each
 *                          packet.
 * \param  framed_data      TRUE if receiving framed ISO SDUs, FALSE otherwise.
 * \param  framed_data      TRUE if variables length SDUs are allowed, FALSE otherwise.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_src_drv_set_from_air_info(SCO_SRC_DRV_DATA *sco_drv_data,
                                        unsigned from_air_length,
                                        unsigned expected_ts,
                                        unsigned ts_step,
                                        bool framed_data,
                                        bool variable_length_sdu);

/**
 * \brief Unlinke sco_src_drv_set_from_air_info, this takes care of the
 *        expected timestamp. The rest of the from-air info has been passed to
 *        the SCO src driver. For SDU numbering, the first expected exp_ts is
 *        obtained right after we schedule the first kick in sco_start_timers,
 *        so it can only be set after that point.
 *
 * \param  sco_drv_data  Pointer to the SCO src drv data structure.
 * \param  expected_ts   Sco metadata timestamp to expect in the first
 *                       packet. For SDU numbering this is calculated when
 *                       scheduling the first kick in sco_start_timers.
 */
extern void sco_src_drv_update_expected_ts(SCO_SRC_DRV_DATA *sco_drv_data,
                                           unsigned expected_ts);

/**
 * \brief Connect the ScoDrv to an input and an output buffer.
 *
 * \param sco_data Pointer to the SCO drv data structure.
 * \param in_buff  Pointer to input buffer.
 * \param out_buff Pointer to output buffer.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_src_drv_data_connect(SCO_SRC_DRV_DATA *sco_data,
                                     tCbuffer *in_buff,
                                     tCbuffer *out_buff);

/**
 * \brief Disconnect the ScoDrv from the input and output buffers.
 *
 * \param sco_data Pointer to the SCO src drv data structure.
 */
extern void sco_src_drv_data_disconnect(SCO_SRC_DRV_DATA *sco_data);

#endif /* SCO_SRC_DRV_H */
