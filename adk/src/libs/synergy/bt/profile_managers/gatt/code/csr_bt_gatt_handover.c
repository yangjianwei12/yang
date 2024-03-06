/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_synergy.h"
#include "csr_bt_gatt_private.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "csr_bt_gatt_private_utils.h"
#include "csr_bt_gatt_handover.h"


#ifdef CSR_LOG_ENABLE
#define CSR_BT_GATT_LTSO_HANDOVER             0
#define CSR_BT_GATT_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtGattLto, CSR_BT_GATT_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_GATT_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtGattLto, CSR_BT_GATT_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_GATT_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtGattLto, CSR_BT_GATT_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_GATT_HANDOVER_LOG_INFO(...)
#define CSR_BT_GATT_HANDOVER_LOG_WARNING(...)
#define CSR_BT_GATT_HANDOVER_LOG_ERROR(...)
#endif

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool gattUnmarshalled = FALSE;

static struct
{
    CsrBtMarshalUtilInst *conv;
    CsrBtConnId btConnId[GATT_MAX_CONNECTIONS]; /* Used during unmarshalling 
                                                 * side during abort */
} *gattConverter;


void CsrBtGattCallBackRegister(CsrBtGattCallbackFunctionPointer cb)
{
    gattMainInstance.cb = cb;
}

static void updateDeviceConnectedList(CsrBtConnId    btConnId)
{
    /* Identify if the btConnId passed is already available in the
     * list maintained in gattConverter. If yes, ignore it.
     * If no, then add an entry in the first available index
     * This is used only at unmarshalling side(in case of abort)
     */
    uint8 i;

    for (i = 0; i < GATT_MAX_CONNECTIONS; i ++)
    {
        if(btConnId == gattConverter->btConnId[i])
            return;
    }

    /* We need to store btConnId passed in list */
    for (i = 0; i < GATT_MAX_CONNECTIONS; i ++)
    {
        if(gattConverter->btConnId[i] == 0)
        {
            gattConverter->btConnId[i] = btConnId;
            break;
        }
    }
}

static void convGattConnInst(CsrBtMarshalUtilInst *conv, CsrBtGattConnElement *connInst)
{
    CsrBtMarshalUtilConvertObj(conv, connInst->peerAddr);
    CsrBtMarshalUtilConvertObj(conv, connInst->cid);
    CsrBtMarshalUtilConvertObj(conv, connInst->l2capFlags);
    CsrBtMarshalUtilConvertObj(conv, connInst->btConnId);
    CsrBtMarshalUtilConvertObj(conv, connInst->mtu);

    updateDeviceConnectedList(connInst->btConnId);
    CsrBtMarshalUtilConvertObj(conv, connInst->idAddr);
    CsrBtMarshalUtilConvertObj(conv, connInst->encrypted);

#ifdef CSR_BT_GATT_INSTALL_EATT
    CsrBtMarshalUtilConvertObj(conv, connInst->connFlags);
    CsrBtMarshalUtilConvertObj(conv, connInst->eattConnection);
    CsrBtMarshalUtilConvertObj(conv, connInst->numCidSucess);
    CsrBtMarshalUtilConvertObj(conv, connInst->cidSuccess);
    CsrBtMarshalUtilConvertObj(conv, connInst->numOfBearer);
    CsrBtMarshalUtilConvertObj(conv, connInst->localInitiated);
#endif

#ifdef GATT_CACHING_CLIENT_ROLE
    CsrBtMarshalUtilConvertObj(conv, connInst->serviceChangeHandle);
#endif
}

static void serGattConnInst(CsrBtMarshalUtilInst *conv, CsrBtGattConnElement *connInst)
{
    convGattConnInst(conv, connInst);
}

static void deserGattInst(CsrBtMarshalUtilInst *conv, CsrBtGattConnElement *connInst)
{
    convGattConnInst(conv, connInst);
}

static void deserGattInstData(CsrBtMarshalUtilInst *conv,
                              GattMainInst *gattMainInstance,
                              const CsrBtTypedAddr *addr)
{
    CsrBtGattConnElement *conn;
    CsrBool connPresent;

    conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(gattMainInstance->connInst,
                                                   CsrBtGattFindConnInstFromAddress,
                                                   addr);

    /* In case, connPresent was already read before an unmarshal resume */
    connPresent = conn ? TRUE : FALSE; 

    CsrBtMarshalUtilConvertObj(conv, connPresent);

    if (connPresent)
    {
        if (!conn)
        {
            conn = CSR_BT_GATT_CONN_INST_ADD_LAST(gattMainInstance->connInst);
        }

        deserGattInst(conv, conn);
    }

}

static void serGattInstData(CsrBtMarshalUtilInst *conv,
                            GattMainInst *gattMainInstance,
                            const CsrBtTypedAddr *addr)
{
    CsrBtGattConnElement *conn;
    CsrBool connPresent;

    conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(gattMainInstance->connInst,
                                                   CsrBtGattFindConnInstFromAddress,
                                                   addr);

    connPresent = (conn != NULL);

    CsrBtMarshalUtilConvertObj(conv, connPresent);

    if (connPresent)
    {
        serGattConnInst(conv, conn);
    }
}

static bool csrBtGattVeto(void)
{
    bool veto = FALSE;
#ifdef CSR_BT_GATT_INSTALL_EATT
    CsrUint8 i;
    CsrCmnListElm_t *elem;
    CsrCmnList_t *cmnList = &(gattMainInstance.connInst);
#endif
    /* Veto if there is any pending long write operation */

    if (CsrCmnListGetCount((CsrCmnList_t *) &(gattMainInstance.prepare)) > 0)
    {
        veto = TRUE;
        return veto;
    }
#ifdef CSR_BT_GATT_INSTALL_EATT

    for (i = 0; i < NO_OF_QUEUE; i++)
    { /* Veto if there is any pending request/response in any of the gatt msg queue */
        if (CsrCmnListGetCount((CsrCmnList_t *) &(gattMainInstance.queue[i])) > 0)
        {
            veto = TRUE;
            break;
        }
    }

    for (elem = cmnList->first; elem; elem = elem->next)
    { /* Veto if there is any pending request/response in access Ind queue of conn Inst */
        CsrBtGattConnElement *connInst = (CsrBtGattConnElement *)elem;

        if (CsrCmnListGetCount((CsrCmnList_t *) &(connInst->accessIndQueue)) > 0)
        {
            veto = TRUE;
            break;
        }
    }
#else
    /* Veto if there is any pending request/response in the gatt msg queue */
    if (CsrCmnListGetCount((CsrCmnList_t *) &(gattMainInstance.queue)) > 0)
    {
        veto = TRUE;
    }

#endif

    if (SynergySchedMessagesPendingForTask(CSR_BT_GATT_IFACEQUEUE, NULL) != 0)
    {
        /* Handover cannot proceed if we do not have an instance */
        veto = TRUE;
    }

    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattVeto %d", veto);

    return veto;
}

static bool csrBtGattMarshal(const tp_bdaddr *vmTpAddrt,
                           CsrUint8 *buf,
                           CsrUint16 length,
                           CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);
    if (!gattConverter)
    {
        gattConverter = CsrPmemZalloc(sizeof(*gattConverter));
        gattConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(gattConverter->conv, length, buf, TRUE);

    serGattInstData(gattConverter->conv, &gattMainInstance, &tpAddrt.addrt);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(gattConverter->conv);

    return CsrBtMarshalUtilStatus(gattConverter->conv);
}

static bool csrBtGattUnmarshal(const tp_bdaddr *vmTpAddrt,
                             const CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!gattConverter)
    {
        gattConverter = CsrPmemZalloc(sizeof(*gattConverter));
        gattConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(gattConverter->conv, length, (void *) buf, TRUE);

    deserGattInstData(gattConverter->conv, &gattMainInstance, &tpAddrt.addrt);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(gattConverter->conv);

    gattUnmarshalled = TRUE;

    return CsrBtMarshalUtilStatus(gattConverter->conv);
}

static void csrBtGattHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                  const bool newPrimary)
{
    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattHandoverCommit");

    if (newPrimary)
    { /* Trigger gatt client util callback function for it to populate
         gattClientUtilInst->addressList */
        CsrBtTypedAddr taddr;
        CsrBtGattConnElement* conn;

        taddr.type = vmTpAddrt->taddr.type;
        taddr.addr.lap = vmTpAddrt->taddr.addr.lap;
        taddr.addr.nap = vmTpAddrt->taddr.addr.nap;
        taddr.addr.uap = vmTpAddrt->taddr.addr.uap;

        /* Get the btConnId for the address to populate in
         * gattClientUtilIns->addressList 
         * btConnId will be present in conn instance.
         */
        conn = CSR_BT_GATT_CONN_INST_FIND_FROM_ADDRESS(gattMainInstance.connInst,
                                         CsrBtGattFindConnInstFromAddress,
                                         &taddr);

        gattMainInstance.cb(&taddr, conn->btConnId);
    }

    gattUnmarshalled = FALSE;
}

static void csrBtGattHandoverComplete(const bool newPrimary)
{
    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattHandoverComplete");

    if (gattConverter)
    {
        uint8 i;
        for(i = 0 ; i < GATT_MAX_CONNECTIONS; i++)
            gattConverter->btConnId[i] = 0;

        CsrBtMarshalUtilDestroy(gattConverter->conv);
        CsrPmemFree(gattConverter);
        gattConverter = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void csrBtGattHandoverAbort(void)
{
    CSR_BT_GATT_HANDOVER_LOG_INFO("csrBtGattHandoverAbort");

    if (gattUnmarshalled)
    {
        uint8 i;

        for( i = 0; i < GATT_MAX_CONNECTIONS; i++)
        {
            /* We need to remove all the conn instance which are already plugged in
             * gattMainInstance during unmarshal.
             * Get the "btConnId" from the list maintained in gattConverter.
             * This list is maintained during unmarshal procedure.
             */
            if(gattConverter->btConnId[i] != 0)
            {
                CsrBtGattConnElement *conn = 
                    CSR_BT_GATT_CONN_INST_FIND_CONNECTED_BT_CONN_ID(gattMainInstance.connInst, 
                                                                &(gattConverter->btConnId[i]));

                if(conn)
                {
                    /* Remove the conn element */
                    CSR_BT_GATT_CONN_INST_REMOVE(gattMainInstance.connInst, conn);
                }
            }
        }

        gattUnmarshalled = FALSE;
    }

    csrBtGattHandoverComplete(FALSE);
}

const handover_interface csr_bt_gatt_handover_if =
    MAKE_BLE_HANDOVER_IF(&csrBtGattVeto,
                         &csrBtGattMarshal,
                         &csrBtGattUnmarshal,
                         &csrBtGattHandoverCommit,
                         &csrBtGattHandoverComplete,
                         &csrBtGattHandoverAbort);

