#ifndef GATT_ASCS_CLIENT_H
#define GATT_ASCS_CLIENT_H

/******************************************************************************
 Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_bt_gatt_prim.h"
#include "csr_list.h"
#include "csr_bt_tasks.h"
#include "service_handle.h"

#define     GATT_ASCS_CLIENT_MESSAGE_BASE               0x66C0
#define     ASE_ID_ALL                                  0xFF

typedef uint8 AseCharType;

#define GATT_ASCS_CLIENT_ASE_SINK                  (AseCharType)0x01  /*!> SINK Characteristics*/
#define GATT_ASCS_CLIENT_ASE_SOURCE                (AseCharType)0x02  /*!> Source Characteristics*/

typedef uint8 CsrBtAseId;
/*!
    @brief persistent data for each known ASCS device.

    Each ASCS device that is bonded can have data associated against
    it so that re-connections are much faster in that case no GATT discovery is required.
*/
typedef struct
{
    uint8           sourceAseCount;
    uint8           sinkAseCount;
    uint8           *sinkAseId;              /*! AseId of SINK ASE char*/
    uint8           *sourceAseId;            /*! AseId of SOURCE ASE char*/
    CsrBtGattHandle startHandle;
    CsrBtGattHandle endHandle;
    CsrBtGattHandle *sinkAseHandle;         /*! SINK ASE Handle: Used by lib, unused by app*/
    CsrBtGattHandle *sinkAseCcdHandle;     /*! ASE CC Handle */
    CsrBtGattHandle *sourceAseHandle;         /*! SOURCE ASE Handle: Used by lib, unused by app*/
    CsrBtGattHandle *sourceAseCcdHandle;     /*! ASE CC Handle */
    CsrBtGattHandle asesAseControlPointHandle;      /*! ASE Control point Handle: Used by lib, unused by app*/
    CsrBtGattHandle asesAseControlPointCcdHandle;      /*! ASE Control point Handle: Used by lib, unused by app*/
} GattAscsClientDeviceData;



/*! @brief Defines of messages a client task may receive from the published Audio Steram Endpoint Service Client library.
 */
typedef uint16 GattAscsServiceMessageId;

#define GATT_ASCS_CLIENT_INIT_CFM           (GATT_ASCS_CLIENT_MESSAGE_BASE)             /*! Confirmation for Init */
#define GATT_ASCS_CLIENT_TERMINATE_CFM      (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0001u)
#define GATT_ASCS_CLIENT_INDICATION_CFM     (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0002u)   /*! Confirmation for Indication */
#define GATT_ASCS_CLIENT_INDICATION_IND     (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0003u)   /*! Indication for ASE notification values */
#define GATT_ASCS_CLIENT_READ_ASE_INFO_CFM  (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0004u)   /*! Confirmation for Read */
#define GATT_ASCS_CLIENT_READ_ASE_STATE_CFM (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0005u)
#define GATT_ASCS_CLIENT_WRITE_ASE_CFM      (GATT_ASCS_CLIENT_MESSAGE_BASE + 0x0006u)
	

/*!
    \brief ASCS Client status code type.
*/
typedef uint16 GattAscsClientStatus;

/*! { */
/*! Values for the AICS status code */
#define GATT_ASCS_CLIENT_STATUS_SUCCESS                   (0x0000u)  /*!> Request was a success*/
#define GATT_ASCS_CLIENT_STATUS_DISCOVERY_ERR             (0x0001u)  /*!> Error in discovery of Characteristics*/
#define GATT_ASCS_CLIENT_STATUS_FAILED                    (0x0002u)  /*!> Request has failed*/
#define GATT_ASCS_CLIENT_STATUS_INSUFFICIENT_RESOURCES    (0x0003u)  /*!> Insufficient Resources to complete
                                                                          the request. */
#define GATT_ASCS_CLIENT_STATUS_NOT_ALLOWED               (0x0004u)  /*!> Request not allowed*/
#define GATT_ASCS_CLIENT_STATUS_NO_CONNECTION             (0x0005u)  /*!> No connection*/

/*! } */


/*!
    @brief Parameters used by the Initialisation API, valid value of these  parameters are must for library initialisation
*/
typedef struct
{
     ConnectionId    cid;           /*!Connection ID of the GATT connection on which the server side ASCS Service need to be accessed*/
     CsrBtGattHandle startHandle;  /*! The first handle of ASCS Service need to be accessed*/
     CsrBtGattHandle endHandle;    /*!The last handle of ASCS Service need to be accessed */
} GattAscsClientInitParams;

/*!
    @brief Initialises the ASCS Client Library.
     Initialize ASCS client library handles, It starts finding out the characteristic handles of ASCS Service.
     Once the initialisation has been completed, GATT_ASCS_CLIENT_INIT_CFM will be received with
     status as enumerated as gatt_ases_client_status.'gatt_ases_client_status_success' has to
     be considered initialisation of library is done successfully and all the required characteristics has been found out

     NOTE:This interface need to be invoked for every new gatt connection when the client wish to use
     ASCS client library

    @param appTask The Task that will receive the messages sent from this ASCS client  library.
    @param clientInitParams as defined in GattAscsClientInitParams, it is must all the parameters are valid
                The memory allocated for GattAscsClientInitParams can be freed once the API returns.
    @param deviceData Pointer to GattAscsClientDeviceData data structure. Pointers inside the
                       structure needs to be freed by application itself after successful init
    @return TRUE if success, FALSE otherwise

*/
void GattAscsClientInit(AppTask appTask ,
                        const GattAscsClientInitParams *const clientInitParams,
                        const GattAscsClientDeviceData *deviceData);

void GattAscsClientTerminate(ServiceHandle srvcHndl);

/*!@brief ASCS Client Library initialisation confirmation
*/
typedef struct __GattAscsClientInitCfm
{
    GattAscsServiceMessageId id;
    ConnectionId             cid;                         /*! connection id */
    ServiceHandle            clientHandle;               /*! service handle for client instance */
    GattAscsClientStatus     status;  /*! Status as per gatt_ases_client_status */

} GattAscsClientInitCfm;


typedef struct __GattAscsClientTerminateCfm
{
    GattAscsServiceMessageId     id;                      /*! service message id*/
    ServiceHandle                clientHandle;           /*! service handle for client instanced */
    GattAscsClientStatus         status;                  /*! status messsage */
    GattAscsClientDeviceData     deviceData;             /*! data to be stored in persistent storage */
} GattAscsClientTerminateCfm;


/*!
    @brief ASCS Client Library notification registration confirmation
             This confirmation will be received on Enabling/Disabling indication
*/
typedef struct __GattAscsClientIndicationCfm
{
    GattAscsServiceMessageId id;
    ServiceHandle            clientHandle;
    GattAscsClientStatus     status;   /*! status as per gatt_ases_client_status */
} GattAscsClientIndicationCfm;


/*!
    @brief ASCS Client Library notification indication, This indication will be received on each notification from ASCS service
*/
typedef struct __GattAscsClientIndicationInd
{
    GattAscsServiceMessageId id;
    ServiceHandle            clientHandle;         /*! service handle for client instance */
    CsrBtAseId               aseId;
    uint16                   sizeValue; /*! Size of data in buffer 'value' */
    uint8                    *value; /*! Data buffer containing PAC record or Location information */
} GattAscsClientIndicationInd;


/*!
    @brief ASCS library read information confirmation
             This confirmation will be received with ASCS
*/
typedef struct __GattAscsClientReadAseInfoCfm
{
    GattAscsServiceMessageId id;
    ServiceHandle            clientHandle;
    GattAscsClientStatus     status; /*! status as per gatt_ascs_client_status */
    uint16                   noOfAse; /*! Size of data in buffer 'value' */
    uint8                    *aseId; /*! Data buffer containing HID informtaion */
    AseCharType              charType;
}GattAscsClientReadAseInfoCfm;

typedef struct __GattAscsClientReadAseStateCfm
{
    GattAscsServiceMessageId id;
    ServiceHandle            clientHandle;
    CsrBtAseId               aseId;
    GattAscsClientStatus     status; /*! status as per gatt_ascs_client_status */
    uint16                   sizeValue; /*! Size of data in buffer 'value' */
    uint8                    *value; /*! Data buffer containing HID informtaion */
}GattAscsClientReadAseStateCfm;

typedef struct __GattAscsClientWriteAseCfm
{
    GattAscsServiceMessageId id;
    ServiceHandle            clientHandle;
    CsrBtAseId               aseId;
    GattAscsClientStatus     status; /*! status as per gatt_ascs_client_status */
}GattAscsClientWriteAseCfm;


/*!
    @brief Register for Ase and Ase control point notification from peer.

    @param btConnId The device for which cccd has to be enabled
    @return TRUE if success, FALSE indicates the ASCS client is not yet initialised to process the request
*/
void GattAscsRegisterForNotification(ServiceHandle srvcHndl,
                                              CsrBtAseId aseId, CsrBool enable);

/*!
    @brief Read Ase Id for ASE characteristics.

    @param ases_client The client instance that was passed into the GattAseClientInit API.
    @param handle data to be read from.
    @param aseCharType Characteristics type

    @return TRUE if success, FALSE otherwise
*/
bool GattAscsReadAseInfoReq(ServiceHandle srvcHndl, AseCharType aseCharType);

/*!
    @brief Read state for given ASE id.

    @param ases_client The client instance that was passed into the GattAseClientInit API.
    @param handle data to be read from.
    @return TRUE if success, FALSE otherwise
*/
void GattAscsClientReadAseStateReq(ServiceHandle srvcHndl, uint8 aseId, AseCharType aseCharType);


/*!
    @brief Write Ase Audio data for ASE characteristics.

    @param ases_client The client instance that was passed into the GattAseClientInit API.
    @param sizeValue size of the data to be write
    @value pointer to the data to be written
    @return TRUE if success, FALSE otherwise
*/
void GattAscsWriteAsesControlPointReq(ServiceHandle srvcHndl, CsrUint16 sizeValue, CsrUint8 *value);



void GattAscsSetAsesControlPointReq(ServiceHandle clientHandle , bool response);

/*!
    @brief This API is used to retrieve the ascs characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.
    @return GattAscsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.
*/

GattAscsClientDeviceData *GattAscsClientGetHandlesReq(ServiceHandle clntHndl);

#endif /* GATT_ASCS_CLIENT_H */

