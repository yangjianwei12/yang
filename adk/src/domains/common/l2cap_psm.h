/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\addtogroup common_domain
\brief      Header file for reserved L2CAP PSMs
*/

#ifndef L2CAP_PSM_H
#define L2CAP_PSM_H


#ifdef USE_DYNAMIC_PEER_EARBUD_PSM

#define L2CAP_PSM_PEER_SIGNALING    L2CA_PSM_INVALID
#define L2CAP_PSM_HANDOVER_PROFILE  L2CA_PSM_INVALID
#define L2CAP_PSM_MIRROR_PROFILE    L2CA_PSM_INVALID

#else

#define L2CAP_PSM_PEER_SIGNALING    0xFEFF
#define L2CAP_PSM_HANDOVER_PROFILE  0xFEFD
#define L2CAP_PSM_MIRROR_PROFILE    0xFEFB

#endif

#endif // L2CAP_PSM_H
