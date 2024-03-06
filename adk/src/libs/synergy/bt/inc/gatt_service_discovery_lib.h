/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef GATT_SERVICE_DISCOVERY_LIB_H__
#define GATT_SERVICE_DISCOVERY_LIB_H__

#include "csr_bt_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GATT_SD_PRIM             (SYNERGY_EVENT_BASE + GATT_SRVC_DISC_PRIM)

typedef uint16 GattSdPrim;
typedef uint16 GattSdResult;

/* ---------- Defines the GATT Service Discovery (SD) GattSdResult ----------*/
#define GATT_SD_RESULT_SUCCESS                         ((GattSdResult) (0x0000))
#define GATT_SD_RESULT_INPROGRESS                      ((GattSdResult) (0x0001))
#define GATT_SD_RESULT_UNACCEPTABLE_PARAMETER          ((GattSdResult) (0x0002))
#define GATT_SD_RESULT_DEVICE_NOT_FOUND                ((GattSdResult) (0x0003))
#define GATT_SD_RESULT_SRVC_LIST_EMPTY                 ((GattSdResult) (0x0004))
#define GATT_SD_RESULT_DEVICE_CONFIG_PRESENT           ((GattSdResult) (0x0005))
#define GATT_SD_RESULT_REGISTER_NOT_PERMITTED          ((GattSdResult) (0x0006))
#define GATT_SD_RESULT_ERROR                           ((GattSdResult) (0x0007))
#define GATT_SD_RESULT_SRVC_ID_NOT_FOUND               ((GattSdResult) (0x0008))
#define GATT_SD_RESULT_BUSY                            ((GattSdResult) (0x0009))




/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM            ((GattSdPrim) (0x0000))
#define GATT_SERVICE_DISCOVERY_START_CFM                                  ((GattSdPrim) (0x0001))
#define GATT_SERVICE_DISCOVERY_STOP_CFM                                   ((GattSdPrim) (0x0002))
#define GATT_SERVICE_DISCOVERY_GET_DEVICE_CONFIG_CFM                      ((GattSdPrim) (0x0003))
#define GATT_SERVICE_DISCOVERY_ADD_DEVICE_CONFIG_CFM                      ((GattSdPrim) (0x0004))
#define GATT_SERVICE_DISCOVERY_REMOVE_DEVICE_CONFIG_CFM                   ((GattSdPrim) (0x0005))
#define GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM                     ((GattSdPrim) (0x0006))
#define GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM                        ((GattSdPrim) (0x0007))
#define GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM                   ((GattSdPrim) (0x0008))
#define GATT_SERVICE_DISCOVERY_MESSAGE_TOP                                (0x0008)

/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/
/* Gatt Service Ids for the supported GATT Services
 * It is a bitwise value where each bit represents a specific GATT Service
 * Bit_14 to Bit_31 are RFU*/
typedef uint32 GattSdSrvcId;
#define GATT_SD_INVALID_SRVC         (0x00000000u)
#define GATT_SD_GATT_SRVC            (0x00000001u)
#define GATT_SD_GAP_SRVC             (0x00000002u)
#define GATT_SD_CSIS_SRVC            (0x00000004u)
#define GATT_SD_PACS_SRVC            (0x00000008u)
#define GATT_SD_ASCS_SRVC            (0x00000010u)
#define GATT_SD_VCS_SRVC             (0x00000020u)
#define GATT_SD_GMCS_SRVC            (0x00000040u)
#define GATT_SD_BASS_SRVC            (0x00000080u)
#define GATT_SD_WMAP_SRVC            (0x00000100u)
#define GATT_SD_TBS_SRVC             (0x00000200u)
#define GATT_SD_TDS_SRVC             (0x00000400u)
#define GATT_SD_TMAS_SRVC            (0x00000800u)
#define GATT_SD_CAS_SRVC             (0x00001000u)
#define GATT_SD_HID_SRVC             (0x00002000u)
#define GATT_SD_DI_SRVC              (0x00004000u)
#define GATT_SD_BATTERY_SRVC         (0x00008000u)
#define GATT_SD_SCAN_PARAM_SRVC      (0x00010000u)
#define GATT_SD_AMS_SRVC             (0x00020000u)
#define GATT_SD_ANCS_SRVC            (0x00040000u)
#define GATT_SD_MICS_SRVC            (0x00080000u)
#define GATT_SD_GMAS_SRVC            (0x00100000u)
#define GATT_SD_QGMAS_SRVC           (0x00200000u)


typedef struct  _GattSdSrvcInfo
{
    GattSdSrvcId                 srvcId;
    uint16                       startHandle;
    uint16                       endHandle;
} GattSdSrvcInfo_t;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryRegisterSupportedServices API.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
} GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryStart API.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
} GATT_SERVICE_DISCOVERY_START_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryStop API.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
} GATT_SERVICE_DISCOVERY_STOP_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryGetDeviceConfig API.

    Note:- The memory for srvcInfo should be free by the receving module.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
    uint16                      srvcInfoCount;
    GattSdSrvcInfo_t           *srvcInfo;
} GATT_SERVICE_DISCOVERY_GET_DEVICE_CONFIG_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryAddDeviceConfig API.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
} GATT_SERVICE_DISCOVERY_ADD_DEVICE_CONFIG_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryRemoveDeviceConfig API.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
} GATT_SERVICE_DISCOVERY_REMOVE_DEVICE_CONFIG_CFM_T;

/*!
    @brief GATT SD library message sent as a result of calling the
           GattServiceDiscoveryFindServiceRange API.

    Note:- The memory for srvcInfo should be free by the receving module.
*/
typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
    uint16                      srvcInfoCount;
    GattSdSrvcInfo_t           *srvcInfo;
} GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T;



typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
} GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM_T;

typedef struct
{
    GattSdPrim                  type;
    GattSdResult                result;
    connection_id_t             cid;
    GattSdSrvcId                primarySrvcId;
    uint16                      srvcInfoCount;
    GattSdSrvcInfo_t*           srvcInfo;
} GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM_T;

/*!
    @brief Initialises the GATT Service Discovery Library.

    @param gash Allocate the GATT Service Discovery.library instance and assign
           it to the gash.
           This instance will be passed as the message handler function
           parameter called by the scheduler to process the message send to
           GATT Service Discovery library.

    @return void
*/
void GattServiceDiscoveryInit(void **gash);


/*!
    @brief De-Initialises the GATT Service Discovery Library.

    @return void
*/
    void GattServiceDiscoveryDeinit(void **gash);

/*!
    @brief Register all the Supported GATT services which should be discovered
           GATT Service Discovery module.
           Based on the flag discoverByUuid, It discover services using
           Discover All Primary Service procedure or Discover Primary Service
           by UUID procedure.

    @param appTask Application task
    @param srvcIds Bitwise values of the supported services listed in
                   GattSdSrvcId
    @param discoverByUuid Flag to enable Service Discovery by UUID or discover
                          all primary services

    @return GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM_T confirmation
            message sent to application with the result.
*/
void GattServiceDiscoveryRegisterSupportedServices(AppTask appTask,
                                   GattSdSrvcId srvcIds,
                                   bool discoverByUuid);

/*!
    @brief Start service discovery of the remote device

           Based on the flag discoverByUuid, It discover services using
           Discover All Primary Service procedure or Discover Primary Service
           by UUID procedure.

    @param appTask Application task
    @param cid Connection Id of the remote device

    @return GATT_SERVICE_DISCOVERY_START_CFM_T confirmation message sent to
            application with the result.
            On service discovery started, In Progress result will be sent
            to application.
            On service discovery completion, Success result will be sent
            to application.
*/
void GattServiceDiscoveryStart(AppTask appTask, connection_id_t cid);


/*!
    @brief Stop service disocvery of the remote device

    @param appTask Application task
    @param cid Connection Id of the remote device

    @return GATT_SERVICE_DISCOVERY_STOP_CFM_T confirmation message sent to
            application with the result.
*/
void GattServiceDiscoveryStop(AppTask appTask, connection_id_t cid);


/*!
    @brief Get Remote Device configuration from Gatt Service Discovery
           module.

    @param appTask Application task
    @param cid Connection Id of the remote device

    @return GATT_SERVICE_DISCOVERY_GET_DEVICE_CONFIG_CFM_T confirmation
            message sent to application with the result.
*/
void GattServiceDiscoveryGetDeviceConfig(AppTask appTask, connection_id_t cid);


/*!
    @brief Add Remote Device Configuration to GATT Service Discovery
           module.

    @param appTask Application task
    @param cid Connection Id of the remote device
    @param srvcInfoListCount
    @param srvcInfoList

    @return GATT_SERVICE_DISCOVERY_ADD_DEVICE_CONFIG_CFM_T confirmation
            message sent to application with the result.
*/
void GattServiceDiscoveryAddDeviceConfig(AppTask appTask,
                            connection_id_t cid,
                            uint16 srvcInfoListCount,
                            GattSdSrvcInfo_t *srvcInfoList);


/*!
    @brief Remove Remote device Configuration from GATT Service Discovery
           module.

    @param appTask Application task
    @param cid Connection Id of the remote device

    @return GATT_SERVICE_DISCOVERY_REMOVE_DEVICE_CONFIG_CFM_T confirmation
            message sent to application with the result.
*/
void GattServiceDiscoveryRemoveDeviceConfig(AppTask appTask, connection_id_t cid);


/*!
    @brief Find the service range of the GATT Service on remote device

           This request can be sent by any task running on the device.

    @param task service task
    @param cid Connection Id of the remote device
    @param srvcIds Bitwise values of the supported services listed in
                   GattSdSrvcId

    @return GATT_SERVICE_DISCOVERY_FIND_SRVC_HNDL_RANGE_CFM_T confirmation
            message sent to application with the result.
*/
void GattServiceDiscoveryFindServiceRange(AppTask task,
                                connection_id_t cid,
                                GattSdSrvcId srvcIds);


/*!
    @brief Start to fin included service for given Primary Service on 
           the remote device

    @param appTask  Application task
    @param cid      Connection Id of the remote device
    @param srvcId   Bitwise value of the already Discoverd Primary Service listed in
                    which was by application in GATT_SERVICE_DISCOVERY_FIND_SRVC_HNDL_RANGE_CFM_T
                    confirmation message.

    @return GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_CFM_T confirmation message sent to
            application with the result.
            This api can be called from any client service irrespective of where primary
            service discovery happend. And This api shall be called only after Primary Service
            Discovery.

*/
void GattServiceDiscoverFindIncludedServiceStart(AppTask appTask, connection_id_t cid, GattSdSrvcId srvcId);

/*!
    @brief Find the Included service range of the GATT Service within icovered Primary Services
           on remote device

    @param appTask            Application task
    @param cid                Connection Id of the remote device
    @param primarysrvcId      Bitwise value of the already Discoverd Primary Service listed in
                              which was by application in GATT_SERVICE_DISCOVERY_FIND_SRVC_HNDL_RANGE_CFM_T
                              confirmation message.
    @param includedSrvcIds    Bitwise values of the supported services listed in
                              GattSdSrvcId
    @param includedSrvcsCount Number of included services whose range is required by application 
                              or upper layer

    @return GATT_SERVICE_DISCOVERY_START_CFM_T confirmation message sent to
            application with the result.
            On service discovery started, In Progress result will be sent
            to application.
            On service discovery completion, Success result will be sent
            to application.
*/

void GattServiceDiscoverFindIncludedServiceRange(AppTask appTask, 
                                                connection_id_t cid, 
                                                GattSdSrvcId primarySrvcId, 
                                                GattSdSrvcId includedSrvcId);

#ifdef __cplusplus
}
#endif

#endif /* GATT_SERVICE_DISCOVERY_LIB_H__ */
