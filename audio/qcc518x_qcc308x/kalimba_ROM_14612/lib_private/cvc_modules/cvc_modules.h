// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CVC_MODULES_H
#define CVC_MODULES_H

.CONST $CVC_VERSION                    0x002000;

#include "portability_macros.h"
#include "filter_bank/filter_bank_library.h"
#include "harm100/harm100_library.h"
#include "dms200/dms200_library.h"
#include "ndvc200/ndvc200_library.h"
#include "agc400/agc400_library.h"
#include "vad410/vad410_library.h"
#include "AdapEq/AdapEq_library.h"
#include "mgdc100/mgdc100_library.h"
#include "asf100/asf100_library.h"
#include "dmss/dmss_library.h"
#include "aec530/aec530_library.h"
#include "aed100/aed100_library.h"
#include "asf200/asf200_library.h"
#include "fbc/fbc_library.h"
#include "blend100/blend100_library.h"
#include "hyst100/hyst100_library.h"
#include "svad100/svad100_library.h"
#include "selfclean100/selfclean100_library.h"
#include "mask100/mask100_library.h"


#include "cvc_stream_asm_defs.h"
.CONST $cvc_stream.frmbuffer_struct.STRUC_SIZE.NO_PEAK 	($cvc_stream.frmbuffer_struct.STRUC_SIZE-1);

#if 0
#include "adf200/adf200_library.h"
#include "ssr/ssr_library.h"
#include "nc100/nc100_library.h"
#endif

#endif // CVC_MODULES_H
