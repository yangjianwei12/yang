/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_perfstats.h
 * \ingroup HAL
 *
 * Public header file for HAL performance stats functions
 */
#ifndef _HAL_PERFSTATS_H_
#define _HAL_PERFSTATS_H_

/**
 * \brief  Get processor clock cycles
 */
extern uint32 hal_get_runclks(void);

/**
 * \brief  Get processor instructions
 */
extern uint32 hal_get_instrs(void);

#endif /* _HAL_PERFSTATS_H_ */
