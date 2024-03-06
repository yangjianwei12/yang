/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_dongle_ui_config.h
\brief      Application specific ui configuration
*/

#ifndef USB_DONGLE_UI_CONFIG_H_
#define USB_DONGLE_UI_CONFIG_H_

#include "ui.h"

/*! \brief Return the ui configuration table for the usb_dongle application.

    The configuration table can be passed directly to the ui component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific ui configuration table.
*/
const ui_config_table_content_t* UsbDongleUi_GetConfigTable(unsigned* table_length);

/*! \brief Configures the Focus Select module in the framework with the
    source prioritisation for the usb_dongle Application.
*/
void UsbDongleUi_ConfigureFocusSelection(void);

#endif /* USB_DONGLE_UI_CONFIG_H_ */

