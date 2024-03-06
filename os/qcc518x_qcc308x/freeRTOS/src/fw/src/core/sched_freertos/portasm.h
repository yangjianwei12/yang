/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/* %%version */

/**
 * \file
 *
 * Declarations for the assembly functions defined in portasm.asm.
 */

#ifndef PORTASM_H
#define PORTASM_H

/**
 * \brief Start the first task.
 */
void vPortStartFirstTaskAsm(void);

/**
 * \brief Yield to another task of equal priority.
 */
void vPortYieldAsm(void);

/**
 * \brief Enter shallow sleep mode until an interrupt occurs.
 */
void enter_shallow_sleep(void);

#ifdef FREERTOS_STANDALONE_BUILD
/**
 * \brief The interrupt handler entrypoint.
 *
 * Required for the standalone port only, Hydra declares this in int_private.h.
 */
void interrupt_handler(void);
#endif /* FREERTOS_STANDALONE_BUILD */

#endif /* !PORTASM_H */
