#ifndef CSR_BT_AVRCP_LIB_H__
#define CSR_BT_AVRCP_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "csr_bt_avrcp_prim.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_tasks.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Common put_message function to reduce code size */
void CsrBtAvrcpMsgTransport(void* msg);

/** \file csr_bt_avrcp_lib.h */
void CsrBtCopyBackwards(CsrBtAvrcpUid dst, CsrBtAvrcpUid src, CsrUint16 len);

#define CSR_BT_AVRCP_UID_COPY(dest, src)           (CsrBtCopyBackwards((dest), (src), sizeof(CsrBtAvrcpUid)))
#define CSR_BT_AVRCP_FEATURE_MASK_RESET(mask)      (CsrMemSet(&(mask), 0, sizeof(CsrBtAvrcpMpFeatureMask))
#define CSR_BT_AVRCP_FEATURE_MASK_COPY(dest, src)  (SynMemCpyS((dest), sizeof(CsrBtAvrcpMpFeatureMask), (src), sizeof(CsrBtAvrcpMpFeatureMask)))

void CsrBtAvrcpConfigRoleNoSupport(CsrBtAvrcpRoleDetails *details);

void CsrBtAvrcpConfigRoleSupport(CsrBtAvrcpRoleDetails          *details,
                                 CsrBtAvrcpConfigRoleMask       roleConfig,
                                 CsrBtAvrcpConfigSrVersion      srAvrcpVersion,
                                 CsrBtAvrcpConfigSrFeatureMask  srFeatures,
                                 CsrCharString                    *providerName,
                                 CsrCharString                    *serviceName);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpConfigReqSend
 *
 *  DESCRIPTION
 *      Before the AVRCP module can be used, it should be configured,
 *      which involves registration of one or two service records depending on the roles to support.
 *      AVRCP responds the application back through CSR_BT_AVRCP_CONFIG_CFM message with the
 *      result.
 *
 *  PARAMETERS
 *      phandle:            application handle
 *      globalConfig:       Global configuration for AVRCP,
 *                          set to CSR_BT_AVRCP_CONFIG_GLOBAL_STANDARD for now
 *      mtu:                Maximum transmission unit to announce to a remote device
 *                          during connection establishment (L2CAP configuration)
 *      tgConfig:           Features of the target, initialize parameter with
 *                          CsrBtAvrcpConfigRoleNoSupport() or CsrBtAvrcpConfigRoleSupport()
 *      ctConfig:           Same as tgConfig but for Controller role
 *      uidCount:           Start value of the "uid counter". This is only relevant for TG devices.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpConfigReqSend(_phandle, _globalConfig, _mtu, _tgConfig, _ctConfig, _uidCount) { \
        CsrBtAvrcpConfigReq *msg = (CsrBtAvrcpConfigReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CONFIG_REQ;                   \
        msg->phandle       = _phandle;                                  \
        msg->globalConfig  = _globalConfig;                             \
        msg->tgDetails     = _tgConfig;                                 \
        msg->ctDetails     = _ctConfig;                                 \
        msg->mtu           = _mtu;                                      \
        msg->uidCount      = _uidCount;                                 \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpActivateReqSend
 *
 *  DESCRIPTION
 *      This signal is used to activate a service and make it accessible from a
 *      remote device. AVRCP responds the application back through
 *      CSR_BT_AVRCP_ACTIVATE_CFM message with the appropriate result.
 *
 *  PARAMETERS
 *      maxIncoming:        Maximum number of simultaneous connections can be established
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpActivateReqSend(_maxIncoming) {                       \
        CsrBtAvrcpActivateReq *msg = (CsrBtAvrcpActivateReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_ACTIVATE_REQ;             \
        msg->maxIncoming       = _maxIncoming;                          \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef INSTALL_AVRCP_DEACTIVATE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpDeactivateReqSend
 *
 *  DESCRIPTION
 *      This signal is used to deactivate a service and make in inaccessible from
 *      other devices. AVRCP responds the application back through
 *      CSR_BT_AVRCP_DEACTIVATE_CFM message with the appropriate result.
 *
 *  PARAMETERS
 *      NA
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpDeactivateReqSend() {                                 \
        CsrBtAvrcpDeactivateReq *msg = (CsrBtAvrcpDeactivateReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_DEACTIVATE_REQ;           \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_DEACTIVATE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpConnectReqSend
 *
 *  DESCRIPTION
 *      Request to initiate an outgoing connection with a remote device for the
 *      specified Bluetooth device address. AVRCP responds the application back
 *      through CSR_BT_AVRCP_CONNECT_CFM with the connection attempt result.
 *
 *  PARAMETERS
 *      deviceAddr:         address of device to connect to
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpConnectReqSend(_deviceAddr) {                 \
        CsrBtAvrcpConnectReq *msg = (CsrBtAvrcpConnectReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_CONNECT_REQ;      \
        msg->deviceAddr        = _deviceAddr;                   \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef INSTALL_AVRCP_CANCEL_CONNECT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCancelConnectReqSend
 *
 *  DESCRIPTION
 *      Request to cancel a prior request for outgoing connection establishment.
 *      AVRCP responds the application back through CSR_BT_AVRCP_CONNECT_CFM
 *      with the appropriate result.
 *
 *  PARAMETERS
 *      deviceAddr:         address of device to cancel the connection to
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCancelConnectReqSend(_deviceAddr) {                   \
        CsrBtAvrcpCancelConnectReq *msg = (CsrBtAvrcpCancelConnectReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_CANCEL_CONNECT_REQ;       \
        msg->deviceAddr        = _deviceAddr;                           \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_CANCEL_CONNECT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpDisconnectReqSend
 *
 *  DESCRIPTION
 *      Request to disconnect the connection for the specified connection id.
 *      AVRCP responds the application back through CSR_BT_AVRCP_DISCONNECT_IND
 *      message with the appropriate result.
 *
 *  PARAMETERS
 *      connectionId        connection ID received through CSR_BT_AVRCP_CONNECT_CFM/IND
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpDisconnectReqSend(_connectionId) {                    \
        CsrBtAvrcpDisconnectReq *msg = (CsrBtAvrcpDisconnectReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_DISCONNECT_REQ;           \
        msg->connectionId      = _connectionId;                         \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpSecurityInReqSend
 *      CsrBtAvrcpSecurityOutReqSend
 *
 *  DESCRIPTION
 *      Set the default security settings for new incoming/outgoing connections.
 *      AVRCP responds the application back through CSR_BT_AVRCP_SECURITY_IN_CFM/
 *      CSR_BT_AVRCP_SECURITY_OUT_CFM with the appropriate result.
 *
 *  PARAMETERS
        appHandle:      application handle
 *      secLevel:       Minimum incoming/outgoing security level requested by the application.
 *                      The application must specify one of the following values:
 *                      CSR_BT_SEC_DEFAULT:   Uses the default security settings.
 *                      CSR_BT_SEC_MANDATORY: Uses the mandatory security settings.
 *                      CSR_BT_SEC_SPECIFY:   Specifies new security settings.
 *                      If CSR_BT_SEC_SPECIFY is set, the following values can be OR'ed additionally,
 *                      i.e., combinations of these values can be used:
 *                      CSR_BT_SEC_AUTHORISATION:  Requires authorisation.
 *                      CSR_BT_SEC_AUTHENTICATION: Requires authentication.
 *                      CSR_BT_SEC_SEC_ENCRYPTION: Requires encryption (implies authentication).
 *                      CSR_BT_SEC_MITM:           Requires MITM protection (implies encryption).
 *      config:         Specifies which channel (control or browsing) to update the security for,
 *                      use one of the following values:
 *                      CSR_BT_AVRCP_SECURITY_CONFIG_CONTROL
 *                      CSR_BT_AVRCP_SECURITY_CONFIG_BROWSING
 *                      CSR_BT_AVRCP_SECURITY_CONFIG_COVER_ART
 *                      CSR_BT_AVRCP_SECURITY_CONFIG_ALL
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpSecurityInReqSend(_appHandle, _secLevel, _config) {   \
        CsrBtAvrcpSecurityInReq *msg = (CsrBtAvrcpSecurityInReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type = CSR_BT_AVRCP_SECURITY_IN_REQ;                       \
        msg->phandle = _appHandle;                                      \
        msg->secLevel = _secLevel;                                      \
        msg->config = _config;                                          \
        CsrBtAvrcpMsgTransport(msg);}

#define CsrBtAvrcpSecurityOutReqSend(_appHandle, _secLevel, _config) {  \
        CsrBtAvrcpSecurityOutReq *msg = (CsrBtAvrcpSecurityOutReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type = CSR_BT_AVRCP_SECURITY_OUT_REQ;                      \
        msg->phandle = _appHandle;                                      \
        msg->secLevel = _secLevel;                                      \
        msg->config = _config;                                          \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

#define CSR_BT_AVRCP_LIB_GFI_HEADER_OFFSET             (11)
#define CSR_BT_AVRCP_LIB_GFI_HEADER_SIZE               (8)
#define CSR_BT_AVRCP_LIB_GFI_ITEM_TYPE_INDEX           (0)
#define CSR_BT_AVRCP_LIB_GFI_ITEM_TYPE_SIZE            (1)
#define CSR_BT_AVRCP_LIB_GFI_ITEM_LENGTH_INDEX         (1)
#define CSR_BT_AVRCP_LIB_GFI_ITEM_LENGTH_SIZE          (2)
#define CSR_BT_AVRCP_LIB_GFI_ITEM_HEADER_SIZE          (CSR_BT_AVRCP_LIB_GFI_ITEM_TYPE_SIZE + CSR_BT_AVRCP_LIB_GFI_ITEM_LENGTH_SIZE)

/* The below defines are for Media Player Item as define by spec.
 * It can be used by GFI related library functions
 */
#define CSR_BT_AVRCP_LIB_GFI_MP_PLAYER_ID_INDEX        (3)
#define CSR_BT_AVRCP_LIB_GFI_MP_MAJOR_TYPE_INDEX       (5)
#define CSR_BT_AVRCP_LIB_GFI_MP_SUB_TYPE_INDEX         (6)
#define CSR_BT_AVRCP_LIB_GFI_MP_PLAY_STATUS_INDEX      (10)
#define CSR_BT_AVRCP_LIB_GFI_MP_FEATURE_MASK_INDEX     (11)
#define CSR_BT_AVRCP_LIB_GFI_MP_CHARSET_INDEX          (27)
#define CSR_BT_AVRCP_LIB_GFI_MP_NAME_LEN_INDEX         (29)
#define CSR_BT_AVRCP_LIB_GFI_MP_NAME_INDEX             (31)
#define CSR_BT_AVRCP_LIB_GFI_MP_PART_SIZE              (31) /* + MP name */

/* The below defines are for Folder Item as define by spec.
 * It can be used by GFI related library functions
 */
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_UID_INDEX          (3)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_TYPE_INDEX         (11)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_PLAYABLE_INDEX     (12)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_CHARSET_INDEX      (13)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_NAME_LEN_INDEX     (15)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_NAME_INDEX         (17)
#define CSR_BT_AVRCP_LIB_GFI_FOLDER_PART_SIZE          (17) /* + folder name*/

/* The below defines are for Media Element Item as define by spec.
 * It can be used by GFI related library functions
 */
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_UID_INDEX           (3)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_TYPE_INDEX          (11)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_CHARSET_INDEX       (12)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_NAME_LEN_INDEX      (14)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_NAME_INDEX          (16)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_COUNT_SIZE      (1)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_PART_SIZE           (16) /* media name and attributes */

/* The below defines are for Attribute Value Entry as define by spec.
 * It can be used by GFI and GIA related
 */
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_PART_SIZE       (8)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_ID_INDEX        (0)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_CHARSET_INDEX   (4)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_VAL_LEN_INDEX   (6)
#define CSR_BT_AVRCP_LIB_GFI_MEDIA_ATT_VAL_INDEX       (8)

/** Get Items Attributes Header related */
#define CSR_BT_AVRCP_LIB_GIA_HEADER_OFFSET             (8)


#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPassThroughReqSend
 *
 *  DESCRIPTION
 *      Request to send a pass-through control command to remote Target device
 *      as specified by the connection id. AVRCP responds the application back
 *      through CSR_BT_AVRCP_CT_PASS_THROUGH_CFM with the appropriate result.
 *
 *  PARAMETERS
 *     phandle:     application phandle
 *     connId:      connection identifier
 *     opId:        Pass through operation identifier.
 *                  Refer to the defines prefixed with AVRCP_PT_OP_ID_ in csr_bt_avrcp_prim.h.
 *     state:       state of the button.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPassThroughReqSend(_phandle, _connId, _opId, _state) { \
        CsrBtAvrcpCtPassThroughReq *msg = (CsrBtAvrcpCtPassThroughReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PASS_THROUGH_REQ;          \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->operationId   = _opId;                                     \
        msg->state         = _state;                                    \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER
#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtGetPlayStatusReqSend
 *
 *  DESCRIPTION
 *      Controller requests to get the play status of remote Target device.
 *      Application is notified with a CSR_BT_AVRCP_CT_GET_PLAY_STATUS_CFM message
 *      with the appropriate result.
 *
 *  PARAMETERS
 *      phandle:     application phandle
 *      connId:      connection identifier
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtGetPlayStatusReqSend(_phandle, _connId) {           \
        CsrBtAvrcpCtGetPlayStatusReq *msg = (CsrBtAvrcpCtGetPlayStatusReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_GET_PLAY_STATUS_REQ;       \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtNotiRegisterReqSend
 *
 *  DESCRIPTION
 *      Controller requests to register the notifications at Target device.
 *      Target notifies the interim(current) values to Controller for all the 
 *      requested notifications in response.
 *      AVRCP notifies the interim values back to application back through
 *      multiple CSR_BT_AVRCP_CT_NOTI_XXX_INDs and sends
 *      CSR_BT_AVRCP_CT_NOTI_REGISTER_CFM message at the end.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      notiMask:           mask for the defines to specify which all notifications to be registered.
 *                          Refer to CsrBtAvrcpNotiMask in csr_bt_avrcp_prim.h for the notifications.
 *      playbackInterval:   Interval in seconds at which the remote target should
 *                          send a notification with the current playback position.
 *      configMask:         Defines used to configure the notifications, which can either be persistent or not.
 *                          If _STANDARD (persistent) is used, the AVRCP profile handles
 *                          renewing the notification registration if the status is changed.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtNotiRegisterReqSend(_phandle, _connId, _notiMask, _playbackInterval, _configMask) { \
        CsrBtAvrcpCtNotiRegisterReq *msg = (CsrBtAvrcpCtNotiRegisterReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_NOTI_REGISTER_REQ;         \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->notiMask      = _notiMask;                                 \
        msg->config        = _configMask;                               \
        msg->playbackInterval = _playbackInterval;                      \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
/* CT PAS helper functions (for parsing attribute and value text) */
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_OFFSET          (13)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_MIN_HEADER_SIZE (5)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_MIN_PART_SIZE   (4)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_NUM_INDEX       (0)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_ID_INDEX        (1)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_CS_INDEX        (2)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_STR_LEN_INDEX   (4)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_STR_INDEX       (5)

#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_OFFSET          (13)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_MIN_HEADER_SIZE (5)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_MIN_PART_SIZE   (4)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_NUM_INDEX       (0)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_ID_INDEX        (1)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_CS_INDEX        (2)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_STR_LEN_INDEX   (4)
#define CSR_BT_AVRCP_CT_LIB_PAS_TXT_VAL_STR_INDEX       (5)

CsrBool CsrBtAvrcpCtLibPasAttribTxtGet(CsrUint16 pasLen, CsrUint8 *pas, CsrUint16 *index, CsrBtAvrcpPasAttId *attId, CsrBtAvrcpCharSet *charset, CsrUint8 *attTxtLen, CsrUint8 **attTxt);
CsrBool CsrBtAvrcpCtLibPasValueTxtGet(CsrUint16 pasLen, CsrUint8 *pas, CsrUint16 *index, CsrBtAvrcpPasValId *valId, CsrBtAvrcpCharSet *charset, CsrUint8 *valTxtLen, CsrUint8 **valTxt);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasAttIdReqSend
 *
 *  DESCRIPTION
 *      Request to get the list of attributes supported by a remote Target device. Application
 *      is notified through CSR_BT_AVRCP_CT_PAS_VAL_ID_CFM with the appropriate result.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasAttIdReqSend(_phandle, _connId) {                \
        CsrBtAvrcpCtPasAttIdReq *msg = (CsrBtAvrcpCtPasAttIdReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_ATT_ID_REQ;            \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasAttTxtReqSend
 *
 *  DESCRIPTION
 *      Request to get the text for the Player Application Settings attributes from
 *      a remote target. AVRCP informs the result through CSR_BT_AVRCP_CT_PAS_ATT_TXT_INDs
 *      which application responds through CsrBtAvrcpCtPasAttTxtResSend.
 *      AVRCP sends CSR_BT_AVRCP_CT_PAS_ATT_TXT_CFM message on the completion
 *      of the procedure.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      attribIdCount:      number of values for which to retrieve the text
 *      attribId:           attribute identifier for which attribute text to be retrieved
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasAttTxtReqSend(_phandle, _connId, _attribIdCount, _attribId) { \
        CsrBtAvrcpCtPasAttTxtReq *msg = (CsrBtAvrcpCtPasAttTxtReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_ATT_TXT_REQ;           \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->attIdCount    = _attribIdCount;                               \
        msg->attId         = _attribId;                                    \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasAttTxtResSend
 *
 *  DESCRIPTION
 *      The PAS information is transferred using AV/C packets and as the maximum
 *      size is limited to 512 bytes, fragmentation can occur.
 *      In case of fragmentation, a CSR_BT_AVRCP_CT_PAS_ATT_TXT_IND message
 *      is sent to the application, which should choose either to request the next
 *      remaining fragment or to abort the procedure by sending a
 *      CSR_BT_AVRCP_CT_PAS_ATT_TXT_RES message to the profile.
 *      Application uses this API to respond for each CSR_BT_AVRCP_CT_PAS_ATT_TXT_IND.
 *
 *    PARAMETERS
 *      connId:             connection identifier
 *      proceed:            Set to TRUE to get next fragment or FALSE to abort the procedure
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasAttTxtResSend(_connId, _proceed) {               \
        CsrBtAvrcpCtPasAttTxtRes *msg = (CsrBtAvrcpCtPasAttTxtRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_ATT_TXT_RES;           \
        msg->connectionId  = _connId;                                   \
        msg->proceed       = _proceed;                                  \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasValIdReqSend
 *
 *  DESCRIPTION
 *      Request to get Player application settings value IDs for the specified
 *      attribute from a remote target.
 *      AVRCP responds the application back through CSR_BT_AVRCP_CT_PAS_VAL_ID_CFM
 *      with the appropriate result.
 *
 *    PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      attribId:           attribute identifier for which values ID to be retrieved
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasValIdReqSend(_phandle, _connId, _attribId) {     \
        CsrBtAvrcpCtPasValIdReq *msg = (CsrBtAvrcpCtPasValIdReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_VAL_ID_REQ;            \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->attId         = _attribId;                                    \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasValTxtReqSend
 *
 *  DESCRIPTION
 *      Request to get the value text for the specified Player Application Settings
 *      attribute from a remote target. AVRCP informs the result through 
 *      CSR_BT_AVRCP_CT_PAS_VAL_TXT_INDs which application responds through
 *      CsrBtAvrcpCtPasValTxtResSend. AVRCP sends CSR_BT_AVRCP_CT_PAS_VAL_TXT_CFM
 *      message on the completion of the procedure.
 *
 *    PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      attribId:           attribute identifier for which values text to be retrieved
 *      validCount:         Number of values for which to retrieve the text
 *      valId:              Pointer to the specified number of value IDs
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasValTxtReqSend(_phandle, _connId, _attribId, _valIdCount, _valId) { \
        CsrBtAvrcpCtPasValTxtReq *msg = (CsrBtAvrcpCtPasValTxtReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_VAL_TXT_REQ;           \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->attId      = _attribId;                                    \
        msg->valIdCount    = _valIdCount;                               \
        msg->valId         = _valId;                                    \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasValTxtResSend
 *
 *  DESCRIPTION
 *      The Player Application Settings information is transferred using AV/C
 *      packets and as the maximum size is limited to 512 bytes, fragmentation can occur.
 *      In case of fragmentation, a CSR_BT_AVRCP_CT_PAS_VAL_TXT_IND message
 *      is sent to the application, which should choose either to request the next
 *      remaining fragment or to abort the procedure by sending a
 *      CSR_BT_AVRCP_CT_PAS_VAL_TXT_RES message to the profile.
 *      Application uses this API to respond for each CSR_BT_AVRCP_CT_PAS_VAL_TXT_IND.
 *
 *  PARAMETERS
 *      connId:             connection identifier
 *      proceed:            Set to TRUE to get next fragment or FALSE to abort the procedure
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasValTxtResSend(_connId, _proceed) {               \
        CsrBtAvrcpCtPasValTxtRes *msg = (CsrBtAvrcpCtPasValTxtRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_VAL_TXT_RES;           \
        msg->connectionId  = _connId;                                   \
        msg->proceed       = _proceed;                                  \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasCurrentReqSend
 *
 *  DESCRIPTION
 *      Get current Player application settings attributes values from remote
 *      Target device. AVRCP responds the application back through
 *      CSR_BT_AVRCP_CT_PAS_CURRENT_CFM message with the appropriate result.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      attribIdCount:      Number of attributes to retrieve the current value
 *      attribId:           Pointer to the specified number of attributes
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasCurrentReqSend(_phandle, _connId, _attribIdCount, _attribId) { \
        CsrBtAvrcpCtPasCurrentReq *msg = (CsrBtAvrcpCtPasCurrentReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_CURRENT_REQ;           \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->attIdCount    = _attribIdCount;                            \
        msg->attId         = _attribId;                                 \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPasSetReqSend
 *
 *  DESCRIPTION
 *      Request to change the values for specified player application settings
 *      attributes. AVRCP responds the application back with CSR_BT_AVRCP_CT_PAS_SET_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      attValPairCount:    Number of attribute/value pairs
 *      attValPair:         Pointer to number of attribute/value pairs
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPasSetReqSend(_phandle, _connId, _attValPairCount, _attValPair) { \
        CsrBtAvrcpCtPasSetReq *msg = (CsrBtAvrcpCtPasSetReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PAS_SET_REQ;               \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->attValPair    = _attValPair;                               \
        msg->attValPairCount = _attValPairCount;                        \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtInformBatteryStatusReqSend
 *
 *  DESCRIPTION
 *      Inform the battery status to the media player on the Target device.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_INFORM_BATTERY_STATUS_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      connId:             connection identifier
 *      batStatus:          battery status. See "CsrBtAvrcpBatteryStatus" for the
 *                          list of possible values in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtInformBatteryStatusReqSend(_phandle, _connId, _batStatus) { \
        CsrBtAvrcpCtInformBatteryStatusReq *msg = (CsrBtAvrcpCtInformBatteryStatusReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_INFORM_BATTERY_STATUS_REQ; \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->batStatus     = _batStatus;                                \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtInformDispCharSetReqSend
 *
 *  DESCRIPTION
 *      Inform a remote Target device of which character sets the controller supports.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_INFORM_DISP_CHARSET_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      charsetCount:   Displayable character set count
 *      charset:        Displayable character set
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtInformDispCharSetReqSend(_phandle, _connId, _charsetCount, _charset) { \
        CsrBtAvrcpCtInformDispCharsetReq *msg = (CsrBtAvrcpCtInformDispCharsetReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_INFORM_DISP_CHARSET_REQ;   \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->charsetCount  = _charsetCount;                             \
        msg->charset       = _charset;                                  \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtSetAddressedPlayerReqSend
 *
 *  DESCRIPTION
 *      Set the "Addressed Player" to Target device. It specifies which media player
 *      on a target device should handle certain media player related commands.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_SET_ADDRESSED_PLAYER_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      playerId:       Unique ID of the media player the controller
 *                      requests to set as the addressed player
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtSetAddressedPlayerReqSend(_phandle, _connId, _playerId) { \
        CsrBtAvrcpCtSetAddressedPlayerReq *msg = (CsrBtAvrcpCtSetAddressedPlayerReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_CT_SET_ADDRESSED_PLAYER_REQ; \
        msg->phandle           = _phandle;                              \
        msg->connectionId      = _connId;                               \
        msg->playerId          = _playerId;                             \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtSetBrowsedPlayerReqSend
 *
 *  DESCRIPTION
 *      Controller sets the "Browsed Player" on the Target device which would
 *      handle browsing commands issued from the controller.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_SET_BROWSED_PLAYER_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      playerId:       Unique ID of the media player the controller
 *                      requests to set as the browsed player
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtSetBrowsedPlayerReqSend(_phandle, _connId, _playerId) { \
        CsrBtAvrcpCtSetBrowsedPlayerReq *msg = (CsrBtAvrcpCtSetBrowsedPlayerReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_CT_SET_BROWSED_PLAYER_REQ; \
        msg->phandle           = _phandle;                              \
        msg->connectionId      = _connId;                               \
        msg->playerId          = _playerId;                             \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtGetFolderItemsReqSend
 *
 *  DESCRIPTION
 *      Request to get the folder items from the browsed media player on target.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_GET_FOLDER_ITEMS_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      scope:          scope. Refer to CsrBtAvrcpScope for defined scopes.
 *      startItem:      Index of the first item to get starting from 0
 *      endItem:        Index of the last item to get starting from 0
 *      attributeMask:  Bitmask combined of different attribute flags.
 *                      Refer to CsrBtAvrcpItemAttMask in csr_bt_avrcp_prim.h
 *                      for defined attribute masks.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtGetFolderItemsReqSend(_phandle, _connId, _scope, _startItem, _endItem, _attributeMask) { \
        CsrBtAvrcpCtGetFolderItemsReq *msg = (CsrBtAvrcpCtGetFolderItemsReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_GET_FOLDER_ITEMS_REQ;      \
        msg->connectionId  = _connId;                                   \
        msg->phandle       = _phandle;                                  \
        msg->scope         = _scope;                                    \
        msg->startItem     = _startItem;                                \
        msg->endItem       = _endItem;                                  \
        msg->attributeMask = _attributeMask;                            \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      Helper functions
 *
 *  DESCRIPTION
 *
 *
 *    PARAMETERS
 *
 *----------------------------------------------------------------------------*/

CsrBool CsrBtAvrcpCtLibGfiNextGet(CsrUint16 *index,
                                  CsrUint16 itemsLen,
                                  CsrUint8 *items,
                                  CsrBtAvrcpItemType *itemType);

CsrBool CsrBtAvrcpCtLibGfiMpGet(CsrUint16 *index,
                                CsrUint16 itemsLen,
                                CsrUint8 *items,
                                CsrUint16 *playerId,
                                CsrBtAvrcpFolderType *majorType,
                                CsrBtAvrcpMpTypeSub *subType,
                                CsrBtAvrcpPlaybackStatus *playbackStatus,
                                CsrBtAvrcpMpFeatureMask *featureMask,
                                CsrBtAvrcpCharSet *charset,
                                CsrUint16 *playerNameLen,
                                CsrUint8 **playerName);

CsrBool CsrBtAvrcpCtLibGfiFolderGet(CsrUint16 *index,
                                    CsrUint16 itemsLen,
                                    CsrUint8 *items,
                                    CsrBtAvrcpUid *folderUid,
                                    CsrBtAvrcpFolderType *folderType,
                                    CsrBtAvrcpFolderPlayableType *playableType,
                                    CsrBtAvrcpCharSet *charset,
                                    CsrUint16 *folderNameLen,
                                    CsrUint8 **folderName);

CsrBool CsrBtAvrcpCtLibGfiMediaGet(CsrUint16 *index,
                                   CsrUint16 itemsLen,
                                   CsrUint8 *items,
                                   CsrBtAvrcpUid *mediaUid,
                                   CsrBtAvrcpMediaType *mediaType,
                                   CsrBtAvrcpCharSet *charset,
                                   CsrUint16 *mediaNameLen,
                                   CsrUint8 **mediaName,
                                   CsrUint8 *attributeCount);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (INSTALL_AVRCP_METADATA_ATTRIBUTES)
CsrBool CsrBtAvrcpCtLibGfiMediaAttributeNextGet(CsrUint16 *index,
                                                CsrUint16 *attIndex,
                                                CsrUint16 itemsLen,
                                                CsrUint8 *items,
                                                CsrBtAvrcpItemMediaAttributeId *attribId);

CsrBool CsrBtAvrcpCtLibGfiMediaAttributeGet(CsrUint16 maxData,
                                            CsrUint16 *attIndex,
                                            CsrUint16 itemsLen,
                                            CsrUint8 *items,
                                            CsrBtAvrcpItemMediaAttributeId *attribId,
                                            CsrBtAvrcpCharSet *charset,
                                            CsrUint16 *attLen,
                                            CsrUint8 **att);
#endif

#ifdef INSTALL_AVRCP_METADATA_ATTRIBUTES
#define CsrBtAvrcpCtLibElementsAttributeGet(maxData, attIndex, itemsLen, items, attribId, charset, attLen, att) \
        CsrBtAvrcpCtLibGfiMediaAttributeGet(maxData, attIndex, itemsLen, items, attribId, charset, attLen, att)
#endif

CsrBool CsrBtAvrcpCtLibItemsAttributeGet(CsrUint16 maxData,
                                         CsrUint16 *attIndex,
                                         CsrUint16 itemsLen,
                                         CsrUint8 *items,
                                         CsrBtAvrcpItemMediaAttributeId *attribId,
                                         CsrBtAvrcpCharSet *charset,
                                         CsrUint16 *attLen,
                                         CsrUint8 **att);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtChangePathReqSend
 *
 *  DESCRIPTION
 *      Request to initiate a change of the current path used for other browsing commands.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_CHANGE_PATH_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      uidCounter:     Current UID counter
 *      folderDir:      Use one of the following values:
 *                      CSR_BT_AVRCP_CHANGE_PATH_UP
 *                      CSR_BT_AVRCP_CHANGE_PATH_DOWN
 *      folderUid:      UID of the folder item
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtChangePathReqSend(_phandle, _connId, _uidCounter, _folderDir, _folderUid) { \
        CsrBtAvrcpCtChangePathReq *msg = (CsrBtAvrcpCtChangePathReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_CHANGE_PATH_REQ;           \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->folderDir     = _folderDir;                                \
        msg->uidCounter    = _uidCounter;                               \
        SynMemCpyS(msg->folderUid, sizeof(CsrBtAvrcpUid), _folderUid, sizeof(CsrBtAvrcpUid));   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined (INSTALL_AVRCP_METADATA_ATTRIBUTES)
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtGetAttributesReqSend
 *
 *  DESCRIPTION
 *      Controller requests the attributes of a media item to remote Target device.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_GET_ATTRIBUTES_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      scope:          scope. Refer to CsrBtAvrcpScope for defined scopes.
 *      uid:            UID of the item
 *      uidCounter:     Current UID counter
 *      attributeMask:  Bitmask combined of the differnt attribute flags.
 *                      Refer to CsrBtAvrcpItemAttMask type for all defined flags.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtGetAttributesReqSend(_phandle, _connId, _scope, _uid, _uidCounter, _attributeMask) { \
        CsrBtAvrcpCtGetAttributesReq *msg = (CsrBtAvrcpCtGetAttributesReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_GET_ATTRIBUTES_REQ;        \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->scope         = _scope;                                    \
        msg->uidCounter    = _uidCounter;                               \
        msg->attributeMask = _attributeMask;                            \
        SynMemCpyS(msg->uid, sizeof(CsrBtAvrcpUid), _uid, sizeof(CsrBtAvrcpUid));               \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtGetAttributesResSend
 *
 *  DESCRIPTION
 *      When a controller requests the Target for attributes of a media item,
 *      On response from Target, Synergy indicates application through
 *      CSR_BT_AVRCP_CT_GET_ATTRIBUTES_IND if Get element attribute response
 *      is fragmented. Application uses this API to respond to this indication
 *      whether to continue sending the next fragment or abort the procedure.
 *
 *    PARAMETERS
 *      connId:         connection identifier
 *      proceed:        Set to TRUE to get next fragment or FALSE to abort the procedure
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtGetAttributesResSend(_connId, _proceed) {           \
        CsrBtAvrcpCtGetAttributesRes *msg = (CsrBtAvrcpCtGetAttributesRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_GET_ATTRIBUTES_RES;        \
        msg->connectionId  = _connId;                                   \
        msg->proceed       = _proceed;                                  \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || INSTALL_AVRCP_METADATA_ATTRIBUTES */

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtPlayReqSend
 *
 *  DESCRIPTION
 *      Request to play a specific media item. At the completion of the request
 *      Synregy sends CSR_BT_AVRCP_CT_PLAY_CFM message to application.
 *
 *  PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      scope:          scope. Refer to CsrBtAvrcpScope in csr_bt_avrcp_prim.h
 *                      for defined scopes
 *      uidCounter:     Current UID counter
 *      uid:            UID of the item
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtPlayReqSend(_phandle, _connId, _scope, _uidCounter, _uid) { \
        CsrBtAvrcpCtPlayReq *msg = (CsrBtAvrcpCtPlayReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_PLAY_REQ;                  \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->scope         = _scope;                                    \
        msg->uidCounter    = _uidCounter;                               \
        SynMemCpyS(msg->uid, sizeof(CsrBtAvrcpUid), _uid, sizeof(CsrBtAvrcpUid));               \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtAddToNowPlayingReqSend
 *
 *  DESCRIPTION
 *      Request to add a media item to the now playing list.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_ADD_TO_NOW_PLAYING_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      scope:          scope. Refer to CsrBtAvrcpScope for defined scopes
 *      uidCounter:     Current UID counter
 *      uid:            UID of the item
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtAddToNowPlayingReqSend(_phandle, _connId, _scope, _uidCounter, _uid) { \
        CsrBtAvrcpCtAddToNowPlayingReq *msg = (CsrBtAvrcpCtAddToNowPlayingReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_ADD_TO_NOW_PLAYING_REQ;    \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->scope         = _scope;                                    \
        msg->uidCounter    = _uidCounter;                               \
        SynMemCpyS(msg->uid, sizeof(CsrBtAvrcpUid), _uid, sizeof(CsrBtAvrcpUid));               \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtSearchReqSend
 *
 *  DESCRIPTION
 *      Request to generate a list of search results from a target.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_SEARCH_REQ
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:        application handle
 *      connId:         connection identifier
 *      text:           NULL-terminated string to search for
 *      charsetId:      Character set ID if application wants to provide any
 *                      other character set ID. Default character set ID
 *                      specified by the controller for the search string is
 *                      UTF_8 (i.e. 0x006A).
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtSearchReqSend(_phandle, _connId, _text, _charsetId) {\
        CsrBtAvrcpCtSearchReq *msg = (CsrBtAvrcpCtSearchReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_SEARCH_REQ;                 \
        msg->phandle       = _phandle;                                   \
        msg->connectionId  = _connId;                                    \
        msg->text          = _text;                                      \
        msg->charsetId     = _charsetId;                                 \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtGetTotalNumberOfItemsReqSend
 *
 *  DESCRIPTION
 *      CsrBtAvrcpCtGetTotalNumberOfItemsReq msg can be used to retrieve the total number 
 *      of items in a folder prior to retrieving a listing of the contents of a browsed folder and can 
 *      be used by Controller to:
 *          - find the total number of Media players on Target
 *          - find total number of media element items in a folder (media file system) on Target
 *          - find total number of media element items in Search Results list on Target
 *          - find total number of media element items in Queue/Now Playing list of addressed 
 *            Media player on Target.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_GET_TOTAL_NUMBER_OF_ITEMS_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      connId:     Unique number for identifying the specific connection 
 *      scope:      The scope of browsing(MPL/MP VFS/SEARCH/NPL)
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtGetTotalNumberOfItemsReqSend(_phandle, _connId, _scope) { \
        CsrBtAvrcpCtGetTotalNumberOfItemsReq *msg = (CsrBtAvrcpCtGetTotalNumberOfItemsReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_GET_TOTAL_NUMBER_OF_ITEMS_REQ; \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->scope         = _scope;                                \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtSetVolumeReqSend
 *
 *  DESCRIPTION
 *      Controller requests to change the volume on a target device.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_SET_VOLUME_CFM
 *      message with appropriate result.
 *
 *  PARAMETERS
 *      phandle:         application phandle
 *      connId:          connection identifier
 *      volume:          volume level to be changed (scale accordingly between the two limits):
 *                       0x00: 0%
 *                       0x7F: 100%
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtSetVolumeReqSend(_phandle, _connId, _volume) {      \
        CsrBtAvrcpCtSetVolumeReq *msg = (CsrBtAvrcpCtSetVolumeReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_CT_SET_VOLUME_REQ;            \
        msg->phandle       = _phandle;                                  \
        msg->connectionId  = _connId;                                   \
        msg->volume        = _volume;                                   \
        CsrBtAvrcpMsgTransport(msg);}

#ifdef INSTALL_AVRCP_UNIT_COMMANDS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtUnitInfoCmdReqSend
 *
 *  DESCRIPTION
 *      Request to send a "unit info command" to the TG device,
 *      with data payload gathered at the application level.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_UNIT_INFO_CMD_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:         application phandle
 *      connId:          connection identifier
 *      pDatalen:        Length in bytes of the data payload to attach
 *      pData:           Pointer to the data stream to send in the message.
 *                       The contents of the allocated area are sent as they
 *                       are and the pointer is de-allocated by the CT
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtUnitInfoCmdReqSend(_phandle, _connId, _pDatalen, _pData) { \
        CsrBtAvrcpCtUnitInfoCmdReq *msg  = (CsrBtAvrcpCtUnitInfoCmdReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type         = CSR_BT_AVRCP_CT_UNIT_INFO_CMD_REQ;          \
        msg->connectionId = _connId;                                    \
        msg->phandle      = _phandle;                                   \
        msg->pDataLen     = _pDatalen;                                  \
        msg->pData        = _pData;                                     \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpCtSubUnitInfoCmdReqSend
 *
 *  DESCRIPTION
 *      Request to send a "sub-unit info command" to the TG device,
 *      with data payload gathered at the application level.
 *      AVRCP responds the application back with CSR_BT_AVRCP_CT_SUB_UNIT_INFO_CMD_CFM
 *      message with appropriate result.
 *
 *    PARAMETERS
 *      phandle:         application phandle
 *      connId:          connection identifier
 *      pDatalen:        Length in bytes of the data payload to attach
 *      pData:           Pointer to the data stream to send in the message.
 *                       The contents of the allocated area are sent as they
 *                       are and the pointer is de-allocated by the CT
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpCtSubUnitInfoCmdReqSend(_phandle, _connId, _pDatalen, _pData) { \
        CsrBtAvrcpCtSubUnitInfoCmdReq *msg  = (CsrBtAvrcpCtSubUnitInfoCmdReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type         = CSR_BT_AVRCP_CT_SUB_UNIT_INFO_CMD_REQ;      \
        msg->connectionId = _connId;                                    \
        msg->phandle      = _phandle;                                   \
        msg->pDataLen     = _pDatalen;                                  \
        msg->pData        = _pData;                                     \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_UNIT_COMMANDS */
#endif /* CSR_BT_INSTALL_AVRCP_CT_13_AND_HIGHER */
#endif /* EXCLUDE_CSR_BT_AVRCP_CT_MODULE */

#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgPassThroughResSend
 *
 *  DESCRIPTION
 *      When Target device receives an incoming pass-through command from Controller.
 *      AVRCP indicates this through CSR_BT_AVRCP_TG_PASS_THROUGH_IND to the
 *      addressed media player application.
 *      Application uses this API to respond the above indication.
 *
 *  PARAMETERS
 *      connId:       connection identifier
 *      msgId:        Unique message ID for associating indications and responses,
 *                    use the ID from an indication in the matching response
 *      status:       Indicates whether the operation is accepted or rejected
 *                    by means of the defines prefixed with
 *                    CSR_BT_AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgPassThroughResSend(_connId, _msgId, _status) {      \
        CsrBtAvrcpTgPassThroughRes *msg = (CsrBtAvrcpTgPassThroughRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_PASS_THROUGH_RES;          \
        msg->connectionId  = _connId;                                   \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgGetPlayStatusResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to get the play status. Synergy notifies
 *      the addressed media player application through CSR_BT_AVRCP_TG_GET_PLAY_STATUS_IND.
 *      Application uses this API to respond to this indication.
 *
 *    PARAMETERS
 *      connId:          connection identifier
 *      songLength:      Total length of the playing song in milliseconds
 *      songPosition:    Current position of the playing song in milliseconds elapsed
 *      playStatus:      Use one of the following values:
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_STOPPED
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_PLAYING
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_PAUSED
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_FWD_SEEK
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_REV_SEEK
 *                       CSR_BT_AVRCP_PLAYBACK_STATUS_ERROR
 *      msgId:           Unique message ID to associate indications and responses,
 *                       use the ID from an indication in the matching response.
 *      status:          Indicates whether the operation is accepted or rejected
 *                       by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgGetPlayStatusResSend(_connId, _songLength, _songPosition, _playStatus, _msgId, _status) { \
        CsrBtAvrcpTgGetPlayStatusRes *msg = (CsrBtAvrcpTgGetPlayStatusRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_GET_PLAY_STATUS_RES;       \
        msg->connectionId  = _connId;                                   \
        msg->songLength    = _songLength;                               \
        msg->songPosition  = _songPosition;                             \
        msg->playStatus    = _playStatus;                               \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgNotiReqSend
 *
 *  DESCRIPTION
 *      When an event occurs on Target media player for the notifications it supports,
 *      application uses this API to notify the changed event to remote controller.
 *
 *  PARAMETERS
 *      playerId:    Unique ID of the media player the message is intended for
 *      notiId:      The ID of the notification. Refer to see different notification
 *                   ids of the type CsrBtAvrcpNotiId in csr_bt_avrcp_prim.h
 *      notiData:    Value included in CHANGED request
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgNotiReqSend(_playerId, _notiId, _notiData) {        \
        CsrBtAvrcpTgNotiReq *msg = (CsrBtAvrcpTgNotiReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_NOTI_REQ;                  \
        msg->playerId      = _playerId;                                 \
        msg->notiId        = _notiId;                                   \
        SynMemCpyS(msg->notiData, CSR_BT_AVRCP_TG_NOTI_MAX_SIZE, _notiData,CSR_BT_AVRCP_TG_NOTI_MAX_SIZE); \
        CsrBtAvrcpMsgTransport(msg);}

void CsrBtAvrcpTgNotiPlaybackStatusReq(CsrUint32 playerId, CsrUint8 playbackStatus);

#if defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) || defined(INSTALL_AVRCP_NOTIFICATIONS)
void CsrBtAvrcpTgNotiTrackReq(CsrUint32 playerId, CsrBtAvrcpUid uid);
void CsrBtAvrcpTgNotiTrackRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrBtAvrcpUid uid);
#endif /* INSTALL_AVRCP_METADATA_ATTRIBUTES || INSTALL_AVRCP_NOTIFICATIONS */

#ifdef INSTALL_AVRCP_NOTIFICATIONS
void CsrBtAvrcpTgNotiTrackStartEndReq(CsrUint32 playerId, CsrBool start);
void CsrBtAvrcpTgNotiPlaybackPositionReq(CsrUint32 playerId, CsrUint32 pos);
void CsrBtAvrcpTgNotiBatStatusReq(CsrUint32 playerId, CsrBtAvrcpBatteryStatus batStatus);
void CsrBtAvrcpTgNotiSystemStatusReq(CsrUint32 playerId, CsrBtAvrcpSystemStatus sysStatus);
void CsrBtAvrcpTgNotiTrackStartEndRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrBool start);
void CsrBtAvrcpTgNotiPlaybackPositionRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrUint32 pos);
void CsrBtAvrcpTgNotiBatStatusRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrBtAvrcpBatteryStatus batStatus);
void CsrBtAvrcpTgNotiSystemStatusRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrBtAvrcpSystemStatus sysStatus);
void CsrBtAvrcpTgNotiVolumeReq(CsrUint32 playerId, CsrUint8 volume);
#endif /* INSTALL_AVRCP_NOTIFICATIONS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgNotiResSend
 *
 *  DESCRIPTION
 *      When a controller registers for a notification for the type that has
 *      previously been enabled when a media player was registered. AVRCP
 *      at Target device notifies this to application through CSR_BT_AVRCP_TG_NOTI_IND.
 *      Application uses this API to respond above indication with the interim
 *      value of the respective notification to notify Controller.
 *
 *  PARAMETERS
 *      connId:          connection identifier
 *      notiId:          The ID of the notification. Refer to see different notification
 *                       ids of the type CsrBtAvrcpNotiId in csr_bt_avrcp_prim.h
 *      notiData:        Value included in CHANGED response
 *      status:          Indicates whether the operation is accepted or rejected
 *                       by means of the defines prefixed with CSR_BT_AVRCP_STATUS_ in csr_bt_avrcp_prim.h.
 *      msgId:           Unique message ID to associate indications and responses,
 *                       use the ID from an indication in the matching response.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgNotiResSend(_connId, _notiId, _notiData, _status, _msgId) { \
        CsrBtAvrcpTgNotiRes *msg = (CsrBtAvrcpTgNotiRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_NOTI_RES;                  \
        msg->connectionId  = _connId;                                   \
        msg->notiId        = _notiId;                                   \
        msg->status        = _status;                                   \
        msg->msgId         = _msgId;                                    \
        SynMemCpyS(msg->notiData, CSR_BT_AVRCP_TG_NOTI_MAX_SIZE, _notiData,CSR_BT_AVRCP_TG_NOTI_MAX_SIZE); \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgNotiVolumeRes
 *
 *  DESCRIPTION
 *      When Target application receives CSR_BT_AVRCP_TG_NOTI_IND for the volume
 *      changed event. Application uses this API to respond Controller about the
 *      changed volume.
 *
 *  PARAMETERS
 *      connId:          connection identifier
 *      status:          Indicates whether the operation is accepted or rejected
 *                       by means of the defines prefixed with CSR_BT_AVRCP_STATUS_ in csr_bt_avrcp_prim.h.
 *      msgId:           Unique message ID to associate indications and responses,
 *                       use the ID from an indication in the matching response.
 *      volume:          Current volume level
 *----------------------------------------------------------------------------*/
void CsrBtAvrcpTgNotiVolumeRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrUint8 volume);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgNotiPlaybackStatusRes
 *
 *  DESCRIPTION
 *      When Target application receives CSR_BT_AVRCP_TG_NOTI_IND for the playack
 *      status changed event. Application uses this API to respond to Controller about 
 *      the changed status.
 *
 *  PARAMETERS
 *      connId:          connection identifier
 *      status:          Indicates whether the operation is accepted or rejected
 *                       by means of the defines prefixed with CSR_BT_AVRCP_STATUS_ in csr_bt_avrcp_prim.h.
 *      msgId:           Unique message ID to associate indications and responses,
 *                       use the ID from an indication in the matching response.
 *      playbackStatus:  playback status
 *----------------------------------------------------------------------------*/
void CsrBtAvrcpTgNotiPlaybackStatusRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrBtAvrcpPlaybackStatus playbackStatus);

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
void CsrBtAvrcpTgNotiNowPlayingReq(CsrUint32 playerId);
void CsrBtAvrcpTgNotiNowPlayingRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId);
void CsrBtAvrcpTgNotiUidsReq(CsrUint32 playerId, CsrUint16 uidCounter);
void CsrBtAvrcpTgNotiUidsRes(CsrUint8 connId, CsrBtAvrcpStatus status, CsrUint32 msgId, CsrUint16 uidCounter);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#ifdef CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgPasCurrentResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests the current PAS values from a target,
 *      Synergy notifies the currently addressed media player through
 *      CSR_BT_AVRCP_TG_PAS_CURRENT_IND message. Application uses this interface
 *      to respond to this indication.
 *
 *  PARAMETERS
 *      connId:   connection identifier
 *      msgId:    Unique message ID to associate indications and responses,
 *                use the ID from an indication in the matching response.
 *      pasCount: Number of attribute/value pairs
 *      pas:      Pointer to number of attribute/value pairs of CsrBtAvrcpPasAttValPair type.
 *      status:   Indicates whether the operation is accepted or rejected
 *                by means of the defines prefixed with CSR_BT_AVRCP_STATUS_
 *                in csr_bt_avrcp_prim.h.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgPasCurrentResSend(_connId, _msgId, _pasCount, _pas, _status) { \
        CsrBtAvrcpTgPasCurrentRes *msg = (CsrBtAvrcpTgPasCurrentRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_PAS_CURRENT_RES;           \
        msg->connectionId  = _connId;                                   \
        msg->msgId         = _msgId;                                    \
        msg->attValPairCount      = _pasCount;                          \
        msg->attValPair           = _pas;                               \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgPasSetReqSend
 *
 *  DESCRIPTION
 *      When a Target application changes the values of player application
 *      settings attributes. It uses this API to notify Controller device
 *      about the change.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      playerId:           Unique ID of the media player the message is intended for
 *      changedPasCount:    Number of PAS attributes to change value for
 *      changedPas:         Pointer to attribute/value pairs
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgPasSetReqSend(_phandle, _playerId, _changedPasCount, _changedPas) { \
        CsrBtAvrcpTgPasSetReq *msg = (CsrBtAvrcpTgPasSetReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_PAS_SET_REQ;               \
        msg->phandle       = _phandle;                                  \
        msg->playerId      = _playerId;                                 \
        msg->attValPair    = _changedPas;                               \
        msg->attValPairCount = _changedPasCount;                        \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgPasSetResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests a target to change the values of
 *      specified attributes. Synergy at Target device notifies the application
 *      through CSR_BT_AVRCP_TG_PAS_SET_IND. Application uses this API to respond
 *      to this indication.
 *
 *  PARAMETERS
 *      connId:   connection identifier
 *      msgId:    Unique message ID to associate indications and responses,
 *                use the ID from an indication in the matching response.
 *      status:   Indicates whether the operation is accepted or rejected
 *                by means of the defines prefixed with CSR_BT_AVRCP_STATUS_
 *                in csr_bt_avrcp_prim.h.
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgPasSetResSend(_connId, _msgId, _status) {           \
        CsrBtAvrcpTgPasSetRes *msg = (CsrBtAvrcpTgPasSetRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_PAS_SET_RES;               \
        msg->connectionId  = _connId;                                   \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

#define CSR_BT_AVRCP_TG_LIB_PAS_INVALID_INDEX      (0xFFFF)

#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_COUNT_IDX      (0)

/* Relative to the beginning of an attribute */
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_SIZE           (5)
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_ID_IDX         (0)
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_LEN_IDX        (1)
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_VAL_COUNT_IDX  (3)
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_TXT_LEN_IDX    (4)
#define CSR_BT_AVRCP_TG_LIB_PAS_ATT_TXT_IDX        (5)

/* Relative to the beginning of a value */
#define CSR_BT_AVRCP_TG_LIB_PAS_VAL_SIZE           (2)
#define CSR_BT_AVRCP_TG_LIB_PAS_VAL_ID_IDX         (0)
#define CSR_BT_AVRCP_TG_LIB_PAS_VAL_TXT_LEN_IDX    (1)
#define CSR_BT_AVRCP_TG_LIB_PAS_VAL_TXT_IDX        (2)

/* TG PAS helper functions */
CsrUint16 CsrBtAvrcpTgLibPasAttribGet(CsrUint16 pasLen, CsrUint8 *pas, CsrUint8 attId);
CsrUint16 CsrBtAvrcpTgLibPasValueGet(CsrUint16 pasLen, CsrUint8 *pas, CsrUint8 attId, CsrUint8 valId);
CsrUint16 CsrBtAvrcpTgLibPasValueFirstGet(CsrUint16 pasLen, CsrUint8 *pas, CsrUint16 attIndex);
CsrBool CsrBtAvrcpTgLibPasAttribNext(CsrUint16 pasLen, CsrUint8 *pas, CsrUint16 *attIndex);
CsrBool CsrBtAvrcpTgLibPasValueNext(CsrUint16 pasLen, CsrUint8 *pas, CsrUint16 attIdIndex, CsrUint16 *valIndex);
CsrUint8 CsrBtAvrcpTgLibPasAttribCount(CsrUint16 pasLen, CsrUint8 *pas);
CsrUint8 CsrBtAvrcpTgLibPasValueCount(CsrUint16 pasLen, CsrUint8 *pas, CsrUint8 attId);

#ifdef CSR_BT_INSTALL_AVRCP_TG_13_AND_HIGHER
void CsrBtAvrcpTgLibPasAttribAdd(CsrUint16 *pasLen, CsrUint8 **pas, CsrBtAvrcpPasAttId attId, const CsrUtf8String *attTxt);
void CsrBtAvrcpTgLibPasValueAdd(CsrUint16 *pasLen, CsrUint8 **pas, CsrBtAvrcpPasAttId attId, CsrBtAvrcpPasValId valId, const CsrUtf8String *valTxt);
#else
#define CsrBtAvrcpTgLibPasAttribAdd(pasLen, pas, attId, attTxt)
#define CsrBtAvrcpTgLibPasValueAdd(pasLen, pas, attId, valId, valTxt)
#endif
#endif /* CSR_BT_INSTALL_AVRCP_PLAYER_APP_SETTINGS */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgMpRegisterReqSend
 *
 *  DESCRIPTION
 *      Before the target role can be used, at least one media player should be registered.
 *      If the device supports TG role and no media player is registered, profile activation fails.
 *      Application uses this API to register a media player.
 *
 *  PARAMETERS
 *      playerHandle:       Task id of the media player
 *      notificationMask:   Bitmask specifying which notifications the media player supports.
 *      configMask:         Bitmask for special configuration of the MP. Set to CSR_BT_AVRCP_TG_MP_REGISTER_CONFIG_NONE for
 *                          no special configuration or to CSR_BT_AVRCP_TG_MP_REGISTER_CONFIG_SET_DEFAULT
 *                          if the media player should be marked as the default media player.
 *      pasLen:             Length of the data contained in *pas
 *      pas:                Tag-based byte stream containing all player application settings for the target.
 *      majorType:          Bitmask specifying the major type of the media player
 *      subType:            Bitmask specifying the sub type of the media player
 *      featureMask:        128-bit mask specifying the features supported by the media player.
 *                          The mask is divided into four blocks of 32- bit.
 *      playerName:         NULL-terminated string (UTF-8) containing the name of the media player
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgMpRegisterReqSend(_playerHandle, _notificationMask, _configMask, _pasLen, _pas, _majorType, _subType, _featureMask, _playerName) { \
        CsrBtAvrcpTgMpRegisterReq *msg = (CsrBtAvrcpTgMpRegisterReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_TG_MP_REGISTER_REQ;       \
        msg->playerHandle      = _playerHandle;                         \
        msg->notificationMask  = _notificationMask;                     \
        msg->configMask        = _configMask;                           \
        msg->pasLen            = _pasLen;                               \
        msg->pas               = _pas;                                  \
        msg->majorType         = _majorType;                            \
        msg->subType           = _subType;                              \
        msg->playerName        = _playerName;                           \
        CSR_BT_AVRCP_FEATURE_MASK_COPY(msg->featureMask, _featureMask); \
        CsrBtAvrcpMsgTransport(msg);}

#ifndef CSR_TARGET_PRODUCT_VM
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgMpUnregisterReqSend
 *
 *  DESCRIPTION
 *      Request to unregister the registered media player.
 *
 *    PARAMETERS
 *      phandle:            application phandle
 *      playerId:           Task id of the media player
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgMpUnregisterReqSend(_phandle, _playerId) {          \
        CsrBtAvrcpTgMpUnregisterReq *msg = (CsrBtAvrcpTgMpUnregisterReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_TG_MP_UNREGISTER_REQ;     \
        msg->phandle           = _phandle;                              \
        msg->playerId          = _playerId;                             \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* !CSR_TARGET_PRODUCT_VM */

#ifdef INSTALL_AVRCP_MEDIA_PLAYER_SELECTION
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgSetAddressedPlayerReqSend
 *
 *  DESCRIPTION
 *      Media player application uses this API to set the addressed player.
 *
 *  PARAMETERS
 *      phandle:            application phandle
 *      playerId:           Task id of the media player
 *      uidCounter:         Current UID counter
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgSetAddressedPlayerReqSend(_phandle, _playerId, _uidCounter) { \
        CsrBtAvrcpTgSetAddressedPlayerReq *msg = (CsrBtAvrcpTgSetAddressedPlayerReq *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_REQ; \
        msg->phandle           = _phandle;                              \
        msg->playerId          = _playerId;                             \
        msg->uidCounter        = _uidCounter;                           \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgSetAddressedPlayerResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to change the addressed player to the target.
 *      Synergy notifies the media player application through CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_IND.
 *      Application uses this API to respond to this application.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      playerId:       Unique ID of the media player the controller requests to set as the browsed player
 *      uidCounter:     Current UID counter
 *      msgId:          Unique message ID for associating indications and responses
 *      status:         Indicates whether the operation succeeded or failed
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgSetAddressedPlayerResSend(_connId, _playerId, _uidCounter, _msgId, _status) { \
        CsrBtAvrcpTgSetAddressedPlayerRes *msg = (CsrBtAvrcpTgSetAddressedPlayerRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_TG_SET_ADDRESSED_PLAYER_RES; \
        msg->uidCounter        = _uidCounter;                           \
        msg->connectionId      = _connId;                               \
        msg->msgId             = _msgId;                                \
        msg->playerId          = _playerId;                             \
        msg->status            = _status;                               \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* INSTALL_AVRCP_MEDIA_PLAYER_SELECTION*/

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgSetBrowsedPlayerResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to set the browsed player on Target.
 *      Synergy notifies the media player application through
 *      CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_IND. Application uses this API
 *      to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      playerId:       Unique ID of the media player the controller requests to set as the browsed player
 *      uidCounter:     Current UID counter
 *      itemsCount:     Number of items in the current folder of the browsed player
 *      folderDepth:    Depth of the current folder. The root folder has no name and its folder depth is 0.
 *                      If /A/BC/DEF is the current folder, then the folder depth is 3.
 *      folderNamesLen: Number of bytes allocated in the folderNames field below
 *      folderNames:    Directory path from the root directory to the current directory,
 *                      separated by either the '\' or the '/' symbol.
 *      msgId:          Unique message ID for associating indications and responses
 *      status:         Indicates whether the operation succeeded or failed
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgSetBrowsedPlayerResSend(_connId, _playerId, _uidCounter, _itemsCount, _folderDepth, _folderNamesLen, _folderNames, _msgId, _status) { \
        CsrBtAvrcpTgSetBrowsedPlayerRes *msg = (CsrBtAvrcpTgSetBrowsedPlayerRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type              = CSR_BT_AVRCP_TG_SET_BROWSED_PLAYER_RES; \
        msg->uidCounter        = _uidCounter;                           \
        msg->itemsCount        = _itemsCount;                           \
        msg->folderDepth       = _folderDepth;                          \
        msg->folderNamesLen    = _folderNamesLen;                       \
        msg->folderNames       = _folderNames;                          \
        msg->msgId             = _msgId;                                \
        msg->playerId          = _playerId;                             \
        msg->status            = _status;                               \
        msg->connectionId      = _connId;                               \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgGetTotalNumberOfItemsResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to retrieve the total number of items
 *      in a folder prior to retrieving a listing of the contents of a browsed folder.
 *      Synergy notifies the application through CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_IND.
 *      Application uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:     The unique connection ID of the AVRCP connection
 *      noOfItems:  Total number of media items in the response message
 *      uidCounter: The current UID counter
 *      msgId:      Unique message ID for associating indications and responses
 *      status:     Indicates whether the operation succeeded or failed
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgGetTotalNumberOfItemsResSend(_connId, _noOfItems, _uidCounter, _msgId, _status) { \
        CsrBtAvrcpTgGetTotalNumberOfItemsRes *msg = (CsrBtAvrcpTgGetTotalNumberOfItemsRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_GET_TOTAL_NUMBER_OF_ITEMS_RES;      \
        msg->connectionId  = _connId;                                   \
        msg->noOfItems     = _noOfItems;                                \
        msg->uidCounter    = _uidCounter;                               \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgGetFolderItemsResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests the folder list for the virtual file
 *      system, now playing list or search results. Synergy notifies the browsed
 *      player application through CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_IND. Browsed
 *      player application uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      itemsCount:     Number of items in the new folder
 *      uidCounter:     Current UID counter
 *      itemsLen:       Length of the media items data in bytes. The length should
 *                      not exceed the value given by maxData. If all media items
 *                      cannot fit into one response, as many complete media items as
 *                      possible should be included in the response
 *      items:          Pointer to the media items data
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgGetFolderItemsResSend(_connId, _itemCount, _uidCounter, _itemsLen, _items, _msgId, _status) { \
        CsrBtAvrcpTgGetFolderItemsRes *msg = (CsrBtAvrcpTgGetFolderItemsRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_GET_FOLDER_ITEMS_RES;      \
        msg->connectionId  = _connId;                                   \
        msg->itemsCount    = _itemCount;                                \
        msg->uidCounter    = _uidCounter;                               \
        msg->itemsLen      = _itemsLen;                                 \
        msg->items         = _items;                                    \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

CsrBool CsrBtAvrcpTgLibGfiFolderAdd(CsrUint16 maxData,
                                    CsrUint16 *itemsLen,
                                    CsrUint8 **items,
                                    CsrBtAvrcpUid *folderUid,
                                    CsrBtAvrcpFolderType folderType,
                                    CsrBtAvrcpFolderPlayableType playableType,
                                    CsrBtAvrcpCharSet charset,
                                    CsrCharString *folderName);

CsrBool CsrBtAvrcpTgLibGfiMediaAdd(CsrUint16 maxData,
                                   CsrUint16 *index,
                                   CsrUint8 **data,
                                   CsrUint16 *mediaIndex,
                                   CsrBtAvrcpUid *mediaUid,
                                   CsrBtAvrcpMediaType mediaType,
                                   CsrBtAvrcpCharSet charset,
                                   CsrCharString *mediaName);

CsrBool CsrBtAvrcpTgLibGfiMediaAttributeAdd(CsrUint16 maxData,
                                            CsrUint16 *itemsLen,
                                            CsrUint8 **items,
                                            CsrUint16 mediaIndex,
                                            CsrBtAvrcpItemMediaAttributeId attribId,
                                            CsrBtAvrcpCharSet charset,
                                            CsrUint16 attLen,
                                            CsrUint8 *att);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgChangePathResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests a change of current path, Synergy notifies
 *      the browsed player application through CSR_BT_AVRCP_TG_CHANGE_PATH_IND.
 *      Browsed player application uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      itemsCount:     Number of items in the new folder
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgChangePathResSend(_connId, _itemsCount, _msgId, _status) { \
        CsrBtAvrcpTgChangePathRes *msg = (CsrBtAvrcpTgChangePathRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_CHANGE_PATH_RES;           \
        msg->connectionId  = _connId;                                   \
        msg->itemsCount    = _itemsCount;                               \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#if defined (CSR_BT_INSTALL_AVRCP_BROWSING) || defined(INSTALL_AVRCP_METADATA_ATTRIBUTES) 
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgGetAttributesResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests the target for attributes of a media item,
 *      Synergy notifies the addressed player through CSR_BT_AVRCP_TG_GET_ATTRIBUTES_IND.
 *      Addressed player application uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      attribCount:    Number of attributes returned in the response message
 *      attribDataLen:  Length of the attribute data in bytes including two bytes
 *                      extra that are filled in by the AVRCP profile
 *      attribData:     Pointer to the attribute data. The first two bytes allocated
 *                      should be left unused.
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgGetAttributesResSend(_connId, _attribCount, _attribDataLen, _attribData, _msgId, _status) { \
        CsrBtAvrcpTgGetAttributesRes *msg = (CsrBtAvrcpTgGetAttributesRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_GET_ATTRIBUTES_RES;        \
        msg->connectionId  = _connId;                                   \
        msg->attribCount   = _attribCount;                              \
        msg->attribDataLen = _attribDataLen;                            \
        msg->attribData    = _attribData;                               \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING || INSTALL_AVRCP_METADATA_ATTRIBUTES*/

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgPlayResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to play a specific media item on a
 *      Target device. Synergy notifies the addressed media player through
 *      CSR_BT_AVRCP_TG_PLAY_IND. Media player application uses this API
 *      to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      uid:            UID of the item
 *      scope:          scope. Refer to CsrBtAvrcpScope for defined scopes
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgPlayResSend(_connId, _uid, _scope, _msgId, _status) { \
        CsrBtAvrcpTgPlayRes *msg = (CsrBtAvrcpTgPlayRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_PLAY_RES;                  \
        msg->connectionId  = _connId;                                   \
        msg->scope         = _scope;                                    \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        SynMemCpyS(msg->uid, sizeof(CsrBtAvrcpUid), _uid, sizeof(CsrBtAvrcpUid));               \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgAddToNowPlayingResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests to add a media element to the now
 *      playing list. Synergy notifies the addressed media player through
 *      CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_IND. Media player application
 *      uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgAddToNowPlayingResSend(_connId, _msgId, _status) {  \
        CsrBtAvrcpTgAddToNowPlayingRes *msg = (CsrBtAvrcpTgAddToNowPlayingRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_ADD_TO_NOW_PLAYING_RES;    \
        msg->connectionId  = _connId;                                   \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgSearchResSend
 *
 *  DESCRIPTION
 *      When a remote controller requests a search, Synergy notifies
 *      the browsed media player application through CSR_BT_AVRCP_TG_SEARCH_IND.
 *      Browsed player application uses this API to respond to this indication.
 *
 *  PARAMETERS
 *      connId:         The unique connection ID of the AVRCP connection
 *      uidCounter:     The current UID counter
 *      numberOfItems:  Number of items in the search result
 *      msgId:          Unique message ID for associating indications and responses,
 *                      use the ID from an indication in the matching response
 *      status:         Indicates whether the operation is accepted or rejected
 *                      by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgSearchResSend(_connId, _uidCounter, _numberOfItems, _msgId, _status) { \
        CsrBtAvrcpTgSearchRes *msg = (CsrBtAvrcpTgSearchRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_SEARCH_RES;                \
        msg->connectionId  = _connId;                                   \
        msg->uidCounter    = _uidCounter;                               \
        msg->numberOfItems = _numberOfItems;                            \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        CsrBtAvrcpMsgTransport(msg);}
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtAvrcpTgSetVolumeExtResSend
 *
 *  DESCRIPTION
 *       Response to the remote controller's request to change the volume on a target.
 *
 *  PARAMETERS
 *      connId:          connection identifier
 *      volume:          actual volume set
 *      msgId:           Unique message ID to associate indications and responses,
 *                       use the ID from an indication in the matching response.
 *      status:          Indicates whether the operation is accepted or rejected
 *                       by the defines prefixed with AVRCP_STATUS_ in csr_bt_avrcp_prim.h
 *      tlabel:          transaction label
 *----------------------------------------------------------------------------*/
#define CsrBtAvrcpTgSetVolumeExtResSend(_connId, _volume, _msgId, _status, _tlabel) { \
        CsrBtAvrcpTgSetVolumeRes *msg = (CsrBtAvrcpTgSetVolumeRes *) CsrPmemZalloc(sizeof(*msg)); \
        msg->type          = CSR_BT_AVRCP_TG_SET_VOLUME_RES;            \
        msg->connectionId  = _connId;                                   \
        msg->volume        = _volume;                                   \
        msg->msgId         = _msgId;                                    \
        msg->status        = _status;                                   \
        msg->tLabel        = _tlabel;                                   \
        CsrBtAvrcpMsgTransport(msg);}

#define CsrBtAvrcpTgSetVolumeResSend(_connId, _volume, _msgId, _status) \
        CsrBtAvrcpTgSetVolumeExtResSend(_connId, _volume, _msgId, _status, CSR_BT_AVRCP_TLABEL_INVALID)

#endif /* EXCLUDE_CSR_BT_AVRCP_TG_MODULE */

/* Accumulates payload from multiple packets into a single buffer */
CsrBool CsrBtAvrcpCtLibAppendPayload(CsrUint8 **buffer,
                                     CsrUint16 *bufferLen,
                                     CsrUint8 *rxData,
                                     CsrUint16 rxDataLen,
                                     CsrUint16 rxDataOffset);
void CsrBtAvrcpFreeRemoteFeaturesInd(CsrBtAvrcpRemoteFeaturesInd **prim);
void CsrBtAvrcpUtilFreeRoleDetails(CsrBtAvrcpRoleDetails *ptr);
void CsrBtAvrcpFreeUpstreamMessageContents(CsrUint16 eventClass, void * message);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_AVRCP_LIB_H__ */
