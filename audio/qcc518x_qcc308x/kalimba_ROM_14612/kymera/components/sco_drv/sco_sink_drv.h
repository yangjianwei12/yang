
/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_sink_drv Sco
 * \file  sco_sink_drv.h
 *
 * Sco sink driver header
 *
 */

#ifndef SCO_SINK_DRV_H
#define SCO_SINK_DRV_H

/****************************************************************************
Include Files
*/
#include "sco_drv/sco_global.h"
#include "stream/stream_audio_data_format.h"

/****************************************************************************
Public Type Declarations
*/

typedef struct SCO_SINK_DRV_DATA SCO_SINK_DRV_DATA;

/****************************************************************************
Public Function Declarations
*/
/**
 * \brief Copies a frame from the SCO input buffer (encoder output buffer) to
 *        the output buffer (MMU buffer).
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 * \param ref_time      reference time to compare incoming data packet
 *                      timestamps against
 * \param max_ts_dev    Max deviation from ref_time to detect
 *                      early/late/in-time packets.
 */
extern void sco_sink_drv_processing(SCO_SINK_DRV_DATA *sco_drv_data,
                                    TIME ref_time,
                                    unsigned max_ts_dev);

/**
 * \brief Create and initialise the sco sink driver data structure.
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 * \param framing_enabled Input parameter to enable disable to air record framing.
* 
 * \returns Pointer to the ScoDrv data structure.
 */
extern SCO_SINK_DRV_DATA* sco_sink_drv_data_create(bool framing_enabled);

/**
 * \brief Destroy the ScoDrv data structure.
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 */
void sco_sink_drv_data_destroy(SCO_SINK_DRV_DATA *sco_drv_data);

/**
 * \brief Set the data format. It should be used when dealing with the to-air
 *        metadata.
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 * \param input_format  The data format.
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_sink_drv_set_sco_data_format(SCO_SINK_DRV_DATA *sco_drv_data,
                                             AUDIO_DATA_FORMAT input_format);

/**
 * \brief Set to air information.
 *
 * \param sco_drv_data   Pointer to the SCO drv data structure.
 * \param to_air_length  Length of the packets in input (in bytes).
 * \param iso_framed     TRUE for ISO links using framed data, FALSE otherwise
 * \param ttp_enable     TRUE if ttp logic should be enabled, FALSE otherwise
 */
extern void sco_sink_drv_set_to_air_info(SCO_SINK_DRV_DATA *sco_drv_data,
                                         unsigned to_air_length,
                                         bool iso_framed,
                                         bool ttp_enable);

/**
 * \brief Unlinke sco_sink_drv_set_to_air_info which passes the rest of the
 *        to-air info to the SCO sink driver, this takes care of the initial
 *        sequence number. For SDU numbering, the initial sequence number is
 *        obtained right after we schedule the first kick in sco_start_timers,
 *        so it can only be set after that point.
 *
 * \param  sco_drv_data  Pointer to the SCO src drv data structure.
 * \param  init_seq_num  Sco metadata seq_num to send with the first packet.
 *                       For SDU numbering this is calculated when scheduling
 *                       the first kick in sco_start_timers.
 */
extern void sco_sink_drv_update_initial_seq_num(SCO_SINK_DRV_DATA *sco_drv_data,
                                           unsigned init_seq_num);
/**
 * \brief Connect the ScoDrv to an input and an output buffer.
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 * \param in_buff       Pointer to input buffer (encoder output buffer).
 * \param out_buff      Pointer to output buffer (MMU buffer).
 *
 * \return TRUE if completed successfully, FALSE on fail.
 */
extern bool sco_sink_drv_data_connect(SCO_SINK_DRV_DATA *sco_drv_data,
                                      tCbuffer *in_buff,
                                      tCbuffer *out_buff);

/**
 * \brief Disconnect the ScoDrv from the input and output buffers.
 *
 * \param sco_drv_data  Pointer to the SCO drv data structure.
 */
extern void sco_sink_drv_data_disconnect(SCO_SINK_DRV_DATA *sco_drv_data);

/**
 * \brief Calc new warp value for rate adjuster based of latest TTP/EP timings
 *
 * \param  sco_drv_data  Pointer to the SCO drv data structure.
 */
extern int sco_sink_drv_ttp_ctrl_get_warp(SCO_SINK_DRV_DATA *sco_drv_data);

/**
 * \brief Get driver's ttp logic flag
 *
 * \param  sco_drv_data  Pointer to the SCO drv data structure.
 *
 * \return TRUE=enabled, FALSE=disabled
 */
extern bool sco_sink_drv_ttp_enabled(SCO_SINK_DRV_DATA *sco_drv_data);

#endif /* SCO_SINK_DRV_H */
