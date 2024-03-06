/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #6 $
******************************************************************************/

/*!
@file    gatt_hids_server.h
@brief   Header file for the GATT HIDS (Human Interface Device Service) Server library.

        This file provides documentation for the GATT HIDS server library
        API (library name: gatt_hids_server).
*/

#ifndef GATT_HIDS_H_
#define GATT_HIDS_H_

#include <service_handle.h>
#include "csr_bt_gatt_prim.h"
#include "csr_bt_tasks.h"


/*struct to save ReportMapData*/
typedef struct
{
    uint8 reportId;
    uint8 reportType;  /* Input == 1, Feature == 3*/
    uint32 reportLen;
}ReportMapData;


/* Size of the characteristic sizes (number of octets)*/
#define REPORT_SIZE            (1)
#define REPORT_MAP_SIZE        (2)
#define HID_INFORMATION_SIZE   (4)
#define HID_CONTROL_POINT_SIZE (1)


#define HIDS_MAX_CONN 1
/*! \brief Enumeration of messages an application task can receive from the
*          GATT HIDS Server Library as a result of receiving a report from the client.
*/
typedef uint16 GattHidsMessageId;

typedef uint8 GattHidInformationFlag;


/* below defines, bType and bTag, come from the table under uint8 GattHidsServerSetReportMap()*/

/*short item format bType*/
#define HIDS_TYPE_MAIN   0
#define HIDS_TYPE_GLOBAL 1
#define HIDS_TYPE_LOCAL  2

/*short item format bTag*/
#define HIDS_TAG_INPUT          8
#define HIDS_TAG_OUTPUT         9
#define HIDS_TAG_COLLECTION     10
#define HIDS_TAG_FEATURE        11
#define HIDS_TAG_END_COLLECTION 12
#define HIDS_TAG_USAGE_PAGE    0
#define HIDS_TAG_REPORT_SIZE   7
#define HIDS_TAG_REPORT_ID     8
#define HIDS_TAG_REPORT_COUNT  9

/*!
  HIDS message Id's
*/
#define GATT_HIDS_SERVER_INIT_CFM                   0
#define GATT_HIDS_SERVER_SET_CONTROL_POINT_CFM      3
#define GATT_HIDS_SERVER_SET_REPORT_MAP_CFM         4
#define GATT_HIDS_SERVER_SET_INPUT_REPORT_CFM       5
#define GATT_HIDS_SERVER_SET_FEATURE_REPORT_CFM     6
#define GATT_HIDS_SERVER_SET_HIDS_INFO_CFM          7
#define GATT_HIDS_SERVER_CONTROL_POINT_WRITE_IND    9
#define GATT_HIDS_SERVER_FEATURE_REPORT_WRITE_IND   10


/*! Values for the HIDS status code */
#define HIDS_SUCCESS                              ((GattHidsServerStatus)0x0000u)  /*!> Request was a success*/
#define HIDS_INPUT_REPORT_NUMBER_EXCEEDS_ONE      ((GattHidsServerStatus)0x0001u)
#define HIDS_FEATURE_REPORT_NUMBER_EXCEEDS_THREE  ((GattHidsServerStatus)0x0002u)
#define HIDS_REPORT_ID_IS_ZERO                    ((GattHidsServerStatus)0x0003u)
#define HIDS_ERROR                                ((GattHidsServerStatus)0x0004u)




/*Report Types*/
#define INPUT_REPORT   1
#define FEATURE_REPORT 3

typedef uint16 GattHidsServerStatus;

typedef struct
{
    uint8            *mapData;
    uint16           mapDataLen;
} GattHidsServerReportMap;

/*! \brief The structure in which the Report characteristic will be stored.
*/
typedef struct
{
    uint8            *data;
    uint16           dataLen;
    uint8            reportId; /*0x00 – 0xFF*/
    uint8            reportType;
} GattHidsServerInputReport;

typedef GattHidsServerInputReport GattHidsServerFeatureReport;

/*! \brief The structure in which the HID Information structure will be stored.
*/
typedef struct
{
    uint16                 bcdHID;
    uint8                  bCountryCode;
    GattHidInformationFlag flags:2;
} GattHidsServerHidInformation;

/*! \brief HID Control Point characteristic value
 */
#define GATT_HIDS_SERVER_SUSPEND 0
#define GATT_HIDS_SERVER_EXIT_SUSPEND  1


/*!
    GattHIDSControlPointWriteInd
    This message is sent by hids when Remote device modifies
    the control point
*/

typedef struct
{
    GattHidsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    uint8 OldControlPoint;
    uint8 NewControlPoint;
} GattHidsServerControlPointWriteInd;

/*!
    GattHidsServerFeatureReportWriteInd
    This message is sent by hids when Remote device modifies
    the control point
*/
typedef struct
{
    GattHidsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    uint8            *data;
    uint16           dataLen;
    GattHidsServerStatus status;
} GattHidsServerFeatureReportWriteInd;

/*!
    GattHidsServerClientConfigWriteInd
    This message is sent by  when Remote device modifies
    the Client Config characteristic of hids Server
*/

typedef struct
{
    GattHidsMessageId id;
    ConnectionId cid;
    CsrBtGattId gattId;
    ServiceHandle srvcHndl;
    uint8 OldClientConfig;
    uint8 NewClientConfig;
} GattHidsServerClientConfigWriteInd;



/*! \brief This is the message the application will recieve what has been written to
           the device.
*/
typedef struct
{
    GattHidsMessageId id;
    ConnectionId      cid;
    GattHidsServerInputReport       report;
} GattHidsServerInputReportInd;

/*! @brief Client Config data.

    This structure contains the client configuration of all the characteristics
    of the Human Device Interface Service
*/
typedef struct
{
    uint16 inputReport1ClientConfig:2;
    uint16 inputReport2ClientConfig:2;
} GattHidsServerConfigType;


/*! @brief Client data.

    This structure contains data for each connected client
 */
typedef struct
{
    connection_id_t        cid;
    GattHidsServerConfigType  clientCfg;
} GattHidsServerClientData;

/* Write CFM messages */
/*! @brief Contents of the GATT_HIDS_SERVER_WRITE_<>_CFM message that is sent by the library,
    as a result of writing the HIDS set info characteristic on the server.
*/
typedef struct
{
    GattHidsMessageId id;
    ServiceHandle srvcHndl;
    GattHidsServerHidInformation hidsInfo;
} GattHidsServerSetHidsInfoCfm;

/* Write CFM messages */
/*! @brief Contents of the GATT_HIDS_SERVER_WRITE_<>_CFM message that is sent by the library,
    as a result of writing the HIDS set control point characteristic on the server.
*/
typedef struct
{
    GattHidsMessageId id;
    ServiceHandle srvcHndl;
    uint8 controlPoint;
} GattHidsServerSetControlPointCfm;

/* Write CFM messages */
/*! @brief Contents of the GATT_HIDS_SERVER_WRITE_<>_CFM message that is sent by the library,
    as a result of writing the HIDS INIT on the server.
*/
typedef struct
{
    GattHidsMessageId id;
    ServiceHandle srvcHndl;
} GattHidsServerInitCfm;

/* Write CFM messages */
/*! @brief Contents of the GATT_HIDS_SERVER_WRITE_<>_CFM message that is sent by the library,
    as a result of writing the HIDS set report map characteristic on the server.
*/
typedef struct
{
    GattHidsMessageId id;
    ServiceHandle srvcHndl;
    uint8 usagePage;
    uint8 totalReportNo;
    uint8 reportId[5];
    uint8 reportType[5];  /* Input == 1, Feature == 3*/
    uint32 reportLen[5];
} GattHidsServerSetReportMapCfm;

/* Write CFM messages */
/*! @brief Contents of the GATT_HIDS_SERVER_WRITE_<>_CFM message that is sent by the library,
    as a result of writing the HIDS set input report characteristic on the server.
*/
typedef struct
{
    GattHidsMessageId id;
    ServiceHandle srvcHndl;
    uint8 reportId;
    uint32 dataLen;
    GattHidsServerStatus status;
} GattHidsServerSetInputReportCfm;


/*!
    @brief Instantiate the GATT HIDS Server Service Library

    The GATT Service Init function is responsible for allocating its instance memory
    and returning a unique service handle for that instance. The Service handle is
    then used for the rest of the API.

    @param theAppTask                The client task that will receive messages from this Service.
    @param startHandle               The first handle in the ATT database for this Service instance.
    @param endHandle                 The last handle in the ATT database for this Service instance.

    @return ServiceHandle If the service handle returned is 0, this indicates a failure
                             during GATT Service initialisation.
*/
ServiceHandle GattHidsServerInit(AppTask theAppTask,
                                 uint16  startHandle,
                                 uint16  endHandle);

/*!
    @brief Add configuration for a paired peer device, identified by its
    Connection ID (CID).

    @param srvcHndl Instance handle for the service.
    @param cid The Connection ID to the peer device.
    @param config Client characteristic configurations for this connection.
           If this is NULL, this indicates a default config should be used for the
           peer device identified by the CID.
    @return status_t status of the Add Configuration operation.
*/
status_t GattHidsServerAddConfig(ServiceHandle srvcHndl,
                                 connection_id_t  cid,
                                 GattHidsServerConfigType *const config);

/*!
    @brief Remove the configuration for a peer device, identified by its
           Connection ID.

    @param srvcHndl Instance handle for the service.
    @param connectionId to Bluetooth Connection Id of the remote device requesting connection

    @return GattHidsServerConfigType Pointer to the peer device configuration
            data. It is the applications responsibility to cache/store
            this between connections of paired peer devices, and for
            freeing the allocated memory.
            If the ConnectionId is not found, the function will return NULL.
*/
GattHidsServerConfigType* GattHidsServerRemoveConfig(ServiceHandle srvcHndl,
                                                connection_id_t connectionId);

/*!
    \brief Set the Input Report characteristic and notifies subscribed clients.

    \param srvcHndl    The GATT service instance handle.
    \param index       report number, 1 or 2
    \param reportId    report ID
    \param length      Length of data.
    \param data        Data to be sent in the report.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
GattHidsServerStatus GattHidsServerSetInputReport(ServiceHandle srvcHndl,
                                   uint8         index,
                                   uint8         reportId,
                                   uint16        length,
                                   uint8         *data);

/*!
    \brief Set the Feature Report characteristic.

    \param srvcHndl    The GATT service instance handle.
    \param index       report number, 1, 2 or 3
    \param reportId    report ID
    \param length      Length of data.
    \param data        Data to be sent in the report.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 GattHidsServerSetFeatureReport(ServiceHandle srvcHndl,
                                     uint8         index,
                                     uint8         reportId,
                                     uint16        length,
                                     uint8         *featureData);

/*!
    \brief Set the Report Map characteristic and notifies subscribed clients.

    \param srvcHndl    The GATT service instance handle.
    \param length      Length of data.
    \param data        map data.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 GattHidsServerSetReportMap(ServiceHandle srvcHndl,
                                  uint16       len,
                                  const uint8  *mapData);

/*!
    \brief Find the number of Feature report report map has sent, MAX must be three.

    \param totalReportNumber   is three
    \param mapData      map data receieved.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 HidsServerFeatureReportNumber(uint8 totalReportNumber, ReportMapData *mapData);

/*!
    \brief Find the number of Input report report map has sent, Max is one.

    \param totalReportNumber   is three
    \param mapData      map data receieved.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 HidsServerInputReportNumber(uint8 totalReportNumber, ReportMapData *mapData);

/*!
    \brief Set Hids Info.

    \param srvcHndl      The GATT service instance handle.
    \param bcdHID        representing version number of base USB HID.
    \param bCountryCode  identifying country HID Device hardware is localized for.
    \param flags         Boolean value indicating whether HID Device is capable of sending a
                         wake-signal to a HID Host.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 GattHidsServerSetHidsInfo(ServiceHandle srvcHndl,
                                uint16        bcdHID,
                                uint8      bCountryCode,
                                uint8       flags);

/*!
    \brief Set Hids Info.

    \param srvcHndl        The GATT service instance handle.
    \param suspendControl  Informs HID Device that HID Host is entering/exiting the Suspend State.

    \return HIDS_SUCCESS code if successful, HIDS_ERROR otherwise.
*/
uint8 GattHidsServerSetControlPoint(ServiceHandle srvcHndl,
                                uint8       suspendControl);


/*!
    \brief De initialization.

    \param srvcHndl        The GATT service instance handle.

    \return SUCCRESS code if successful, FALSE otherwise.
*/
uint8 GattHidsServerTerminate(ServiceHandle srvcHndl);
#endif /* GATT_HIDS_SERVER_H_ */
