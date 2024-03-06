/*!
\copyright  Copyright (c) 2008 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       earbud_ui_config.h
\brief      Application specific ui configuration
*/

#ifndef EARBUD_UI_CONFIG_H_
#define EARBUD_UI_CONFIG_H_

#include "ui.h"

#include "ui_user_config.h"

#include <touch.h>
/*! \brief Return the ui configuration table for the earbud application.

    The configuration table can be passed directly to the ui component in
    domains.

    \param table_length - used to return the number of rows in the config table.

    \return Application specific ui configuration table.
*/
const ui_config_table_content_t* AppUi_GetConfigTable(unsigned* table_length);

/*! \brief Configures the Focus Select module in the framework with the
    source prioritisation for the Earbud Application.
*/
void AppUi_ConfigureFocusSelection(void);

#ifdef INCLUDE_CAPSENSE
const touch_event_config_t* AppUi_GetCapsenseEventTable(unsigned* table_length);
#endif

#ifdef INCLUDE_UI_USER_CONFIG
const ui_user_config_gesture_id_map_t* AppUi_GetGestureMap(unsigned* table_length);

#ifdef INCLUDE_VOICE_UI
const ui_user_config_composite_gesture_t * AppUi_GetCompositeGestureMap(unsigned* map_length);
#endif

#endif

#endif /* EARBUD_UI_CONFIG_H_ */
