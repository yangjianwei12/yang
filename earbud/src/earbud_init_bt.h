/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file		earbud_init_bt.h
\defgroup   earbud\src\earbud_init_bt
\brief      Used by Earbud applicatiob to facilitate iniation of Bluetooth handling
  
*/

#ifndef EARBUD_SRC_EARBUD_INIT_BT_H_
#define EARBUD_SRC_EARBUD_INIT_BT_H_

#include <message.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#endif

/*@{*/

#ifdef USE_SYNERGY
#include "peer_pair_le.h"
#include "peer_find_role.h"
#define INIT_CL_CFM CSR_BT_CM_BLUECORE_INITIALIZED_IND
#define INIT_READ_LOCAL_NAME_CFM CM_PRIM
#define INIT_READ_LOCAL_BD_ADDR_CFM CM_PRIM
#define INIT_PEER_PAIR_LE_CFM PEER_PAIR_LE_INIT_CFM
#define INIT_PEER_FIND_ROLE_CFM PEER_FIND_ROLE_INIT_CFM
#else
#define INIT_CL_CFM CL_INIT_CFM
#define INIT_READ_LOCAL_NAME_CFM CL_DM_LOCAL_BD_ADDR_CFM
#define INIT_READ_LOCAL_BD_ADDR_CFM CL_DM_LOCAL_BD_ADDR_CFM
#define INIT_PEER_PAIR_LE_CFM 0
#define INIT_PEER_FIND_ROLE_CFM 0
#endif

#ifdef USE_BDADDR_FOR_LEFT_RIGHT
bool AppInitBt_ConfigInit(Task init_task);
bool AppInitBt_HandleReadLocalBdAddrCfm(Message message);
#endif

bool AppInitBt_ConnectionInit(Task init_task);
#ifndef USE_SYNERGY
void AppInitBt_StartBtInit(void);
#endif
bool AppInitBt_RegisterForBtMessages(Task init_task);

/*@}*/

#endif /* EARBUD_SRC_EARBUD_INIT_BT_H_ */
