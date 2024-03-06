/*!
   \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \defgroup   tmap_profile   LE TMAP
   @{
   \ingroup    profiles
   \brief      Header file for TMAP profile
*/

#ifndef TMAP_PROFILE_H_
#define TMAP_PROFILE_H_

#include <logging.h>

#include <panic.h>
#include <stdlib.h>

/*! \brief Initialize the TMAP profile. This function initializes the gatt tmas server.
 */
void TmapProfile_Init(void);

#endif /* TMAP_PROFILE_H_ */
/*! @} */