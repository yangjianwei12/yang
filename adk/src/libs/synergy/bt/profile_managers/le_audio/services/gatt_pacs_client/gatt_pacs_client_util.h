
/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #59 $
******************************************************************************/

#ifndef GATT_PACS_CLIENT_UTIL_H_
#define GATT_PACS_CLIENT_UTIL_H_

#include "gatt_pacs_client_private.h"

typedef struct
{
    connection_id_t cid;
    uint16 startHandle;
    uint16 endHandle;
} GattClientRegistrationParams;

ServiceHandle getPacsServiceHandle(GPacsC** gattPacsClient, CsrCmnList_t* list);

bool GattRegisterPacsClient(GattClientRegistrationParams* regParam, GPacsC* gattPacsClient);

void InitPacRecordCharcList(CsrCmnListElm_t* elem);

void pacRecordRead(CsrCmnListElm_t* elem, void* data);
void writeSinkPacRecordCccd(CsrCmnListElm_t* elem, void* data);
void writeSourcePacRecordCccd(CsrCmnListElm_t* elem, void* data);
CsrBool pacsInstFindBySrvcHndl(CsrCmnListElm_t* elem, void* data);

#define PACS_ADD_SERVICE_HANDLE(_List) \
    (ServiceHandleListElm_t *)CsrCmnListElementAddLast(&(_List), \
                                              sizeof(ServiceHandleListElm_t))

#define PACS_REMOVE_SERVICE_HANDLE(_List,_ClientHandle) \
                              CsrCmnListIterateAllowRemove(&(_List), \
                                        pacsInstFindBySrvcHndl,(void *)(&(_ClientHandle)))

#endif /* !GATT_PACS_CLIENT_UTIL_H_ */
