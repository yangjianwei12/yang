/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       
    \ingroup    generic_broadcast_scan_server
    \brief      generic_broadcast_scan_server_handover functions are defined
*/

#ifdef INCLUDE_GBSS

#include <panic.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include "marshal.h"
#include "handover_if.h"
#include "logging.h"
#include "generic_broadcast_scan_server_marshal_desc.h"
#include "generic_broadcast_scan_server.h"
#include "generic_broadcast_scan_server_private.h"
#include "generic_broadcast_scan_server_volume.h"
#include "gatt.h"
#include "bt_types.h"

/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static uint32 unmarshalled_cid = 0;

/*! \brief GBSS has no conditions to check and veto */
static bool genericBroadcastScanServer_Veto(void)
{
    return FALSE;
}

/*! \brief Find GBSS configuration and marshal if any exists */
static bool genericBroadcastScanServer_Marshal(const tp_bdaddr *tp_bd_addr,
                                               uint8 *buf,
                                               uint16 length,
                                               uint16 *written)
{
    bool marshalled;
    int brs;
    generic_broadcast_scan_server_marshal_data_t obj;
    gatt_cid_t cid_to_marshal = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);
    gbss_srv_data_t *gbss_server = genericBroadcastScanServer_GetInstance();

    if (cid_to_marshal != INVALID_CID)
    {
        gbss_client_data_t *con = genericBroadcastScanServer_FindConnection(cid_to_marshal);

        if (con)
        {
            obj.cid = cid_to_marshal;
            obj.gbss_scan_ccc = con->client_cfg.gbss_scan_ccc;

            for (brs = 0; brs < NUM_GBSS_BRS; brs++)
            {
                obj.gbss_rcv_state_ccc[brs] = con->client_cfg.gbss_rcv_state_ccc[brs];
            }

            obj.gbss_scan_cp_ccc = con->client_cfg.gbss_scan_cp_ccc;
            obj.gbss_scan_cp_rsp_opcode = con->scan_cp_response.opcode;
            obj.gbss_scan_cp_rsp_status = con->scan_cp_response.status_code;
            obj.gbss_scan_cp_rsp_param_len = con->scan_cp_response.param_len;
            obj.gbss_scan_cp_rsp_params_source_id = con->scan_cp_response.params.source_id;
            obj.gbss_scan_cp_rsp_params_broadcast_id = con->scan_cp_response.params.broadcast_id;

            obj.gbss_volume_state_ccc = con->client_cfg.gbss_volume_state_ccc;
            obj.gbss_volume_setting = gbss_server->gbss_volume_data.volume_setting;
            obj.gbss_volume_change_counter = gbss_server->gbss_volume_data.change_counter;
            obj.gbss_volume_mute = gbss_server->gbss_volume_data.mute;

#ifdef ENABLE_RDP_DEMO
            obj.gbss_src_state_ntf_counter = gbss_server->src_state_ntf_counter;
#endif

            DEBUG_LOG("genericBroadcastScanServer_Marshal: Marshalling for addr[0x%06x]", tp_bd_addr->taddr.addr.lap);
            marshaller_t marshaller = MarshalInit(mtdesc_generic_broadcast_scan_server_mgr, GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_OBJ_TYPE_COUNT);
            MarshalSetBuffer(marshaller, (void *)buf, length);
            marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(generic_broadcast_scan_server_marshal_data_t));
            *written = marshalled ? MarshalProduced(marshaller) : 0;
            MarshalDestroy(marshaller, FALSE);
            return marshalled;
        }
    }

    *written = 0;
    return TRUE;
}

/*! \brief Unmarshal and fill GBSS client config data */
static bool genericBroadcastScanServer_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                                 const uint8 *buf,
                                                 uint16 length,
                                                 uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    int brs;
    marshal_type_t unmarshalled_type;
    generic_broadcast_scan_server_marshal_data_t *data = NULL;
    gatt_cid_t cid_to_unmarshal;
    gbss_client_data_t *con;
    gbss_srv_data_t *gbss_server = genericBroadcastScanServer_GetInstance();

    DEBUG_LOG("genericBroadcastScanServer_Unmarshal");

    unmarshaller_t unmarshaller = UnmarshalInit(mtdesc_generic_broadcast_scan_server_mgr, GENERIC_BROADCAST_SCAN_SERVER_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *)buf, length);

    if (Unmarshal(unmarshaller, (void **)&data, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(generic_broadcast_scan_server_marshal_data_t));
        PanicNull(data);
        cid_to_unmarshal = data->cid;
        PanicFalse(cid_to_unmarshal != INVALID_CID);
        con = genericBroadcastScanServer_FindConnection(cid_to_unmarshal);

        if(!con)
        {/* does not exist add it */
            con = genericBroadcastScanServer_AddConnection(cid_to_unmarshal);
        }

        if (con)
        {
            con->cid = cid_to_unmarshal;
            con->client_cfg.gbss_scan_ccc = data->gbss_scan_ccc;
            for (brs = 0; brs < NUM_GBSS_BRS; brs++)
            {
                con->client_cfg.gbss_rcv_state_ccc[brs] = data->gbss_rcv_state_ccc[brs];
            }
        }

        con->client_cfg.gbss_scan_cp_ccc = data->gbss_scan_cp_ccc;
        con->scan_cp_response.opcode = data->gbss_scan_cp_rsp_opcode;
        con->scan_cp_response.status_code = data->gbss_scan_cp_rsp_status;
        con->scan_cp_response.param_len = data->gbss_scan_cp_rsp_param_len;
        con->scan_cp_response.params.source_id = data->gbss_scan_cp_rsp_params_source_id;
        con->scan_cp_response.params.broadcast_id = data->gbss_scan_cp_rsp_params_broadcast_id;

        con->client_cfg.gbss_volume_state_ccc = data->gbss_volume_state_ccc;
        gbss_server->gbss_volume_data.volume_setting = data->gbss_volume_setting;
        gbss_server->gbss_volume_data.change_counter = data->gbss_volume_change_counter;
        gbss_server->gbss_volume_data.mute = data->gbss_volume_mute;

#ifdef ENABLE_RDP_DEMO
        gbss_server->src_state_ntf_counter = data->gbss_src_state_ntf_counter;
#endif

        unmarshalled_cid = cid_to_unmarshal;
        *consumed = UnmarshalConsumed(unmarshaller);
        UnmarshalDestroy(unmarshaller, TRUE);
        return TRUE;
    }
    else
    {
        *consumed = 0;
        DEBUG_LOG("genericBroadcastScanServer_Unmarshal: failed unmarshal");
        UnmarshalDestroy(unmarshaller, TRUE);
        return FALSE;
    }
}

/*! \brief Handle commit operation on primary */
static void genericBroadcastScanServer_HandleCommitForPrimary(void)
{
    DEBUG_LOG("genericBroadcastScanServer_HandoverCommit For Primary");
}

/*! \brief Handle commit operation on secondary */
static void genericBroadcastScanServer_HandleCommitForSecondary(void)
{
    DEBUG_LOG("genericBroadcastScanServer_HandoverCommit For Secondary");
}

/*! \brief Handle commit for GBSS */
static void genericBroadcastScanServer_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    UNUSED(tp_bd_addr) ;

    if (is_primary)
    {
        genericBroadcastScanServer_HandleCommitForPrimary();
    }
    else
    {
        genericBroadcastScanServer_HandleCommitForSecondary();
    }
}

/*! \brief Handle handover complete for GBSS */
static void genericBroadcastScanServer_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
    /* mark complete of unmarshalled data */
    unmarshalled_cid = 0;
    DEBUG_LOG("genericBroadcastScanServer_HandoverComplete");
}

/*! \brief On abort, reset GBSS configs in the secondary */
static void genericBroadcastScanServer_HandoverAbort(void)
{
    uint8 index;
    gbss_client_data_t *con = genericBroadcastScanServer_FindConnection(unmarshalled_cid);
    gbss_srv_data_t *gbss_server = genericBroadcastScanServer_GetInstance();
    DEBUG_LOG("genericBroadcastScanServer_HandoverAbort");


    if (con)
    {
        unmarshalled_cid = 0;
        con->cid = 0;
        con->client_cfg.gbss_scan_ccc = 0;

        for (index = 0; index < NUM_GBSS_BRS; index++)
            con->client_cfg.gbss_rcv_state_ccc[index] = 0;

        con->client_cfg.gbss_scan_cp_ccc = 0;
        con->scan_cp_response.opcode = 0xFF;
        con->scan_cp_response.status_code = 0;
        con->scan_cp_response.param_len = 0;
        con->scan_cp_response.params.source_id = 0;
        con->scan_cp_response.params.broadcast_id = 0;

        con->client_cfg.gbss_volume_state_ccc = 0;
        gbss_server->gbss_volume_data.volume_setting = GBSS_DEFAULT_AUDIO_VOLUME;
        gbss_server->gbss_volume_data.change_counter = 0;
        gbss_server->gbss_volume_data.mute = 0;

#ifdef ENABLE_RDP_DEMO
        gbss_server->src_state_ntf_counter = 0;
#endif
    }
}

/*! \brief On abort, GBSS handover interfaces */
const handover_interface generic_broadcast_scan_server_handover_if =
        MAKE_BLE_HANDOVER_IF(&genericBroadcastScanServer_Veto,
                             &genericBroadcastScanServer_Marshal,
                             &genericBroadcastScanServer_Unmarshal,
                             &genericBroadcastScanServer_HandoverCommit,
                             &genericBroadcastScanServer_HandoverComplete,
                             &genericBroadcastScanServer_HandoverAbort);

#endif /* INCLUDE_GBSS */
