/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       usb_dongle_ui.c
\brief      UsbDongle application User Interface Indications
*/

#include "usb_dongle_ui.h"
#include "ui.h"
#include "led_manager.h"
#include "usb_dongle_leds_config_table.h"
#include <ui_indicator_leds.h>
#include "input_event_manager.h"

bool UsbDongleUi_Init(Task init_task)
{
    UNUSED(init_task);

    UiLeds_SetLedConfiguration(
                usb_dongle_ui_leds_table,
                UsbDongleLedsConfigTable_GetSize(),
                usb_dongle_ui_leds_context_indications_table,
                UsbDongleLedsConfigTable_ContextsTableGetSize());

    return TRUE;
}
