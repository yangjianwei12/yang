/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \file  conv_to_audio_struct.h
 * \ingroup capabilities
 *
 * convert to audio capability C header file to be translated to asm header. <br>
 *
 */

#ifndef AUDIO_CONV_TO_STRUCT_H
#define AUDIO_CONV_TO_STRUCT_H

/****************************************************************************
Private Type Declarations
*/

/* convert_to_audio based on DECODER infrastructure,
    here is "codec" specific data */
typedef struct conv_to_audio_codec
{
    /* used for assessing whether the frame_process produced something */
    unsigned samples_converted;
} conv_to_audio_codec;

#endif /* AUDIO_CONV_TO_STRUCT_H */
