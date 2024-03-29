/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#ifndef BAP_INIT_H_
#define BAP_INIT_H_

#include "bap_server_private.h"
#include "bap_server.h"
#include "service_handle.h"


#define BAP_INVALID_HANDLE     0

#define BAP_SERVER_ISOC_TYPE_UNICAST          (uint16_t)0x01
#define BAP_SERVER_ISOC_TYPE_BROADCAST        (uint16_t)0x02

/*!
NAME
    bapServerPacsUtilitiesGetPacsInstance
    
DESCRIPTION
    Get the PACS instace from the BAP.
*/

ServiceHandle bapServerPacsUtilitiesGetPacsInstance(void);

bapProfileHandle bapServerGetBapInstance(void);

bool bapServerAddConnectionIdToList(BAP *bapInst, ConnId connectionId);

void bapServerAddConfigToConnection(BAP *bapInst, ConnId connectionId);

/*!
NAME
    bapServerRemoveConnectionIdFromList
    
DESCRIPTION
    Remove the Connection Id from given BAP instance cid list.
*/
void bapServerRemoveConnectionIdFromList(BAP *bapInst, ConnId connectionId);

#endif
