/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      LE Audio Broadcast Self-Scan module private header.
*/
#ifndef LE_BROADCAST_MANAGER_SELF_SCAN_PRIVATE_H
#define LE_BROADCAST_MANAGER_SELF_SCAN_PRIVATE_H

#if defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN)

#include "le_broadcast_manager_self_scan.h"


/*! Default self-scan timeout if client does not specify their own timeout. */
#define LE_BROADCAST_SELF_SCAN_TIMEOUT_DEFAULT_MS   D_SEC(30)

/*! Minimum timeout value allowed. */
#define LE_BROADCAST_SELF_SCAN_TIMEOUT_MINIMUM_MS   D_SEC(0.5)

/*! Maximum timeout value allowed. */
#define LE_BROADCAST_SELF_SCAN_TIMEOUT_MAXIMUM_MS   D_SEC(1200)


/* The types used for the self-scan task data are generated from the .typedef file. */
#include "le_broadcast_manager_self_scan_typedef.h"


extern le_bm_self_scan_task_data_t self_scan_data;

/*! Get pointer to the self scan data structure */
#define leBroadcastManager_SelfScanGetTaskData()    (&self_scan_data)

/*! Get the Task for the self-scan module. */
#define leBroadcastManager_SelfScanGetTask()    (&self_scan_data.task)


extern le_bm_periodic_scan_callback_interface_t self_scan_periodic_scan_interface;

/*! Get a pointer to the self scan periodic scan interface implementation. */
#define leBroadcastManager_SelfScanGetPeriodicScanInterface()    (&self_scan_periodic_scan_interface)


/*! \brief Create a new self-scan client instance.

    \param[in] task Client Task that notifications are sent to.
    \param[in] params Parameters for the self-scan.

    \return Pointer to a self-scan client instance or NULL if it could not be created.
*/
le_bm_self_scan_client_t *LeBroadcastManager_SelfScanClientCreate(Task task, const self_scan_params_t *params);

/*! \brief Destroy a self-scan client instance.

    \param client_to_destroy Pointer to self-scan client instance to destroy.
*/
void LeBroadcastManager_SelfScanClientDestroy(le_bm_self_scan_client_t *client_to_destroy);

/*! \brief Check if any self-scan client has an ongoing scan active.

    \return TRUE if at least one client is actively scanning; FALSE otherwise.
*/
bool LeBroadcastManager_SelfScanIsAnyClientActive(void);

#endif /* defined(INCLUDE_LE_AUDIO_BROADCAST_LOCAL_SCAN) */

#endif // LE_BROADCAST_MANAGER_SELF_SCAN_PRIVATE_H
