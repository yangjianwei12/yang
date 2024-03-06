/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

/**
 * This file contains assembly code for the Kalimba FreeRTOS port.
 *
 * Including the interrupt handler, task start, shallow sleep and yield.
 */

#include "kaldwarfregnums.h"
#include "dwarf_constants.h"


/**
 * \brief Pop the caller saved registers from the current stack.
 *
 * Caller saved registers are the registers that code initiating a function call
 * must save before performing the call as they are not guaranteed to be
 * unmodified on return from the call.
 *
 * The compiler assumes the following scratch registers are destroyed during a
 * function call, meaning if our ISR handler adheres to the Kalimba calling
 * convention (which is what we're assuming with this interrupt stack layout)
 * there's no guarantee these registers will not be modified by the function
 * call so need saving.
 *
 * Scratch registers:
 * rMAC, R0, R1, R2, R3, R10, I3, I7, M3
 *
 * The 'call' instruction itself will load the current PC+4 into rLink so rLink
 * also needs to be part of the caller saved register set.
 *
 * The interrupt handler uses the stack so FP should also be stored on entry
 * to the interrupt handler, making it part of the caller saved register set.
 *
 * There are some fixed values that compiler relies on, typically these
 * shouldn't change, but it's possible some assembler code temporarily changes
 * them and in that case we need to have stacked the values to context switch
 * correctly.
 *
 * Fixed registers:
 * M0=0, M1=4, M2=-4, L0=0, L1=0, L4=0, L5=0
 *
 * Some memory mapped registers could be in use, for example if the task we're
 * interrupting is performing a divide. These need stacking before we call the
 * interrupt handler.
 *
 * Memory mapped registers:
 * DoLoopStart, DoLoopEnd, DivResult, DivRemainder, ArithmeticMode
 *
 * The Bank 3 registers for circular buffer access could also have been modified
 * by assembly code or the Embedded C extensions for circular buffers. 
 * These also need to be stacked prior to a function call as functions make 
 * no attempt to retain their values.
 *
 * Circular buffer registers:
 * B0, B1, B4, B5
 */
#define POP_CALLER_SAVED_REGISTERS \
    popm <rMAC2, rMAC12, rMAC0, DoLoopStart, DoLoopEnd, DivResult, DivRemainder, B0, B1, B4, B5>; \
    popm <I3, I7, M0, M1, M2, M3, L0, L1, L4, L5>; \
    popm <r0, r1, r3, r10, rLink>; \
    M[$MM_RINTLINK] = r1; \
    M[$ARITHMETIC_MODE] = r0; \
    popm <FP, r0, r1, r2, rFlags>;

/**
 * \brief Pop the callee saved registers from the current stack.
 *
 * Callee saved registers are the registers that a function needs to save and
 * restore if it is going to modify them. Splitting our context into caller and
 * callee registers allows the interrupt handler to avoid ever stacking the
 * callee saved registers if it is not goint to perform a context switch.
 *
 * The full list of registers that will potentially be saved by a function
 * call is:
 * FP, R4, R5, R6, R7, R8, R9, rLink, rMACB, I0, I1, I2, I4, I5, I6
 *
 * The stack is ordered so that we can push and pop the relevant registers in as
 * few instructions as possible.
 */
#define POP_CALLEE_SAVED_REGISTERS \
    popm <rMACB2, rMACB12, rMACB0>; \
    popm <I0, I1, I2, I4, I5, I6>; \
    popm <r4, r5, r6, r7, r8, r9>;

/**
 * \brief Pops the callee followed by the caller registers
 */
#define POP_ALL_REGISTERS_FROM_TASK \
    POP_CALLEE_SAVED_REGISTERS \
    POP_CALLER_SAVED_REGISTERS

/**
 * \brief Set the stack pointer.
 *
 * Sets the stack pointer and updates the HW stack overflow detection.
 *
 * One way to do this is to disable exceptions whilst modifying the stack start
 * and end address, however this clears the exception state. Another way is to
 * set the start / end values in an order that doesn't cause the CPU to detect
 * an out of bounds access, that's what has been done here. See B-299171 for
 * more information.
 *
 * Note that there must be at least one instruction after writing to
 * $STACK_POINTER before the SP register sees the new value.
 *
 * Trashes: r10, $STACK_START_ADDR, $STACK_POINTER, $STACK_END_ADDR
 */
#define SET_STACK_POINTER(_sp, _start, _end) \
    M[$STACK_START_ADDR] = Null; /* Move stack start address to the beginning of memory. */ \
    r10 = 0xFFFFFFFC;         /* Move stack end address to the end of memory (word aligned). */ \
    M[$STACK_END_ADDR] = r10; \
    M[$STACK_POINTER] = _sp; \
    M[$STACK_START_ADDR] = _start; \
    M[$STACK_END_ADDR] = _end;

/**
 * Offsets for reading the stack start and end address from a TCB.
 */
#define STACK_START_TCB_OFFSET (48)

/**
 * \brief Sets the stack pointer register to the stack value in the current TCB
 *
 * Trashes: r0, r1, r2, r3, r10,
 *          $STACK_START_ADDR,
 *          $STACK_POINTER,
 *          $STACK_END_ADDR
 */
#define SET_STACK_POINTER_FROM_CURRENT_TCB \
    r0 = M[$_pxCurrentTCB];              /* Load pointer to current TCB */ \
    r1 = M[r0];                          /* Load stack pointer. */ \
    r2 = M[r0 + STACK_START_TCB_OFFSET]; /* Load stack start address. */ \
    r3 = M[r2];  /* Load stack end address, first entry on the stack. */ \
    SET_STACK_POINTER(r1, r2, r3)

/**
 * \brief Starts the first task
 */
.MODULE $vPortStartFirstTaskAsm;
    .CODESEGMENT PM;
    .MINIM;
$_vPortStartFirstTaskAsm:
    /* Save the current stack pointer from main for the interrupt handler to
       use. */
    r0 = M[$STACK_POINTER];
    M[$_interrupt_stack] = r0;
    SET_STACK_POINTER_FROM_CURRENT_TCB
    /* Restore the new context. */
    POP_ALL_REGISTERS_FROM_TASK
    /* Unblock interrupts before returning to the first task.
       Before starting the scheduler FreeRTOS blocks interrupts assuming each
       task will store the interrupt blocked / unblocked state in its stack
       frame. */
    M[$INT_BLOCK_PRIORITY] = Null;
    /* rti rather than rts so the rIntLink register is used as the destination PC
       and the rIntFlags is shifted down into rFlags for us. */
    rti;
.ENDMODULE;

/**
 * ASM constants can't be used in some places in an instruction.
 * To avoid duplicating these constants it's redefined here even though it's
 * already defined in io_defs.h.
 */
#define UM_FLAG_MASK_DEF (0x80)

/**
 * \brief Yields to another task of the same priority
 */
.MODULE $vPortYieldAsm;
    .CODESEGMENT PM;
    .MINIM;
$_vPortYieldAsm:
.vPortYieldAsm_Start:
    /* This function is always called by the FreeRTOS port code from user mode
       with interrupts blocked.

       First exit user mode to prevent an interrupt descheduling us during the
       context switch. This needs to happen before we unblock interrupts later
       in this function. We must unblock interrupts here as the block interrupts
       state isn't part of the task context.

       Some FreeRTOS ports store the interrupt state within the context and can
       support yielding to another task from within a critical section. The
       Kalimba port just defers the yield until the end of the critical section
       and always resumes tasks with interrupts unblocked.

       Note that if interrupts weren't blocked it would be possible for an
       interrupt to occur one cycle after clearing the user mode (UM) flag, this
       would result in an interrupt being taken whilst we're not in user mode
       which would confuse the interrupt handler. Hence, we also need to block
       interrupts before this call and unblock interrupts after we are sure an
       interrupt won't be taken. We rely on setting the user mode (UM) flag
       allowing interrupts again as part of the return to a new task using rti.
    */
    rFlags = rFlags AND ~UM_FLAG_MASK_DEF;

    /* Save the current context. */
    pushm <FP(=SP), r0, r1, r2, rFlags>;
.vPortYieldAsm_PUSH_FP:  /* label for annotation of the FP push */
    /* pop rFlags so we can emulate an interrupt stack frame. */
    pop r0;
    r0 = r0 AND 0xff;
    /* Set the user mode flag in the copy of rFlags that we're pushing to the
       stack. */
    r0 = r0 OR UM_FLAG_MASK_DEF;
    /* Left shift by 8 so the 'INT' section of the rFlags copy are populated.
       When this value is popped using 'rti' this value will be shifted down
       8 bits into the user rFlags. */
    r0 = r0 LSHIFT 8;
    push r0;

    /* Duplicate rLink in the rIntLink position as we use rti to return to
       a new task and it uses the value in rIntLink to return to. */
    r0 = M[$ARITHMETIC_MODE];
    r1 = rLink;
    pushm <r0, r1, r3, r10, rLink>;
.vPortYieldAsm_PUSH_rLink:
    pushm <I3, I7, M0, M1, M2, M3, L0, L1, L4, L5>;
    pushm <rMAC2, rMAC1, rMAC0, DoLoopStart, DoLoopEnd, DivResult, DivRemainder, B0, B1, B4, B5>;
    pushm <r4, r5, r6, r7, r8, r9>;
.vPortYieldAsm_PUSH_r4_r9:
    pushm <I0, I1, I2, I4, I5, I6>;
    pushm <rMACB2, rMACB1, rMACB0>;

    /* Exit the critical section now we're safely out of user mode and have
       stacked the callers registers do don't need to preserve them any
       longer. */
    M[$INT_BLOCK_PRIORITY] = Null;

    /* Store the new top of the stack to the TCB. */
    r0 = M[$_pxCurrentTCB];
    r1 = M[$STACK_POINTER];
    M[r0] = r1;

    /* Switch task. */
    call $_vTaskSwitchContext;
    SET_STACK_POINTER_FROM_CURRENT_TCB

    /* Restore the new context. */
    POP_ALL_REGISTERS_FROM_TASK

    /* rti rather than rts so the rIntLink register is used as the destination
       PC and the rIntFlags is shifted down into rFlags for us. This will result
       in the user mode (UM) bit being set effectively unblocking interrupts. */
    rti;
.vPortYieldAsm_End:
.ENDMODULE;

/**
 * Optionally record the PC that was interrupted to run the interrupt handler.
 *
 * rIntLink contains the value for the PC on return from an interrupt. Recording
 * a history of it shows us where tasks have been interrupted in the past. This
 * can be useful for debugging and testing interrupt blocking problems.
 */
#ifdef RECORD_INTERRUPTED_PC_HISTORY
#define INTERRUPTED_PC_HISTORY_ELEMENTS (4)
#define RECORD_INTERRUPTED_PC_DATA \
    .VAR interrupted_pc_history[INTERRUPTED_PC_HISTORY_ELEMENTS]; \
    .VAR invocations = 0;
#define RECORD_INTERRUPTED_PC \
    r0 = M[$M.interrupt.handler.invocations]; \
    r1 = r0 AND (INTERRUPTED_PC_HISTORY_ELEMENTS - 1); \
    r1 = r1 LSHIFT 2; \
    r2 = M[$MM_RINTLINK]; \
    M[$M.interrupt.handler.interrupted_pc_history + r1] = r2; \
    r0 = r0 + 1; \
    M[$M.interrupt.handler.invocations] = r0;
#else
#define RECORD_INTERRUPTED_PC_DATA
#define RECORD_INTERRUPTED_PC
#endif

/**
 * The start address of the interrupt stack, non-zero if the scheduler has been
 * started.
 */
.VAR/DM $_interrupt_stack = 0;

/**
 * Indicates whether a context switch should be performed on exit from the
 * interrupt. Set by portYIELD_FROM_ISR().
 */
.VAR/DM $_context_switch_on_return_from_interrupt = 0;

/**
 * \brief Check whether we're supposed to be in the interrupt handler.
 *
 * A hardware issue (see B-174186) means that it's possible for an interrupt to
 * be handled a few instructions after they have been blocked. Work around that
 * here by checking in software whether the block interrupts register has been
 * set or not.
 *
 * Note that blocking interrupts should not block exception level interrupts
 * so the check must be against the currently running interrupt priority level.
 * Since exceptions aren't common and hitting the block interrupts race isn't
 * common we test if block interrupts is 0 first and continue as quickly as
 * possible, the priority check only happens if we hit the race or this is
 * an exception.
 */
#define CHECK_INT_BLOCK_PRIORITY \
    Null = M[$INT_BLOCK_PRIORITY]; \
    if Z jump interrupts_enabled; \
    /* The block priority may be non-zero but the interrupt still needs \
       executing if this interrupt priority is >= INT_BLOCK_PRIORITY */ \
    push r0; \
    push r1; \
    r0 = M[$INT_SAVE_INFO]; \
    r0 = r0 AND $INT_SAVE_INFO_PRIORITY_NEW_MASK; \
    r1 = M[$INT_BLOCK_PRIORITY]; \
    r1 = r1 LSHIFT $INT_SAVE_INFO_PRIORITY_NEW_POSN; \
    /* Abort the interrupt if the block priority is greater than the interrupt \
       priority. */ \
    Null = r1 - r0; \
    if LS jump higher_priority_interrupts_enabled; \
    /* Don't handle this interrupt it should have been blocked, reset the \
       interrupt state so it appears as though we haven't handled it. */ \
    r0 = M[$INT_SAVE_INFO]; \
    r0 = r0 OR $INT_LOAD_INFO_DONT_CLEAR_MASK; \
    M[$INT_LOAD_INFO] = r0; \
    pop r1; \
    pop r0; \
    rti; \
higher_priority_interrupts_enabled: \
    pop r1; \
    pop r0; \
interrupts_enabled:

/**
 * Optional feature to measure the time spent in shallow sleep.
 * Useful for measuring CPU utilisation.
 */
#ifdef ENABLE_SHALLOW_SLEEP_TIMING
   /* Variables to support measuring time spent in shallow sleep */
   .VAR/DM $_total_shallow_sleep_time = 0;

/**
 * Start the shallow sleep timer, should be called with interrupts blocked.
 * Start time is stored in r0.
 */
#define SHALLOW_SLEEP_TIMING_START \
    r0 = M[$TIMER_TIME];

#define SHALLOW_SLEEP_TIMING_STOP \
    rMAC = M[$TIMER_TIME]; \
    rMAC = rMAC - r0; \
    rMAC = rMAC + M[$_total_shallow_sleep_time]; \
    M[$_total_shallow_sleep_time] = rMAC;
#else
#define SHALLOW_SLEEP_TIMING_START
#define SHALLOW_SLEEP_TIMING_STOP
#endif

/**
 * If enabled keeps a count of the number of times each interrupt source has
 * been handled.
 *
 * Trashes: _tmp
 */
#ifdef LOG_IRQS
#define LOG_IRQ(_reg0, _tmp) \
    _tmp = M[_reg0 + &$_irq_counts]; \
    _tmp = _tmp + 1; \
    M[_reg0 + &$_irq_counts] = _tmp;
#else
#define LOG_IRQ(_reg0, _tmp)
#endif

/**
 * This file can be compiled for kalsim which simulates the Audio Kalimba core.
 */
#ifdef SUBSYSTEM_AUDIO
/* Audio does not have the CLKGEN_CORE_CLK_RATE register. */
#define RESET_CORE_CLK_RATE
#else
#define RESET_CORE_CLK_RATE M[$CLKGEN_CORE_CLK_RATE] = Null;
#endif

/**
 * The latency recording feature records the maximum time we spend in an
 * interrupt and the maximum time spent with interrupts blocked.
 *
 * The apps1.fw.env.var.latency variable in port.c stores this information.
 */
#ifdef RECORD_LATENCY
/* We're using rMAC to hold the interrupt nest count across this call.
   rMAC is a caller saved register so must be stacked before calling. */
#define INTERRUPT_TIMING_START call $_xPortInterruptTimingStart;
#define INTERRUPT_TIMING_STOP call $_xPortInterruptTimingStop;
#else
#define INTERRUPT_TIMING_START
#define INTERRUPT_TIMING_STOP
#endif

/**
 * \brief Interrupt handler entry point.
 *
 * The assembly code in this routine must avoid modifying any callee saved
 * registers.
 *
 * i.e. Do not modify r4, r5, r6, r7, r8, r9, I0, I1, I2, I4, I5, I6 or rMACB.
 *
 * If these registers are modified here then they may not have been saved on the
 * stack yet since they are only stacked if a context switch actually occurs
 * which is at the end of this function.
 */
.MODULE $M.interrupt.handler;
   .CODESEGMENT INTERRUPT_HANDLER_PM;
   .DATASEGMENT DM;
   RECORD_INTERRUPTED_PC_DATA
   .MINIM;
   $interrupt.handler:
   $_interrupt_handler:
.interrupt_handler_Start:
    /* If the interrupt has fired because of a stack overflow alert
       P0 and loop in P1 to avoid using any stack. */
    Null = M[$STACK_OVERFLOW_PC];
    if NZ jump error_stack_overflow;
    /* Return early if interrupts are blocked, this can happen as clearing and
       setting the interrupt blocking related registers takes a few (<5) cycles
       to take effect. */
    CHECK_INT_BLOCK_PRIORITY
    /* Push caller saved registers. */
    pushm <FP(=SP), r0, r1, r2, rFlags>;
.interrupt_handler_Push1:
    r0 = M[$ARITHMETIC_MODE];
    r1 = M[$MM_RINTLINK];
    pushm <r0, r1, r3, r10, rLink>;
    pushm <I3, I7, M0, M1, M2, M3, L0, L1, L4, L5>;
    pushm <rMAC2, rMAC1, rMAC0, DoLoopStart, DoLoopEnd, DivResult, DivRemainder, B0, B1, B4, B5>;
.interrupt_handler_Push2:

    /* Ideally the timer would be from when the interrupt was raised. However
       we're trying to catch excessively long interrupt durations so a few
       microseconds from interrupt to here shouldn't be a big problem.
       This musn't rely on or modify the compiler fixed registers as they
       haven't been reset yet, so could use values modified by the interrupted
       task. */
    INTERRUPT_TIMING_START

    /* Reset the compiler fixed registers. */

    /* Reset arithmetic mode */
    M[$ARITHMETIC_MODE] = Null;

    /* Clear bit reverse flag */
    r0 = $BR_FLAG_MASK;
    r0 = r0 XOR -1;
    rFlags = rFlags AND r0;

    /* Clear the length registers */
    L0 = Null;
    L1 = Null;
    L4 = Null;
    L5 = Null;

    /* Clear the base registers */
    push Null;
    B5 = M[SP - 4];
    B4 = M[SP - 4];
    B1 = M[SP - 4];
    pop B0;

    /* Set the modify registers as the compiler expects */
    M0 = Null;
    M1 = 4;
    M2 = -4;

    RECORD_INTERRUPTED_PC

    /* Enter a new nest level. */
    /* Only need to switch stack at the first interrupt nesting level. */
    rMAC = MBU[$_interrupt_nest_count];
    if NZ jump dont_switch_stack;
    /* No need to switch the stack if the scheduler is not running yet */
    r0 = M[$_interrupt_stack];
    if Z jump dont_switch_stack;
    r1 = SP; /* Remember the old stack pointer for exiting this nest level. */
    /* Set the stack extremes to the main stack values $MEM_MAP_STACK_START and
       $STACK_SIZE_BYTES - STACK_OVERRUN_PROTECTION_BYTES */
    r2 = $MEM_MAP_STACK_START;
    r3 = r2 + ($STACK_SIZE_BYTES - STACK_OVERRUN_PROTECTION_BYTES);

    /* The stack pointer is switched here but the frame pointer is not. A
       debugger will unwind using the frame pointer and so unwind through the
       interrupted task's stack, which is the intention. */
    SET_STACK_POINTER(r0, r2, r3)

    /* Store the interrupted task's stack pointer on the interrupt stack.
       Alternatively, this could be stored directly in the pxTopOfStack
       member of the current TCB. However, storing it on the stack is a bit
       faster at the cost of 4 extra bytes of memory. */
    push r1;

dont_switch_stack:
    rMAC = rMAC + 1;
    MB[$_interrupt_nest_count] = rMAC;

    /* Block interrupts.

       This takes a few cycles to take effect, but we don't have to worry
       about that as UM=0 (effectively blocking interrupts), and won't
       become 1 for more than a few cycles. */
    M[$INT_UNBLOCK] = Null;

    /* The scheduler might have work to do. */
    M[$GOTO_SHALLOW_SLEEP] = Null;

    /* Save the interrupt hardware context to support nested interrupts. */
    r0 = M[$INT_SAVE_INFO];
    push r0;

    /* Acknowledge the interrupt.
       After this, INT_SOURCE is valid up until interrupts are re-enabled. */
    M[$INT_ACK] = Null;

    /* Load the 5-bit interrupt source register. */
    r0 = M[$INT_SOURCE];
    /* Multiply by 4 to calculate the array offset. */
    r0 = r0 LSHIFT 2;
    r1 = M[r0 + &$_isr_call_table];

    /* Update irq counters if enabled. */
    LOG_IRQ(r0, r2)

    /* Un-block interrupts for the duration of the handler call. */
    rFlags = $UM_FLAG_MASK;
    r0 = 1;
    M[$INT_UNBLOCK] = r0;

    /* The stack layout assumes that this function uses the Kalimba calling
       convention. */
    call r1;

    /* Exiting user-mode doesn't block interrupts until one instruction after
       the write has been executed. Add a nop so we know it's safe for an
       interrupt to occur on the next instruction. */
    rFlags = Null;
    nop;

    /* The old value of INT_SAVE_INFO needs to be written to INT_LOAD_INFO
       to re-enable interrupts at this level. */
    pop r0;
    r0 = r0 OR $INT_LOAD_INFO_DONT_CLEAR_MASK;
    M[$INT_LOAD_INFO] = r0;

    /* Exit the current nest level. */
    r0 = MBU[$_interrupt_nest_count];
    r0 = r0 - 1;
    MB[$_interrupt_nest_count] = r0;
    if NZ jump restore_context_and_return;

    /* It's possible we're context switching to a new task after being in deep
       sleep in the idle task. If that's the case we need to send an IPC message
       to P0 so it knows P1 is no longer ready to deep sleep. */
    Null = M[$_context_switch_on_return_from_interrupt];
    if NZ call $_dorm_disable_deep_sleep_if_enabled;

    /* We're at nest level 0. */
    /* No need to update the stack if the scheduler isn't running. */
    Null = M[$_interrupt_stack];
    if Z jump restore_context_and_return_from_nest_level_0;
    /* Nest level 0 and scheduler is running, restore the task stack. */
    r0 = M[$_pxCurrentTCB]; /* Load pointer to current TCB */
    pop r1; /* Load stack pointer from the current stack. */
    r2 = M[r0 + STACK_START_TCB_OFFSET]; /* Load stack start address. */
    r3 = M[r2]; /* Load stack end address, first entry on the stack. */
    SET_STACK_POINTER(r1, r2, r3)

    /* Has a context switch been requested? */
    Null = M[$_context_switch_on_return_from_interrupt];
    if Z jump restore_context_and_return_from_nest_level_0;

    /* We've decided to perform context switch. */
    M[$_context_switch_on_return_from_interrupt] = Null;

    /* Append the callee saved registers to convert the interrupt stacked
       registers into a full CPU context. */
    pushm <r4, r5, r6, r7, r8, r9>;
    pushm <I0, I1, I2, I4, I5, I6>;
    pushm <rMACB2, rMACB1, rMACB0>;

    /* Store stack pointer to current TCB */
    r0 = M[$_pxCurrentTCB];
    r1 = M[$STACK_POINTER];
    M[r0] = r1;

    /* Switch task. */
    call $_vTaskSwitchContext;
    SET_STACK_POINTER_FROM_CURRENT_TCB

    /* Restore the new context. */
    POP_CALLEE_SAVED_REGISTERS
restore_context_and_return_from_nest_level_0:
    INTERRUPT_TIMING_STOP
restore_context_and_return:
    /* POP_CALLER_SAVED_REGISTERS is inlined here so labels can be added for
       debug annotations. */
    popm <rMAC2, rMAC12, rMAC0, DoLoopStart, DoLoopEnd, DivResult, DivRemainder, B0, B1, B4, B5>;
    popm <I3, I7, M0, M1, M2, M3, L0, L1, L4, L5>;
    popm <r0, r1, r3, r10, rLink>;
    M[$MM_RINTLINK] = r1;
    M[$ARITHMETIC_MODE] = r0;
    popm <FP, r0, r1, r2, rFlags>;
.interrupt_handler_Pop1:
    rti;
.interrupt_handler_End:
error_stack_overflow:
    /* Trigger the P0 panic_interrupt_handler() in the ipc module. */
    M[$P1_TO_P0_INTERPROC_EVENT_2] = Null;
$stack_overflow_error:
    jump $stack_overflow_error;
.ENDMODULE;

/**
 * \brief Enter shallow sleep mode until an interrupt occurs.
 *
 * Interrupts must be blocked when calling this function.
 */
.MODULE $M.enter_shallow_sleep;
    .CODESEGMENT PM;
    .MINIM;
$_enter_shallow_sleep:
.enter_shallow_sleep_Start:
#ifndef DISABLE_SHALLOW_SLEEP
#ifdef DEBUG_SLEEP_ON_PIO
    r0 = M[$APPS_SYS_PIO_DRIVE + (DEBUG_SLEEP_ON_PIO/32)];
    r0 = r0 OR (1<<(DEBUG_SLEEP_ON_PIO & 0x1f));
    M[$APPS_SYS_PIO_DRIVE + (DEBUG_SLEEP_ON_PIO/32)] = r0;
#endif

    /* Stores the current time in r0. */
    SHALLOW_SLEEP_TIMING_START

    /* Interrupts are blocked so if an interrupt happens here we will
       immediately wake from sleep and continue in this context until
       interrupts are unblocked. */
    rMAC = 1;
    M[$GOTO_SHALLOW_SLEEP] = rMAC;

    /* Kalimba can take 2 cycles to go to sleep, hence 2 nops. */
    nop;
    nop; /* Processor sleeps on this instruction until an interrupt fires. */

    /* Resume normal clock rate, interrupts are still blocked so it's not
       possible for an interrupt handler to have started executing with a
       lowered clock rate. */
    RESET_CORE_CLK_RATE

    /* Stop the timer after resetting the clock rate to minimise the time we
       spend at the reduced clock rate. */
    SHALLOW_SLEEP_TIMING_STOP

#ifdef DEBUG_SLEEP_ON_PIO
    r0 = M[$APPS_SYS_PIO_DRIVE + (DEBUG_SLEEP_ON_PIO/32)];
    r0 = r0 AND ~(1<<(DEBUG_SLEEP_ON_PIO & 0x1f));
    M[$APPS_SYS_PIO_DRIVE + (DEBUG_SLEEP_ON_PIO/32)] = r0;
#endif
#endif
    rts;
.enter_shallow_sleep_End:
.ENDMODULE;

/**
 * When RECORD_LATENCY is not enabled optimised versions of block and unblock
 * interrupts are used.
 */
#ifndef RECORD_LATENCY
.MODULE $M.block_interrupts;
    .CODESEGMENT PM;
    .MINIM;
$_block_interrupts:
$_block_interrupts_before_sleep:
    r0 = 3;
    M[$INT_BLOCK_PRIORITY] = r0;
    r0 = MBU[$_yield_and_block_count];
    r0 = r0 + 1;
    MB[$_yield_and_block_count] = r0;
    rts;
.ENDMODULE;

.MODULE $M.unblock_interrupts;
    .CODESEGMENT PM;
    .MINIM;
$_unblock_interrupts:
$_unblock_interrupts_after_sleep:
    r0 = MBU[$_yield_and_block_count];
    r0 = r0 - 1;
    MB[$_yield_and_block_count] = r0;
    if NZ jump yield_or_still_blocked;
    /* Yield and counter are zero, unblock interrupts and return. */
    M[$INT_BLOCK_PRIORITY] = Null;
    rts;
yield_or_still_blocked:
    /* Yield may be set and/or interrupt blocking is nested. */
    Null = r0 - 0x80; /* The yield flag is in the top bit. */
    /* Return if we are nested, or not nested and yield is not set. */
    if NE rts;
    /* yield_and_block_count is equal to 0x80 so we need to yield. */
    MB[$_yield_and_block_count] = Null;
    /* vPortYieldAsm will re-enable interrupts. */
    jump $_vPortYieldAsm;
.ENDMODULE;
#endif /* !RECORD_LATENCY */

/**
 * CIE (Common Information Entry) for interrupt_handler.
 *
 * This is a minimal dwarf frame in that it only documents the push/pops
 * associated with registers needed for stack unwinding (FP, rINTLINK, rLINK,
 * rMAC). The effect of this is that it may require more effort to debug
 * the interrupted function in some cases (by looking on the stack manually).
 *
 * The stack is always unwound through the task that was interrupted, including
 * when the ISR's stack is switched to use the main stack and when the stack
 * is switched to another task on a context switch. This way a backtrace from
 * the ISR always shows the context that has been interrupted, which we
 * currently think is most useful.
 *
 * For more information on debug annotations refer to the dwarf specification:
 * http://dwarfstd.org/doc/Dwarf3.pdf
 *
 * dwarfdump and kobjdump -W are also useful for inspecting this information.
 */
    .section ".debug_frame"
.ISR_CIE_Pointer:
    .4byte  .ISR_CIE_End-.ISR_CIE_Start /* CIE length */
.ISR_CIE_Start:
    .4byte  0xFFFFFFFF /* CIE_id */
    .byte   0x01 /* CIE version */
    .string "" /* CIE augmentation */
    .byte   0x01 /* code alignment factor */
    .byte   0x04 /* data alignment factor */
    .byte   KalDwarfRegisterNum_RegrINTLINK /* return address register */
    /* The FP is not updated immediately on entry but we have started a new
       frame, use the SP until the FP is updated. */
    .byte   DW_CFA_def_cfa /* initial CFA (Canonical Frame Address) rule */
    .byte   KalDwarfRegisterNum_RegSP  /* operand 0: register number */
    .byte   0x00                       /* operand 1: non-factored offset */
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegSP
    /* KCC calling convention has rMAC, r0, r1, r2, r3, r10 as caller-saved
       regisers (aka temporary, volatile, call-clobbered). However, the ISR
       must preserve the values of all registers, so all registers are set
       to DW_CFA_same_value in the ISR's CIE. */
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegrMAC
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR0
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR1
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR2
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR3
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR4
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR5
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR6
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR7
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR8
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR9
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR10
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegrLINK
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegFP

    /* Align to 4 bytes (size of an address) */
    .byte   DW_CFA_nop
    .byte   DW_CFA_nop
.ISR_CIE_End:

/**
 * FDE (Frame Description Entry) for interrupt_handler.
 */
    .4byte  .ISR_FDE_End-.ISR_FDE_Start /* FDE length */
.ISR_FDE_Start:
    .4byte  .ISR_CIE_Pointer /* CIE_pointer */
    .4byte  .interrupt_handler_Start /* initial_location */
    .4byte  .interrupt_handler_End-.interrupt_handler_Start  /* address_range */

    /* interrupt_handler_Push1 */
    .byte   DW_CFA_advance_loc2
    .2byte  .interrupt_handler_Push1-.interrupt_handler_Start
    /* FP updated on this push, use this for the CFA. */
    .byte   DW_CFA_def_cfa
    .byte   KalDwarfRegisterNum_RegFP
    .byte   0x00
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegFP
    .byte   0x00
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegR0
    .byte   0x01
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegR1
    .byte   0x02
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegR2
    .byte   0x03
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegFlags
    .byte   0x04

    /* interrupt_handler_Push2 */
    .byte   DW_CFA_advance_loc2
    .2byte  .interrupt_handler_Push2-.interrupt_handler_Push1
    /* Some registers aren't required for call stack unwinding and don't have a
       dwarf register number. The comments are left here to help keep count of
       where we are on the stack relative to the CFA. */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegARITHMETIC_MODE */
    /*.byte   0x05 */
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegrINTLINK
    .byte   0x06
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegR3
    .byte   0x07
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegR10
    .byte   0x08
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegrLINK
    .byte   0x09
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegI3
    .byte   0x0a
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegI7
    .byte   0x0b
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegM0 */
    /*.byte   0x0c */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegM1 */
    /*.byte   0x0d */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegM2 */
    /*.byte   0x0e */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegM3 */
    /*.byte   0x0f */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegL0 */
    /*.byte   0x10 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegL1 */
    /*.byte   0x11 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegL4 */
    /*.byte   0x12 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegL5 */
    /*.byte   0x13 */
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegrMAC2
    .byte   0x14
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegrMAC1
    .byte   0x15
    .byte   DW_CFA_offset + KalDwarfRegisterNum_RegrMAC0
    .byte   0x16
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegDoLoopStart */
    /*.byte   0x17 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegDoLoopEnd */
    /*.byte   0x18 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegDivResult */
    /*.byte   0x19 */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegDivRemainder */
    /*.byte   0x1a */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegB0 */
    /*.byte   0x1b */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegB1 */
    /*.byte   0x1c */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegB4 */
    /*.byte   0x1d */
    /*.byte   DW_CFA_offset + KalDwarfRegisterNum_RegB5 */
    /*.byte   0x1e */

    /* interrupt_handler_Pop1 */
    .byte   DW_CFA_advance_loc2
    .2byte  .interrupt_handler_Pop1-.interrupt_handler_Push2
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegrMAC2
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegrMAC12
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegrMAC0
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegDoLoopStart */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegDoLoopEnd */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegDivResult */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegDivRemainder */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegB0 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegB1 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegB4 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegB5 */
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegI3
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegI7
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegM0 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegM1 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegM2 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegM3 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegL0 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegL1 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegL4 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegL5 */
    /*.byte   DW_CFA_restore + KalDwarfRegisterNum_RegARITHMETIC_MODE */
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegrINTLINK
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegR3
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegR10
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegrLINK
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegFP
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegR0
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegR1
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegR2
    .byte   DW_CFA_restore + KalDwarfRegisterNum_RegFlags
    .byte   DW_CFA_def_cfa /* CFA is now the SP, the FP has been restored to the previous frame. */
    .byte   KalDwarfRegisterNum_RegSP
    .byte   0x00

    /* Align to 4 bytes (size of an address) */
    .byte   DW_CFA_nop
    .byte   DW_CFA_nop
    .byte   DW_CFA_nop
.ISR_FDE_End:

/**
 * Generic CIE (Common Information Entry) for a function adhering to the KCC
 * calling convention
.*/
.Task_CIE_Pointer:
    .4byte  .Task_CIE_End-.Task_CIE_Start /*CIE len*/
.Task_CIE_Start:
    .4byte  0xFFFFFFFF /* CIE_id */
    .byte   0x01 /* CIE version */
    .string "" /* CIE augmentation */
    .byte   0x01 /* code alignment factor */
    .byte   0x04 /* data alignment factor */
    .byte   KalDwarfRegisterNum_RegrLINK /* ret addr rule */
    .byte   DW_CFA_def_cfa /* initial CFA rule */
    .byte   KalDwarfRegisterNum_RegSP
    .byte   0x00

    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegSP
    .byte   DW_CFA_undefined
    .byte   KalDwarfRegisterNum_RegrMAC
    .byte   DW_CFA_undefined
    .byte   KalDwarfRegisterNum_RegR0
    .byte   DW_CFA_undefined
    .byte   KalDwarfRegisterNum_RegR1
    .byte   DW_CFA_undefined
    .byte   KalDwarfRegisterNum_RegR2
    .byte   DW_CFA_undefined
    .byte   KalDwarfRegisterNum_RegR3
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR4
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR5
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR6
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR7
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR8
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR9
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegR10
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegrLINK
    .byte   DW_CFA_same_value
    .byte   KalDwarfRegisterNum_RegFP

    /* Align to 4 bytes (size of an address) */
    .byte   DW_CFA_nop
    .byte   DW_CFA_nop
.Task_CIE_End:

/**
 * FDE (Frame Description Entry) for enter_shallow_sleep.
 *
 * This is a leaf function so no pushes / pops need to be annotated.
 */
    .4byte .enter_shallow_sleep_FDE1_End-.enter_shallow_sleep_FDE1_Start /*FDE len*/
.enter_shallow_sleep_FDE1_Start:
    .4byte .Task_CIE_Pointer /* declare the common info */
    .4byte .enter_shallow_sleep_Start /* initial_location */
    .4byte .enter_shallow_sleep_End-.enter_shallow_sleep_Start /* address_range */
.enter_shallow_sleep_FDE1_End:

/**
 * FDE (Frame Description Entry) for vPortYieldAsm.
 */
    .4byte .vPortYieldAsm_FDE1_End-.vPortYieldAsm_FDE1_Start /*FDE len*/
.vPortYieldAsm_FDE1_Start:
    .4byte .Task_CIE_Pointer /* declare the common info */
    .4byte .vPortYieldAsm_Start /* initial_location */
    .4byte .vPortYieldAsm_End-.vPortYieldAsm_Start /* address_range */

    .byte  DW_CFA_advance_loc2 /* CFA changes after the first pushm (has 1 arg)*/
    .2byte .vPortYieldAsm_PUSH_FP-.vPortYieldAsm_Start
    .byte  DW_CFA_def_cfa /* this frame's CFA now=FP (2 args)*/
    .byte  KalDwarfRegisterNum_RegFP
    .byte  0x00
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegFP /* caller's FP */
    .byte  0x00 /* now stacked @CFA+0 */

    .byte  DW_CFA_advance_loc2 /* rLink is pushed here, record this */
    .2byte .vPortYieldAsm_PUSH_rLink-.vPortYieldAsm_PUSH_FP
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegrLINK
     /* caller's rLINK now stacked @CFA+9 */
    .byte  0x09

    .byte  DW_CFA_advance_loc2 /* r4-r9 are pushed here */
    .2byte .vPortYieldAsm_PUSH_r4_r9-.vPortYieldAsm_PUSH_rLink
    /* set them up as being offsets to CFA */
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR4
    .byte  0x1f
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR5
    .byte  0x20
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR6
    .byte  0x21
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR7
    .byte  0x22
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR8
    .byte  0x23
    .byte  DW_CFA_offset|KalDwarfRegisterNum_RegR9
    .byte  0x24
.vPortYieldAsm_FDE1_End:
