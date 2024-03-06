/*!
```code
\copyright  Copyright (c) 2019-2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   tws TWS Topology
\ingroup    topologies
\brief      TWS topology public interface.
*/

#ifndef TWS_TOPOLOGY_H_
#define TWS_TOPOLOGY_H_

#include <message.h>
#include <bdaddr.h>

#include <peer_find_role.h>
#include <power_manager.h>

#include "domain_message.h"
#include "hdma.h"

#include <bdaddr.h>

#include "tws_topology_defaults.h"

/*!
```

# 1. Overview
---
The topology component is responsible for managing the roles and connection between devices which make a product. A product may be a standalone device or a pair of devices e.g. earbuds. 
Topology is not responsible for managing the connection between these devices and an AG i.e. topology is only concerned with managing connections between identical devices that have a potential 
peer relationship.

Topology provides an API that allows for application level control of devices and their peer (if any). An application must decide when a device joins/leaves topology, which will determine 
when a role is assigned and, if the device has a peer, the point at which it joins the peer network. Upon joining the peer network a device will assume the role of primary or secondary device.
The first device to join the network will be assigned the primary role. The primary device is responsible for accepting connection requests from a peer as well as being controlled by the application
to make/accept connections to an AG. A primary device has two sub-roles: standalone (device not connected to peer) or coupled (device connected with peer). Once a peer is established, the primary
device is responsible for mirroring the AG links to the secondary device. The primary is also responsible for triggering handover requests, which involves coordinating the tasks required to complete
the swapping of primary and secondary roles; this responsibility includes notifying interested clients of a role change. The second device that joins the peer network shall be assigned the role of 
secondary. The secondary device largely behaves as a 'dumb' device, executing the instructions it is given by the primary e.g. volume change

***Note***: *INFORMATION DETAILED WITHIN THIS DOCUMENT CURRENTLY PERTAINS TO EARBUD TOPOLOGY ONLY*

## 1.1. Responsibilities
---
Topology is reponsible for handling:
- Peer pairing
- Assigning role
- Allowing/disallowing peer connections
- Swapping roles
- Notifying clients of Topology events (including role change)

***Note***: *TOPOLOGY IS NOT RESPONSIBLE FOR MANAGING HANDSETS*

# 2. Design
---
Topology coordinates/controls a number of components in order to meet its requirements (this can be seen in Dependencies). At the core of the topology design is the Topology State Machine. 
The state machine has a number of features:
- Enter & Exit functions
- Goals.
    - A goal is an action (or set of actions) that must be successfully completed.
    - Each state may have a goal that must be achieved before transitioning to the next state.
- Procedures.
    - Procedures form the code that is executed in order to achieve a goal
    - A goal may have multiple procedures that are required in order to satisfy the goal. A set of procedures is known as a procedure 'script'.

# 3. Dependencies
---
Topology has a dependency on a number of components. The component dependencies are almost entirely related to finding role, connecting to peer, and swapping roles. 

## 3.1 Topology Component Diagrams
---
The following component diagrams illustatrate the dependencies that TWS Topology has on external components and the external component API that Topology uses.
### 3.1.1 TWS Topology &rarr; Services Dependencies
```plantuml
@startuml
package topologies {
package tws_component {
class tws
}
}
package topologies {
package common_component {
interface common_api_used_by_tws 
common_api_used_by_tws : GoalsEngine_ActivateGoal
common_api_used_by_tws : GoalsEngine_ClearGoal
common_api_used_by_tws : GoalsEngine_CreateGoalSet
common_api_used_by_tws : GoalsEngine_FindGoalForProcedure
common_api_used_by_tws : GoalsEngine_IsAnyGoalPending
common_api_used_by_tws : GoalsEngine_IsGoalActive
common_api_used_by_tws : GoalsEngine_IsGoalQueued
common_api_used_by_tws : Procedures_DelayedCancelCfmCallback
common_api_used_by_tws : Procedures_DelayedCompleteCfmCallback
}
}
package services {
package hdma_component {
interface hdma_api_used_by_tws 
hdma_api_used_by_tws : Hdma_Destroy
hdma_api_used_by_tws : Hdma_Init
hdma_api_used_by_tws : Hdma_ExternalHandoverRequest
}
}
package services {
package peer_find_role_component {
interface peer_find_role_api_used_by_tws 
peer_find_role_api_used_by_tws : PeerFindRole_SetPreservedRoleInGlobalVariable
peer_find_role_api_used_by_tws : PeerFindRole_FindRole
peer_find_role_api_used_by_tws : PeerFindRole_IsActive
peer_find_role_api_used_by_tws : PeerFindRole_FindRoleCancel
peer_find_role_api_used_by_tws : PeerFindRole_RegisterTask
peer_find_role_api_used_by_tws : PeerFindRole_UnregisterTask
peer_find_role_api_used_by_tws : PeerFindRole_GetFixedRole
}
}
package services {
package state_proxy_component {
interface state_proxy_api_used_by_tws 
state_proxy_api_used_by_tws : StateProxy_InitialStateReceived
}
}
package services {
package peer_pair_le_component {
interface peer_pair_le_api_used_by_tws 
peer_pair_le_api_used_by_tws : PeerPairLe_FindPeer
peer_pair_le_api_used_by_tws : PeerPairLe_PairPeerWithAddress
}

common_api_used_by_tws <-- tws
peer_find_role_api_used_by_tws <-- tws
hdma_api_used_by_tws <-- tws
state_proxy_api_used_by_tws <-- tws
peer_pair_le_api_used_by_tws <-- tws

@enduml
```
### 3.1.2 TWS Topology &rarr; Domain Dependencies (Peer Connection Control)
```plantuml
@startuml
package topologies {
package tws_component {
class tws
}
}
package domains {
package pairing_component {
interface pairing_api_used_by_tws 
pairing_api_used_by_tws : Pairing_ActivityClientRegister
}
}
package domains {
package handover_profile_component {
interface handover_profile_api_used_by_tws 
handover_profile_api_used_by_tws : HandoverProfile_Connect
handover_profile_api_used_by_tws : HandoverProfile_TerminateSdpPrimitive
handover_profile_api_used_by_tws : HandoverProfile_Handover
}
}
package domains {
package mirror_profile_component {
interface mirror_profile_api_used_by_tws 
mirror_profile_api_used_by_tws : MirrorProfile_ClientRegister
mirror_profile_api_used_by_tws : MirrorProfile_Connect
mirror_profile_api_used_by_tws : MirrorProfile_TerminateSdpPrimitive
}
}
package domains {
package sdp_component {
interface sdp_api_used_by_tws 
sdp_api_used_by_tws : appSdpGetTwsSinkServiceRecord
sdp_api_used_by_tws : appSdpGetTwsSinkServiceRecordSize
sdp_api_used_by_tws : appSdpSetTwsSinkServiceRecordPeerBdAddr
}
}
sdp_api_used_by_tws <-- tws
handover_profile_api_used_by_tws <-- tws
pairing_api_used_by_tws <-- tws
mirror_profile_api_used_by_tws <-- tws
@enduml
```
### 3.1.2 TWS Topology &rarr; Domain Dependencies (BT)
```plantuml
@startuml
package topologies {
package tws_component {
class tws
}
}
package domains {
package bt_device_component {
interface bt_device_api_used_by_tws 
bt_device_api_used_by_tws : appDeviceIsPeer
bt_device_api_used_by_tws : BtDevice_IsPairedWithPeer
bt_device_api_used_by_tws : appDeviceIsBredrHandsetConnected
bt_device_api_used_by_tws : BtDevice_SetMyAddress
bt_device_api_used_by_tws : appDeviceGetPrimaryBdAddr
bt_device_api_used_by_tws : appDeviceGetPeerBdAddr
bt_device_api_used_by_tws : appDeviceGetSecondaryBdAddr
bt_device_api_used_by_tws : appDeviceIsPeerConnected
}
}
package domains {
package connection_manager_component {
connection_manager_api_used_by_tws : ConManagerRegisterConnectionsClient
connection_manager_api_used_by_tws : ConManagerTerminateAllAcls
connection_manager_api_used_by_tws : ConManagerSendCloseAclRequest
connection_manager_api_used_by_tws : ConManagerReleaseAcl
connection_manager_api_used_by_tws : ConManagerCreateAcl
connection_manager_api_used_by_tws : ConManagerAllowConnection
connection_manager_api_used_by_tws : ConManagerIsConnected
connection_manager_api_used_by_tws : ConManagerDisconnectAllLeConnectionsRequest
connection_manager_api_used_by_tws : ConManagerRegisterTpConnectionsObserver
connection_manager_api_used_by_tws : ConManagerUnregisterTpConnectionsObserver
}
}
package domains {
package bredr_scan_manager_component {
interface bredr_scan_manager_api_used_by_tws 
bredr_scan_manager_api_used_by_tws : BredrScanManager_InquiryScanParametersRegister
bredr_scan_manager_api_used_by_tws : BredrScanManager_PageScanParametersRegister
bredr_scan_manager_api_used_by_tws : BredrScanManager_PageScanRelease
bredr_scan_manager_api_used_by_tws : BredrScanManager_PageScanRequest
bredr_scan_manager_api_used_by_tws : BredrScanManager_ScanDisable
bredr_scan_manager_api_used_by_tws : BredrScanManager_ScanEnable
}
}
package domains {
package link_policy_component {
interface link_policy_api_used_by_tws 
link_policy_api_used_by_tws : appLinkPolicyHandleAddressSwap
}
}
package domains {
package le_advertising_manager_component {
interface le_advertising_manager_api_used_by_tws 
le_advertising_manager_api_used_by_tws : LeAdvertisingManager_ParametersRegister
le_advertising_manager_api_used_by_tws : LeAdvertisingManager_AllowAdvertising
le_advertising_manager_api_used_by_tws : LeAdvertisingManager_ParametersSelect
}
}
package domains {
package le_scan_manager_component {
interface le_scan_manager_api_used_by_tws 
le_scan_manager_api_used_by_tws : LeScanManager_Disable
le_scan_manager_api_used_by_tws : LeScanManager_Enable
}
}

bt_device_api_used_by_tws <-- tws
connection_manager_api_used_by_tws <-- tws
bredr_scan_manager_api_used_by_tws <-- tws
link_policy_api_used_by_tws <-- tws
le_advertising_manager_api_used_by_tws <-- tws
le_scan_manager_api_used_by_tws <-- tws
@enduml
```
### 3.1.3 TWS Topology &rarr; Domain Dependencies (Utility)
```plantuml
@startuml
package topologies {
package tws_component {
class tws
}
}
package domains {
package peer_signalling_component {
interface peer_signalling_api_used_by_tws 
peer_signalling_api_used_by_tws : appPeerSigClientRegister
peer_signalling_api_used_by_tws : appPeerSigMarshalledMsgChannelTaskRegister
peer_signalling_api_used_by_tws : appPeerSigConnect
peer_signalling_api_used_by_tws : appPeerSigTerminateSdpPrimitive
peer_signalling_api_used_by_tws : appPeerSigMarshalledMsgChannelTx
}
}
package domains {
package power_component {
interface power_api_used_by_tws 
power_api_used_by_tws : appPowerPerformanceProfileRelinquish
power_api_used_by_tws : appPowerPerformanceProfileRequest
power_api_used_by_tws : SystemState_GetTransitionTask
}
}
package domains {
package common_component {
interface common_domain_api_used_by_tws 
common_domain_api_used_by_tws : TimestampEvent
}
}
peer_signalling_api_used_by_tws <-- tws
power_api_used_by_tws <-- tws
common_domain_api_used_by_tws <-- tws
@enduml
```

## 3.2 Handover Trigger Sequence Diagram
---
HDMA (Handover Decision Making Algorithm) is worth a little more attention: HDMA is responsible for deciding when conditions have been met that should trigger a handover (e.g. battery level low). 
HDMA can be configured through a number of defines (see defines in hdma utils.h). Topology initialises HDMA and in doing so becomes the exclusive recipient of hdma notifications. Handover can 
then be triggered in two different ways 1) HDMA determining that a handover should take place due to device conditions 2) the application explicitly requesting a handover should take place. 
In both cases Topology triggers the start of the handover process.

*Note: The period that Topology will attempt handover, retrying failed attempts, can be configured within the timeouts parameter of tws_topology_product_behaviour_t*
```plantuml
@startuml
participant app
participant tws
participant hdma
participant handover_profile
participant hdma_handover_trigger 

app -> tws : TwsTopology_EnableRoleSwapSupport
group handover initiated by hdma
note over hdma
HDMA can be configured to trigger on events like
like battery level, rssi etc.
end note

tws -> hdma : Hdma_Init
hdma_handover_trigger -> hdma : <Battery Low>
hdma -> tws : HDMA_HANDOVER_NOTIFICATION
tws -> handover_profile : HandoverProfile_Handover 
end group

group handover initiated by app
app -> tws : TwsTopology_SwapRole
tws -> handover_profile : HandoverProfile_Handover
end group
@enduml
```
# 4. Control
---
Control of topology is managed entirely at the application level. The majority of Topology API calls are made in earbud_topology_default.c with, for historical reasons, some API calls being made in earbud_sm.c. 
- earbud_sm.c:
    - Configuration of Topology behaviour
    - Starting and stopping Topology
- earbud_topology_default.c:
    - Joining/Leaving Topology

# 5. Simplified Use Cases
---
```code
-------------------------------
|          KEY                |
|-----------------------------|
| X         | PFR DISABLED    |
|-----------|-----------------|
| O-------> | PFR ENABLED (L) |
|-----------|-----------------|
| <-------O | PFR ENABLED (R) |
|-----------|-----------------|
| O-------O | PEER CONNECTION |
|-----------------------------|

----------------------------------------         ----------------------------------------           ----------------------------------------          ---------------------------------------
|          PRODUCT BEHAVIOUR           |         |              DEVICE 1                |           |              DEVICE 2                |          |          PRODUCT BEHAVIOUR          |
|--------------------------------------|         |--------------------------------------|           |--------------------------------------|          |-------------------------------------|
| 1) Initialise Topology               |    (1a) | TwsTopology_Init                     |           | TwsTopology_Init                     | (5a)     | 5) Initialise Topology              |
|--------------------------------------|         |--------------------------------------|           |--------------------------------------|          |-------------------------------------|
| Event:Device taken out of case...    |    (1b) | TwsTopology_ConfigureProductBehaviour|           | TwsTopology_ConfigureProductBehaviour| (5b)     | Event:Device taken out of case...   |
|--------------------------------------| --\     |--------------------------------------|           |--------------------------------------|      /-- |-------------------------------------|
| 2) Join Topology                     | --/(1c) | TwsTopology_Start                    |           | TwsTopology_Start                    | (5c) \-- | 6) Join Topology                    |
|--------------------------------------|         |--------------------------------------|           |--------------------------------------|          |-------------------------------------|
| 3) Obtain role                       |         |                                      |           |                                      |          | 7) Obtain role                      |
|--------------------------------------|         |--------------------------------------|           |--------------------------------------|          |-------------------------------------|
| 4) Initiate/allow Handset Conections |     (2) | TwsTopology_Join                     |           | TwsTopology_Join                     | (6)      | 8) Initiate/allow Handset Conections|
|--------------------------------------|         |--------------------------------------|           |--------------------------------------|          |-------------------------------------|
                                                                    | |                                           | |
                                                                    | |                  /-------\                | |
                                                                    | |                  \-------/                | |
                                                                    \ /                                           \ /
                                                    (3) Device Is Assigned Primary Role               Device Is Assigned Secondary Role (7)
                                        ||                                                   | |
                                        ||                                                   | |
                                       ---------                                             | |
                                      |  -----  |                                            | |
                                      | |     | | /--------\   -----------                   \ /                 -----------
                                      | |     | | \--------/  |  PRIMARY  O------------------------------------O SECONDARY  |
                                      | |     | |              -----------                                       -----------
                                      |  -----  |
                                      |         |          TwsTopology_Leave(auto)                                                          ||
                                       ---------                  | |                                                                       ||
                                                                  | |                                                                       --------
                                                                  | |                                                                      |  -----  |
                                                                  | |                                                                      | |     | |
                                                                  \ /                                                                      | |     | |
                                                              -----------                                       -----------  /-----------\ | |     | |
                                                             |   IDLE    X                             <-------O  PRIMARY  | \-----------/ |  -----  |
                                                              -----------                                       -----------                |         |
                                                                                                                                            ---------

                                                           TwsTopology_Join                                                                 ||
                                                                  | |                                                                       ||
                                                                  | |                                                                       --------
                                                                  | |                                                                      |  -----  |
                                                                  | |                                                                      | |     | |
                                                                  \ /                                                                      | |     | |
                                                             -----------                                        -----------  /-----------\ | |     | |
                                                            | SECONDARY  O------------------------------------O  PRIMARY   | \-----------/ |  -----  |
                                                             -----------                                        -----------                |         |
                                                                                                                                            ---------

                                                                                                              TwsTopology_Leave
                                        ||                                                                         | |
                                        ||                                                                         | |
                                       ---------                                                                   | |
                                      |  -----  |                                                                  \ /
                                      | |     | | /--------\   -----------                                     -----------
                                      | |     | | \--------/  |  PRIMARY  O-------->                           X  IDLE    |
                                      | |     | |              -----------                                     -----------
                                      |  -----  |
                                      |         |
                                       ---------
                                                             TwsTopology_Leave
                                        ||                         | |
                                        ||                         | |
                                       ---------                   | |
                                      |  -----  |                  \ /
                                      | |     | | /--------\   -----------                                     -----------
                                      | |     | | \--------/  |  PRIMARY X                                     X  IDLE    |
                                      | |     | |              -----------                                     -----------
                                      |  -----  |
                                      |         |
                                       ---------
                                                                                                            TwsTopology_Join
                                       ||                                                                            | |
                                       ||                                                                            | |
                                      ---------                                                                      | |
                                     |  -----  |                                                                     \ /
                                     | |     | | /--------\   -----------                                        -----------
                                     | |     | | \--------/  |  PRIMARY X                               <-------O PRIMARY  |
                                     | |     | |              -----------                                        -----------
                                     |  -----  |
                                     |         |
                                      ---------
                                                             TwsTopology_Join
                                       ||                          | |
                                       ||                          | |
                                      ---------                    | |
                                     |  -----  |                   \ /
                                     | |     | | /--------\   -----------                                        -----------
                                     | |     | | \--------/  |  PRIMARY  O-------------------------------------O SECONDARY |
                                     | |     | |              -----------                                        -----------
                                     |  -----  |
                                     |         |
                                      ---------
                                                                                                               TwsTopology_Leave
                                       ||                                                                            | |
                                       ||                                                                            | |
                                      ---------                                                                      | |
                                     |  -----  |                                                                     \ /
                                     | |     | | /--------\   -----------                                        -----------
                                     | |     | | \--------/  |  PRIMARY  O-------->                              X  IDLE    |
                                     | |     | |              -----------                                        -----------
                                     |  -----  |
                                     |         |
                                      ---------
                                                           TwsTopology_Leave
                                       ||                         | |
                                       ||                         | |
                                      ---------                   | |
                                     |  -----  |                  \ /
                                     | |     | |              -----------                                         -----------
                                     | |     | |             |   IDLE   X                                         X  IDLE    |
                                     | |     | |              -----------                                         -----------
                                     |  -----  |
                                     |         |
                                      ---------
```

# 6. API
---
### API is documented below
```code
 */

/*!@{*/
/*! Definition of messages that TWS Topology can send to clients. */
typedef enum
{
    /*! Confirmation that the TWS Topology module has initialised, sent
        once #TwsTopology_Init() has completed */
    TWS_TOPOLOGY_INIT_COMPLETED =  TWS_TOPOLOGY_MESSAGE_BASE,

    /*! Confirmation that TWS Topology has started, sent in response
        to #TwsTopology_Start(). Clients shall be passed a message of
        #TWS_TOPOLOGY_START_COMPLETED for further information */
    TWS_TOPOLOGY_START_COMPLETED,

    /*! Confirmation that TWS Topology has stopped, sent in response
        to #TwsTopology_Stop(). Clients shall be passed a message of
        #TWS_TOPOLOGY_STOP_COMPLETED for further information */
    TWS_TOPOLOGY_STOP_COMPLETED,
    
    /*! Indication to clients that topology has moved from the started state to the 
     * initial idle state.
     * Note: This message is only sent once upon device initialisation */
    TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED,

    /*! Indication to clients that the Earbud role has changed.
        Clients shall be passed a message of type #TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED for
        further information*/
    TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED,

    /*! Indication to clients that a #TwsTopology_Join() command has been completed. */
    TWS_TOPOLOGY_REQUEST_JOIN_COMPLETED,

    /*! Indication to clients that a #TwsTopology_Leave() command has been completed. */
    TWS_TOPOLOGY_REQUEST_LEAVE_COMPLETED,

    /*! This must be the final message */
    TWS_TOPOLOGY_MESSAGE_END
} tws_topology_message_t;

/*! Definition of status code returned by TWS Topology. */
typedef enum
{
    /*! The operation has been successful */
    tws_topology_status_success,

    /*! The requested operation has failed. */
    tws_topology_status_fail,
} tws_topology_status_t;

/*! Definition of the Earbud roles in a TWS Topology. */
typedef enum
{
    /*! Role is not yet known. */
    tws_topology_role_none,

    /*! Earbud has the Primary role. */
    tws_topology_role_primary,

    /*! Earbud has the Secondary role. */
    tws_topology_role_secondary,

} tws_topology_role;

/*! Definition of the #TWS_TOPOLOGY_START_COMPLETED message. */
typedef struct 
{
    /*! Result of the #TwsTopology_Start() operation. */
    tws_topology_status_t       status;
} TWS_TOPOLOGY_START_COMPLETED_T;

/*! Definition of the #TWS_TOPOLOGY_STOP_COMPLETED message. */
typedef struct 
{
    /*! Result of the #TwsTopology_Stop() operation. 
        If this is not tws_topology_status_success then the topology was not 
        stopped cleanly within the time requested */
    tws_topology_status_t       status;
} TWS_TOPOLOGY_STOP_COMPLETED_T;

/*! Indication of a change in the Earbud role. */
typedef struct
{
    /*! New Earbud role. */
    tws_topology_role           role;
    
    /*! Internal error forced Topology to assign tws_topology_role_none */
    bool error_forced_no_role;
} TWS_TOPOLOGY_ROLE_CHANGE_COMPLETED_T;

/*! Used to inform topology of device type (see TwsTopology_ConfigureProductBehaviour) */
typedef enum {
    topology_device_type_invalid,
    
    /*! Not fully supported. In current implementation EB's currently need to be first peer-paired to
     * obtain single_device behaviour */
    // TODO - requires renaming to topology_device_no_peer
    topology_device_type_standalone,
    
    topology_device_type_with_peer,
    
    topology_device_type_max
}topology_device_type_t;

/*! Commands to control how TwsTopology_Leave behaves (see TwsTopology_Leave) */
typedef enum {role_change_auto, role_change_force_reset}role_change_t;

/*! Used to search for peers */
typedef struct
{
    /*! List of bt addresses that topology should use to find peer */
    bdaddr (*bdaddr_list)[];

    /*! Number of potential peer devices */
    uint8   num_peers;
}peer_search_t;

/*! Product-configurable timeout. Defaults are defined in TWS_TOPOLOGY_DEFAULT_TIMEOUTS */
typedef struct
{
    /*! Timeout for a TWS Topology Stop command to complete (in seconds).
        Note: This should be set such that in a normal case all activities will
            have completed. */
    unsigned send_stop_cmd_sec:4;

    /*! Initial time for a peer find role command before notifying that a role
        has not yet been found. */
    unsigned peer_find_role_sec:4;

    /*! Time for Primary to wait for BR/EDR ACL connection to be made by the Secondary
        following role selection, before falling back to retry role selection. */
    unsigned primary_wait_for_secondary_sec:4;

    /*! Time for Secondary to wait for BR/EDR ACL connection to Primary following
        role selection, before falling back to retry role selection and potentially
        becoming a Standalone primary. */
    unsigned secondary_wait_for_primary_sec:4;

    /*! After exiting the IDLE state (e.g. at boot or from closed-case), this is the timeout for the Primary and Secondary
        attempt to connect to each other and perform a Static Handover, if necessary.
    */
    unsigned post_idle_static_handover_timeout_sec:4;

    /*! Whenever Peer Find Role is skipped and the Preserved Role is assumed from PS, this timeout replaces
        secondary_wait_for_primary_sec. */
    unsigned post_idle_secondary_wait_for_primary_sec:4;

    /*! Whenever primary receives out of case state proxy event from remote, wait for this timeout
     *  before doing the static handover */
    unsigned post_idle_secondary_out_of_case_timeout_sec:4;

    /*! Time duration for which the handover window is active. Handover retries will be allowed during this window period.
        This window period is long because some procedures in the BT stack can in rare situations cause veto for this long.*/
    unsigned max_handover_window_sec:6;

    /*! Time in seconds to delay device reset after becoming idle.
        Note: Idleness is typically associated with going into case (or lid closed when case lid events supported).
        Note: Watchdog must be included in build for this parameter to have any effect
        Note: Set to 0 to disable. */
    unsigned reset_device_sec:4;

    /*! Time for Handover to be retried following a previous handover attempt. */
    uint16 handover_retry_ms;
}topology_timeouts_t;

/*! Callbacks for application specific topology behaviour (see TwsTopology_SetBehaviourConfig) */
typedef struct
{
    /*! Type of device can be:
     * 1) device without peer (standalone)
     * 2) device with peer */
    topology_device_type_t device_type:3;

    /*! For devices with a peer, this parameter determines whether handover is supported
     * Note: This can be overridden at runtime using the API functions
     * TwsTopology_EnableRoleSwapSupport & TwsTopology_DisableRoleSwapSupport */
    bool support_role_swap:1;

    /*! An application may explicitly specify the bt address of the peers they wish to find
        Note: If this member is initiated to zero then a peer will be found automatically
        Note: If specifying a peer address then this data must be persistent 
        Note: Multiple peers currently not supported */
    peer_search_t   peer_search;

    /*! Timeout for various topology events
     *  Note: Default timeouts can be set by initialising structure with TWS_TOPOLOGY_DEFAULT_TIMEOUTS
     *  Note: If no timeouts are set then the device wll panic */
    topology_timeouts_t timeouts;

    /*! Initialises application specific behaviour e.g. registering with clients for notifications */
    void(*init)(void);

    /*! Should revert any application specific initialisation e.g. deregistering from clients */
    void(*deinit)(void);

    /*! Application specific authorisation of a role swap e.g. only allow if connected to a handset */
    bool(*authoriseStartRoleSwap)(hdma_handover_reason_t handover_reason);
}const tws_topology_product_behaviour_t;

/*! \brief Initialise the TWS topology component

    \param init_task    Task to send init completion message (if any) to

    \note  This must be called before any other Topology API

    \returns TRUE
*/
bool TwsTopology_Init(Task init_task);

/*! \brief Configures the product behaviour for topology and MUST be called at least once before 
 *         calling #TwsTopology_Start (but after #TwsTopology_Init). The product behaviour defines 
 *         the conditions under which a device joins/leaves topology, together with the control
 *         of AG connections. Once #TwsTopology_Start has been called this API may be called again
 *         without stopping/restarting Topology.

    \param[in] tws_topology_product_behaviour cpnfiguration for product behaviour. 
               Configuration data must be persistent

    \note This API allows for product behaviour to be changed as required e.g. DFU
          It may be called any number of times and will call the deinit callback (if one is
          already configured) before calling the new init callback. It may be called any time
          that the device is in a stable-role state e.g. no role, primary, secondary. Calling
          this function while the role is transitioning has undefined behaviour.
          Support for identifying stability of role will be added in a future release.
    \note This API will cause a panic if the config parameters are invalid
*/
void TwsTopology_ConfigureProductBehaviour(const tws_topology_product_behaviour_t* tws_topology_product_behaviour);

/*! \brief Register client task to receive TWS topology messages.

    \param[in] client_task Task to receive messages.
*/
void TwsTopology_RegisterMessageClient(Task client_task);

/*! \brief Unregister client task to stop receiving TWS topology messages.

    \param[in] client_task Task to unregister.
*/
void TwsTopology_UnRegisterMessageClient(Task client_task);

/*! \brief Start the TWS topology

    \note TwsTopology_ConfigureProductBehaviour MUST be called before TwsTopology_Start
    \note This API will cause a panic if TwsTopology_ConfigureProductBehaviour has not
          been called prior to calling this API
    \note Application should wait to receive #TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED before
          calling any other topology API. Unless invalid configuation causes a panic,
          this notification is guaranteed to be received.
    
    \todo To allow for the application behaviour to be adapted, error
    conditions are reported to the application. This avoids continual
    retries and may allow applications to try different behaviour.
*/
void TwsTopology_Start(void);

/*! \brief Stop the TWS topology

    The topology will enter a known clean state then send a message to 
    confirm.

    The device should be restarted after the #TWS_TOPOLOGY_STOP_COMPLETED message
    is sent.
*/
void TwsTopology_Stop(void);

/*! \brief Evaluates whether role is Primary (regardless of connection with peer).
    \return TRUE if Earbud is the Primary, otherwise FALSE.
*/
bool TwsTopology_IsRolePrimary(void);

/*! \brief Evaluates whether role is Primary and connected with peer.

    \return TRUE if Earbud is the Primary and connected with peer,
        otherwise FALSE.
*/
bool TwsTopology_IsRolePrimaryConnectedToPeer(void);

/*! \brief Evaluates whether role is primary, but without connection to peer.

    \return  TRUE if Earbud is the standalone Primary otherwise FALSE.
*/
bool TwsTopology_IsRoleStandAlonePrimary(void);

/*! \brief Evaluates whether role is Secondary.
    \return TRUE if Earbud is the Secondary, otherwise FALSE.
*/
bool TwsTopology_IsRoleSecondary(void);

/*! \brief Causes the device to find its role and become connectable to its peer.
 *         If a previous call to Join has been made, but not yet processed, that command
 *         will be cancelled and the new command issued
 * 
 *  \note Topology is concerned only with topology within the context of its peer.
 *        There are no decisions/assumptions made on the state of AG - the application
 *        needs to manage when a device joins the peer topology, together with AG connections.
 *  \note To guarantee there are no race conditions the application should call #TwsTopology_Start
 *        then wait to receive the notification #TWS_TOPOLOGY_INITIAL_IDLE_COMPLETED before calling
 *        this API.
 */
void TwsTopology_Join(void);

/*! \brief Causes the device to leave topology. Application will receive #TWS_TOPOLOGY_REQUEST_LEAVE_COMPLETED 
 *         notification once all leave actions have been performed. If a previous call to Leave has been made, 
 *         but not yet processed, that command will be cancelled and the new command issued.
 *         This function will always results in the cancellation of Peer Find Role, but may not, dependent on
 *         the parameter role_change, relinquish its role.
 * 
 *  \param[in] role_change (see below)
 *      1) if argument role_change is set to role_change_auto will assume a role which is dependent on its
 *         current role, as follows:
 *         a) Primary (connected with peer): Role swap occurs and device is assigned no role.
 *         b) Primary (not connected to peer): Device remains as primary, but no longer accepts peer connections
 *            i.e. it remains in standing primary role, but will not accept peer connections.
 *         c) Secondary: Device is assigned no role
 *      2) if argument role_change is set to role_change_force_reset will always force device to relinquish role:
 *         a) Primary (connected with peer): Role swap occurs and device is assigned no role.
 *         b) Primary (not connected to peer): Device is assigned no role.
 *         c) Secondary: Device is assigned no role
 *      The only difference then is whether a device is allowed to move into a standing primary role after calling
 *      #TwsTopology_Leave. In both cases the device will immediately cancel PFR.
 * 
 *  \note Topology is concerned only with topology within the context of its peer.
 *        There are no decisions/assumptions made on the state of AG - the application
 *        needs to manage when a device joins the peer topology, together with AG connections.
*/
void TwsTopology_Leave(role_change_t role_change);

/*! \brief Request to swap roles i.e. primary-->secondary & secondary-->primary

    \note Topology must have been initially configured to support role swap for this API to attempt
          a role swap. Configure by setting the support_role_swap parameter when configuring product
          behaviour (#TwsTopology_ConfigureProductBehaviour) or explicitly invoking
          #TwsTopology_EnableRoleSwapSupport.
    \note The duration that topology will continue to attempt handover, after a failed handover, is
          configured in the timeout parameter #max_handover_window_sec. This is part of the product
          behaviour configuration defined by calling #TwsTopology_ConfigureProductBehaviour
    \note If swap role is supported and there is a peer connected, topology will call the function
          authoriseStartRoleSwap (registered during #TwsTopology_ConfigureProductBehaviour) to ask
          the application if a handover can be performed. The current default application behaviour
          checks if there is a handset connected as current ADK does not support role swap without
          a handset being connected.
    \note Current implementation does not report failure of handover.
*/
void TwsTopology_SwapRole(void);

/*! \brief Request to swap roles i.e. primary-->secondary & secondary-->primary, then leave topology

    \note Topology must have been initially configured to support roles swap for this API to attempt
          a role swap. Configure by setting the support_role_swap parameter when configuring product
          behaviour (TwsTopology_ConfigureProductBehaviour) or explicitly invoking
          TwsTopology_EnableRoleSwapSupport.
*/
void TwsTopology_SwapRoleAndLeave(void);

/*! \brief Enables support for role swap

    \note This functions only enables support for role swap. TO initiate a role swap then
          TwsTopology_SwapRole or TwsTopology_SwapRoleAndLeave must also be called
*/
void TwsTopology_EnableRoleSwapSupport(void);

/*! \brief Disables support for role swap
*/
void TwsTopology_DisableRoleSwapSupport(void);

/*! \brief Determines the current state of role swap support
 *  \return TRUE if role swap is supported, otherwise FALSE
*/
bool TwsTopology_IsRoleSwapSupported(void);

#ifdef ENABLE_SKIP_PFR
/*! \brief function to send the secondary out of case timeout internal message
    to the primary, so that primary can decide whether it should do static
    handover or not based on it's local phy state.

    \return none.
 */
void TwsTopology_SendSecondaryOutOfCaseMessage(void);

/*! \brief function to cancel the secondary out of case internal message timeout

    \return none.
 */
void TwsTopology_CancelSecondaryOutOfCaseMessage(void);
#endif

/*!@}*/
#endif /* TWS_TOPOLOGY_H_ */
