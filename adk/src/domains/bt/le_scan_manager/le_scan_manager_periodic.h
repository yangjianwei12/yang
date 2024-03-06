/*!
    \copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_scan_manager
    \brief      LE periodic scanning component
    @{
*/

#ifndef LE_SCAN_MANAGER_PERIODIC_H_
#define LE_SCAN_MANAGER_PERIODIC_H_

#include "le_scan_manager.h"
#include "le_scan_manager_protected.h"

typedef struct {
    le_scan_manager_status_t result;
    Task scan_task;
}LE_SCAN_MANAGER_PERIODIC_STOP_CFM_T;

typedef struct {
    le_scan_manager_status_t result;
}LE_SCAN_MANAGER_PERIODIC_DISABLE_CFM_T;

typedef struct {
    le_scan_manager_status_t result;
}LE_SCAN_MANAGER_PERIODIC_ENABLE_CFM_T;

#ifdef INCLUDE_ADVERTISING_EXTENSIONS

void leScanManager_PeriodicScanInit(void);

void leScanManager_StartPeriodicScanFindTrains(Task task, le_periodic_advertising_filter_t* filter);

bool leScanManager_PeriodicScanStop(Task req_task, Task scan_task);

bool LeScanManager_IsPeriodicTaskScanning(Task task);

#ifdef USE_SYNERGY
bool leScanManager_HandlePeriodicCmMessages(void *msg);
#else
bool leScanManager_HandlePeriodicClMessages(MessageId id, Message message);
#endif

bool leScanManager_PeriodicScanDisable(Task req_task);

bool leScanManager_PeriodicScanEnable(Task req_task);


#else
    
#define leScanManager_PeriodicScanInit() ((void)(0))

#define leScanManager_StartPeriodicScanFindTrains(x, y) ((void)(0))

#define leScanManager_PeriodicScanStop(x, y) (FALSE)

#define LeScanManager_IsPeriodicTaskScanning(x) (FALSE)

#ifdef USE_SYNERGY
#define leScanManager_HandlePeriodicCmMessages(x) (FALSE)
#else
#define leScanManager_HandlePeriodicClMessages(x, y) (FALSE)
#endif

#define leScanManager_PeriodicScanDisable(x) (FALSE)

#define leScanManager_PeriodicScanEnable(x) (FALSE)

#endif /* INCLUDE_ADVERTISING_EXTENSIONS */


#endif /* LE_SCAN_MANAGER_PERIODIC_H_ */
/* @} */
