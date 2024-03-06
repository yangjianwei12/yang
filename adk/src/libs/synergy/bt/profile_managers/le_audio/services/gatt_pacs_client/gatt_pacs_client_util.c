/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_bt_gatt_lib.h"
#include "gatt_pacs_client_private.h"
#include "gatt_pacs_client_util.h"
#include "gatt_pacs_client_debug.h"
#include "gatt_pacs_client_msg_handler.h"

PacsC* mainInst;

bool GattRegisterPacsClient(GattClientRegistrationParams* regParam, GPacsC* gattPacsClient)
{
    CsrBtTypedAddr addr;

    gattPacsClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_PACS_CLIENT_IFACEQUEUE);

    if (gattPacsClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
            &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattPacsClient->srvcElem->gattId,
                                                  regParam->startHandle,
                                                  regParam->endHandle,
                                                  addr);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

ServiceHandle getPacsServiceHandle(GPacsC** gattPacsClient, CsrCmnList_t* list)
{
    ServiceHandleListElm_t* elem = PACS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gattPacsClient, sizeof(GPacsC));

    if ((*gattPacsClient) != NULL)
        (*gattPacsClient)->srvcElem = elem;

    return elem->service_handle;
}

void InitPacRecordCharcList(CsrCmnListElm_t* elem)
{
    /* Initialize a PAC record Charc list element. This function is called every
     * time a new entry is made on the Pac Record Charc list */
    CsrBtPacRecordCharacElement* cElem = (CsrBtPacRecordCharacElement*)elem;

    cElem->recordId = 0;
    cElem->declarationHandle = INVALID_PACS_HANDLE;
    cElem->valueHandle = INVALID_PACS_HANDLE;
    cElem->pacRecordCccdHandle = INVALID_PACS_HANDLE;
    cElem->endHandle = INVALID_PACS_HANDLE;
}

void pacRecordRead(CsrCmnListElm_t* elem, void* data)
{
    CsrBtPacRecordCharacElement* pacRecordElem = (CsrBtPacRecordCharacElement*)elem;
    GPacsC* pacs = (GPacsC*)data;

    CsrBtGattReadReqSend(pacs->srvcElem->gattId,
                         pacs->srvcElem->cid,
                         pacRecordElem->valueHandle,
                         0);
}

void writeSourcePacRecordCccd(CsrCmnListElm_t* elem, void* data)
{
    CsrBtPacRecordCharacElement* pacRecordElem = (CsrBtPacRecordCharacElement*)elem;
    GPacsC* pacs = (GPacsC*)data;

    pacs->writeSourceCccdCount++;
    pacsClientWriteRequestSend(pacs->srvcElem->gattId,
                               pacs->srvcElem->cid,
                               pacRecordElem->pacRecordCccdHandle,
                               pacs->notifyEnable);
}

void writeSinkPacRecordCccd(CsrCmnListElm_t* elem, void* data)
{
    CsrBtPacRecordCharacElement* pacRecordElem = (CsrBtPacRecordCharacElement*)elem;
    GPacsC* pacs = (GPacsC*)data;

    pacs->writeSinkCccdCount++;
    pacsClientWriteRequestSend(pacs->srvcElem->gattId,
                             pacs->srvcElem->cid,
                             pacRecordElem->pacRecordCccdHandle,
                             pacs->notifyEnable);
}

CsrBool pacsInstFindBySrvcHndl(CsrCmnListElm_t* elem, void* data)
{
    ServiceHandleListElm_t* clntHndlElm = (ServiceHandleListElm_t*)elem;
    ServiceHandle clientHandle = *(ServiceHandle*)data;

    return (clntHndlElm->service_handle == clientHandle);
}

static void pacsInitClientHandleList(CsrCmnListElm_t* elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ServiceHandleListElm_t* cElem = (ServiceHandleListElm_t*)elem;

    cElem->service_handle = 0;
}

void GattPacsClientInit(void** gash)
{
    mainInst = CsrPmemZalloc(sizeof(PacsC));
    *gash = mainInst;

    CsrCmnListInit(&mainInst->clientHandleList, 0, pacsInitClientHandleList, NULL);

    GATT_PACS_CLIENT_INFO("Pacs: GattPacsClientInit\n\n");
}

#ifdef ENABLE_SHUTDOWN
void gattPacsClientDeinit(void** gash)
{
    GATT_PACS_CLIENT_INFO("Pacs:  gattPacsClientDeinit\n\n");
    CsrCmnListDeinit(&mainInst->clientHandleList);
    CsrPmemFree(mainInst);
}
#endif

GattPacsClientDeviceData *GattPacsClientGetHandlesReq(ServiceHandle clntHndl)
{
    GPacsC *pacsClient = ServiceHandleGetInstanceData(clntHndl);

    if (pacsClient)
    {
        CsrBtPacRecordCharacElement *pacsElem;
        uint8 index = 0;
        GattPacsClientDeviceData *pacsHandles = CsrPmemZalloc(sizeof(GattPacsClientDeviceData));

        /* Allocate memory only when source/sink count is non zero */
        if (pacsClient->sinkPacRecordCount != 0)
        {
            pacsHandles->pacsSinkPacRecordHandle = (uint16*)CsrPmemZalloc(pacsClient->sinkPacRecordCount * sizeof(uint16));
            pacsHandles->pacsSinkPacRecordCcdHandle = (uint16*)CsrPmemZalloc(pacsClient->sinkPacRecordCount * sizeof(uint16));
        }

        if (pacsClient->sourcePacRecordCount != 0)
        {
            pacsHandles->pacsSourcePacRecordHandle = (uint16*)CsrPmemZalloc(pacsClient->sourcePacRecordCount * sizeof(uint16));
            pacsHandles->pacsSourcePacRecordCcdHandle = (uint16*)CsrPmemZalloc(pacsClient->sourcePacRecordCount * sizeof(uint16));
        }

        pacsHandles->sinkPacRecordCount = pacsClient->sinkPacRecordCount;
        pacsHandles->sourcePacRecordCount = pacsClient->sourcePacRecordCount;

        pacsHandles->pacsSinkAudioLocationHandle = pacsClient->pacsSinkAudioLocationHandle;
        pacsHandles->pacsSinkAudioLocationCcdHandle = pacsClient->pacsSinkAudioLocationHandle;

        pacsHandles->pacsSourceAudioLocationHandle = pacsClient->pacsSourceAudioLocationHandle;
        pacsHandles->pacsSourceAudioLocationCcdHandle = pacsClient->pacsSourceAudioLocationCcdHandle;

        pacsHandles->pacsAvailableAudioContextHandle = pacsClient->pacsAvailableAudioContextHandle;
        pacsHandles->pacsAvailableAudioContextCcdHandle = pacsClient->pacsAvailableAudioContextCcdHandle;

        pacsHandles->pacsSupportedAudioContextHandle = pacsClient->pacsSupportedAudioContextHandle;
        pacsHandles->pacsSupportedAudioContextCcdHandle = pacsClient->pacsSupportedAudioContextCcdHandle;

        pacsHandles->endHandle = pacsClient->endHandle;
        pacsHandles->startHandle = pacsClient->startHandle;

        pacsHandles->sinkPacRecordCount = pacsClient->sinkPacRecordCount;
        pacsHandles->sourcePacRecordCount = pacsClient->sourcePacRecordCount;

        pacsElem = (CsrBtPacRecordCharacElement*)pacsClient->sinkPacRecordList.first;

        while (pacsElem)
        {
            pacsHandles->pacsSinkPacRecordHandle[index] = pacsElem->valueHandle;
            pacsHandles->pacsSinkPacRecordCcdHandle[index] = pacsElem->pacRecordCccdHandle;
            index++;
            pacsElem = pacsElem->next;
        }

        pacsElem = (CsrBtPacRecordCharacElement*)pacsClient->sourcePacRecordList.first;
        index = 0;

        while (pacsElem)
        {
            pacsHandles->pacsSourcePacRecordHandle[index] = pacsElem->valueHandle;
            pacsHandles->pacsSourcePacRecordCcdHandle[index] = pacsElem->pacRecordCccdHandle;
            index++;
            pacsElem = pacsElem->next;
        }
        return pacsHandles;
    }
    return NULL;
}
