/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup ui_indicator
    \brief      Private messages and symbols used by the UI Tones module.
    @{
*/
#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <csrtypes.h>

#define UI_KYMERA_RESOURCE_LOCKED 1U

extern uint16 ui_kymera_lock;

/*! \brief Get the address of the kymera resource lock */
static inline uint16 *ui_GetKymeraResourceLockAddress(void)
{
    return &ui_kymera_lock;
}

/*! \brief Query if the kymera resource is locked */
static inline bool ui_IsKymeraResourceLocked(void)
{
    return (ui_kymera_lock == UI_KYMERA_RESOURCE_LOCKED);
}

/*! \brief Set the kymera resource lock */
static inline void ui_SetKymeraResourceLock(void)
{
    ui_kymera_lock = UI_KYMERA_RESOURCE_LOCKED;
}

/*! \brief Clear the kymera resource lock */
static inline void ui_ClearKymeraResourceLock(void)
{
    ui_kymera_lock = 0;
}

#endif // UI_PRIVATE_H

/*! @} */