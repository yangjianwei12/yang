/****************************************************************************
Copyright (c) 2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version

FILE NAME
    micp.h

DESCRIPTION
    Header file for the Microphone Control Profile (MICP) library.
*/

/*!
@file    micp.h
@brief   Header file for the GATT MICP library.

        This file provides documentation for the GATT MICP library
        API (library name: micp).
*/

#ifndef MICP_H
#define MICP_H

#include "csr_bt_gatt_lib.h"
#include "service_handle.h"


#include "gatt_mics_client.h"
#include "gatt_aics_client.h"

/*!
    \brief Profile handle type.
*/
typedef ServiceHandle MicpProfileHandle;

#define MICP_MESSAGE_BASE 0x00

/*!
    @brief AICS handles.

*/
typedef struct
{
    uint16                   startHandle;
    uint16                   endHandle;
    GattAicsClientDeviceData handles;
} MicpAicsHandle;

/*!
    @brief MICP handles.

*/
typedef struct
{
    GattMicsClientDeviceData micsHandle;

    uint16         aicsHandleLength;
    MicpAicsHandle *aicsHandle;
} MicpHandles;

/*!
    @brief Initialisation parameters.

*/
typedef struct
{
    connection_id_t cid;
} MicpInitData;

/*!
    \brief MICP status code type.
*/
typedef uint16 MicpStatus;

/*! { */
/*! Values for the MICP status code */
#define MICP_STATUS_SUCCESS              ((MicpStatus)0x0000u)  /*!> Request was a success*/
#define MICP_STATUS_IN_PROGRESS          ((MicpStatus)0x0001u)  /*!> Request in progress*/
#define MICP_STATUS_INVALID_PARAMETER    ((MicpStatus)0x0002u)  /*!> Invalid parameter was supplied*/
#define MICP_STATUS_DISCOVERY_ERR        ((MicpStatus)0x0003u)  /*!> Error in discovery of one of the services*/
#define MICP_STATUS_FAILED               ((MicpStatus)0x0004u)  /*!> Request has failed*/

/*!
    \brief IDs of messages a profile task can receive from the
           MICP library.
*/
typedef uint16 MicpMessageId;

/*! { */
/*! Values for MicpMessageId */
#define MICP_INIT_CFM                          ((MicpMessageId)MICP_MESSAGE_BASE)
#define MICP_DESTROY_CFM                       ((MicpMessageId)MICP_MESSAGE_BASE + 0x0001u)
#define MICP_MICS_TERMINATE_CFM                ((MicpMessageId)MICP_MESSAGE_BASE + 0x0002u)
#define MICP_AICS_TERMINATE_CFM                ((MicpMessageId)MICP_MESSAGE_BASE + 0x0003u)
#define MICP_NTF_CFM                           ((MicpMessageId)MICP_MESSAGE_BASE + 0x0004u)
#define MICP_READ_MUTE_VALUE_CFM               ((MicpMessageId)MICP_MESSAGE_BASE + 0x0005u)
#define MICP_READ_MUTE_VALUE_CCC_CFM           ((MicpMessageId)MICP_MESSAGE_BASE + 0x0006u)
#define MICP_MUTE_VALUE_IND                    ((MicpMessageId)MICP_MESSAGE_BASE + 0x0007u)
#define MICP_SET_MUTE_VALUE_CFM                ((MicpMessageId)MICP_MESSAGE_BASE + 0x0008u)
#define MICP_MESSAGE_TOP                       ((MicpMessageId)MICP_MESSAGE_BASE + 0x0009u)

/*! } */

/*!
    @brief Profile library message sent as a result of calling the MicpInitReq API.
*/
typedef struct
{
    MicpMessageId     id;
    MicpStatus        status;     /*! Status of the initialisation attempt*/
    connection_id_t  cid;
    MicpProfileHandle prflHndl;   /*! MICP profile handle*/
    uint16           aicsNum;    /*! Number of AICS instances*/
} MicpInitCfm;

/*!
    @brief Profile library message sent as a result of calling the MicpDestroyReq API.

    This message will send at first with the value of status of MICP_STATUS_IN_PROGRESS.
    Another MICP_DESTROY_CFM message will be sent with the final status (success or fail),
    after MICP_MICS_TERMINATE_CFM and
    MICP_AICS_TERMINATE_CFM have been received.
*/
typedef struct
{
    MicpMessageId      id;
    MicpProfileHandle  prflHndl;  /*! MICP profile handle*/
    MicpStatus         status;    /*! Status of the destroy attempt*/
} MicpDestroyCfm;

/*!
    @brief Profile library message sent as a result of calling the MicpDestroyReq API and
           of the receiving of the GATT_MICS_CLIENT_TERMINATE_CFM message from the gatt_mics_client
           library.
*/
typedef struct
{
    MicpMessageId               id;
    MicpProfileHandle           prflHndl;        /*! MICP profile handle*/
    MicpStatus                  status;          /*! Status of the termination attempt*/
    GattMicsClientDeviceData    micsHandle;       /*! Characteristic handles of MICS*/
} MicpMicsTerminateCfm;

/*!
    @brief Profile library message sent as a result of calling the MicpDestroyReq API and
           of the receiving of all the GATT_AICS_CLIENT_TERMINATE_CFM messages from the
           gatt_aics_client library.
*/
typedef struct
{
    MicpMessageId             id;
    MicpProfileHandle         prflHndl;       /*! MICP profile handle*/
    MicpStatus                status;         /*! Status of the termination attempt*/
    uint16                   aicsSizeValue;  /*! Number of AICS client instances*/
    uint16                   startHandle;    /*! Start handle of the AICS instance */
    uint16                   endHandle;      /*! End handle of the AICS instance */
    GattAicsClientDeviceData aicsValue;      /*! Characteristic handles of the AICS client instances*/
    bool                     moreToCome;     /*! TRUE if more of this message will come, FALSE otherwise*/
} MicpAicsTerminateCfm;

/*! @brief Contents of the MICP_NTF_CFM message that is sent by the library,
    as a result of setting notifications on the server for the Mute characteristic.
 */
typedef struct
{
    MicpMessageId     id;
    MicpProfileHandle prflHndl;
    status_t         status;
} MicpNtfCfm;

/*! @brief Contents of the MICP_SET_MUTE_VALUE_CFM message that is sent by the library,
    as a result of writing the Mute characteristic on the server using the Set Mute
    operation.
 */
typedef MicpNtfCfm MicpSetMuteValueCfm;

/*! @brief Contents of the MICP_MUTE_VALUE_IND message that is sent by the library,
    as a result of a notification of the remote Mute characteristic value.
 */
typedef struct
{
    MicpMessageId     id;
    MicpProfileHandle prflHndl;
    uint8            muteValue;
} MicpMuteValueInd;

/*! @brief Contents of the MICP_READ_MUTE_VALUE_CCC_CFM message that is sent by the library,
    as a result of reading of the Mute Client Configuration characteristic on the server.
 */
typedef struct
{
    MicpMessageId     id;
    MicpProfileHandle prflHndl;   /*! MICP profile handle*/
    status_t         status;      /*! Status of the reading attempt*/
    uint16           sizeValue;  /*! Value size*/
    uint8            value[1];   /*! Read value */
} MicpReadMuteValueCccCfm;

/*! @brief Contents of the MICP_READ_MUTE_VALUE_CFM message that is sent by the library,
    as a result of a reading of the Mute characteristic on the server.
 */
typedef struct
{
    MicpMessageId     id;
    MicpProfileHandle prflHndl;
    status_t         status;
    uint8            muteValue;
} MicpReadMuteValueCfm;

/*!
    @brief Initialises the Gatt MICP Library.

    NOTE: This interface need to be invoked for every new gatt connection that wishes to use
    the Gatt MICP library.

    @param appTask           The Task that will receive the messages sent from this immediate alert profile library
    @param clientInitParams  Initialisation parameters
    @param deviceData        If peer device's MICS handles are known. AICS handle is reserved and shall be set to NULL. 
                             Else pass NULL to do Gatt service range discovery and then MICS Client init
    @param includedServices  Reserved and shall be set to FALSE

    NOTE: A MICP_INIT_CFM with MicpStatus code equal to MICP_STATUS_IN_PROGRESS will be received as indication that
          the profile library initialisation started. Once completed MICP_INIT_CFM will be received with a MicpStatus
          that indicates the result of the initialisation.
*/
void MicpInitReq(AppTask appTask,
                const MicpInitData *clientInitParams,
                const MicpHandles *deviceData,
                bool includedServices);

/*!
    @brief When a GATT connection is removed, the application must remove all profile and
    client service instances that were associated with the connection.
    This is the clean up routine as a result of calling the MicpInitReq API.

    @param profileHandle The Profile handle.

    NOTE: A MICP_DESTROY_CFM with MicpStatus code equal to MICP_STATUS_IN_PROGRESS will be received as indication
          that the profile library destroy started. Once completed MICP_DESTROY_CFM will be received with a
          MicpStatus that indicates the result of the destroy.
*/
void MicpDestroyReq(MicpProfileHandle profileHandle);

/*!
    @brief Get the service handles of all the AICS client instances initialised.

    @param profileHandle The Profile handle.
    @param aicsNum       Pointer to the uint16 variable provided by the application in which the library
                          will save the number of AICS Client instances.

    @return Pointer to a ServiceHandle structure containing all the service handles of the AICS Client
            instances initialised.

    NOTE: It's responsibility of the application to free the memory of the returned pointer.
          If there are no AICS Client instances initialised, this function will return NULL and the
          aicsNum will be 0;
*/
ServiceHandle *MicpGetAicsServiceHandlesRequest(MicpProfileHandle profileHandle,
                                                  uint16 *aicsNum);

/*!
    @brief This API is used to write the client characteristic configuration of the Mute
    characteristic on a remote device, to enable notifications with the server.
    An error will be returned if the server does not support notifications.

    @param profileHandle       The Profile handle.
    @param notificationsEnable Set to TRUE to enable notifications on the server, FALSE to disable them.

    NOTE: MICP_NTF_CFM message will be sent to the registered application Task.

*/
void MicpRegisterForNotificationReq(MicpProfileHandle profileHandle,
                                              bool notificationsEnable);

/*!
    @brief This API is used to read the Client Configuration Characteristic of the Mute
           characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A MICP_READ_MUTE_VALUE_CCC_CFM message will be sent to the registered application Task.

*/
void MicpReadMuteValueCccReq(MicpProfileHandle profileHandle);

/*!
    @brief This API is used to read the Mute characteristic.

    @param profileHandle  The Profile handle.

    NOTE: A MICP_READ_MUTE_VALUE_CFM message will be sent to the registered application Task.

*/
void MicpReadMuteValueReq(MicpProfileHandle profileHandle);

/*!
    @brief This API is used to retrieve the Mute characteristic and descriptor handles stored
           by the profile library during discovery procedure.

    @param clntHndl      The service handle for this GATT Client Service.

    @return GattMicsClientDeviceData : The structure containing characteristic and descriptor handles info.
            If the handles are not found or any other error happens in the process, NULL will be returned.

    NOTE: This is not a message passing based API, the handles, if found, will be returned immediately
          to the profile library.

*/
GattMicsClientDeviceData *MicpGetAttributeHandles(MicpProfileHandle profileHandle);

/*!
    @brief This API is used to write the Mute characteristic in order to execute
           the Set Mute operation.

    @param profileHandle  The Profile handle.
    @param muteValue      Value of mute to set

    NOTE: A MICP_SET_MUTE_VALUE_CFM message will be sent to the registered application Task.

*/
void MicpSetMuteValueReq(MicpProfileHandle profileHandle, uint8 muteValue);

#endif /* MICP_H */

