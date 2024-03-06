/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lis25ba.h
\brief      The header file defines the API to initialize the PCM and I2C interface of LIS25BA accelerometer.
*/

#ifndef _LIS25BA_H_
#define _LIS25BA_H_

#ifdef INCLUDE_LIS25BA_ACCELEROMETER

#include <library.h>
#include <stream.h>
#include <audio_pcm_common.h>

const pcm_config_t *Peripherals_Lis25baGetPcmInterfaceSettings(void);
void Peripherals_Lis25baInitializeI2cInterface(void);
void Peripherals_Lis25baEnableDevice(void);
void Peripherals_Lis25baDisableDevice(void);

#endif /* INCLUDE_LIS25BA_ACCELEROMETER */

#endif /* _LIS25BA_H_ */
