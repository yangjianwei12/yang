/* Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>

#include <service_handle.h>
#include "gatt_csis_client_private.h"
#include "gatt_csis_client_msg_handler.h"
#include "csr_pmem.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_core_stack_pmalloc.h"
#include "gatt_csis_client_debug.h"

GattCsisClient *csisClientMain;

static void InitCsisServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

static ServiceHandle getCsisServiceHandle(GCsisC** gattCsisClient, CsrCmnList_t* list)
{
    ServiceHandleListElm_t* elem = CSIS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **)gattCsisClient, sizeof(GCsisC));

    if ((*gattCsisClient) != NULL)
       (*gattCsisClient)->srvcElem = elem;

    return elem->service_handle;
}

static bool GattCsisRegisterClient(GattCsisClientRegistrationParams *regParam, GCsisC *gattCsisClient)
{
    CsrBtTypedAddr addr;

    gattCsisClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_CSIS_CLIENT_IFACEQUEUE);

    GATT_CSIS_CLIENT_INFO("GattCsisRegisterClient %d\n", gattCsisClient->srvcElem->gattId);
    if (gattCsisClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattCsisClient->srvcElem->gattId,
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

/****************************************************************************/
void GattCsisClientInit(
                 AppTask appTask,
                 const GattCsisClientInitParams *const clientInitParams,
                 const GattCsisClientDeviceData *deviceData)
{
    GCsisC *csisClient = NULL;
    ServiceHandle srvcHndl = 0;
    GattCsisClientRegistrationParams registrationParams;

    /* validate the input parameters */
    if ((appTask == CSR_SCHED_QID_INVALID) || (clientInitParams == NULL))
    {
        GATT_CSIS_CLIENT_PANIC("Invalid initialisation parameters\n");
        return;
    }

    srvcHndl = getCsisServiceHandle(&csisClient, &(csisClientMain->serviceHandleList));

    if (csisClient)
    {
        memset(&registrationParams, 0, sizeof(GattCsisClientRegistrationParams));

        /* Set up library handler for external messages */
        csisClient->lib_task = CSR_BT_CSIS_CLIENT_IFACEQUEUE;

        /* Store the Task function parameter.
           All library messages need to be sent here */
        csisClient->app_task = appTask;

        if (deviceData)
        {
            memcpy(&(csisClient->handles), deviceData, sizeof(GattCsisClientDeviceData));
        }
        else
        {
            memset(&(csisClient->handles), INVALID_CSIS_HANDLE, sizeof(GattCsisClientDeviceData));
        }

        /* Save the start and the end handles */
        csisClient->handles.startHandle = clientInitParams->startHandle;
        csisClient->handles.endHandle = clientInitParams->endHandle;


        /*Store the CSIS handle with Invalid Value */

        /* Set the end handle for Lock characteristics to endhandle of the service */
        csisClient->csisLockEndHandle = INVALID_CSIS_HANDLE;
        /* Set the end handle for SIRK characteristics to endhandle of the service */
        csisClient->csisSirkEndHandle = INVALID_CSIS_HANDLE;
        /* Set the end handle for SIZE characteristics to endhandle of the service */
        csisClient->csisSizeEndHandle = INVALID_CSIS_HANDLE;

        csisClient->pendingCmd = CSIS_CLIENT_PENDING_NONE;
        csisClient->srvcElem->cid = clientInitParams->cid;

        /* Setup data required for Gatt Client to be registered with the GATT */
        registrationParams.cid = clientInitParams->cid;
        registrationParams.startHandle = clientInitParams->startHandle;
        registrationParams.endHandle = clientInitParams->endHandle;

        CSR_UNUSED(srvcHndl);
        if (GattCsisRegisterClient(&registrationParams, csisClient))
        {
            if (!deviceData)
            {
                GATT_CSIS_CLIENT_INFO("SKD CHAR Disc %d %d\n",
                    csisClient->handles.startHandle, csisClient->handles.endHandle);
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(csisClient->srvcElem->gattId,
                                                            csisClient->srvcElem->cid,
                                                            csisClient->handles.startHandle,
                                                            csisClient->handles.endHandle);
            }
            else
            {
                sendCsisClientInitCfm(csisClient, CSR_BT_GATT_RESULT_SUCCESS,
                                              CSIS_CHECK_SIZE_SUPPORT(csisClient),
                                              CSIS_CHECK_LOCK_SUPPORT(csisClient));
            }
        }
        else
        {
            GATT_CSIS_CLIENT_ERROR("Register with the GATT failed!\n");
            sendCsisClientInitCfm(csisClient, GATT_CSIS_CLIENT_STATUS_FAILED,
                                              CSIS_CHECK_SIZE_SUPPORT(csisClient),
                                              CSIS_CHECK_LOCK_SUPPORT(csisClient));
        }
    }
    else
    {
        /* Not able to get the service handle send the confirmation with error */
        MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientInitCfm);

        /* Fill in client reference */
        message->srvcHndl = 0x00;
        /* Fill in size characteristics support */
        message->sizeSupport = 0x00;
        /* Fill in lock characteristics support */
        message->lockSupport = 0x00;
        /* Fill in the status */
        message->status = GATT_CSIS_CLIENT_STATUS_FAILED;
        message->cid = clientInitParams->cid;

        /* send the confirmation message to app task  */
        CsisMessageSend(appTask, GATT_CSIS_CLIENT_INIT_CFM, message);
    }
}


/****************************************************************************/
void GattCsisClientTerminate(ServiceHandle srvcHndl)
{
    GCsisC *csisClient = ServiceHandleGetInstanceData(srvcHndl);

    /* Validate the Input Parameters */
    if (csisClient)
    {
        AppTask app_task = csisClient->app_task;
        MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientTerminateCfm);

        /* Unregister with the GATT Manager and verify the result */

        CsrBtGattUnregisterReqSend(csisClient->srvcElem->gattId);

        memcpy(&(message->deviceData), &(csisClient->handles), sizeof(GattCsisClientDeviceData));
        message->srvcHndl = srvcHndl;
        if (ServiceHandleFreeInstanceData(srvcHndl))
        {
            message->status = GATT_CSIS_CLIENT_STATUS_SUCCESS;

            /* Free srvcHndl maintained in serviceHandleList*/
            CSIS_REMOVE_SERVICE_HANDLE(csisClientMain->serviceHandleList, srvcHndl);
        }
        else
        {
            message->status = GATT_CSIS_CLIENT_STATUS_FAILED;
        }

        CsisMessageSend(app_task, GATT_CSIS_CLIENT_TERMINATE_CFM, message);

    }
    else
    {
        GATT_CSIS_CLIENT_PANIC("GCSISC: Null instance");
    }

}

/****************************************************************************/
void GattCsisRegisterForNotification(ServiceHandle srvcHndl, GattCsisClientCsInfo csInfo, bool enable)
{
    GCsisC *csisClient = ServiceHandleGetInstanceData(srvcHndl);
    uint16 handle;

    /* Validate the Input Parameters */
    if (csisClient == NULL)
    {
        GATT_CSIS_CLIENT_PANIC("GCSISC: Null instance");
        return;
    }

    switch (csInfo)
    {
        case GATT_CSIS_CLIENT_SIRK:
            handle = csisClient->handles.csisSirkCcdHandle;
            break;

        case GATT_CSIS_CLIENT_SIZE:
            handle = csisClient->handles.csisSizeCcdHandle;
            break;

        case GATT_CSIS_CLIENT_LOCK:
            handle = csisClient->handles.csisLockCcdHandle;
            break;

        default:
            handle = INVALID_CSIS_HANDLE;
    }

    if (handle != INVALID_CSIS_HANDLE)
    {
        MAKE_CSIS_CLIENT_MESSAGE(CsisClientInternalMsgNotificationReq);

        message->srvcHndl = csisClient->srvcElem->service_handle;
        message->handle = handle;
        message->enable = enable;

        GATT_CSIS_CLIENT_DEBUG("Func:GattCsisRegisterForNotification() enable = %x\n", message->enable);

        CsisMessageSendConditionally(CSR_BT_CSIS_CLIENT_IFACEQUEUE, CSIS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ,
                message, &csisClient->pendingCmd);
    }
}

void GattCsisReadCsInfoReq(ServiceHandle srvcHndl, GattCsisClientCsInfo csInfo)
{
    uint16 handle;
    GCsisC *csisClient = ServiceHandleGetInstanceData(srvcHndl);

    /* Validate the Input Parameters */
    if (csisClient == NULL)
    {
        GATT_CSIS_CLIENT_PANIC("GCSISC: Null instance");
        return;
    }

    switch (csInfo)
    {
        case GATT_CSIS_CLIENT_SIRK:
            handle = csisClient->handles.csisSirkHandle;
            break;

        case GATT_CSIS_CLIENT_SIZE:
            handle = csisClient->handles.csisSizeHandle;
            break;

        case GATT_CSIS_CLIENT_RANK:
            handle = csisClient->handles.csisRankHandle;
            break;

        case GATT_CSIS_CLIENT_LOCK:
            handle = csisClient->handles.csisLockHandle;
            break;

        default:
            handle = INVALID_CSIS_HANDLE;
    }

    if (handle != INVALID_CSIS_HANDLE)
    {
        MAKE_CSIS_CLIENT_MESSAGE(CsisClientInternalMsgReadCsInfoReq);

        message->srvcHndl = csisClient->srvcElem->service_handle;
        message->handle = handle;
        GATT_CSIS_CLIENT_DEBUG("Func:GattCsisReadCoordinatedSetInfoReq() handle = %x\n", handle);

        CsisMessageSendConditionally(CSR_BT_CSIS_CLIENT_IFACEQUEUE, CSIS_CLIENT_INTERNAL_MSG_READ_CS_INFO_REQ,
                message, &csisClient->pendingCmd);
    }
}

void GattCsisWriteLockReq(ServiceHandle srvcHndl, bool enableLock)
{
    GCsisC *csisClient = ServiceHandleGetInstanceData(srvcHndl);

    /* Validate the Input Parameters */
    if (csisClient == NULL)
    {
        GATT_CSIS_CLIENT_PANIC("GCSISC: Null instance");
        return;
    }

    if (csisClient->handles.csisLockHandle != INVALID_CSIS_HANDLE)
    {
        MAKE_CSIS_CLIENT_MESSAGE(CsisClientInternalMsgWriteLockReq);

        message->srvcHndl = csisClient->srvcElem->service_handle;
        message->handle = csisClient->handles.csisLockHandle;
        message->enableLock = enableLock;

        GATT_CSIS_CLIENT_DEBUG("Func:GattCsisWriteLockReq() enable_lock = %x\n", message->enableLock);
        CsisMessageSendConditionally(CSR_BT_CSIS_CLIENT_IFACEQUEUE, CSIS_CLIENT_INTERNAL_MSG_WRITE_LOCK_REQ,
                message, &csisClient->pendingCmd);
    }
}

void gattCsisClientInit(void **gash)
{
    csisClientMain = CsrPmemZalloc(sizeof(*csisClientMain));
    *gash = csisClientMain;

    CsrCmnListInit(&csisClientMain->serviceHandleList, 0, InitCsisServiceHandleList, NULL);
}

CsrUint8 csisInstFindBySrvcHndl(CsrCmnListElm_t* elem, void* data)
{
    ServiceHandleListElm_t* clnt_hndl_elm = (ServiceHandleListElm_t*)elem;
    ServiceHandle clientHandle = *(ServiceHandle*)data;

    return (clnt_hndl_elm->service_handle == clientHandle);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattCsisClientDeInit(void **gash)
{
    CsrCmnListDeinit(&csisClientMain->serviceHandleList);
    CsrPmemFree(csisClientMain);
}
#endif

GattCsisClientDeviceData *GattCsisClientGetHandlesReq(ServiceHandle clntHndl)
{
    CsisC *csisClient = ServiceHandleGetInstanceData(clntHndl);

    if (csisClient)
    {
        GattCsisClientDeviceData *csisHandles = CsrPmemZalloc(sizeof(GattCsisClientDeviceData));
        memcpy(csisHandles, &(csisClient->handles), sizeof(GattCsisClientDeviceData));
        return csisHandles;
    }
    GATT_CSIS_CLIENT_ERROR("GattCsisClientGetHandlesReq: csisClient Null instance");
    return NULL;
}
