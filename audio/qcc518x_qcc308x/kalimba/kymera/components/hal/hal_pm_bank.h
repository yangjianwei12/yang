/****************************************************************************
 * Copyright 2019 -2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_pm_bank.h
 * \ingroup HAL
 *
 * Set of functions shared between chips to program PM banks.
 */

#ifndef HAL_PM_BANK_H
#define HAL_PM_BANK_H

#include "types.h"

/****************************************************************************
Public Constant and macros
*/
#ifdef CHIP_CRESCENDO
#define HAL_MAX_CACHE_BANKS            3
#ifdef INSTALL_2WAY_CACHE
#define HAL_NUM_CACHE_BANKS            3
#elif defined(INSTALL_CACHE)
#define HAL_NUM_CACHE_BANKS            1
#else
#define HAL_NUM_CACHE_BANKS            0
#endif
#else
#define HAL_MAX_CACHE_BANKS            5
#if defined (INSTALL_2WAY_CACHE) || defined (INSTALL_SWITCHABLE_CACHE)
#define HAL_NUM_CACHE_BANKS            5
#elif defined(INSTALL_2WAYHALF_CACHE)
#define HAL_NUM_CACHE_BANKS            3
#elif defined(INSTALL_CACHE)
#define HAL_NUM_CACHE_BANKS            1
#else
#define HAL_NUM_CACHE_BANKS            0
#endif
#endif

#ifndef PM_RAM_SIZE_WORDS
#error "PM_RAM_SIZE_WORDS needs to be defined for the chip"
#endif

#ifndef NUMBER_PM_BANKS
#error "NUMBER_PM_BANKS needs to be defined for the chip"
#endif

#define PM_BANK_SIZE ((PM_RAM_SIZE_WORDS*PC_PER_INSTRUCTION)/NUMBER_PM_BANKS)


/****************************************************************************
Public types
*/

typedef enum
{
    HAL_PM_BANK_FIRST = 0,
    HAL_PM_BANK_LAST  = NUMBER_PM_BANKS - 1,
    HAL_PM_BANK_NUMBER = NUMBER_PM_BANKS
} HAL_PM_BANK;

typedef enum
{
    HAL_PM_OWNER_PROCESSOR_0 = 0,
    HAL_PM_OWNER_PROCESSOR_1 = 1,
    HAL_PM_OWNER_CACHE       = 2,
    HAL_PM_OWNER_INVALID     = 0xffff,
} HAL_PM_OWNER;

/****************************************************************************
Public function definitions
*/

#ifdef __KCC__
/**
 * \brief Initialise of PM RAM access
 */
extern void hal_pm_initialize(void);
#else
#define hal_pm_initialize()
#endif

/**
 * \brief Configure a bank's arbiter
 *
 * \param bank  The bank number.
 * \param owner Which IP block can access it.
 */
extern void hal_pm_configure_arbiter(HAL_PM_BANK bank, HAL_PM_OWNER owner);

#if defined(PM_BANKS_CAN_BE_POWERED_OFF)
/**
 * \brief Update PM banks power state.
 *
 * Scans through the banks permissions and then power off
 * the banks that cannot be written to and power on the banks
 * that can.
 */
extern void hal_pm_update_banks_power(void);

/**
 * \brief Returns the number of banks that always have to be powered off.
 */
extern uint16 hal_pm_get_min_powered_off_banks(void);
#else
#define hal_pm_update_banks_power()
#define hal_pm_get_min_powered_off_banks() 0
#endif 

#endif /* HAL_PM_BANK_H */
