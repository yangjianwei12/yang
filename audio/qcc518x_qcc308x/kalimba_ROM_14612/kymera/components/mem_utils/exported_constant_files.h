/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file
 * \ingroup exp_const
 * File identifiers mapping .dyn files to a file number
 */
#ifndef EXPORTED_CONSTANT_FILES_H
#define EXPORTED_CONSTANT_FILES_H

#include "types.h"

#ifdef INSTALL_CAPABILITY_CONSTANT_EXPORT

/**
 * Exported constant file numbers
 *
 * This enumeration provides unique IDs for constants that (may be) stored
 * in an external file system. To create a new id add a new entry to the enumeration,
 * ideally at the end of a range - to avoid existing constants being renumbered.
 */
typedef enum
{
    /* Probably not to be exported */
    IIR_RESAMPLEV2_DYNAMICMEM = 0x100,
    IIR_RESAMPLEV2_DYNAMICMEMLOWMIPS = 0x101,

    /* Shared CVC files */
    CVCLIB_DATA = 0x200,
    OMS280_DYNAMICMEM,
    AEC530_DYNAMICMEM,
    ASF100_DYNAMICMEM,
    FILTER_BANK_DYNAMICMEM,
    VAD410_DYNAMICMEM,

    /* CVC Receive */
    CVC_RCV_CAP_DATA = 0x300,

    /* CVC Receive downloadable */
    CVC_RCV_CAP_DATA_DOWNLOADABLE,

    /* CVC Send */
    CVC_SEND_CAP_HEADSET_DATA_1MIC = 0x400,
    CVC_SEND_CAP_HEADSET_DATA_2MIC,
    CVC_SEND_CAP_SPEAKER_DATA_1MIC,
    CVC_SEND_CAP_SPEAKER_DATA_2MIC,
    CVC_SEND_CAP_AUTO_DATA_1MIC,
    CVC_SEND_CAP_AUTO_DATA_2MIC,
    CVC_SEND_CAP_SPEAKER_DATA_3MIC,
    CVC_SEND_CAP_SPEAKER_DATA_4MIC,

    /* CVC Send downloadable */
    CVC_SEND_CAP_HEADSET_DATA_1MIC_DOWNLOADABLE,
    CVC_SEND_CAP_HEADSET_DATA_2MIC_DOWNLOADABLE,
    CVC_SEND_CAP_SPEAKER_DATA_1MIC_DOWNLOADABLE,
    CVC_SEND_CAP_SPEAKER_DATA_2MIC_DOWNLOADABLE,
    CVC_SEND_CAP_AUTO_DATA_1MIC_DOWNLOADABLE,
    CVC_SEND_CAP_AUTO_DATA_2MIC_DOWNLOADABLE,
    CVC_SEND_CAP_SPEAKER_DATA_3MIC_DOWNLOADABLE,
    CVC_SEND_CAP_SPEAKER_DATA_4MIC_DOWNLOADABLE,

    CVC_SEND_CAP_HEADSET_2MIC_DATA_VA_WAKEON,
    CVC_SEND_CAP_HEADSET_2MIC_DATA_VA_BARGEIN,
    CVC_SEND_CAP_HEADSET_2MIC_DATA_VA_WAKEON_DOWNLOADABLE,
    CVC_SEND_CAP_HEADSET_2MIC_DATA_VA_BARGEIN_DOWNLOADABLE,

} exported_constants;

extern uint16 cvclib_dataDynTable_Main[];
extern uint16 CVC_RCV_CAP_dataDynTable_Main[];
extern uint16 oms280_DynamicMemDynTable_Main[];
extern uint16 filter_bank_DynamicMemDynTable_Main[];
extern uint16 vad410_DynamicMemDynTable_Main[];
extern uint16 aec530_DynamicMemDynTable_Main[];
extern uint16 ASF100_DynamicMemDynTable_Main[];

#endif /* INSTALL_CAPABILITY_CONSTANT_EXPORT */

extern uint16 iir_resamplev2_DynamicMemDynTable_Main[];
extern uint16 iir_resamplev2_DynamicMemLowMipsDynTable_Main[];

#endif /* EXPORTED_CONSTANTS_H */
