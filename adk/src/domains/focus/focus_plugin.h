/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   focus_domain Focus
\ingroup    domains
*/
#ifndef FOCUS_PLUGIN_H
#define FOCUS_PLUGIN_H

/*! @{ */

typedef bool (*focus_plugin_set_config_t)(unsigned config_option, void* config);
typedef bool (*focus_plugin_get_config_t)(unsigned config_option, void* config);

typedef struct
{
    focus_plugin_set_config_t set_config;
    focus_plugin_get_config_t get_config;
} focus_plugin_configure_t;

/*! \brief Register callbacks to configure the plugin */
void Focus_PluginConfigure(focus_plugin_configure_t const * plugin_configure);

/*! \brief Interface to set plugin specific configuration. */
bool Focus_SetConfig(unsigned config_option, void* config);

/*! \brief Interface to get plugin specific configuration. */
bool Focus_GetConfig(unsigned config_option, void* config);

/*! @} */

#endif /* FOCUS_PLUGIN_H */
