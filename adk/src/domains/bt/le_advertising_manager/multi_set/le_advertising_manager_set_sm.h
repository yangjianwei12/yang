/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup    le_advertising_manager_multi_set
    \brief      Header file for management of Bluetooth Low Energy advertising sets.
    @{
*/

#ifndef LE_ADVERTISING_MANAGER_SET_SM_H
#define LE_ADVERTISING_MANAGER_SET_SM_H

#include "le_advertising_manager_data_packet.h"

#include "domain_message.h"


/*! \brief Mask used to mark a state as a 'steady' state. */
#define LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK 0x20

/*! \brief State of an advertising set.

@startuml Advertising Set States

NULL -d-> UNREGISTERED : Create a new advertising set state machine

UNREGISTERED --> REGISTERING : INTERNAL_REGISTER

REGISTERING --> IDLE : Advertising set registered ok

IDLE --> CONFIGURING_SET_PARAMS : INTERNAL_UPDATE_PARAMS_REQ
IDLE --> CONFIGURING_SET_DATA : INTERNAL_UPDATE_DATA_REQ
IDLE --> UNREGISTERING : INTERNAL_UNREGISTER

CONFIGURING_SET_PARAMS --> CONFIGURING_SET_ADDRESS : If the address for this advertising set needs to be set separately.
CONFIGURING_SET_PARAMS --> IDLE : Advertising params set ok\nPrevious state was IDLE
CONFIGURING_SET_PARAMS --> ACTIVE : Advertising params set ok\nPrevious state was ACTIVE

CONFIGURING_SET_ADDRESS --> IDLE : Address set ok\nPrevious state was IDLE
CONFIGURING_SET_ADDRESS --> ACTIVE : Address set ok\nPrevious state was ACTIVE

CONFIGURING_SET_DATA --> CONFIGURING_SET_SCAN_RESP_DATA : If this advertising set has scan response data.
CONFIGURING_SET_DATA --> IDLE : Advertising data set ok\nPrevious state was IDLE
CONFIGURING_SET_DATA --> ACTIVE : Advertising data set ok\nPrevious state was ACTIVE

CONFIGURING_SET_SCAN_RESP_DATA --> IDLE : Scan response data set ok\nPrevious state was IDLE
CONFIGURING_SET_SCAN_RESP_DATA --> ACTIVE : Scan response data set ok\nPrevious state was ACTIVE

IDLE --> ENABLING : INTERNAL_ENABLE

ENABLING --> ACTIVE : Advertising set enabled ok

ACTIVE --> SUSPENDING : INTERNAL_DISABLE
ACTIVE --> CONFIGURING_SET_PARAMS : INTERNAL_UPDATE_PARAMS_REQ
ACTIVE --> CONFIGURING_SET_DATA : INTERNAL_UPDATE_DATA_REQ

ACTIVE --> UNREGISTERING : INTERNAL_UNREGISTER Note: is it ok to unregister a set while it is active?

SUSPENDING --> IDLE : Advertising set disabled ok

UNREGISTERING --> UNREGISTERED : Advertising set unregistered ok

UNREGISTERED --> NULL : Destroy state machine

@enduml
*/
typedef enum {
    LE_ADVERTISING_MANAGER_SET_STATE_NULL = 0,
    LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERED = 1 + LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK,
    LE_ADVERTISING_MANAGER_SET_STATE_REGISTERING = 2,
    LE_ADVERTISING_MANAGER_SET_STATE_IDLE = 3 + LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK,
    LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_PARAMS = 4,
    LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_ADDRESS = 5,
    LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_DATA = 6,
    LE_ADVERTISING_MANAGER_SET_STATE_CONFIGURING_SET_SCAN_RESP_DATA = 7,
    LE_ADVERTISING_MANAGER_SET_STATE_ENABLING = 8,
    LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE = 9 + LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK,
    LE_ADVERTISING_MANAGER_SET_STATE_SUSPENDING = 10,
    LE_ADVERTISING_MANAGER_SET_STATE_UNREGISTERING = 11
} le_advertising_manager_set_state_t;

#define LeAdvertisingManager_SetSmIsSteadyState(state) \
    (((state) & LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK) == LE_ADVERTISING_MANAGER_SET_STEADY_STATE_MASK)


/*! \brief Advertising parameters for an advertising set. */
typedef struct {
    uint16 adv_event_properties;
    uint32 primary_adv_interval_min;
    uint32 primary_adv_interval_max;
    uint8 primary_adv_channel_map;
    uint8 own_addr_type;
    typed_bdaddr peer_tpaddr;
    uint8 adv_filter_policy;
    uint16 primary_adv_phy;
    uint8 secondary_adv_max_skip;
    uint16 secondary_adv_phy;
    uint16 adv_sid;
    ble_local_addr_type random_addr_type;
    typed_bdaddr random_addr;
    uint16 random_addr_generate_rotation_timeout_minimum_in_minutes;
    uint16 random_addr_generate_rotation_timeout_maximum_in_minutes;

} le_advertising_manager_set_params_t;

/*! \brief Advertising data for an advertising set.

    Contains buffers for both the advertising data and
    scan response data.
*/
typedef struct {
    le_advertising_manager_data_packet_t adv_data;
    le_advertising_manager_data_packet_t scan_resp_data;
} le_advertising_manager_set_adv_data_t;

/*! \brief Task data for a single advertising set. */
typedef struct {
    /*! Task that is registered with the profiles for this advertising set handle. */
    TaskData task_data;

    /*! Client interface to send notifications to. */
    struct le_advertising_manager_set_client_interface_t_ *client_if;

    /*! Advertising set handle for this set. */
    uint8 adv_handle;

    /*! Current state */
    le_advertising_manager_set_state_t state;

    /*! Target state */
    le_advertising_manager_set_state_t target_state;

    /*! Retries left for setting the extended advert address. */
    uint8 extended_advert_rpa_retries;

    /*! Conditional lock for internal messages. */
    uint16 internal_msg_lock;

    /*! Parameters for this advertising set */
    le_advertising_manager_set_params_t params;

    /*! Buffers containing the advert and scan response data for this advertising set. */
    le_advertising_manager_set_adv_data_t data;

    /*! Flag to set and reset during handling of CSR_BT_CM_EXT_ADV_TERMINATED_IND internally */
    bool adv_terminate_ind_processing;

} le_advertising_manager_set_state_machine_t;

#define LeAdvertisingManager_SetSmGetTask(sm) (&(sm)->task_data)

#define LeAdvertisingManager_SetSmGetClientInterface(sm) ((sm)->client_if)

#define LeAdvertisingManager_SetSmGetAdvHandle(sm) ((sm)->adv_handle)

#define LeAdvertisingManager_SetSmGetState(sm) ((sm)->state)

#define LeAdvertisingManager_SetSmGetParams(sm) (&(sm)->params)

#define LeAdvertisingManager_SetSmGetAdvertDataPacket(sm) (&(sm)->data.adv_data)

#define LeAdvertisingManager_SetSmGetScanResponseDataPacket(sm) (&(sm)->data.scan_resp_data)


/*
    Public API
*/

/*! \brief Callback interface implemented by the client of a set to receive notifications from the set. */
typedef struct le_advertising_manager_set_client_interface_t_ {
    /* Result of registering the advertising set with the BT stack. */
    void (*RegisterCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of setting the advertising set params with the BT stack. */
    void (*UpdateParamsCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of setting the advertising set data with the BT stack. */
    void (*UpdateDataCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of enabling the advertising set with the BT stack. */
    void (*EnableCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of disabling the advertising set with the BT stack. */
    void (*DisableCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of unregistering the advertising set with the BT stack. */
    void (*UnregisterCfm)(le_advertising_manager_set_state_machine_t *sm, bool success);

    /* Result of random address rotatation timeout. */
    void (*RandomAddressRotateTimeout)(le_advertising_manager_set_state_machine_t *sm);

    /* Indicating changes in the set's configured random address, if applicable. */
    void (*RandomAddressChangedInd)(le_advertising_manager_set_state_machine_t *sm, bdaddr new_bdaddr);
} le_advertising_manager_set_client_interface_t;


/*! \brief Initalise this module. */
void LeAdvertisingManager_SetSmInit(void);

/*! \brief Create a new advertising set state machine

    \param client_if Callback interface to the client for notifications from the advertising set.
    \param handle Advertising handle to use for the set.
*/
le_advertising_manager_set_state_machine_t *LeAdvertisingManager_SetSmCreate(le_advertising_manager_set_client_interface_t *client_if, uint8 adv_handle);

/*! \brief Destroy an existing advertising set state machine

    The state machine must be in the unregistered state before it can be destroyed.
    If this is called when in the wrong state it will panic.

    \param sm Advertising Set state machine to destroy.
*/
void LeAdvertisingManager_SetSmDestroy(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Find an advertising set state machine by its adv_handle

    \param adv_handle Handle to match to find the state machine.

    \return A pointer to a advertising set state machine is a match was found;
            NULL otherwise.
*/
le_advertising_manager_set_state_machine_t *LeAdvertisingManager_SetSmGetByAdvHandle(uint8 adv_handle);

/*! \brief Change the state of a LE Advertising Set state machine. */
void LeAdvertisingManager_SetSmSetState(le_advertising_manager_set_state_machine_t *sm, le_advertising_manager_set_state_t state);

/*! \brief Set the target end state for an advertising state machine.

    Note: This should only be used internally within the advertising set state
          machine.

    \param sm State machine to set target state for.
    \param target_state The target state. This should only ever be 'idle'
                        (LE_ADVERTISING_MANAGER_SET_STATE_IDLE) or 'active'
                        (LE_ADVERTISING_MANAGER_SET_STATE_ACTIVE).
*/
void LeAdvertisingManager_SetSmSetTargetState(le_advertising_manager_set_state_machine_t *sm, le_advertising_manager_set_state_t target_state);

/*! \brief Register this advertising set with the BT stack.

    The advertising set must first be registered with tbe BT stack before it
    can be configured and made active.

    This is an asynchronous operation. The result will be returned in the
    RegisterCfm callback.

    \param sm Advertising set to registerk.
*/
void LeAdvertisingManager_SetSmRegister(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Unregister this advertising set with the BT stack.

    The advertising set must be unregistered with the BT stack before it
    can be destroyed. The advertising set can only be unregistered if
    it is not currently active.

    This is an asynchronous operation. The result will be returned in the
    UnregisterCfm callback.

    \param sm Advertising set to unregister.
*/
void LeAdvertisingManager_SetSmUnregister(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Update the parameters used by an advertising set.

    This is an asynchronous operation. The result will be returned in the
    UpdateParamsCfm callback.

    The parameters can be changed when the advertising set is active or idle.

    \param sm Advertising Set to update parameters for.
    \param params Parameters to use for this advertising set.
*/
void LeAdvertisingManager_SetSmUpdateParams(le_advertising_manager_set_state_machine_t *sm, const le_advertising_manager_set_params_t *params);

/*! \brief Update the data used by an advertising set

    This is an asynchronous operation. The result will be returned in the
    UpdateDataCfm callback.

    The data can be changed when the advertising set is active or idle.

    \param sm Advertising Set to update data for.
    \param adv_data The advert data and (optional) scan response data buffers.
*/
void LeAdvertisingManager_SetSmUpdateData(le_advertising_manager_set_state_machine_t *sm, const le_advertising_manager_set_adv_data_t* adv_data);

/*! \brief Enable an advertising set.

    The advertising set must first have been configured with calls
    to #LeAdvertisingManager_SetSmUpdateParams and
    #LeAdvertisingManager_SetSmUpdateData, otherwise enabling may fail.

    Once an advedrtising set has been configured it can enabled and disabled
    without having to be reconfigured each time.

    This is an asynchronous operation. The result will be returned in the
    EnableCfm callback.

    \param sm Advertising Set to enable.
*/
void LeAdvertisingManager_SetSmEnable(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Disable an advertising set.

    This is an asynchronous operation. The result will be returned in the
    DisableCfm callback.

    \param sm Advertising Set to disable.
*/
void LeAdvertisingManager_SetSmDisable(le_advertising_manager_set_state_machine_t *sm);


/*! \brief Is this advertising set currently active.

    \param sm Advertising Set to check.
    \return TRUE if active, FALSE otherwise.
*/
bool LeAdvertisingManager_SetSmIsActive(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Is this advertising set currently starting.

    \param sm Advertising Set to check.
    \return TRUE if active, FALSE otherwise.
*/
bool LeAdvertisingManager_SetSmIsStarting(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Is this advertising set currently inactive.

    \param sm Advertising Set to check.
    \return TRUE if active, FALSE otherwise.
*/
bool LeAdvertisingManager_SetSmIsInactive(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Is this advertising set currently suspending.

    \param sm Advertising Set to check.
    \return TRUE if active, FALSE otherwise.
*/
bool LeAdvertisingManager_SetSmIsSuspending(le_advertising_manager_set_state_machine_t *sm);

/*! \brief Get number of supported advertising sets.

    \return Number of supported advertising sets.
*/
uint8 LeAdvertisingManager_SetSmGetNumberOfSupportedSets(void);

/*! \brief Get the next unused adv_handle.

    Gets the next unused adv_handle that is not currently in use by another
    advertising set.

    \return An unused adv_handle.
*/
uint8 LeAdvertisingManager_SetSmGetNextUnusedAdvHandle(void);

/*! \brief Enable/disable random address rotation timer if the advertising set uses random address and there is a valid timer configuration for the advertising set.

    \param sm Advertising Set to check.
    \param enable TRUE to enable, FALSE to disable.

*/
void leAdvertisingManager_SetSmEnableAddressRotateTimer(le_advertising_manager_set_state_machine_t * sm, bool enable);

#endif /* LE_ADVERTISING_MANAGER_SET_SM_H */

/*! @} */