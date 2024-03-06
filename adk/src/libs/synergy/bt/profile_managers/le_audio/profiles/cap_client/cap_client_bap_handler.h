/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/
#ifndef CAP_CLIENT_BAP_HANDLER_H
#define CAP_CLIENT_BAP_HANDLER_H

#include "cap_client_private.h"

void capClientHandleBapMsg(CAP_INST *inst, const Msg msg);

#ifdef INSTALL_LEA_UNICAST_CLIENT
void capClientHandleUpdateMetadataCfm(CAP_INST *inst,
                                BapUnicastClientUpdateMetadataCfm *cfm,
                                CapClientGroupInstance* cap);

void capClientHandleUpdateMetadataInd(CAP_INST *inst,
                                BapUnicastClientUpdateMetadataInd *ind,
                                CapClientGroupInstance* cap);
#endif /*  INSTALL_LEA_UNICAST_CLIENT */
#endif
