/*!
    \copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version     
    \file
    \addtogroup bredr_scan_manager
    \brief      Private interface to module managing inquiry and page scanning.
    @{
*/

#ifndef BREDR_SCAN_MANAGER_PRIVATE_H_
#define BREDR_SCAN_MANAGER_PRIVATE_H_

#include "bredr_scan_manager.h"
#include "bredr_scan_manager_protected.h"
#include "task_list.h"
#include "connection_abstraction.h"
#include "logging.h"
#include "bandwidth_manager.h"

#include <message.h>
#include <panic.h>
#include <hydra_macros.h>

/*! \brief Scan states, one-hot encoded */
typedef enum bsm_scan_states
{
    /*! Scanning is disabled */
    BSM_SCAN_DISABLED   = 0x01,
    /*! Scanning is being enabled */
    BSM_SCAN_ENABLING   = 0x02,
    /*! Scanning is being disabled */
    BSM_SCAN_DISABLING  = 0x04,
    /*! Scanning is enabled */
    BSM_SCAN_ENABLED    = 0x08,
} bsm_scan_enable_state_t;

/* helper macro to set normal/truncated state without affecting the rest of the bits */
#define SET_BSM_STATE(state_var, state) (state_var) = (state)
/* helper macro to test whether a particular state bit is set */
#define TEST_BSM_STATE(state_var, state) ((state_var) == (state))

/*! \brief Scan context, for page and inquiry scanning */
typedef struct
{
    /*! Configured operating parameter set */
    const bredr_scan_manager_parameters_t *params;

    /*! List of clients */
    task_list_with_data_t clients;

    /*! Currently set scan parameters. */
    bredr_scan_manager_scan_parameters_t *curr_scan_params;

    /*! The current scan state */
    bsm_scan_enable_state_t state;

    /*! Active index in array of params */
    uint8 params_index;

    /*! Highest priority scanning type supported */
    bredr_scan_manager_scan_type_t type;

    /*! Flag to indicate that the instance is paused */
    bool paused;

    /*! Flag to indicate that the instance is throttled */
    bool throttled;

    /*! Flag to indicate that the instance needs a clean up */
    bool needs_cleanup;

} bsm_scan_context_t;

/*! @brief Scan Manager state. */
typedef struct
{
    /*! Module's task */
    TaskData task_data;

    /*! Page scan context */
    bsm_scan_context_t page_scan;

    /*! Truncated page scan context */
    bsm_scan_context_t truncated_page_scan;

    /*! Inquiry scan context */
    bsm_scan_context_t inquiry_scan;

    /*! Current PS / TPS parameters */
    bredr_scan_manager_scan_parameters_t curr_ps_params;

    /*! Current IS parameters */
    bredr_scan_manager_scan_parameters_t curr_is_params;

    /*! The task that requested the general disable (if any). Cleared once re-enabled */
    Task disable_task;

    /*! Flag to indicate that throttle scan has been requested */
    bool thorottle_scan_requested;

    /*! Pointer to callback function to call when eir setup is complete */
    eir_setup_complete_callback_t eir_setup_complete_callback;

    /*! Indicates EIR setup is in progress */
    bool eir_setup_in_progress;
} bredr_scan_manager_state_t;

extern bredr_scan_manager_state_t bredr_scan_manager;

#define bredrScanManager_GetTask() (&bredr_scan_manager.task_data)

#define bredrScanManager_PausedMsgId(context) ((context) == bredrScanManager_PageScanContext())? BREDR_SCAN_MANAGER_PAGE_SCAN_PAUSED_IND : ((context) == bredrScanManager_InquiryScanContext())? BREDR_SCAN_MANAGER_INQUIRY_SCAN_PAUSED_IND : BREDR_SCAN_MANAGER_TRUNCATED_PAGE_SCAN_PAUSED_IND
#define bredrScanManager_ResumedMsgId(context) ((context) == bredrScanManager_PageScanContext())? BREDR_SCAN_MANAGER_PAGE_SCAN_RESUMED_IND : ((context) == bredrScanManager_InquiryScanContext())? BREDR_SCAN_MANAGER_INQUIRY_SCAN_RESUMED_IND : BREDR_SCAN_MANAGER_TRUNCATED_PAGE_SCAN_RESUMED_IND

/*! \brief Helper function to read the task's scan type from the task list data. */
static inline bredr_scan_manager_scan_type_t bredrScanManager_ListDataGet(const task_list_data_t *data)
{
    return (bredr_scan_manager_scan_type_t)(data->s8);
}

/*! \brief Get pointer to page scan context.
    \return The context.
*/
static inline bsm_scan_context_t *bredrScanManager_PageScanContext(void)
{
    return &bredr_scan_manager.page_scan;
}

/*! \brief Get pointer to truncated page scan context.
    \return The context.
*/
static inline bsm_scan_context_t *bredrScanManager_TruncatedPageScanContext(void)
{
    return &bredr_scan_manager.truncated_page_scan;
}

/*! \brief Get pointer to inquiry scan context.
    \return The context.
*/
static inline bsm_scan_context_t *bredrScanManager_InquiryScanContext(void)
{
    return &bredr_scan_manager.inquiry_scan;
}

/*! \brief Set the disable task.

    \param disabler The new disable task.

    \return The old disable task.
*/
static inline Task bredrScanManager_SetDisableTask(Task disabler)
{
    Task old = bredr_scan_manager.disable_task;
    bredr_scan_manager.disable_task = disabler;
    return old;
}

/*! \brief Get the disable task.

    \return The disable task.
*/
static inline Task bredrScanManager_GetDisableTask(void)
{
    return bredr_scan_manager.disable_task;
}

/*! \brief Determine if scanning is disabled.

    \return TRUE if disabled.
*/
static inline bool bredrScanManager_IsDisabled(void)
{
    return (bredr_scan_manager.disable_task != NULL);
}

static inline bool bredrScanManager_IsThrottleScanRequested(void)
{
    return bredr_scan_manager.thorottle_scan_requested;
}

void bredrScanManager_AdjustPageScanBandwidth(bool throttle_required);

/*! \brief Send throttle indication to clients
 *
 *     \param context Pointer to scan manager context object.
 *
 *     \param throttle_required Bool to indicate throttle scan is required.
*/
void bredrScanManager_InstanceSendScanThrottleInd(bsm_scan_context_t *context, bool throttle_required);

/*! \brief Check whether requested scan type is registered by any client

    \param context Pointer to scan context

    \param type Scan type which shall be checked for registered or not

    \return TRUE, if the scan type is registered, FALSE otherwise.
*/
bool bredrScanManager_IsScanTypeRegistered(bsm_scan_context_t *context,  bredr_scan_manager_scan_type_t type);

/*! \brief Send disable confirm message to the disable client.

    \param disabled TRUE if disabled. FALSE otherwise.
*/
static inline void bredrScanManager_SendDisableCfm(bool disabled)
{
    MESSAGE_MAKE(cfm, BREDR_SCAN_MANAGER_DISABLE_CFM_T);
    cfm->disabled = disabled;
    MessageSend(bredrScanManager_GetDisableTask(), BREDR_SCAN_MANAGER_DISABLE_CFM, cfm);
}

/*! \brief Initialise the scanner instance.
    \param context The scan context
*/
void bredrScanManager_InstanceInit(bsm_scan_context_t *context);

/*! \brief Register a scan parameter set.
    \param context The scan context.
    \param params The scan params.
*/
void bredrScanManager_InstanceParameterSetRegister(bsm_scan_context_t *context,
                                                   const bredr_scan_manager_parameters_t *params);

/*! \brief Select the user of a scan parameter set.
    \param context The scan context.
    \param index Index in the registered scan parameter set.
*/
void bredrScanManager_InstanceParameterSetSelect(bsm_scan_context_t *context, uint8 index);

/*! \brief Add or update a client to the scan instance.
    \param context The scan context (either page or inquiry scan context).
    \param client The client to add/update.
    \param type The client's scan type.
*/
void bredrScanManager_InstanceClientAddOrUpdate(bsm_scan_context_t *context, Task client,
                                                bredr_scan_manager_scan_type_t type);

/*! \brief Remove a client from the client list.
    \param context The scan context (either page or inquiry scan context).
    \param client The client to remove.
*/
void bredrScanManager_InstanceClientRemove(bsm_scan_context_t *context, Task client);

/*! \brief Inform scan context the firmware has completed its transition and is
           now in the last requested scan state.
    \param context The scan context (either page or inquiry scan context).
*/
bool bredrScanManager_InstanceCompleteTransition(bsm_scan_context_t *context);

/*! \brief Query if the client has enabled scanning.
    \param context The scan context (either page or inquiry scan context).
    \param client The client to query.
*/
bool bredrScanManager_InstanceIsScanEnabledForClient(bsm_scan_context_t *context, Task client);

/*! \brief Call the ConnectionWriteScanEnable function with appropriate
    parameters (considering both page and inquiry scan contexts)
    \param context The scan context (either page or inquiry scan context).
    \note This function may enable or disable scanning considering both page
    and inquiry scan contexts.
*/
void bredrScanManager_ConnectionWriteScanEnable(void);

/*! \brief Call the ConnectionWrite[X]scanActivity function with appropriate
    parameters (considering both page and inquiry scan contexts)
*/
void bredrScanManager_ConnectionWriteScanActivity(bsm_scan_context_t *context);

/*! \brief Handle the CL message.
*/
void bredrScanManager_ConnectionHandleClDmWriteScanEnableCfm(void);

/*! \brief Adjust page scan activity parameters based on throttle requirement as
           informed by Bandwidth Manager.

    \param throttle_required Indication about whether page scan activity
            shall throttle its bandwidth usage or not.
*/
void bredrScanManager_AdjustPageScanBandwidth(bool throttle_required);

/*! \brief Compare present and new required scan parameters and call the
           connection library function to change parameters if they differ.

    \param context Pointer to scan manager context object.

    \param type Scan type.

*/
void bredrScanManager_InstanceUpdateScanActivity(bsm_scan_context_t *context,
                                                        bredr_scan_manager_scan_type_t type);

/*! \brief Set goals for scans based on the current requests from clients.
*/
void bredrScanManager_SetGoals(void);

/*! \brief Chase the set goals.
*/
void bredrScanManager_RunStateMachine(void);

/*! \brief Find the highest priority scan type across all clients.
 *
 *     \param context Pointer to scan manager context object.
 *
 *     \param throttle Bool to indicate throttle scan is requested.
 *
 *     \return Highest priorty scan type supported by the instance.
 */
bredr_scan_manager_scan_type_t bredrScanManager_InstanceFindMaxScanType(bsm_scan_context_t *context, bool throttle);

/*! \brief Check the status of clients in the instance,
 *  remove the clients with pending release scan parameters and
 *  reset fields in the instance that is used in common by the clients
 *  if all the clients have been removed from the instance
 *
 *     \param context Pointer to scan manager context object.
 */
void bredrScanManager_InstanceCleanUp(bsm_scan_context_t *context);
#endif

/*! @} */