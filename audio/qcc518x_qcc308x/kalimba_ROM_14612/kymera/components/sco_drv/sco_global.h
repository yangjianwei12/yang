/****************************************************************************
 * Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup sco_drv ScoDrv
 * \file  sco_global.h
 *
 * Sco Global header
 *
 */

#ifndef SCO_GLOBAL_H
#define SCO_GLOBAL_H

/****************************************************************************
Include Files
*/
#include "buffer/cbuffer_c.h"

/****************************************************************************
Public Constant Declarations
*/

/** Default buffer size TODO in words that of the sco side buffer */
#define SCO_DEFAULT_SCO_BUFFER_SIZE                       256

/* A BT record with this timestamp in the header will be accepted as
 * in-sequence. */
#define SCO_DRV_EXPECTED_TS_UNKNOWN                 (-1)

/****************************************************************************
Public Type Declarations
*/

/** Generic storage for the input and output buffers. */
typedef struct SCO_TERMINAL_BUFFERS
{
    /** The buffer at the main input terminal */
    tCbuffer *ip_buffer;

    /** The buffer at the main output terminal */
    tCbuffer *op_buffer;

    /** Additional terminals for ISO operators. */
    struct ISO_ADDITIONAL_TERMINALS *others;
} SCO_TERMINAL_BUFFERS;

typedef enum {
    OK = 0,
    CRC_ERROR = 1,
    NOTHING_RECEIVED = 2,
    NEVER_SCHEDULED = 3,     /* this corresponds to the scos_time_missed debug variable
                             * in the BlueCore firmware. */
    OK_WBM = 4,
    ZERO_PACKET = 0xFE, /* Although this is an "internal" status value 
                         * (it doesn't come from the BTSS) it's passed to
                         * the decoder in case it does anything special with 
                         * zero-length packets */
    NO_PACKET = 0xFF
}metadata_status;

typedef struct SCO_PRIVATE_METADATA
{
    metadata_status status;   /* just the status information for now */
    unsigned discarded_data;   /* Field indicates the total amount of discarded data */
}SCO_PRIVATE_METADATA;

/****************************************************************************
Public Macro Declarations
*/

/**
 * Convert a quantity in octets (as seen by the BTSS) to SCO words
 * (as seen by the audio subsystem), assuming the buffer mode is 16-bit
 * unpacked. If the length in octets is uneven the SCO words number will
 * be rounded up.
 */
#define CONVERT_OCTETS_TO_SCO_WORDS(x) (((x) + 1) / 2)

/**
 * Convert a quantity in SCO words back to octets.
 */
#define CONVERT_SCO_WORDS_TO_OCTETS(x) ((x) * 2)

#define ODD_VALUE(x) (((x) & 0x1) == 1)

/* SCO packet length in octets plus the extra byte of padding that BTSS will
 * add in case of odd length packets. This happens because BTSS needs to write
 * the packets word aligned.
 */
#define SCO_PACKET_OCTETS_ADD_PADDING(x) ((x) + ((x) & 0x1))

/**
 * Sco debug messages.
 */
#if defined(SCO_DEBUG) && defined(SCO_DEBUG_PRINTF)
#include <stdio.h>
#define SCO_DBG_MSG(x)                 printf(x);printf("\n")
#define SCO_DBG_MSG1(x, a)             printf(x, a);printf("\n")
#define SCO_DBG_MSG2(x, a, b)          printf(x, a, b);printf("\n")
#define SCO_DBG_MSG3(x, a, b, c)       printf(x, a, b, c);printf("\n")
#define SCO_DBG_MSG4(x, a, b, c, d)    printf(x, a, b, c, d);printf("\n")
#define SCO_DBG_MSG5(x, a, b, c, d, e) printf(x, a, b, c, d, e);printf("\n")
#elif defined(SCO_DEBUG) && defined(SCO_DEBUG_LOG)
#define SCO_DBG_MSG(x)                 L3_DBG_MSG(x)
#define SCO_DBG_MSG1(x, a)             L3_DBG_MSG1(x, a)
#define SCO_DBG_MSG2(x, a, b)          L3_DBG_MSG2(x, a, b)
#define SCO_DBG_MSG3(x, a, b, c)       L3_DBG_MSG3(x, a, b, c)
#define SCO_DBG_MSG4(x, a, b, c, d)    L3_DBG_MSG4(x, a, b, c, d)
#define SCO_DBG_MSG5(x, a, b, c, d, e) L3_DBG_MSG5(x, a, b, c, d, e)
#else  /* SCO_DEBUG && SCO_DEBUG_LOG */
#define SCO_DBG_MSG(x)                 ((void)0)
#define SCO_DBG_MSG1(x, a)             ((void)0)
#define SCO_DBG_MSG2(x, a, b)          ((void)0)
#define SCO_DBG_MSG3(x, a, b, c)       ((void)0)
#define SCO_DBG_MSG4(x, a, b, c, d)    ((void)0)
#define SCO_DBG_MSG5(x, a, b, c, d, e) ((void)0)
#endif /* SCO_DEBUG */

#if defined(SCO_DEBUG) && defined(SCO_DEBUG_ERROR_LOG)
#define SCO_DBG_ERRORMSG(x)                 L1_DBG_MSG(x)
#define SCO_DBG_ERRORMSG1(x, a)             L1_DBG_MSG1(x, a)
#define SCO_DBG_ERRORMSG2(x, a, b)          L1_DBG_MSG2(x, a, b)
#define SCO_DBG_ERRORMSG3(x, a, b, c)       L1_DBG_MSG3(x, a, b, c)
#define SCO_DBG_ERRORMSG4(x, a, b, c, d)    L1_DBG_MSG4(x, a, b, c, d)
#define SCO_DBG_ERRORMSG5(x, a, b, c, d, e) L1_DBG_MSG5(x, a, b, c, d, e)
#else  /* SCO_DEBUG && SCO_DEBUG_LOG */
#define SCO_DBG_ERRORMSG(x)                 ((void)0)
#define SCO_DBG_ERRORMSG1(x, a)             ((void)0)
#define SCO_DBG_ERRORMSG2(x, a, b)          ((void)0)
#define SCO_DBG_ERRORMSG3(x, a, b, c)       ((void)0)
#define SCO_DBG_ERRORMSG4(x, a, b, c, d)    ((void)0)
#define SCO_DBG_ERRORMSG5(x, a, b, c, d, e) ((void)0)
#endif /* SCO_DEBUG */


/* Common PIO APIs used for the encoders, decoders and endpoints. */

#if defined(SCO_DEBUG_PIO_OP) || defined(SCO_DEBUG_PIO_EP)

#include "audio_pios/audio_pios.h"


extern void sco_pio_set_pio_in_mask(audio_pios *pio_mask, unsigned pio);
extern void sco_pio_clr_pio_mask(audio_pios *pio_mask);

extern void sco_pio_init_pio(unsigned pio);
extern void sco_pio_set_pio(unsigned pio);
extern void sco_pio_clr_pio(unsigned pio);

#endif /* defined(SCO_DEBUG_PIO_OP) || defined(SCO_DEBUG_PIO_EP) */


/* PIO APIs used in the encoders and decoders */

#ifdef SCO_DEBUG_PIO_OP
/*
 * To reserve PIOs 32 and 33 for the audio SS (3)
 * first set the Curator mib key PioSubsystemAllocationElements:
 * PioSubsystemAllocationElements = [20 03 21 03]
 *
 * PIO32 is used for the decoder and PIO33 for the encoder.
 */

#define DEBUG_PIO_DEC 32
#define DEBUG_PIO_ENC 33

extern void sco_pio_init_pio_enc(void);
extern void sco_pio_init_pio_dec(void);
extern void sco_pio_set_pio_enc(void);
extern void sco_pio_set_pio_dec(void);
extern void sco_pio_clr_pio_enc(void);
extern void sco_pio_clr_pio_dec(void);

#else

#define sco_pio_init_pio_dec()  ((void)0)
#define sco_pio_init_pio_enc()  ((void)0)
#define sco_pio_set_pio_enc()   ((void)0)
#define sco_pio_set_pio_dec()   ((void)0)
#define sco_pio_clr_pio_enc()   ((void)0)
#define sco_pio_clr_pio_dec()   ((void)0)

#endif /* SCO_DEBUG_PIO_OP */


/* PIO APIs used in the from-air and to-air endpoints. */

#ifdef SCO_DEBUG_PIO_EP
/*
 * To reserve PIOs 3 and 3 for the audio SS (3)
 * first set the Curator mib key PioSubsystemAllocationElements:
 * PioSubsystemAllocationElements = [02 03 03 03]
 *
 * PIO2 is used for the from-air endpoint and PIO3 for the to-air endpoint.
 */

#define DEBUG_PIO_FROM_AIR_EP 2
#define DEBUG_PIO_TO_AIR_EP 3

extern void sco_pio_init_pio_to_air_ep(void);
extern void sco_pio_init_pio_from_air_ep(void);
extern void sco_pio_set_pio_to_air_ep(void);
extern void sco_pio_set_pio_from_air_ep(void);
extern void sco_pio_clr_pio_to_air_ep(void);
extern void sco_pio_clr_pio_from_air_ep(void);

#else

#define sco_pio_init_pio_to_air_ep()    ((void)0)
#define sco_pio_init_pio_from_air_ep()  ((void)0)
#define sco_pio_set_pio_to_air_ep()     ((void)0)
#define sco_pio_set_pio_from_air_ep()   ((void)0)
#define sco_pio_clr_pio_to_air_ep()     ((void)0)
#define sco_pio_clr_pio_from_air_ep()   ((void)0)

#endif /* SCO_DEBUG_PIO_EP */

/****************************************************************************
Public Function Declarations
*/

#endif /* SCO_GLOBAL_H */
