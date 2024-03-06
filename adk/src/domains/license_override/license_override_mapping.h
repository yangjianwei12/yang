/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup license_override
\brief      Access to the license override mapping.
@{
*/

#ifndef LICENSE_OVERRIDE_MAPPING_H
#define LICENSE_OVERRIDE_MAPPING_H

#define FEATURE_LICENSE_KEY_SIZE 64

/*! A mapping of a Bluetooth address range to license key value. */
typedef struct
{
    /*! The minimum Bluetooth address for which the license key override will be performed. */
    bdaddr min_addr;
    /*! The maximum Bluetooth address for which the license key override will be performed. */
    bdaddr max_addr;
    /*! The feature license key to override with. */
    uint8 license_key_override[FEATURE_LICENSE_KEY_SIZE];
} feature_license_key_override_mapping_t;

extern feature_license_key_override_mapping_t feature_license_key_override_mapping[];
extern uint8 feature_license_key_override_mapping_size;

#endif /* LICENSE_OVERRIDE_MAPPING_H */
/*! @} */
