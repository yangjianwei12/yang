/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
#ifndef CONV_TO_AUDIO_EMBED_H
#define CONV_TO_AUDIO_EMBED_H

/* TODO as EmbeddedC features are used more throughout the firmware
        is expected that this file will change scope and move
        to a more common place */

#ifdef __KCC__

/* #pragma introduced with EmbeddedC kcc are not recognised by gcc
    so redefine it to use _Pragma */
#define EMBEDDEDC_PRAGMA(x) _Pragma(x)

#else

/* assumed __GNUC__ */

/* #pragma introduced with EmbeddedC kcc are not recognised by gcc */
#define EMBEDDEDC_PRAGMA(x)

/* EmbeddedC symbols unknown to gcc */
#define __DM1
#define __DM2
#define __setup_circ_buf_DM1(a,b,c) (a) 
#define __setup_circ_buf_DM2(a,b,c) (a) 

#endif /* __KCC__ */

#endif /* CONV_TO_AUDIO_EMBED_H */
