/* Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd. */
/*  */


#include "gatt_csis_client_private.h"
#include "gatt_csis_client_msg_handler.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "gatt_csis_client_debug.h"

#define GATT_CSIS_CLIENT_DEBUG_LIB
/***************************************************************************
NAME
    GattCsisClientStatus

DESCRIPTION
   Utility function to map gatt status to gatt csis status
*/
static GattCsisClientStatus csisGetStatus(const CsrBtResultCode status)
{
    GattCsisClientStatus csisStatus;
    
    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            csisStatus = GATT_CSIS_CLIENT_STATUS_SUCCESS;
            break;
            
        case CSR_BT_GATT_ACCESS_RES_READ_NOT_PERMITTED:
        case CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED:
            csisStatus = GATT_CSIS_CLIENT_STATUS_NOT_ALLOWED;
            break;

        case GATT_CSIS_CLIENT_LOCK_DENIED:
            csisStatus = GATT_CSIS_CLIENT_LOCK_DENIED;
            break;

        case GATT_CSIS_CLIENT_LOCK_RELEASE_NOT_ALLOWED:
            csisStatus = GATT_CSIS_CLIENT_LOCK_RELEASE_NOT_ALLOWED;
            break;

        case GATT_CSIS_CLIENT_INVALID_LOCAL_VALUE:
            csisStatus = GATT_CSIS_CLIENT_INVALID_LOCAL_VALUE;
            break;

        case GATT_CSIS_CLIENT_OOB_SIRK_ONLY:
            csisStatus = GATT_CSIS_CLIENT_OOB_SIRK_ONLY;
            break;

        case GATT_CSIS_CLIENT_LOCK_ALREADY_GRANTED:
            csisStatus = GATT_CSIS_CLIENT_LOCK_ALREADY_GRANTED;
            break;

        default:
            csisStatus = GATT_CSIS_CLIENT_STATUS_FAILED;
            break;            
    }
    
    return csisStatus;
}
/***************************************************************************
NAME
    sendCsisClientInitCfm

DESCRIPTION
   Utility function to send init confirmation to application  
*/
void sendCsisClientInitCfm(GCsisC *const csisClient,
                                            const GattCsisClientStatus status,
                                            bool sizeSupport,
                                            bool lockSupport)
{
    MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientInitCfm);

    /* Fill in client reference */
    message->srvcHndl = csisClient->srvcElem->service_handle;
    /* Fill in size characteristics support */
    message->sizeSupport = (uint16)sizeSupport;
    /* Fill in lock characteristics support */
    message->lockSupport = (uint16) lockSupport;
    /* Fill in the status */
    message->status = csisGetStatus(status);
    message->cid = csisClient->srvcElem->cid;
    /* send the confirmation message to app task  */
    CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_INIT_CFM, message);

    /*Clear the pending flag*/
    CLEAR_PENDING_FLAG(csisClient->pendingCmd);
}


/***************************************************************************
NAME
    sendCsisClientTerminateCfm

DESCRIPTION
   Utility function to send terminate confirmation to application  
*/
void sendCsisClientTerminateCfm(GCsisC *const csisClient,
                                            const GattCsisClientStatus status,
                                            connection_id_t cid)
{
    MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientTerminateCfm);

    message->status = status;
    memcpy(&(message->deviceData), &(csisClient->handles), sizeof(GattCsisClientDeviceData));
    message->cid = cid;
    message->srvcHndl = csisClient->srvcElem->service_handle;
    /* send the confirmation message to app task  */
    CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_TERMINATE_CFM, message);

}


/***************************************************************************
NAME
    sendCsisClientNotificationCfm

DESCRIPTION
   Utility function to send notification confirmation to application  
*/
static void sendCsisClientNotificationCfm(GCsisC *const csisClient,
                                                const CsrBtResultCode status,
                                                uint16 handle)
{
    MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientNotificationCfm);

    /* Fill in client reference */
    message->srvcHndl = csisClient->srvcElem->service_handle;
    /* Fill in the status */
    message->status = csisGetStatus(status);

    if (handle == csisClient->handles.csisSirkCcdHandle)
        message->csInfo = GATT_CSIS_CLIENT_SIRK;
    else if (handle == csisClient->handles.csisSizeCcdHandle)
        message->csInfo = GATT_CSIS_CLIENT_SIZE;
    else
        message->csInfo = GATT_CSIS_CLIENT_LOCK;
    message->srvcHndl = csisClient->srvcElem->service_handle;

    /* Send the confirmation message to app task  */
    CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_NOTIFICATION_CFM, message);
}

/***************************************************************************
NAME
    sendCsisClientWriteLockCfm

DESCRIPTION
   Utility function to send lock confirmation confirmation to application  
*/
static void sendCsisClientWriteLockCfm(GCsisC *const csisClient,
                                                const CsrBtResultCode status)
{
    MAKE_CSIS_CLIENT_MESSAGE(GattCsisClientWriteLockCfm);

    /* Fill in client reference */
    message->srvcHndl = csisClient->srvcElem->service_handle;
    /* Fill in the status */
    message->status = csisGetStatus(status);
    /* Send the confirmation message to app task  */
    CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_WRITE_LOCK_CFM, message);
}


/***************************************************************************
NAME
    csisDiscoverCharDescriptors

DESCRIPTION
   Discover csis characteristics descriptors
*/
static void csisDiscoverCharDescriptors(GCsisC *const csisClient)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(csisClient->srvcElem->gattId,
                                                 csisClient->srvcElem->cid,
                                                 csisClient->handles.startHandle + 1,
                                                 csisClient->handles.endHandle);
}

/***************************************************************************
NAME
    csisDiscoverLockCharDescriptors

DESCRIPTION
    Discover csis characteristics descriptors
*/
static void csisDiscoverLockCharDescriptors(GCsisC *const csisClient)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(csisClient->srvcElem->gattId,
                                             csisClient->srvcElem->cid,
                                             csisClient->handles.csisLockHandle + 1,
                                             csisClient->csisLockEndHandle);
}

 /***************************************************************************
NAME
    handleCsisDiscoverCharDescriptorsRsp

DESCRIPTION
   Process csis characteristics descriptors response
*/
static void handleCsisDiscoverCharDescriptorsRsp(GCsisC *const csisClient,
                            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_CSIS_CLIENT_INFO("cfm->status = %x cfm->handle %x cfm->uuid_type :%x, more:%d\n", cfm->status, cfm->handle, cfm->uuid_type, cfm->more_to_come);

    if (cfm->status == CSR_BT_GATT_RESULT_SUCCESS)
    {
        if (cfm->uuid[0] == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID)
        {
            /* Store the handle to register for notifications later */
            if ((csisClient->handles.csisSirkHandle == csisClient->pendingHandle) &&
                csisClient->sirkNotificationSupported)
            {
                csisClient->handles.csisSirkCcdHandle = cfm->handle;
                csisClient->pendingHandle = 0;
            }
            else if ((csisClient->handles.csisSizeHandle == csisClient->pendingHandle)&&
                 csisClient->sirkNotificationSupported)
            {
                 csisClient->handles.csisSizeCcdHandle = cfm->handle;
                 csisClient->pendingHandle = 0;
            }
            else if (csisClient->handles.csisLockHandle == csisClient->pendingHandle)
            {
                csisClient->handles.csisLockCcdHandle = cfm->handle;
                csisClient->pendingHandle = 0;
            }

            GATT_CSIS_CLIENT_DEBUG("CCD Handle = %x %d\n", cfm->handle, cfm->more_to_come);

        }
        else if((cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_SET_IDENTITY_RESOLVING_KEY)
                || (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_COORDINATED_SET_LOCK)
                 || (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_COORDINATED_SET_RANK)
                   || (cfm->uuid[0] == GATT_CHARACTERISTIC_UUID_COORDINATED_SET_SIZE))
        {
            csisClient->pendingHandle = cfm->handle;
        }
        /*Ignore other descriptors*/
    }    

    if (!cfm->more_to_come)
    {
        /*Send init cfm */
        sendCsisClientInitCfm(csisClient, CSR_BT_GATT_RESULT_SUCCESS,
                                  CSIS_CHECK_SIZE_SUPPORT(csisClient),
                                  CSIS_CHECK_LOCK_SUPPORT(csisClient));
    }
}


 /***************************************************************************
NAME
    handleCsisReadResponseCfm

DESCRIPTION
   Handle CSIS read response
*/
static void handleCsisReadResponseCfm(GCsisC *const csisClient,
                            const CsrBtGattReadCfm *cfm)
{
    GattCsisClientStatus csis_client_status = csisGetStatus(cfm->resultCode);

    GattCsisClientReadCsInfoCfm *message =
             (GattCsisClientReadCsInfoCfm *)calloc(1,sizeof(GattCsisClientReadCsInfoCfm) + cfm->valueLength);

    GATT_CSIS_CLIENT_INFO("Func:handleCsisReadResponseCfm() status = %x \n",cfm->resultCode);

    /* Fill in client reference */
    message->srvcHndl = csisClient->srvcElem->service_handle;
    /* Fill in the status */
    message->status = csis_client_status;

    if (cfm->handle == csisClient->handles.csisSirkHandle)
        message->csInfo = GATT_CSIS_CLIENT_SIRK;
    else if (cfm->handle == csisClient->handles.csisSizeHandle)
        message->csInfo = GATT_CSIS_CLIENT_SIZE;
    else if (cfm->handle == csisClient->handles.csisLockHandle)
        message->csInfo = GATT_CSIS_CLIENT_LOCK;
    else
        message->csInfo = GATT_CSIS_CLIENT_RANK;

    /* If status is success, then fill in the data */
    if ((csis_client_status == GATT_CSIS_CLIENT_STATUS_SUCCESS)&&(cfm->valueLength))
    {
        message->sizeValue = cfm->valueLength;
        /* Copy the CS info data */
        memmove(message->value, cfm->value, cfm->valueLength);
    }

    /* Send the confirmation message to app task  */
    CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_READ_CS_INFO_CFM, message);

    /* Free the memory as message has copied the value from confirmation */
    if (cfm->value)
        CsrPmemFree(cfm->value);


    CLEAR_PENDING_FLAG(csisClient->pendingCmd);
}
 
/***************************************************************************
NAME
    handleCsisDiscoverCharCfm

DESCRIPTION
   Handles CSIS discover char confirmation 
*/
static void handleCsisDiscoverCharCfm(GCsisC *const csisClient,
               const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T*const charCfm)
{
    if ((csisClient != NULL) && (charCfm->status == CSR_BT_GATT_RESULT_SUCCESS))
    {
        GATT_CSIS_CLIENT_INFO(" handleCsisDiscoverCharCfm handle (%x) uuid(0x%x)\n ", charCfm->handle, charCfm->uuid[0]);

        /* Verify the char UUID is of interest to CSIP, else ignore */
        if(charCfm->uuid_type == ATT_UUID16)
        {
            switch(charCfm->uuid[0])
            {
                case GATT_CHARACTERISTIC_UUID_SET_IDENTITY_RESOLVING_KEY:
                {
                    csisClient->handles.csisSirkHandle = charCfm->handle;
                }
                break;

                case GATT_CHARACTERISTIC_UUID_COORDINATED_SET_SIZE:
                {
                    csisClient->handles.csisSizeHandle = charCfm->handle;
                }
                break;

                case GATT_CHARACTERISTIC_UUID_COORDINATED_SET_LOCK:
                {
                    csisClient->handles.csisLockHandle = charCfm->handle;
                }
                break;

                case GATT_CHARACTERISTIC_UUID_COORDINATED_SET_RANK:
                {
                    csisClient->handles.csisRankHandle = charCfm->handle;
                }
                break;

                default:
                {
                   /* Unknown Handle, Should not reach here */
                   GATT_CSIS_CLIENT_WARNING("Func:handleCsisDiscoverCharCfm(), UnKnownHandle\n");
                }
                break;
            }

            /* Update end handle for csis characteristics */
            if ((csisClient->handles.csisLockHandle != INVALID_CSIS_HANDLE)  &&
                (charCfm->handle > csisClient->handles.csisLockHandle) &&
                (charCfm->handle < csisClient->csisLockEndHandle))
            {
                csisClient->csisLockEndHandle = charCfm->handle-1;
                GATT_CSIS_CLIENT_DEBUG(" LOCK start hd %x end %x\n", csisClient->handles.csisLockHandle, csisClient->csisLockEndHandle);
            }
            else if ((csisClient->handles.csisSirkHandle!= INVALID_CSIS_HANDLE)  &&
                (charCfm->handle > csisClient->handles.csisSirkHandle) &&
                (charCfm->handle < csisClient->csisSirkEndHandle))
            {
                csisClient->csisSirkEndHandle = charCfm->handle-1;
                GATT_CSIS_CLIENT_DEBUG(" csisSirkEndHandle (%d)\n ", csisClient->csisSirkEndHandle);
                if ((csisClient->csisSirkEndHandle - csisClient->handles.csisSirkHandle) > 1)
                {
                    csisClient->sirkNotificationSupported = CSIS_SIRK_SIZE_NOTIFICATION_SUPPORTED;
                }
            }
            else if ((csisClient->handles.csisSizeHandle!= INVALID_CSIS_HANDLE)  &&
                (charCfm->handle > csisClient->handles.csisSizeHandle) &&
                (charCfm->handle < csisClient->csisSizeEndHandle))
            {
                csisClient->csisSizeEndHandle = charCfm->handle-1;
                GATT_CSIS_CLIENT_DEBUG(" csisClient->csisSizeEndHandle (%d)\n ", csisClient->csisSizeEndHandle);
            }

        }

        if (!charCfm->more_to_come)
        {
            if (csisClient->handles.csisLockHandle != INVALID_CSIS_HANDLE)
            {
                csisDiscoverCharDescriptors(csisClient);
            }
            else if (csisClient->sirkNotificationSupported != CSIS_SIRK_SIZE_NOTIFICATION_SUPPORTED)
            {
                /* Lock characteristics not supported,
                   SIZE ccd and SIRK ccd not supported */
                sendCsisClientInitCfm(csisClient, charCfm->status,
                                    CSIS_CHECK_SIZE_SUPPORT(csisClient), CSIS_LOCK_NOT_SUPPORTED);
            }
            else
            {
                GATT_CSIS_CLIENT_DEBUG("LOCK supported \n");
                /* SIRK characteristics not supported,
                   LOCK supported */
                csisDiscoverLockCharDescriptors(csisClient);
            }
        }
    }
    else
    {
        GATT_CSIS_CLIENT_PANIC("CSIS_CLIENT is NULL\n");
    }
}

/***************************************************************************
NAME
    handleCsisWriteResponseCfm

DESCRIPTION
   Handles csis write response confirmation 
*/
static void handleCsisWriteResponseCfm(GCsisC *const csisClient,
              const CsrBtGattWriteCfm *const writeCfm)
{
    if (csisClient != NULL)
    {
        if (csisClient->pendingCmd == CSIS_CLIENT_WRITE_PENDING_LOCK)
        {
            sendCsisClientWriteLockCfm(csisClient, writeCfm->resultCode);
        }
        else
        {
            sendCsisClientNotificationCfm(csisClient, writeCfm->resultCode, writeCfm->handle);
        }
        
        CLEAR_PENDING_FLAG(csisClient->pendingCmd);
    }    
}

/***************************************************************************
NAME
    handleCsisClientNotificationInd

DESCRIPTION
   Handles csis notification ind from peer
*/
static void handleCsisClientNotificationInd(GCsisC *const csisClient,
              const CsrBtGattClientNotificationInd *const ind)
{  
    if ((csisClient != NULL) && ((csisClient->lockNotificationEnabled)||
        (csisClient->sirkNotificationEnabled) ||
        (csisClient->sizeNotificationEnabled)) && (ind->valueLength))
    {
        GattCsisClientNotificationInd *message = (GattCsisClientNotificationInd *)
                calloc(1,sizeof(GattCsisClientNotificationInd) + ind->valueLength);
        /* Fill in client reference */
        message->srvcHndl = csisClient->srvcElem->service_handle;

        if (ind->valueHandle == csisClient->handles.csisSirkHandle)
            message->csInfo = GATT_CSIS_CLIENT_SIRK;
        else if (ind->valueHandle == csisClient->handles.csisSizeHandle)
            message->csInfo = GATT_CSIS_CLIENT_SIZE;
        else if (ind->valueHandle == csisClient->handles.csisLockHandle)
            message->csInfo = GATT_CSIS_CLIENT_LOCK;

        if (ind->valueLength)
        {
            message->sizeValue = ind->valueLength;
            /* Copy the CS info data */
            memmove(message->value, ind->value, ind->valueLength);
            CsrPmemFree(ind->value);
        }

		/* Send the lock notification value to app task  */
        CsisMessageSend(csisClient->app_task, GATT_CSIS_CLIENT_NOTIFICATION_IND, message);
    }
}


/****************************************************************************/
static void handleCsisGattMsg(void *task, MsgId id, Msg msg)
{
    GCsisC *const csisClient = (GCsisC*)task;

    GATT_CSIS_CLIENT_INFO("handleCsisGattMsg(%d) \n",id);

    switch (id)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            handleCsisDiscoverCharCfm(csisClient,
                                                     (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            handleCsisDiscoverCharDescriptorsRsp(csisClient,
                                                               (const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)msg);
        }
        break;

        case CSR_BT_GATT_WRITE_CFM:
        {
            /* Write/Notification Confirmation */
            handleCsisWriteResponseCfm(csisClient,
                                             (const CsrBtGattWriteCfm*) msg);
        }
        break;

        case CSR_BT_GATT_READ_CFM:
        {
            handleCsisReadResponseCfm(csisClient,
                                (const CsrBtGattReadCfm *) msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_NOTIFICATION_IND:
        {
            handleCsisClientNotificationInd(csisClient,
                                        (const CsrBtGattClientNotificationInd *) msg);
        }
        break;

        case CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM:
        {
             GATT_CSIS_CLIENT_INFO("CSIS CSR_BT_GATT_CLIENT_REGISTER_SERVICE_CFM \n");

        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            GATT_CSIS_CLIENT_WARNING("GCSIS: CSIS Client GattMgr Msg not handled \n");
        }
        break;
    }
}


/***************************************************************************
NAME
    csisClientRegisterForNotification

DESCRIPTION
   Utility function to send registration request for notification 
*/
static void csisClientRegisterForNotification(GCsisC *const csisClient,
                                            uint16 handle, bool enable)
{
    uint8* value = (uint8*)(CsrPmemZalloc(GATT_CSIS_WRITE_CCD_VALUE_LENGTH));

    value[0] = enable ? CSIS_NOTIFICATION_VALUE : 0;
    value[1] = 0;

    if(handle == csisClient->handles.csisLockCcdHandle)
    {
        csisClient->lockNotificationEnabled =  value[0];
    }
    else if(handle == csisClient->handles.csisSirkCcdHandle)
    {
        csisClient->sirkNotificationEnabled = value[0];
    }
    else
    {
        csisClient->sizeNotificationEnabled = value[0];
    }

    CsrBtGattWriteReqSend(csisClient->srvcElem->gattId,
                                    csisClient->srvcElem->cid,
                                    handle,
                                    0,
                                    GATT_CSIS_WRITE_CCD_VALUE_LENGTH,
                                    value);
}

/***************************************************************************
NAME
    csisClientWriteLockReq

DESCRIPTION
   Utility function to reset energy expended
*/
static void csisClientWriteLockReq(GCsisC *const csisClient, bool enableLock)
{
    uint8 *value = (uint8*)(CsrPmemZalloc(sizeof(uint8)));

    value[0] = enableLock ? LOCKED_VALUE: UNLOCKED_VALUE;

    CsrBtGattWriteReqSend(csisClient->srvcElem->gattId,
                          csisClient->srvcElem->cid,
                          csisClient->handles.csisLockHandle,
                          0,
                          sizeof(uint8),
                          value);

}


/***************************************************************************
NAME
    csisClientReadCsInfoReq

DESCRIPTION
   Utility function to read cs info
*/
static void csisClientReadCsInfoReq(GCsisC *const csisClient, uint16 handle)
{
    CsrBtGattReadReqSend(csisClient->srvcElem->gattId,
                         csisClient->srvcElem->cid,
                         handle,
                         0);
}

/***************************************************************************
NAME
    handleCsisInternalMessage

DESCRIPTION
    Handles CSIS internal messages 
*/
static void  handleCsisInternalMessage(void *task, MsgId id, Msg msg)
{
    GCsisC *const csisClient = (GCsisC*)task;
    
    GATT_CSIS_CLIENT_INFO("handleCsisInternalMessage(%d) \n",id);

    if (csisClient != NULL)
    {
        switch(id)
        {
            case CSIS_CLIENT_INTERNAL_MSG_DISCOVER:
            {
                /* Start by discovering Characteristic handles */
                CsrBtGattDiscoverAllCharacOfAServiceReqSend(csisClient->srvcElem->gattId,
                                                                csisClient->srvcElem->cid,
                                                                csisClient->handles.startHandle,
                                                                csisClient->handles.endHandle);

                SET_PENDING_FLAG(CSIS_CLIENT_INIT_PENDING, csisClient->pendingCmd);
            }
            break;

            case CSIS_CLIENT_INTERNAL_MSG_NOTIFICATION_REQ:
            {
                csisClientRegisterForNotification(csisClient,
                        ((const CsisClientInternalMsgNotificationReq*)msg)->handle,
                        ((const CsisClientInternalMsgNotificationReq*)msg)->enable);
                SET_PENDING_FLAG(CSIS_CLIENT_WRITE_PENDING_NOTIFICATION, csisClient->pendingCmd);
            }
            break;

            case CSIS_CLIENT_INTERNAL_MSG_READ_CS_INFO_REQ:
            {
                csisClientReadCsInfoReq(csisClient,
                       ((const CsisClientInternalMsgReadCsInfoReq*)msg)->handle);
                SET_PENDING_FLAG(CSIS_CLIENT_READ_PENDING_CS_INFO_REQ, csisClient->pendingCmd);
            }
            break;

            case CSIS_CLIENT_INTERNAL_MSG_WRITE_LOCK_REQ:
            {
                csisClientWriteLockReq(csisClient,
                        ((const CsisClientInternalMsgWriteLockReq*)msg)->enableLock);
                SET_PENDING_FLAG(CSIS_CLIENT_WRITE_PENDING_LOCK, csisClient->pendingCmd);
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                GATT_CSIS_CLIENT_WARNING("Unknown Message received from Internal To lib \n");
            }
            break;
        }
    }
}


/****************************************************************************/
void gattCsisClientMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *message = NULL;
    GattCsisClient *inst = *((GattCsisClient **)gash);

    if (CsrSchedMessageGet(&eventClass, &message))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                CsrBtGattPrim *id = message;
                GCsisC *csisClient = (GCsisC *) GetServiceClientByGattMsg(&inst->serviceHandleList, message);
                void *msg = GetGattManagerMsgFromGattMsg(message, id);
                if (csisClient)
                {
                    handleCsisGattMsg(csisClient, *id, msg);
                }

                if (msg != message)
                {
                    CsrPmemFree(msg);
                    msg = NULL;
                }
            }
                break;
            case CSIS_CLIENT_PRIM:
            {
                GattCsisInternalMessageId *id = (GattCsisInternalMessageId *)message;
                GCsisC *csisClient = (GCsisC *) GetServiceClientByServiceHandle(message);
                if (csisClient)
                {
                    handleCsisInternalMessage(csisClient, *id, message);
                }
            }
                break;
            default:
                GATT_CSIS_CLIENT_WARNING("GCSIS: Client Msg not handled\n ");
        }
                SynergyMessageFree(eventClass, message);
    }
}

