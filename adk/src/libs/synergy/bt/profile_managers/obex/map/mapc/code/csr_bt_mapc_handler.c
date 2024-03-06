/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_MAPC_MODULE

#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_private_lib.h"
#endif /* EXCLUDE_CSR_BT_SD_MODULE */

#include "csr_bt_util.h"
#include "csr_bt_mapc_handler.h"
#include "csr_bt_mapc_sef.h"
#include "csr_bt_mapc_private_prim.h"
#include "csr_log_text_2.h"

#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_obex_streams.h"
#include "csr_bt_obex_util.h"
#include "csr_streams.h"
#endif


#ifdef CSR_LOG_ENABLE
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtMapcLth);
#endif

static const MapcStateHandlerType mapcStateHandlers[CSR_BT_MAPC_PRIM_DOWNSTREAM_COUNT] =
{
    CsrBtMapcGetInstanceIdsReqHandler,                    /* CSR_BT_MAPC_GET_INSTANCE_IDS_REQ */
    CsrBtMapcConnectReqHandler,                           /* CSR_BT_MAPC_CONNECT_REQ */
    CsrBtMapcCancelConnectReqHandler,                     /* CSR_BT_MAPC_CANCEL_CONNECT_REQ */
    CsrBtMapcDisconnectReqHandler,                        /* CSR_BT_MAPC_DISCONNECT_REQ */
    CsrBtMapcSelectMasInstanceResHandler,                 /* CSR_BT_MAPC_SELECT_MAS_INSTANCE_RES */
    CsrBtMapcSetFolderReqHandler,                         /* CSR_BT_MAPC_SET_FOLDER_REQ */
    CsrBtMapcSetBackFolderReqHandler,                     /* CSR_BT_MAPC_SET_BACK_FOLDER_REQ */
    CsrBtMapcSetRootFolderReqHandler,                     /* CSR_BT_MAPC_SET_ROOT_FOLDER_REQ */
    CsrBtMapcGetFolderListingReqHandler,                  /* CSR_BT_MAPC_GET_FOLDER_LISTING_REQ */
    CsrBtMapcGetFolderListingResHandler,                  /* CSR_BT_MAPC_GET_FOLDER_LISTING_RES */
    CsrBtMapcGetMessageListingReqHandler,                 /* CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ */
    CsrBtMapcGetMessageListingResHandler,                 /* CSR_BT_MAPC_GET_MESSAGE_LISTING_RES */
    CsrBtMapcGetMessageReqHandler,                        /* CSR_BT_MAPC_GET_MESSAGE_REQ */
    CsrBtMapcGetMessageResHandler,                        /* CSR_BT_MAPC_GET_MESSAGE_RES */
    CsrBtMapcSetMessageStatusReqHandler,                  /* CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ */
    CsrBtMapcPushMessageReqHandler,                       /* CSR_BT_MAPC_PUSH_MESSAGE_REQ */
    CsrBtMapcPushMessageResHandler,                       /* CSR_BT_MAPC_PUSH_MESSAGE_RES */
    CsrBtMapcUpdateInboxReqHandler,                       /* CSR_BT_MAPC_UPDATE_INBOX_REQ */
    CsrBtMapcAbortReqHandler,                             /* CSR_BT_MAPC_ABORT_REQ */
    CsrBtMapcNotificationRegistrationReqHandler,          /* CSR_BT_MAPC_NOTIFICATION_REGISTRATION_REQ */
    CsrBtMapcEventNotificationResHandler,                 /* CSR_BT_MAPC_EVENT_NOTIFICATION_RES */
    CsrBtMapcSecurityInReqHandler,                        /* CSR_BT_MAPC_SECURITY_IN_REQ */
    CsrBtMapcSecurityOutReqHandler,                       /* CSR_BT_MAPC_SECURITY_OUT_REQ */
    CsrBtMapcRegisterQIDReqHandler,                       /* CSR_BT_MAPC_REGISTER_QID_REQ */
    CsrBtMapcGetMasInstanceInformationReqHandler,         /* CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_REQ */
    CsrBtMapcGetConversationListingReqHandler,            /* CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ */
    CsrBtMapcGetConversationListingResHandler,            /* CSR_BT_MAPC_GET_CONVERSATION_LISTING_RES */
    CsrBtMapcGetOwnerStatusReqHandler,                    /* CSR_BT_MAPC_GET_OWNER_STATUS_REQ */
    CsrBtMapcSetOwnerStatusReqHandler,                    /* CSR_BT_MAPC_SET_OWNER_STATUS_REQ */
    CsrBtMapcSetNotificationFilterReqHandler,             /* CSR_BT_MAPC_SET_NOTIFICATION_FILTER_REQ */
};

static const MapcStateHandlerType mapcPrivateReqHandlers[CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_COUNT] =
{
    CsrBtMapcAddNotiReqHandler,                           /* CSR_BT_MAPC_ADD_NOTI_REQ */
    CsrBtMapcRemoveNotiReqHandler,                        /* CSR_BT_MAPC_REMOVE_NOTI_REQ */
    CsrBtMapcServiceCleanupHandler,                       /* CSR_BT_MAPC_SERVICE_CLEANUP */
};

static const MapcStateHandlerType mapcPrivateCfmHandlers[CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_COUNT] =
{
    CsrBtMapcAddNotiCfmHandler,                           /* CSR_BT_MAPC_ADD_NOTI_CFM */
    CsrBtMapcRemoveNotiCfmHandler,                        /* CSR_BT_MAPC_REMOVE_NOTI_CFM */
};


#ifdef CSR_BT_GLOBAL_INSTANCE
static uint8 num_mapc_inst_registered = 0;
static MapcInstanceData csrBtMapcInstData[NUM_MAPC_INST];
#endif


void CsrBtMapcHandler(void **gash);
void CsrBtMapcInit(void **gash);

#ifdef CSR_STREAMS_ENABLE
static void csrBtMapcStreamDataCfmHandler(void *inst, CsrBtCmDataCfm **cfm);
static void csrBtMapcStreamDataIndHandler(void *inst, CsrBtCmDataInd **ind);
#endif


static void csrBtMapcInitCommon(MapcInstanceData *pInst)
{
    pInst->obexClientInst = ObexUtilInit(pInst->mapcInstanceId, pInst, CSR_BT_MAPC_CLIENT_INST_ID);

#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecInLevel(&pInst->secIncoming, CSR_BT_SEC_DEFAULT,
                CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_INCOMING,
                CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_INCOMING,
                CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);

    CsrBtScSetSecOutLevel(&pInst->secOutgoing, CSR_BT_SEC_DEFAULT,
                CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_OUTGOING,
                CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_OUTGOING,
                CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#endif 

#ifndef EXCLUDE_CSR_BT_SD_MODULE
    /* Tell the SD that it must look for the OBEX_MESSAGE_ACCESS_SERVER_UUID service,
       when it perform a SD_READ_AVAILABLE_SERVICE_REQ                          */
    CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_OBEX_MESSAGE_ACCESS_SERVER_UUID);
    CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_OBEX_MESSAGE_ACCESS_PROFILE_UUID);
#endif /* EXCLUDE_CSR_BT_SD_MODULE */

    /* OBEX Server part of MAPC Client */
    pInst->maxFrameSize = CSR_BT_MAPC_PROFILE_DEFAULT_MTU_SIZE;

    if (pInst->mapcInstanceId == CSR_BT_MAPC_IFACEQUEUE)
    {
        /* This is the MAPC manager */
        pInst->mapcInstances = CsrPmemZalloc(sizeof(CsrBtMapcInstancePool));


        CsrBtCmContextRegisterReqSend(CSR_BT_MAPC_IFACEQUEUE, MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID);
#ifndef EXCLUDE_CSR_BT_GOEP_20_MODULE
        CsrBtCmContextl2caRegisterReqSend(CSR_BT_MAPC_IFACEQUEUE,
                                      CSR_BT_ASSIGN_DYNAMIC_PSM,
                                      L2CA_MODE_MASK_ENHANCED_RETRANS,
                                      0, /*flags*/
                                      MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID);
#endif /* EXCLUDE_CSR_BT_GOEP_20_MODULE */
    }
    CSR_LOG_TEXT_REGISTER(&CsrBtMapcLth, "MCE", 0, NULL);
}

void CsrBtMapcInit(void **gash)
{
    MapcInstanceData *pInst;
    CsrBtMapcRegisterQidReq *prim;

#ifdef CSR_BT_GLOBAL_INSTANCE
    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CsrBtMapcInit (%d)", num_mapc_inst_registered));

    if (num_mapc_inst_registered >= NUM_MAPC_INST)
    {
        CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, "num_mapc_inst_registered >= NUM_MAPC_INST");
    }

    *gash = &csrBtMapcInstData[num_mapc_inst_registered];
    num_mapc_inst_registered++;
#else
    /* allocate instance data */
    *gash                           = (MapcInstanceData *) CsrPmemZalloc(sizeof(MapcInstanceData));
#endif
    pInst                           = *gash;
    pInst->mapcInstanceId           = CsrSchedTaskQueueGet();

    /* init the instance data */
    csrBtMapcInitCommon(pInst);

    prim                  = CsrPmemAlloc(sizeof(CsrBtMapcRegisterQidReq));
    prim->type            = CSR_BT_MAPC_REGISTER_QID_REQ;
    prim->mapcInstanceId  = pInst->mapcInstanceId;
    CsrBtMapcMessagePut(CSR_BT_MAPC_IFACEQUEUE, prim);
}


static void csrBtMapcCmMessageHandler(MapcInstanceData *pInst, void **msg)
{
    CsrUint8    id;
    CsrBool     validMsg = FALSE;
    CsrUint32   connectionId = 0;

    CsrPrim *primtype           = *msg; 
#ifdef CSR_TARGET_PRODUCT_VM
    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "csrBtMapcCmMessageHandler MESSAGE:CsrBtCmPrim:%d instance = %p, client = %p", *primtype, pInst, pInst->obexClientInst));
#else
    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "csrBtMapcCmMessageHandler type = 0x%x, instance = %p, client = %p", *primtype, pInst, pInst->obexClientInst));
#endif
    
    CSR_UNUSED(primtype);

    if (ObexUtilGetInstIdentifierFromCmUpstreamMessage(*msg, &id))
    {    
        CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "csrBtMapcCmMessageHandler id = 0x%x", id)); 
        if (id == MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID)
        {
            CsrPrim *type           = *msg;
            validMsg = TRUE;
            if (*type == CSR_BT_CM_REGISTER_CFM)
            {
                CsrBtCmRegisterCfm* cfm = (CsrBtCmRegisterCfm*)*msg;
                pInst->mnsServerChannel = cfm->serverChannel;
                CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CSR_BT_CM_REGISTER_CFM (res = 0x%x, sup = 0x%x, serverChannel = %d)", cfm->resultCode, cfm->resultSupplier, cfm->serverChannel)); 
            }
            else if (*type == CSR_BT_CM_L2CA_REGISTER_CFM)
            {
                CsrBtCmL2caRegisterCfm* cfm = (CsrBtCmL2caRegisterCfm*)*msg;
                pInst->mnsPsm = cfm->localPsm;
                csrBtMapcRegisterMnsRecord(pInst);
                CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CSR_BT_CM_L2CA_REGISTER_CFM (res = 0x%x, sup = 0x%x, psm = %x)", cfm->resultCode, cfm->resultSupplier, cfm->localPsm)); 

            }
            else if (*type == CSR_BT_CM_SDS_EXT_REGISTER_CFM)
            {
                /* Nothing to be done */
                CsrBtCmSdsExtRegisterCfm* sds_cfm = (CsrBtCmSdsExtRegisterCfm*)*msg;
                CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CSR_BT_CM_SDS_EXT_REGISTER_CFM (res = 0x%x, sup = 0x%x, context = 0x%x", sds_cfm->resultCode, sds_cfm->resultSupplier, sds_cfm->context)); 
                CSR_UNUSED(sds_cfm);
            }
            else
            {
                validMsg = FALSE;
            }
        }
        else if (id == CSR_BT_MAPC_CLIENT_INST_ID)
        {
            if (ObexUtilCmMsgHandler(pInst->obexClientInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION)
            { /* This message is handled by the common OBEX library */
                validMsg = TRUE;
            }
        }
        else if (id >= (CSR_BT_MAPC_CLIENT_INST_ID + 1))
        {
            MapcNotiService_t *notiService = NOTI_SERVICE_LIST_GET_INSTID(&pInst->notiServiceList, id);
            if (notiService && (ObexUtilCmMsgHandler(notiService->obexInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
            { /* This message is handled by the common OBEX library */
                validMsg = TRUE;
            }
        }
    }    
#ifdef CSR_STREAMS_ENABLE
    else 
    { 
        /* For stream data messages retrieved there is no context information available hence these messages are
         * processed based on the connection identifier of the stream, When a connection is trying to be established
         * with the peer side and when the connection identifier matches*/
        if(ObexUtilGetConnectionIdFromCmUpstreamMessage(*msg, &connectionId))
        {
            /* if the connection id is not found then this is a fresh connection as the mas connection id is only 
             * set after receiving the MAP connection confirmation hence the check for 0 */
            CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "csrBtMapcCmMessageHandler connid = %x, btcon = %x, masConId = %x", connectionId, pInst->masConnId));
            if((pInst->masConnId == 0) || (connectionId == pInst->masConnId))
            {
                if (ObexUtilCmMsgHandler(pInst->obexClientInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION)
                {/* This message is handled by the common OBEX library */
                    validMsg = TRUE;
                }
            }
            else 
            {
                MapcNotiService_t *notiService = NULL;
                notiService = ((MapcNotiService_t *)CsrCmnListSearchOffsetUint32((&pInst->notiServiceList), offsetof(MapcNotiService_t, connId), connectionId));

                /* if the connection id is not found then this is a fresh incoming connection and hence connection id is not set on the 
                 * the server side */
                if(notiService == NULL)
                {
                    notiService = ((MapcNotiService_t *)CsrCmnListSearchOffsetUint32((&pInst->notiServiceList), offsetof(MapcNotiService_t, connId), 0));
                }

                if (notiService && (ObexUtilCmMsgHandler(notiService->obexInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
                { /* This message is handled by the common OBEX library */

                    validMsg = TRUE;
                }
            }
        }
    }
#endif /* CSR_STREAMS_ENABLE */    
    if (!validMsg)
    {
        /* State/Event ERROR! */
        CsrBtMapcPrim *type;
        type = (CsrBtMapcPrim*) *msg;
        CSR_UNUSED(type);
        CsrGeneralException(CsrBtMapcLth,
                            0,
                            CSR_BT_CM_PRIM,
                            *type,
                            0,
                            "MAPC - invalid CM message");
        CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, *msg);
    }
}


#ifdef CSR_STREAMS_ENABLE
static void csrBtMapcStreamDataCfmHandler(void *inst, CsrBtCmDataCfm **cfm)
{
    MapcInstanceData *pInst = inst;
    csrBtMapcCmMessageHandler(pInst, (void**)cfm);
}

static void csrBtMapcStreamDataIndHandler(void *inst, CsrBtCmDataInd **ind)
{
    MapcInstanceData *pInst = inst;
    csrBtMapcCmMessageHandler(pInst, (void**)ind);
}
#endif /* CSR_STREAMS_ENABLE */


void CsrBtMapcHandler(void **gash)
{
    CsrUint16        eventClass=0;
    void            *msg   = NULL;
    MapcInstanceData *pInst = *gash;

    CsrSchedMessageGet(&eventClass, &msg);

    switch(eventClass)
    {
        case CSR_BT_MAPC_PRIM:
        {
            CsrBtMapcPrim *type;
            type = (CsrBtMapcPrim*) msg;
#ifdef CSR_TARGET_PRODUCT_VM
            CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CsrBtMapcHandler MESSAGE:CsrBtMapcPrim:0x%x", *type));
#endif
            if (*type < CSR_BT_MAPC_PRIM_DOWNSTREAM_COUNT &&
                (mapcStateHandlers[*type] != NULL) &&
                (mapcStateHandlers[*type](pInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
            {
                ;
            }

            else 
            if (*type >= CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_LOWEST &&
                    *type <= CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_HIGHEST &&
                    (mapcPrivateReqHandlers[*type - CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_LOWEST] != NULL) &&
                    (mapcPrivateReqHandlers[*type - CSR_BT_MAPC_PRIM_DOWNSTREAM_PRIVATE_LOWEST](pInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
            {
                ;
            }
            else 
            if (*type >= CSR_BT_MAPC_PRIM_UPSTREAM_PRIVATE_LOWEST &&
                    *type <= CSR_BT_MAPC_PRIM_UPSTREAM_PRIVATE_HIGHEST &&
                    (mapcPrivateCfmHandlers[*type - CSR_BT_MAPC_PRIM_UPSTREAM_PRIVATE_LOWEST] != NULL) &&
                    (mapcPrivateCfmHandlers[*type - CSR_BT_MAPC_PRIM_UPSTREAM_PRIVATE_LOWEST](pInst, msg) != CSR_BT_OBEX_UTIL_STATUS_EXCEPTION))
            {
                ;
            }
            else
            {
                /* State/Event ERROR! */
                CsrGeneralException(CsrBtMapcLth,
                                    0,
                                    eventClass,
                                    *type,
                                    0,
                                    "MAPC");
                CsrBtMapcFreeDownstreamMessageContents(eventClass, msg);
            }
            break;
        }
#ifdef CSR_STREAMS_ENABLE
        case MESSAGE_MORE_SPACE:
        {        
            CsrUint8 protocol = CSR_BT_CONN_ID_IS_L2CA(ObexUtilGetConnId(pInst->obexClientInst))?L2CAP_ID:RFCOMM_ID;
            CsrBtObexMessageMoreSpaceHandler(pInst, csrBtMapcStreamDataCfmHandler, (MessageMoreSpace *)msg, protocol);
            msg = NULL; /*Message already freed*/
            break;
        }
        case MESSAGE_MORE_DATA:
        {
            CsrUint8 protocol = CSR_BT_CONN_ID_IS_L2CA(ObexUtilGetConnId(pInst->obexClientInst))?L2CAP_ID:RFCOMM_ID;
            CsrBtObexMessageMoreDataHandler(pInst, csrBtMapcStreamDataIndHandler, (MessageMoreData *)msg, protocol);
            msg = NULL; /*Message already freed*/
            break;
        }
#endif

        case CSR_BT_CM_PRIM :
        {
            csrBtMapcCmMessageHandler(pInst, &msg);
            break;
        }
        case CSR_SCHED_PRIM:
        {
            break;
        }
        default:
        {/* Unknown event type!?!? */
            CsrGeneralException(CsrBtMapcLth, 0, eventClass, 0, 0, "MAPC Unknown event type");
            break;
        }
    }
    SynergyMessageFree(eventClass, msg);
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
    This function is called by the scheduler to perform a graceful shutdown
    of a scheduler task.
    This function must:
    1)    empty the input message queue and free any allocated memory in the
        messages.
    2)    free any instance data that may be allocated.
****************************************************************************/
void CsrBtMapcDeinit(void ** gash)
{
    CsrUint16        eventClass=0;
    void            *msg=NULL;
    CsrBool          lastMsg     = FALSE;
    MapcInstanceData *pInst      = *gash;

    while (!lastMsg)
    {
        lastMsg = (CsrBool)(!CsrSchedMessageGet(&eventClass, &msg));

        if (!lastMsg)
        {
            switch (eventClass)
            {
                case CSR_BT_MAPC_PRIM:
                {
                    CsrBtMapcFreeDownstreamMessageContents(eventClass, msg);
                    break;
                }

                case CSR_BT_CM_PRIM:
                {
                    CsrBtCmFreeUpstreamMessageContents(eventClass, msg);
                    break;
                }
                default:
                {
                    CsrGeneralException(CsrBtMapcLth, 0,eventClass, 0, 0, "MAPC");
                    break;
                }
            }
            CsrPmemFree (msg);
        }
    }

    if (pInst->obexClientInst)
    {
        ObexUtilDeinit(&pInst->obexClientInst);
    }
    while (pInst->mapcInstances)
    {
        CsrBtMapcInstancePool   *temp = pInst->mapcInstances;
        pInst->mapcInstances = pInst->mapcInstances->next;
        
        CsrPmemFree(temp);
    }


#ifdef CSR_BT_GLOBAL_INSTANCE
    CsrMemSet(pInst, 0, sizeof(MapcInstanceData));
    if (num_mapc_inst_registered)
        num_mapc_inst_registered--;
#else
    CsrPmemFree(pInst);
#endif

    *gash = NULL;
}
#endif    /* ENABLE_SHUTDOWN */

#endif /*EXCLUDE_CSR_BT_MAPC_MODULE*/
