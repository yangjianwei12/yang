/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#ifndef STREAM_FOR_USB_AUDIO_ENDPOINT_H
#define STREAM_FOR_USB_AUDIO_ENDPOINT_H

/**
 * \brief configures the usb audio endpoint
 *
 * \param ep            Pointer to the endpoint that we want to configure
 * \param sample_rate   Sample rate of the endpoint
 * \param nrof_channels Number of channels in the usb audio connection
 * \param subframe_size Size of subframe in number of bits
 *
 * \return TRUE if configured successfully else FALSE
 */
extern bool stream_usb_audio_configure_ep(ENDPOINT *ep,
                                          unsigned sample_rate,
										  unsigned nrof_channels,
										  unsigned subframe_size);

/**
 * \brief querying the endpoint whether it can enact rate adjustment.
 *
 * \param ep              Pointer to the endpoint that we want to query.
 * \param rate_adjust_val If endpoint enacting the address of rate
 *                        adjust value will be returned on this parameter,
 *                        else NULL will be returned.
 *
 * \return TRUE if query was successful.
 */
extern bool stream_usb_audio_can_enact_rate_adjust(ENDPOINT *ep,
                                                   unsigned **rate_adjust_val);

/**
 * \brief sets the cbops op that is used for rate adjustment
 *
 * \param ep             Pointer to the usb audio endpoint
 * \param rate_adjust_op Pointer to the rate adjustment op
 */
extern void stream_usb_audio_set_cbops_sra_op(ENDPOINT *ep,
                                              cbops_op *rate_adjust_op);

#endif /* STREAM_FOR_USB_AUDIO_ENDPOINT_H */
