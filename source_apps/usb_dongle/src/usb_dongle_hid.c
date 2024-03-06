/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      USB dongle HID interface

*/

#include "usb_dongle_hid.h"
#include "usb_hid_datalink.h"
#include "usb_dongle_logging.h"
#include "ui.h"

#include "usb_dongle_config.h"

#ifdef INCLUDE_SOURCE_APP_LE_AUDIO
#include "usb_dongle_lea.h"
#include "usb_dongle_lea_config.h"
#include "le_audio_client.h"
#endif /* INCLUDE_SOURCE_APP_LE_AUDIO */

/*! \brief Configure command header data length */
#define USB_DONGLE_REPORT_DATA_HEADER_LEN   (3)

/*! \brief Length of data for USB_HID_CMD_SET_MODE command */
#define USB_HID_CMD_SET_MODE_DATA_LEN       (2)

/*! \brief USB HID Command types */
typedef enum
{
    /*! Command for setting the dongle mode */
    USB_HID_CMD_SET_MODE,

    /*! Command for setting broadcast source name */
    USB_HID_CMD_SET_LEA_BROADCAST_NAME,

    /*! Command for setting broadcast code */
    USB_HID_CMD_SET_LEA_BROADCAST_CODE,

    /*! Command for setting broadcast advertisement settings */
    USB_HID_CMD_SET_LEA_BROADCAST_ADV,

    /*! Command for setting BAP audio config  for broadcast */
    USB_HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG,

    /*! Command for setting Broadcast ID */
    USB_HID_CMD_SET_LEA_BROADCAST_ID,

} usb_hid_command_t;

typedef struct
{
    /*! Report ID of the received HID report */
    uint8 report_id;

    /*! Configure command. See usb_hid_command_t */
    uint8 cmd;

    /*! Length of the command data */
    uint8 len;
} usb_lea_hid_command_data_t;

static void usbDongleHid_HandleHidReports(uint8 report_id, const uint8 *data, uint16 size)
{
    const usb_lea_hid_command_data_t *hid_hdr = (const usb_lea_hid_command_data_t *)data;
    /* Command data is after the first three bytes (ie, report id, cmd and len)  */
    const uint8 *hid_data = &data[USB_DONGLE_REPORT_DATA_HEADER_LEN];

    if (report_id != HID_REPORTID_CONFIGURE_CMD ||
        data == NULL || hid_hdr->len > size - USB_DONGLE_REPORT_DATA_HEADER_LEN)
    {
        /* Ignore if the data received is not valid or report ID does not match */
        return;
    }

    switch (hid_hdr->cmd)
    {
#if defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)
        case USB_HID_CMD_SET_LEA_BROADCAST_NAME:
        {
            DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set bcast name");

            UsbDongle_LeaConfigSetLeaBroadcastSourceName((const char*)hid_data, hid_hdr->len);
        }
        break;

        case USB_HID_CMD_SET_LEA_BROADCAST_CODE:
        {
            DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set bacast code");

            UsbDongle_LeaConfigSetLeaBroadcastCode(hid_data, hid_hdr->len);
        }
        break;

        case USB_HID_CMD_SET_LEA_BROADCAST_ADV:
        {
            DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set broadcast advertisement settings");

            UsbDongle_LeaConfigSetLeaBroadcastAdvSettings(hid_data, hid_hdr->len);
        }
        break;

        case USB_HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG:
        {
            DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set broadcast BAP audio config");

            UsbDongle_LeaConfigSetBroadcastAudioConfig(hid_data, hid_hdr->len);
        }
        break;

        case USB_HID_CMD_SET_LEA_BROADCAST_ID:
        {
            DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set broadcast ID");

            UsbDongle_LeaConfigSetLeaBroadcastID(hid_data, hid_hdr->len);
        }
        break;

#endif /* defined(INCLUDE_SOURCE_APP_LE_AUDIO) && defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

        case USB_HID_CMD_SET_MODE:
        {
            if (hid_hdr->len == USB_HID_CMD_SET_MODE_DATA_LEN &&
                UsbDongleConfig_SetNewMode(hid_data[0], hid_data[1]))
            {
                DEBUG_LOG_FN_ENTRY("usbDongleHid_HandleHidReports set new mode %d", hid_data[0]);

                Ui_InjectUiInput(ui_input_set_dongle_mode);
            }
        }
        break;

        default:
            DEBUG_LOG_ALWAYS("usbDongleHid_HandleHidReports unexpected command 0x%x", hid_hdr->cmd);
            break;
    }
}


bool UsbDongleHid_Init(Task task)
{
    DEBUG_LOG_FN_ENTRY("UsbDongleHid_Init");

    /* Register to receive report data from host via USB HID Datalink class */
    UsbHid_Datalink_RegisterHandler(usbDongleHid_HandleHidReports);

    UNUSED(task);

    return TRUE;
}
