/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_scan_manager
    \brief      LE Extended scanning component
    @{
*/

#ifndef LE_SCAN_MANAGER_EXTENDED_H
#define LE_SCAN_MANAGER_EXTENDED_H

#include "le_scan_manager.h"
#include "le_scan_manager_protected.h"

#ifdef ENABLE_LE_EXTENDED_SCANNING

/*! \brief Data sent with LE_SCAN_MANAGER_EXTENDED_STOP_CFM message. */
typedef struct
{
    le_scan_manager_status_t result;
    Task scan_task;
} LE_SCAN_MANAGER_EXTENDED_STOP_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_EXTENDED_DISABLE_CFM message. */
typedef struct
{
    le_scan_manager_status_t result;
} LE_SCAN_MANAGER_EXTENDED_DISABLE_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_EXTENDED_ENABLE_CFM message. */
typedef struct
{
    le_scan_manager_status_t result;
} LE_SCAN_MANAGER_EXTENDED_ENABLE_CFM_T;


/*! \brief Initializes the LE Extended Scan Module. */
void LeScanManager_ExtendedScanInit(void);

/*! \brief Starts the LE Extended Scan */
void LeScanManager_ExtendedScanStart(Task task, le_extended_advertising_filter_t *filter);

/*! \brief Stops the LE Extended Scan */
bool LeScanManager_ExtendedScanStop(Task req_task, Task scan_task);

/*! \brief Handles Connection manager messages */
bool LeScanManager_HandleExtendedCmMessages(void *msg);

/*! \brief Enable extended scan */
bool leScanManager_ExtendedScanEnable(Task req_task);

/*! \brief Disable extended scan */
bool leScanManager_ExtendedScanDisable(Task req_task);

/*! \brief Is extended task scanning */
bool LeScanManager_IsExtendedTaskScanning(Task task);

#endif /* ENABLE_LE_EXTENDED_SCANNING */

#endif /* LE_SCAN_MANAGER_EXTENDED_H */
/* @} */
