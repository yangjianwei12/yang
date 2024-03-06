/****************************************************************************
 * Copyright (c) 2019 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 *
 * \file  anc_compander_proc.h
 * \ingroup anc_compander
 *
 * ANC Compander operator shared definitions and include files
 *
 */

#ifndef _ANC_COMPANDER_PROC_H_
#define _ANC_COMPANDER_PROC_H_

#include "anc_compander_cap.h"
#include "anc_compander_defs.h"
#include "compander_c.h"

/* Shift value for converting makeup gain to the gain smooth history format.
 * Gain smooth history is Q8.24, makeup gain is Q2.30 so right shift by 6.
 */
#define ADRC_MAKEUP_TO_GAIN_HIST_SHIFT  6

/****************************************************************************
ASM function definitions
*/

/**
 * \brief  Initialize the ANC Compander data object.
 *
 * \param  compander_obj  Pointer to the Compander object parameters.
 *
 * \return  void
 */
extern void anc_compander_initialize(t_compander_object *compander_obj);


#endif /* _ANC_COMPANDER_PROC_H_ */