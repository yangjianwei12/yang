#include "mips_monitor_struct_asm_defs.h"

// *****************************************************************************
// MODULE:
//    $M.mips_monitor_proc_func
//
// DESCRIPTION:
//    Define here your processing function
//
// INPUTS:
//   - Your input registers
//
// OUTPUTS:
//   - Your output registers
//
// TRASHED REGISTERS:
//    C calling convention respected.
//
// NOTES:
//
// *****************************************************************************
.MODULE $M.mips_monitor_proc_func;
   .CODESEGMENT PM;
   .MAXIM;

$_mips_monitor_proc_func:

    /*
     * TODO Assembly processing code goes here ...
     */

    r0 = M[r0 + $mips_monitor_struct.MIPS_MONITOR_OP_DATA_struct.MY_DATA_FIELD];
    rts;

.ENDMODULE;
