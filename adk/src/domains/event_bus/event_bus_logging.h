/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file 
\addtogroup event_bus
\ingroup    domains
\brief      Debug logging level stuff for the Event Bus.
@{
*/

#ifndef EVENT_BUS_LOGGING_H_
#define EVENT_BUS_LOGGING_H_

#define DEBUG_LOG_MODULE_NAME       event_bus

#include <logging.h>

DEBUG_LOG_DEFINE_LEVEL_VAR

#if (defined(DISABLE_LOG) || defined(DESKTOP_BUILD))
#define EventBus_GetCurrentLoggingLevel()   DEBUG_LOG_LEVEL_ERROR
#else
#define EventBus_GetCurrentLoggingLevel()   debug_log_level_event_bus
#endif

#endif /* EVENT_BUS_LOGGING_H_ */
/*! @} End of group documentation */