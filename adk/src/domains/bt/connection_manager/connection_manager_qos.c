/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#include "connection_manager_qos.h"
#include "connection_manager_data.h"
#include "connection_manager_params.h"
#include "connection_manager_list.h"
#include "connection_manager_msg.h"
#include "connection_manager_config.h"

#include <logging.h>
#include <panic.h>
#include <local_addr.h>

static cm_qos_t cm_default_qos;
static cm_qos_t cm_max_qos;

static cm_qos_t conManagerGetQosToUse(cm_connection_t* connection);

/******************************************************************************/
static bool conManagerGetParamsToUse(const cm_connection_t* connection,
                                     cm_qos_t qos, ble_connection_params* params)
{
    conManagerTaskData *connmgr = ConManagerGetTaskData();

    if(cm_qos_params[qos])
    {
        *params = *cm_qos_params[qos];

        if (connmgr->connection_params_adjust_callback.LeParams)
        {
            uint16 conn_interval_min = params->conn_interval_min;
            uint16 conn_interval_max = params->conn_interval_max;

            const tp_bdaddr *tpaddr = connection ? &connection->tpaddr : NULL;

            if (connmgr->connection_params_adjust_callback.LeParams(tpaddr,
                                                        &conn_interval_min,&conn_interval_max))
            {
                DEBUG_LOG("conManagerGetParamsToUse connection interval adjusted %d-%d => %d-%d",
                        params->conn_interval_min, params->conn_interval_max,
                        conn_interval_min, conn_interval_max);

                if (   params->conn_interval_min <= conn_interval_min
                    && conn_interval_min <= params->conn_interval_max
                    && params->conn_interval_min <= conn_interval_max
                    && conn_interval_max <= params->conn_interval_max
                    && conn_interval_min <= conn_interval_max)
                {
                    params->conn_interval_min = conn_interval_min;
                    params->conn_interval_max = conn_interval_max;
                }
                else
                {
                    DEBUG_LOG_ERROR("conManagerGetParamsToUse plugin adjusted parameters badly");
                    Panic();
                }
            }
        }
        params->own_address_type = LocalAddr_GetBleType();
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
void conManagerSendParameterUpdate(cm_connection_t* connection)
{
    if(connection && connection->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        cm_qos_t qos = conManagerGetQosToUse(connection);

        DEBUG_LOG("conManagerSendParameterUpdate: qos=enum:cm_qos_t:%d", qos);

        ConManagerDebugAddressVerbose(&connection->tpaddr);

        if(qos != cm_qos_passive)
        {
            ble_connection_params params;

            if(conManagerGetParamsToUse(connection, qos, &params))
            {
                DEBUG_LOG("ConManagerSendParameterUpdate ask for %d-%d[%d] Have %d[%d] In prog:%d",
                                params.conn_interval_min, params.conn_interval_max, params.conn_latency,
                                connection->conn_interval, connection->slave_latency,
                                connection->le_update_in_progress);

                /* Only request an update if the current link setting is
                   not compatible with the requested parameters */
                if (   connection->conn_interval
                    && !connection->le_update_in_progress
                    && (   (params.conn_latency != connection->slave_latency)
                        || (connection->conn_interval < params.conn_interval_min)
                        || (params.conn_interval_max < connection->conn_interval)))
                {
                    typed_bdaddr *taddr = &connection->tpaddr.taddr;

                    /* Block additional parameter updates */
                    connection->le_update_in_progress = params.conn_interval_min;
                    Task task = ConManagerGetTask(connection);
                    if (task)
                    {
                        MessageCancelAll(task, CON_MANAGER_INTERNAL_MSG_QOS_TIMEOUT);
                        MessageSendLater(task, 
                                         CON_MANAGER_INTERNAL_MSG_QOS_TIMEOUT, NULL,
                                         D_SEC(appConfigDelayBleParamUpdateTimeout()));
                    }

                    DEBUG_LOG_VERBOSE("conManagerSendParameterUpdate: ConnectionDmBleConnectionParametersUpdateReq");
                    DEBUG_LOG_VERBOSE("  addr %u %04x %02x %06x", taddr->type, taddr->addr.nap, taddr->addr.uap, taddr->addr.lap);
                    DEBUG_LOG_VERBOSE("  min_interval %u", params.conn_interval_min);
                    DEBUG_LOG_VERBOSE("  max_interval %u", params.conn_interval_max);
                    DEBUG_LOG_VERBOSE("  latency %u", params.conn_latency);
                    DEBUG_LOG_VERBOSE("  timeout %u", params.supervision_timeout);

                    ConnectionDmBleConnectionParametersUpdateReq(
                        ConManagerGetTask(connection), // Make sure we get failures
                        (typed_bdaddr*)&connection->tpaddr.taddr, 
                        params.conn_interval_min, 
                        params.conn_interval_max, 
                        params.conn_latency, 
                        params.supervision_timeout, 
                        LE_CON_EVENT_LENGTH_MIN, 
                        LE_CON_EVENT_LENGTH_MAX);
                }
                else
                {
                    DEBUG_LOG_VERBOSE("conManagerSendParameterUpdate: nothing to do");
                }
            }
        }
    }
}

/******************************************************************************/
static void conManagerUpdateConnectionParameters(cm_connection_t* connection)
{
    if(conManagerGetConnectionState(connection) == ACL_CONNECTED)
    {
        conManagerSendInternalMsgApplyQos(connection);
    }
}

/******************************************************************************/
static cm_qos_t conManagerGetConnectionQos(cm_connection_t* connection)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        cm_qos_t qos;
        
        for(qos = cm_qos_max - 1; qos > cm_qos_invalid; qos--)
        {
            if(qos_list[qos] > 0)
            {
                return qos;
            }
        }
    }
    
    return cm_qos_invalid;
}

/******************************************************************************/
static void conManagerRecordQos(cm_connection_t* connection, cm_qos_t qos)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        qos_list[qos]++;
    }
}

/******************************************************************************/
static void conManagerReleaseQos(cm_connection_t* connection, cm_qos_t qos)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        PanicFalse(qos_list[qos] > 0);
        
        qos_list[qos]--;
    }
}

/******************************************************************************/
static cm_qos_t conManagerGetQosToUse(cm_connection_t* connection)
{
    cm_qos_t qos = conManagerGetConnectionQos(connection);
    
    if(qos == cm_qos_invalid)
    {
        qos = cm_default_qos;
    }
    
    return MIN(qos, cm_max_qos);
}

/******************************************************************************/
static bool conManagerConnectionQosIsDefault(cm_connection_t* connection)
{
    cm_qos_t qos = conManagerGetConnectionQos(connection);
    
    if(qos == cm_qos_invalid || qos == cm_default_qos)
    {
        return TRUE;
    }
    
    return FALSE;
}

/******************************************************************************/
static void conManagerValidateQos(cm_transport_t transport_mask, cm_qos_t qos)
{
    PanicFalse(transport_mask == cm_transport_ble);
    PanicFalse(qos > cm_qos_invalid);
    PanicFalse(qos < cm_qos_max);
}

/******************************************************************************/
void ConManagerApplyQosOnConnect(cm_connection_t* connection)
{
    /* Locally initiated connection will already be using default parameters*/
    if(conManagerConnectionIsLocallyInitiated(connection))
    {
        if(conManagerConnectionQosIsDefault(connection))
        {
            return;
        }
    }
    
    conManagerUpdateConnectionParameters(connection);
}

/******************************************************************************/
void ConnectionManagerQosInit(void)
{
    cm_default_qos = cm_qos_invalid;
    cm_max_qos = cm_qos_max;
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_power);
}

/******************************************************************************/
void ConManagerRequestDefaultQos(cm_transport_t transport_mask, cm_qos_t qos)
{
    conManagerValidateQos(transport_mask, qos);
    
    if(qos > cm_default_qos)
    {
        cm_qos_t qos_to_use;
        ble_connection_params params;
        
        cm_default_qos = qos;
        
        qos_to_use = conManagerGetQosToUse(NULL);
        if(conManagerGetParamsToUse(NULL, qos_to_use, &params))
        {
            ConnectionDmBleSetConnectionParametersReq(&params);
        }
    }
}

/******************************************************************************/
void ConManagerApplyQosPreConnect(cm_connection_t* connection)
{
    if (connection)
    {
        ble_connection_params params;
        cm_qos_t qos = conManagerGetQosToUse(connection);

        DEBUG_LOG("ConManagerApplyQosPreConnect (%d). Connection:%p", qos, connection);
        ConManagerDebugAddressVerbose(&connection->tpaddr);

        if(conManagerGetParamsToUse(connection, qos, &params))
        {
            ConnectionDmBleSetConnectionParametersReq(&params);
        }
    }
}

/******************************************************************************/
void ConManagerRequestDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    conManagerValidateQos(TransportToCmTransport(tpaddr->transport), qos);
    
    if(connection)
    {
        cm_qos_t prev_qos = conManagerGetConnectionQos(connection);
        conManagerRecordQos(connection, qos);

        /* This can result in repeated updates if qos == prev_qos.
           Assumption here is that sending too many is harmless, but
           too few and we might not correct a failed update */
        if(qos >= prev_qos)
        {
            if (prev_qos == cm_qos_invalid)
            {
                prev_qos = conManagerGetQosToUse(connection);
            }
            if (qos!=prev_qos)
            {
                DEBUG_LOG("ConManagerRequestDeviceQos: connection:%p [0x%06lx] from enum:cm_qos_t:%d to enum:cm_qos_t:%d",
                        connection, tpaddr->taddr.addr.lap,  prev_qos, qos);
            }
            conManagerSendInternalMsgUpdateQos(connection);
        }
        else
        {
            DEBUG_LOG("ConManagerRequestDeviceQos: connection:%p [0x%06lx] enum:cm_qos_t:%d recorded not actioned (current higher enum:cm_qos_t:%d)",
                        connection, tpaddr->taddr.addr.lap, qos, prev_qos);
        }
    }
}

void ConManagerUpdateDeviceQos(const tp_bdaddr *tpaddr)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);

    if(connection)
    {
        conManagerSendInternalMsgApplyQos(connection);
    }
}


/******************************************************************************/
void ConManagerReleaseDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    conManagerValidateQos(TransportToCmTransport(tpaddr->transport), qos);
    
    if(connection)
    {
        cm_qos_t fallback_qos;
        cm_qos_t prev_qos = conManagerGetConnectionQos(connection);
        
        conManagerReleaseQos(connection, qos);
        fallback_qos = conManagerGetQosToUse(connection);

        if(fallback_qos != prev_qos)
        {
            DEBUG_LOG("ConManagerReleaseDeviceQos: connection:%p [0x%06lx] releasing enum:cm_qos_t:%d was  enum:cm_qos_t:%d new  enum:cm_qos_t:%d",
                        connection, tpaddr->taddr.addr.lap, qos, prev_qos, fallback_qos);
            
            conManagerSendInternalMsgUpdateQos(connection);
        }
        else
        {
            DEBUG_LOG("ConManagerReleaseDeviceQos: connection:%p [0x%06lx] releasing enum:cm_qos_t:%d SAME",
                        connection, tpaddr->taddr.addr.lap, qos);
        }
    }
}

/******************************************************************************/
void ConManagerSetMaxQos(cm_qos_t qos)
{
    cm_list_iterator_t iterator;
    cm_connection_t* connection = ConManagerListHeadConnection(&iterator);
    DEBUG_LOG("ConManagerSetMaxQos");
    
    PanicFalse(qos > cm_qos_invalid);
    PanicFalse(qos != cm_qos_passive);
    
    cm_max_qos = qos;
    
    while(connection)
    {
        conManagerSendInternalMsgUpdateQos(connection);
        connection = ConManagerListNextConnection(&iterator);
    }
}

void conManagerQosCheckNewConnParams(cm_connection_t *connection)
{
#if defined(INCLUDE_LEA_LINK_POLICY)
    cm_qos_t qos;

    qos = conManagerGetConnectionQos(connection);

    DEBUG_LOG("conManagerQosCheckNewConnParams conn:%p %d-%d. enum:cm_qos_t:%d",
              connection, 
              connection->conn_interval, connection->slave_latency, qos);

    switch (qos)
    {
        case cm_qos_invalid:
        case cm_qos_passive:
            break;

        default:
            /* Always "update" connection parameters.
               If different to those now expected, update will be requested
               If same/compatible, there will be no change */
            conManagerUpdateConnectionParameters(connection);
            break;
    }
#else
    UNUSED(connection);
#endif
}

