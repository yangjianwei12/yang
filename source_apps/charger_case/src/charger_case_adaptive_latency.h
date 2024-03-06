/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header for the adaptive latency charger case component
*/

#ifndef CHARGER_CASE_ADAPTIVE_LATENCY_H
#define CHARGER_CASE_ADAPTIVE_LATENCY_H
#include <message.h>

/*! \brief Initialise the adaptive latency component of the charger case
 *
 *  \param init_task Task to send init completion message to
 *  \return Always returns TRUE
*/
bool ChargerCase_AdaptiveLatencyInit (Task init_task);

#endif /* CHARGER_CASE_ADAPTIVE_LATENCY_H */
