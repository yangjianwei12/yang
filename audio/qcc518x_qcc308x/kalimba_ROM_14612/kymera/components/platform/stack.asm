// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.
// *****************************************************************************

// *****************************************************************************
// NAME:
//    Stack Library
//
// DESCRIPTION:
//    This library has functions and macros to provide a simple software
//    stack.
//
//    The Kalimba DSP has an rLink register that is used to hold a subroutine
//    return address.  When a subroutine call is performed by a CALL
//    instruction, rLink is set to the subroutine return address.  The
//    subroutine return is preformed by an RTS instruction, its effect is
//    equivalent to a JUMP to the value of the rLink register.
//
//    When a subroutine is nested within another, a method of saving and
//    restoring the value of the rLink register at the start and end of the
//    subroutine is required.  A software stack that allows pushing and
//    popping the rLink register is the most convienient way to achieve this.
//    This functionality is supported by the Stack library.
//
//    The default size of the stack is 32 words, this may be changed by editing
//    the definition of $stack.SIZE and rebuilding the library
//
//    When developing applications it is useful to be able to catch stack
//    overflows or underflows to find bugs in a program.  By defining
//    DEBUG_STACK_OVERFLOW macro at the top of your application source code
//    extra code is inserted to detect stack over and underflows.
//
//
//    Planned use is as follows:
//
//    @verbatim
//    .MODULE $test;
//
//       // push rLink onto stack
//       push rLink;
//
//       ...    // your code
//       ...
//
//       // pop rLink from stack
//       // Traditional method
//       jump $pop_rLink_and_rts;
//
//       // Alternative method
//       pop rLink;
//       rts;
//
//    .ENDMODULE;
//    @endverbatim
//
// *****************************************************************************
#ifndef STACK_INCLUDED
#define STACK_INCLUDED

// FIXME: having these hardcoded on causes extra power consumption, so
// arguably this should be run-time configurable.
// However, Dave Hargreaves says that the likely power penalty is
// negligible (2014-09).
#define ENABLE_DEBUG_PERFORMANCE_COUNTERS

#ifdef DEBUG_ON
   #ifndef DEBUG_STACK_OVERFLOW
   #define DEBUG_STACK_OVERFLOW
   #endif
#endif

#include "stack.h"
#include "architecture.h"
#include "interrupt.h"
#include "portability_macros.h"

.MODULE $stack;
   .DATASEGMENT DM;

   // Small stack used during boot
   .CONST $stack.BOOT_SIZE 62;
   .VAR/DM_SHARED_ZI boot_buffer[$stack.BOOT_SIZE];

   // The following variables are present to allow ACAT to find the stack
   // again in case it is analysing a coredump using the exception stack.
   .VAR start;
   .VAR size;

.ENDMODULE;





// *****************************************************************************
// MODULE:
//    $stack.initialise
//
// DESCRIPTION:
//    Initialise the software stack
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0
//
// *****************************************************************************

.MODULE $M.stack.initialise;
   .CODESEGMENT STACK_INITIALISE_PM;
   .DATASEGMENT DM;

   #ifdef DEBUG_ON
      .CONST MAGIC_KEY    0x445248;
      .VAR stack_initialised_magic_key = MAGIC_KEY;
   #endif

   $stack.initialise:
   $_stack_initialise:

   #ifdef DEBUG_ON
      // The stack should only be initialised once - we check that it
      // hasn't been initialised before by confirming the magic key is set.
      r0 = M[stack_initialised_magic_key];
      Null = r0 - MAGIC_KEY;
      // If error is called here it will most probably be because of memory
      // corruption in the stack buffer or some rogue code jumping to address 0
      // or the like.
      if NZ call $error;
      M[stack_initialised_magic_key] = Null;
   #endif


   r0 = &$stack.boot_buffer;
   r1 = $stack.BOOT_SIZE *ADDR_PER_WORD;
   M[$stack.start] = r0;
   M[$stack.size] = r1;

   r1 = &$stack.boot_buffer + ($stack.BOOT_SIZE-1)*ADDR_PER_WORD;

   stack_set_ok:
   M[$STACK_END_ADDR] = r1;
   M[$STACK_START_ADDR] = r0;
   M[$STACK_POINTER] = r0;
   M[$FRAME_POINTER] = r0;

   // Zero the stack contents
   I0 = r0;
   r10 = $stack.BOOT_SIZE;
   r1 = 0;
   
   do zero_stack_loop;
   M[I0, MK1] = r1;
zero_stack_loop:

   #if defined(ENABLE_DEBUG_PERFORMANCE_COUNTERS)
       // enable the performance counters in debug mode
       r0 = 1;
       M[$DBG_COUNTERS_EN] = r0;
   #endif
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $stack.pop_rLink_and_rts
//
// DESCRIPTION:
//    Shared code to pop rLink from the stack and rts.
//    Important: Code must be called with a jump instruction rather than a call.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************
.MODULE $M.pop_rLink_and_rts;
   .CODESEGMENT POP_RLINK_AND_RTS_PM;

   $pop_rLink_and_rts:
   pop rLink;
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $stack.set_stack_regs
//
// DESCRIPTION:
//    Change the stack of the current processor. 
//
// INPUTS:
//    - r0 contains the start address to the buffer to use as the new stack.
//    - r1 contains the number of octets in the buffer to use as the new stack.
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0, r1 and r2
//
// *****************************************************************************
.MODULE $M.set_stack_regs;
   .CODESEGMENT STACK_INITIALISE_PM;
   .DATASEGMENT DM;

$_set_stack_regs:

   pushm <r0, rLink>;
   call $_interrupt_block;
   popm <r0, rLink>;
   M[$stack.start] = r0;
   M[$stack.size] = r1;

   r2 = r1 + r0;
   r2 = r2 - ADDR_PER_WORD;

   r3 = M[$EXCEPTION_EN];
   M[$EXCEPTION_EN] = 0;

   M[$STACK_END_ADDR] = r2;
   M[$STACK_START_ADDR] = r0;
   M[$STACK_POINTER] = r0;
   M[$FRAME_POINTER] = r0;

   // Zero the stack contents
   I3 = r0;
   r10 = r1 LSHIFT -2;
   r1 = 0;
   
   do zero_stack_loop;
   M[I3, MK1] = r1;
zero_stack_loop:

   pushm <FP(=SP)>;

   M[$EXCEPTION_EN] = r3;
   push rLink;
   call $_interrupt_unblock;
   pop rLink;
   rts;
.ENDMODULE;

#endif
