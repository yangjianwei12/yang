/*!
   \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup usb_hid_class
   \brief      Header file for using default descriptors of USB HID Consumer Transport
   @{
*/

#ifndef USB_HID_CONSUMER_TRANSPORT_DESCRIPTORS_H
#define USB_HID_CONSUMER_TRANSPORT_DESCRIPTORS_H

#include "usb_hid_class.h"

/*! Default USB HID report IDs */
#define USB_HID_CONSUMER_TRANSPORT_REPORT_ID    0x01
#define USB_HID_TELEPHONY_REPORT_ID             0x02
#define USB_HID_TELEPHONY_PHONE_MUTE_USAGE_ID   0x2f
#define USB_HID_TELEPHONY_HOOK_USAGE_ID         0x20
#define USB_HID_TELEPHONY_PGMBLE_BTN_USAGE_ID   0x07
#define USB_HID_LED_MUTE_REPORT_ID              0x09
#define USB_HID_LED_OFF_HOOK_REPORT_ID          0x17
#define USB_HID_LED_RING_REPORT_ID              0x18
#define USB_HID_LED_HOLD_REPORT_ID              0x20


/*! Default USB HID consumer transport class descriptor */
extern const usb_hid_class_desc_t usb_hid_consumer_transport_class_desc;
/*! Default USB HID consumer transport report descriptor */
extern const usb_hid_report_desc_t usb_hid_consumer_transport_report_desc;
/*! Default USB HID consumer transport endpoint config */
extern const usb_hid_endpoint_desc_t usb_hid_consumer_transport_endpoint;
/*! Default USB HID consumer transport configuration */
extern const usb_hid_config_params_t usb_hid_consumer_transport_config;

#endif /* USB_HID_CONSUMER_TRANSPORT_DESCRIPTORS_H */

/*! @} */