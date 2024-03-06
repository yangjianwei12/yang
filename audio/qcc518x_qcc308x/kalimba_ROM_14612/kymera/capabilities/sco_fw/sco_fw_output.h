/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_fw Sco
 * \file  sco_fw_output.h
 *
 * SCO framework header for decoded audio output
 *
 */

#ifndef SCO_FW_OUTPUT_H
#define SCO_FW_OUTPUT_H

/****************************************************************************
Include Files
*/
#include "sco_common_struct.h"
/****************************************************************************
Public Function Declarations
*/

/* Metadata related functions. */
extern bool enough_space_to_run(SCO_COMMON_RCV_OP_DATA* sco_data,
                                unsigned output_words);

/**
 * \brief Add kymera metadata tags associated
 *        to the generated output buffer data.
 *
 * \param sco_data         Pointer to the common SCO rcv operator data.
 * \param output_samples   Amount of samples produced.
 * \param use_input_toa    Tell the decoder whether to use the ToA present
 *                         in the metadata input tag or not.
 */
extern void sco_fw_generate_output_metadata(SCO_COMMON_RCV_OP_DATA *sco_data,
                                            unsigned output_samples);

/**
 * \brief Reads information from the Kymera metadata associated to a frame in
 *        the decoder input buffer. This function will analyse the private data
 *        associated to the frame, which contains:
 *        - Indication on the status of the data receieved. This function will
 *          provide an overall status of the frame data.
 *        - Indication that the data to read was not contiguous, as some data
 *          have been discarded by the sco_drv. This function will align the
 *          buffer to the frame boundaries and return a status accordingly.
 *
 * \param sco_data      Pointer to the common SCO rcv operator data.
 * \param input_octets  Amount of octets consumed from the input.
 * \param status        Kymera metadata status. This should be an indication
 *                      whether to run PLC or not.
 * \param frame_info    Information about the frame format. 
 *                      If frame_info->whole_frame is TRUE, the function  
 *                      will look for and process only a frame worth of data, 
 *                      checking for frame boundary tags, PACKET_START and 
 *                      PACKET_END, and discarding any octets before the
 *                      beginning of the frame (PACKET_START). Similarly, 
 *                      there shouldn't be any tags found after PACKET_END.
 *
 * \return TRUE if the private metadata was read, FALSE otherwise.
 */
extern bool sco_dec_read_metadata(SCO_COMMON_RCV_OP_DATA *sco_data,
                                  unsigned input_octets,
                                  metadata_status *ret_status, 
                                  const SCO_ISO_FRAME_INFO *frame_info);

/**
 * \brief  Checks if there is enough data in the input buffer. Number of
 *         required data bytes is taken from the metadata tag(s) length.
 *         Discard the octets before the start tag, to be aligned with the
 *         beginning of the frame.
 *
 * \param  sco_data    Pointer to the common SCO rcv operator data
 * \param  data_bytes  Returned metadata length.
 *
 * \return  TRUE if there is enough data in the input buffer, FALSE otherwise.
 */
extern bool enough_data_full_tag(SCO_COMMON_RCV_OP_DATA *sco_data, 
                                 unsigned *data_bytes);

/**
 * \brief  One encoded frame can stretch in multiple from air records.
 *         Each record has a status (for example OK, CRC error, Nothing received)
 *         This function calculates the cumulative status of such an encoded frame.
 *
 * \param  old_status  Previous/old cumulative encoded frame status.
 * \param  cur_status  Current record's metadata status.
 *
 * \return  New cumulative encoded frame status.
 */
extern metadata_status get_cumulative_status(metadata_status old_status,
                                             metadata_status cur_status);
#endif /* SCO_FW_OUTPUT_H */
