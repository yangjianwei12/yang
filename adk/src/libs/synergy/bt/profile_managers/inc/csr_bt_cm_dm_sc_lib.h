#ifndef CSR_BT_CM_DM_SC_LIB_H__
#define CSR_BT_CM_DM_SC_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_bt_cm_prim.h"

#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_prim.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void CsrBtCmScDmAddDeviceReq(CsrBtTypedAddr  typedAddr,
                             DM_SM_TRUST_T   trust,
                             DM_SM_KEYS_T    *keys);

/* ---------------------------------------------------------------------------------------------------
   Name:
   CsrBtCmScDmRemoveDeviceReq

   Description:

   This API is used to remove authenticated devices from the paired device 
   list.This API can be used either to delete individual authenticated device or
   all non-priority authenticated devices in which case deviceAddr field in the request will be
   marked as all 0s.

   Note: the devices which are marked priority will not get deleted. In order 
   to delete such devices, the priority needs to be reset to FALSE before calling this 
   API.

   Deleting individual authenticated device.
   CM sends CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM with num_keys_deleted as 1 to indicate the 
   deletion of the device. 
   CM sends security indication CSR_BT_CM_SECURITY_EVENT_IND with event as CSR_BT_CM_SECURITY_EVENT_DEBOND 
   if application has subscribed for CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND.

   Deleting all authenticated devices.
   For each deleted device in the list, CM sends CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM with num_keys_deleted
   as 1 to indicate the deletion.
   Cm sends single security indication CSR_BT_CM_SECURITY_EVENT_IND with event as CSR_BT_CM_SECURITY_EVENT_DEBOND
   if application has subscribed for CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND. Device address will be 
   set to all 0s.

   Note: if the device to be deleted doesn't exists, CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM will
   be received with num_keys_deleted as 0.

   Parameters:
    CsrBtDeviceAddr deviceAddr         Device address to be removed. All zeros should be
                                       passed to remove all authenticated devices except
                                       priority devices.
    CsrBtAddressType addressType     - Address type of the device. Invalid address type
                                       CSR_BT_ADDR_INVALID shall be passed for removing all
                                       devices.
   -----------------------------------------------------------------------------------------------------*/
void CsrBtCmScDmRemoveDeviceReq(CsrBtDeviceAddr deviceAddr, CsrBtAddressType addressType);

#define CsrBtCmScDmCancelConnectReq(_deviceAddr) {                      \
        CsrBtCmSmCancelConnectReq *prim__ = (CsrBtCmSmCancelConnectReq *) CsrPmemAlloc(sizeof(CsrBtCmSmCancelConnectReq)); \
        prim__->type = CSR_BT_CM_SM_CANCEL_CONNECT_REQ;                 \
        prim__->deviceAddr = _deviceAddr;                               \
        CsrBtCmPutMessageDownstream(prim__);}

void CsrBtCmScDmDeleteStoredLinkKeyReq(CsrBtDeviceAddr deviceAddr, delete_all_flag_t flag);
void CsrBtCmScDmAuthenticateReq(CsrBtDeviceAddr deviceAddr);

#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
void CsrBtCmScDmEncryptionReq(CsrSchedQid appHandle, CsrBtDeviceAddr deviceAddr, CsrBool encMode);
#endif

#define CsrBtCmScDmSetSecModeReq(_mode, _mode3Enc) {                    \
        CsrBtCmSmSetSecModeReq *prim__ = (CsrBtCmSmSetSecModeReq *) CsrPmemAlloc(sizeof(CsrBtCmSmSetSecModeReq)); \
        prim__->type = CSR_BT_CM_SM_SET_SEC_MODE_REQ;                   \
        prim__->mode = _mode;                                           \
        prim__->mode3Enc = _mode3Enc;                                   \
        CsrBtCmPutMessageDownstream(prim__);}

#define CsrBtCmScDmSetDefaultSecLevelReq(_seclDefault) {                \
        CsrBtCmSmSetDefaultSecLevelReq *prim__ = (CsrBtCmSmSetDefaultSecLevelReq *) CsrPmemAlloc(sizeof(CsrBtCmSmSetDefaultSecLevelReq)); \
        prim__->type = CSR_BT_CM_SM_SET_DEFAULT_SEC_LEVEL_REQ;          \
        prim__->seclDefault = seclDefault;                              \
        CsrBtCmPutMessageDownstream(prim__);}

void CsrBtCmScDmPinRequestRes(CsrBtDeviceAddr deviceAddr, CsrUint8 pinLength, CsrUint8 *pin);
void CsrBtCmScDmPinRequestNegRes(CsrBtDeviceAddr deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmAuthoriseRes
 *
 *  DESCRIPTION
 *      This API is used to respond to CSR_BT_CM_SM_AUTHORISE_IND.
 *      This is a request from the bluestack Security Manager for authorization
 *      from the application when an untrusted or unknown device is attempting
 *      to access a service that requires authorization in security mode.
 *
 *  PARAMETERS
 *        deviceAddr:      The Bluetooth address of the remote device.
 *        incoming:        TRUE for incoming connection request. FALSE for
 *                         outgoing connection request.
 *        authorisation:   TRUE for accepted authorization. FALSE for rejected.
 *        channel:         Channel for the protocol defined by the protocolId
 *                         that the access is being requested on
 *                         (e.g. RFCOMM server channel number).
 *        protocolId:      The protocol identifier L2CAP or RFCOMM.
 *        addressType:     Type of device address e.g., Public, or random.
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmAuthoriseRes(CsrBtDeviceAddr deviceAddr,
                             CsrBool incoming,
                             CsrUint16 authorisation,
                             CsrUint16 channel,
                             dm_protocol_id_t protocolId,
                             CsrBtAddressType addressType);

#define CsrBtCmScDmAuthoriseNegRes(deviceAddr, incoming, channel, protocolId, addressType) CsrBtCmScDmAuthoriseRes(deviceAddr, incoming, FALSE, channel, protocolId, addressType)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmRegisterReq
 *
 *  DESCRIPTION
 *      This API provides option for setting security requirements 
 *      for all incoming and outgoing connections. It is recommended to use 
 *      this API for incoming connections only, as in case of outgoing
 *      connections, respective connect request APIs CmContextConnectReqSend 
 *      for RFCOMM, CmL2caConnectReqConftabSend, CmL2caTpConnectRequest for L2CAP 
 *      sets up the security requirements.
 *
 *      This API doesn't generate any response.
 *
 *  PARAMETERS
 *        protocolId:    Protocol Identifier, refer to SEC_PROTOCOL_X in
 *                       dm_prim.h for more information.
 *        channel:       Protocol channel number. E.g. Server channel for RFCOMM,
 *                       PSM for L2CAP connections.
 *        outgoingOk:    Setting this to TRUE will apply this security settings
 *                       for outgoing connections also, otherwise it will be
 *                       used only for incoming connections.
 *        securityLevel: Security level mask. Refer to section "BR/EDR security levels" 
 *                       for BR/EDR security and "LE Service Security Requirements" for 
 *                       LE security in dm_prim.h.
 *        minEncKeySize: Minimum Encryption key Size.
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmRegisterReq(dm_protocol_id_t protocolId,
                            CsrUint16 channel,
                            CsrBool outgoingOk,
                            dm_security_level_t securityLevel,
                            CsrUint8 minEncKeySize);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmUnRegisterReq
 *
 *  DESCRIPTION
 *      This API is used to unregister security requirements for a service
 *      previously registered with CsrBtCmScDmRegisterReq.
 *
 *      This API doesn't generate any response.
 *
 *  PARAMETERS
 *        protocolId:    Protocol Identifier, refer to SEC_PROTOCOL_X in
 *                       dm_prim.h for more information.
 *        channel:       Protocol channel number. E.g. Server channel for RFCOMM,
 *                       PSM for L2CAP connections.
 *----------------------------------------------------------------------------*/
#define CsrBtCmScDmUnRegisterReq(_channel, _protocolId) {               \
        CsrBtCmSmUnregisterReq *prim__ = (CsrBtCmSmUnregisterReq *) CsrPmemAlloc(sizeof(CsrBtCmSmUnregisterReq)); \
        prim__->type = CSR_BT_CM_SM_UNREGISTER_REQ;                     \
        prim__->channel = _channel;                                     \
        prim__->protocolId = _protocolId;                               \
        CsrBtCmPutMessageDownstream(prim__);}

#define CsrBtCmScDmBondingReq(_deviceAddr, _authenticationRequirements) { \
        CsrBtCmSmBondingReq *prim__ = (CsrBtCmSmBondingReq *) CsrPmemZalloc(sizeof(*prim__)); \
        prim__->type                = CSR_BT_CM_SM_BONDING_REQ;         \
        prim__->deviceAddr          = _deviceAddr;                      \
        CsrBtCmPutMessageDownstream(prim__);}

#define CsrBtCmScDmBondingCancelReq(_deviceAddr, _force, _addressType) {   \
        CsrBtCmSmBondingCancelReq *prim__ = (CsrBtCmSmBondingCancelReq *) CsrPmemZalloc(sizeof(*prim__)); \
        prim__->type         = CSR_BT_CM_SM_BONDING_CANCEL_REQ;         \
        prim__->deviceAddr   = _deviceAddr;                             \
        prim__->force        = _force;                                  \
        prim__->addressType  = _addressType;                            \
        CsrBtCmPutMessageDownstream(prim__);}

#ifdef __cplusplus
}
#endif

#endif
