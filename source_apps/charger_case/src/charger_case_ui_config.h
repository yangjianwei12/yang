/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       charger_case_ui_config.h
\brief      Application specific ui configuration
*/

#ifndef CHARGER_CASE_UI_CONFIG_H_
#define CHARGER_CASE_UI_CONFIG_H_

#include "ui.h"

/*! \brief Return the ui configuration table for the charger_case application.

    The configuration table can be passed directly to the ui component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific ui configuration table.
*/
const ui_config_table_content_t* ChargerCaseUi_GetConfigTable(unsigned* table_length);

#endif /* CHARGER_CASE_UI_CONFIG_H_ */

