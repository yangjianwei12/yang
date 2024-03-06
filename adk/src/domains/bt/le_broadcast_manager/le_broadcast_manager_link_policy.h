/*!
   \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
               All Rights Reserved.
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \file
   \addtogroup leabm
   \brief      LE Broadcast Manager Link Policy.
   @{
*/

#ifndef LE_BROADCAST_MANAGER_LINK_POLICY_H
#define LE_BROADCAST_MANAGER_LINK_POLICY_H

#if defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LEA_LINK_POLICY)

void LeBroadcastManager_LinkPolicyInit(void);

#else
    
#define LeBroadcastManager_LinkPolicyInit()

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST) && defined(INCLUDE_LEA_LINK_POLICY) */

#endif /* LE_BROADCAST_MANAGER_LINK_POLICY_H */
/*! @} */