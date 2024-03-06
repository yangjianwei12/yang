/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       motion.h
    \defgroup   motion Motion
    @{
        \ingroup    sensor_domain
        \brief      Header file for motion API
*/
#ifndef MOTION_H
#define MOTION_H

#include <message.h>


/*! \brief Sample and return whether or not we're in motion

    \return TRUE if in motion, otherwise FALSE
 */
#ifdef INCLUDE_MOTION
bool Motion_IsInMotion(void);
#else
#define Motion_IsInMotion() FALSE
#endif  /* INCLUDE_MOTION */

#endif /* MOTION_H */

/*! @} */