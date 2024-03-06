// *****************************************************************************
// Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#include "aac_library.h"
#include "core_library.h"

#ifdef AACDEC_SBR_ADDITIONS

// *****************************************************************************
// MODULE:
//    $aacdec.sbr_read_one_word_from_flash
//
// DESCRIPTION:
//    read one word from an SBR table in flash
//
// INPUTS:
//    r0 = address of word in flash segment
//
// OUTPUTS:
//    r0 = the word read from flash
//
// TRASHED REGISTERS:
//    none
//  TODO his probably doesn't make sense any more, just r0 = M[r0]; in the caller
//
// *****************************************************************************
.MODULE $M.aacdec.sbr_read_one_word_from_flash;
   .CODESEGMENT AACDEC_SBR_READ_ONE_WORD_FROM_FLASH_PM;
   .DATASEGMENT DM;

   $aacdec.sbr_read_one_word_from_flash:

   // push rLink onto stack
   push rLink;

   r0 = M[r0];

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif



