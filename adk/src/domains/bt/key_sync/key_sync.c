/*!
\copyright  Copyright (c) 2005 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Component handling synchronisation of keys between peers.
*/

#include "key_sync.h"
#include "key_sync_private.h"
#include "key_sync_marshal_defs.h"

#include <peer_signalling.h>
#include <bt_device.h>
#include <system_reboot.h>

#include <device_properties.h>
#include <device_db_serialiser.h>

#include <device_list.h>

#include <logging.h>
#include <connection_abstraction.h>

#include <panic.h>
#include <stdlib.h>
#include <stdio.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(key_sync_messages)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(KEY_SYNC, KEY_SYNC_MESSAGE_END)

#define MAKE_KEY_SYNC_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! \brief Key Sync task data. */
key_sync_task_data_t key_sync;

/*! \brief Handle confirmation of transmission of a marshalled message. */
static void keySync_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);
}

static void keySync_HandleKeySyncConfirmation(const bdaddr* bd_addr, bool synced)
{
    if(!synced)
    {
        DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation sync failure reported by peer");
    }
    else
    {
        if(!appDeviceSetHandsetAddressForwardReq(bd_addr, FALSE))
        {
            DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation FAILED TO CLEAR ADDRESS FWD REQD!");
        }
        else
        {
            key_sync_task_data_t *ks = keySync_GetTaskData();
            DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation cleared ADDRESS FWD REQD");

            DeviceDbSerialiser_SerialiseDevice(BtDevice_GetDeviceForBdAddr(bd_addr));

            if(TaskList_Size(ks->listeners) > 0)
            {
                KEY_SYNC_DEVICE_COMPLETE_IND_T *msg = PanicUnlessMalloc(sizeof(KEY_SYNC_DEVICE_COMPLETE_IND_T));
                msg->bd_addr = *bd_addr;
                TaskList_MessageSendWithSize(ks->listeners, KEY_SYNC_DEVICE_COMPLETE_IND, msg, sizeof(KEY_SYNC_DEVICE_COMPLETE_IND_T));
            }
        }
    }
}

static bool keySync_IsPairedHandset(device_t device)
{
    uint16 flags = 0;

    if (BtDevice_GetDeviceType(device) != DEVICE_TYPE_HANDSET)
    {
        return FALSE;
    }

    if(!Device_GetPropertyU16(device, device_property_flags, &flags))
    {
        return FALSE;
    }

    if((flags & DEVICE_FLAGS_NOT_PAIRED) == DEVICE_FLAGS_NOT_PAIRED)
    {
        return FALSE;
    }

    return TRUE;
}

static bool keySync_ShouldBeSyncedHandset(device_t device)
{
    uint16 flags = 0;

    if (BtDevice_GetDeviceType(device) != DEVICE_TYPE_HANDSET)
    {
        return FALSE;
    }

    if(!Device_GetPropertyU16(device, device_property_flags, &flags))
    {
        return FALSE;
    }

    /* Check for flags which will prevent key sychronisation */
    if((flags & (DEVICE_FLAGS_NOT_PAIRED | DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD)))
    {
        DEBUG_LOG_INFO("keySync_ShouldBeSyncedHandset: Device flags %x prevent synchronisation", flags);
        return FALSE;
    }

    return TRUE;
}

static void keySync_GetPairedHandsetBdaddrs(device_t device, void *data)
{
    key_sync_paired_list_req_t * msg = (key_sync_paired_list_req_t *)data;

    if (keySync_IsPairedHandset(device))
    {
        bdaddr handset_addr = DeviceProperties_GetBdAddr(device);
        msg->bd_addrs[msg->num_handsets++] = handset_addr;

        DEBUG_LOG_ALWAYS("keySync_GetPairedHandsetBdaddrs, device %p, handset bd_addr [%04x,%02x,%06lx]",
                          device, handset_addr.nap, handset_addr.uap, handset_addr.lap);
    }
}

static void keySync_GetPairedHandsetsCount(device_t device, void *data)
{
    uint8 * count = (uint8 *)data;

    if (keySync_IsPairedHandset(device))
    {
        *count = *count + 1;
    }
}

static uint8 marked_for_delete = 0;

static void keySync_CompareDeviceLists(device_t device, void *data)
{
    key_sync_paired_list_req_t * msg = (key_sync_paired_list_req_t *)data;

    /* When comparing lists, we consider ONLY handsets we believe to be already synced.
     * Any of our handsets still awaiting forwarding will be forwarded very soon,
     * so make sure we don't compare or attempt to delete them */
    if (keySync_ShouldBeSyncedHandset(device))
    {
        /* If this current handset is present in the list we received from the peer,
         * then it means our separate TDL records of this handset are already in sync
         * or about to be in sync, so ignore it and move on to the next handset */
        bdaddr current = DeviceProperties_GetBdAddr(device);
        for (unsigned index = 0; index < msg->num_handsets; index++)
        {
            if (BdaddrIsSame(&msg->bd_addrs[index], &current))
            {
                return;
            }
        }

        /* Otherwise mark this handset for deletion,
         * since the peer doesn't have this handset in its TDL at all
         */
        uint16 flags = 0;
        PanicFalse(Device_GetPropertyU16(device,
                                         device_property_flags,
                                         &flags));
        PanicFalse(Device_SetPropertyU16(device,
                                         device_property_flags,
                                         flags | DEVICE_FLAGS_DELETE_ON_BOOT));

        marked_for_delete++;

        DEBUG_LOG_ALWAYS("keySync_CompareDeviceLists, device %p marked for deletion, bd_addr [%04x,%02x,%06lx]",
                          device, current.nap, current.uap, current.lap);
    }
}

static void keySync_SyncDeviceLists(bool is_request)
{
    DEBUG_LOG_FN_ENTRY("keySync_SyncDeviceLists");

    uint8 count = 0;

    /* Here, we send ALL our paired handsets to the peer because as of this instant,
     * we believe they are all already or soon-to-be forwarded/synced.
     *
     * - handsets marked for forwarding will be forwarded very soon and will become synced.
     * - handsets which aren't marked for forwarding are expected to be already synced.
     */
    DeviceList_Iterate(keySync_GetPairedHandsetsCount, &count);

    /* allocate space for the message and begin its assembly:
     * the same structure is used for both the request and confirmation */
    size_t size_data = count * sizeof(bdaddr);
    key_sync_paired_list_req_t* req = PanicUnlessMalloc(sizeof(key_sync_paired_list_req_t) + size_data);
    req->num_handsets = 0;

    if (count)
    {
        DeviceList_Iterate(keySync_GetPairedHandsetBdaddrs, req);
    }

    /* send our list to the peer */
    appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                     req, is_request ? MARSHAL_TYPE_key_sync_paired_list_req_t
                                                     : MARSHAL_TYPE_key_sync_paired_list_cfm_t);
}

/*! \brief Handle incoming marshalled messages from peer key sync component. */
static void keySync_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE_key_sync_req_t:
        {
            key_sync_req_t* req = (key_sync_req_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd rx key to add");
            /* update local PDL, note the API uses 16-bit words */
            ConnectionSmAddAuthDevice(keySync_GetTask(), &req->bd_addr, req->bits.trusted, TRUE,
                                      req->link_key_type, req->size_link_key / sizeof(uint16), (uint16*)req->link_key);
        }
        break;

        case MARSHAL_TYPE_key_sync_paired_device_req_t:
        {
            key_sync_paired_device_req_t* req = (key_sync_paired_device_req_t*)ind->msg;
#ifdef USE_SYNERGY
            uint8 offset = 0;
            CsrBtDeviceAddr deviceAddr = { 0 };

            BdaddrConvertVmToBluestack(&deviceAddr, &req->bd_addr);
#endif /* USE_SYNERGY */
            typed_bdaddr taddr = {0};
            taddr.type = TYPED_BDADDR_PUBLIC;
            taddr.addr = req->bd_addr;

            device_t device = BtDevice_GetDeviceForBdAddr(&taddr.addr);
            if (device)
            {
                uint16 flags = 0;
                Device_GetPropertyU16(device, device_property_flags, &flags);
                if ((flags & DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS) == 0)
                {
                    PanicFalse(BtDevice_SetFlags(device, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS));
                    ConnectionSmDeleteAuthDeviceReq(taddr.type, &taddr.addr);
                }
                else
                {
                    /* Removal of device has been already requested. Ignore */
                }
            }

            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd rx paired device to add");
#ifdef USE_SYNERGY
            if (req->size_data >= sizeof(CsrBtTdDbLeKeys))
            {
                offset += sizeof(CsrBtTdDbLeKeys);
            }

            if (req->size_data - offset == sizeof(CsrBtTdDbBredrKey))
            {
                CmWriteBredrKeysReqSend(keySync_GetTask(),
                                        CSR_BT_ADDR_PUBLIC,
                                        &deviceAddr,
                                        (CsrBtCmKey *) CsrMemDup(&req->data[offset],
                                                                 sizeof(CsrBtTdDbBredrKey)));
                if (device)
                {
                    /* Since the key type is part of a bit field, CSR_BT_TD_DB_SC_KEY_BREDR_KEY needs to be a 1-bit value. */
                    PanicFalse(BtDevice_SetFlags(device, DEVICE_FLAGS_KEY_TYPE, CSR_BT_TD_DB_SC_KEY_BREDR_KEY));
                }
            }

            if (req->size_data >= sizeof(CsrBtTdDbLeKeys))
            {
                /* Set the offset to zero for extracting the LE keys information */
                offset = 0;
                CmWriteLeKeysReqSend(keySync_GetTask(),
                                     CSR_BT_ADDR_PUBLIC,
                                     &deviceAddr,
                                     (CsrBtCmKey *) CsrMemDup(&req->data[offset],
                                                              sizeof(CsrBtTdDbLeKeys)));
                if (device)
                {
                    /* Since the key type is part of a bit field, CSR_BT_TD_DB_SC_KEY_LE_KEYS needs to be a 1-bit value. */
                    PanicFalse(BtDevice_SetFlags(device, DEVICE_FLAGS_KEY_TYPE, (CSR_BT_TD_DB_SC_KEY_LE_KEYS << 5)));
                }
            }
#else
            ConnectionSmAddAuthDeviceRawRequest(keySync_GetTask(), &taddr, req->size_data / sizeof(uint16), (uint16*)req->data);
#endif
        }
        break;

        case MARSHAL_TYPE_key_sync_cfm_t:
        {
            key_sync_cfm_t* cfm = (key_sync_cfm_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd synced %u", cfm->synced);
            keySync_HandleKeySyncConfirmation(&cfm->bd_addr, cfm->synced);
        }
        break;

        case MARSHAL_TYPE_key_sync_paired_list_req_t:
        {
            key_sync_paired_list_req_t* req = (key_sync_paired_list_req_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd, peer-req reports %u paired handsets", req->num_handsets);

            /* check if we have any handsets which the peer doesn't */
            marked_for_delete = 0;
            DeviceList_Iterate(keySync_CompareDeviceLists, req);

            if (marked_for_delete)
            {
                DeviceDbSerialiser_Serialise();
                SystemReboot_Reboot();
            }

            /* Send CFM to peer containing our own list of paired handsets */
            keySync_SyncDeviceLists(FALSE);
        }
        break;

        case MARSHAL_TYPE_key_sync_paired_list_cfm_t:
        {
            key_sync_paired_list_cfm_t* cfm = (key_sync_paired_list_cfm_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd, peer-cfm reports %u paired handsets", cfm->num_handsets);

            /* check if we have any handsets which the peer doesn't */
            marked_for_delete = 0;
            DeviceList_Iterate(keySync_CompareDeviceLists, cfm);

            if (marked_for_delete)
            {
                DeviceDbSerialiser_Serialise();
                SystemReboot_Reboot();
            }
        }
        break;

        default:
        break;
    }
    
    if(ind->msg)
    {
        free(ind->msg);
    }
}

#ifndef USE_SYNERGY
static void keySync_HandleClSmGetAuthDeviceRawConfirm(CL_SM_GET_AUTH_DEVICE_RAW_CFM_T* cfm)
{
    bdaddr peer_addr;

    DEBUG_LOG_ALWAYS("keySync_HandleClSmGetAuthDeviceRawConfirm %u size %u", cfm->status, cfm->size_data);

    if ((cfm->status == success) && appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* data size is specified in 16-bit words, adjust to 8-bit */
        size_t size_data = cfm->size_data * sizeof(uint16);
        
        key_sync_paired_device_req_t* req = PanicUnlessMalloc(sizeof(key_sync_paired_device_req_t) + (size_data - 1));

        req->bd_addr = cfm->peer_taddr.addr;
        req->size_data = size_data;
        memcpy(req->data, cfm->data, size_data);

        /* send to counterpart on other earbud */
        appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                         req, MARSHAL_TYPE_key_sync_paired_device_req_t);
    }
    else
    {
        DEBUG_LOG_ALWAYS("keySync_HandleClSmGetAuthDeviceRawConfirm no peer to send to");
    }
}
#endif

static void keySync_AddDeviceAttributes(bdaddr* bd_addr)
{
    device_t handset_device = PanicNull(BtDevice_GetDeviceCreateIfNew(bd_addr, DEVICE_TYPE_HANDSET));
    PanicFalse(BtDevice_SetDefaultProperties(handset_device));
    PanicFalse(BtDevice_SetFlags(handset_device, DEVICE_FLAGS_PRE_PAIRED_HANDSET, DEVICE_FLAGS_PRE_PAIRED_HANDSET));
    PanicFalse(BtDevice_SetFlags(handset_device, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS, DEVICE_FLAGS_NO_FLAGS));
    BtDevice_AddSupportedProfiles((const bdaddr *)bd_addr, BtDevice_GetSupportedProfilesForDevice(BtDevice_GetSelfDevice()));
    DeviceDbSerialiser_SerialiseDevice(handset_device);
}

static void keySync_SendKeySyncCfm(bdaddr* bd_addr, bool synced)
{
    DEBUG_LOG_ALWAYS("keySync_SendKeySyncCfm");
    key_sync_cfm_t* key_cfm = PanicUnlessMalloc(sizeof(key_sync_cfm_t));
    key_cfm->bd_addr = *bd_addr;
    key_cfm->synced = synced;
    appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                     key_cfm, MARSHAL_TYPE_key_sync_cfm_t);
}

static void keySync_HandleSmAddAuthDeviceConfirm(bdaddr* bd_addr, bool success)
{
    bdaddr peer_addr;

    DEBUG_LOG_ALWAYS("keySync_HandleSmAddAuthDeviceConfirm %u", success);

    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        keySync_SendKeySyncCfm(bd_addr, success);
        keySync_AddDeviceAttributes(bd_addr);
        BtDevice_PrintAllDevices();

        if (success)
        {
#ifndef USE_SYNERGY
            /* The privacy mode is contained in the raw TDL data that was
               synced from the peer earbud, but the app needs to read it
               and call ConnectionDmUlpSetPrivacyModeReq to make sure it is
               sent throught to the controller. */
            typed_bdaddr taddr = {0};
            taddr.type = TYPED_BDADDR_PUBLIC;
            taddr.addr = *bd_addr;
            privacy_mode mode = privacy_mode_network;

            if (ConnectionAuthGetBlePrivacyMode(&taddr, &mode))
            {
                ConnectionDmUlpSetPrivacyModeReq(&taddr, mode);
                DEBUG_LOG("keySync_HandleSmAddAuthDeviceConfirm mode enum:privacy_mode:%u", mode);
            }
#endif

            /* Set the link behavior within bluestack to disable connection retires */
            BtDevice_SetLinkBehavior(bd_addr);
        }
    }
    else
    {
        DEBUG_LOG_ALWAYS("keySync_HandleSmAddAuthDeviceConfirm no peer to send to");
    }
}

#ifdef USE_SYNERGY
static void keySync_HandleCmPrim(Message message)
{
    CsrBtCmPrim *primType = (CsrBtCmPrim *)message;

    switch (*primType)
    {
        case CSR_BT_CM_DATABASE_CFM:
        {
            CsrBtCmDatabaseCfm *cfm = (CsrBtCmDatabaseCfm *) message;

            if (cfm->opcode == CSR_BT_CM_DB_OP_WRITE)
            {
                device_t device;
                bdaddr addr = {0};
                bool create_auth_device;

                BdaddrConvertBluestackToVm(&addr, &cfm->deviceAddr);
                device = BtDevice_GetDeviceForBdAddr(&addr);

                /* if auth device is not present then it needs to be created. */
                create_auth_device = !device;

                if (device)
                {
                    /* In case of synergy there are individual confirmations for LE and BREDR transport.
                     * In order to serialize this and act upon the final confirmation, device property
                     * flag DEVICE_FLAGS_KEY_TYPE is used which indicates which transport confirmation
                     * will be last. The auth device needs to be created only for the final confirmation. */
                    uint16 flags = 0;
                    uint8 key_type;

                    if (appDeviceGetFlagsForDevice(device, &flags))
                    {
                        /* If this is the final confirmation, create the auth device. */
                        key_type = ((flags & DEVICE_FLAGS_KEY_TYPE) >> 5);
                        create_auth_device = (cfm->keyType == key_type);
                    }
                }

                if (create_auth_device)
                {
                    if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
                    {
                        DEBUG_LOG("CSR_BT_CM_DATABASE_CFM result:0x%04x supplier:0x%04x", cfm->resultCode, cfm->resultSupplier);
                    }

                    /* Auth device needs to be created. */
                    keySync_HandleSmAddAuthDeviceConfirm(&addr, cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS);
                }
            }
        }
        break;

        default:
            DEBUG_LOG("keySync_HandleCmPrim unexpected CM prim %d", *primType);
            break;
    }

    CmFreeUpstreamMessageContents(message);
}
#endif

/*! Key Sync Message Handler. */
static void keySync_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    
    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            keySync_HandleCmPrim(message);
            break;
#endif
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            keySync_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            keySync_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

#ifndef USE_SYNERGY
        case CL_SM_GET_AUTH_DEVICE_RAW_CFM:
            keySync_HandleClSmGetAuthDeviceRawConfirm((CL_SM_GET_AUTH_DEVICE_RAW_CFM_T*)message);
            break;
        case CL_SM_ADD_AUTH_DEVICE_CFM:
        {
            CL_SM_ADD_AUTH_DEVICE_CFM_T* cfm = (CL_SM_ADD_AUTH_DEVICE_CFM_T*)message;
            keySync_HandleSmAddAuthDeviceConfirm(&cfm->bd_addr, cfm->status == success);
            break;
        }
#endif

        default:
            break;
    }
}

bool KeySync_Init(Task init_task)
{
    key_sync_task_data_t *ks = keySync_GetTaskData();

    UNUSED(init_task);

    DEBUG_LOG_ALWAYS("KeySync_Init");

    /* Initialise component task data */
    memset(ks, 0, sizeof(*ks));
    ks->task.handler = keySync_HandleMessage;
    ks->listeners = TaskList_CreateWithCapacity(2);

    /* Register with peer signalling to use the key sync msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(keySync_GetTask(), 
                                               PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                               key_sync_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
    return TRUE;
}

static void keySync_FindHandsetsAndCheckForSync(device_t device, void *data)
{
#ifdef USE_SYNERGY
    bdaddr peer_addr;
#endif /* USE_SYNERGY */
    uint16 flags = 0;
    typed_bdaddr handset_taddr = {0};

    UNUSED(data);
    
    DEBUG_LOG("KeySync_Sync Device %p", device);

    if (BtDevice_GetDeviceType(device) != DEVICE_TYPE_HANDSET)
    {
        DEBUG_LOG("KeySync_Sync Device not DEVICE_TYPE_HANDSET");
        return;
    }

    if(!Device_GetPropertyU16(device, device_property_flags, &flags))
    {
        DEBUG_LOG("KeySync_Sync No flags property");
        return;
    }
    
    if((flags & DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD) != DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD)
    {
        DEBUG_LOG("KeySync_Sync DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD not set");
        return;
    }
    
    handset_taddr.addr = DeviceProperties_GetBdAddr(device);
    handset_taddr.type = TYPED_BDADDR_PUBLIC;

    DEBUG_LOG_ALWAYS("KeySync_Sync found key to sync, handset bd_addr [0x%04x,0x%02x,0x%06lx]",
                      handset_taddr.addr.nap, handset_taddr.addr.uap, handset_taddr.addr.lap);
#ifdef USE_SYNERGY
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        bool le = FALSE;
        bool bredr = FALSE;
        uint8 size_data = 0;
        CsrBtTdDbLeKeys leKeys = { 0 };
        CsrBtTdDbBredrKey bredrKey = { 0 };
        CsrBtDeviceAddr deviceAddr = { 0 };

        BdaddrConvertVmToBluestack(&deviceAddr, &handset_taddr.addr);

        if (CsrBtTdDbGetLeKeys(CSR_BT_ADDR_PUBLIC,
                               &deviceAddr,
                               &leKeys) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        { /* LE keys exists */
            le = TRUE;
            size_data += sizeof(leKeys);
        }

        if (CsrBtTdDbGetBredrKey(CSR_BT_ADDR_PUBLIC,
                                 &deviceAddr,
                                 &bredrKey) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        { /* BREDR key exists */
            bredr = TRUE;
            size_data += sizeof(bredrKey);
        }

        if (bredr || le)
        {
            uint8 offset = 0;
            key_sync_paired_device_req_t *req = PanicUnlessMalloc(sizeof(*req) + size_data - 1);

            req->bd_addr = handset_taddr.addr;
            req->size_data = size_data;

            if (le)
            {
                memcpy(&req->data[offset], &leKeys, sizeof(leKeys));
                offset += sizeof(leKeys);
            }

            if (bredr)
            {
                memcpy(&req->data[offset], &bredrKey, sizeof(bredrKey));
                offset += sizeof(bredrKey);
            }

            /* send to counterpart on other earbud */
            appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                             PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                             req,
                                             MARSHAL_TYPE_key_sync_paired_device_req_t);
        }
        else
        {
            DEBUG_LOG("KeySync_Sync keys not found");
        }
    }
    else
    {
        DEBUG_LOG("KeySync_Sync no peer to send to");
    }
#else
    ConnectionSmGetAuthDeviceRawRequest(keySync_GetTask(), &handset_taddr);
#endif /* USE_SYNERGY */
}

void KeySync_Sync(void)
{
    DEBUG_LOG("KeySync_Sync");
    keySync_SyncDeviceLists(TRUE);
    DeviceList_Iterate(keySync_FindHandsetsAndCheckForSync, NULL);
}

bool KeySync_IsDeviceInSync(device_t device)
{
    DEBUG_LOG_VERBOSE("KeySync_IsDeviceInSync device lap 0x%x", DeviceProperties_GetBdAddr(device).lap);

    deviceType type = BtDevice_GetDeviceType(device);

    if (type == DEVICE_TYPE_EARBUD || type == DEVICE_TYPE_SELF)
    {
        DEBUG_LOG_VERBOSE("KeySync_IsDeviceInSync Device type is ERABUD or SELF which are always in sync");
        return TRUE;
    }
    else if(type == DEVICE_TYPE_HANDSET)
    {
        uint16 flags;
        if(Device_GetPropertyU16(device, device_property_flags, &flags))
        {
            bool in_sync = (0 == (flags & (DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD |
                                           DEVICE_FLAGS_NOT_PAIRED)));
            DEBUG_LOG_VERBOSE("KeySync_IsDeviceInSync in sync 0x%x", in_sync);
            return in_sync;
        }
    }

    DEBUG_LOG_ERROR("KeySync_IsDeviceInSync device with lap 0x%x is in the wrong state", DeviceProperties_GetBdAddr(device).lap);
    Panic();

    return FALSE;
}

void KeySync_RegisterListener(Task listener)
{
    key_sync_task_data_t *ks = keySync_GetTaskData();
    TaskList_AddTask(ks->listeners, listener);
}

bool KeySync_IsPdlUpdateInProgress(const bdaddr *bd_addr)
{
    uint16 flags = 0;
    bool update_in_progress = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);

    if (device)
    {
        Device_GetPropertyU16(device, device_property_flags, &flags);
        update_in_progress = ((flags & DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS) == DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS);
    }

    return update_in_progress;
}
