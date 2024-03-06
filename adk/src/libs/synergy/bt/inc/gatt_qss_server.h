/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:     $Revision: #2 $
******************************************************************************/

/*!
@file   gatt_qss_server.h
@brief  Header File for the GATT Qualcomm Snapdragon Sound Service library

        This file provide the documentation for the GATT Qualcomm Snapdragon Sound Service library.
        The public API for the GATT Qualcomm Snapdragon Sound Service are defined.
        API (library name: gatt_qss_server).
*/

#ifndef GATT_QSS_SERVER_H
#define GATT_QSS_SERVER_H

#include "service_handle.h"
#include "csr_bt_tasks.h"
#include "csr_bt_gatt_lib.h"

#define QSS_NOT_SUPPORTED                                       (0x00)
#define QSS_SUPPORTED                                           (0x01)

#define LOSSLESS_MODE_NOT_SUPPORTED                             (0x00)
#define LOSSLESS_MODE_SUPPORTED                                 (0x01)


/* LOSSSLESS FORMATS */
#define LOSSLESS_FORMAT_UNKNOWN                                 (0x00)

#define LOSSLESS_44_1_kHz_16_BITS                               (0x01)
#define LOSSLESS_44_1_kHz_24_BITS                               (0x02)
#define LOSSLESS_44_1_kHz_32_BITS                               (0x03)
#define LOSSLESS_44_1_kHz_48_BITS                               (0x04)

#define LOSSLESS_48_0_kHz_16_BITS                               (0x05)
#define LOSSLESS_48_0_kHz_24_BITS                               (0x06)
#define LOSSLESS_48_0_kHz_32_BITS                               (0x07)
#define LOSSLESS_48_0_kHz_48_BITS                               (0x08)

#define LOSSLESS_96_0_kHz_16_BITS                               (0x09)
#define LOSSLESS_96_0_kHz_24_BITS                               (0x0a)
#define LOSSLESS_96_0_kHz_32_BITS                               (0x0b)
#define LOSSLESS_96_0_kHz_48_BITS                               (0x0c)

#define LOSSLESS_192_0_kHz_16_BITS                              (0x0d)
#define LOSSLESS_192_0_kHz_24_BITS                              (0x0e)
#define LOSSLESS_192_0_kHz_32_BITS                              (0x0f)
#define LOSSLESS_192_0_kHz_48_BITS                              (0x10)

/* Primitive of messages an application task can receive from the QSS server library. */
typedef uint16 GattQssServerMessageInd;

#define GATT_QSS_SERVER_READ_QSS_SUPPORT_IND                    (0x01)
#define GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND               (0x02)
#define GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND                 (0x03)
#define GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_CLIENT_CONFIG_IND   (0x04)
#define GATT_QSS_SERVER_WRITE_LOSSLESS_AUDIO_CLIENT_CONFIG_IND  (0X05)

/*! @brief  Contents of GATT_QSS_SERVER_READ_IND indication message. This indication message is sent by the library 
            to application to read the characteristics value.
            Following indication message is send to read respective characteristics value
            GATT_QSS_SEVER_READ_QSS_SUPPORT_IND         - QSS Support,
            GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND   - User Descreiption 
            GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND     - Lossless Audio. 
            To support ATT_READ_BLOB_REQ, offset value is passed to application by library.
 */
typedef struct __GATT_QSS_SERVER_READ_IND__
{
    GattQssServerMessageInd ind;
    uint32 cid;
    uint16 offset;
} GATT_QSS_SERVER_READ_IND;

/*! @brief  Contents of GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND indication message.
            This indication message is sent by the library to application, to read the client config of characteristic's. 
            GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_CLIENT_CONFIG_IND indication message to read lossless audio client characterisitic value.
*/
typedef struct __GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND__
{
    GattQssServerMessageInd ind;
    uint32 cid;
}GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND;

/*! @brief  Contents of GATT_QSS_SERVER_WRITE_LOSSLESS_AUDIO_CLIENT_CONFIG_IND write client config indication message.
            This indication message is sent by the library to application, to write the client config of charateristic's
*/
typedef struct __GATT_QSS_SERVER_WRITE_CLIENT_CONFIG_IND__
{
    GattQssServerMessageInd ind;
    uint32 cid;
    uint16 clientConfig;
} GATT_QSS_SERVER_WRITE_CLIENT_CONFIG_IND;

/*!
    @brief Initialises the Qualcomm Snapdragon Sound (QSS) Service Library Instance.
    
    @param appTask      The appTask will receive the messages sent from this QSS library.
    @param startHandle  The start handle of service.
    @param endHandle    The end handle of service.

    @return ServiceHandle Returns 16-bit ServiceHandle to application on successfull registration of service.
                          To respond for any request application should use the ServiceHandle value. 
                          Returns 0 on unsuccessfull registration.
    
*/
ServiceHandle GattQssServerInit(AppTask appTask, uint16 startHandle, uint16 endHandle);

/*!
    @brief This API is used to send QSS Support response to remote device on receiving GATT_QSS_SERVER_READ_QSS_SUPPORT_IND indication message.

    @param srvHndl       The Service handle of the QSS server instance.
    @param cid           Connection identifier from the GATT_QSS_SERVER_READ_QSS_SUPPORT_IND indication message.
    @param isQssSupport  Boolean value to indicate QSS support. 1 - QSS supported, 0 - QSS not supported.
    
    @return return TRUE if success, FALSE otherwise
*/
bool GattQssServerReadQssSupportResponse(ServiceHandle srvHndl, ConnectionId cid, uint8 isQssSupport);

/*!
    @brief This API is used to send QSS user description response to remote device on receiving GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND indication message.

    @param srvHndl     Service handle shared with application to respond for GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND indication received.
    @param cid         Connection identifier from the GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND indication message.
    @param length      Total Length of user description pointed by description pointer.
    @param description Pointer to the user desicription string. The user description string should be offset as per the offset value recived in the
                       GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND indication message.

    @return return TRUE if success, FALSE otherwise
*/
bool GattQssServerReadUserDescriptionResponse(ServiceHandle srvHndl, ConnectionId cid, uint16 length, uint8* description);


/*!
    @brief This API is used to send Lossless audio response to remote device on reciving GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND indication message.

    @param srvHndl       The Service handle of the QSS server instance.
    @param cid           Connection identifier from the GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND indication message.
    @param losslessAudio The lossless audio data is 4 octet value. The format of losslessAudio data is [LOSSLESS_MODE, LOSSLESS_FORMAT, BITRATE]. 
                         First octet - 0x00 for LOSSLESS Mode not supported and 0x01 for LOSSLESS Mode supported.
                         Second octet - LOSSLESS FORMAT.
                         Third and forth octet - bitrate in kbps.
                         
    @return return TRUE if success, FALSE otherwise
*/
bool GattQssServerReadLosslessAudioResponse(ServiceHandle srvHndl, ConnectionId cid, uint32 losslessAudio);

/*!
    @brief This API is used to send the client config response to a remote device on receiving GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND indication message.

    @param srvHndl      The Service handle of the QSS server instance.
    @param cid          Connection identifier of the remote device.
    @param clientConfig The client configuration of the charaterisitic.

    @return return TRUE if success, FALSE otherwise.
*/
bool GattQssServerReadClientConfigResponse(ServiceHandle srvHndl, ConnectionId cid, uint16 clientConfig);

/*!
    @brief This API is used to send Lossless Audio notification to connected remote device when notification is enabled by respective remote device.

    @param srvHndl       The Service handle of the QSS server insatance.
    @param cid           Connection identifier from the application to send the notification.
    @param losslessAudio The lossless audio data is 4 octet value. The format of losslessAudio data is [LOSSLESS_MODE, LOSSLESS_FORMAT, BITRATE]. 
                         First octet - 0x00 for LOSSLESS Mode not supported and 0x01 for LOSSLESS Mode supported.
                         Second octet - LOSSLESS FORMAT.
                         Third and forth octet - bitrate in kbps.

    @return return TRUE if success, FALSE otherwise.
*/
bool GattQssServerSendLosslessAudioNotification(ServiceHandle srvHndl, ConnectionId cid, uint32 losslessAudio);

#endif /* GATT_QSS_SERVER_H */