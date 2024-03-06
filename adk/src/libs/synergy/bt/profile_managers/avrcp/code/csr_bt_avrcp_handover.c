/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_bt_util.h"
#include "csr_bt_panic.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_streams.h"

#ifdef CSR_LOG_ENABLE
#define CSR_BT_AVRCP_LTSO_HANDOVER              0
#define CSR_BT_AVRCP_HANDOVER_LOG_INFO(...)     CSR_LOG_TEXT_INFO((CsrBtAvrcpLto, CSR_BT_AVRCP_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_AVRCP_HANDOVER_LOG_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtAvrcpLto, CSR_BT_AVRCP_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_AVRCP_HANDOVER_LOG_ERROR(...)    CSR_LOG_TEXT_ERROR((CsrBtAvrcpLto, CSR_BT_AVRCP_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_AVRCP_HANDOVER_LOG_INFO(...)
#define CSR_BT_AVRCP_HANDOVER_LOG_WARNING(...)
#define CSR_BT_AVRCP_HANDOVER_LOG_ERROR(...)
#endif

static CsrBtMarshalUtilInst *avrcpConverter;

static void convAvrcpConnDetails(CsrBtMarshalUtilInst *conv, CsrBtAvrcpConnDetails *connDetails)
{
    CsrBtMarshalUtilConvertObj(conv, connDetails->btConnId);
    CsrBtMarshalUtilConvertObj(conv, connDetails->mtu);
    CsrBtMarshalUtilConvertObj(conv, connDetails->state);
    CsrBtMarshalUtilConvertObj(conv, connDetails->ctTLabel);
}

static void deserAvrcpConnDetails(CsrBtMarshalUtilInst *conv, CsrBtAvrcpConnDetails *connDetails)
{
    CsrBool dataSendAllowed = connDetails->dataSendAllowed; /* In case dataSendAllowed is already read before an unmarshal resume */

    convAvrcpConnDetails(conv, connDetails);

    CsrBtMarshalUtilConvertObj(conv, dataSendAllowed);
    connDetails->dataSendAllowed = dataSendAllowed;
}

static void serAvrcpConnDetails(CsrBtMarshalUtilInst *conv, CsrBtAvrcpConnDetails *connDetails)
{
    CsrBool dataSendAllowed = connDetails->dataSendAllowed;

    convAvrcpConnDetails(conv, connDetails);

    CsrBtMarshalUtilConvertObj(conv, dataSendAllowed);
}

static void convAvrcpConnInst(CsrBtMarshalUtilInst *conv, AvrcpConnInstance_t *connInst)
{
    CsrBtMarshalUtilConvertObj(conv, connInst->appConnId);
    CsrBtMarshalUtilConvertObj(conv, connInst->sdpState);
    CsrBtMarshalUtilConvertObj(conv, connInst->connDirection);
    CsrBtMarshalUtilConvertObj(conv, connInst->remoteFeatures);
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->notiList);
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->ctRequestedNotifications);
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->notiConfig);
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->activeNotifications);
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->tgSdpAvrcpVersion);
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrBtMarshalUtilConvertObj(conv, connInst->ctLocal->tgSdpSupportedFeatures);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
    CsrBtMarshalUtilConvertObj(conv, connInst->tgLocal->notificationsActive);
    CsrBtMarshalUtilConvertObj(conv, connInst->tgLocal->notiList);
}

static void deserAvrcpConnInst(CsrBtMarshalUtilInst *conv, AvrcpConnInstance_t *connInst)
{
    convAvrcpConnInst(conv, connInst);

    deserAvrcpConnDetails(conv, &connInst->control);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    deserAvrcpConnDetails(conv, &connInst->browsing);
#endif
}

static void serAvrcpConnInst(CsrBtMarshalUtilInst *conv, AvrcpConnInstance_t *connInst)
{
    convAvrcpConnInst(conv, connInst);

    serAvrcpConnDetails(conv, &connInst->control);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    serAvrcpConnDetails(conv, &connInst->browsing);
#endif
}

static void convAvrcpInstData(CsrBtMarshalUtilInst *conv, AvrcpInstanceData_t *avrcpInst)
{
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->activateStateCont);
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->incomingCurrent);
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->mtu);
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->ctLocal);
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->srAvrcpVersionHighest);
#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->numActiveAvrcpConns);
#endif
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    CsrBtMarshalUtilConvertObj(conv, avrcpInst->activateStateBrow);
#endif
}

static void deserAvrcpInstData(CsrBtMarshalUtilInst *conv,
                               AvrcpInstanceData_t *avrcpInst,
                               const CsrBtDeviceAddr *addr)
{
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *) &avrcpInst->connList,
                                                             (CsrBtDeviceAddr *) addr);
    CsrBool connPresent = (connInst != NULL); /* In case, connPresent was already read before an unmarshal resume */

    convAvrcpInstData(conv, avrcpInst);

    CsrBtMarshalUtilConvertObj(conv, connPresent);

    if (connPresent)
    {
        if (!connInst)
        {
            connInst = CsrBtAvrcpUtilConnAdd(avrcpInst, (CsrBtDeviceAddr *) addr);
        }
        deserAvrcpConnInst(conv, connInst);
    }
}

static void serAvrcpInstData(CsrBtMarshalUtilInst *conv,
                             AvrcpInstanceData_t *avrcpInst,
                             const CsrBtDeviceAddr *addr)
{
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *) &avrcpInst->connList,
                                                             (CsrBtDeviceAddr *) addr);
    CsrBool connPresent = (connInst != NULL);

    convAvrcpInstData(conv, avrcpInst);

    CsrBtMarshalUtilConvertObj(conv, connPresent);

    if (connPresent)
    {
        serAvrcpConnInst(conv, connInst);
    }
}


static bool csrBtAvrcpVeto(void)
{
    bool veto = FALSE;

    if (csrBtAvrcpInstance.appState == AVRCP_STATE_APP_BUSY)
    {
         veto = TRUE;
    }
    else if (csrBtAvrcpInstance.saveQueue ||
             SynergySchedMessagesPendingForTask(CSR_BT_AVRCP_IFACEQUEUE, NULL) != 0)
    {
        /* If there are pending messages in either the synergy queue or savequeue of AVRCP, veto handover. */
        veto = TRUE;
    }

    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpVeto %d", veto);

    return veto;
}

static bool csrBtAvrcpMarshal(const tp_bdaddr *vmTpAddrt,
                              CsrUint8 *buf,
                              CsrUint16 length,
                              CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!avrcpConverter)
    {
        avrcpConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(avrcpConverter, length, buf, TRUE);

    serAvrcpInstData(avrcpConverter, &csrBtAvrcpInstance, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(avrcpConverter);

    return CsrBtMarshalUtilStatus(avrcpConverter);
}

static bool csrBtAvrcpUnmarshal(const tp_bdaddr *vmTpAddrt,
                                const CsrUint8 *buf,
                                CsrUint16 length,
                                CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!avrcpConverter)
    {
        avrcpConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(avrcpConverter, length, (CsrUint8 *) buf, TRUE);

    deserAvrcpInstData(avrcpConverter, &csrBtAvrcpInstance, &tpAddrt.addrt.addr);

    *written = length - CsrBtMarshalUtilRemainingLengthGet(avrcpConverter);

    return CsrBtMarshalUtilStatus(avrcpConverter);
}

static void csrBtAvrcpHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                     bool newPrimary)
{
    CsrBtTpdAddrT tpAddrt = { 0 };
    AvrcpConnInstance_t *connInst;

    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpHandoverCommit");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *) &csrBtAvrcpInstance.connList,
                                        &tpAddrt.addrt.addr);

    if (connInst)
    {
        if (newPrimary)
        {
            if (connInst->control.btConnId != CSR_BT_CONN_ID_INVALID)
            {
                CsrStreamsRegister(CM_GET_UINT16ID_FROM_BTCONN_ID(connInst->control.btConnId),
                                   L2CAP_ID,
                                   CSR_BT_AVRCP_IFACEQUEUE);

                CsrStreamsSourceHandoverPolicyConfigure(CM_GET_UINT16ID_FROM_BTCONN_ID(connInst->control.btConnId),
                                                        L2CAP_ID,
                                                        SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if (connInst->browsing.btConnId != CSR_BT_CONN_ID_INVALID)
                {
                    CsrStreamsRegister(CM_GET_UINT16ID_FROM_BTCONN_ID(connInst->browsing.btConnId),
                                       L2CAP_ID,
                                       CSR_BT_AVRCP_IFACEQUEUE);

                    CsrStreamsSourceHandoverPolicyConfigure(CM_GET_UINT16ID_FROM_BTCONN_ID(connInst->browsing.btConnId),
                                                            L2CAP_ID,
                                                            SOURCE_HANDOVER_ALLOW_WITHOUT_DATA);
                }
#endif
            }

#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS
            /* Accept incoming AVRCP connections  */
            if (csrBtAvrcpInstance.numActiveAvrcpConns < csrBtAvrcpInstance.incomingMaximum)
            {
                AVRCP_CHANGE_STATE(csrBtAvrcpInstance.activateStateCont, AVRCP_STATE_ACT_DEACTIVATED);
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
                if (connInst->browsing.btConnId != CSR_BT_CONN_ID_INVALID)
                { /* Browsing is connected, it means CM would have used existing l2cap connection element
                   * while un-marshalling this instance. If "activateStateBrow" is AVRCP_STATE_ACT_ACTIVATED
                   * then create an additional element to allow CM to accept further incoming connection. */
                    if (csrBtAvrcpInstance.activateStateBrow == AVRCP_STATE_ACT_ACTIVATED)
                    {
                        AVRCP_CHANGE_STATE(csrBtAvrcpInstance.activateStateBrow, AVRCP_STATE_ACT_DEACTIVATED);
                    }
                }
                else
                {
                    AVRCP_CHANGE_STATE(csrBtAvrcpInstance.activateStateBrow, AVRCP_STATE_ACT_ACTIVATED);
                }
#endif
                CsrBtAvrcpUtilConnectAccept(&csrBtAvrcpInstance);
            }
#endif
        }
        else
        { /* Remove the connection instance from new secondary */
#if 0
            CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *) connInst, NULL);
            AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *) &csrBtAvrcpInstance.connList,
                                   connInst);
#endif
        }
    }
}

static void csrBtAvrcpHandoverComplete(bool newPrimary)
{
    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpHandoverComplete");

    if (avrcpConverter)
    {
        CsrBtMarshalUtilDestroy(avrcpConverter);
        avrcpConverter = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void csrBtAvrcpHandoverAbort(void)
{
    CSR_BT_AVRCP_HANDOVER_LOG_INFO("csrBtAvrcpHandoverAbort");
    if (avrcpConverter && CsrBtMarshalUtilTypeGet(avrcpConverter) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        while (csrBtAvrcpInstance.connList.count)
        {
            AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_FIRST((CsrCmnList_t *) &csrBtAvrcpInstance.connList);
            CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t *) connInst, NULL);
            AVRCP_LIST_CONN_REMOVE((CsrCmnList_t *) &csrBtAvrcpInstance.connList,
                                    connInst);
        }
#ifdef CSR_BT_RESTRICT_MAX_PROFILE_CONNECTIONS        
        csrBtAvrcpInstance.numActiveAvrcpConns = 0;
#endif
    }
    csrBtAvrcpHandoverComplete(FALSE);
}

const handover_interface csr_bt_avrcp_handover_if =
        MAKE_BREDR_HANDOVER_IF(&csrBtAvrcpVeto,
                               &csrBtAvrcpMarshal,
                               &csrBtAvrcpUnmarshal,
                               &csrBtAvrcpHandoverCommit,
                               &csrBtAvrcpHandoverComplete,
                               &csrBtAvrcpHandoverAbort);

