#ifndef CSR_BT_CM_DM_SC_SSP_LIB_H__
#define CSR_BT_CM_DM_SC_SSP_LIB_H__
/******************************************************************************
 Copyright (c) 2008-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_types.h"
#include "bluetooth.h"
#include "csr_bt_cm_dm_sc_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CSR_BT_INSTALL_CM_SC_MODE_CONFIG
void CsrBtCmScDmSecModeConfigReq(CsrUint16 writeAuthEnable, CsrUint16 config);
#endif

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmIoCapabilityRequestRes
 *
 *  DESCRIPTION
 *      This API is used to accept CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND
 *      by providing appropriate IO capabilities.
 *
 *  PARAMETERS
 *        deviceAddr:       The Bluetooth address of the remote device.
 *        addressType:      Type of device address e.g., Public, or random.
 *        transportType:    Type of bluetooth transport, refer to CsrBtTransportType
 *        ioCapability:     Apart from IO capabilities from specification,this
 *                          field's MSB is utilized to indicate application flexibility
 *                          on MITM requirement when MITM flag is set in
 *                          authentication_requirements field. At present, MITM
 *                          flexibility is supported only for LE.
 *        authenticationRequirements:   Bit mask used to request authentication
 *                                      requirements of the user. 
 *        oobDataPresent:   Type of OOB data which is present. Refer to dm_prim.h.
 *        oobHashC:         The remote device's out of band hash value
 *                          (must be set NULL if unavailable). For BLE legacy pairing,
 *                          only the oob_hash_c is required.
 *        oobRandR:         The remote device's out of band random value
 *                          (must be set NULL if unavailable).
 *        keyDistribution:  Bit mask used to specify keys to be exchanged with
 *                          remote device during LE pairing.
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmIoCapabilityRequestRes(CsrBtDeviceAddr deviceAddr,
                                       CsrBtAddressType addressType,
                                       CsrBtTransportType transportType,
                                       CsrUint8 ioCapability,
                                       CsrUint8 authenticationRequirements,
                                       CsrUint8 oobDataPresent,
                                       CsrUint8 *oobHashC,
                                       CsrUint8 *oobRandR,
                                       CsrUint16 keyDistribution);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmIoCapabilityRequestNegRes
 *
 *  DESCRIPTION
 *      This API is used to reject CSR_BT_CM_SM_IO_CAPABILITY_REQUEST_IND
 *      by providing appropriate error code in the reason field.
 *
 *  PARAMETERS
 *        deviceAddr:      The Bluetooth address of the remote device.
 *        addressType:     Type of device address e.g., Public, or random.
 *        transportType:   Type of bluetooth transport, refer to CsrBtTransportType
 *        reason:          Reason indicating why the request is rejected.
 *                         See hci_error_t in hci.h.
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmIoCapabilityRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                          CsrBtAddressType addressType,
                                          CsrBtTransportType transportType,
                                          hci_error_t reason);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmUserConfirmationRequestRes
 *
 *  DESCRIPTION
 *      This API is used to accept CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND.
 *
 *  PARAMETERS
 *        deviceAddr:    The Bluetooth address of the remote device.
 *        addressType:   Type of device address e.g., Public, or random.
 *        transportType: Type of bluetooth transport, refer to CsrBtTransportType
 *----------------------------------------------------------------------------*/
#define CsrBtCmScDmUserConfirmationRequestRes(_deviceAddr, _addressType, _transportType) {\
        CsrBtCmSmUserConfirmationRequestRes *prim__ = (CsrBtCmSmUserConfirmationRequestRes *) CsrPmemZalloc(sizeof(*prim__)); \
        prim__->type       = CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_RES; \
        prim__->deviceAddr = _deviceAddr;                               \
        prim__->reason     = HCI_SUCCESS; /* user accepted */           \
        prim__->addressType = _addressType;                             \
        prim__->transportType = _transportType;                         \
        CsrBtCmPutMessageDownstream(prim__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmUserConfirmationRequestNegRes
 *
 *  DESCRIPTION
 *      This API is used to reject CSR_BT_CM_SM_USER_CONFIRMATION_REQUEST_IND.
 *
 *  PARAMETERS
 *        deviceAddr:    The Bluetooth address of the remote device.
 *        addressType:   Type of device address e.g., Public, or random.
 *        transportType: Type of bluetooth transport, refer to CsrBtTransportType
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmUserConfirmationRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                              CsrBtAddressType addressType,
                                              CsrBtTransportType transportType);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmUserPasskeyRequestRes
 *
 *  DESCRIPTION
 *      This API is used to accept CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND.
 *
 *  PARAMETERS
 *        deviceAddr:    The Bluetooth address of the remote device.
 *        addressType:   Type of device address e.g., Public, or random.
 *        transportType: Type of bluetooth transport, refer to CsrBtTransportType
 *        numericValue:  Passkey value.
 *----------------------------------------------------------------------------*/
#define CsrBtCmScDmUserPasskeyRequestRes(_deviceAddr, _addressType, _transportType, _numericValue) {\
        CsrBtCmSmUserPasskeyRequestRes *prim__ = (CsrBtCmSmUserPasskeyRequestRes *) CsrPmemZalloc(sizeof(*prim__)); \
        prim__->type         = CSR_BT_CM_SM_USER_PASSKEY_REQUEST_RES;   \
        prim__->deviceAddr   = _deviceAddr;                             \
        prim__->addressType = _addressType;                             \
        prim__->transportType = _transportType;                         \
        prim__->numericValue = _numericValue;                           \
        CsrBtCmPutMessageDownstream(prim__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmUserPasskeyRequestNegRes
 *
 *  DESCRIPTION
 *      This API is used to reject CSR_BT_CM_SM_USER_PASSKEY_REQUEST_IND.
 *
 *  PARAMETERS
 *        deviceAddr:    The Bluetooth address of the remote device.
 *        addressType:   Type of device address e.g., Public, or random.
 *        transportType: Type of bluetooth transport, refer to CsrBtTransportType
 *----------------------------------------------------------------------------*/
void CsrBtCmScDmUserPasskeyRequestNegRes(CsrBtDeviceAddr deviceAddr,
                                         CsrBtAddressType addressType,
                                         CsrBtTransportType transportType);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmReadLocalOobDataReq
 *
 *  DESCRIPTION
 *      This API is used to read local OOB data which could be used by 
 *      remote device for authentication. Application not supporting LESC
 *      shall not be calling into this API for LE transport.
 *
 *  PARAMETERS
 *        transportType: Type of bluetooth transport, refer to CsrBtTransportType
 *----------------------------------------------------------------------------*/
#define CsrBtCmScDmReadLocalOobDataReq(_transportType)                              \
    do {                                                                            \
        CsrBtCmSmReadLocalOobDataReq *prim__ = (CsrBtCmSmReadLocalOobDataReq *) CsrPmemZalloc(sizeof(*prim__));  \
        prim__->type             = CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_REQ;            \
        prim__->transportType = _transportType;                                     \
        CsrBtCmPutMessageDownstream(prim__);                                        \
    } while (0)


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScDmSendKeypressNotificationReq
 *
 *  DESCRIPTION
 *      This API is used to tell the remote device about keypresses during
 *      local passkey entry.
 *
 *  PARAMETERS
 *        deviceAddr:       The Bluetooth address of the remote device.
 *        addressType:      Type of device address e.g., Public, or random.
 *        transportType:    Type of bluetooth transport, refer to CsrBtTransportType
 *        notificationType: type of notification.
 *----------------------------------------------------------------------------*/
#define CsrBtCmScDmSendKeypressNotificationReq(_deviceAddr, _addressType, _transportType, _notificationType) {\
        CsrBtCmSmSendKeypressNotificationReq *prim__ = (CsrBtCmSmSendKeypressNotificationReq *) CsrPmemZalloc(sizeof(*prim__)); \
        prim__->type = CSR_BT_CM_SM_SEND_KEYPRESS_NOTIFICATION_REQ; \
        prim__->deviceAddr = _deviceAddr;                           \
        prim__->addressType = _addressType;                         \
        prim__->transportType = _transportType;                     \
        prim__->notificationType = _notificationType;               \
        CsrBtCmPutMessageDownstream(prim__);}

#ifdef INSTALL_CM_SM_REPAIR
void CsrBtCmSmSendRepairRes(CsrBtDeviceAddr deviceAddr,
                            CsrBool accept,
                            CsrUint16 repairId,
                            CsrBtAddressType addressType);
#endif /* INSTALL_CM_SM_REPAIR */

#else
#define CsrBtCmScDmIoCapabilityRequestRes(deviceAddr, addressType, transportType, ioCapability, authenticationRequirements, oobDataPresent,oobHashC, oobRandR)
#define CsrBtCmScDmIoCapabilityRequestNegRes(deviceAddr, addressType, transportType, reason)
#define CsrBtCmScDmUserConfirmationRequestRes(deviceAddr, addressType, transportType)
#define CsrBtCmScDmUserConfirmationRequestNegRes(deviceAddr, addressType, transportType)
#define CsrBtCmScDmUserPasskeyRequestRes(deviceAddr,  addressType, transportType, numericValue)
#define CsrBtCmScDmUserPasskeyRequestNegRes(deviceAddr, addressType, transportType)
#define CsrBtCmScDmReadLocalOobDataReq()
#define CsrBtCmScDmSendKeypressNotificationReq(deviceAddr, addressType, TransportType, notificationType)
#define CsrBtCmSmSendRepairRes(deviceAddr, accept, repairId, addressType)
#endif

#ifdef __cplusplus
}
#endif

#endif
