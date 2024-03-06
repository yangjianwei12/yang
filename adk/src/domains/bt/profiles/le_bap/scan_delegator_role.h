/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_bap
    \brief
    @{
*/

#ifndef SCAN_DELEGATOR_ROLE_H_
#define SCAN_DELEGATOR_ROLE_H_


#include <gatt_bass_server.h>

#include <bdaddr.h>

#include "bt_types.h"
#include "le_broadcast_manager_types.h"

#define SCAN_DELEGATOR_CLIENT_BIS_SYNC_NO_SYNC_TO_PA(bis_sync, bis_index) (bis_sync & ~(1<<bis_index))
#define SCAN_DELEGATOR_CLIENT_BIS_SYNC_SYNC_TO_PA(bis_sync, bis_index) (bis_sync | (1<<bis_index))
#define SCAN_DELEGATOR_CLIENT_BIS_SYNC_NO_PREFERENCE            0xFFFFFFFFUL
#define SCAN_DELEGATOR_SERVER_BIS_SYNC_STATE_NO_SYNC_TO_PA(bis_sync, bis_index) SCAN_DELEGATOR_CLIENT_BIS_SYNC_NO_SYNC_TO_PA(bis_sync, bis_index)
#define SCAN_DELEGATOR_SERVER_BIS_SYNC_STATE_SYNC_TO_PA(bis_sync, bis_index) SCAN_DELEGATOR_CLIENT_BIS_SYNC_SYNC_TO_PA(bis_sync, bis_index)
#define SCAN_DELEGATOR_SERVER_BIS_SYNC_STATE_FAILED_SYNC_BIG    0xFFFFFFFFUL

#define SCAN_DELEGATOR_BROADCAST_CODE_SIZE  GATT_BASS_SERVER_BROADCAST_CODE_SIZE
#define SCAN_DELEGATOR_SOURCE_ID_INVALID    0


typedef enum
{
    scan_delegator_server_pa_sync_state_no_sync_to_pa = GATT_BASS_SERVER_NOT_SYNCHRONIZED,
    scan_delegator_server_pa_sync_state_syncinfo_request = GATT_BASS_SERVER_SYNC_INFO_REQUEST,
    scan_delegator_server_pa_sync_state_sync_to_pa = GATT_BASS_SERVER_SYNCHRONIZED,
    scan_delegator_server_pa_sync_state_fail_sync_to_pa = GATT_BASS_SERVER_FAILED_TO_SYNCHRONIZE,
    scan_delegator_server_pa_sync_state_no_past = GATT_BASS_SERVER_NO_PAST
} scan_delegator_server_pa_sync_state_t;

typedef enum
{
    scan_delegator_server_big_encryption_not_encrypted = GATT_BASS_SERVER_NOT_ENCRYPTED,
    scan_delegator_server_big_encryption_broadcast_code_needed = GATT_BASS_SERVER_BROADCAST_CODE_REQUIRED,
    scan_delegator_server_big_encryption_decrypting = GATT_BASS_SERVER_DECRYPTING,
    scan_delegator_server_big_encryption_bad_code = GATT_BASS_BAD_CODE
} scan_delegator_server_big_encryption_t;

typedef enum
{
    scan_delegator_status_success = BAP_SERVER_STATUS_SUCCESS,
    scan_delegator_status_in_progress = BAP_SERVER_STATUS_IN_PROGRESS,
    scan_delegator_status_invalid_parameter = BAP_SERVER_STATUS_INVALID_PARAMETER,
    scan_delegator_status_not_allowed = BAP_SERVER_STATUS_NOT_ALLOWED,
    scan_delegator_status_failed = BAP_SERVER_STATUS_FAILED,
    scan_delegator_status_bc_source_in_sync = BAP_SERVER_STATUS_BC_SOURCE_IN_SYNC,
    scan_delegator_status_invalid_source_id = BAP_SERVER_STATUS_INVALID_SOURCE_ID,
    scan_delegator_status_no_empty_brs = BAP_SERVER_STATUS_NO_EMPTY_BRS,
    scan_delegator_status_brs_not_changed = BAP_SERVER_STATUS_BRS_NOT_CHANGED,
} scan_delegator_status_t;

typedef struct
{
    le_bm_pa_sync_t pa_sync;
    typed_bdaddr advertiser_address;
    uint32 broadcast_id;
    uint8 source_adv_sid;
    uint16 pa_interval;
    typed_bdaddr assistant_address;
    uint8 num_subgroups;
    le_bm_source_subgroup_t *subgroups;
} scan_delegator_client_add_broadcast_source_t;

typedef struct
{
    scan_delegator_server_pa_sync_state_t pa_sync_state;
    scan_delegator_server_big_encryption_t big_encryption;
    typed_bdaddr source_address;
    uint32 broadcast_id;
    uint8 source_adv_sid;
    uint8 *bad_code;
    uint8 num_subgroups;
    le_bm_source_subgroup_t *subgroups;
} scan_delegator_server_get_broadcast_source_state_t;

typedef scan_delegator_server_get_broadcast_source_state_t scan_delegator_server_add_broadcast_source_state_t;

typedef struct
{
    uint8 source_id;
    le_bm_pa_sync_t pa_sync;
    uint16 pa_interval;
    typed_bdaddr assistant_address;
    uint8 num_subgroups;
    le_bm_source_subgroup_t *subgroups;
} scan_delegator_client_modify_broadcast_source_t;

typedef struct
{
    scan_delegator_server_pa_sync_state_t pa_sync_state;
    scan_delegator_server_big_encryption_t big_encryption;
    uint8 *bad_code;
    uint8 num_subgroups;
    le_bm_source_subgroup_t *subgroups;
} scan_delegator_server_modify_broadcast_source_state_t;

typedef struct
{
    uint8 source_id;
    const uint8 *broadcast_code;
} scan_delegator_client_broadcast_code_t;

typedef struct
{
    uint8 source_id;
} scan_delegator_client_remove_broadcast_source_t;

typedef struct
{
    scan_delegator_status_t status;
    uint16 sync_handle;
    typed_bdaddr taddr_source;
    uint8 source_adv_sid;
    uint16 service_data;
} scan_delegator_periodic_sync_t;

typedef struct
{
    uint8  receive_state_ccc_size;
    uint16 receive_state_ccc[];
} scan_delegator_config_t;

typedef struct
{
    le_bm_pa_sync_t pa_sync;
    uint8 num_subgroups;
    const uint32 *bis_sync;
    typed_bdaddr assistant_address;
} scan_delegator_target_sync_state_t;


/*! \brief Callback interface

    Implemented and registered by the application to receive and influence BASS behaviour
 */
typedef struct
{
    /*! \brief Called when the remote device has started scanning
     */
    void (*LeBapScanDelegator_RemoteScanningStart)(void);
    /*! \brief Called when the remote device has stopped scanning
     */
    void (*LeBapScanDelegator_RemoteScanningStop)(void);
    /*! \brief Called when the client has added a Source
     */
    void (*LeBapScanDelegator_AddSource)(scan_delegator_client_add_broadcast_source_t * source);
    /*! \brief Called when the client has modified the Source Sync information
     */
    void (*LeBapScanDelegator_ModifySource)(scan_delegator_client_modify_broadcast_source_t * source);
    /*! \brief Called when the remote device sends the broadcast code
     */
    void (*LeBapScanDelegator_BroadcastCode)(scan_delegator_client_broadcast_code_t * code);
    /*! \brief Called when the client has removed a Source
     */
    void (*LeBapScanDelegator_RemoveSource)(scan_delegator_client_remove_broadcast_source_t * source);
    /*! \brief Called when periodic sync transfer information has been received 
     */
    void (*LeBapScanDelegator_PeriodicSync)(scan_delegator_periodic_sync_t * sync);
    /*! \brief Called to retrieve a BASS client config
    */
    void * (*LeBapScanDelegator_RetrieveClientConfig)(gatt_cid_t cid);
    /*! \brief Called to store an BASS client config
    */
    void (*LeBapScanDelegator_StoreClientConfig)(gatt_cid_t cid, void * config, uint8 size);
    /*! \brief Called to get the target PA/BIS sync state (can be different from the state stored in the characteristics)
    */
    scan_delegator_target_sync_state_t (*LeBapScanDelegator_GetTargetSyncState)(uint8 source_id);
} LeBapScanDelegator_callback_interface_t;

/*! \brief Initialises the BAP Scan Delegator role

    Initialises the BASS and adds LE advertising data.

    \param number_broadcast_sources The number of Broadcast Sources that should be handled.
    \param callbacks_to_register LeBapScanDelegator_callback_interface_t to register
 */
void LeBapScanDelegator_Init(uint8 number_broadcast_sources, const LeBapScanDelegator_callback_interface_t * callbacks_to_register);

/*! \brief Adds a Broadcast Source Receive state on the server

    \param source_id The Source_ID of the Broadcast Source Receive state. Zero can be supplied to add to a free Broadcast Source Receive state, in which case it will be set to the allocated Source ID if added successfully.
    \param source_state The Broadcast Source Receive state to set.

    \return Result of the operation, #scan_delegator_status_t.
 */
scan_delegator_status_t LeBapScanDelegator_AddBroadcastSourceState(uint8 *source_id, const scan_delegator_server_add_broadcast_source_state_t * source_state);

/*! \brief Modifies a Broadcast Source Receive state on the server

    \param source_id The Source_ID of the Broadcast Source Receive state.
    \param source_state The Broadcast Source Receive state to set.
    
    \return Result of the operation, #scan_delegator_status_t.
 */
scan_delegator_status_t LeBapScanDelegator_ModifyBroadcastSourceState(uint8 source_id, const scan_delegator_server_modify_broadcast_source_state_t * source_state);

/*! \brief Removes a Broadcast Source Receive state on the server

    \param source_id The Source_ID of the Broadcast Source Receive state.
    
    \return Result of the operation, #scan_delegator_status_t.
 */
scan_delegator_status_t LeBapScanDelegator_RemoveBroadcastSourceState(uint8 source_id);

/*! \brief Gets the Broadcast Source Receive state on the server

    \param source_id The Source_ID of the Broadcast Source Receive state.
    \param source_state The Broadcast Source Receive state to get.
    
    \return Result of the operation, #scan_delegator_status_t.
 */
scan_delegator_status_t LeBapScanDelegator_GetBroadcastSourceState(uint8 source_id, scan_delegator_server_get_broadcast_source_state_t * source_state);

/*! \brief Frees all the memory associated with the Broadcast Source Receive state that was allocated using the LeBapScanDelegator_GetBroadcastSourceState API.

    \param source_state The Broadcast Source Receive state to free.
 */
void LeBapScanDelegator_FreeBroadcastSourceState(scan_delegator_server_get_broadcast_source_state_t * source_state);

/*! \brief Frees the subgroups memory associated with the Broadcast Source Receive state that was allocated using the LeBapScanDelegator_GetBroadcastSourceState API.

    \param subgroups_data The Broadcast Source Receive state subgroups data to free.
 */
void LeBapScanDelegator_FreeBroadcastSourceSubgroupsData(uint8 num_subgroups, le_bm_source_subgroup_t *subgroups_data);

/*! \brief Gets the Broadcast Code provided by the client, to enable the server to decrypt an encrypted BIS.

    If the Broadcast Code has been provided by the client this API can be used to read it.
    If the Broadcast Code has not been provided, this API will return NULL. In this case the Broadcast Receive State 
    should be set appropriately to notify the client that the Broadcast Code is required in order to decrypt an encrypted BIS.

    \param source_id The Source_ID of the Broadcast Source Receive state.
    
    \return The Broacast Code of length SCAN_DELEGATOR_BROADCAST_CODE_SIZE if it has been provided by the client. NULL if the Broadcast Code is not known.
 */
uint8 * LeBapScanDelegator_GetBroadcastCode(uint8 source_id);

/*! \brief Sets the Broadcast Code provided by the client, to enable the server to decrypt an encrypted BIS.

    If the Broadcast Code has been provided by the client this API can be used to read it.
    If the Broadcast Code has not been provided, this API will return NULL. In this case the Broadcast Receive State 
    should be set appropriately to notify the client that the Broadcast Code is required in order to decrypt an encrypted BIS.

    \param source_id The Source_ID of the Broadcast Source Receive state.
    
    \return The Broacast Code of length SCAN_DELEGATOR_BROADCAST_CODE_SIZE if it has been provided by the client. NULL if the Broadcast Code is not known.
 */
scan_delegator_status_t LeBapScanDelegator_SetBroadcastCode(uint8 source_id, uint8 * broadcast_code);

/*! \brief Gets the Source_IDs of all the Broadcast Source Receive State that can be stored.

    This API can be used when adding a new Broadcast Source. It will return all the Source_ID that can be used to store
    Broadcast Receive State.
    The caller can then decide if the Source_ID is currently in use and choose a free one, or reuse an existing Source
    if there are are no free Source available, and has decided it no longer is interested in the previous Broadcast Source.

    \param number_source_id The number of Source_ID returned.
    
    \return The list of Source_ID that can be used to store Broadcast Source Receive State. The memory MUST be freed by the caller after use.
 */
uint8 * LeBapScanDelegator_GetBroadcastSourceIds(uint16 * number_source_id);

/*! \brief Handler for all connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the Scan Delegator is interested in. If a message
    is processed then the function returns TRUE.

    \param  id              Identifier of the connection library message
    \param  message         The message content (if any)
    \param  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \returns TRUE if the message has been processed, otherwise FALSE.
*/
bool LeBapScanDelegator_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);

/*! \brief Query if the scan delegator has any GATTclient's connected.
    \return TRUE if any clients connected.
*/
bool LeBapScanDelegator_IsAnyClientConnected(void);

/*! \brief Set the scan delegator callback interface functions.
    \param callbacks_to_register The new callbacks for the scan delegator to use.
    \return The previously registered callback interface.
*/
const LeBapScanDelegator_callback_interface_t *LeBapScanDelegator_RegisterCallbacks(const LeBapScanDelegator_callback_interface_t *callbacks_to_register);

/*! \brief Checks service_data to see if if AdvA in PAST matches ADV_EXT_IND. This information wil be sent by the Broadcast Assistant using the service_data value within the LE Periodic Advertising Sync Transfer command. 
    \param service_data The service_data value within the LE Periodic Advertising Sync Transfer command sent by the Broadcast Assistant.
    \return TRUE if AdvA in PAST matches ADV_EXT_IND. FALSE otherwise.
*/
#define LeBapScanDelegator_DoesServiceDataAdvAMatchAdvExtInd(service_data)      ((service_data & 0x0001)^0x0001)

/*! \brief Checks service_data to see if AdvA in PAST matches Source_Address. This information wil be sent by the Broadcast Assistant using the service_data value within the LE Periodic Advertising Sync Transfer command. 
    \param service_data The service_data value within the LE Periodic Advertising Sync Transfer command sent by the Broadcast Assistant.
    \return TRUE if AdvA in PAST matches Source_Address. FALSE otherwise.
*/
#define LeBapScanDelegator_DoesServiceDataAdvAMatchSourceAddress(service_data)  ((service_data & 0x0002)^0x0002)

/*! \brief Gets the PAST Source ID from the service_data. This information wil be sent by the Broadcast Assistant using the service_data value within the LE Periodic Advertising Sync Transfer command. 
    \param service_data The service_data value within the LE Periodic Advertising Sync Transfer command sent by the Broadcast Assistant.
    \return The Source ID within the service_data sent by the Broadcast Assistant.
*/
#define LeBapScanDelegator_GetServiceDataSourceId(service_data)                 ((service_data >> 8)&0xff)

/*! \brief Get the BAP profile instance handle.
    \return the bap profile handle.
*/
ServiceHandle leBapScanDelegator_GetBapProfileHandle(void);

/*! \brief Enable or disable the PAST procedure in the controller for a given address.

    \param taddr Address of the device to configure PAST for.
    \param enable TRUE to enable PAST; FALSE otherwise.
*/
void LeBapScanDelegator_ConfigurePastForAddr(typed_bdaddr *taddr, bool enable);

#endif /* SCAN_DELEGATOR_ROLE_H_ */
/*! @} */