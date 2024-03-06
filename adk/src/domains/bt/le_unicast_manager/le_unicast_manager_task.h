/*!
   \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup le_unicast_manager
   \brief      Private types and functions for le_unicast_manager_task.
   @{
*/

#ifndef LE_UNICAST_MANAGER_TASK_H_
#define LE_UNICAST_MANAGER_TASK_H_

#include "le_unicast_manager_private.h"

/*! \brief Initialises the Unicast Manager Task
*/
void LeUnicastManagerTask_Init(void);

/*! Reset the Task data for the unicast manager. */
void LeUnicastManager_TaskReset(void);

/*! Utility to identify if there is a reconfiguration needed for this instance. */
bool LeUnicastManager_IsReconfigRequired(le_um_instance_t *inst, multidevice_side_t side);

#endif /* LE_UNICAST_MANAGER_TASK_H_ */

/*! @} */