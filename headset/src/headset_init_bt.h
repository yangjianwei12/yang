/*!
\copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_init_bt.h
\brief      Header file for initialisation
*/

#ifndef HEADSET_INIT_BT_H
#define HEADSET_INIT_BT_H


#include <message.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#endif

#ifdef USE_SYNERGY
#define INIT_CL_CFM CSR_BT_CM_BLUECORE_INITIALIZED_IND
#define INIT_READ_LOCAL_BD_ADDR_CFM CM_PRIM

#else
#define INIT_CL_CFM CL_INIT_CFM
#define INIT_READ_LOCAL_BD_ADDR_CFM CL_DM_LOCAL_BD_ADDR_CFM
#endif


bool AppInitBt_ConnectionInit(Task init_task);

#ifndef USE_SYNERGY
TaskData *AppInitBt_GetTask(void);
void AppInitBt_StartBtInit(void);
#endif

bool AppInitBt_InitHandleClDmLocalBdAddrCfm(Message message);



#endif // HEADSET_INIT_BT_H
