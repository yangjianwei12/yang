/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  hal_emulator.h
 * \ingroup HAL
 *
 * Helper functions to deal with the pecularities of running on an emulator.
 */

#ifndef HAL_EMULATOR_H
#define HAL_EMULATOR_H

#include "types.h"

typedef enum
{
    HAL_EMULATOR_INTERFACE_ANALOG = 0,
    HAL_EMULATOR_INTERFACE_PCM = 1
} HAL_EMULATOR_INTERFACE;

#if !defined(RUNNING_ON_KALSIM)
/**
 * \brief Detects if the firmware is running on an emulator.
 *
 * \return TRUE if running on an emulator
 */
extern bool hal_emulator_is_detected(void);

/**
 * \brief Configure registers specific to the emulator needed
 *        to make the audio extension board functional.
 */
extern void hal_emulator_initialize_audio_board(void);

/**
 * \brief Configure the emulator's audio board to either route
 *        PCM signals on a pin header or route analogue signals
 *        to and from 3.5mm audio jacks.
 *
 * \param mode Analogue or PCM
 */
extern void hal_emulator_configure_audio_board(HAL_EMULATOR_INTERFACE mode);
#else
#define hal_emulator_is_detected() FALSE
#define hal_emulator_initialize_audio_board()
#define hal_emulator_configure_audio_board(x)
#endif /* !defined(RUNNING_ON_KALSIM) */

#endif /* HAL_EMULATOR_H */