/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_sched.h"
#include "csr_util.h"
#include "csr_list.h"
#include "csr_bt_result.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_gatt_client_util_lib.h"
#include "csr_bt_gatt_client_util.h"
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_gatt_handover.h"
#endif

CsrBtGattClientUtilInst *gattClientUtilInst;

/*bool srvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data);*/
#define FIND_CLIENT_INST_BY_SERVICE_HANDLE(_Handle) \
                              (void *)ServiceHandleGetInstanceData(_Handle)

#define FIND_SERVICE_HANDLE_BY_GATTID(_List,_id) \
                              ((ServiceHandleListElm_t *)CsrCmnListSearch(&(_List), \
                               srvcHndlFindByGattId,(void *)(&(_id))))

typedef int32 srvcInternal;

typedef struct
{
    srvcInternal id;
    ServiceHandle srvc_hndl;
} getSrvcHndl;

void *GetServiceClientByServiceHandle(void *msg)
{
    void *client_service = NULL;
    getSrvcHndl *util = (getSrvcHndl *) msg;

    client_service = ServiceHandleGetInstanceData(util->srvc_hndl);
    return client_service;
}

static CsrBool srvcHndlFindByGattId(CsrCmnListElm_t *elem, void *data)
{
    ServiceHandleListElm_t *clnt_hndl_elm = (ServiceHandleListElm_t *)elem;
    CsrBtGattId     gattId   = *(CsrBtGattId *) data;

    return (clnt_hndl_elm->gattId == gattId);
}

/* The list should be having element type as ServiceHandleListElm_t only */
void *GetServiceClientByGattMsg(CsrCmnList_t *list, void *msg)
{
    typedef struct
    {
        CsrPrim type;
        CsrBtGattId gattid;
    } getGattId;

    getGattId *gattMsg = (getGattId *) msg;
    void *client_service = NULL;
    ServiceHandleListElm_t* elem = NULL;

    /* Find vcs_client instance using gattId */
    elem = FIND_SERVICE_HANDLE_BY_GATTID(*list,
                                         gattMsg->gattid);
    if (elem)
        client_service = FIND_CLIENT_INST_BY_SERVICE_HANDLE(elem->service_handle);

    return client_service;
}

void *GetGattManagerMsgFromGattMsg(void *message, CsrBtGattPrim *id)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)message;

    switch (*prim)
    {
        case CSR_BT_GATT_DISCOVER_CHARAC_IND:
        {
            att_uuid_type_t uuidType;
            CsrBtGattDiscoverCharacInd *gattMsg = (CsrBtGattDiscoverCharacInd *) message;

            GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg = CsrPmemZalloc(sizeof(GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T));

            *id = CSR_BT_GATT_DISCOVER_CHARAC_CFM;

            msg->cid = gattMsg->btConnId;
            msg->declaration = gattMsg->declarationHandle;
            msg->properties = gattMsg->property;
            msg->handle = gattMsg->valueHandle;
            msg->status = ATT_RESULT_SUCCESS;
            msg->more_to_come = TRUE;

            if (gattMsg->uuid.length == CSR_BT_UUID16_SIZE)
                uuidType = ATT_UUID16;
            else if (gattMsg->uuid.length == CSR_BT_UUID128_SIZE)
                uuidType = ATT_UUID128;
            else if (gattMsg->uuid.length == CSR_BT_UUID32_SIZE)
                uuidType = ATT_UUID32;
            else
                uuidType = ATT_UUID_NONE;

            msg->uuid_type = uuidType;

            CsrMemSet(msg->uuid, 0, CSR_BT_UUID128_SIZE);

            if (uuidType == ATT_UUID16)
            {
                msg->uuid[0] = CSR_BT_UUID_GET_32(gattMsg->uuid);
            }
            else
            {
                msg->uuid[0] = ((CsrUint32)((gattMsg->uuid).uuid[0]|((gattMsg->uuid).uuid[1]<<8)|((gattMsg->uuid).uuid[2]<<16)|((gattMsg->uuid).uuid[3]<<24)));
                msg->uuid[1] = ((CsrUint32)((gattMsg->uuid).uuid[4]|((gattMsg->uuid).uuid[5]<<8)|((gattMsg->uuid).uuid[6]<<16)|((gattMsg->uuid).uuid[7]<<24)));
                msg->uuid[2] = ((CsrUint32)((gattMsg->uuid).uuid[8]|((gattMsg->uuid).uuid[9]<<8)|((gattMsg->uuid).uuid[10]<<16)|((gattMsg->uuid).uuid[11]<<24)));
                msg->uuid[3] = ((CsrUint32)((gattMsg->uuid).uuid[12]|((gattMsg->uuid).uuid[13]<<8)|((gattMsg->uuid).uuid[14]<<16)|((gattMsg->uuid).uuid[15]<<24)));
            }

            return msg;
        }
        case CSR_BT_GATT_DISCOVER_CHARAC_CFM:
        {
            CsrBtGattDiscoverCharacCfm *gattMsg = (CsrBtGattDiscoverCharacCfm*) message;

            GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *msg = CsrPmemZalloc(sizeof(GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T));


            msg->cid                = gattMsg->btConnId;
            msg->declaration        = CSR_BT_GATT_ATTR_HANDLE_INVALID;
            msg->properties         = 0;
            msg->handle             = CSR_BT_GATT_ATTR_HANDLE_INVALID;
            msg->status             = ATT_RESULT_SUCCESS;
            msg->more_to_come       = FALSE;
            msg->uuid_type          = ATT_UUID_NONE;

            return msg;
        }
        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_IND:
        {
            att_uuid_type_t uuidType;
            CsrBtGattDiscoverCharacDescriptorsInd* gattMsg = (CsrBtGattDiscoverCharacDescriptorsInd*) message;

            GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *msg = CsrPmemZalloc(sizeof(GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T));

            *id = CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM;

            msg->cid                = gattMsg->btConnId;
            msg->handle             = gattMsg->descriptorHandle;
            msg->more_to_come       = TRUE;
            msg->status             = ATT_RESULT_SUCCESS;

            if (gattMsg->uuid.length == CSR_BT_UUID16_SIZE)
                uuidType = ATT_UUID16;
            else if (gattMsg->uuid.length == CSR_BT_UUID128_SIZE)
                uuidType = ATT_UUID128;
            else if (gattMsg->uuid.length == CSR_BT_UUID32_SIZE)
                uuidType = ATT_UUID32;
            else
                uuidType = ATT_UUID_NONE;

            msg->uuid_type          = uuidType;
            CsrMemSet(msg->uuid, 0, CSR_BT_UUID128_SIZE);

            if (uuidType == ATT_UUID16)
            {
                msg->uuid[0]            = CSR_BT_UUID_GET_32(gattMsg->uuid);
            }
            else
            {
                msg->uuid[0] = ((CsrUint32)((gattMsg->uuid).uuid[0]|((gattMsg->uuid).uuid[1]<<8)|((gattMsg->uuid).uuid[2]<<16)|((gattMsg->uuid).uuid[3]<<24)));
                msg->uuid[1] = ((CsrUint32)((gattMsg->uuid).uuid[4]|((gattMsg->uuid).uuid[5]<<8)|((gattMsg->uuid).uuid[6]<<16)|((gattMsg->uuid).uuid[7]<<24)));
                msg->uuid[2] = ((CsrUint32)((gattMsg->uuid).uuid[8]|((gattMsg->uuid).uuid[9]<<8)|((gattMsg->uuid).uuid[10]<<16)|((gattMsg->uuid).uuid[11]<<24)));
                msg->uuid[3] = ((CsrUint32)((gattMsg->uuid).uuid[12]|((gattMsg->uuid).uuid[13]<<8)|((gattMsg->uuid).uuid[14]<<16)|((gattMsg->uuid).uuid[15]<<24)));

            }
            return msg;
        }
        case CSR_BT_GATT_DISCOVER_CHARAC_DESCRIPTORS_CFM:
        {
            CsrBtGattDiscoverCharacDescriptorsCfm* gattMsg = (CsrBtGattDiscoverCharacDescriptorsCfm*) message;

            GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *msg = CsrPmemZalloc(sizeof(GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T));

            msg->cid                = gattMsg->btConnId;
            msg->handle             = CSR_BT_GATT_ATTR_HANDLE_INVALID;
            msg->more_to_come       = FALSE;
            msg->status             = ATT_RESULT_SUCCESS;
            msg->uuid_type          = ATT_UUID_NONE;

            return msg;
        }
        default :
            break;

    }
    return message;
}

CsrBool CsrBtGattClientUtilFindConnectedBtConnId(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattClientUtilDeviceElement *conn = (CsrBtGattClientUtilDeviceElement *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) value;
    return ((conn->btConnId == btConnId) ? TRUE : FALSE);
}

CsrBool CsrBtGattClientUtilFindConnectedBtAddr(CsrCmnListElm_t *elem, void *value)
{
    CsrBtGattClientUtilDeviceElement *conn = (CsrBtGattClientUtilDeviceElement *)elem;
    CsrBtTypedAddr     *addr   = (CsrBtTypedAddr *) value;

    return CsrBtAddrEq(&conn->addr, addr);
}

CsrBool CsrBtGattClientUtilFindAddrByConnId(CsrBtConnId btConnId, CsrBtTypedAddr *addr)
{
    CsrBtGattClientUtilDeviceElement *inst = CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTCONNID(gattClientUtilInst->addressList,
                                                                                &btConnId);

    if (inst)
    {
        CsrBtAddrCopy(addr, &(inst->addr));
        return TRUE;
    }
    return FALSE;
}

CsrBtConnId CsrBtGattClientUtilFindConnIdByAddr(CsrBtTypedAddr *addr)
{
    CsrBtGattClientUtilDeviceElement *inst = CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTADDR(gattClientUtilInst->addressList,
                                                                                addr);

    if (inst)
    {
        return inst->btConnId;
    }
    return INVALID_BTCONNID;
}

#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
CsrBool CsrBtGattClientUtilFIndEncryptionStatus(CsrBtTypedAddr *addr)
{
	CsrBtGattClientUtilDeviceElement *inst = CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTADDR(gattClientUtilInst->addressList,
																				addr);

	if (inst)
	{
		return inst->encrypted;
	}

	return FALSE;
}
#endif

#ifdef CSR_TARGET_PRODUCT_VM
static void csrBtGattClientUtilAddDeviceInfo(CsrBtTypedAddr *addr, CsrBtConnId btConnId)
{
    CsrBtGattClientUtilDeviceElement *elem =
                    CSR_BT_GATT_CLIENT_UTIL_ADD_PRIM_DEVICE(gattClientUtilInst->addressList);
    elem->btConnId  = btConnId;
    CsrBtAddrCopy(&(elem->addr), addr);
}
#endif

/* Initialize the connection instance */
static void deviceInstInit(CsrCmnListElm_t *elem)
{
    CsrBtGattClientUtilDeviceElement *inst = (CsrBtGattClientUtilDeviceElement *) elem;

    inst->btConnId = CSR_BT_CONN_ID_INVALID;
    CsrBtAddrZero(&inst->addr);
#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
	inst->encrypted = FALSE;
#endif
}

static void CsrBtGattClientUtilCmPrimHandler(CsrBtGattClientUtilInst *inst)
{
	CsrPrim *primType;
	primType = (CsrPrim *)inst->recvMsg;

	switch (*primType)
	{
		case CSR_BT_CM_SET_EVENT_MASK_CFM:
			break;
#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
		case CSR_BT_CM_ENCRYPT_CHANGE_IND:
		{
			CsrBtTypedAddr addr;
            CsrBtGattClientUtilDeviceElement *elem;
			CsrBtCmEncryptChangeInd *ind = (CsrBtCmEncryptChangeInd *) inst->recvMsg;
			addr.addr = ind->deviceAddr;
			addr.type = ind->deviceAddrType;
			elem = CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTADDR(inst->addressList, &addr);
			if (elem &&  ind->encryptType)
			{
				elem->encrypted = TRUE;
			}

		}
		break;
#endif
		default:
			break;
	}
	CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, inst->recvMsg);
}

static void CsrBtGattClientUtilPrimHandler(CsrBtGattClientUtilInst *inst)
{
    CsrPrim *primType;
    primType = (CsrPrim *) inst->recvMsg;

    switch (*primType)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            {
                CsrBtGattRegisterCfm *cfm = (CsrBtGattRegisterCfm*)inst->recvMsg;
                if(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
                {
                    inst->gattId = cfm->gattId;
                    CsrBtGattSetEventMaskReqSend(cfm->gattId,\
                        CSR_BT_GATT_EVENT_MASK_SUBSCRIBE_ATT_LE_FIXED_CHANNEL_STATUS);
                }
            break;
            }
        case CSR_BT_GATT_CONNECT_IND:
            {
                CsrBtGattConnectInd  *ind = (CsrBtGattConnectInd *) inst->recvMsg;

                if (ind->resultSupplier == CSR_BT_SUPPLIER_GATT &&
                    ind->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
                {
                    CsrBtGattClientUtilDeviceElement *elem =
                                    CSR_BT_GATT_CLIENT_UTIL_ADD_PRIM_DEVICE(inst->addressList);
                    elem->btConnId  = ind->btConnId;
                    CsrBtAddrCopy(&(elem->addr), &(ind->address));
                }
            }
            break;
        case CSR_BT_GATT_DISCONNECT_IND:
        {
            CsrBtGattDisconnectInd  *ind = (CsrBtGattDisconnectInd *) inst->recvMsg;

            CsrBtGattClientUtilDeviceElement *elem = CSR_BT_GATT_CLIENT_UTIL_FIND_DEVICE_BY_BTCONNID(gattClientUtilInst->addressList,
                                                                                        &(ind->btConnId));
            CSR_BT_GATT_CLIENT_UTIL_REMOVE_PRIM_DEVICE(gattClientUtilInst->addressList, elem);
        }
        break;
    }

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, inst->recvMsg);
}

void CsrBtGattClientUtilInit(void **gash)
{
    gattClientUtilInst  = CsrPmemZalloc(sizeof(*gattClientUtilInst));

    CsrCmnListInit(&gattClientUtilInst->addressList,
                   0,
                   deviceInstInit,
                   NULL);

    *gash = gattClientUtilInst;

    CsrBtGattRegisterReqSend(CSR_BT_GATT_CLIENT_UTIL_IFACEQUEUE, 0);
#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
    CsrBtCmSetEventMaskReqSend(CSR_BT_GATT_CLIENT_UTIL_IFACEQUEUE,
                (CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ENCRYPT_CHANGE), CSR_BT_CM_EVENT_MASK_COND_ALL);
#endif

#ifdef CSR_TARGET_PRODUCT_VM
    CsrBtGattCallBackRegister(csrBtGattClientUtilAddDeviceInfo);
#endif
}

void CsrBtGattClientUtilHandler(void **gash)
{
    CsrUint16 eventClass = 0;

    CSR_UNUSED(gash);

    if (CsrSchedMessageGet(&eventClass, &gattClientUtilInst->recvMsg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
                CsrBtGattClientUtilPrimHandler(gattClientUtilInst);
                break;
			case CSR_BT_CM_PRIM:
				CsrBtGattClientUtilCmPrimHandler(gattClientUtilInst);
				break;
            default:
                /* Unknown event type */
                break;
        }

        SynergyMessageFree(eventClass, gattClientUtilInst->recvMsg);
        gattClientUtilInst->recvMsg = NULL;
    }
}

static uint16 gattAccessIndIteratorGetBufferSize(const CsrBtGattDbAccessWriteInd *accessInd, uint8 bufferIndex)
{
    if (bufferIndex < accessInd->writeUnitCount)
        return accessInd->writeUnit[bufferIndex].valueLength;
    else
        return 0;
}

static uint8 gattAccessIndIteratorIsOutOfBoundRead(GattAccessWriteIndIterator *iter)
{
    if ((iter->bufferIndex >= iter->accessInd->writeUnitCount) ||
            iter->offset >= iter->accessInd->writeUnit[iter->bufferIndex].valueLength)/* should never happen unless changed forcibly outside the functions */
    {
        iter->error = TRUE;
    }
    return iter->error;
}

static void gattAccessIndIteratorIncr(GattAccessWriteIndIterator *iter)
{
    iter->offset++;
    if (iter->offset >= gattAccessIndIteratorGetBufferSize(iter->accessInd, iter->bufferIndex))
    {
        iter->offset = 0;
        iter->bufferIndex++;
    }
}

void GattAccessIndIteratorInitialise(GattAccessWriteIndIterator *iter, const CsrBtGattDbAccessWriteInd *accessInd)
{
    if (accessInd == NULL)
    {
        iter->error = TRUE;
    }
    iter->accessInd = accessInd;
    iter->bufferIndex = 0;
    iter->offset = 0;
    iter->error = FALSE;
}

uint8 GattAccessIndIteratorRead8(GattAccessWriteIndIterator* iter)
{
    if (gattAccessIndIteratorIsOutOfBoundRead(iter) == FALSE)
    {
        uint8 returnVal = iter->accessInd->writeUnit[iter->bufferIndex].value[iter->offset];
        gattAccessIndIteratorIncr(iter);
        return returnVal;
    }
    return 0;
}

uint16 GattAccessIndIteratorRead16(GattAccessWriteIndIterator* iter)
{
    uint16 value;
    value  = GattAccessIndIteratorRead8(iter);
    value += GattAccessIndIteratorRead8(iter) << 0x08;
    return value;
}

uint32 GattAccessIndIteratorRead24(GattAccessWriteIndIterator* iter)
{
    uint32 value;
    value  = GattAccessIndIteratorRead8(iter);
    value += GattAccessIndIteratorRead8(iter) << 0x08;
    value += GattAccessIndIteratorRead8(iter) << 0x10;
    return value;
}

uint32 GattAccessIndIteratorRead32(GattAccessWriteIndIterator* iter)
{
    uint32 value;
    value  = GattAccessIndIteratorRead8(iter);
    value += GattAccessIndIteratorRead8(iter) << 0x08;
    value += GattAccessIndIteratorRead8(iter) << 0x10;
    value += GattAccessIndIteratorRead8(iter) << 0x18;
    return value;
}

uint8* GattAccessIndIteratorReadMultipleOctets(GattAccessWriteIndIterator* iter, uint8 numOctets)
{
    uint8* dest = NULL;

    if (!numOctets)
    {
        return NULL;
    }

    dest = CsrPmemZalloc(numOctets);
    {/*! TODO: optimise this to use SynMemCpyS instead of reading 8-by-8*/
        int i;
        for (i = 0; i < numOctets; i++)
        {
            dest[i] = GattAccessIndIteratorRead8(iter);
            if (iter->error == TRUE)
            {
                CsrPmemFree(dest);
                return NULL;
            }
        }
    }

    return dest;
}

void GattBuffIteratorInitialise(GattBuffIterator* iter, uint8* buffer, uint16 size)
{
    iter->dataStart = buffer;
    iter->data = buffer;
    iter->error = FALSE;
    iter->size = size;
}

bool GattBuffIteratorWrite8(GattBuffIterator* iter, uint8 value)
{
    if ((iter->data - iter->dataStart) < iter->size)
        *iter->data++ = value;
    else
        iter->error = TRUE;

    return ( ! iter->error);
}

bool GattBuffIteratorWrite16(GattBuffIterator* iter, uint16 value)
{
    bool result;

    result = GattBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);

    return result;
}

bool GattBuffIteratorWrite24(GattBuffIterator* iter, uint32 value)
{
    bool result;

    result = GattBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x10) & 0x00FF);

    return result;
}

bool GattBuffIteratorWrite32(GattBuffIterator* iter, uint32 value)
{
    bool result;

    result = GattBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x10) & 0x00FF);
    if (result)
        result = GattBuffIteratorWrite8(iter, (value >> 0x18) & 0x00FF);

    return result;
}

bool GattBuffIteratorWriteMultipleOctets(GattBuffIterator* iter, uint8* src, uint8 numOctets)
{
    if (numOctets)
    {
        uint8* dest = iter->data;

        GattBuffIteratorSkipOctets(iter, numOctets);
        if (! GATT_BUFF_ITERATOR_ERROR_DETECTED(iter))
        {
            size_t dstRemainingOctets = (size_t)iter->size - (dest - iter->dataStart);
            SynMemCpyS(dest, dstRemainingOctets, src, numOctets);
        }
    }
    return ( ! iter->error );
}

void GattBuffIteratorSkipOctets(GattBuffIterator* iter, uint8 numOctets)
{
    iter->data += numOctets;
    if ((iter->data - iter->dataStart) <= iter->size)
    {
        /* no error: iter->data still points within the bounds of the buffer */
    }
    else
        iter->error = TRUE;
}

#ifdef ENABLE_SHUTDOWN
/****************************************************************************
 This function is called by the scheduler to perform a graceful shutdown
 of a scheduler task.
 This function must:
     1)  empty the input message queue and free any allocated memory in the
     messages.
     2)  free any instance data that may be allocated.
 ****************************************************************************/
void CsrBtGattClientUtilDeinit(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    CsrBtGattClientUtilInst *inst = *gash;

    while (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
                break;
        }

        SynergyMessageFree(eventClass, msg);
    }
    CsrPmemFree(inst);
    inst = NULL;
}
#endif /* ENABLE_SHUTDOWN */
