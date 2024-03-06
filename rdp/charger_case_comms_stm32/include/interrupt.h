/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interrupts
*/

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <stdint.h>

/*-----------------------------------------------------------------------------
------------------ PROTOTYPES -------------------------------------------------
-----------------------------------------------------------------------------*/

void interrupt_init(void);
void EXTI_line_enable(uint16_t pin);
void EXTI_line_disable(uint16_t pin);

#endif /* INTERRUPT_H_ */
