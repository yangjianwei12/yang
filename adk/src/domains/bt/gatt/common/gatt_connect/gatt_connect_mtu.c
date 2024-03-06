/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    gatt_connect
    \brief      Handling GATT MTU exchange
*/

#include "gatt_connect.h"
#include "gatt_connect_list.h"
#include "gatt_connect_mtu.h"
#ifdef USE_SYNERGY
#include <gatt_lib.h>
#endif
#include <logging.h>

#include <panic.h>

#ifndef USE_SYNERGY
#define DEFAULT_MTU 65
#else
#define DEFAULT_MTU 124
#endif

static unsigned mtu_local;

static void updateConnectionMtu(unsigned cid, unsigned mtu_remote)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    GattConnect_SetMtu(connection, MIN(mtu_local, mtu_remote));
}

#ifdef USE_SYNERGY
void gattConnect_HandleExchangeMtuInd(uint32 gattId, CsrBtGattRemoteClientExchangeMtuInd* ind)
{
    DEBUG_LOG("gattConnect_HandleExchangeMtuInd: mtu = %d", ind->mtu);
    updateConnectionMtu(ind->btConnId, ind->mtu);
    GattRemoteClientExchangeMtuResSend(gattId, ind->btConnId, mtu_local);
}
#else
void gattConnect_HandleExchangeMtuInd(GATT_EXCHANGE_MTU_IND_T* ind)
{
    updateConnectionMtu(ind->cid, ind->mtu);
    GattExchangeMtuResponse(ind->cid, mtu_local);
}
#endif

#ifdef USE_SYNERGY
void GattConnect_SendExchangeMtuReq(uint32 gattId, uint32 btConnId)
{
    DEBUG_LOG("GattConnect_SendExchangeMtuReq: mtu = %d", mtu_local);
    GattClientExchangeMtuReqSend(gattId, btConnId, mtu_local);
}
#else
void GattConnect_SendExchangeMtuReq(Task task, unsigned cid)
{
    GattExchangeMtuRequest(task, cid, mtu_local);
}
#endif

#ifdef USE_SYNERGY
void gattConnect_HandleExchangeMtuCfm(CsrBtGattClientExchangeMtuCfm* cfm)
{
    DEBUG_LOG("gattConnect_HandleExchangeMtuCfm: mtu = %d", cfm->mtu);

    switch (cfm->resultCode)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            updateConnectionMtu(cfm->btConnId, cfm->mtu);
        break;
        
        case ATT_RESULT_INVALID_MTU:
            if(cfm->resultSupplier == CSR_BT_SUPPLIER_ATT)
            {
                Panic();
            }
        break;
        
        /* NB. Synergy does not enforce single MTU exchange so has 
           no equivalent of gatt_status_mtu_already_exchanged */
        
        default:
        break;
    }
}
#else
void gattConnect_HandleExchangeMtuCfm(GATT_EXCHANGE_MTU_CFM_T* cfm)
{
    DEBUG_LOG("gattConnect_HandleExchangeMtuCfm: status = %d, mtu = %d", cfm->status, cfm->mtu);
    switch (cfm->status)
    {
        case gatt_status_success:
            updateConnectionMtu(cfm->cid, cfm->mtu);
            break;
        case gatt_status_invalid_mtu:
        case gatt_status_mtu_already_exchanged:
            Panic();
            break;
        default:
            break;
    }
}
#endif

void GattConnect_UpdateMinAcceptableMtu(unsigned mtu)
{
    mtu_local = MAX(mtu_local, mtu);
}

void GattConnect_SetMtu(gatt_connection_t* connection, unsigned mtu)
{
    if(connection)
    {
        connection->mtu = mtu;
    }
}

unsigned GattConnect_GetMtu(unsigned cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    
    if(connection)
        return connection->mtu;
    
    return GATT_CONNECT_MTU_INVALID;
}

void GattConnect_MtuInit(void)
{
    mtu_local = DEFAULT_MTU;
}
