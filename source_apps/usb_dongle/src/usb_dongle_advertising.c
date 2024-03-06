
/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Module for setting up BREDR page scanning
*/

#include "usb_dongle_advertising.h"

/* framework includes */
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <local_addr.h>
/* system includes */
#include <connection.h>
#include <macros.h>
#include <panic.h>
#include <rtime.h>
#include <uuid.h>

/* Advertising parameters to pass to LE/BREDR scan managers */

static const bredr_scan_manager_scan_parameters_set_t inquiry_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = US_TO_BT_SLOTS(2560000), .window = US_TO_BT_SLOTS(11250) },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = US_TO_BT_SLOTS(320000),  .window = US_TO_BT_SLOTS(11250) },
        },
    },
};

static const bredr_scan_manager_scan_parameters_set_t page_scan_params_set[] =
{
    {
        {
            [SCAN_MAN_PARAMS_TYPE_SLOW] = { .interval = US_TO_BT_SLOTS(1280000), .window = US_TO_BT_SLOTS(22500) },
            [SCAN_MAN_PARAMS_TYPE_FAST] = { .interval = US_TO_BT_SLOTS(100000),  .window = US_TO_BT_SLOTS(11250) },
        },
    },
};

const bredr_scan_manager_parameters_t inquiry_scan_params =
{
    inquiry_scan_params_set, ARRAY_DIM(inquiry_scan_params_set)
};

static const bredr_scan_manager_parameters_t page_scan_params =
{
    page_scan_params_set, ARRAY_DIM(page_scan_params_set)
};

/* Public functions */

bool UsbDongleAdvertising_Init(Task init_task)
{
    UNUSED(init_task);

    /* Set scan parameters for BREDR & LE advertising */
    BredrScanManager_PageScanParametersRegister(&page_scan_params);
    BredrScanManager_InquiryScanParametersRegister(&inquiry_scan_params);

    /* Allow LE advertising & connections */
    ConManagerAllowHandsetConnect(TRUE);
    ConManagerAllowConnection(cm_transport_bredr, TRUE);

    return TRUE;
}
