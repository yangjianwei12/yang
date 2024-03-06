/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   license_override License Override
\ingroup    domains
\brief      Definition of API to override licenses for certain platforms
            This can be used to override licenses via Upgrades for particular address ranges
*/

/*! @{ */

#ifndef LICENSE_OVERRIDE_H
#define LICENSE_OVERRIDE_H

#include "license_override_mapping.h"

/*! \brief Override the default license for this build if a mapping has been defined. */
void LicenseOverride_Override(feature_license_key_override_mapping_t * mapping, uint8 mapping_size);

#endif /* LICENSE_OVERRIDE_H */
/*! @} */
