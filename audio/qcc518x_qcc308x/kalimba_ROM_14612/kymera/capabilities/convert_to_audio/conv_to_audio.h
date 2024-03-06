/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  conv_to_audio.h
 * \ingroup capabilities
 *
 * convert_to_audio capability public header file. <br>
 *
 */

#ifndef AUDIO_CONV_TO_CAPABILITY_H
#define AUDIO_CONV_TO_CAPABILITY_H

#include "capabilities.h"
#include "conv_to_audio_embed.h"

/** The capability data structure for conv_to_audio capability */
extern const CAPABILITY_DATA convert_to_audio_cap_data;

/****************************************************************************
Private Function Definitions
*/

/**
 * \brief opmgr message handlers
 *
 * \param op_data Pointer to the operator instance data.
 * \param message_data Pointer to the create request message
 * \param response_id Location to write the response message id
 * \param response_data Location to write a pointer to the response message
 *
 * \return Whether the response_data field has been populated with a valid
 * response
 */
/** 
 * \brief Create operator.
 * Allocates specific capability memory and initialises the operator.
 */
extern bool conv_to_audio_create( OPERATOR_DATA *op_data, void *message_data,
                                unsigned *response_id, void **response_data);


/* data processing function */

/**
 * \brief Frame processing function.
 * Converts 16-bit to 32-bit words (audio samples).
 * TODO Currently only mono 16-bit implemented.
 *
 * \param op_data Pointer to the operator instance data.
 * \param touched Structure to return the terminals which this operator wants kicked
 */
extern void conv_to_audio_process_data(OPERATOR_DATA *op_data, TOUCHED_TERMINALS *touched);

#endif /* AUDIO_CONV_TO_CAPABILITY_H */
