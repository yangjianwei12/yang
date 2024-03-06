#include "earbud_topology_default.h"

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
#include "earbud_sm.h"

static void earbudTopologyDefault_MessageHandler(Task task, MessageId id, Message message);

TaskData app_tws_topology_task = {.handler = earbudTopologyDefault_MessageHandler};

static void earbudTopologyDefault_Init(void);
static void earbudTopologyDefault_Deinit(void);
static bool earbudTopologyDefault_AuthoriseStartHandover(hdma_handover_reason_t handover_reason);

typedef enum {
    REJOIN_TOPOLOGY_INTERNAL_MESSAGE = INTERNAL_MESSAGE_BASE
}earbud_topology_internal_messsage_t;

static const tws_topology_product_behaviour_t tws_topology_config =
{
    .device_type = topology_device_type_with_peer,
    .support_role_swap = TRUE,
    .peer_search = {0},
    .init = earbudTopologyDefault_Init,
    .deinit = earbudTopologyDefault_Deinit,
    .authoriseStartRoleSwap = earbudTopologyDefault_AuthoriseStartHandover,
    .timeouts = TWS_TOPOLOGY_DEFAULT_TIMEOUTS,
};

const tws_topology_product_behaviour_t* EarbudTopologyDefault_GetConfig(void)
{
    return &tws_topology_config;
}

static void earbudTopologyDefault_RegisterForStateProxyEvents(void)
{
    if (earbudTopology_StateProxyEventsOfInterestMask())
    {
        StateProxy_EventRegisterClient(&app_tws_topology_task, earbudTopology_StateProxyEventsOfInterestMask());
    }
}

static void earbudTopologyDefault_Deinit(void)
{
    appPhyStateUnregisterClient(&app_tws_topology_task);
#if defined(INCLUDE_CASE_COMMS) && defined(HAVE_CC_MODE_EARBUDS)
    CcWithCase_UnregisterStateClient(&app_tws_topology_task);
#endif
    TwsTopology_UnRegisterMessageClient(&app_tws_topology_task);
    appPowerClientUnregister(&app_tws_topology_task);
    StateProxy_EventUnregisterClient(&app_tws_topology_task, earbudTopology_StateProxyEventsOfInterestMask());
}

static void earbudTopologyDefault_Init(void)
{
    appPhyStateRegisterClient(&app_tws_topology_task);
#if defined(INCLUDE_CASE_COMMS) && defined(HAVE_CC_MODE_EARBUDS)
    CcWithCase_RegisterStateClient(&app_tws_topology_task);
#endif
    TwsTopology_RegisterMessageClient(&app_tws_topology_task);
    appPowerClientRegister(&app_tws_topology_task);    
    appPowerClientAllowSleep(&app_tws_topology_task);
    earbudTopologyDefault_RegisterForStateProxyEvents();      
}

static bool earbudTopologyDefault_isHandsetConnected(void)
{
    bool authorise = FALSE;
#ifdef ENABLE_LE_HANDOVER
    /* When LE handover is enabled HDMA is required if the handset is connected
    over any transport */
    bool handset_connected = appDeviceIsHandsetConnected();
#else
    bool handset_connected = appDeviceIsBredrHandsetConnected();
#endif
    bool handset_reconnecting = HandsetService_IsHandsetInBredrContextPresent(handset_bredr_context_link_loss_reconnecting);

    if (handset_connected || handset_reconnecting)
    {
        DEBUG_LOG_INFO("earbudTopologyDefault_isHandsetConnected run");
        authorise = TRUE;
    }
    else
    {
        DEBUG_LOG_INFO("earbudTopologyDefault_isHandsetConnected ignore - (handset_connected %u | handset reconnecting %u",
                  handset_connected, handset_reconnecting);
    }
    return authorise;
}

static bool earbudTopologyDefault_AuthoriseStartHandover(hdma_handover_reason_t handover_reason)
{
    bool authorise = FALSE;
    if(earbudTopologyDefault_isHandsetConnected())
    {
    #ifdef ENABLE_LE_HANDOVER
        if (!HandsetService_IsAnyDeviceConnected())
    #else
        if (!MirrorProfile_IsBredrMirroringConnected())
    #endif
        {
            DEBUG_LOG_INFO("earbudTopologyDefault_AuthoriseStartHandover, ignore as mirroring disconnected");
        }
        else if ((handover_reason == HDMA_HANDOVER_REASON_RSSI) && (BandwidthManager_IsFeatureRunning(BANDWIDTH_MGR_FEATURE_A2DP_LL)))
        {
            DEBUG_LOG_INFO("earbudTopologyDefault_AuthoriseStartHandover, ignore as aptX adaptive is in low latency mode");
        }
        else
        {
            authorise = TRUE;
        }
    }
    DEBUG_LOG("earbudTopologyDefault_AuthoriseStartHandover: authorise=%d", authorise);
    return authorise;
}

static void earbudTopologyDefault_AllowHandsetInvokedConnections(void)
{
    DEBUG_LOG("earbudTopologyDefault_AllowHandsetInvokedConnections");
    ConManagerAllowHandsetConnect(TRUE);
    HandsetService_ConnectableRequest(&app_tws_topology_task);
}

static void earbudTopologyDefault_DisconnectAndBlockHandsets(void)
{
    DEBUG_LOG("earbudTopologyDefault_DisconnectAndBlockHandsets");
    HandsetService_DisconnectAll(&app_tws_topology_task, HCI_ERROR_OETC_USER);
    HandsetService_StopReconnect(&app_tws_topology_task);
    HandsetService_CancelConnectableRequest(&app_tws_topology_task);
    ConManagerAllowHandsetConnect(FALSE);
}

static inline void earbudTopologyDefault_LeaveIfKeepAliveForDfuDisabled(void)
{
    if(!appIsKeepTopologyAliveForDfuEnabled())
    {
        TwsTopology_Leave(role_change_force_reset);
    }
}

static void earbudTopologyDefault_HandleInCaseEvent(void)
{
    if (CcWithCase_EventsEnabled())
    {
        if(!HandsetService_IsAnyDeviceConnected())
        {
            earbudTopologyDefault_LeaveIfKeepAliveForDfuDisabled();
        }
        else
        {
            // Handset connected. Leave if a) secondary b) standalone primary c) primary connected to peer and peer is out of case
            if(!TwsTopology_IsRolePrimaryConnectedToPeer() ||
                    (TwsTopology_IsRolePrimaryConnectedToPeer() && StateProxy_IsPeerOutOfCase()))
            {
                earbudTopologyDefault_LeaveIfKeepAliveForDfuDisabled();
            }
        }
    }
    else
    {
        earbudTopologyDefault_LeaveIfKeepAliveForDfuDisabled();
    }
}

static inline void earbudTopologyDefault_HandleOutOfCaseEvent(void)
{
    if(!TwsTopology_IsRolePrimary() && !TwsTopology_IsRoleSecondary())
    {
        TwsTopology_Join();
    }
}

static bool earbudTopologyDefault_HandlePhyStateChange(const PHY_STATE_CHANGED_IND_T* phy_state)
{
    DEBUG_LOG("earbudTopologyDefault_HandlePhyStateChange state=enum:phyState:%d, event=enum:phy_state_event:%d", phy_state->new_state, phy_state->event);
    if (phy_state->new_state == PHY_STATE_UNKNOWN)
    {
        DEBUG_LOG_WARN("earbudTopologyDefault_HandlePhyStateChange PHY_STATE_UNKNOWN");
    }
    else if (phy_state->new_state == PHY_STATE_IN_CASE)
    {
        earbudTopologyDefault_HandleInCaseEvent();
    }
    else
    {
        earbudTopologyDefault_HandleOutOfCaseEvent();
    }

    return TRUE;
}

#if defined(INCLUDE_CASE_COMMS) && defined(HAVE_CC_MODE_EARBUDS)
static inline void earbudTopologyDefault_HandleCaseLidStateChange(const CASE_LID_STATE_T* case_comms)
{    
    PanicFalse(CcWithCase_EventsEnabled());
    
    if(case_comms->lid_state != CASE_LID_STATE_UNKNOWN)
    {
        if (CcWithCase_GetLidState() == CASE_LID_STATE_OPEN)
        {
            TwsTopology_Join();
        }
        else if (CcWithCase_GetLidState() == CASE_LID_STATE_CLOSED)
        {
            if(!appPhyStateIsOutOfCase())
            {
                earbudTopologyDefault_LeaveIfKeepAliveForDfuDisabled();
            }
        }
        else
        {
            // Unknown state
            Panic();
        }
    }
}
#endif

static inline bool earbudTopologyDefault_IsDeviceInHandsetConnectableState(TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T* msg)
{
    return msg->role == tws_topology_role_primary && (appIsKeepTopologyAliveForDfuEnabled() || 
                                                      appPhyStateIsOutOfCase() ||
                                                      (CcWithCase_EventsEnabled() && CcWithCase_GetLidState() == CASE_LID_STATE_OPEN));
}

static inline bool earbudTopologyDefault_IsRejoinRequired(TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T* msg)
{
    return (msg->role == tws_topology_role_none) && msg->error_forced_no_role && 
                        (appSmIsOutOfCase() || 
                        (CcWithCase_EventsEnabled() && (CcWithCase_GetLidState() == CASE_LID_STATE_OPEN)));
}

static void earbudTopologyDefault_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
        case APP_POWER_SLEEP_PREPARE_IND:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler APP_POWER_SLEEP_PREPARE_IND");    
            appPowerSleepPrepareResponse(&app_tws_topology_task);
        break;
        
        case APP_POWER_SHUTDOWN_PREPARE_IND:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler APP_POWER_SHUTDOWN_PREPARE_IND");
            TwsTopology_Leave(role_change_force_reset);
        break;
        
        case PHY_STATE_CHANGED_IND:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler PHY_STATE_CHANGED_IND");
            earbudTopologyDefault_HandlePhyStateChange((PHY_STATE_CHANGED_IND_T*)(message));
        break;
            
        case TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED");
            if(appIsKeepTopologyAliveForDfuEnabled())
            {
                TwsTopology_Join();
            }                                
        break;            

#if defined(INCLUDE_CASE_COMMS) && defined(HAVE_CC_MODE_EARBUDS)
        case CASE_LID_STATE:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler CASE_LID_STATE");
            earbudTopologyDefault_HandleCaseLidStateChange((CASE_LID_STATE_T*)message);
        break;
#endif

        case TWS_TOPOLOGY_REQUEST_JOIN_COMPLETED:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler TWS_TOPOLOGY_REQUEST_JOIN_IND");
        break;

        case TWS_TOPOLOGY_REQUEST_LEAVE_COMPLETED:
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler TWS_TOPOLOGY_REQUEST_LEAVE_IND");
            if(appPowerIsShuttingDown())
            {
                appPowerShutdownPrepareResponse(&app_tws_topology_task);   
            }
        break;

        case TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED:
        {
            TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T* msg = (TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T*)message;
            DEBUG_LOG_INFO("earbudTopologyDefault_MessageHandler TWS_TOPOLOGY_ROLE_CHANGE_COMPLETE role=enum:tws_topology_role:%d", msg->role);
            if(earbudTopologyDefault_IsDeviceInHandsetConnectableState(msg))
            {
                earbudTopologyDefault_AllowHandsetInvokedConnections();
            }
            else
            {
                earbudTopologyDefault_DisconnectAndBlockHandsets();
                if(earbudTopologyDefault_IsRejoinRequired(msg))
                {
                    MessageSend(&app_tws_topology_task, REJOIN_TOPOLOGY_INTERNAL_MESSAGE, NULL);
                }
            }
        }
        break;

        case REJOIN_TOPOLOGY_INTERNAL_MESSAGE:
            TwsTopology_Join();
        break;
        
        default:
            DEBUG_LOG_WARN("earbudTopologyDefault_MessageHandler Unhandled message %d", id);
        break;
    }
}
