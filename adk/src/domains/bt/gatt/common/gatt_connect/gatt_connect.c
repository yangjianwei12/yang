/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    gatt_connect
    \brief      Application support for GATT connect/disconnect
*/

#include "gatt_connect.h"
#include "gatt_connect_observer.h"
#include "gatt_connect_list.h"
#include "gatt_connect_mtu.h"
#ifdef USE_SYNERGY
#include <csr_bt_cm_prim.h>
#include <cm_lib.h>
#include <gatt_lib.h>
#include <gatt_handler_db.h>
#else
#include <gatt_manager.h>
#endif
#include <gatt.h>
#include <logging.h>

#include <panic.h>
#include <l2cap_prim.h>

#include <connection_manager.h>
#ifdef ENABLE_LE_DEBUG_SECONDARY
#include <le_debug_secondary.h>
#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(gatt_connect_message_t)

#ifdef USE_SYNERGY
#define GATT_RESULT_SUCCESS_ID         CSR_BT_GATT_RESULT_SUCCESS
#define GATT_RESULT_LINK_TRANSFERRED   CSR_BT_GATT_RESULT_LINK_TRANSFERRED
#else
#define GATT_RESULT_SUCCESS_ID         gatt_status_success
#define GATT_RESULT_LINK_TRANSFERRED   (0)  /* @ToDo: Not yet supported in legacy library */ 
#endif

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(APP_GATT, APP_GATT_MESSAGE_END)

#endif

static void gattConnect_MessageHandler(Task task, MessageId id, Message msg);

#ifdef USE_SYNERGY
gatt_connect_task_data gatt_task_data = {.task = gattConnect_MessageHandler};
#define GattConnectGetTaskData()     (&gatt_task_data)
#define GattConnectGetTask()         (&gatt_task_data.task)

/*! Earbud GATT database, for the required GATT and GAP servers. */
extern uint16 gattDatabase[];
#else
TaskData gatt_task_data = {gattConnect_MessageHandler};
#endif
Task gatt_connect_init_task;

#ifdef USE_SYNERGY
static void gattConnect_HandleConnectInd(CsrBtGattConnectInd* ind)
{
    gatt_connect_task_data *sp = GattConnectGetTaskData();
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(ind->btConnId);
    
    DEBUG_LOG("gattConnect_HandleConnectInd, cid 0x%04x ,result 0x%04x, supplier 0x%04x", ind->btConnId, ind->resultCode, ind->resultSupplier);
    
    /* Ignore the indication if we already have a connection instance 
       to avoid repeating the MTU exchange, etc. */
    if(connection == NULL)
    {
        if(ind->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
        {
            connection = GattConnect_CreateConnection(ind->btConnId);
            sp->peer_address = ind->address;

            if (connection)
            {
               GattConnect_SetMtu(connection, ind->mtu);
               GattConnect_SendExchangeMtuReq(sp->gatt_id, ind->btConnId);
               GattConnect_ObserverNotifyOnConnection(ind->btConnId);
            }
        }
    }
}

#else
static void gattConnect_HandleConnectInd(GATT_MANAGER_CONNECT_IND_T* ind)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(ind->cid);
    
    DEBUG_LOG("gattConnect_HandleConnectInd, cid 0x%04x flags 0x%04d", ind->cid, ind->flags);
    
    /* Ignore the indication if we already have a connection instance 
       to avoid repeating the MTU exchange, etc. */
    if(connection == NULL)
    {
        if(L2CA_CONFLAG_IS_LE(ind->flags))
        {
            connection = GattConnect_CreateConnection(ind->cid);
            
            if(connection)
            {
                GattConnect_SetMtu(connection, ind->mtu);
                GattConnect_SendExchangeMtuReq(&gatt_task_data, ind->cid);
                GattConnect_ObserverNotifyOnConnection(ind->cid);
            }
        }
    }
    
    /* Accept connection if we have an available connection instance */
    GattManagerConnectResponse(ind->cid, ind->flags, connection ? TRUE : FALSE);
}
#endif

#ifdef USE_SYNERGY
static void gattConnect_HandleFlatDbRegisterWithGattCfm(CsrBtGattFlatDbRegisterCfm* cfm)
{
    DEBUG_LOG("gattConnect_HandleFlatDbRegisterWithGattCfm result:0x%04x supplier:0x%04x",cfm->resultCode, cfm->resultSupplier);

    PanicFalse(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS);

    if(gatt_connect_init_task)
    {
        MessageSend(gatt_connect_init_task, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM, NULL);
    }
}
#endif

static void gattConnect_UpdateDisconnectReasonCode(gatt_connection_t *connection, uint16 reason_code)
{
    DEBUG_LOG("gattConnect_UpdateDisconnectReasonCode %d", reason_code);

    if (connection != NULL)
    {
        switch (reason_code)
        {
            case GATT_RESULT_SUCCESS_ID:
                connection->disconnect_reason_code = gatt_connect_disconnect_reason_success;
            break;

#ifdef ENABLE_LE_HANDOVER
            case GATT_RESULT_LINK_TRANSFERRED:
                connection->disconnect_reason_code = gatt_connect_disconnect_reason_link_transferred;
            break;
#endif

            default:
                connection->disconnect_reason_code = gatt_connect_disconnect_reason_other;
            break;
        }
    }
}

#ifdef USE_SYNERGY
static void gattConnect_HandleDisconnectInd(CsrBtGattDisconnectInd* ind)
{
    uint32 cid = ind->btConnId;
    gatt_connection_t *connection = GattConnect_FindConnectionFromCid(cid);

    gattConnect_UpdateDisconnectReasonCode(connection, ind->reasonCode);
#else
static void gattConnect_HandleDisconnectInd(GATT_MANAGER_DISCONNECT_IND_T* ind)
{
    uint16 cid = ind->cid;
    gatt_connection_t *connection = GattConnect_FindConnectionFromCid(cid);

    gattConnect_UpdateDisconnectReasonCode(connection, ind->status);
#endif
    GattConnect_ObserverNotifyOnDisconnection(cid);
    GattConnect_DestroyConnection(cid);
}

#ifdef USE_SYNERGY
static void gattConnect_HandleRegisterWithGattCfm(CsrBtGattRegisterCfm* cfm)
{
    gatt_connect_task_data *sp = GattConnectGetTaskData();

    DEBUG_LOG("gattConnect_HandleRegisterWithGattCfm, status:0x%04x supplier:0x%04x", cfm->resultCode, cfm->resultSupplier);

    PanicFalse(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS);

    /* Register with Synergy GATT for Gatt connect/disconnect notifications */
    CsrBtGattSetEventMaskReqSend(cfm->gattId, CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_GAP);
    sp->gatt_id = cfm->gattId;

}
#else
static void gattConnect_HandleRegisterWithGattCfm(GATT_MANAGER_REGISTER_WITH_GATT_CFM_T* cfm)
{
    PanicFalse(cfm->status == gatt_manager_status_success);
    if(gatt_connect_init_task)
    {
        MessageSend(gatt_connect_init_task, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM, NULL);
    }
}
#endif

static void gattConnect_HandleConManagerConnection(const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
#ifdef USE_SYNERGY
    DEBUG_LOG("gattConnect_HandleConManagerConnection, incoming %d",ind->incoming);
#else
    if (ConManagerIsTpAclLocal(&ind->tpaddr))
    {
        GattManagerConnectAsCentral(&gatt_task_data, &ind->tpaddr.taddr, gatt_connection_ble_master_directed, TRUE);
    }
#endif
}

static void gattConnect_HandleConManagerDisconnection(const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    UNUSED(ind);
#ifndef USE_SYNERGY
    GattManagerCancelConnectAsCentral(&gatt_task_data);
#endif
}

static void gattConnect_DisconnectRequestedResponse(gatt_cid_t cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    if (connection)
    {
        DEBUG_LOG("gattConnect_DisconnectRequestedResponse, cid 0x%04x pending_disconnects %d", cid, connection->pending_disconnects);
        if (connection->pending_disconnects)
        {
            connection->pending_disconnects--;
            if (connection->pending_disconnects == 0)
            {
#ifndef USE_SYNERGY
                /* Synergy library relies on CM ACL close request for disconnection of GATT links */
                GattManagerDisconnectRequest(connection->cid);
#endif /* !USE_SYNERGY */
            }
        }
    }
}

static void gattConnect_HandleConManagerDisconnectRequested(const CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T *ind)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromTpaddr(&ind->tpaddr);
    if (connection)
    {
        connection->pending_disconnects = GattConnect_ObserverGetNumberDisconnectReqCallbacksRegistered();
        DEBUG_LOG("gattConnect_HandleConManagerDisconnectRequested, cid 0x%04x pending_disconnects %d", connection->cid, connection->pending_disconnects);
        if (connection->pending_disconnects)
        {
            GattConnect_ObserverNotifyOnDisconnectRequested(connection->cid, gattConnect_DisconnectRequestedResponse);
        }
        else
        {
#ifndef USE_SYNERGY
            /* Synergy library relies on CM ACL close request for disconnection of GATT links */
            GattManagerDisconnectRequest(connection->cid);
#endif /* !USE_SYNERGY */
        }
    }
}

#ifndef USE_SYNERGY
static void gattConnect_HandleGattManagerConnectAsCentral(const GATT_MANAGER_CONNECT_AS_CENTRAL_CFM_T *cfm)
{
    gatt_connection_t* connection;
    
    if(GattConnect_FindConnectionFromCid(cfm->cid))
    {
        /* We may already have handled this via GATT_MANAGER_CONNECT_IND, in
           which case we don't need to create connection or exchange MTU. */
        return;
    }
    
    connection = GattConnect_CreateConnection(cfm->cid);

    DEBUG_LOG("gattConnect_HandleGattManagerConnectAsCentral, cid 0x%04x flags 0x%04d", cfm->cid, cfm->flags);

    if(connection)
    {
        GattConnect_SetMtu(connection, cfm->mtu);
        GattConnect_SendExchangeMtuReq(&gatt_task_data, cfm->cid);
        GattConnect_ObserverNotifyOnConnection(cfm->cid);
    }
}
#endif

#ifdef USE_SYNERGY
/*! \brief Handle encryption change indications

    If a LE ACL that is registered with GATT is encrypted notify
    the registered GATT observers.

    \param  ind     The change indication to process
*/
static void gattConnect_HandleCmEncryptChangeInd(CsrBtCmEncryptChangeInd *ind)
{
    tp_bdaddr tp_addr = { 0 };
    bool encryption_enabled = (ind->encryptType != CSR_BT_CM_ENC_TYPE_NONE && ind->encryptType != CSR_BT_CM_ENC_TYPE_KEY_REFRESH);

    DEBUG_LOG("gattConnect_HandleCmEncryptChangeInd address %x encrypT_type %u",
              ind->deviceAddr.lap,
              ind->encryptType);

    if (encryption_enabled && (ind->transportType == LE_ACL))
    {
        tp_addr.transport = ind->transportType;
        tp_addr.taddr.type = ind->deviceAddrType;
        BdaddrConvertBluestackToVm(&tp_addr.taddr.addr, &ind->deviceAddr);

        /* find matching connection */
        gatt_connection_t *connection = GattConnect_FindConnectionFromTpaddr(&tp_addr);
        if (connection)
        {
            DEBUG_LOG("gattConnect_HandleCmEncryptChangeInd cid 0x%4x encrypted %d",
                      connection->cid, encryption_enabled);

            connection->encrypted = encryption_enabled;

            GattConnect_ObserverNotifyOnEncryptionChanged(connection->cid, encryption_enabled);
        }
    }
}

static void gattConnect_HandleCmPrim(Message msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *)msg;

    switch (*prim)
    {
        case CSR_BT_CM_SET_EVENT_MASK_CFM:
            break;

        case CSR_BT_CM_ENCRYPT_CHANGE_IND:
            gattConnect_HandleCmEncryptChangeInd((CsrBtCmEncryptChangeInd *)msg);
            break;

        default:
            DEBUG_LOG("gattConnect_HandleCmPrim, Unexpected CM primitive");
            break;
    }

    CmFreeUpstreamMessageContents((void *)msg);
}
#else
/*! \brief Handle encryption change indications

    If a LE ACL that is registered with GATT is encrypted notify
    the registered GATT observers.

    \param  ind     The change indication to process
*/
static void gattConnect_HandleClSmEncryptionChangeInd(CL_SM_ENCRYPTION_CHANGE_IND_T *ind)
{
    DEBUG_LOG("gattConnect_HandleClSmEncryptionChangeInd address %x encrypted %u",
              ind->tpaddr.taddr.addr.lap,
              ind->encrypted);

    if (ind->encrypted && (ind->tpaddr.transport == TRANSPORT_BLE_ACL))
    {
        gatt_connection_t *connection = GattConnect_FindConnectionFromTpaddr(&ind->tpaddr);
        if (connection)
        {
            DEBUG_LOG("gattConnect_HandleClSmEncryptionChangeInd cid 0x%4x encrypted %d",
                      connection->cid, ind->encrypted);

            GattConnect_ObserverNotifyOnEncryptionChanged(connection->cid, ind->encrypted);
        }
    }
}

bool GattConnect_HandleConnectionLibraryMessages(MessageId id, Message message,
                                                 bool already_handled)
{
    switch (id)
    {
        case CL_SM_ENCRYPTION_CHANGE_IND:
            gattConnect_HandleClSmEncryptionChangeInd((CL_SM_ENCRYPTION_CHANGE_IND_T *)message);
            return TRUE;

         default:
             break;
    }

    return already_handled;
}
#endif

static void gattConnect_GattMessageHandler(MessageId id, Message msg)
{
#ifdef USE_SYNERGY
    CsrBtGattPrim *primType = (CsrBtGattPrim *)msg;
    if (id != GATT_PRIM)
        return;

#ifdef ENABLE_LE_DEBUG_SECONDARY
    /*  Drop GATT Prims when secondary earbud is advertising with a modified local IRK in debug mode, To avoid
        routing of these messages to registered clients.*/
    if (LeDebugSecondary_IsSecondaryIRKInUse())
    {
        GattFreeUpstreamMessageContents((void *)msg);
        return;
    }
#endif /* ENABLE_LE_DEBUG_SECONDARY */

    switch(*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            gattConnect_HandleRegisterWithGattCfm((CsrBtGattRegisterCfm*)msg);
        break;
        case CSR_BT_GATT_FLAT_DB_REGISTER_CFM:
            gattConnect_HandleFlatDbRegisterWithGattCfm((CsrBtGattFlatDbRegisterCfm*)msg);
        break;
        case CSR_BT_GATT_DISCONNECT_IND:
            gattConnect_HandleDisconnectInd((CsrBtGattDisconnectInd*)msg);
        break;
        case CSR_BT_GATT_CONNECT_IND:
            gattConnect_HandleConnectInd((CsrBtGattConnectInd*)msg);
        break;
        case CSR_BT_GATT_CLIENT_EXCHANGE_MTU_CFM:
            gattConnect_HandleExchangeMtuCfm((CsrBtGattClientExchangeMtuCfm*)msg);
        break;
        case CSR_BT_GATT_REMOTE_CLIENT_EXCHANGE_MTU_IND:
        {
            gatt_connect_task_data *sp = GattConnectGetTaskData();
            gattConnect_HandleExchangeMtuInd(sp->gatt_id, (CsrBtGattRemoteClientExchangeMtuInd*)msg);
        }
        break;
        case CSR_BT_GATT_MTU_CHANGED_IND:
        {
            DEBUG_LOG("CSR_BT_GATT_MTU_CHANGED_IND: mtu = %d", ((CsrBtGattMtuChangedInd*)msg)->mtu);
        }
        break;
        default:
        break;
    }

    GattFreeUpstreamMessageContents((void *)msg);
#else
    switch(id)
    {
        case GATT_MANAGER_CONNECT_IND:
            gattConnect_HandleConnectInd((GATT_MANAGER_CONNECT_IND_T*)msg);
        break;
        
        case GATT_MANAGER_DISCONNECT_IND:
            gattConnect_HandleDisconnectInd((GATT_MANAGER_DISCONNECT_IND_T*)msg);
        break;
        
        case GATT_EXCHANGE_MTU_IND:
            gattConnect_HandleExchangeMtuInd((GATT_EXCHANGE_MTU_IND_T*)msg);
        break;
        
        case GATT_EXCHANGE_MTU_CFM:
            gattConnect_HandleExchangeMtuCfm((GATT_EXCHANGE_MTU_CFM_T*)msg);
        break;
        
        case GATT_MANAGER_REGISTER_WITH_GATT_CFM:
            gattConnect_HandleRegisterWithGattCfm((GATT_MANAGER_REGISTER_WITH_GATT_CFM_T*)msg);
        break;
        
        case GATT_MANAGER_CONNECT_AS_CENTRAL_CFM:
            gattConnect_HandleGattManagerConnectAsCentral((const GATT_MANAGER_CONNECT_AS_CENTRAL_CFM_T *)msg);
        break;
        default:
        break;        
    }
#endif
}

static void gattConnect_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    gattConnect_GattMessageHandler(id, msg);

    switch(id)
    {

        case CON_MANAGER_TP_CONNECT_IND:
            gattConnect_HandleConManagerConnection((const CON_MANAGER_TP_CONNECT_IND_T *)msg);
        break;
            
        case CON_MANAGER_TP_DISCONNECT_IND:
            gattConnect_HandleConManagerDisconnection((const CON_MANAGER_TP_DISCONNECT_IND_T *)msg);
        break;
        
        case CON_MANAGER_TP_DISCONNECT_REQUESTED_IND:
            gattConnect_HandleConManagerDisconnectRequested((const CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T *)msg);
        break;

#ifdef USE_SYNERGY
        case CM_PRIM:
            gattConnect_HandleCmPrim(msg);
        break;
#endif
        default:
        break;
    }
}

bool GattConnect_Init(Task init_task)
{
#ifdef USE_SYNERGY
    gatt_connect_init_task = init_task;
#else
    UNUSED(init_task);
#endif
    GattConnect_MtuInit();
    GattConnect_ListInit();
    GattConnect_ObserverInit();
#ifdef USE_SYNERGY
    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, &gatt_task_data.task);
    GattRegisterReqSend(&gatt_task_data.task,1234);
    CmSetEventMaskReqSend(&gatt_task_data.task,
                          CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE,
                          CSR_BT_CM_EVENT_MASK_COND_ALL);
#else
    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, &gatt_task_data);
    PanicFalse(GattManagerInit(&gatt_task_data));
#endif
    return TRUE;
}

device_t GattConnect_GetBtDevice(unsigned cid)
{
    bdaddr public_addr;
    device_t device = NULL;

    if (GattConnect_GetPublicAddrFromConnectionId(cid, &public_addr))
    {
        device = BtDevice_GetDeviceForBdAddr(&public_addr);
    }
    return device;
}

device_t GattConnect_GetBtLeDevice(unsigned cid)
{
    tp_bdaddr tpaddr;

    if (GattConnect_FindTpaddrFromCid(cid, &tpaddr))
    {
        return  BtDevice_GetDeviceFromTpAddr(&tpaddr);
    }

    return NULL;
}

bool GattConnect_IsDeviceTypeOfPeer(unsigned cid)
{
    device_t device = GattConnect_GetBtDevice(cid);
    deviceType device_type = DEVICE_TYPE_MAX;

    if (device != NULL)
    {
        device_type = BtDevice_GetDeviceType(device);
    }

    return (device_type == DEVICE_TYPE_EARBUD || device_type == DEVICE_TYPE_SELF);
}

bool GattConnect_IsEncrypted(unsigned cid)
{
    bool is_encrypted = FALSE;
    gatt_connection_t *connection = GattConnect_FindConnectionFromCid(cid);

    if (connection != NULL)
    {
        is_encrypted = connection->encrypted;
    }

    return is_encrypted;
}

#ifndef USE_SYNERGY
bool GattConnect_ServerInitComplete(Task init_task)
{
    gatt_connect_init_task = init_task;
    GattManagerRegisterWithGatt();
    return TRUE;
}
#endif

#ifdef USE_SYNERGY
bool GattConnect_RegisterDB(Task init_task)
{
   gatt_connect_init_task = init_task;

   DEBUG_LOG("GattConnect_RegisterDB");

   /* Register the Flat database */
   GattFlatDbRegisterReqSend(&gatt_task_data.task, 0, GattGetDatabaseSize()/sizeof(uint16), &gattDatabase[0]);
   return TRUE;
}

CsrBtTypedAddr GattConnect_GetPeerBDAddr(void)
{
    gatt_connect_task_data *sp = GattConnectGetTaskData();
    return sp->peer_address;
}
#endif

bool GattConnect_GetTpaddrFromConnectionId(unsigned cid, tp_bdaddr * tpaddr)
{
    return GattConnect_FindTpaddrFromCid(cid, tpaddr);
}

bool GattConnect_GetPublicAddrFromConnectionId(unsigned cid, bdaddr * addr)
{
    bool result = FALSE;
    tp_bdaddr tpaddr = {0};

    if (GattConnect_FindTpaddrFromCid(cid, &tpaddr))
    {
        tp_bdaddr public_tpaddr = {0};

        if (ConManagerResolveTpaddr(&tpaddr, &public_tpaddr))
        {
            *addr = public_tpaddr.taddr.addr;
            result = TRUE;
        }
    }

    return result;
}

unsigned GattConnect_GetConnectionIdFromTpaddr(const tp_bdaddr *tpaddr)
{
    gatt_connection_t *connection = GattConnect_ResolveAndFindConnection(tpaddr);

    return connection != NULL ? connection->cid : INVALID_CID;
}

gatt_connect_disconnect_reason_t GattConnect_GetDisconnectReasonCode(gatt_cid_t cid)
{
    gatt_connect_disconnect_reason_t reason_code = gatt_connect_disconnect_reason_no_valid_connection;
    gatt_connection_t *connection = GattConnect_FindConnectionFromCid(cid);

    if (connection != NULL)
    {
        reason_code = connection->disconnect_reason_code;
    }

    return reason_code;
}

