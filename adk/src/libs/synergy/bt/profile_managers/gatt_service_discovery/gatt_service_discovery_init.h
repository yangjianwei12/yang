/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef GATT_SERVICE_DISCOVERY_INIT_H__
#define GATT_SERVICE_DISCOVERY_INIT_H__

#include "gatt_service_discovery_private.h"
#include "gatt_service_discovery_debug.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Gatt Service Discovery : Device List specific macros */
#define GATT_SD_DL_ADD_DEVICE(list)            gattSdDlAddDevice(list)
#define GATT_SD_DL_REMOVE_DEVICE(list, elem)   gattSdDlRemoveDevice(list, elem)
#define GATT_SD_DL_FIND_BY_CONNID(list, btConnId)   gattSdDlFindByConnid(list, btConnId)
#define GATT_SD_DL_CLEANUP(list)               gattSdDlCleanup(list)


/* Gatt Service Discovery : Device Service List specific macros */
#define GATT_SD_SL_FIND_DUPLICATE(list, elem)         gattSdSlFindDuplicate(list, elem)
#define GATT_SD_SL_ADD_SRVC(list)                     gattSdSlAddService(list)
#define GATT_SD_SL_CLEANUP(list)                      gattSdSlCleanup(list)
#define GATT_SD_SL_FIND_BY_SDSRVCID(list, sdSrvcId)   gattSdSlFindBySdsrvcId(list, sdSrvcId)


/* Gatt Service Discovery : Included Service List specific macros */
#define GATT_SD_ISL_ADD_SRVC(list)                      gattSdIncLSrvAddService(list)
#define GATT_SD_ISL_FIND_BY_SRVCID(list, sdSrvcId)      gattSdIslFindBySdsrvcId(list, sdSrvcId)
#define GATT_SD_ISL_CLEANUP(list)                       gattSdIslCleanup(list)



/* GATT Service Discovery state */
typedef enum
{
    GATT_SRVC_DISC_STATE_IDLE,
    GATT_SRVC_DISC_STATE_INPROGRESS
} GATT_SRVC_DISC_STATE;

/* GATT Service discovery internal structure */
typedef struct  __GGSD
{
    AppTaskData                     lib_task;
    AppTask                         app_task;
    AppTask                         findInclSrvcsTask;

    /* GATT Service Discovery State */
    GATT_SRVC_DISC_STATE         state;

    /* List of remote devices */
    GattSdDeviceElement*         deviceList;
    uint16                       deviceListCount;
    /* GATT Services to be discover  */
    GattSdSrvcId                 srvcIds;
    /* Primary Service discovery procedure flag  */
    bool                         discoverByUuid;
    /* Current connection id */
    connection_id_t              curCid;
    /* Current Service id */
    GattSdSrvcId                 curSrvcId;
    /* GATT registration ID */
    uint32                       gattId;
} GGSD;

/***************************************************************************
NAME
    gattServiceDiscoveryGetInstance

DESCRIPTION
    Returns the GATT Service Discovery Instance.
*/
GGSD* gattServiceDiscoveryGetInstance(void);

/***************************************************************************
NAME
    gattServiceDiscoveryIsInit

DESCRIPTION
    Returns TRUE if GATT Service Discovery library is initialzed else FALSE.
*/
bool gattServiceDiscoveryIsInit(void);


#ifdef __cplusplus
}
#endif

#endif /* GATT_SERVICE_DISCOVERY_INIT_H__ */

