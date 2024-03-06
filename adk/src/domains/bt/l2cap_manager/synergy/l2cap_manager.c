/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       l2cap_manager.c
\brief	    Implementation of module providing L2CAP connections.
*/

#ifdef INCLUDE_L2CAP_MANAGER
#include "l2cap_manager_private.h"

/* Enable debug log outputs with per-module debug log levels.
 * The log output level for this module can be changed with the PyDbg command:
 *      >>> apps1.log_level("l2cap_manager", 3)
 * Where the second parameter value means:
 *      0:ERROR, 1:WARN, 2:NORMAL(= INFO), 3:VERBOSE(= DEBUG), 4:V_VERBOSE(= VERBOSE), 5:V_V_VERBOSE(= V_VERBOSE)
 * See 'logging.h' and PyDbg 'log_level()' command descriptions for details. */
#define DEBUG_LOG_MODULE_NAME l2cap_manager
#include <logging.h>
DEBUG_LOG_DEFINE_LEVEL_VAR

#include <l2cap_prim.h>
#include <bt_device.h>
#include <connection.h>
#include <domain_message.h>
#include <panic.h>
#include <service.h>
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <task_list.h>
#include <stdlib.h>
#include <message.h>
#include "sds_prim.h"

/******************************************************************************
 * General Definitions
 ******************************************************************************/
#ifdef HOSTED_TEST_ENVIRONMENT
#define STATIC_FOR_TARGET
#else
#define STATIC_FOR_TARGET   static
#endif

#define L2CAP_MANAGER_SET_TYPED_BDADDR(DEST, TRANSPORT, TYPE, ADDR) \
    do { \
        (DEST).transport  = (TRANSPORT); \
        (DEST).taddr.type = (TYPE); \
        (DEST).taddr.addr = (ADDR); \
    } while (0);

/******************************************************************************
 * Global variables
 ******************************************************************************/
l2cap_manager_task_data_t l2cap_manager_task_data;

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

static void l2capManager_SdpSearchReq(const tp_bdaddr *tpaddr, const bool retry, l2cap_manager_psm_instance_t* psm_inst);


/******************************************************************************
 * Instance handling functions
 ******************************************************************************/
/*! \brief Generate an instance ID.

    \param[IN] type     The data type of an element of the linked-list.

    \return Pointer to the new blank element.
*/
STATIC_FOR_TARGET linked_list_key l2capManager_GetNewInstanceId(l2cap_manager_linked_list_type_t type)
{
    static linked_list_key instance_id_counter_psm = 0;
    static linked_list_key instance_id_counter_l2cap_link = 0;
    linked_list_key instance_id;

    switch (type)
    {
        case l2cap_manager_linked_list_type_psm_instance:
            instance_id = L2CAP_MANAGER_INSTANCE_ID_FLAG_PSM | instance_id_counter_psm;
            instance_id_counter_psm += 1;
            instance_id_counter_psm &= L2CAP_MANAGER_INSTANCE_ID_FLAG_ID_FIELD_MASK;
            break;
        case l2cap_manager_linked_list_type_l2cap_link_instance:
            instance_id = L2CAP_MANAGER_INSTANCE_ID_FLAG_L2CAP_LINK | instance_id_counter_l2cap_link;
            instance_id_counter_l2cap_link += 1;
            instance_id_counter_l2cap_link &= L2CAP_MANAGER_INSTANCE_ID_FLAG_ID_FIELD_MASK;
            break;
        default:
            DEBUG_LOG_ERROR("l2capManager GetNewInstanceId: ERROR! Invalid linked-list type: %d", type);
            Panic();
            instance_id = L2CAP_MANAGER_INSTANCE_ID_INVALID;
    }
    return instance_id;
}


/*! \brief Create a new PSM instance and add it to the linked-list.

    \note A unique instance ID will be assigned to the new PSM instance.

    \return Pointer to the new PSM instance.
*/
STATIC_FOR_TARGET l2cap_manager_psm_instance_t* l2capManager_CreatePsmInstance(void)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *new_element;

    new_element = (l2cap_manager_psm_instance_t*) PanicUnlessMalloc(sizeof(l2cap_manager_psm_instance_t));
    if (new_element)
    {
        if (task_inst->psm_instances)
        {
            l2cap_manager_psm_instance_t *ptr = task_inst->psm_instances;

            while (ptr->next != NULL)
                ptr = ptr->next;
            ptr->next = new_element;
        }
        else
        {
            task_inst->psm_instances = new_element;     /* Add the first element to the linked-list. */
        }
        new_element->next = NULL;
        new_element->instance_id = l2capManager_GetNewInstanceId(l2cap_manager_linked_list_type_psm_instance);
    }

    return new_element;
}


STATIC_FOR_TARGET l2cap_manager_psm_instance_t* l2capManager_SearchPsmInstance(const linked_list_key instance_id)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = task_inst->psm_instances;

    while (psm_inst != NULL)
    {
        if (psm_inst->instance_id == instance_id)
            return psm_inst;
        psm_inst = psm_inst->next;
    }
    return NULL;
}


STATIC_FOR_TARGET l2cap_manager_psm_instance_t* l2capManager_SearchPsmInstanceByState(const uint16 state)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = task_inst->psm_instances;

    while (psm_inst != NULL)
    {
        if (psm_inst->state == state)
            return psm_inst;
        psm_inst = psm_inst->next;
    }
    return NULL;
}


STATIC_FOR_TARGET l2cap_manager_psm_instance_t* l2capManager_SearchPsmInstanceByLocalPsm(const uint16 local_psm)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = task_inst->psm_instances;

    while (psm_inst != NULL)
    {
        if (psm_inst->local_psm == local_psm)
            return psm_inst;
        psm_inst = psm_inst->next;
    }
    return NULL;
}


STATIC_FOR_TARGET bool l2capManager_GetPsmAndL2capInstanceBySink(const Sink sink, l2cap_manager_psm_instance_t **psm_inst, l2cap_manager_l2cap_link_instance_t **l2cap_inst)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_ptr = task_inst->psm_instances;
    l2cap_manager_l2cap_link_instance_t *l2cap_ptr = NULL;

    while (psm_ptr != NULL)
    {
        l2cap_ptr = psm_ptr->l2cap_instances;
        while (l2cap_ptr != NULL)
        {
            if (l2cap_ptr->sink == sink)
            {
                *psm_inst = psm_ptr;
                *l2cap_inst = l2cap_ptr;
                return TRUE;
            }
            l2cap_ptr = l2cap_ptr->next;
        }
        psm_ptr = psm_ptr->next;
    }
    return FALSE;
}


STATIC_FOR_TARGET l2cap_manager_l2cap_link_instance_t* l2capManager_SearchL2capLinkInstanceByBdAddr(l2cap_manager_psm_instance_t *psm_inst, const tp_bdaddr *tpaddr)
{
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = psm_inst->l2cap_instances;
    bdaddr addr = tpaddr->taddr.addr;

    PanicNull(psm_inst);
    while (l2cap_inst != NULL)
    {
        if (BdaddrIsSame(&l2cap_inst->remote_dev.taddr.addr, &addr))
            return l2cap_inst;
        l2cap_inst = l2cap_inst->next;
    }
    return NULL;
}


STATIC_FOR_TARGET bool l2capManager_CreateL2capLinkInstance(l2cap_manager_psm_instance_t *psm_inst, const tp_bdaddr *tpaddr, l2cap_manager_l2cap_link_instance_t **l2cap_inst)
{
    PanicNull(psm_inst);

    l2cap_manager_l2cap_link_instance_t *ptr = psm_inst->l2cap_instances;
    l2cap_manager_l2cap_link_instance_t *last = NULL;
    bdaddr addr = tpaddr->taddr.addr;

    while (ptr != NULL)
    {
            if (BdaddrIsSame(&ptr->remote_dev.taddr.addr, &addr))
            {
                DEBUG_LOG_DEBUG("L2capManager CreateL2capLinkInstance: ALREADY EXISTS! The link instance for %04X-%02X-%06X (0x%p)",
                                tpaddr->taddr.addr.nap, tpaddr->taddr.addr.uap, tpaddr->taddr.addr.lap, ptr);
                *l2cap_inst = ptr;       /* The link instance for the BD-ADDR already exists! */
                return FALSE;           /* A new instance is *not* created. */
            }
        last = ptr;
        ptr = ptr->next;
    }

    {
        l2cap_manager_l2cap_link_instance_t *new_inst = (l2cap_manager_l2cap_link_instance_t*) PanicUnlessMalloc(sizeof(l2cap_manager_l2cap_link_instance_t));

        new_inst->next                 = NULL;
        new_inst->link_status          = L2CAP_MANAGER_LINK_STATE_NULL;
        new_inst->local_psm            = psm_inst->local_psm;
        new_inst->remote_dev           = *tpaddr;
        new_inst->connection_id        = 0;
        new_inst->identifier           = 0;
        new_inst->mtu_remote           = 0;
        new_inst->flush_timeout_remote = 0;
        new_inst->mode                 = 0;
        new_inst->sink                 = L2CAP_MANAGER_INVALID_SINK;
        new_inst->source               = L2CAP_MANAGER_INVALID_SOURCE;
        new_inst->context              = NULL;

        if (last == NULL)
            psm_inst->l2cap_instances = new_inst;
        else
            last->next = new_inst;
        psm_inst->num_of_links += 1;

        DEBUG_LOG_DEBUG("L2capManager CreateL2capLinkInstance: CREATED: A new link instance: 0x%p", new_inst);
        *l2cap_inst = new_inst;
        return TRUE;    /* A new instance is created. */
    }
}


STATIC_FOR_TARGET bool l2capManager_DeleteL2capLinkInstanceBySink(l2cap_manager_psm_instance_t *psm_inst, const Sink sink)
{
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = psm_inst->l2cap_instances;
    l2cap_manager_l2cap_link_instance_t *prev      = NULL;

    PanicNull(psm_inst);
    while (l2cap_inst != NULL)
    {
        if (l2cap_inst->sink == sink)
        {
            if (prev == NULL)
                psm_inst->l2cap_instances = l2cap_inst->next;
            else
                prev->next = l2cap_inst->next;
            free(l2cap_inst);

            if (0 < psm_inst->num_of_links)
                psm_inst->num_of_links -= 1;
            else
            {
                DEBUG_LOG_ERROR("L2capManager DeleteL2capLinkInstanceBySink: ERROR! 'num_of_links' is already zero!");
                Panic();
            }
            DEBUG_LOG_DEBUG("L2capManager DeleteL2capLinkInstanceBySink: DELETED: A link instance: 0x%p", l2cap_inst);
            return TRUE;
        }
        prev = l2cap_inst;
        l2cap_inst = l2cap_inst->next;
    }
    DEBUG_LOG_WARN("L2capManager DeleteL2capLinkInstanceBySink: WARNING! Cannot find a link instance for the sink: 0x%p", sink);
    return FALSE;
}

STATIC_FOR_TARGET bool l2capManager_GetPsmAndL2capInstanceByConnId(const uint16 conn_id, l2cap_manager_psm_instance_t **psm_inst, l2cap_manager_l2cap_link_instance_t **l2cap_inst)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_ptr = task_inst->psm_instances;
    l2cap_manager_l2cap_link_instance_t *l2cap_ptr = NULL;

    while (psm_ptr != NULL)
    {
        l2cap_ptr = psm_ptr->l2cap_instances;
        while (l2cap_ptr != NULL)
        {
            if (l2cap_ptr->connection_id == conn_id)
            {
                *psm_inst = psm_ptr;
                *l2cap_inst = l2cap_ptr;
                return TRUE;
            }
            l2cap_ptr = l2cap_ptr->next;
        }
        psm_ptr = psm_ptr->next;
    }
    return FALSE;
}


/******************************************************************************
 * Functions called by the message handler functions
 ******************************************************************************/
static void l2capManager_ConnectL2cap(const tp_bdaddr *tpaddr, l2cap_manager_psm_instance_t *psm_inst)
{
    l2cap_manager_status_t  status;
    l2cap_manager_l2cap_link_config_t config;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    CsrBtDeviceAddr addr;

    /* Read the SDP search pattern from the client's callback function. */
    status = (psm_inst->functions->get_l2cap_link_config)(tpaddr, &config);
    PanicFalse(status == l2cap_manager_status_success);

    DEBUG_LOG_DEBUG("L2capManager ConnectL2cap: (%04X-%02X-%06X)",
                    tpaddr->taddr.addr.nap, tpaddr->taddr.addr.uap, tpaddr->taddr.addr.lap);
    DEBUG_LOG_VERBOSE("L2capManager ConnectL2cap: Local PSM:  0x%04X", psm_inst->local_psm);
    DEBUG_LOG_VERBOSE("L2capManager ConnectL2cap: Remote PSM: 0x%04X", psm_inst->remote_psm);
    DEBUG_LOG_VERBOSE("L2capManager ConnectL2cap: ConfTab Length: %d", config.conftab_length);
    DEBUG_LOG_VERBOSE("L2capManager ConnectL2cap: ConfTab:        %p", config.conftab);

    task_inst->pending_connections++;
    psm_inst->state = L2CAP_MANAGER_PSM_STATE_CONNECTING;

    BdaddrConvertVmToBluestack(&addr, &tpaddr->taddr.addr);

    CmL2caConnectReqConftabSend(&task_inst->task,
                                addr,
                                psm_inst->local_psm,
                                psm_inst->remote_psm,
                                config.security_level,
                                0,
                                (config.conftab_length / sizeof(uint16)),
                                CsrMemDup(config.conftab, config.conftab_length),
                                CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);
}

/*! \brief Convert the disconnect status of the Connection Library to of the L2CAP Manager. */
static l2cap_manager_disconnect_status_t l2capManager_ConvertDisconnectStatus(uint16 in_status)
{
    switch (in_status)
    {
        case L2CA_DISCONNECT_NORMAL:
            return l2cap_manager_disconnect_successful;
        case L2CA_DISCONNECT_TIMEOUT:
            return l2cap_manager_disconnect_timed_out;
        case L2CA_DISCONNECT_LINK_LOSS:
            return l2cap_manager_disconnect_link_loss;
        case L2CA_DISCONNECT_LINK_TRANSFERRED:
            return l2cap_manager_disconnect_transferred;
        default:
            DEBUG_LOG_WARN("L2capManager ConvertDisconnectStatus: WARNING! Disconnect status code 0x%X is not mapped!", in_status);
            return l2cap_manager_disconnect_unknown_reason;
    }
}

static void l2capManager_NotifyFailedSdpSearch(l2cap_manager_psm_instance_t *psm_inst, l2cap_manager_l2cap_link_instance_t *l2cap_inst, const bdaddr *bd_addr)
{
    tp_bdaddr tpaddr;

    DEBUG_LOG_WARN("L2capManager NotifyFailedSdpSearch:");
    PanicNull(psm_inst);
    L2CAP_MANAGER_SET_TYPED_BDADDR(tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, *bd_addr);

    /* Notify the client that SDP Search is failed. */
    l2cap_manager_connect_cfm_t cfm_to_client =
    {
        .status               = l2cap_manager_connect_status_failed_sdp_search,
        .local_psm            = psm_inst->local_psm,
        .remote_psm           = L2CAP_MANAGER_PSM_INVALID,
        .tpaddr               = tpaddr,
        .sink                 = 0,
        .connection_id        = 0,
        .mtu_remote           = 0,
        .flush_timeout_remote = 0,
        .mode                 = 0,
        .qos_remote           = { 0 }
    };

    if (psm_inst->functions->handle_connect_cfm)
        (psm_inst->functions->handle_connect_cfm)(&cfm_to_client, l2cap_inst->context);
}

/*! \brief Clean up after disconnection of an L2CAP link. */
static bool l2capManager_CleanUpByDisconnection(l2cap_manager_psm_instance_t *psm_inst, const Sink sink)
{
    if (l2capManager_DeleteL2capLinkInstanceBySink(psm_inst, sink))
    {
        DEBUG_LOG_DEBUG("L2capManager CleanUpByDisconnection: Deleted the link instance for (sink: 0x%p)", sink);
        return TRUE;
    }
    else
    {
        DEBUG_LOG_ERROR("L2capManager CleanUpByDisconnection: ERROR! Failed to delete the link instance for (sink: 0x%p)", sink);
        Panic();
    }
    return FALSE;
}


/******************************************************************************
 * The message handler functions
 ******************************************************************************/
static void l2capManager_HandleMessageMoreData(const MessageMoreData *msg_more_data)
{
    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;
    Source source = msg_more_data->source;
    Sink sink = StreamSinkFromSource(source);

    if (FALSE == l2capManager_GetPsmAndL2capInstanceBySink(sink, &psm_inst, &l2cap_inst))
    {
        DEBUG_LOG_ERROR("L2capManager HandleMessageMoreData: ERROR! Failed to find the PSM/L2CAP instances for the source (0x%p)", msg_more_data->source);
        Panic();
    }

    if (psm_inst->functions->process_more_data)
    {
        l2cap_manager_message_more_data_t more_data;
        PanicFalse(source == l2cap_inst->source);

        more_data.connection_id = l2cap_inst->connection_id;
        more_data.source        = source;
        (psm_inst->functions->process_more_data)(&more_data, l2cap_inst->context);
    }
}


static void l2capManager_HandleMessageMoreSpace(const MessageMoreSpace *msg_more_space)
{
    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;

    if (FALSE == l2capManager_GetPsmAndL2capInstanceBySink(msg_more_space->sink, &psm_inst, &l2cap_inst))
    {
        DEBUG_LOG_ERROR("L2capManager HandleMessageMoreSpace: ERROR! Failed to find the PSM/L2CAP instances for the sink (0x%p)", msg_more_space->sink);
        return;
    }

    if (psm_inst->functions->process_more_space)
    {
        l2cap_manager_message_more_space_t more_space;

        more_space.connection_id = l2cap_inst->connection_id;
        more_space.sink          = msg_more_space->sink;
        (psm_inst->functions->process_more_space)(&more_space, l2cap_inst->context);
    }
}

static void l2capManager_HandleCmL2caRegisterCfm(const CsrBtCmL2caRegisterCfm *cfm)
{
    DEBUG_LOG_DEBUG("L2capManager HandleCmL2caRegisterCfm: (PSM:0x%04X, Status:%d)", cfm->localPsm, cfm->resultCode);

    /* We have registered the PSM used for mirror profile link with
       connection manager,        uint8 *record = PanicUnlessMalloc(appSdpGetMirrorProfileServiceRecordSize()); */
    if (CSR_BT_RESULT_CODE_CM_SUCCESS == cfm->resultCode)
    {
        l2cap_manager_sdp_record_t  sdp_record = { 0 };
        l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
        l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByState(L2CAP_MANAGER_PSM_STATE_PSM_REGISTRATION);
        PanicNull(psm_inst);
        
        /* Keep a copy of the registered L2CAP PSM. */
        psm_inst->local_psm = cfm->localPsm;

        /* Get the SDP service record through the client's callback function. */
        if (psm_inst->functions->get_sdp_record)
            (psm_inst->functions->get_sdp_record)(cfm->localPsm, &sdp_record);
        else
        {
            DEBUG_LOG_ERROR("L2capManager HandleCmL2caRegisterCfm: ERRRO! 'get_sdp_record' callback handler is NULL!");
            Panic();
        }
        DEBUG_LOG_VERBOSE("L2capManager HandleCmL2caRegisterCfm: SDP(Record:%p, Size:%d, OffsetToPsm:%d)",
                          sdp_record.service_record, sdp_record.service_record_size, sdp_record.offset_to_psm);

        if (sdp_record.service_record != NULL && sdp_record.service_record_size != 0)
        {
            /* Copy and update SDP record */
            uint8 *record = PanicUnlessMalloc(sdp_record.service_record_size);

            psm_inst->sdp_record = record;
            memcpy(record, sdp_record.service_record, sdp_record.service_record_size);

            /* Write L2CAP PSM into service record */
            record[sdp_record.offset_to_psm + 0] = (cfm->localPsm >> 8) & 0xFF;
            record[sdp_record.offset_to_psm + 1] = (cfm->localPsm) & 0xFF;

            psm_inst->state = L2CAP_MANAGER_PSM_STATE_SDP_REGISTRATION;
            /* Register service record */
            ConnectionRegisterServiceRecord(&task_inst->task, sdp_record.service_record_size, record);

            /* Do not 'free(record)' at this point.
             * Otherwise, the connection library will 'Panic()'.
             * 'psm_inst->sdp_record' must be freed later. */
        }
        else
        {
            DEBUG_LOG_WARN("L2capManager HandleCmL2caRegisterCfm: Valid SDP record is not supplied (Record:%p, Size:%d)", sdp_record.service_record, sdp_record.service_record_size);
        }
    }
    else
    {
        DEBUG_LOG_ERROR("L2capManager HandleCmL2caRegisterCfm: ERROR! Failed to register a PSM! (Status:%d)", cfm->resultCode);
        Panic();
        return;
    }
}

static void l2capManager_HandleCmSdsRegisterCfm(const CsrBtCmSdsRegisterCfm *cfm)
{
    DEBUG_LOG_DEBUG("L2capManager HandleCmSdsRegisterCfm: (result:0x%04x, supplier:0x%04x, Handle:0x%08X)",
                    cfm->resultCode, cfm->resultSupplier, cfm->serviceRecHandle);

    if (cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByState(L2CAP_MANAGER_PSM_STATE_SDP_REGISTRATION);
        PanicNull(psm_inst);

        DEBUG_LOG_DEBUG("L2capManager HandleCmSdsRegisterCfm: SDP record registered (Handle:0x%08X)", cfm->serviceRecHandle);

        /* Save the SDP service record handle assigned by the stack. */
        psm_inst->service_handle = cfm->serviceRecHandle;
        psm_inst->state = L2CAP_MANAGER_PSM_STATE_READY;

        /* Notify the client that the PSM has been registered. */
        if (psm_inst->functions->registered_ind)
        {
            (psm_inst->functions->registered_ind)(l2cap_manager_status_success);
        }
    }
    else if (cfm->resultSupplier == CSR_BT_SUPPLIER_SDP_SDS &&
             cfm->resultCode == SDS_PENDING)
    {
        DEBUG_LOG_WARN("L2capManager HandleCmSdsRegisterCfm: Pending the SDP record registration result:0x%04x", cfm->resultCode);
    }
    else
    {
        DEBUG_LOG_ERROR("L2capManager HandleCmSdsRegisterCfm: ERROR! Failed to register an SDP record!");
        Panic();
    }
}

static void l2capManager_HandleCmL2caConnectAcceptInd(const CsrBtCmL2caConnectAcceptInd *ind)
{
    l2cap_manager_connect_ind_t ind_from_remote;
    l2cap_manager_connect_rsp_t rsp_by_client = { 0 };
    l2cap_manager_l2cap_link_instance_t* l2cap_inst = NULL;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByLocalPsm(ind->localPsm);
    bdaddr bd_addr;
    PanicNull(psm_inst);

    DEBUG_LOG_DEBUG("L2capManager HandleConnectInd");
    DEBUG_LOG_DEBUG("L2capManager HandleConnectInd: - Remote addr: %04X-%02X-%06X",
                    ind->deviceAddr.nap, ind->deviceAddr.uap, ind->deviceAddr.lap);
    DEBUG_LOG_DEBUG("L2capManager HandleConnectInd: - Local PSM:      0x%04X", ind->localPsm);
    DEBUG_LOG_DEBUG("L2capManager HandleConnectInd: - Identifier:     0x%02X", ind->identifier);
    DEBUG_LOG_DEBUG("L2capManager HandleConnectInd: - BtConnId:       0x%04X", ind->btConnId);

    BdaddrConvertBluestackToVm(&bd_addr, &ind->deviceAddr);
    L2CAP_MANAGER_SET_TYPED_BDADDR(ind_from_remote.tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
    ind_from_remote.local_psm         = ind->localPsm;
    ind_from_remote.remote_psm        = 0;                      /* Not known yet. */
    ind_from_remote.identifier        = ind->identifier;
    ind_from_remote.connection_id     = CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId);

    l2capManager_CreateL2capLinkInstance(psm_inst, &ind_from_remote.tpaddr, &l2cap_inst);
    if (l2cap_inst == NULL)
    {
        DEBUG_LOG_ERROR("l2capManager_HandleCmL2caConnectAcceptInd: ERROR! Failed to allocate a link instance!");
        Panic();
    }

    L2CAP_MANAGER_SET_TYPED_BDADDR(l2cap_inst->remote_dev, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
    l2cap_inst->local_psm     = ind->localPsm;
    l2cap_inst->identifier    = ind->identifier;
    l2cap_inst->connection_id = CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId);
    l2cap_inst->link_status   = L2CAP_MANAGER_LINK_STATE_CONNECTING_BY_REMOTE;

    {
        /* Notify the client that a remote device attempts to connect this device.
         * The client's response is set to 'rsp_by_client'. */
        void *context = NULL;
    
        if (psm_inst->functions->respond_connect_ind)
            (psm_inst->functions->respond_connect_ind)(&ind_from_remote, &rsp_by_client, &context);
        /* Note that this 'context' pointer is used by the client.
         * We just save this for the client's use later. */
        l2cap_inst->context = context;
    }

    task_inst->pending_connections++;
    psm_inst->state = L2CAP_MANAGER_PSM_STATE_CONNECTING;

    /* Send a response accepting or rejecting the connection. */
    CmL2caConnectAcceptRspSend(&task_inst->task,
                               rsp_by_client.response,
                               ind->btConnId,
                               ind->localPsm,
                               ind->deviceAddr,
                               ind->identifier,
                               (rsp_by_client.conftab_length / sizeof(uint16)),
                               CsrMemDup(rsp_by_client.conftab, rsp_by_client.conftab_length),
                               CSR_BT_SC_DEFAULT_ENC_KEY_SIZE);

}

static uint16 l2capManager_ExtractPsm(CmnCsrBtLinkedListStruct *sdpTagList, l2cap_manager_psm_instance_t *psm_inst)
{
    uint16 psm = L2CA_PSM_INVALID;
    CsrBtUuid128 *tmpUuid128;
    CsrBtUuid32 tmpUuid32;
    uint16 tmpResult;
    uint16 dummy1, dummy2; /* Currently unused */
    l2cap_manager_sdp_search_pattern_t sdp_search_pattern;
    l2cap_manager_status_t  status;

    /* Read the SDP search pattern from the client's callback function. */
    status = (psm_inst->functions->get_sdp_search_pattern)(NULL, &sdp_search_pattern);
    PanicFalse(status == l2cap_manager_status_success);

    if(sdp_search_pattern.service_uuid_type == SDP_DATA_ELEMENT_SIZE_128_BITS)
    {
        if (CsrBtUtilSdrGetServiceUuid128AndResult(sdpTagList,
                                                   0,
                                                   &tmpUuid128,
                                                   &tmpResult,
                                                   &dummy1,
                                                   &dummy2))
        {
            if (tmpResult == SDR_SDC_SEARCH_SUCCESS &&
                !memcmp(tmpUuid128, sdp_search_pattern.service_uuid128, sizeof(sdp_search_pattern.service_uuid128)))
            {
                psm = CsrBtUtilSdrGetL2capPsm(sdpTagList, 0);

            }
        }
    }
    else if(sdp_search_pattern.service_uuid_type == SDP_DATA_ELEMENT_SIZE_32_BITS)
    {
        if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                                  0,
                                                  &tmpUuid32,
                                                  &tmpResult,
                                                  &dummy1,
                                                  &dummy2))
        {
            if (tmpResult == SDR_SDC_SEARCH_SUCCESS &&
                tmpUuid32 == sdp_search_pattern.service_uuid32)
            {
                psm = CsrBtUtilSdrGetL2capPsm(sdpTagList, 0);

            }
        }
    }

    return psm;
}

static void l2capManager_SdpResultHandler(void *inst,
                                          CmnCsrBtLinkedListStruct *sdpTagList,
                                          CsrBtDeviceAddr deviceAddr,
                                          CsrBtResultCode resultCode,
                                          CsrBtSupplier resultSupplier)
{
    tp_bdaddr tpaddr;
    bdaddr bd_addr;
    bool retry = FALSE, report_sdp_search_failure = TRUE;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByState(L2CAP_MANAGER_PSM_STATE_SDP_SEARCH);
    PanicNull(psm_inst);

    BdaddrConvertBluestackToVm(&bd_addr, &deviceAddr);
    DEBUG_LOG_DEBUG("l2capManager_SdpResultHandler: (Status:0x%X, Supplier:0x%X)", resultCode, resultSupplier);
    DEBUG_LOG_VERBOSE("L2capManager SdpResultHandler: - Remote addr:  %04X-%02X-%06X",
                      bd_addr.nap, bd_addr.uap, bd_addr.lap);

    if (resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        uint16 psm = L2CA_PSM_INVALID;

        /* Read the remote device's PSM from the SDP attributes. */
        psm = l2capManager_ExtractPsm(sdpTagList, psm_inst);
        if (psm != L2CA_PSM_INVALID)
        {
            psm_inst->remote_psm = psm;
            DEBUG_LOG_DEBUG("L2capManager SdpResultHandler: OK! (remote psm:%d)", psm);

            L2CAP_MANAGER_SET_TYPED_BDADDR(tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
            l2cap_inst = l2capManager_SearchL2capLinkInstanceByBdAddr(psm_inst, &tpaddr);
            PanicNull(l2cap_inst);

            l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_LOCAL_INITIATED_CONNECTING;
            l2capManager_ConnectL2cap(&tpaddr, psm_inst);
            report_sdp_search_failure = FALSE;
        }
        else
        {
            DEBUG_LOG_WARN("L2capManager SdpResultHandler: WARNING! No PSM found in the remote device's SDP record!");
        }
    }
    else if (resultSupplier == CSR_BT_SUPPLIER_SDP_SDC &&
             resultCode == SDC_NO_RESPONSE_DATA)
    {
        DEBUG_LOG_WARN("L2capManager SdpResultHandler: WARNING! SDP, No response data!");
    }
    else
    {
        /* A SDP search attempt has failed. Let's retry! */
        psm_inst->sdp_search_attempts += 1;
        if (psm_inst->sdp_search_attempts <= psm_inst->sdp_search_max_retries)
        {
            retry = TRUE;
            DEBUG_LOG_DEBUG("L2capManager SdpResultHandler: SDP search retry attempt: %d", psm_inst->sdp_search_attempts);

            L2CAP_MANAGER_SET_TYPED_BDADDR(tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
            l2capManager_SdpSearchReq(&tpaddr, TRUE, psm_inst);     /* Retry the SDP search. */
            report_sdp_search_failure = FALSE;                      /* Not yet. */
        }
        else
        {
            DEBUG_LOG_WARN("L2capManager SdpResultHandler: WARNING! All the SDP search attempts failed: %d", psm_inst->sdp_search_attempts);
        }
    }

    if (report_sdp_search_failure)
    {
        /* Let the client know that SDP search atttempt(s) have failed. */
        L2CAP_MANAGER_SET_TYPED_BDADDR(tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
        l2cap_inst = l2capManager_SearchL2capLinkInstanceByBdAddr(psm_inst, &tpaddr);
        PanicNull(l2cap_inst);
        l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_DISCONNECTED;

        l2capManager_NotifyFailedSdpSearch(psm_inst, l2cap_inst, &bd_addr);
        psm_inst->state = L2CAP_MANAGER_PSM_STATE_READY;
    }

    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);
    if (!retry)
    {
        if(l2cap_inst->link_status == L2CAP_MANAGER_LINK_STATE_DISCONNECTED)
        {
            MESSAGE_MAKE(msg, L2CAP_INTERNAL_CLOSE_SDP_T);
            msg->psm_inst = psm_inst;
            MessageSend(&task_inst->task, L2CAP_INTERNAL_CLOSE_SDP, msg);
        }
    }

    UNUSED(inst);
}

static void l2capManager_ResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

    if(cbType == CSR_SDC_OPT_CB_SEARCH_RESULT)
    {
        l2capManager_SdpResultHandler(params->instData, params->sdpTagList, params->deviceAddr, params->resultCode, params->resultSupplier);
    }
}

static void l2capManager_SdpSearchReq(const tp_bdaddr *tpaddr, const bool retry, l2cap_manager_psm_instance_t* psm_inst)
{
    l2cap_manager_sdp_search_pattern_t sdp_search_pattern;
    l2cap_manager_status_t  status;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    CmnCsrBtLinkedListStruct *sdpTagList = NULL;
    uint16 shIndex;
    CsrBtDeviceAddr addr;

    BdaddrConvertVmToBluestack(&addr, &tpaddr->taddr.addr);

    if (!psm_inst->sdp_search_data)
    {
        psm_inst->sdp_search_data = CsrBtUtilSdcInit(l2capManager_ResultHandler,
                                                      TrapToOxygenTask(&task_inst->task));
    }

    /* Read the SDP search pattern from the client's callback function. */
    status = (psm_inst->functions->get_sdp_search_pattern)(tpaddr, &sdp_search_pattern);
    PanicFalse(status == l2cap_manager_status_success);

    if(sdp_search_pattern.service_uuid_type == SDP_DATA_ELEMENT_SIZE_128_BITS)
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid128(sdpTagList,
                                                                     &sdp_search_pattern.service_uuid128,
                                                                     &shIndex);
    }
    else if(sdp_search_pattern.service_uuid_type == SDP_DATA_ELEMENT_SIZE_32_BITS)
    {
        sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList,
                                                                    sdp_search_pattern.service_uuid32,
                                                                    &shIndex);
    }
    else
    {
        return;
    }
    
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList,
                                         shIndex,
                                         CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER,
                                         (CsrUint8 *)sdp_search_pattern.attribute_list,
                                         sdp_search_pattern.attribute_list_size);

    psm_inst->sdp_search_max_retries = sdp_search_pattern.max_num_of_retries;
    if (retry == FALSE)
    {
        /* Reset the 'attempts' counter, as this is the first try. */
        psm_inst->sdp_search_attempts = 0;
    }

    /* Perform SDP search */
    psm_inst->state = L2CAP_MANAGER_PSM_STATE_SDP_SEARCH;
    CsrBtUtilSdcSearchStart((void *) task_inst,
                            psm_inst->sdp_search_data,
                            sdpTagList,
                            addr);

}

static void l2capManager_Connected(psm_t localPsm, CsrBtDeviceAddr deviceAddr,
                                   CsrBtConnId btConnId, l2ca_mtu_t mtu,
                                   psm_t local_psm, CsrBtResultCode status)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;
    bdaddr bd_addr;
    l2cap_manager_connect_cfm_t cfm_to_client =
    {
        .status               = l2cap_manager_connect_status_failed,
        .local_psm            = local_psm,
        .remote_psm           = L2CAP_MANAGER_PSM_INVALID,
        .tpaddr               = { 0 },
        .sink                 = 0,
        .connection_id        = 0,
        .mtu_remote           = 0,
        .flush_timeout_remote = 0,
        .mode                 = 0,
        .qos_remote           = { 0 }
    };
    l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByLocalPsm(localPsm);
    PanicNull(psm_inst);

    /* The pending counter must be more than zero. Otherwise, something goes wrong. */
    PanicFalse(0 < task_inst->pending_connections);
    task_inst->pending_connections--;

    BdaddrConvertBluestackToVm(&bd_addr, &deviceAddr);
    L2CAP_MANAGER_SET_TYPED_BDADDR(cfm_to_client.tpaddr, TRANSPORT_BREDR_ACL, TYPED_BDADDR_PUBLIC, bd_addr);
    l2cap_inst = l2capManager_SearchL2capLinkInstanceByBdAddr(psm_inst, &cfm_to_client.tpaddr);
    PanicNull(l2cap_inst);

    if(status == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        Sink sink = StreamL2capSink(CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId));
        Source source = StreamSourceFromSink(sink);

        DEBUG_LOG_DEBUG("L2capManager Connection Successful");

        DEBUG_LOG_VERBOSE("L2capManager Connected: Local PSM:            0x%04X", local_psm);
        DEBUG_LOG_VERBOSE("L2capManager Connected: sink:                 0x%04X", sink);
        DEBUG_LOG_VERBOSE("L2capManager Connected: Connection ID:        0x%04X", CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId));
        DEBUG_LOG_VERBOSE("L2capManager Connected: Remote addr:          %04X-%02X-%06X",
                          l2cap_inst->remote_dev.taddr.addr.nap, l2cap_inst->remote_dev.taddr.addr.uap, l2cap_inst->remote_dev.taddr.addr.lap);
        DEBUG_LOG_VERBOSE("L2capManager Connected: Remote MTU:           0x%04X", mtu);

        /* Notify the client that the connection has been established. */
        cfm_to_client.status               = l2cap_manager_connect_status_success;
        cfm_to_client.local_psm            = local_psm;
        cfm_to_client.sink                 = sink;
        cfm_to_client.connection_id        = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
        cfm_to_client.mtu_remote           = mtu;

        /* Set the link parameters to the link instance. */
        l2cap_inst->local_psm               = local_psm;
        l2cap_inst->connection_id           = CM_GET_UINT16ID_FROM_BTCONN_ID(btConnId);
        l2cap_inst->mtu_remote              = mtu;
        l2cap_inst->flush_timeout_remote    = 0;
        l2cap_inst->mode                    = 0;
        l2cap_inst->sink                    = sink;
        l2cap_inst->source                  = source;
        memset(&l2cap_inst->qos_remote, 0, sizeof(l2cap_inst->qos_remote));

        MessageStreamTaskFromSink(sink, &task_inst->task);
        MessageStreamTaskFromSource(source, &task_inst->task);

        PanicFalse(SinkConfigure(sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL));
        PanicFalse(SourceConfigure(source, VM_SOURCE_MESSAGES, VM_MESSAGES_ALL));

        /* Set the handover policy */
        PanicFalse(SourceConfigure(source, STREAM_SOURCE_HANDOVER_POLICY, SOURCE_HANDOVER_ALLOW_WITHOUT_DATA));

        l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_CONNECTED;
        psm_inst->state = L2CAP_MANAGER_PSM_STATE_CONNECTED;
    }
    else
    {
        DEBUG_LOG_DEBUG("L2capManager Connection Failed");
        /* Delete the L2CAP instance as the connection establishment attempt is failed. */
        l2capManager_DeleteL2capLinkInstanceBySink(psm_inst, L2CAP_MANAGER_INVALID_SINK);
        psm_inst->state = L2CAP_MANAGER_PSM_STATE_READY;
    }

    if (psm_inst->functions->handle_connect_cfm)
    (psm_inst->functions->handle_connect_cfm)(&cfm_to_client, l2cap_inst->context);
}

static void l2capManager_HandleCmL2caConnectCfm(const CsrBtCmL2caConnectCfm *cfm)
{
    DEBUG_LOG_DEBUG("L2capManager HandleCmL2caConnectCfm: (Status:%d Supplier:%d)", cfm->resultCode, cfm->resultSupplier);

    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByLocalPsm(cfm->localPsm);
    PanicNull(psm_inst);

    l2capManager_Connected(cfm->localPsm, cfm->deviceAddr, cfm->btConnId, cfm->mtu, cfm->localPsm, cfm->resultCode);

    if(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
    {
        if (psm_inst->sdp_search_data)
        {
            CsrBtUtilRfcConCancel(task_inst, psm_inst->sdp_search_data);
            CsrBtUtilSdcRfcDeinit(&psm_inst->sdp_search_data);
        }
    }
}


/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device. */
static void l2capManager_HandleCmL2caConnectAcceptCfm(const CsrBtCmL2caConnectAcceptCfm *cfm)
{
    DEBUG_LOG_DEBUG("L2capManager HandleCmL2caConnectAcceptCfm: (Status:%d Supplier:%d)", cfm->resultCode, cfm->resultSupplier);

    l2capManager_Connected(cfm->localPsm, cfm->deviceAddr, cfm->btConnId, cfm->mtu, cfm->localPsm, cfm->resultCode);
}

static void l2capManager_HandleCmL2caDisconnectInd(const CsrBtCmL2caDisconnectInd *ind)
{
    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;
    uint16 connection_id     = CM_GET_UINT16ID_FROM_BTCONN_ID(ind->btConnId);
    Sink sink;

    if (ind->reasonCode == L2CA_DISCONNECT_NORMAL)
        DEBUG_LOG_DEBUG("L2capManager HandleCmL2caDisconnectInd: Success");
    else if (ind->reasonCode == L2CA_DISCONNECT_TIMEOUT)
        DEBUG_LOG_DEBUG("L2capManager HandleCmL2caDisconnectInd: Timed out (No response from the peer).");    /* No response for 30 seconds results in this! */
    else
        DEBUG_LOG_WARN("L2capManager HandleCmL2caDisconnectInd: WARNING! The status is other than successful: %d", ind->reasonCode);

    if (FALSE == l2capManager_GetPsmAndL2capInstanceByConnId(connection_id, &psm_inst, &l2cap_inst))
    {
        DEBUG_LOG_ERROR("L2capManager HandleCmL2caDisconnectInd: ERROR! Failed to find the PSM/L2CAP instances for the connection id (0x%X)", connection_id);
        Panic();
    }

    StreamConnectDispose(l2cap_inst->source);

    if (!ind->localTerminated)
    {
        l2cap_manager_disconnect_ind_t ind_to_client;

        sink = StreamL2capSink(connection_id);

        /* Response is required only for remote disconnection. */
        ConnectionL2capDisconnectResponse(ind->l2caSignalId, sink);

        ind_to_client.identifier = ind->l2caSignalId;
        ind_to_client.status     = l2capManager_ConvertDisconnectStatus(ind->reasonCode);
        ind_to_client.sink       = sink;
        DEBUG_LOG_VERBOSE("L2capManager HandleDisconnectInd: (Identifier:0x%02X, Sink:0x%04X)", ind->l2caSignalId, sink);

        if (psm_inst->functions->respond_disconnect_ind)
        {
            (psm_inst->functions->respond_disconnect_ind)(&ind_to_client, l2cap_inst->context);
        }
    }
    else
    {
        l2cap_manager_disconnect_cfm_t cfm_to_client;

        sink = l2cap_inst->sink;

        cfm_to_client.status = l2capManager_ConvertDisconnectStatus(ind->reasonCode);
        cfm_to_client.sink = sink;
        DEBUG_LOG_VERBOSE("L2capManager HandleCmL2caDisconnectInd (localTerminated): (Sink:0x%04X)", l2cap_inst->sink);

        if (psm_inst->functions->handle_disconnect_cfm)
        {
            (psm_inst->functions->handle_disconnect_cfm)(&cfm_to_client, l2cap_inst->context);
        }
    }
    l2capManager_CleanUpByDisconnection(psm_inst, sink);
    psm_inst->state = L2CAP_MANAGER_PSM_STATE_READY;
}

static void l2capManager_HandleCmPrim(Message message)
{
    DEBUG_LOG("l2capManager_HandleCmPrim CM Prim ");

    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    if (*prim == CSR_BT_CM_SDS_REGISTER_CFM)
    {
        l2capManager_HandleCmSdsRegisterCfm((const CsrBtCmSdsRegisterCfm *) message);
    }
    else if (CsrBtUtilSdcVerifyCmMsg(prim))
    {
        l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();

        l2cap_manager_psm_instance_t *psm_inst = l2capManager_SearchPsmInstanceByState(L2CAP_MANAGER_PSM_STATE_SDP_SEARCH);

        if (psm_inst)
        {
            CsrBtUtilSdcCmMsgHandler(task_inst,
                                     psm_inst->sdp_search_data,
                                     prim);
        }
    }
    else
    {
        switch (*prim)
        {
            case CSR_BT_CM_L2CA_REGISTER_CFM:
                l2capManager_HandleCmL2caRegisterCfm((const CsrBtCmL2caRegisterCfm *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_IND:
                l2capManager_HandleCmL2caConnectAcceptInd((const CsrBtCmL2caConnectAcceptInd *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_CFM:
                l2capManager_HandleCmL2caConnectCfm((const CsrBtCmL2caConnectCfm *) message);
                break;

            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM:
                l2capManager_HandleCmL2caConnectAcceptCfm((const CsrBtCmL2caConnectAcceptCfm *) message);
                break;

            case CSR_BT_CM_L2CA_DISCONNECT_IND:
                l2capManager_HandleCmL2caDisconnectInd((const CsrBtCmL2caDisconnectInd *) message);
                break;

            default:
                DEBUG_LOG("l2capManager_HandleCmPrim Unhandled CM Prim 0x%04x",
                          *prim);
                break;
        }
    }
    CmFreeUpstreamMessageContents((void *) message);
}

static void l2capManager_CloseSdp(L2CAP_INTERNAL_CLOSE_SDP_T *msg)
{
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    if (msg->psm_inst->sdp_search_data)
    {
        DEBUG_LOG_DEBUG("L2capManager CloseSdp local psm: %d", msg->psm_inst->local_psm);
        CsrBtUtilRfcConCancel(task_inst, msg->psm_inst->sdp_search_data);
        CsrBtUtilSdcRfcDeinit(&msg->psm_inst->sdp_search_data);
    }
}

/******************************************************************************
 * The main message handler of the L2CAP manager
 ******************************************************************************/
/*! \brief L2CAP Manager task message handler.

    \note The connection library dependent function.
 */
static void l2capManager_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        /* Connection library messages */
        case MESSAGE_MORE_DATA:
            DEBUG_LOG_DEBUG("L2capManager HandleMessage: MESSAGE_MORE_DATA");
            l2capManager_HandleMessageMoreData((const MessageMoreData*) message);
            break;

        case MESSAGE_MORE_SPACE:
            DEBUG_LOG_DEBUG("L2capManager HandleMessage: MESSAGE_MORE_SPACE");
            l2capManager_HandleMessageMoreSpace((const MessageMoreSpace*) message);
            break;

        case CM_PRIM:
            l2capManager_HandleCmPrim(message);
            break;

        case L2CAP_INTERNAL_CLOSE_SDP:
            l2capManager_CloseSdp((L2CAP_INTERNAL_CLOSE_SDP_T *) message);
            break;

        default:
            DEBUG_LOG_WARN("L2capManager HandleMessage: Unhandled message: 0x%04X", id);
            break;
    }
}


/******************************************************************************
 * PUBLIC API
 ******************************************************************************/
bool L2capManager_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG_DEBUG("L2capManager Init");
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();
    memset(task_inst, 0, sizeof(*task_inst));

    task_inst->task.handler = l2capManager_HandleMessage;

    /* Initialise the linked-list of the PSM instances. */
    task_inst->pending_connections  = 0;
    task_inst->num_of_psm_instances = 0;
    task_inst->psm_instances        = NULL;

    return TRUE;
}


l2cap_manager_status_t L2capManager_Register(uint16 psm, const l2cap_manager_functions_t* functions, l2cap_manager_instance_id *instance_id)
{
    l2cap_manager_psm_instance_t *psm_inst;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();

    DEBUG_LOG_DEBUG("L2capManager Register");
    /* Both 'process_more_data' and 'process_more_space' handlers must be set.
     * Or, both 'process_more_data' and 'process_more_space' handlers must be NULL. */
    if (functions->process_more_data)
    {
        if (functions->process_more_space == NULL)
        {
            DEBUG_LOG_ERROR("L2capManager Register: ERROR! 'process_more_space' handler must be set if 'process_more_data' handler is set.");
            Panic();
        }
    }
    else
    {
        if (functions->process_more_space)
        {
            DEBUG_LOG_ERROR("L2capManager Register: ERROR! 'process_more_data' handler must be set if 'process_more_space' handler is set.");
            Panic();
        }
    }

    *instance_id = L2CAP_MANAGER_PSM_INSTANCE_ID_INVALID;
    psm_inst = l2capManager_CreatePsmInstance();
    if (psm_inst == NULL)
        return l2cap_manager_failed_to_allocate_an_instance;

    DEBUG_LOG_VERBOSE("L2capManager Register: (psm_inst:%p)", psm_inst);

    /* Initialise the new PSM instance. */
    psm_inst->state               = L2CAP_MANAGER_PSM_STATE_PSM_REGISTRATION;
    psm_inst->local_psm           = psm;
    psm_inst->remote_psm          = L2CAP_MANAGER_PSM_INVALID;
    psm_inst->sdp_search_attempts = 0;
    psm_inst->num_of_links        = 0;
    psm_inst->l2cap_instances     = NULL;
    psm_inst->functions           = functions;
    *instance_id = psm_inst->instance_id;
    psm_inst->sdp_search_data = NULL;

    DEBUG_LOG_VERBOSE("L2capManager Register: functions->get_sdp_record:           %p", functions->get_sdp_record);
    DEBUG_LOG_VERBOSE("L2capManager Register: psm_inst->functions:                 %p", psm_inst->functions);
    DEBUG_LOG_VERBOSE("L2capManager Register: psm_inst->functions->get_sdp_record: %p", psm_inst->functions->get_sdp_record);

    /* Register a Protocol/Service Multiplexor (PSM) that will be used for this
     * client. The remote device can use the same or a different PSM at its end. */
    ConnectionL2capRegisterRequest(&task_inst->task, psm, 0);

    return l2cap_manager_status_success;
}


l2cap_manager_status_t L2capManager_Connect(const tp_bdaddr *tpaddr, l2cap_manager_instance_id instance_id, void *context)
{
    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t* l2cap_inst = NULL;

    DEBUG_LOG_DEBUG("L2capManager Connect");

    /* Find the PSM instance from the linked-list. */
    psm_inst = l2capManager_SearchPsmInstance(instance_id);
    PanicNull(psm_inst);    /* NB: The PSM must be registered prior to calling this function. */

    /* Create a new L2CAP link instance. */
    l2capManager_CreateL2capLinkInstance(psm_inst, tpaddr, &l2cap_inst);
    if (l2cap_inst == NULL)
        return l2cap_manager_failed_to_allocate_an_instance;

    /* Note that this 'context' pointer is used by the client.
        * We just save this for the client's use later. */
    l2cap_inst->context = context;

    /* Check if the remote PSM is known or not. */
    if (psm_inst->remote_psm == L2CAP_MANAGER_PSM_INVALID)
    {
        /* Request SDP search, as we need to get the remote PSM by SDP search. */
        l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_LOCAL_INITIATED_SDP_SEARCH;
        l2capManager_SdpSearchReq(tpaddr, FALSE, psm_inst);      /* First attempt of the SDP search. */
    }
    else
    {
        /* The remote PSM is already known.
         * Initiate an L2CAP connection request. */
        l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_LOCAL_INITIATED_CONNECTING;
        l2capManager_ConnectL2cap(tpaddr, psm_inst);
    }

    return l2cap_manager_status_success;
}


l2cap_manager_status_t L2capManager_Disconnect(Sink sink, l2cap_manager_instance_id instance_id)
{
    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;
    l2cap_manager_task_data_t *task_inst = l2capManagerGetTaskData();

    DEBUG_LOG_DEBUG("L2capManager Disconnect");
    PanicNull(sink);

    /* Make sure that the PSM instance with the 'instance_id' exists. */
    psm_inst = l2capManager_SearchPsmInstance(instance_id);
    PanicNull(psm_inst);    /* NB: The PSM must be registered prior to calling this function. */

    if (l2capManager_GetPsmAndL2capInstanceBySink(sink, &psm_inst, &l2cap_inst))
    {
        if(l2cap_inst->link_status == L2CAP_MANAGER_LINK_STATE_LOCAL_INITIATED_SDP_SEARCH)
        {
            if (psm_inst->sdp_search_data)
            {
                DEBUG_LOG_DEBUG("L2capManager CloseSdp");
                CsrBtUtilRfcConCancel(task_inst, psm_inst->sdp_search_data);
                CsrBtUtilSdcRfcDeinit(&psm_inst->sdp_search_data);
            }
        }
        l2cap_inst->link_status = L2CAP_MANAGER_LINK_STATE_DISCONNECTING;
    }
    else
    {
        DEBUG_LOG_ERROR("L2capManager Disconnect: ERROR! Failed to find the PSM/L2CAP instances for the sink (0x%p)", sink);
        Panic();
    }

    /* Tell the connection library to disconnect the link. */
    ConnectionL2capDisconnectRequest(&task_inst->task, sink);

    return l2cap_manager_status_success;
}

bool L2capManager_IsConnected(const tp_bdaddr *tpaddr, l2cap_manager_instance_id instance_id)
{
    DEBUG_LOG_DEBUG("L2capManager_IsConnected");

    l2cap_manager_psm_instance_t *psm_inst = NULL;
    l2cap_manager_l2cap_link_instance_t *l2cap_inst = NULL;

    /* Make sure that the PSM instance with the 'instance_id' exists. */
    psm_inst = l2capManager_SearchPsmInstance(instance_id);
    PanicNull(psm_inst);

    l2cap_inst = l2capManager_SearchL2capLinkInstanceByBdAddr(psm_inst, tpaddr);
    if (l2cap_inst)
    {
        l2cap_manager_link_state_t state = l2cap_inst->link_status;
        if ((state != L2CAP_MANAGER_LINK_STATE_DISCONNECTED) &&
            (state != L2CAP_MANAGER_LINK_STATE_DISCONNECTING) &&
            (state != L2CAP_MANAGER_LINK_STATE_NULL))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif /* INCLUDE_L2CAP_MANAGER */
