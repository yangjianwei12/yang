// *****************************************************************************
//  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
//  All Rights Reserved.
//  Qualcomm Technologies International, Ltd. Confidential and Proprietary.
//  Notifications and licenses are retained for attribution purposes only
// %%version
//
// $Change$  $DateTime$
// *****************************************************************************

// ASM function for ANC compander operator data processing
// The function(s) obey the C compiler calling convention (see documentation,
// CS-124812-UG)

#include "core_library.h"
#include "cbuffer_asm.h"
#include "portability_macros.h"
#include "stack.h"

// *****************************************************************************
// MODULE:
//    $_anc_compander_initialize
//  extern void anc_compander_initialize(t_compander_object *compander_obj);
//
// DESCRIPTION:
//    Initialize function
//
// INPUTS:
//    - r0 = compander object
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
.MODULE $M.anc_compander_initialize;
    .CODESEGMENT PM;
$_anc_compander_initialize:

   PUSH_ALL_C

   r8 = r0;
   call $audio_proc.cmpd.initialize;

   POP_ALL_C
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $_compander_processing
// extern unsigned anc_compander_processing(t_compander_object *compander_obj);
//
// DESCRIPTION:
//    Data processing function.
//
// INPUTS:
//    - r0 = compander object
//
// OUTPUTS:
//    - None
//
// TRASHED REGISTERS:
//    C compliant
//
// *****************************************************************************
.MODULE $M.anc_compander_proc;
    .CODESEGMENT PM;
$_anc_compander_processing:
   
   PUSH_ALL_C
   
   r8 = r0;
   call $audio_proc.compander.stream_process;
   // return amount processed
   r0 = M[r8 + $audio_proc.compander.SAMPLES_TO_PROCESS];

   POP_ALL_C
   rts;

.ENDMODULE;