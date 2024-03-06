/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/

#include "io_map.asm"
#include "io_defs.asm"

// Extra defines to work with assembly code

// Map flag bit symbols from register definitions
// to the ones used in existing asm code
   .CONST $N_FLAG                         $N_FLAG_MASK;
   .CONST $Z_FLAG                         $Z_FLAG_MASK;
   .CONST $C_FLAG                         $C_FLAG_MASK;
   .CONST $V_FLAG                         $V_FLAG_MASK;
   .CONST $UD_FLAG                        $UD_FLAG_MASK;
   .CONST $SV_FLAG                        $SV_FLAG_MASK;
   .CONST $BR_FLAG                        $BR_FLAG_MASK;
   .CONST $UM_FLAG                        $UM_FLAG_MASK;

   .CONST $NOT_N_FLAG                     (65535-$N_FLAG);
   .CONST $NOT_Z_FLAG                     (65535-$Z_FLAG);
   .CONST $NOT_C_FLAG                     (65535-$C_FLAG);
   .CONST $NOT_V_FLAG                     (65535-$V_FLAG);
   .CONST $NOT_UD_FLAG                    (65535-$UD_FLAG);
   .CONST $NOT_SV_FLAG                    (65535-$SV_FLAG);
   .CONST $NOT_BR_FLAG                    (65535-$BR_FLAG);
   .CONST $NOT_UM_FLAG                    (65535-$UM_FLAG);

