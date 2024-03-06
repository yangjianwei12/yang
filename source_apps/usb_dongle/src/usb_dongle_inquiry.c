/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation for setting up Inquiry Manager
*/

#include "usb_dongle_inquiry.h"

#ifdef INCLUDE_SOURCE_APP_BREDR_AUDIO
#include "usb_dongle_config.h"
#include <inquiry_manager.h>
#include <bt_device_class.h>
#include <macros.h>

static const inquiry_manager_scan_parameters_t inquiry_params_set[]=
{
    {
        .max_responses = APP_CONFIG_INQUIRY_MAX_RESPONSES,
        .timeout = APP_CONFIG_INQUIRY_TIMEOUT,
        .class_of_device = APP_CONFIG_INQUIRY_COD_FILTER
    },
};

bool UsbDongleInquiry_Init(Task init_task)
{
    UNUSED(init_task);

    /* Set scan parameters for BREDR & LE advertising */
    InquiryManager_RegisterParameters(inquiry_params_set, ARRAY_DIM(inquiry_params_set));

    return TRUE;
}

#endif /* INCLUDE_SOURCE_APP_BREDR_AUDIO */

