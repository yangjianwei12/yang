/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/


#include "micp.h"
#include "micp_indication.h"

/****************************************************************************/
void micpHandleMicsMuteValueInd(MICP *micp_inst,
                                const GattMicsClientMuteValueInd *ind)
{
    MAKE_MICP_MESSAGE(MicpMuteValueInd);

    message->id = MICP_MUTE_VALUE_IND;
    message->prflHndl = micp_inst->micp_srvc_hdl;
    message->muteValue = ind->muteValue;

    MicpMessageSend(micp_inst->app_task, message);
}

