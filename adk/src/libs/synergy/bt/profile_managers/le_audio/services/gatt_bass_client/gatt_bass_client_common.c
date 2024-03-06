/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_bass_client_debug.h"
#include "gatt_bass_client_private.h"
#include "gatt_bass_client_common.h"

gatt_bass_client *bass_client_main;

ServiceHandle getBassServiceHandle(GBASSC **gatt_bass_client, CsrCmnList_t *list)
{
    ServiceHandle newHandle = ServiceHandleNewInstance((void **)gatt_bass_client, sizeof(GBASSC));

    if((*gatt_bass_client))
    {
        ServiceHandleListElm_t *elem = BASS_ADD_SERVICE_HANDLE(*list);

        elem->service_handle = newHandle;
        (*gatt_bass_client)->srvcElem = elem;
    }

    return newHandle;
}

CsrBool bassInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clntHndlElm = (ServiceHandleListElm_t *)elem;
    ServiceHandle clientHandle = *(ServiceHandle *)data;

    return (clntHndlElm->service_handle == clientHandle);
}

bool GattBassRegisterClient(gatt_bass_client_registration_params_t *reg_param,
                                                    GBASSC *gatt_bass_client)
{
    TYPED_BD_ADDR_T addr;

    gatt_bass_client->srvcElem->gattId = CsrBtGattRegister(CSR_BT_BASS_CLIENT_IFACEQUEUE);

    if (gatt_bass_client->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(reg_param->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gatt_bass_client->srvcElem->gattId,
                                                  reg_param->start_handle,
                                                  reg_param->end_handle,
                                                  addr);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

/*******************************************************************************/
void bassClientSendInternalMsgInitRead(GBASSC *gatt_bass_client)
{
    bass_client_handle_t *ptr = gatt_bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        MAKE_BASS_CLIENT_INTERNAL_MESSAGE(BASS_CLIENT_INTERNAL_MSG_INIT_READ);

        message->clnt_hndl = gatt_bass_client->srvcElem->service_handle;
        message->handle = ptr->handle;

        BassMessageSendConditionally(gatt_bass_client->lib_task,
                                 BASS_CLIENT_INTERNAL_MSG_INIT_READ,
                                 message,
                                 &gatt_bass_client->pending_cmd);

        ptr = ptr->next;
    }
}

/*******************************************************************************/
void bassClientAddHandle(GBASSC *bass_inst,
                         uint8 source_id,
                         uint16 handle,
                         uint16 handle_ccc)
{
    if (!bass_inst->client_data.broadcast_receive_state_handles_first)
    {
        /* It's the first time I'm adding a new handle in the list */
        bass_inst->client_data.broadcast_receive_state_handles_first = CsrPmemAlloc(sizeof(bass_client_handle_t));
        bass_inst->client_data.broadcast_receive_state_handles_first->source_id = source_id;
        bass_inst->client_data.broadcast_receive_state_handles_first->handle = handle;
        bass_inst->client_data.broadcast_receive_state_handles_first->handle_ccc = handle_ccc;
        bass_inst->client_data.broadcast_receive_state_handles_first->next = NULL;
        bass_inst->client_data.broadcast_receive_state_handles_last = bass_inst->client_data.broadcast_receive_state_handles_first;
    }
    else
    {
        /* There is already other elements in the list */
        bass_inst->client_data.broadcast_receive_state_handles_last->next = CsrPmemAlloc(sizeof(bass_client_handle_t));
        bass_inst->client_data.broadcast_receive_state_handles_last->next->source_id = source_id;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->handle = handle;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->handle_ccc = handle_ccc;
        bass_inst->client_data.broadcast_receive_state_handles_last->next->next = NULL;
        bass_inst->client_data.broadcast_receive_state_handles_last = bass_inst->client_data.broadcast_receive_state_handles_last->next;
    }
}

/*******************************************************************************/
void bassClientSendBroadcastReceiveStateSetNtfCfm(GBASSC *const bass_client,
                                                  const status_t status,
                                                  uint8 source_id)
{
    MAKE_BASS_CLIENT_MESSAGE(GattBassClientBroadcastReceiveStateSetNtfCfm);

    message->clntHndl = bass_client->srvcElem->service_handle;
    message->sourceId = source_id;
    message->status = status;

    BassMessageSend(bass_client->app_task,
                GATT_BASS_CLIENT_BROADCAST_RECEIVE_STATE_SET_NTF_CFM,
                message);
}

/*******************************************************************************/
void bassClientSendBroadcastAudioScanControlOpCfm(GBASSC *const bass_client,
                                                  const status_t status,
                                                  GattBassClientMessageId id)
{
    if (id != GATT_BASS_CLIENT_MESSAGE_TOP)
    {
        /* We will use GATT_BASS_CLIENT_REMOTE_SCAN_STOP_CFM to
         * create the message, because the structute of all the write confermations
         * is the same, but we will send the right message using the id parameter */
        MAKE_BASS_CLIENT_MESSAGE(GattBassClientRemoteScanStopCfm);

        /* Fill in client reference */
        message->clntHndl = bass_client->srvcElem->service_handle;

        /* Fill in the status */
        message->status = status;

        /* Send the confirmation message to app task  */
        BassMessageSend(bass_client->app_task, id, message);
    }
}

/*******************************************************************************/
GattBassClientStatus bassClientConvertGattStatus(status_t status)
{
    if (status == CSR_BT_GATT_RESULT_SUCCESS)
        return GATT_BASS_CLIENT_STATUS_SUCCESS;
    else
        return GATT_BASS_CLIENT_STATUS_FAILED;
}

/*******************************************************************************/
bool bassClientSourceIdFromCccHandle(GBASSC *bass_client,
                                     uint16 handle_ccc,
                                     uint8 *source_id)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while (ptr)
    {
        if(ptr->handle_ccc == handle_ccc)
        {
            (*source_id) = ptr->source_id;
            return TRUE;
        }

        ptr = ptr->next;
    }

    return FALSE;
}

/*******************************************************************************/
void bassClientDestroyBroadcastReceiveStateHandlesList(GBASSC *bass_inst)
{
    bass_client_handle_t *ptr = bass_inst->client_data.broadcast_receive_state_handles_first;
    bass_client_handle_t *tmp = NULL;

    while(ptr)
    {
        /* Save the next element of the list. */
        tmp = ptr->next;

        /* Free the actual element */
        free(ptr);

        /* Point to the next element of the list. */
        ptr = tmp;
    }

    bass_inst->client_data.broadcast_receive_state_handles_last = NULL;
    bass_inst->client_data.broadcast_receive_state_handles_first = NULL;
}

/*******************************************************************************/
void bassClientSendReadBroadcastReceiveStateCccCfm(GBASSC *bass_client,
                                                   status_t status,
                                                   uint16 size_value,
                                                   const uint8 *value,
                                                   uint8 source_id)
{
    MAKE_BASS_CLIENT_MESSAGE_WITH_LEN(GattBassClientReadBroadcastReceiveStateCccCfm, size_value);

    message->clntHndl = bass_client->srvcElem->service_handle;
    message->status = status;
    message->sizeValue = size_value;
    message->sourceId = source_id;

    memmove(message->value, value, size_value);

    BassMessageSend(bass_client->app_task, GATT_BASS_CLIENT_READ_BROADCAST_RECEIVE_STATE_CCC_CFM, message);
}

/*******************************************************************************/
uint16 bassClientHandleFromSourceId(GBASSC *bass_client,
                                    uint8 source_id,
                                    bool isCccHandle)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        if(ptr->source_id == source_id)
        {
            if(isCccHandle)
                return ptr->handle_ccc;
            else
                return ptr->handle;
        }

        ptr = ptr->next;
    }

    return 0;
}

/*******************************************************************************/
static void InitServiceHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}


void gatt_bass_client_init(void **gash)
{
    bass_client_main = CsrPmemZalloc(sizeof(*bass_client_main));
    *gash = bass_client_main;

    CsrCmnListInit(&bass_client_main->service_handle_list, 0, InitServiceHandleList, NULL);
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void GattBassClientDeInit(void **gash)
{
    CsrCmnListDeinit(&bass_client_main->service_handle_list);
    CsrPmemFree(bass_client_main);
}
#endif

/****************************************************************************/
void bassClientSwapByteTrasmissionOrder(uint8 *valueToSwap,
                                        uint8 len,
                                        uint8 *swapedValue)
{
    uint8 i,j;

    for(i=0, j=len-1; i<len && j>=0; i++, j--)
    {
        swapedValue[i] = valueToSwap[j];
    }
}

/****************************************************************************/
void bassClientExtractBroadcastReceiveStateCharacteristicValue(uint8 *value,
                                                               GattBassClientBroadcastReceiveState *brscValue)
{
    uint8 *ptr = NULL;

    brscValue->sourceId = value[0];
    brscValue->sourceAddress.type = value[1];

    brscValue->sourceAddress.addr.nap = (((uint16) value[7]) << 8) | ((uint16) value[6]);
    brscValue->sourceAddress.addr.uap = value[5];
    brscValue->sourceAddress.addr.lap = (((uint32) value[4]) << 16) |
                                    (((uint32) value[3]) << 8) |
                                    ((uint32) value[2]);

    brscValue->advSid = value[8];

    brscValue->broadcastId = (uint32) value[9] |
                             (uint32) value[10] << 8 |
                             (uint32) value[11] << 16;

    brscValue->paSyncState = (GattBassClientPaSyncStateType) value[12];
    brscValue->bigEncryption = (GattBassClientBroadcastBigEncryptionType) value[13];

    ptr = (&(value[14]));

    if(brscValue->bigEncryption == GATT_BASS_CLIENT_BAD_CODE)
    {
        brscValue->badcode = (uint8*) CsrPmemAlloc(sizeof(uint8) * BASS_CLIENT_BROADCAST_CODE_SIZE);
        bassClientSwapByteTrasmissionOrder(ptr,
                                           BASS_CLIENT_BROADCAST_CODE_SIZE,
                                           brscValue->badcode);
        ptr += BASS_CLIENT_BROADCAST_CODE_SIZE ;
    }
    else
    {
        brscValue->badcode = NULL;
    }

    brscValue->numSubGroups = *ptr;

    if(brscValue->numSubGroups)
    {
        uint8 i = 0;

        brscValue->subGroupsData =
                (GattBassClientSubGroupsData *) CsrPmemAlloc(sizeof(GattBassClientSubGroupsData) * brscValue->numSubGroups);

        for(i=0; i<(brscValue->numSubGroups); i++)
        {
            brscValue->subGroupsData[i].bisSync = (uint32) *(++ptr);
            brscValue->subGroupsData[i].bisSync |= ((uint32) *(++ptr)) << 8 ;
            brscValue->subGroupsData[i].bisSync |= ((uint32) *(++ptr)) << 16;
            brscValue->subGroupsData[i].bisSync |= ((uint32) *(++ptr)) << 24;

            brscValue->subGroupsData[i].metadataLen = *(++ptr);

            if(brscValue->subGroupsData[i].metadataLen)
            {
                brscValue->subGroupsData[i].metadata = (uint8 *) CsrPmemAlloc(sizeof(uint8) * brscValue->subGroupsData[i].metadataLen);
                SynMemCpyS(brscValue->subGroupsData[i].metadata,
                             brscValue->subGroupsData[i].metadataLen,
                               ++ptr,
                                 brscValue->subGroupsData[i].metadataLen);

                ptr += (brscValue->subGroupsData[i].metadataLen - 1);
            }
            else
            {
                brscValue->subGroupsData[i].metadata  = NULL;
            }
        }
    }
    else
    {
        brscValue->subGroupsData = NULL;
    }
}

/****************************************************************************/
void bassClientSetSourceId(GBASSC *bass_client,
                           uint16 handle,
                           uint8 sourceId)
{
    bass_client_handle_t *ptr = bass_client->client_data.broadcast_receive_state_handles_first;

    while(ptr)
    {
        /* Find the right Broadacast Receive State characteristic by the handle */
        if (handle == ptr->handle)
        {
            /* Found the right handle: save the sourceId */
            ptr->source_id = sourceId;
            break;
        }

        ptr = ptr->next;
    }
}

/****************************************************************************/
void gattBassClientPanic(void)
{
    GATT_BASS_CLIENT_PANIC("Invalid BASS Client instance!\n");
}

GattBassClientDeviceData *GattBassClientGetHandlesReq(ServiceHandle clntHndl)
{
    GBASSC *bassClient = ServiceHandleGetInstanceData(clntHndl);

    if (bassClient)
    {
        GattBassClientDeviceData* bassHandles = CsrPmemZalloc(sizeof(GattBassClientDeviceData));
        bassHandles->broadcastSourceNum = bassClient->client_data.broadcast_source_num ;
        return bassHandles;
    }
    return NULL;
}
