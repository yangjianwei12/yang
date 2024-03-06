/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    usb_hid_class
    \brief      USB HID Datalink default descriptors
*/

#include "usb_hid_datalink_descriptors.h"

static const uint8 report_descriptor_hid_datalink[] =
{
    USB_HID_DATALINK_REPORT_DESC_COMMON,
    USB_HID_DATALINK_REPORT_SET_COMMAND_DESC
};

/* See the USB HID 1.11 spec section 6.2.1 for description */
static const uint8 interface_descriptor_hid_datalink[] =
{
    HID_DESCRIPTOR_LENGTH,                  /* bLength */
    B_DESCRIPTOR_TYPE_HID,                  /* bDescriptorType */
    0x11, 0x01,                             /* HID class release number (1.00).
                                             * The 1st and the 2nd byte denotes
                                             * the minor & major Nos respectively
                                             */
    0x00,                                   /* Country code (None) */
    0x01,                                   /* Only one class descriptor to follow */
    B_DESCRIPTOR_TYPE_HID_REPORT,           /* Class descriptor type (HID Report) */
    sizeof(report_descriptor_hid_datalink), /* Report descriptor length. LSB first */
    0x00                                    /* followed by MSB */
};

const usb_hid_class_desc_t usb_hid_datalink_class_desc = {
        .descriptor = interface_descriptor_hid_datalink,
        .size_descriptor = sizeof(interface_descriptor_hid_datalink)
};

const usb_hid_report_desc_t usb_hid_datalink_report_desc = {
        .descriptor = report_descriptor_hid_datalink,
        .size_descriptor = sizeof(report_descriptor_hid_datalink)
};

const usb_hid_endpoint_desc_t usb_hid_datalink_endpoints[USB_HID_NUM_OF_DATALINK_END_POINTS] = {
        {
            .is_to_host = FALSE,
            .wMaxPacketSize = 64,
            .bInterval = 1
        },
        {
            .is_to_host = TRUE,
            .wMaxPacketSize = 64,
            .bInterval = 1
        }
};

const usb_hid_config_params_t usb_hid_datalink_config = {
        .class_desc = &usb_hid_datalink_class_desc,
        .report_desc = &usb_hid_datalink_report_desc,
        .endpoints = usb_hid_datalink_endpoints,
        .num_endpoints = ARRAY_DIM(usb_hid_datalink_endpoints)
};
