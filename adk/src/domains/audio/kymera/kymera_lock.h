/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera private header with lock related definitions
*/

#ifndef KYMERA_LOCK_H_
#define KYMERA_LOCK_H_

/*!@{ \name Macros to set and clear bits in the lock. */
#define appKymeraSetToneLock(theKymera) (theKymera)->lock |= (1U << 0)
#define appKymeraClearToneLock(theKymera) (theKymera)->lock &= ~(1U << 0)
#define appKymeraSetA2dpStartingLock(theKymera) (theKymera)->lock |= (1U << 1)
#define appKymeraClearA2dpStartingLock(theKymera) (theKymera)->lock &= ~(1U << 1)
#define appKymeraSetScoStartingLock(theKymera) (theKymera)->lock |= (1U << 2)
#define appKymeraClearScoStartingLock(theKymera) (theKymera)->lock &= ~(1U << 2)
#define appKymeraSetLeStartingLock(theKymera) (theKymera)->lock |= (1U << 3)
#define appKymeraClearLeStartingLock(theKymera) (theKymera)->lock &= ~(1U << 3)
#define appKymeraSetAncStartingLock(theKymera) (theKymera)->lock |= (1U << 4)
#define appKymeraClearAncStartingLock(theKymera) (theKymera)->lock &= ~(1U << 4)
#define appKymeraSetAdaptiveAncStartingLock(theKymera) (theKymera)->lock |= (1U << 5)
#define appKymeraClearAdaptiveAncStartingLock(theKymera) (theKymera)->lock &= ~(1U << 5)
#define appKymeraSetGamingBackgroundStartingLock(theKymera) (theKymera)->lock |= (1U << 6)
#define appKymeraClearGamingBackgroundStartingLock(theKymera) (theKymera)->lock &= ~(1U << 6)
/*!@}*/

#endif /* KYMERA_LOCK_H_ */
