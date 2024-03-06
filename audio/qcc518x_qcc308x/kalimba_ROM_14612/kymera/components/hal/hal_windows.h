/****************************************************************************
 * Copyright 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_windows.h
 * \ingroup HAL
 *
 * Header for functions to handle memory map access windows.
 */

#ifndef HAL_WINDOWS_H
#define HAL_WINDOWS_H

/* There are 8 NVMEM windows available in total.
   The first 4 are only usable via the data memory space while
   the remaining are usable via both the data memory space and the
   program memory space.
   Currently only 4 windows are used by the firmware. */
#define HAL_WINDOWS_FIRMWARE_INDEX         0
#define HAL_WINDOWS_CAP_FROM_SQIF_INDEX    1
#define HAL_WINDOWS_DATA_MEMORY_INDEX      2
#define HAL_WINDOWS_SRAM_READ_INDEX        3
#define HAL_WINDOWS_SRAM_WRITE_INDEX       4

/**
 * Enumeration of the possible windows in the subsystem.
 * Each window can be mapped in Data Memory space, Program Memory space
 * or both. The windows are identified by where they point to.
 */
typedef enum
{
    HAL_WINDOWS_FIRMWARE,       /*!< Points either to ROM or to audio SQIF. */
#if defined(RUN_CAPABILITIES_FROM_SQIF)
    HAL_WINDOWS_CAP_FROM_SQIF,  /*!< Points to audio SQIF_FLASH. */
#endif
#if defined(INSTALL_EXTERNAL_MEM)
    HAL_WINDOWS_SRAM_READ,      /*!< Points to external SQIF SRAM in data space. */
    HAL_WINDOWS_SRAM_WRITE,     /*!< Points to external SQIF SRAM in data space. */
#endif
#if defined(CHIP_HAS_NVRAM_ACCESS_TO_DM)
    HAL_WINDOWS_DATA_MEMORY,    /*!< Points to data memory and is mapped in
                                     program space only. */
#endif
    HAL_WINDOWS_PROGRAM_MEMORY  /*!< Points to program memory and is mapped in
                                     data space only. */
} HAL_WINDOWS;

/**
 * \brief Open a window in Data and Program spaces.
 *
 * \param window Indicate where the window open to.
 */
extern void hal_window_open(HAL_WINDOWS window);

/**
 * \brief Close a window in Data and Program spaces.
 *
 * \param window Indicate which window to close.
 */
extern void hal_window_close(HAL_WINDOWS window);

#endif /* HAL_WINDOWS_H */

