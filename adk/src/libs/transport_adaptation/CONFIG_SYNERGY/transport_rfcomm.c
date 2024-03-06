/* Copyright (c) 2018 - 2022 Qualcomm Technologies International, Ltd. */
/*    */
/**
 * \file
 * Routines for managing a transport channel over RFCOMM
 */

#include <panic.h>
#include <print.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include <connection_no_ble.h>
#include "transport_rfcomm.h"
#include "transport_adaptation.h"
#include "transport_adaptation_common.h"
#include <connection_no_ble.h>

#include "cm_lib.h"
#include "csr_bt_td_db.h"
#include "logging.h"

#define RFCOMM_DEFAULT_CONFIG   (0)

/******************************************************************************
Utility function to convert bdaddr received from rfcomm connection ind
to tp_bdaddr format using trusted devices list.
*/
static void transportRfcommConvBdaddrToTpaddr(CsrBtDeviceAddr *addr,
                                              tp_bdaddr *tpaddr)
{
    /* Scan through the persistent storage to find a matching device profile. */
    if (CsrBtTdDbDeviceExists(CSR_BT_ADDR_PUBLIC, addr))
    {
        BdaddrConvertBluestackToVm(&tpaddr->taddr.addr, addr);
        tpaddr->taddr.type = CSR_BT_ADDR_PUBLIC;
    }

    tpaddr->transport = TRANSPORT_BREDR_ACL;
}

/******************************************************************************/
void transportRfcommRegister(Task task, uint8 server_channel)
{
    PRINT(("TRFCOMM:transportRfcommRegister\n"));
    ConnectionRfcommAllocateChannel(task, server_channel);
}

/******************************************************************************/
void transportRfcommDeregister(Task task, uint8 server_channel)
{
    PRINT(("TRFCOMM:transportRfcommDeregister\n"));

    CmContextCancelAcceptConnectReqSend(NULL, server_channel, 0);
    CmUnRegisterReqSend(server_channel);

    MAKE_TA_CLIENT_MESSAGE(TRANSPORT_DEREGISTER_CFM, msg);

    msg->status = SUCCESS;
    msg->transport = TRANSPORT_RFCOMM;
    msg->transport_id = (uint16) (server_channel);
    TA_SEND_DEREGISTER_CFM(msg);
    UNUSED(task);
}

/******************************************************************************/
void transportRfcommConnect(Task task,
                            const bdaddr* bd_addr,
                            uint16 local_transport_id,
                            uint8 remote_transport_id)
{
    CsrBtDeviceAddr addr;
    RFC_PORTNEG_VALUES_T portVals = {0};

    PRINT(("TRFCOMM:transportRfcommConnect\n"));

    BdaddrConvertVmToBluestack(&addr, bd_addr);

    CmConnectExtReqSend(task,
                        local_transport_id,
                        remote_transport_id,
                        CSR_BT_HF_PROFILE_DEFAULT_MTU_SIZE,
                        FALSE,
                        FALSE,
                        portVals,
                        CSR_BT__DEFAULT_OUTGOING_SECURITY_,
                        addr,
                        CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                        CSR_BT_DEFAULT_MODEM_STATUS,
                        CSR_BT_DEFAULT_MSC_TIMEOUT,
                        CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);
}

void transportRfcommConnectResponse(Task task,
                                    const bdaddr* bd_addr,
                                    uint16 transport_id,
                                    Sink sink,
                                    bool response)
{
    CsrBtDeviceAddr addr;

    PRINT(("TRFCOMM:transportRfcommConnectResponse\n"));

    BdaddrConvertVmToBluestack(&addr, bd_addr);

    CmRfcConnectAcceptRspSend(task,
                              CM_CREATE_RFC_CONN_ID(SinkGetRfcommConnId(sink)),
                              addr,
                              response,
                              transport_id,
                              CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                              CSR_BT_DEFAULT_BREAK_SIGNAL,
                              CSR_BT_DEFAULT_MSC_TIMEOUT);

}

/******************************************************************************/
void transportRfcommDisconnect(Task task, Sink sink)
{
    PRINT(("TRFCOMM:transportRfcommDisconnect\n"));
    ConnectionRfcommDisconnectRequest(task, sink);
}

/******************************************************************************/
void handleRfcommCmMessage(Task task, Message message)
{
    const CsrBtCmPrim *prim = (const CsrBtCmPrim *) message;

    PRINT(("handleRfcommCmMessage\n"));

    switch (*prim)
    {
        case CSR_BT_CM_REGISTER_CFM:
        {
            const CsrBtCmRegisterCfm *cfm = (const CsrBtCmRegisterCfm *) message;
            MAKE_TA_CLIENT_MESSAGE(TRANSPORT_REGISTER_CFM, msg);

            PRINT(("TRFCOMM:CSR_BT_CM_REGISTER_CFM\n"));

            if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                msg->status = SUCCESS;
            else
                msg->status = FAIL;

            msg->transport = TRANSPORT_RFCOMM;
            msg->transport_id = (uint16) (cfm->serverChannel);

            TA_SEND_REGISTER_CFM(msg);

            break;
        }

        case CSR_BT_CM_RFC_CONNECT_ACCEPT_IND:
        {
            const CsrBtCmRfcConnectAcceptInd *ind = (CsrBtCmRfcConnectAcceptInd *) message;
            tp_bdaddr tpaddr = { 0 };
            uint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId);
            Sink sink = StreamRfcommSink(cid);

            PRINT(("TRFCOMM:CSR_BT_CM_RFC_CONNECT_ACCEPT_IND\n"));
                
            MAKE_TA_CLIENT_MESSAGE(TRANSPORT_CONNECT_IND, msg);

            transportRfcommConvBdaddrToTpaddr(&ind->deviceAddr, &tpaddr);

            msg->transport    = TRANSPORT_RFCOMM;
            msg->transport_id = (uint16)(ind->localServerChannel);
            msg->sink         = sink;
            msg->addr         = tpaddr;
            msg->btConnId     = ind->btConnId;
            TA_SEND_CONNECT_IND(msg);

            break;
        }

        case CSR_BT_CM_CONNECT_ACCEPT_CFM:
        {
            const CsrBtCmConnectAcceptCfm *cfm = (const CsrBtCmConnectAcceptCfm *) message;
            tp_bdaddr tpaddr = { 0 };
            uint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(cfm->btConnId);
            Sink sink = StreamRfcommSink(cid);

            PRINT(("TRFCOMM:CSR_BT_CM_CONNECT_ACCEPT_CFM\n"));

            MAKE_TA_CLIENT_MESSAGE(TRANSPORT_CONNECT_CFM, msg);

            transportRfcommConvBdaddrToTpaddr(&cfm->deviceAddr, &tpaddr);

            msg->transport = TRANSPORT_RFCOMM;
            msg->transport_id = (uint16) (cfm->serverChannel);
            msg->sink = sink;
            msg->addr = tpaddr;
            msg->btConnId = cfm->btConnId;

            if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                msg->status = SUCCESS;
            }
            else
            {
                msg->status = FAIL;
            }

            TA_SEND_CONNECT_CFM(msg);

            break;
        }

        case CSR_BT_CM_DISCONNECT_IND:
        {
            const CsrBtCmDisconnectInd *ind = (const CsrBtCmDisconnectInd *) message;
            uint16 cid = CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId);
            Sink sink = StreamRfcommSink(cid);

            MAKE_TA_CLIENT_MESSAGE(TRANSPORT_DISCONNECT_CFM, msg);

            DEBUG_LOG_INFO("TRFCOMM:CSR_BT_CM_DISCONNECT_IND reasonSupplier: 0x%x, reasonCode: 0x%x\n",
                           ind->reasonSupplier, ind->reasonCode);

            if(!ind->localTerminated)
            {
                /* Response is required only for remote disconnection. */
                ConnectionRfcommDisconnectResponse(sink);
            }

            if (ind->reasonCode != RFC_SUCCESS && 
                ((ind->reasonSupplier == CSR_BT_SUPPLIER_RFCOMM &&
                 ind->reasonCode != RFC_NORMAL_DISCONNECT &&
                 ind->reasonCode != RFC_ABNORMAL_DISCONNECT &&
                 ind->reasonCode != RFC_LINK_TRANSFERRED) ||
                (ind->reasonSupplier == CSR_BT_SUPPLIER_L2CAP_DISCONNECT &&
                 ind->reasonCode != L2CA_DISCONNECT_LINK_LOSS)))
            {
                msg->status = FAIL;
            }
            else
            {
                msg->status = SUCCESS;
            }

            msg->transport = TRANSPORT_RFCOMM;
            msg->sink = sink;
            msg->btConnId = ind->btConnId;

            TA_SEND_DISCONNECT_CFM(msg);

            break;
        }

        case CSR_BT_CM_PORTNEG_IND:
        {
            CsrBtRespondCmPortNegInd((void *) message);
            break;
        }

            /*  Things to ignore  */
        case CSR_BT_CM_CONTROL_IND:
            break;

        default:
        {
            PRINT(("TRFCOMM:INVALID MESSAGE\n"));
            /* indicate we couldn't handle the message */
            break;
        }
    }

    CmFreeUpstreamMessageContents((void *) message);
    UNUSED(task);
}
