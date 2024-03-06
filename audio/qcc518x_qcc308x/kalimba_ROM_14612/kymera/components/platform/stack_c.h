/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file stack_c.h
 * \ingroup platform
 *
 * Function to manipulate the stack during boot.
 */

#ifndef STACK_C_H
#define STACK_C_H

/**
 * \brief Switch the stack of the current processor to a supplied buffer
 *        allocated on the heap.
 *
 * \param ptr  Pointer to the start of a buffer to use as the new stack.
 * \param size Number of octets of the buffer to use as the new stack.
 */
extern void set_stack_regs(void *ptr, unsigned size);

#endif /* STACK_C_H */