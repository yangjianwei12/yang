/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_ascs_client_private.h"
#include "gatt_ascs_client_util.h"
#include "csr_bt_gatt_lib.h"

AscsC *ascsClientMain;

bool GattRegisterAscsClient(GattAscsClientRegistrationParams *regParam, GAscsC *gattAscsClient)
{
    CsrBtTypedAddr addr;

    gattAscsClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_ASCS_CLIENT_IFACEQUEUE);

    if (gattAscsClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattAscsClient->srvcElem->gattId,
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

ServiceHandle getAscsServiceHandle(GAscsC **gattAscsClient, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = ASCS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **)gattAscsClient, sizeof(GAscsC));

    if ((*gattAscsClient) != NULL)
        (*gattAscsClient)->srvcElem = elem;


    return elem->service_handle;
}

CsrBool ascsInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clntHndlElm = (ServiceHandleListElm_t *)elem;
    ServiceHandle clientHandle = *(ServiceHandle *)data;

    return (clntHndlElm->service_handle == clientHandle);
}

void InitAscsAseCharcList(CsrCmnListElm_t *elem)
{
    /* Initialize a ASCS ASE Charc list element. This function is called every
     * time a new entry is made on the ASE Charc list */
    CsrBtAseCharacElement *cElem = (CsrBtAseCharacElement *) elem;

    cElem->aseId = 0;
    cElem->declarationHandle = 0;
    cElem->valueHandle = 0;
    cElem->aseCccdHandle = 0;
    cElem->endHandle = 0;
}


void InitAscsServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a ASCS Service Handle list element. This function is called every
     * time a new entry is made on the service handle list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gatt_ascs_client_init(void **gash)
{
    ascsClientMain = CsrPmemZalloc(sizeof(*ascsClientMain));
    *gash = ascsClientMain;

    CsrCmnListInit(&ascsClientMain->clientHandleList, 0, InitAscsServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattAscsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&ascsClientMain->clientHandleList);
    CsrPmemFree(ascsClientMain);
}
#endif

GattAscsClientDeviceData *GattAscsClientGetHandlesReq(ServiceHandle clntHndl)
{
    GAscsC *ascsClient = ServiceHandleGetInstanceData(clntHndl);
    uint8 index = 0;

    if (ascsClient)
    {
        CsrBtAseCharacElement *aseElem;
        GattAscsClientDeviceData* ascsHandles = CsrPmemZalloc(sizeof(GattAscsClientDeviceData));

        /* Allocate memory only when source/sink count is non zero */
        if (ascsClient->aseSinkCharacCount != 0)
        {
            ascsHandles->sinkAseId = (uint8*)CsrPmemZalloc(ascsClient->aseSinkCharacCount * sizeof(uint8));
            ascsHandles->sinkAseCcdHandle = (uint16*)CsrPmemZalloc(ascsClient->aseSinkCharacCount * sizeof(uint16));
            ascsHandles->sinkAseHandle = (uint16*)CsrPmemZalloc(ascsClient->aseSinkCharacCount * sizeof(uint16));
        }

        if (ascsClient->aseSourceCharacCount != 0)
        {
            ascsHandles->sourceAseId = (uint8*)CsrPmemZalloc(ascsClient->aseSourceCharacCount * sizeof(uint8));
            ascsHandles->sourceAseCcdHandle = (uint16*)CsrPmemZalloc(ascsClient->aseSourceCharacCount * sizeof(uint16));
            ascsHandles->sourceAseHandle = (uint16*)CsrPmemZalloc(ascsClient->aseSourceCharacCount * sizeof(uint16));
        }

        ascsHandles->startHandle = ascsClient->startHandle;
        ascsHandles->endHandle = ascsClient->endHandle;
        ascsHandles->asesAseControlPointHandle = ascsClient->asesAseControlPointHandle;
        ascsHandles->asesAseControlPointCcdHandle = ascsClient->asesAseControlPointCcdHandle;

        ascsHandles->sinkAseCount = ascsClient->aseSinkCharacCount;
        ascsHandles->sourceAseCount = ascsClient->aseSourceCharacCount;

        aseElem = (CsrBtAseCharacElement*)ascsClient->asesSinkCharacList.first;

        while (aseElem)
        {
            ascsHandles->sinkAseId[index] = aseElem->aseId;
            ascsHandles->sinkAseCcdHandle[index] = aseElem->aseCccdHandle;
            ascsHandles->sinkAseHandle[index] = aseElem->valueHandle;
            aseElem = aseElem->next;
            index++;
        }

        aseElem = (CsrBtAseCharacElement*)ascsClient->asesSourceCharacList.first;
        index = 0;

        while (aseElem)
        {
            ascsHandles->sourceAseId[index] = aseElem->aseId;
            ascsHandles->sourceAseCcdHandle[index] = aseElem->aseCccdHandle;
            ascsHandles->sourceAseHandle[index] = aseElem->valueHandle;
            aseElem = aseElem->next;
            index++;
        }
        return ascsHandles;
    }
    return NULL;
}
