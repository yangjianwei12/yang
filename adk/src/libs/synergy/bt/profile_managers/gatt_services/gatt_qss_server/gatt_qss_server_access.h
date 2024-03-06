/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#ifndef GATT_QSS_SERVER_ACCESS_H_
#define GATT_QSS_SERVER_ACCESS_H_

#include "gatt_qss_server.h"
#include "gatt_qss_server_private.h"
#include "gatt_qss_server_debug.h"

#ifndef CSR_TARGET_PRODUCT_VM
#include "gatt_leaudio_server_db.h"
#else
#include "gatt_qss_server_db.h"
#endif

#define QSS_NOTIFICATION_DISABLED               (0x00)
#define QSS_NOTIFICATION_ENABLED                (0x01)

#define GATT_QSS_SUPPORT_OCTATE_SIZE            (0x01)
#define GATT_QSS_LOSSLESS_AUDIO_OCTET_SIZE      (0x04)
#define GATT_QSS_CLIENT_CONFIG_OCTET_SIZE       (0x02)

/***************************************************************************
NAME
    gattQssServerHandleQssSupportReadAccess

DESCRIPTION
    Handles the GATT_QSS_SERVER_READ_ACCESS message sent to the HANDLE_QSS_SUPPORT.
*/
void gattQssServerHandleQssSupportReadAccess(const GQSSS* gattQssServerInst, 
                          const GATT_MANAGER_SERVER_ACCESS_IND_T*accessInd);

/***************************************************************************
NAME
    gattQssServerHandleUserDescriptionReadAccess

DESCRIPTION
    Handles the GATT_QSS_SERVER_READ_ACCESS message sent to the HANDLE_USER_DESCRIPTION.
*/
void gattQssServerHandleUserDescriptionReadAccess(const GQSSS* gattQssServerInst,
                              const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

/***************************************************************************
NAME
    gattQssServerHandleLosslessAudioReadAccess

DESCRIPTION
    Handles the GATT_QSS_SERVER_READ_ACCESS message sent to the HANDLE_LOSSLESS_AUDIO.
*/
void gattQssServerHandleLosslessAudioReadAccess(const GQSSS* gattQssServerInst,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

/***************************************************************************
NAME
    gattQssServerHandleLosslessAudioClientConfigReadAccess

DESCRIPTION
    Handles the GATT_QSS_SERVER_READ_ACCESS message sent to the 
    HANDLE_LOSSLESS_AUDIO_CLIENT_CONFIG.
*/
void gattQssServerHandleLosslessAudioClientConfigReadAccess(const GQSSS* gattQssServerInst,
                                        const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

/***************************************************************************
NAME
    gattQssServerSendQssSupportAccessRsp

DESCRIPTION
    Send Qss Support access response to the GATT library.
*/
void gattQssServerSendQssSupportAccessRsp(const GQSSS* gattQssServerInst,
                                          uint32 condId,
                                          uint8 qssSupport, 
                                          uint16 result);

/***************************************************************************
NAME
    gattQssServerSendUserDescriptionAccessRsp

DESCRIPTION
    Send User Description access response to the GATT library.
*/
void gattQssServerSendUserDescriptionAccessRsp(const GQSSS* gattQssServerInst,
                                               uint32 condId,
                                               uint16 size,
                                               uint8* userDescription,
                                               uint16 result);

/***************************************************************************
NAME
    gattQssServerSendLosslessAudioAccessRsp

DESCRIPTION
    Send Lossless Audio access response to the GATT library.
*/
void gattQssServerSendLosslessAudioAccessRsp(const GQSSS* gattQssServerInst,
                                             uint32 condId,
                                             uint32 losslessAudio,
                                             uint16 result);

/***************************************************************************
NAME
    gattQssServerSendLosslessAudioConfigAccessRsp

DESCRIPTION
    Send Lossless Audio client config access response to the GATT library.
*/
void gattQssServerSendLosslessAudioConfigAccessRsp(const GQSSS* gattQssServerInst,
                                                   uint32 condId,
                                                   uint16 config);

/***************************************************************************
NAME
    gattQssServerHandleWriteAccessIndication

DESCRIPTION
    Handles the CsrBtGattDbAccessWriteInd message that was sent to the Qss library.
*/
void gattQssServerHandleWriteAccessIndication(const GQSSS* gattQssServerInst,
                          const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

/***************************************************************************
NAME
    gattQssServerHandleReadAccessIndication

DESCRIPTION
    Handles the CsrBtGattDbAccessReadInd message that was sent to the Qss library.
*/
void gattQssServerHandleReadAccessIndication(const GQSSS *gattQssServerInst,
                         const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd);

#endif
