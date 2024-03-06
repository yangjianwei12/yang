/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CCP_COMMON_H_
#define CCP_COMMON_H_

#include <gatt_telephone_bearer_client.h>

#include "ccp.h"
#include "ccp_private.h"


/***************************************************************************
NAME
    ccpDestroyReqTbsInstList

DESCRIPTION
    Destroy all the lists of TBS instances in the profile memory instance.
*/
void ccpDestroyReqTbsInstList(CCP *ccp_inst);

/***************************************************************************
NAME
    ccpDestroyReqTbsSrvcHndlList

DESCRIPTION
    Destroy all the lists of service handles in the profile memory instance.
*/
void ccpDestroyReqTbsSrvcHndlList(CCP *ccp_inst);

/***************************************************************************
NAME
    ccpDestroyReqAllInstList

DESCRIPTION
    Destroy all the lists of TBS instances in the profile memory instance.
*/
void ccpDestroyReqAllInstList(CCP *ccp_inst);


/***************************************************************************
NAME
    ccpDestroyReqAllSrvcHndlList

DESCRIPTION
    Destroy all the lists of service handles in the profile memory instance.
*/
void ccpDestroyReqAllSrvcHndlList(CCP *ccp_inst);



#endif
