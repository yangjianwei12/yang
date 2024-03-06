/****************************************************************************
 * Copyright (c) 2023 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_sco_api.h
 *
 */

/****************************************************************************
Include Files
*/
#include "capabilities.h"


/****************************************************************************
Function Prototypes
*/

/**
 * \brief Gets ISO sink endpoint connected to this operator.
 *        Must be called after connecting ISO sink endpoint (E.g. In Operator start).    
 * \param op_data Pointer to the operator instance data.
 *
 * \return Returns a pointer to the ISO sink endpoint.
 */
extern ENDPOINT* ext_get_iso_sink_ep_for_this_operator (OPERATOR_DATA *op_data);

/**
 * \brief Gets ISO sink endpoint's scheduled fire time.
 *        It can be used to adjust rate of output frame delivery.    
 * \param endpoint Pointer to the ISO sink endpoint obtained from ext_get_iso_sink_ep_for_this_operator.
 *
 * \return Returns the scheduled fire time of ISO sink endpoint.
 */
extern TIME ext_get_iso_sink_ep_fire_time (ENDPOINT* endpoint);