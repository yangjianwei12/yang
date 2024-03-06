/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for bluetooth initialisation module
*/


#ifndef CHARGER_CASE_INIT_BT_H
#define CHARGER_CASE_INIT_BT_H

#include <message.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#endif

/*@{*/


#ifdef USE_SYNERGY
#define INIT_CL_CFM                     CSR_BT_CM_BLUECORE_INITIALIZED_IND
#define INIT_READ_LOCAL_BD_ADDR_CFM     CM_PRIM
#else
#define INIT_CL_CFM                     CL_INIT_CFM
#define INIT_READ_LOCAL_BD_ADDR_CFM     CL_DM_LOCAL_BD_ADDR_CFM
#endif



/*! \brief  to the main app Task.
*/
bool AppConnectionInit(Task init_task);


/*! \brief  Start the BT initialization.
 */
void ChargerCase_StartBtInit(void);

/*! \brief  Post initialization, any event registeration etc for BT can be done here.
 */
bool ChargerCase_RegisterForBtMessages(Task init_task);

/*! \brief  Message dispatcher registration.
 */
bool AppMessageDispatcherRegister(Task init_task);


#endif /* CHARGER_CASE_INIT_BT_H */

