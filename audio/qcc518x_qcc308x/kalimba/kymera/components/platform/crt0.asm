// *****************************************************************************
// Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
// *****************************************************************************

#include "kalsim.h"

// -- setup C runtime, and call main() --
.MODULE $M.crt0;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

   $_crt0:

   call $_pm_ram_initialise;
   call $_dm_initialise;
   call $_stack_initialise;

   // The compiler expects these registers set up this way.
   // See the ABI in CS-124812-UG.
   M0 = 0;
   M1 = ADDR_PER_WORD;
   M2 = -ADDR_PER_WORD;
   // The compiler also expects L0=L1=L4=L5=0.
   // Note: at this point this seems guaranteed by prior code.

   call $_main;
   jump $_terminate;
.ENDMODULE;

.MODULE $M.dm_initialise;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;
   // $_pm_code_limit is used by the patch code to ensure that code patches
   // don't overwrite existing PM RAM contents. Its value is set when code
   // gets copied into PM RAM.
   .VAR $_pm_code_limit;

$_dm_initialise:

    // Initialise the .mem_guard region
    r10= $DM_INIT_MEM_GUARD_SIZE_DWORDS;
    I1 = $MEM_MAP_DM_INITC_MEM_GUARD_ROM_ADDR;
    I0 = $MEM_MAP_DM_GUARD_START;
    do initc_mem_guard_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_mem_guard_copy_loop:

    // Initialise the .initc_dm2 region
    r10 = $DM2_INIT_SIZE_DWORDS;
    I1 =  $MEM_MAP_DM2_INITC_ROM_ADDR;
    I0 =  $MEM_MAP_DM2_INITC_START;
    do initc_dm2_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm2_copy_loop:

    // Initialise the .initc_dm1 region
    r10 = $DM1_INIT_SIZE_DWORDS;
    I1 = $MEM_MAP_DM1_INITC_ROM_ADDR;
    I0 = $MEM_MAP_DM1_INITC_START;
    do initc_dm1_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm1_copy_loop:

    // Common shared regions are initialised by P0 only.
    // Initialise the .initc_dm1_p0 region
    r10= $DM1_INIT_P0_SIZE_DWORDS;
    I1 = $MEM_MAP_DM1_INITC_P0_ROM_ADDR;
    I0 = $MEM_MAP_DM1_P0_INITC_START;
    do initc_dm1_p0_copy_loop;
        r1 = M[I1,4];
        M[I0,4] = r1;
    initc_dm1_p0_copy_loop:

    // Zero initialise the dm1 P0 .bss data region
    I0 = $MEM_MAP_DM1_P0_BSS_START;
    r10 = $MEM_MAP_DM1_P0_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm1_bss_p0_zero_loop;
        M[I0,4] = r2;
    dm1_bss_p0_zero_loop:

    // Store the correct limit of used PM RAM
    r1 = $PM_RAM_PATCH_CODE_START;
    // Linker symbol has MSB set for code, so mask it off to get the real address
    r1 = r1 AND 0x7FFFFFFF;
    M[$_pm_code_limit] = r1;

    // Zero initialise the dm1 .bss data region
    I0 = $MEM_MAP_DM1_BSS_START;
    r10 = $MEM_MAP_DM1_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm1_bss_zero_loop;
        M[I0,4] = r2;
    dm1_bss_zero_loop:

    // Zero initialise the dm2 .bss data region
    I0 = $MEM_MAP_DM2_BSS_START;
    r10 = $MEM_MAP_DM2_BSS_LENGTH_DWORDS;
    r2 = 0;
    do dm2_bss_zero_loop;
        M[I0,4] = r2;
    dm2_bss_zero_loop:

   rts;
.ENDMODULE;

// Zero all the PM RAM, to make sure the cache behaves itself

.MODULE $M.pm_ram_initialise;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

$_pm_ram_initialise:

   // Enable the windows so PM RAM is visible in DM space
   r0 = 1;
   M[$PMWIN_ENABLE] = r0;

   L0 = 0;
   L1 = 0;

   // Clear all the PM RAM via the windows

    // Only P0 clear the PM RAM
   I0 = PM_RAM_WINDOW;
   r1 = 0;
   r10 = PM_RAM_SIZE_WORDS;

   do pm_zero_loop;
      M[I0,ADDR_PER_WORD] = r1;
   pm_zero_loop:

   // Copy code to be run from RAM
   r10 = $PM_INIT_SIZE_DWORDS;
   I1 = $MEM_MAP_PM_INIT_ROM_ADDR;
   I0 = $MEM_MAP_RAM_CODE_START;
   do init_pm_copy_loop;
       r1 = M[I1,4];
       M[I0,4] = r1;
   init_pm_copy_loop:

   // Enable external exceptions related to PM banks
   r1 = M[$EXT_EXCEPTION_EN];
   r1 = r1 OR $PM_BANK_EXCEPTION_EN_MASK;
   M[$EXT_EXCEPTION_EN] = r1;

   // Enable programmable exception regions for PM
   // Default cache region is defined at build time.
   r0 = PM_RAM_CACHE_BANK_SIZE_WORDS;

   // We don't have a stack yet, so stash rLink in another register
   r2 = rLink;
   call $_pm_exception_region_init;
   rLink = r2;

   // Disable the window again
   r0 = 0;
   M[$PMWIN_ENABLE] = r0;

   rts;
.ENDMODULE;

// Set an exception region covering the P0 PM cache
// Call with r0 = cache size in words

.MODULE $M.pm_exception_region_init;
   .CODESEGMENT EXT_DEFINED_PM;
   .DATASEGMENT DM;

// void pm_exception_region_init(unsigned cache_size_words);

$_pm_exception_region_init:

   // Enable programmable exception regions for PM
   // Cache region start is defined at build time.
   r1 = PM_RAM_START_ADDRESS + PM_RAM_WINDOW_CACHE_START_ADDRESS_OFFSET;
   M[$PM_PROG_EXCEPTION_REGION_START_ADDR] = r1;

   r0 = r0 LSHIFT 2; // Size in bytes
   r1 = r1 + r0; // Start address + size
   r1 = r1 - 1; // Region start and end addresses are inclusive.
   M[$PM_PROG_EXCEPTION_REGION_END_ADDR] = r1;

   r1 =  M[$PROG_EXCEPTION_REGION_ENABLE];
   r1 = r1 OR $PM_PROG_EXCEPTION_REGION_ENABLE_MASK;
   M[$PROG_EXCEPTION_REGION_ENABLE] = r1;

   rts;
.ENDMODULE;


// -- abort, exit, terminate routine -- !!Quits with kalsim, loops on a real chip!!
.MODULE $M.abort_and_exit;
   .CODESEGMENT EXT_DEFINED_PM;

   $_abort:
   r0 = -1;

   $_exit:
   /* For calls to exit, r0 is set to exit code already */

   $_finish_no_error:
   $_terminate:
   TERMINATE
.ENDMODULE;
