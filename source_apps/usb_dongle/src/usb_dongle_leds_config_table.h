/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_leds_config_table.h
\brief      Header file for the USB dongle led config table.
*/

#ifndef USB_DONGLE_LEDS_CONFIG_TABLE_H
#define USB_DONGLE_LEDS_CONFIG_TABLE_H

#include <ui_indicator_leds.h>

extern const ui_event_indicator_table_t usb_dongle_ui_leds_table[];
extern const ui_provider_context_consumer_indicator_table_t usb_dongle_ui_leds_context_indications_table[];
uint8 UsbDongleLedsConfigTable_GetSize(void);
uint8 UsbDongleLedsConfigTable_ContextsTableGetSize(void);
#endif // USB_DONGLE_LEDS_CONFIG_TABLE_H
