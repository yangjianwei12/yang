#include "earbud_topology_simple.h"

#include "tws_topology.h"

#include "phy_state.h"
#include "panic.h"
#include "logging.h"
#include "cc_with_case.h"
#include "handset_service.h"
#include "state_proxy.h"
#include "mirror_profile.h"
#include "bandwidth_manager.h"
#include "tws_topology_goals.h"

#include "handset_service.h"

static void earbudTopologySimple_Init(void);
static void earbudTopologySimple_Deinit(void);
static bool earbudTopologySimple_AuthoriseStartHandover(hdma_handover_reason_t handover_reason);
static void earbudTopologySimple_MessageHandler(Task task, MessageId id, Message message);

TaskData app_tws_topology_simple_task = {.handler = earbudTopologySimple_MessageHandler};

static const tws_topology_product_behaviour_t tws_topology_config =
{
    .device_type = topology_device_type_with_peer,
    .support_role_swap = TRUE,
    .peer_search = {0},
    .init = earbudTopologySimple_Init,
    .deinit = earbudTopologySimple_Deinit,
    .authoriseStartRoleSwap = earbudTopologySimple_AuthoriseStartHandover,
    .timeouts = TWS_TOPOLOGY_DEFAULT_TIMEOUTS,
};

const tws_topology_product_behaviour_t* EarbudTopologySimple_GetConfig(void)
{
    return &tws_topology_config;
}

static void earbudTopologySimple_Init(void)
{
    TwsTopology_RegisterMessageClient(&app_tws_topology_simple_task);
}

static void earbudTopologySimple_Deinit(void)
{
    TwsTopology_UnRegisterMessageClient(&app_tws_topology_simple_task);
}

static bool earbudTopologySimple_AuthoriseStartHandover(hdma_handover_reason_t handover_reason)
{
    bool authorise = FALSE;
    if(handover_reason == HDMA_HANDOVER_REASON_EXTERNAL)
    {
        /* Only authorise explicit app. requested handovers */
        authorise = TRUE;
    }
    return authorise;
}

static void earbudTopologySimple_AllowHandsetInvokedConnections(void)
{
    ConManagerAllowHandsetConnect(TRUE);
    HandsetService_ConnectableRequest(&app_tws_topology_simple_task);
}

static void earbudTopologySimple_DisconnectAndBlockHandsets(void)
{
    HandsetService_DisconnectAll(&app_tws_topology_simple_task, HCI_ERROR_OETC_USER);
    HandsetService_StopReconnect(&app_tws_topology_simple_task);
    HandsetService_CancelConnectableRequest(&app_tws_topology_simple_task);
    ConManagerAllowHandsetConnect(FALSE);
}

static void earbudTopologySimple_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED:
        {
            TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T* msg = (TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T*)message;
            if(msg->role == tws_topology_role_primary)
            {
                earbudTopologySimple_AllowHandsetInvokedConnections();
            }
            else
            {
                earbudTopologySimple_DisconnectAndBlockHandsets();
            }
        }
        break;

        default:
        break;
    }
}
