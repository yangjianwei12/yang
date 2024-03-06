/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #59 $
******************************************************************************/

#include "gatt_transport_discovery_server_private.h"
#include "gatt_transport_discovery_server_msg_handler.h"
#include "gatt_transport_discovery_server.h"
#include "csr_bt_gatt_lib.h"

GTDS TdsServerInst;

/****************************************************************************/
bool GattTransportDiscoveryServerInit(AppTask appTask,
                                    GTDS_T *const tds,
                                    uint16 startHandle,
                                    uint16 endHandle
                                    )
{
    CsrBtGattId gattId;
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s", __FUNCTION__));

    /* validate the input parameters */
    if ((appTask == CSR_SCHED_QID_INVALID) || (tds == NULL))
    {

        GATT_TDS_SERVER_PANIC(("TDS: Invalid Initialisation parameters"));
    }

    /* Reset all the service library memory. */
    CsrMemSet(tds, 0, sizeof(GTDS_T));

    /* Store application message handler as application messages need to be posted here */
    tds->app_task = appTask;

    /* Fill in the registration parameters */
    tds->lib_task = CSR_BT_TDS_SERVER_IFACEQUEUE;
    /* Try to register this instance of TDS library to Gatt profile */
    /* Register with the GATT  */
     gattId = CsrBtGattRegister(tds->lib_task);
    /* verify the result */
    if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
    {
        return FALSE;
    }
    else
    {
        tds->gattId = gattId;
        TdsServerInst.tds = tds;
        GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: tds->gattId=%d, %u\n", tds->gattId, (unsigned int)TdsServerInst.tds));

        CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle, endHandle);
    }
    return TRUE;
}


void GattTdsServerTaskInit(void** gash)
{
    *gash = &TdsServerInst;
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: %u\n", (unsigned int)TdsServerInst.tds));

    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: GattTdsServerTaskInit\n"));
}
#ifdef ENABLE_SHUTDOWN
void GattTdsServerTaskDeinit(void** gash)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS: GattTdsServerTaskDeinit\n"));
}
#endif


/****************************************************************************/
bool GattTdsServerReadClientConfigResponse( const GTDS_T *tds, connection_id_t cid, uint16 client_config)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: tds=%u, client_config=%d, cid=%d\n", __FUNCTION__, (unsigned int)tds, client_config, cid));

    /* Validate the input parameters */
    if(tds == NULL)
    {
        GATT_TDS_SERVER_PANIC(("GTDS: Null instance"));
    }

    /* Validate the input parameters */
    if ( (cid == 0)  || (client_config > 0x04) )
    {
        return FALSE;
    }

    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: %d\n", __FUNCTION__, __LINE__));
    /* Send the client config response */
    sendTdsServerClientConfigReadAccessRsp(tds, cid, client_config);
    return TRUE;
}


/****************************************************************************/
bool GattTdsServerSendNotification(const GTDS_T *tds, connection_id_t cid, uint16 tds_ind_size, uint8 *tds_ind_data)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s", __FUNCTION__));

    /* Validate this instance */
    if(tds == NULL)
    {
        GATT_TDS_SERVER_PANIC(("GTDS: Null instance"));
    }

    /* Validate the input parameters */
    if ((cid == 0) || (tds_ind_size == 0) || (tds_ind_data == NULL))
    {
        return FALSE;
    }
    sendTdsServerControlPointIndication(tds, cid, tds_ind_size, tds_ind_data);
    return TRUE;
}


/***************************************************************************
NAME
    GattTdsServerSendResponse
    
DESCRIPTION
    Send response to the write request on TDS Service attributes.
*/
void GattTdsServerSendResponse(const GATT_TRANSPORT_DISCOVERY_SERVER_WRITE_CONTROL_POINT_IND_T *ind, uint16 result)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: Send Write Respone for the write request on control point", __FUNCTION__));
    CsrBtGattDbWriteAccessResSend(ind->tds->gattId,
                                 ind->cid,
                                 ind->handle,
                                 result);

}

/****************************************************************************/
bool GattTdsServerReadBredrHandoverDataResponse(const GTDS_T *tds, 
                                                connection_id_t cid,
                                                uint8_t bredr_features,
                                                CsrBtDeviceAddr bd_addr,
                                                uint24_t class_of_device)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: tds=%u, cid=%d\n", __FUNCTION__, (unsigned int)tds, cid));

    uint8_t value[GATT_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA_VALUE_SIZE];
    /* Validate the input parameters */
    if(tds == NULL)
    {
        GATT_TDS_SERVER_PANIC(("GTDS: Null instance"));
    }

    /* Validate the input parameters */
    if (cid == 0)
    {
        return FALSE;
    }

    value[0] = (bredr_features & 0xFF);

    value[1] = (uint8)bd_addr.lap & 0xFF;
    value[2] = (uint8)((bd_addr.lap >> 8) & 0xFF);
    value[3] = (uint8)((bd_addr.lap >> 16) & 0xFF);
    value[4] = (uint8)bd_addr.uap & 0xFF;
    value[5] = (uint8)bd_addr.nap & 0xFF;
    value[6] = (uint8)((bd_addr.nap >> 8) & 0xFF);

    value[7] = (uint8)class_of_device & 0xFF;
    value[8] = (uint8)((class_of_device >> 8) & 0xFF);
    value[8] = (uint8)((class_of_device >> 16) & 0xFF);

    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: %d\n", __FUNCTION__, __LINE__));

    sendTdsServerReadAccessRsp(
             tds->gattId,
             cid,
             HANDLE_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA,
             CSR_BT_GATT_ACCESS_RES_SUCCESS,
             GATT_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA_VALUE_SIZE,
             value);

    return TRUE;
}

/****************************************************************************/
bool GattTdsServerReadBredrTransportBlockDataResponse(const GTDS_T *tds,
                                                      connection_id_t cid,
                                                      uint16_t size_value,
                                                      uint8_t *value)
{
    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: tds=%u, cid=%d\n", __FUNCTION__, (unsigned int)tds, cid));

    /* Validate the input parameters */
    if(tds == NULL)
    {
        GATT_TDS_SERVER_PANIC(("GTDS: Null instance"));
    }

    /* Validate the input parameters */
    if (cid == 0)
    {
        return FALSE;
    }

    GATT_TDS_SERVER_DEBUG_INFO(("\nTDS %s: %d\n", __FUNCTION__, __LINE__));

    sendTdsServerReadAccessRsp(
             tds->gattId,
             cid,
             HANDLE_TRANSPORT_DISCOVERY_BREDR_TRANSPORT_BLOCK_DATA,
             CSR_BT_GATT_ACCESS_RES_SUCCESS,
             size_value,
             value);

    return TRUE;
}
