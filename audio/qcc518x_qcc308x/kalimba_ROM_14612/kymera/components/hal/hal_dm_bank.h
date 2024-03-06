/****************************************************************************
 * Copyright 2019 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_dm_bank.h
 * \ingroup HAL
 *
 * Header for functions to configure the chip Data Memory banks.
 */

#ifndef HAL_DM_BANK_H
#define HAL_DM_BANK_H

#include "proc/proc.h"

/****************************************************************************
Public types
*/

typedef enum
{
    HAL_DM_BUS_DM1 = 0,
    HAL_DM_BUS_DM2 = 1,
} HAL_DM_BUS;

typedef enum
{
    HAL_DM_BANK_FIRST  = 0,
    HAL_DM_BANK_LAST   = NUMBER_DM_BANKS - 1,
    HAL_DM_BANK_NUMBER = NUMBER_DM_BANKS
} HAL_DM_BANK;

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Initialise of DM RAM access
 */
extern void hal_dm_initialize(void);

/**
 * \brief Configure a bank's arbiter
 *
 * \param bank The bank number.
 * \param core Number of the processor meant to own the bank.
 * \param bus  Which of DM1 or DM2 should access the bank.
 * \param allow_other_cores Whether the other (non-owner) core(s)
 *             has write access to this bank
 */
extern void hal_dm_configure_arbiter(HAL_DM_BANK bank,
                                     PROC_ID_NUM core,
                                     HAL_DM_BUS bus,
                                     bool allow_other_cores);

/**
 * \brief Get the maximum number of data memory banks usable on the chip.
 *
 * This number can be further reduced by the system to save power.
 *
 * \return A number inferior or equal to HAL_DM_BANK_NUMBER.
 */
extern HAL_DM_BANK hal_dm_get_number_banks(void);

#if defined(DM_BANKS_CAN_BE_POWERED_OFF)
/**
 * \brief Update DM banks power state.
 *
 * \param start Index of the first bank to be updated.
 * \param end   Index of the last bank to be updated.
 *
 * Scans through the banks permissions and then power off
 * the banks that cannot be written to and power on the banks
 * that can.
 */
extern void hal_dm_update_banks_power(HAL_DM_BANK start, HAL_DM_BANK end);
#else
#define hal_dm_update_banks_power(x, y)
#endif 

#endif /* HAL_DM_BANK_H */
