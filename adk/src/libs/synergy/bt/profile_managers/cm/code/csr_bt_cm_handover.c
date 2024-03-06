/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_bt_util.h"
#include "csr_bt_panic.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_l2cap.h"
#include "csr_bt_cm_rfc.h"
#ifdef CSR_STREAMS_ENABLE
#include "csr_bt_cm_streams_handler.h"
#endif
#include "dm_prim.h"
#include "message.h"

#ifdef CSR_BT_LE_ENABLE
#include "csr_bt_cm_le.h"
#endif

#ifdef CSR_LOG_ENABLE
#define CSR_BT_CM_LTSO_HANDOVER             0
#define CSR_BT_CM_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtCmLto, CSR_BT_CM_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_CM_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtCmLto, CSR_BT_CM_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_CM_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtCmLto, CSR_BT_CM_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_CM_HANDOVER_LOG_INFO(...)
#define CSR_BT_CM_HANDOVER_LOG_WARNING(...)
#define CSR_BT_CM_HANDOVER_LOG_ERROR(...)
#endif

#define GET_CURRENT_DEVICE(pConv) (pConv->device[pConv->deviceIndex])
#define GET_CURRENT_DEVICE_FROM_INDEX(pConv,index) (index < (MAX_NUM_OF_LINK_UNMARSHAL) ? pConv->device[index] : NULL)

/* Maximum number of csrBtCmDeviceConnInfoRef. For each handset, 2 (BREDR and LE) csrBtCmDeviceConnInfoRef are required */
#define MAX_NUM_OF_LINK_UNMARSHAL ((NUM_OF_ACL_CONNECTION - 1)*2)

typedef struct
{
    const CsrBtTpdAddrT *tpAddr;
    CsrUint8 count;
} csrBtCmDeviceConnCountRef;

typedef struct
{
    const CsrBtTpdAddrT *tpAddr;
    CsrBtMarshalUtilInst *conv;
} csrBtCmDeviceConnConvertRef;

typedef struct
{
    struct
    {
        CsrUint8 count;
        struct
        {
            CsrBtConnId connId;
            CsrUint8 channel;
            CsrUint16 context;
        } *connInfo;
    } rfc;

    struct
    {
        CsrUint8 count;
        struct
        {
            CsrBtConnId connId;
            psm_t psm;
        } *connInfo;
    } l2ca;

    CsrBtTpdAddrT tpAddrt;
} csrBtCmDeviceConnInfoRef;

/* cmConverter structure members device and deviceIndex are used only during unmarshal */
static struct cmConverter_tag
{
    CsrBtMarshalUtilInst *conv;
    csrBtCmDeviceConnInfoRef *device[MAX_NUM_OF_LINK_UNMARSHAL]; /* Peer ACL excluded */
    CsrUint8 deviceIndex;
#ifdef CSR_BT_LE_ENABLE
    CsrUint8 connCacheCnt; /* LE connCache nodes unmarshalled */
#endif
} *cmConverter;

static CsrBtTpdAddrT currentLinkAddr;

#if 0
static CsrBool cmRfcRemoveConnAddr(CsrCmnListElm_t *elem, void *data)
{
    cmRfcConnElement *rfcElem = (cmRfcConnElement *) elem;
    CsrBtDeviceAddr *addr = (CsrBtDeviceAddr *) data;

    if (rfcElem->cmRfcConnInst)
    {
        if (CsrBtBdAddrEq(addr, &rfcElem->cmRfcConnInst->deviceAddr))
        {
            if (rfcElem->cmRfcConnInst->remoteServerChan == CSR_BT_NO_SERVER)
            { /* Incoming connection */
                rfcElem->cmRfcConnInst->btConnId = CSR_BT_CONN_ID_INVALID;
                rfcElem->cmRfcConnInst->state = CSR_BT_CM_RFC_STATE_IDLE;
            }
            else
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static CsrBool cmL2caRemoveConnAddr(CsrCmnListElm_t *elem, void *data)
{
    cmL2caConnElement *l2caConnElem = (cmL2caConnElement *) elem;
    CsrBtDeviceAddr *addr = (CsrBtDeviceAddr *) data;

    if (l2caConnElem->cmL2caConnInst)
    {
        if (CsrBtBdAddrEq(&l2caConnElem->cmL2caConnInst->deviceAddr, addr))
        {
            if (l2caConnElem->cmL2caConnInst->remotePsm == NO_REMOTE_PSM)
            { /* Incoming connection */
                l2caConnElem->cmL2caConnInst->btConnId = BTCONN_ID_RESERVED;
                l2caConnElem->cmL2caConnInst->state = CSR_BT_CM_L2CAP_STATE_IDLE;
            }
            else
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}
#endif

static void cmL2caConnInstCount(CsrCmnListElm_t *elem, void *data)
{
    cmL2caConnElement *l2caConnElem = (cmL2caConnElement *) elem;
    csrBtCmDeviceConnCountRef *ref = (csrBtCmDeviceConnCountRef *) data;

    if (l2caConnElem->cmL2caConnInst)
    {
        if (l2caConnElem->cmL2caConnInst->transportType == ref->tpAddr->tp_type &&
            CsrBtBdAddrEq(&l2caConnElem->cmL2caConnInst->deviceAddr, &ref->tpAddr->addrt.addr))
        {
            ref->count++;
        }
    }
}

static void cmRfcConnInstCount(CsrCmnListElm_t *elem, void *data)
{
    cmRfcConnElement *rfcConnElem = (cmRfcConnElement *) elem;
    csrBtCmDeviceConnCountRef *ref = (csrBtCmDeviceConnCountRef *) data;

    if (rfcConnElem->cmRfcConnInst)
    {
        if (CsrBtBdAddrEq(&rfcConnElem->cmRfcConnInst->deviceAddr, &ref->tpAddr->addrt.addr))
        {
            ref->count++;
        }
    }
}

static CsrBool cmHandoverUnmarshalledRfcInstRemove(CsrCmnListElm_t *elem, void *data)
{
    CsrBool remove = FALSE;
    cmRfcConnElement *rfcElem = (cmRfcConnElement*) elem;

    if (rfcElem->unmarshalledAdded)
    { /* Was added due to unmarshalling */
        remove = TRUE;
    }
    else if (rfcElem->unmarshalledFilled)
    { /* Connect accept instance already existed, reset it */
        cmRfcConnInstType *connInst = rfcElem->cmRfcConnInst;

        rfcElem->unmarshalledFilled = FALSE;

        if (connInst)
        { /* Reset connection instance */
            connInst->btConnId = CSR_BT_CONN_ID_INVALID;
            connInst->remoteServerChan = CSR_BT_NO_SERVER;
            CSR_BT_CM_STATE_CHANGE(connInst->state, CSR_BT_CM_RFC_STATE_IDLE);
            CsrBtBdAddrZero(&connInst->deviceAddr);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
            /* Reset the SCO parameters if un-marshalled */
            if (connInst->eScoParms)
            {
                CsrPmemFree(connInst->eScoParms);
                connInst->eScoParms = NULL;
            }
#endif /* !EXCLUDE_CSR_BT_RFC_MODULE */
        }
        else
        { /* Zombie connection instance */
            remove = TRUE;
        }
    }

    CSR_UNUSED(data);

    return remove;
}

static CsrBool cmHandoverUnmarshalledL2capInstRemove(CsrCmnListElm_t *elem, void *data)
{
    CsrBool remove = FALSE;
    cmL2caConnElement *l2caElem = (cmL2caConnElement*) elem;

    if (l2caElem->unmarshalledAdded)
    { /* Was added due to unmarshalling */
        remove = TRUE;
    }
    else if (l2caElem->unmarshalledFilled)
    { /* Connect accept instance already existed, reset it */
        cmL2caConnInstType *connInst = l2caElem->cmL2caConnInst;

        l2caElem->unmarshalledFilled = FALSE;

        if (connInst)
        { /* Reset connection instance */
            connInst->remotePsm = NO_REMOTE_PSM;
            connInst->btConnId = BTCONN_ID_RESERVED;
            CSR_BT_CM_STATE_CHANGE(connInst->state, CSR_BT_CM_L2CAP_STATE_IDLE);
            CsrBtBdAddrZero(&connInst->deviceAddr);
        }
        else
        { /* Zombie connection instance */
            remove = TRUE;
        }
    }

    CSR_UNUSED(data);

    return remove;
}

static void cmHandoverAclInstRemove(CsrBtTpdAddrT tpAddrt)
{
    cmInstanceData_t *cmData = CmGetInstanceDataPtr();
    aclTable *acl;

    returnAclConnectionElement(cmData,
                               tpAddrt.addrt.addr,
                               &acl);
    if (acl)
    {
        CsrBtDeviceAddr zeroAddr = { 0 };

        csrBtCmAclElemInit(cmData, acl, &zeroAddr);
    }
}

static void cmHandoverUnmarshalledInstRemove(CsrBtTpdAddrT tpAddrt)
{
    if (cmConverter && CsrBtMarshalUtilTypeGet(cmConverter->conv) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        cmInstanceData_t *cmData = CmGetInstanceDataPtr();

        CsrCmnListIterateAllowRemove(&cmData->l2caVar.connList,
                                     cmHandoverUnmarshalledL2capInstRemove,
                                     &tpAddrt);

        CsrCmnListIterateAllowRemove(&cmData->rfcVar.connList,
                                     cmHandoverUnmarshalledRfcInstRemove,
                                     &tpAddrt);

        cmHandoverAclInstRemove(tpAddrt);
    }
}

static void convAclTable(CsrBtMarshalUtilInst *conv, aclTable *acl)
{
    const CsrUint8 bitfieldBlobOffset = CsrOffsetOf(aclTable, encryptType) + sizeof(acl->encryptType);
    const CsrUint16 bitfieldBlobSize = CsrOffsetOf(aclTable, lmpVersion) - bitfieldBlobOffset;
    void *bitfieldBlobPtr = &((CsrUint8 *) acl)[bitfieldBlobOffset];

    CsrBtMarshalUtilConvertObj(conv, acl->remoteFeatures);
    CsrBtMarshalUtilConvertObj(conv, acl->curSsrSettings);
    CsrBtMarshalUtilConvertObj(conv, acl->linkPolicySettings);
    CsrBtMarshalUtilConvertObj(conv, acl->cod);
    CsrBtMarshalUtilConvertObj(conv, acl->l2capExtendedFeatures);
    CsrBtMarshalUtilConvertObj(conv, acl->latency);
    CsrBtMarshalUtilConvertObj(conv, acl->flushTo);
    CsrBtMarshalUtilConvertObj(conv, acl->lsto);
    CsrBtMarshalUtilConvertObj(conv, acl->interval);
    CsrBtMarshalUtilConvertObj(conv, acl->role);
    CsrBtMarshalUtilConvertObj(conv, acl->encryptType);
    CsrBtMarshalUtilConvertObj(conv, acl->mode);

#ifdef CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_CHANNEL_TYPE
    CsrBtMarshalUtilConvertObj(conv, acl->logicalChannelTypeMask);
    CsrBtMarshalUtilConvertObj(conv, acl->noOfGuaranteedLogicalChannels);
#endif

    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);
}

static void deserRoleVariables(CsrBtMarshalUtilInst *conv,
                               roleVariables *roleVar,
                               const CsrBtDeviceAddr *addr)
{
    CsrUint8 i;
    CsrBool present = FALSE;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (CsrBtBdAddrEq(&roleVar->aclVar[i].deviceAddr, addr))
        {
            present = TRUE;
            break;
        }
    }

    CsrBtMarshalUtilConvertObj(conv, present);

    if (present)
    {
        aclTable *acl;

        if (i < NUM_OF_ACL_CONNECTION)
        {
            acl = &roleVar->aclVar[i];
        }
        else
        { /* Add a new ACL connection instance */
            CsrBtDeviceAddr zeroAddr = { 0 };
            cmInstanceData_t *cmData = CmGetInstanceDataPtr();

            returnAclConnectionElement(cmData, zeroAddr, &acl);

            if (acl)
            {
                csrBtCmAclElemInit(cmData, acl, addr);
            }
        }

        convAclTable(conv, acl);
    }
}

static void serRoleVariables(CsrBtMarshalUtilInst *conv,
                             roleVariables *roleVar,
                             const CsrBtDeviceAddr *addr)
{
    CsrUint8 i;
    CsrBool present = FALSE;

    for (i = 0; i < NUM_OF_ACL_CONNECTION; i++)
    {
        if (CsrBtBdAddrEq(&roleVar->aclVar[i].deviceAddr, addr))
        {
            present = TRUE;
            break;
        }
    }

    CsrBtMarshalUtilConvertObj(conv, present);

    if (present)
    {
        convAclTable(conv, &roleVar->aclVar[i]);
    }
}

#ifdef CSR_BT_LE_ENABLE
static void convConnCache(CsrBtMarshalUtilInst *conv,
                             cmInstanceData_t *cmData,
                             const CsrBtTpdAddrT *tpAddrt)
{
    leConnVar *conn = CsrBtCmLeFindConn(cmData, &tpAddrt->addrt);
    CsrBool flag = conn ? TRUE : FALSE;

    CsrBtMarshalUtilConvertObj(conv, flag);

    if (flag)
    {
        if (!conn)
        {
            conn = CsrPmemAlloc(sizeof(*conn));
            conn->next = cmData->leVar.connCache;
            cmData->leVar.connCache = conn;
            cmConverter->connCacheCnt++;
            conn->addr = tpAddrt->addrt;
        }

        CsrBtMarshalUtilConvertObj(conv, conn->connParams);
        CsrBtMarshalUtilConvertObj(conv, conn->master);
        CsrBtMarshalUtilConvertObj(conv, conn->idAddr);
    }
}

static void deserLeVariables(CsrBtMarshalUtilInst *conv,
                             cmInstanceData_t *cmData,
                             const CsrBtTpdAddrT *tpAddrt)
{
    CsrUint8 bitfieldBlobOffset;
    CsrUint16 bitfieldBlobSize;
    void *bitfieldBlobPtr;

    convConnCache(conv, cmData, tpAddrt);

    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.params);

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.localStaticAddr);
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.staticAddrSet);
    bitfieldBlobOffset = CsrOffsetOf(leVariables, staticAddrSet) + sizeof(cmData->leVar.staticAddrSet);
#else
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.pvtAddrTimeout);
    bitfieldBlobOffset = CsrOffsetOf(leVariables, pvtAddrTimeout) + sizeof(cmData->leVar.pvtAddrTimeout);
#endif

    bitfieldBlobSize = sizeof(leVariables) - bitfieldBlobOffset;
    bitfieldBlobPtr = &((CsrUint8 *) &cmData->leVar)[bitfieldBlobOffset];

    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);
}

static void serLeVariables(CsrBtMarshalUtilInst *conv,
                             cmInstanceData_t *cmData,
                             const CsrBtTpdAddrT *tpAddrt)
{
    CsrUint8 bitfieldBlobOffset;
    CsrUint16 bitfieldBlobSize;
    void *bitfieldBlobPtr;

    convConnCache(conv, cmData, tpAddrt);

    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.params);

#ifdef CSR_BT_LE_RANDOM_ADDRESS_TYPE_STATIC
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.localStaticAddr);
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.staticAddrSet);
    bitfieldBlobOffset = CsrOffsetOf(leVariables, staticAddrSet) + sizeof(cmData->leVar.staticAddrSet);
#else
    CsrBtMarshalUtilConvertObj(conv, cmData->leVar.pvtAddrTimeout);
    bitfieldBlobOffset = CsrOffsetOf(leVariables, pvtAddrTimeout) + sizeof(cmData->leVar.pvtAddrTimeout);
#endif

    bitfieldBlobSize = sizeof(leVariables) - bitfieldBlobOffset;
    bitfieldBlobPtr = &((CsrUint8 *) &cmData->leVar)[bitfieldBlobOffset];
    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);
}
#endif /* #ifdef CSR_BT_LE_ENABLE */

static void convL2caConnInst(CsrBtMarshalUtilInst *conv, cmL2caConnInstType *l2caConnInst)
{
    const CsrUint8 bitfieldBlobOffset = CsrOffsetOf(cmL2caConnInstType, outgoingFlush) + sizeof(l2caConnInst->outgoingFlush);
    const CsrUint16 bitfieldBlobSize = sizeof(cmL2caConnInstType) - bitfieldBlobOffset;
    void *bitfieldBlobPtr = &((CsrUint8 *) l2caConnInst)[bitfieldBlobOffset];

    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->appHandle);
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->remotePsm);

    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->outgoingMtu);
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->incomingMtu);
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->outgoingFlush);

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->ssrSettings);
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->ssrAccepted);
#endif

#ifndef EXCLUDE_CSR_BT_CM_LEGACY_PAIRING_DETACH
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->secLevel);
#endif

    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);
}

static void deserL2caConnInst(CsrBtMarshalUtilInst *conv, cmL2caConnInstType *l2caConnInst)
{
    convL2caConnInst(conv, l2caConnInst);
}

static void serL2caConnInst(CsrBtMarshalUtilInst *conv, cmL2caConnInstType *l2caConnInst)
{
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->btConnId);
    CsrBtMarshalUtilConvertObj(conv, l2caConnInst->psm);

    convL2caConnInst(conv, l2caConnInst);
}

static void l2caConnInstSer(CsrCmnListElm_t *elem, void *data)
{
    cmL2caConnElement *l2caConnElem = (cmL2caConnElement *) elem;
    csrBtCmDeviceConnConvertRef *ref = (csrBtCmDeviceConnConvertRef *) data;

    if (l2caConnElem->cmL2caConnInst)
    {
        if (l2caConnElem->cmL2caConnInst->transportType == ref->tpAddr->tp_type &&
            CsrBtBdAddrEq(&l2caConnElem->cmL2caConnInst->deviceAddr, &ref->tpAddr->addrt.addr))
        {
            serL2caConnInst(ref->conv, l2caConnElem->cmL2caConnInst);
        }
    }
}

static void deserL2caVariables(CsrBtMarshalUtilInst *conv,
                               l2CaVariables *l2caVar,
                               const CsrBtTpdAddrT *tpAddrt)
{
    csrBtCmDeviceConnInfoRef *device = GET_CURRENT_DEVICE(cmConverter);

    if (device)
    {
        CsrUint8 i;
        cmL2caConnElement *l2caElem = NULL;

        CsrBtMarshalUtilConvertObj(conv, device->l2ca.count);

        if (!device->l2ca.connInfo)
        {
            device->l2ca.connInfo = CsrPmemZalloc(sizeof(*device->l2ca.connInfo) * device->l2ca.count);
        }

        for (i = 0; i < device->l2ca.count; i++)
        {
            CsrBtMarshalUtilConvertObj(conv, device->l2ca.connInfo[i].connId);
            CsrBtMarshalUtilConvertObj(conv, device->l2ca.connInfo[i].psm);

            if (CsrBtMarshalUtilStatus(cmConverter->conv))
            { /* Proceed only if connId and psm has been successfully deserialized */
                cmInstanceData_t *cmData = CmGetInstanceDataPtr();

                l2caElem = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId,
                                                &device->l2ca.connInfo[i].connId);
                if (!l2caElem)
                {
                    l2caElem = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromReserveBtConnIdPsm,
                                                    &device->l2ca.connInfo[i].psm);
                }

                if (!l2caElem)
                {
                    if (CsrBtCmElementCounterIncrement(cmData))
                    {
                        l2caElem = (cmL2caConnElement*) CsrCmnListElementAddLast(&l2caVar->connList,
                                                                                 sizeof(*l2caElem));
                        l2caElem->elementId = cmData->elementCounter;

                        /* Mark this element as added due to unmarshalling so that
                         * it can be removed in case of abort */
                        l2caElem->unmarshalledAdded = TRUE;
                    }
                    else
                    {
                        CsrPanic(CSR_TECH_BT,
                                 CSR_BT_PANIC_MYSTERY,
                                 "CsrBtCmElementCounterIncrement Failed");
                    }
                }

                deserL2caConnInst(conv, l2caElem->cmL2caConnInst);

                /* Mark this element as filled with unmarshalled data so that it
                 * can be reset in case of abort */
                l2caElem->unmarshalledFilled = TRUE;

                l2caElem->cmL2caConnInst->btConnId      = device->l2ca.connInfo[i].connId;
                l2caElem->cmL2caConnInst->psm           = device->l2ca.connInfo[i].psm;
                l2caElem->cmL2caConnInst->deviceAddr    = tpAddrt->addrt.addr;
                l2caElem->cmL2caConnInst->addressType   = tpAddrt->addrt.type;
                l2caElem->cmL2caConnInst->transportType = tpAddrt->tp_type;
            }
        }
        /* Handover gets completed in non-transitioned state only, hence there shall not be any active element. */
        l2caVar->activeElemId = CM_ERROR;
    }
}

static void serL2caVariables(CsrBtMarshalUtilInst *conv,
                             l2CaVariables *l2caVar,
                             const CsrBtTpdAddrT *tpAddr)
{
    csrBtCmDeviceConnCountRef countRef;
    csrBtCmDeviceConnConvertRef convRef;

    countRef.tpAddr = tpAddr;
    countRef.count = 0;
    CsrCmnListIterate((CsrCmnList_t*) &l2caVar->connList,
                      cmL2caConnInstCount,
                      &countRef);
    CsrBtMarshalUtilConvertObj(conv, countRef.count);

    convRef.tpAddr = tpAddr;
    convRef.conv = conv;
    CsrCmnListIterate((CsrCmnList_t*) &l2caVar->connList,
                      l2caConnInstSer,
                      &convRef);
}

static void convRfcConnInst(CsrBtMarshalUtilInst *conv, cmRfcConnInstType *rfcConnInst)
{
    const CsrUint8 bitfieldBlobOffset = CsrOffsetOf(cmRfcConnInstType, mscTimeout) + sizeof(rfcConnInst->mscTimeout);
    const CsrUint16 bitfieldBlobSize = sizeof(cmRfcConnInstType) - bitfieldBlobOffset;
    void *bitfieldBlobPtr = &((CsrUint8 *) rfcConnInst)[bitfieldBlobOffset];

    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->appHandle);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->classOfDevice);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->remoteServerChan);

    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->profileMaxFrameSize);

    CsrBtMarshalUtilConvert(conv, bitfieldBlobPtr, bitfieldBlobSize);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    {
        CsrBool present = rfcConnInst->eScoParms ? TRUE : FALSE;

        CsrBtMarshalUtilConvertObj(conv, present);

        if (present)
        {
            if (!rfcConnInst->eScoParms)
            {
                rfcConnInst->eScoParms = CsrPmemZalloc(sizeof(*rfcConnInst->eScoParms));
            }

            CsrBtMarshalUtilConvertObj(conv, *rfcConnInst->eScoParms);
        }
    }
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1 && defined(CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS)
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->ssrSettings);
#endif

    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->modemStatus);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->signalBreak);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->mscTimeout);
}

static void deserRfcConnInst(CsrBtMarshalUtilInst *conv, cmRfcConnInstType *rfcConnInst)
{
    convRfcConnInst(conv, rfcConnInst);

#ifndef EXCLUDE_CSR_BT_SCO_MODULE
    /* This is not really marshalled, so ensure that it is NULL */
    if (rfcConnInst->eScoParms)
    {
        rfcConnInst->eScoParms->negotiateCnt = NULL;
    }
#endif
}

static void serRfcConnInst(CsrBtMarshalUtilInst *conv, cmRfcConnInstType *rfcConnInst)
{
    CsrUint8 dlci;
    Sink sink = StreamRfcommSink(CM_GET_UINT16ID_FROM_BTCONN_ID(rfcConnInst->btConnId));

    if (sink == NULL)
    {
        CsrPanic(CSR_TECH_BT,
                 CSR_BT_PANIC_MYSTERY,
                 "Sink is NULL");
    }

    dlci = SinkGetRfcommDlci(sink);
    CsrBtMarshalUtilConvertObj(conv, dlci);

    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->btConnId);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->serverChannel);
    CsrBtMarshalUtilConvertObj(conv, rfcConnInst->context);

    convRfcConnInst(conv, rfcConnInst);
}

static void rfcConnInstSer(CsrCmnListElm_t *elem, void *data)
{
    cmRfcConnElement *rfcConnElem = (cmRfcConnElement *) elem;
    csrBtCmDeviceConnConvertRef *ref = (csrBtCmDeviceConnConvertRef *) data;

    if (rfcConnElem->cmRfcConnInst)
    {
        if (CsrBtBdAddrEq(&rfcConnElem->cmRfcConnInst->deviceAddr, &ref->tpAddr->addrt.addr))
        {
            serRfcConnInst(ref->conv, rfcConnElem->cmRfcConnInst);
        }
    }
}

static void deserRfcVariables(CsrBtMarshalUtilInst *conv,
                              rfcVariables *rfcVar,
                              const CsrBtDeviceAddr *addr)
{
    csrBtCmDeviceConnInfoRef *device = GET_CURRENT_DEVICE(cmConverter);

    if (device)
    {
        CsrUint8 i;
        cmRfcConnElement *rfcElem = NULL;

        CsrBtMarshalUtilConvertObj(conv, device->rfc.count);

        if (!device->rfc.connInfo)
        {
            device->rfc.connInfo = CsrPmemZalloc(sizeof(*device->rfc.connInfo) * device->rfc.count);
        }

        for (i = 0; i < device->rfc.count; i++)
        {
            CsrUint8 dlci;
            CsrBtMarshalUtilConvertObj(conv, dlci);
            CsrBtMarshalUtilConvertObj(conv, device->rfc.connInfo[i].connId);
            CsrBtMarshalUtilConvertObj(conv, device->rfc.connInfo[i].channel);
            CsrBtMarshalUtilConvertObj(conv, device->rfc.connInfo[i].context);

            if (CsrBtMarshalUtilStatus(cmConverter->conv))
            {
                cmConnIdServerType connRef;
                cmInstanceData_t *cmData = CmGetInstanceDataPtr();

                connRef.bt_conn_id = device->rfc.connInfo[i].connId;
                connRef.server = device->rfc.connInfo[i].channel;

                rfcElem = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdServer,
                                              &connRef);
                if (!rfcElem)
                {
                    cmConnIdLocalRemoteServersContextType searchData;
                    CsrBtDeviceAddr zeroBdAddr = { 0 };

                    /* We do not have a connection Id stored yet, as it is known first now;
                     * therefore search for the server channel and the "no-connection-id" field.
                     * And remote server is CSR_BT_NO_SERVER for the context received from peer.
                     * Context acts a link between RFC element and the application instance. Hence,
                     * this needs to be same as the one received in the handover data.*/
                    searchData = CsrBtCmReturnConnIdLocalRemoteServersContextStruct(CSR_BT_CONN_ID_INVALID,
                                                                                    device->rfc.connInfo[i].channel,
                                                                                    CSR_BT_NO_SERVER,
                                                                                    zeroBdAddr,
                                                                                    device->rfc.connInfo[i].context);

                    rfcElem  = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdLocalRemoteServersContext,
                                                   &searchData);
                }

                if (!rfcElem)
                {
                    if (CsrBtCmElementCounterIncrement(cmData))
                    {
                        rfcElem = (cmRfcConnElement *) CsrCmnListElementAddLast(&rfcVar->connList,
                                                                                sizeof(*rfcElem));
                        rfcElem->elementId = cmData->elementCounter;

                        /* Mark this element as added due to unmarshalling so that
                         * it can be removed in case of abort */
                        rfcElem->unmarshalledAdded = TRUE;
                    }
                    else
                    {
                        CsrPanic(CSR_TECH_BT,
                                 CSR_BT_PANIC_MYSTERY,
                                 "CsrBtCmElementCounterIncrement Failed");
                        return;
                    }
                }

                rfcElem->cmRfcConnInst->dlci = dlci;
                deserRfcConnInst(conv, rfcElem->cmRfcConnInst);

                /* Mark this element as filled with unmarshalled data so that it
                 * can be reset in case of abort */
                rfcElem->unmarshalledFilled = TRUE;

                rfcElem->cmRfcConnInst->deviceAddr = *addr;
                rfcElem->cmRfcConnInst->btConnId = device->rfc.connInfo[i].connId;
                rfcElem->cmRfcConnInst->serverChannel = device->rfc.connInfo[i].channel;
                rfcElem->cmRfcConnInst->context = device->rfc.connInfo[i].context;
            }
        }
        /* Handover gets completed in non-transitioned state only, hence there shall not be any active element. */
        rfcVar->activeElemId = CM_ERROR;
    }
}

static void serRfcVariables(CsrBtMarshalUtilInst *conv,
                            rfcVariables *rfcVar,
                            const CsrBtTpdAddrT *tpAddr)
{
    csrBtCmDeviceConnCountRef countRef;
    csrBtCmDeviceConnConvertRef convRef;

    countRef.tpAddr = tpAddr;
    countRef.count = 0;
    CsrCmnListIterate((CsrCmnList_t*) &rfcVar->connList,
                      cmRfcConnInstCount,
                      &countRef);
    CsrBtMarshalUtilConvertObj(conv, countRef.count);

    convRef.tpAddr = tpAddr;
    convRef.conv = conv;
    CsrCmnListIterate((CsrCmnList_t*) &rfcVar->connList,
                      rfcConnInstSer,
                      &convRef);
}

static void convDmVariables(CsrBtMarshalUtilInst *conv, dmVariables *dmVar)
{
    CsrBtMarshalUtilConvertObj(conv, dmVar->activePlayer);
    CsrBtMarshalUtilConvertObj(conv, dmVar->codWrittenToChip);
    CsrBtMarshalUtilConvertObj(conv, dmVar->seqNumber);
}

static void deserDmVariables(CsrBtMarshalUtilInst *conv,
                             dmVariables *dmVar,
                             const CsrBtDeviceAddr *addr)
{
    convDmVariables(conv, dmVar);
    CSR_UNUSED(addr);
}

static void serDmVariables(CsrBtMarshalUtilInst *conv,
                           dmVariables *dmVar,
                           const CsrBtDeviceAddr *addr)
{
    convDmVariables(conv, dmVar);
    CSR_UNUSED(addr);
}

static void deserCmInstanceData(CsrBtMarshalUtilInst *conv,
                                cmInstanceData_t *cmData,
                                const CsrBtTpdAddrT *tpAddrt)
{
    deserDmVariables(conv, &cmData->dmVar, &tpAddrt->addrt.addr);

    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_BREDR)
    {
        deserRfcVariables(conv, &cmData->rfcVar, &tpAddrt->addrt.addr);
    }

    deserL2caVariables(conv, &cmData->l2caVar, tpAddrt);

    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_BREDR)
    {
        deserRoleVariables(conv, &cmData->roleVar, &tpAddrt->addrt.addr);
    }

#ifdef CSR_BT_LE_ENABLE
    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_LE)
    {
        deserLeVariables(conv, cmData, tpAddrt);
    }
#endif
}

static void serCmInstanceData(CsrBtMarshalUtilInst *conv,
                              cmInstanceData_t *cmData,
                              const CsrBtTpdAddrT *tpAddrt)
{
    serDmVariables(conv, &cmData->dmVar, &tpAddrt->addrt.addr);

    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_BREDR)
    {
        serRfcVariables(conv, &cmData->rfcVar, tpAddrt);
    }

    serL2caVariables(conv, &cmData->l2caVar, tpAddrt);

    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_BREDR)
    {
        serRoleVariables(conv, &cmData->roleVar, &tpAddrt->addrt.addr);
    }

#ifdef CSR_BT_LE_ENABLE
    if (tpAddrt->tp_type == CSR_BT_TRANSPORT_LE)
    {
       serLeVariables(conv, cmData, tpAddrt);
    }
#endif
}

static bool cmHandoverRfcConnIdUpdate(cmRfcConnElement *rfcElem,
                                      const CsrBtTpdAddrT *tpAddrt)
{
    Sink sink;
    tp_bdaddr vmTpAddrt = { 0 };
    CsrUint16 cid = L2CA_CID_INVALID;

    BdaddrConvertTpBluestackToVm(&vmTpAddrt, tpAddrt);

    /* Fetch sink from dlci to get cid */
    sink = StreamRfcommSinkFromDlci(&vmTpAddrt, rfcElem->cmRfcConnInst->dlci);

    /* Extract new CID for the connection instance */
    if (sink)
    {
        cid = SinkGetRfcommConnId(sink);

        /* Update the connection ID */
        rfcElem->cmRfcConnInst->btConnId = CM_CREATE_RFC_CONN_ID(cid);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void cmHandoverCommitRfc(const csrBtCmDeviceConnInfoRef *device)
{
    CsrUint8 i;
        
    for (i = 0; i < device->rfc.count; i++)
    {
        cmConnIdServerType connRef;
        cmRfcConnElement *rfcElem;
        cmInstanceData_t *cmData = CmGetInstanceDataPtr();

        /* Find connection element */
        connRef.bt_conn_id = device->rfc.connInfo[i].connId;
        connRef.server = device->rfc.connInfo[i].channel;
        rfcElem = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromConnIdServer,
                                      &connRef);

        if (cmHandoverRfcConnIdUpdate(rfcElem, &device->tpAddrt) &&
            VmOverrideRfcommConnContext(CM_GET_UINT16ID_FROM_BTCONN_ID(rfcElem->cmRfcConnInst->btConnId),
                                        CSR_BT_CM_CONTEXT_UNUSED))
        {
            /* Update the RFCOMM channel registration in Bluestack */
            rfcElem->unmarshalledAdded = FALSE;
            rfcElem->unmarshalledFilled = FALSE;
        }
        else
        {
            /* Channel registration has failed. This could happen if either incorrect channel has
             * been marshalled by peer device or this device has somehow retained an old unused
             * channel id which did not get cleaned up in last handover.
             * In either case, we can remove the connection instance. */
            CSR_BT_CM_HANDOVER_LOG_ERROR("cmHandoverCommitRfc Failed for RFC DLCI:0x02%x. Unmarshalling Status(Added:%d, Filled:%d)",
                                         rfcElem->cmRfcConnInst->dlci, rfcElem->unmarshalledAdded, rfcElem->unmarshalledFilled);
            if (cmHandoverUnmarshalledRfcInstRemove((CsrCmnListElm_t*)rfcElem, (void*)NULL))
            {
                CsrCmnListElementRemove(&cmData->rfcVar.connList, (CsrCmnListElm_t*)rfcElem);
            }
        }
    }
}

static void cmHandoverCommitL2ca(const csrBtCmDeviceConnInfoRef *device)
{
    CsrUint8 i;

    for (i = 0; i < device->l2ca.count; i++)
    {
        cmInstanceData_t *cmData = CmGetInstanceDataPtr();
        cmL2caConnElement *l2caElem = CM_FIND_L2CA_ELEMENT(CsrBtCmL2caFindL2caConnElementFromBtConnId,
                                                           &device->l2ca.connInfo[i].connId);

        if (VmOverrideL2capConnContext(CM_GET_UINT16ID_FROM_BTCONN_ID(l2caElem->cmL2caConnInst->btConnId),
                                       CSR_BT_CM_CONTEXT_UNUSED))
        {
            l2caElem->unmarshalledAdded = FALSE;
            l2caElem->unmarshalledFilled = FALSE;

        }
        else
        {
            /* Channel registration has failed. This could happen if either incorrect channel has
             * been marshalled by peer device or this device has somehow retained an old unused
             * channel id which did not get cleaned up in last handover.
             * In either case, we can remove the connection instance. */
            CSR_BT_CM_HANDOVER_LOG_ERROR("VmOverrideL2capConnContext Failed for L2CA connid:0x02%x. Unmarshalling Status(Added:%d, Filled:%d)",
                                         l2caElem->cmL2caConnInst->btConnId, l2caElem->unmarshalledAdded, l2caElem->unmarshalledFilled);
            if (cmHandoverUnmarshalledL2capInstRemove((CsrCmnListElm_t*)l2caElem, (void*)NULL))
            {
                CsrCmnListElementRemove(&cmData->l2caVar.connList, (CsrCmnListElm_t*)l2caElem);
            }
        }
    }
}

static void cmHandoverDeinit(bool onAbort)
{
    if (cmConverter)
    {

        CsrUint8 i;
        for (i = 0; i < MAX_NUM_OF_LINK_UNMARSHAL ; i++)
        {
            if (cmConverter->device[i])
            {
                if (onAbort)
                {
                    cmHandoverUnmarshalledInstRemove(cmConverter->device[i]->tpAddrt);
                }

                CsrPmemFree(cmConverter->device[i]->l2ca.connInfo);
                CsrPmemFree(cmConverter->device[i]->rfc.connInfo);
                CsrPmemFree(cmConverter->device[i]);
            }
        }

        CsrBtMarshalUtilDestroy(cmConverter->conv);
        CsrPmemFree(cmConverter);
        cmConverter = NULL;
    }
}

/* Return TRUE if a message is a mode change event for a given link identified by bd address, FALSE otherwise. */
static CsrBool csrBtCmIsModeChangeMessage(Task task, MessageId id, Message message)
{
    const DM_HCI_MODE_CHANGE_EVENT_IND_T *prim = (DM_HCI_MODE_CHANGE_EVENT_IND_T *) message;
    CSR_UNUSED(task);

    /* Check if there is any pending mode change event for the current link is present or not. */
    return (id == MESSAGE_BLUESTACK_DM_PRIM &&
            prim->type == DM_HCI_MODE_CHANGE_EVENT_IND &&
            currentLinkAddr.tp_type == BREDR_ACL &&
            CsrBtBdAddrEq(&currentLinkAddr.addrt.addr, &prim->bd_addr));
}

/* Return TRUE if the message is not allowed to be on the pending queue at the time of handover, FALSE otherwise. */
static CsrBool csrBtCmIsDisallowedMessage(Task task, MessageId id, Message message)
{
    CsrBool ret = TRUE;
    CSR_UNUSED(task);

    if (id == MESSAGE_BLUESTACK_DM_PRIM)
    {
        const DM_UPRIM_T *prim = message;
        if (prim->type == DM_HCI_ULP_BIGINFO_ADV_REPORT_IND ||
            prim->type == DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND ||
            prim->type == DM_HCI_MODE_CHANGE_EVENT_IND)
        {
            ret = FALSE;
        }
    }
    else if (id == MESSAGE_MORE_DATA)
    {
        ret = FALSE;
    }

    return ret;
}

static bool csrBtCmVeto(void)
{
    bool veto = FALSE;
    cmInstanceData_t *cmData  = CmGetInstanceDataPtr();

    if (cmData->recvMsgP != NULL)
    { /* CM has pending requests, veto Handover  */
        veto = TRUE;
    }
    else if (cmData->rfcVar.connectState != CM_RFC_IDLE &&
        cmData->rfcVar.connectState != CM_RFC_CONNECTED)
    { /* RFC connection in transient state, veto Handover  */
        veto = TRUE;
    }
    else if (cmData->l2caVar.connectState != CM_L2CA_IDLE &&
        cmData->l2caVar.connectState != CM_L2CA_CONNECT &&
        cmData->l2caVar.connectState != CM_L2CA_CONNECT_PENDING)
    { /* L2CAP connection in transient state, veto Handover  */
        veto = TRUE;
    }
    else if (cmData->dmVar.lockMsg != CSR_BT_CM_DM_QUEUE_UNLOCK)
    { /* DM Queue locked, in transient state, veto Handover  */
        veto = TRUE;
    }
    else if (cmData->smVar.smInProgress)
    { /* SM Queue locked, in transient state, veto Handover  */
        veto = TRUE;
    }
#ifdef CSR_BT_LE_ENABLE
    else if (cmData->leVar.scanMode != CSR_BT_CM_LE_MODE_OFF ||
             cmData->leVar.advMode != CSR_BT_CM_LE_MODE_OFF)
    { /* CM is in scanning or advertising, veto Handover */
        veto = TRUE;
    }
#endif
    else
    {
        CsrUint16 disallowedMessages;

        /* Check if there are any messages in the queue which are disallowed during handover. */
        disallowedMessages = SynergySchedMessagePendingMatch(CSR_BT_CM_IFACEQUEUE, TRUE, (void *)csrBtCmIsDisallowedMessage);

        if (disallowedMessages)
        {
            CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmVeto number of disallowed messages: %d", disallowedMessages);
            veto = TRUE;
        }
    }
    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmVeto %d", veto);

    return veto;
}

static bool csrBtCmVetoLink(const tp_bdaddr *vmTpAddrt)
{
    CsrUint16 modeChangeEventMessages;

    BdaddrConvertTpVmToBluestack(&currentLinkAddr, vmTpAddrt);

    /* Count number of mode change events which are present in the queue for this link. */
    modeChangeEventMessages = SynergySchedMessagePendingMatch(CSR_BT_CM_IFACEQUEUE, TRUE, (void *)csrBtCmIsModeChangeMessage);

    /* Reset the current bd address for the next link. */
    CsrMemSet(&currentLinkAddr, 0x0, sizeof(currentLinkAddr));

    /* if there are one or more mode change events pending for the same link, handover needs to be vetoed. */
    return (modeChangeEventMessages == 1);
}

static bool csrBtCmMarshal(const tp_bdaddr *vmTpAddrt,
                           CsrUint8 *buf,
                           CsrUint16 length,
                           CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!cmConverter)
    {
        cmConverter = CsrPmemZalloc(sizeof(*cmConverter));
        cmConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(cmConverter->conv, length, buf, TRUE);

    serCmInstanceData(cmConverter->conv,
                      CmGetInstanceDataPtr(),
                      &tpAddrt);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(cmConverter->conv);

    return CsrBtMarshalUtilStatus(cmConverter->conv);
}

static bool csrBtCmUnmarshal(const tp_bdaddr *vmTpAddrt,
                             const CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!cmConverter)
    {
        cmConverter = CsrPmemZalloc(sizeof(*cmConverter));
        cmConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
        cmConverter->device[cmConverter->deviceIndex] = CsrPmemZalloc(sizeof(csrBtCmDeviceConnInfoRef));
        cmConverter->device[cmConverter->deviceIndex]->tpAddrt = tpAddrt;
    }
    else
    {
        /* Do not increment the index in case unmarshal resumes */
        if (!(CsrBtBdAddrEq(&cmConverter->device[cmConverter->deviceIndex]->tpAddrt.addrt.addr, &tpAddrt.addrt.addr) && 
            cmConverter->device[cmConverter->deviceIndex]->tpAddrt.tp_type == tpAddrt.tp_type))
        {
            cmConverter->deviceIndex++;
            cmConverter->device[cmConverter->deviceIndex] = CsrPmemZalloc(sizeof(csrBtCmDeviceConnInfoRef));
            cmConverter->device[cmConverter->deviceIndex]->tpAddrt = tpAddrt;
        }
    }

    CsrBtMarshalUtilResetBuffer(cmConverter->conv,
                                length,
                                (CsrUint8*) buf,
                                TRUE);

    deserCmInstanceData(cmConverter->conv,
                        CmGetInstanceDataPtr(),
                        &tpAddrt);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(cmConverter->conv);

    return CsrBtMarshalUtilStatus(cmConverter->conv);
}

static void csrBtCmHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                  bool newPrimary)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmHandoverCommit");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (newPrimary)
    {
        CsrUint8 deviceIndex;
        csrBtCmDeviceConnInfoRef *device = NULL;

        for (deviceIndex = 0; deviceIndex < MAX_NUM_OF_LINK_UNMARSHAL ; deviceIndex++)
        {
            if (CsrBtBdAddrEq(&cmConverter->device[deviceIndex]->tpAddrt.addrt.addr, &tpAddrt.addrt.addr) &&
                cmConverter->device[deviceIndex]->tpAddrt.tp_type == tpAddrt.tp_type)
            {
                break;
            }
        }

        device = GET_CURRENT_DEVICE_FROM_INDEX(cmConverter, deviceIndex);

        if (device && (device->tpAddrt.tp_type == CSR_BT_TRANSPORT_LE)) 
        {
           return;
        }

        if (device)
        {
            cmHandoverCommitRfc(device);
            cmHandoverCommitL2ca(device);
        }
        else
        {
            CsrPanic(CSR_TECH_BT,
                     CSR_BT_PANIC_MYSTERY,
                     "device is NULL");    
        }
    }
    else
    {
#if 0 /* No clean way to reinstate 'connect accept' instances */
        cmInstanceData_t *cmData = CmGetInstanceDataPtr();

        CsrCmnListIterateAllowRemove(&cmData->rfcVar.connList,
                                     cmRfcRemoveConnAddr,
                                     &tpAddrt.addrt.addr);

        CsrCmnListIterateAllowRemove(&cmData->l2caVar.connList,
                                     cmL2caRemoveConnAddr,
                                     &tpAddrt.addrt.addr);
#endif

    /* At New Secondary, handset connection will be removed in the  handling 
       of ACL close ind received during handover with reason(Link transferred) */
#if 0
        {
            leConnVar *conn;
            leConnVar **cppn;
            cmInstanceData_t *cmData = CmGetInstanceDataPtr();

            /* Remove the handset connection at the New Secondary */
            for (cppn = &(cmData->leVar.connCache); (conn = *cppn) != NULL; cppn = &(conn->next))
            {
                if (CsrBtAddrEq(&(conn->addr), &tpAddrt.addrt))
                {
                    *cppn = conn->next;
                    CsrPmemFree(conn);
                    break;
                }
            }
        }
#endif
        cmHandoverAclInstRemove(tpAddrt);
    }
}

static void csrBtCmHandoverComplete(bool newPrimary)
{
    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmHandoverComplete");

    cmHandoverDeinit(FALSE);

    CSR_UNUSED(newPrimary);
}

static void csrBtCmHandoverAbort(void)
{
    CSR_BT_CM_HANDOVER_LOG_INFO("csrBtCmHandoverAbort");

#ifdef CSR_BT_LE_ENABLE
    /* Note that abort() is called only once for multi point handover */
    if (cmConverter)
    {
        while (cmConverter->connCacheCnt)
        {
            cmInstanceData_t *cmData = CmGetInstanceDataPtr();

            leConnVar *next = cmData->leVar.connCache->next;
            CsrPmemFree(cmData->leVar.connCache);
            cmData->leVar.connCache = next;
            cmConverter->connCacheCnt--;
        }
    }
#endif

    cmHandoverDeinit(TRUE);
}

const handover_interface csr_bt_cm_handover_if =
        MAKE_HANDOVER_IF_VPL(&csrBtCmVeto,
                             &csrBtCmVetoLink,
                             &csrBtCmMarshal,
                             &csrBtCmUnmarshal,
                             &csrBtCmHandoverCommit,
                             &csrBtCmHandoverComplete,
                             &csrBtCmHandoverAbort);

void CsrBtCmUpdateScoHandle(CsrBtConnId connId,
                            hci_connection_handle_t scoHandle)
{
    cmRfcConnElement *rfcElem;
    cmInstanceData_t *cmData = CmGetInstanceDataPtr();

    /* Find connection element */
    rfcElem = CM_FIND_RFC_ELEMENT(CsrBtCmRfcFindRfcConnElementFromBtConnId,
                                  &connId);

    if (rfcElem && rfcElem->cmRfcConnInst && rfcElem->cmRfcConnInst->eScoParms)
    {
        rfcElem->cmRfcConnInst->eScoParms->handle = scoHandle;
        if (scoHandle != NO_SCO && scoHandle != SCOBUSY_ACCEPT)
        {
            /* Update the SCO channel registration in Bluestack */
            if (!VmOverrideSyncConnContext(scoHandle, CSR_BT_CM_CONTEXT_UNUSED))
            {                
                /* This could happen if SCO has been disconnected right after P0 handover complete, 
                 * before application had a chance to register the SCO handle. In this case P1 will
                 * not get any event of SCO disconnection. To cleanup, we need to internally send 
                 * a disconnection event.
                 */            
                CSR_BT_CM_HANDOVER_LOG_ERROR("CsrBtCmUpdateScoHandle: SCO Channel Registration Failed for Handle:0x%02x!!", scoHandle);
                CsrBtCmDmSyncDisconnectRfcHandler(cmData, rfcElem->cmRfcConnInst, HCI_SUCCESS, HCI_ERROR_OETC_USER);
            }
        }
    }
}
