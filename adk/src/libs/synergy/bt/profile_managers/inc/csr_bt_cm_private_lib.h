#ifndef CSR_BT_CM_PRIVATE_LIB_H__
#define CSR_BT_CM_PRIVATE_LIB_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/


#include "csr_synergy.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_private_prim.h"
#endif
#include "csr_bt_cm_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*
 *  LE local controller features is 64 bits long.
 *  CSR_BT_LE_LOCAL_FEATURE_SUPPORTED macro is used to check individual feature
 *  bit from the feature set of 64 bitmask; this macro manipulates the feature
 *  set split into csrUint8; 8bits and returns non-zero (not necessarily 1) for
 *  a supported feature. If a feature is present the bit is set.
 *---------------------------------------------------------------------------*/
#ifdef CSR_BT_LE_ENABLE
/* Macro for local controller supported LL privacy feature */
#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
#define CSR_BT_LE_FEATURE_LL_PRIVACY            ULP_FEATURE_LL_PRIVACY
#endif /* End of CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
/* Macro for local controller supported LE Extended Advertising feature */
#define LE_FEATURE_EXTENDED_ADVERTISING         ULP_FEATURE_EXTENDED_ADVERTISING

/* Query macro */
#define CSR_BT_LE_LOCAL_FEATURE_MASK(_feature)  (1U << ((_feature > 7) ? (_feature - 8) : _feature))
#define CSR_BT_LE_LOCAL_FEATURE_INDEX(_feature) (_feature >> 3)

#define CSR_BT_LE_LOCAL_FEATURE_SUPPORTED(_featureset, _feature) \
    ((_featureset)[CSR_BT_LE_LOCAL_FEATURE_INDEX(_feature)] \
      & CSR_BT_LE_LOCAL_FEATURE_MASK(_feature))
#endif /* End of CSR_BT_LE_ENABLE */

#define SniffSettingsEq(set1, set2) ((CsrBool) (CsrMemCmp(set1, set2, sizeof(CsrBtSniffSettings)) ? FALSE : TRUE))

#ifdef INSTALL_CM_INQUIRY
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmInquiryExtReqSend
 *
 *  DESCRIPTION
 *      Initiate an inquiry to discover other Bluetooth devices.
 *      Inquiry resuts will be sent to application through CSR_BT_CM_INQUIRY_RESULT_IND and
 *      on completion CSR_BT_CM_INQUIRY_CFM will be sent to the application.
 *
 *  PARAMETERS
 *        phandle:             Identity of the calling process
 *        inquiryAccessCode:   The inquiry access code to use (CSR_BT_CM_ACCESS_CODE_*) defined in csr_bt_cm_prim.h
 *        inquiryTxPowerLevel: TX power level to be used when inquiring only
 *        configMask:          The inquiry config mask to use (CSR_BT_CM_INQUIRY_CONFIG_*) defined in csr_bt_cm_prim.h 
 *        maxResponses:        Maximum number of responses after which inquiry will be terminated.   
 *        inquiryTime:         Maximum amount of time for inquiry before it is terminated. 
 *----------------------------------------------------------------------------*/
CsrBtCmInquiryReq *CsrBtCmInquiryExtReq_struct(CsrSchedQid phandle,
                                               CsrUint24   inquiryAccessCode,
                                               CsrInt8     inquiryTxPowerLevel,
                                               CsrUint32   configMask,
                                               CsrUint8    maxResponses,
                                               CsrUint8    inquiryTimeout);

#define CsrBtCmInquiryExtReqSend(_phandle, _inquiryAccessCode, _inquiryTxpowerLevel, _configMask, \
                                 _maxResponses, _inquiryTimeout) { \
        CsrBtCmInquiryReq *msg__;                                       \
        msg__=CsrBtCmInquiryExtReq_struct(_phandle, _inquiryAccessCode, _inquiryTxpowerLevel, _configMask, \
                                          _maxResponses, _inquiryTimeout); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmInquiryReqSend
 *
 *  DESCRIPTION
 *      Initiate an inquiry to discover other Bluetooth devices.
 *      Inquiry resuts will be sent to application through CSR_BT_CM_INQUIRY_RESULT_IND and
 *      inquiry process would continue until cancelled by the application.
 *
 *  PARAMETERS
 *        phandle:            Identity of the calling process
 *        inquiryAccessCode:  The inquiry access code to use (CSR_BT_CM_ACCESS_CODE_*) defined in csr_bt_cm_prim.h
 *----------------------------------------------------------------------------*/
CsrBtCmInquiryReq *CsrBtCmInquiryReq_struct(CsrSchedQid phandle,
                                            CsrUint24   inquiryAccessCode,
                                            CsrInt8     inquiryTxPowerLevel,
                                            CsrUint32   configMask);

#define CsrBtCmInquiryReqSend(_phandle, _inquiryAccessCode, _inquiryTxpowerLevel, _configMask) \
        CsrBtCmInquiryExtReqSend(_phandle, _inquiryAccessCode, _inquiryTxpowerLevel, _configMask, CSR_BT_UNLIMITED, HCI_INQUIRY_LENGTH_MAX)


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelInquiryReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        phandle:            protocol handle
 *----------------------------------------------------------------------------*/
CsrBtCmCancelInquiryReq *CsrBtCmCancelInquiryReq_struct(CsrSchedQid  phandle);

#define CsrBtCmCancelInquiryReqSend(_phandle) {         \
        CsrBtCmCancelInquiryReq *msg__;                 \
        msg__=CsrBtCmCancelInquiryReq_struct(_phandle); \
        CsrBtCmPutMessageDownstream(msg__);}

#endif /* INSTALL_CM_INQUIRY */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadIacReqSend
 *
 *  DESCRIPTION
 *      Submits to read the current Inquiry Access Code of the BC chip
 *
 *  PARAMETERS
 *        appHandle:             application handle
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_IAC
CsrBtCmReadIacReq *CsrBtCmReadIacReq_struct(CsrSchedQid    appHandle);

#define CsrBtCmReadIacReqSend(_appHandle) {             \
        CsrBtCmReadIacReq *msg__;                       \
        msg__=CsrBtCmReadIacReq_struct(_appHandle);     \
        CsrBtCmPutMessageDownstream(msg__);}
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteIacReqSend
 *
 *  DESCRIPTION
 *      Submits to write a new Inquiry Access Code to the BC chip
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *      iac:                 The new Inquiry Access Code
 *----------------------------------------------------------------------------*/
CsrBtCmWriteIacReq *CsrBtCmWriteIacReq_struct(CsrSchedQid appHandle,
                                              CsrUint24 iac);

#define CsrBtCmWriteIacReqSend(_appHandle, _iac) {              \
        CsrBtCmWriteIacReq *msg__;                              \
        msg__=CsrBtCmWriteIacReq_struct(_appHandle, _iac);      \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcOpenReqSend
 *
 *  DESCRIPTION
 *      Submits to open a SDC channel. Must only be used if application starts
 *      reading Attr direct hereafter
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_SDC
CsrBtCmSdcOpenReq *CsrBtCmSdcOpenReq_struct(CsrSchedQid appHandle,
                                            CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcOpenReqSend(_appHandle, _deviceAddr) {                \
        CsrBtCmSdcOpenReq *msg__;                                       \
        msg__=CsrBtCmSdcOpenReq_struct(_appHandle, _deviceAddr);        \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelOpenReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_OPEN_REQ
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
CsrBtCmSdcCancelSearchReq *CsrBtCmSdcCancelOpenReq_struct(CsrSchedQid appHandle,
                                                          CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcCancelOpenReqSend(_appHandle, _deviceAddr) {          \
        CsrBtCmSdcCancelSearchReq *msg__;                               \
        msg__=CsrBtCmSdcCancelOpenReq_struct(_appHandle, _deviceAddr);  \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmAlwaysSupportMasterRoleReqSend
 *
 *  DESCRIPTION
 *      Make sure that Synergy BT always tries to be MASTER, even if it only got one
 *      ACL connection
 *
 *  PARAMETERS
 *        alwaysSupportMasterRole:
 *              TRUE then Synergy BT always tries to be master. FALSE then BCHS
 *              only tries to be MASTER if it got more than one ACL connection
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_ALWAYS_SUPPORT_MASTER_ROLE
CsrBtCmAlwaysSupportMasterRoleReq *CsrBtCmAlwaysSupportMasterRoleReq_struct(CsrBool  alwaysSupportMasterRole);

#define CsrBtCmAlwaysSupportMasterRoleReqSend(_alwaysSupportMasterRole) { \
        CsrBtCmAlwaysSupportMasterRoleReq *msg__;                       \
        msg__=CsrBtCmAlwaysSupportMasterRoleReq_struct(_alwaysSupportMasterRole); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelAcceptConnectReqSend
 *      CsrBtCmContextCancelAcceptConnectReqSend
 *
 *  DESCRIPTION
 *      ...

 *
 *  PARAMETERS
 *        phandle:                protocol handle
 *        serverChannel:          local server channel
 *        context                 Opaque context number returned only if there
 *                                is one to cancel otherwise the context
 *                                given in CsrBtCmContextConnectAcceptReqSend
 *----------------------------------------------------------------------------*/
CsrBtCmCancelAcceptConnectReq *CsrBtCmCancelAcceptConnectReq_struct(CsrSchedQid          phandle,
                                                                    CsrUint8   serverChannel,
                                                                    CsrUint16       context);

#define CsrBtCmContextCancelAcceptConnectReqSend(_phandle, _serverChannel, _context) { \
        CsrBtCmCancelAcceptConnectReq *msg__;                           \
        msg__=CsrBtCmCancelAcceptConnectReq_struct(_phandle, _serverChannel, _context); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCmCancelAcceptConnectReqSend(_phandle, _serverChannel) CsrBtCmContextCancelAcceptConnectReqSend(_phandle, _serverChannel, CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmConnectReqSend
 *      CsrBtCmContextConnectReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        localServerCh:        local server channel
 *      serviceHandle:        The handle of the service
 *        profileMaxFrameSize:maximum frame size (bytes)
 *        requestPortPar:
 *        validportPar:
 *        portPar:            Port parameter typedefs
 *        secLevel:           Level of security to be applied
 *        CsrBtDeviceAddr:    deviceAddr
 *        context             Opaque context number
 *        modemStatus         modem status data
 *        breakSignal         break signal data
 *        mscTimeout          Time in msec that the RFC shall wait for MSC
 *        minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmConnectReq *CsrBtCmConnectReq_struct(CsrSchedQid          appHandle,
                                            CsrUint8         localServerCh,
                                            CsrBtUuid32           serviceHandle,
                                            CsrUint16             profileMaxFrameSize,
                                            CsrBool               requestPortPar,
                                            CsrBool               validportPar,
                                            RFC_PORTNEG_VALUES_T   portPar,
                                            dm_security_level_t   secLevel,
                                            CsrBtDeviceAddr       deviceAddr,
                                            CsrUint16             context,
                                            CsrUint8            modemStatus,
                                            CsrUint8            breakSignal,
                                            CsrUint8            mscTimeout,
                                            CsrUint8            minEncKeySize);

#define CsrBtCmContextConnectReqSend(_appHandle,_localServerCh,_serviceHandle,_profileMaxFrameSize,_requestPortPar,_validportPar, \
                                     _portPar,_secLevel,_deviceAddr, _context, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize) { \
        CsrBtCmConnectReq *msg__;                                       \
        msg__=CsrBtCmConnectReq_struct(_appHandle,_localServerCh,_serviceHandle,_profileMaxFrameSize,_requestPortPar, \
                                       _validportPar,_portPar,_secLevel,_deviceAddr, _context, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCmConnectReqSend(_appHandle,_localServerCh,_serviceHandle,_profileMaxFrameSize,_requestPortPar, \
                              _validportPar,_portPar,_secLevel,_deviceAddr, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize) \
    CsrBtCmContextConnectReqSend(_appHandle,_localServerCh,_serviceHandle,_profileMaxFrameSize,_requestPortPar,_validportPar, \
                                 _portPar,_secLevel,_deviceAddr, CSR_BT_CM_CONTEXT_UNUSED, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmConnectExtReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        localServerCh:        local server channel
 *      remoteServerCh:     remote server channel
 *        profileMaxFrameSize:maximum frame size (bytes)
 *        requestPortPar:
 *        validportPar:
 *        portPar:            Port parameter typedefs
 *        secLevel:           Level of security to be applied
 *        CsrBtDeviceAddr:        deviceAddr
 *        modemStatus         modem status data
 *        breakSignal         break signal data
 *        mscTimeout          Time in msec that the RFC shall wait for MSC
 *        minEncKeySize         Minimum encryption key size
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_CONNECT_EXT
CsrBtCmConnectExtReq *CsrBtCmConnectExtReq_struct(CsrSchedQid           appHandle,
                                                  CsrUint8       localServerChannel,
                                                  CsrUint8       remoteServerChannel,
                                                  CsrUint16            profileMaxFrameSize,
                                                  CsrBool              requestPortPar,
                                                  CsrBool              validPortPar,
                                                  RFC_PORTNEG_VALUES_T portPar,
                                                  dm_security_level_t secLevel,
                                                  CsrBtDeviceAddr        deviceAddr,
                                                  CsrUint8            modemStatus,
                                                  CsrUint8            breakSignal,
                                                  CsrUint8            mscTimeout,
                                                  CsrUint8            minEncKeySize);

#define CsrBtCmConnectExtReqSend(_appHandle,_localServerChannel,_remoteServerChannel,_profileMaxFrameSize,_requestPortPar, \
                                 _validportPar,_portPar,_secLevel,_deviceAddr, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize) { \
        CsrBtCmConnectExtReq *msg__;                                    \
        msg__=CsrBtCmConnectExtReq_struct(_appHandle,_localServerChannel,_remoteServerChannel,_profileMaxFrameSize,_requestPortPar, \
                                          _validportPar,_portPar,_secLevel,_deviceAddr, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelConnectReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        localServerCh:        local server channel
 *        CsrBtDeviceAddr:        deviceAddr
 *----------------------------------------------------------------------------*/
CsrBtCmCancelConnectReq *CsrBtCmCancelConnectReq_struct(CsrSchedQid     appHandle,
                                                        CsrUint8 localServerChannel,
                                                        CsrBtDeviceAddr  deviceAddr);

#define CsrBtCmCancelConnectReqSend(_appHandle,_localServerChannel,_deviceAddr) { \
        CsrBtCmCancelConnectReq *msg__;                                 \
        msg__=CsrBtCmCancelConnectReq_struct(_appHandle,_localServerChannel,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelConnectExtReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        localServerCh:        local server channel
 *        CsrBtDeviceAddr:        deviceAddr
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_CONNECT_EXT
CsrBtCmCancelConnectReq *CsrBtCmCancelConnectExtReq_struct(CsrSchedQid     appHandle,
                                                           CsrUint8 localServerChannel,
                                                           CsrBtDeviceAddr  deviceAddr);

#define CsrBtCmCancelConnectExtReqSend(_appHandle,_localServerChannel,_deviceAddr) { \
        CsrBtCmCancelConnectReq *msg__;                                 \
        msg__=CsrBtCmCancelConnectExtReq_struct(_appHandle,_localServerChannel,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmConnectAcceptReqSend
 *      CsrBtCmContextConnectAcceptReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        classOfDevice:        The Class Of Device of the profile
 *        timeout:              connection establish time out timer (range: secs)
 *        profileMaxFrameSize:  maximum frame size (bytes)
 *        serverChannel:        local server channel
 *        secLevel:             Level of security to be applied
 *        profileUuid:          The local profile Uuid
 *        context:              Opaque context number
 *        modemStatus:          modem status data
 *        breakSignal:          break signal data
 *        mscTimeout:           Time in msec that the RFC shall wait for MSC
 *        devAddr:              device address
 *        minEncKeySize         Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmConnectAcceptReq *CsrBtCmConnectAcceptReq_struct(CsrSchedQid appHandle,
                                                        CsrUint24 classOfDevice,
                                                        CsrUint16 timeout,
                                                        CsrUint16 profileMaxFrameSize,
                                                        CsrUint8 serverChannel,
                                                        dm_security_level_t secLevel,
                                                        uuid16_t profileUuid,
                                                        CsrUint16 context,
                                                        CsrUint8 modemStatus,
                                                        CsrUint8 breakSignal,
                                                        CsrUint8 mscTimeout,
                                                        CsrBtDeviceAddr devAddr,
                                                        CsrUint8 minEncKeySize);

#define CsrBtCmConnectAcceptReqSend(_appHandle,_classOfDevice,_timeout,_profileMaxFrameSize,_serverChannel,_secLevel,_profileUuid, _modemStatus, _breakSignal, _mscTimeout, _minEncKeySize) \
do                                                                                                                                                                          \
{                                                                                                                                                                           \
    CsrBtDeviceAddr zeroBdAddr__ = { 0,0,0 };                                                                                                                               \
    CsrBtCmContextConnectAcceptReqSend(_appHandle,_classOfDevice,_timeout,_profileMaxFrameSize,_serverChannel,_secLevel,                                                    \
                                       _profileUuid, CSR_BT_CM_CONTEXT_UNUSED, _modemStatus, _breakSignal, _mscTimeout, zeroBdAddr__, _minEncKeySize)                                       \
} while (0)

#define CsrBtCmContextConnectAcceptReqSend(_appHandle,_classOfDevice,_timeout,_profileMaxFrameSize,_serverChannel,_secLevel, \
                                          _profileUuid, _context, _modemStatus, _breakSignal, _mscTimeout, _devAddr, _minEncKeySize) { \
        CsrBtCmConnectAcceptReq *msg__;                                 \
        msg__=CsrBtCmConnectAcceptReq_struct(_appHandle,_classOfDevice,_timeout,_profileMaxFrameSize,_serverChannel,_secLevel, \
                                             _profileUuid, _context, _modemStatus, _breakSignal, _mscTimeout, _devAddr, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRegisterReqSend
 *      CsrBtCmContextRegisterReqSend
 *  DESCRIPTION
 *      ....
 *
 *  PARAMETERS
 *        phandle:            protocol handle
 *        context             Opaque context number returned in CsrBtCmRegisterCfm
 *----------------------------------------------------------------------------*/
#define CsrBtCmContextRegisterReqSend(_phandle, _context) CsrBtCmPublicRegisterReqSend(_phandle, _context, CSR_BT_CM_SERVER_CHANNEL_DONT_CARE)

#define CsrBtCmRegisterReqSend(_phandle) CsrBtCmContextRegisterReqSend(_phandle, CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDisconnectReqSend
 *      CsrBtCmContextDisconnectReqSend
 *
 *  DESCRIPTION
 *      ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        context               Opaque context number returned only if no
 *                              connection is present otherwise the context
 *                              given in CsrBtCmExtConnectAcceptReqSend or
 *                              CsrBtCmContextConnectReqSend is return
 *----------------------------------------------------------------------------*/
CsrBtCmDisconnectReq *CsrBtCmDisconnectReq_struct(CsrBtConnId  btConnId,
                                                  CsrUint16      context);

#define CsrBtCmContextDisconnectReqSend(_btConnId, _context) {  \
        CsrBtCmDisconnectReq *msg__;                            \
        msg__=CsrBtCmDisconnectReq_struct(_btConnId, _context); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCmDisconnectReqSend(_btConnId) CsrBtCmContextDisconnectReqSend(_btConnId, CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScoConnectReqSend
 *
 *  DESCRIPTION
 *      ......
 *
 *  PARAMETERS
 *      btConnId              connection ID
 *      parms
 *      parmsLen
 *----------------------------------------------------------------------------*/
CsrBtCmScoConnectReq *CsrBtCmScoConnectReq_struct(CsrSchedQid            appHandle,
                                                  CsrUint8         pcmSlot,
                                                  CsrBool          pcmReassign,
                                                  CsrBtCmScoCommonParms       *parms,
                                                  CsrUint16        parmsLen,
                                                  CsrBtConnId btConnId);

#define CsrBtCmScoConnectReqSend(_appHandle, _pcmSlot, _pcmReassign, _parms, _parmsLen, _btConnId) { \
        CsrBtCmScoConnectReq *msg__;                                    \
        msg__=CsrBtCmScoConnectReq_struct(_appHandle, _pcmSlot, _pcmReassign, _parms, _parmsLen, _btConnId); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScoAcceptConnectReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *      btConnId              connection ID
 *        audioQuality:
 *      txBandwidth
 *      rxBandwidth
 *      maxLatency
 *      voiceSettings
 *      reTxEffort
 *----------------------------------------------------------------------------*/
CsrBtCmScoAcceptConnectReq *CsrBtCmScoAcceptConnectReq_struct(CsrSchedQid            appHandle,
                                                              CsrBtConnId btConnId,
                                                              hci_pkt_type_t theAudioQuality,
                                                              CsrUint32       theTxBandwidth,
                                                              CsrUint32       theRxBandwidth,
                                                              CsrUint16       theMaxLatency,
                                                              CsrUint16       theVoiceSettings,
                                                              CsrUint8        theReTxEffort);

#define CsrBtCmScoAcceptConnectReqSend(_appHandle, _btConnId,_audioQuality,_txBandwidth,_rxBandWidth,_maxLatency,_voiceSettings,_reTxEffort) { \
        CsrBtCmScoAcceptConnectReq *msg__;                              \
        msg__=CsrBtCmScoAcceptConnectReq_struct(_appHandle, _btConnId,_audioQuality,_txBandwidth,_rxBandWidth,_maxLatency,_voiceSettings,_reTxEffort); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmMapScoPcmResSend
 *
 *  DESCRIPTION
 *      ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *----------------------------------------------------------------------------*/
CsrBtCmMapScoPcmRes *CsrBtCmMapScoPcmRes_struct(CsrBtConnId btConnId,
                                                hci_error_t             acceptResponse,
                                                CsrBtCmScoCommonParms   *parms,
                                                CsrUint8                 thePcmSlot,
                                                CsrBool                  thePcmReassign);

#define CsrBtCmMapScoPcmResSend(_btConnId,_acceptResponse,_parms,_pcmSlot,_pcmReassign) { \
        CsrBtCmMapScoPcmRes *msg__;                                     \
        msg__=CsrBtCmMapScoPcmRes_struct(_btConnId,_acceptResponse,_parms,_pcmSlot,_pcmReassign); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtcmScoRenegotiateReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        audioQuality:
 *      maxLatency
 *      reTxEffort
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_SCO_RENEGOTIATE
CsrBtCmScoRenegotiateReq *CsrBtCmScoRenegotiateReq_struct(CsrSchedQid            appHandle,
                                                          CsrBtConnId btConnId,
                                                          hci_pkt_type_t theAudioQuality,
                                                          CsrUint16       theMaxLatency,
                                                          CsrUint8        theReTxEffort);

#define CsrBtcmScoRenegotiateReqSend(_appHandle, _btConnId,_audioQuality,_maxLatency,_reTxEffort) { \
        CsrBtCmScoRenegotiateReq *msg__;                                \
        msg__=CsrBtCmScoRenegotiateReq_struct(_appHandle, _btConnId,_audioQuality,_maxLatency,_reTxEffort); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScoDisconnectReqSend
 *
 *  DESCRIPTION
 *      ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *----------------------------------------------------------------------------*/
CsrBtCmScoDisconnectReq *CsrBtCmScoDisconnectReq_struct(CsrSchedQid            appHandle,
                                                        CsrBtConnId btConnId);

#define CsrBtCmScoDisconnectReqSend(_appHandle, _btConnId) {            \
        CsrBtCmScoDisconnectReq *msg__;                                 \
        msg__=CsrBtCmScoDisconnectReq_struct(_appHandle, _btConnId);    \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScoCancelReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *----------------------------------------------------------------------------*/
CsrBtCmScoCancelAcceptConnectReq *CsrBtCmScoCancelReq_struct(CsrSchedQid            appHandle,
                                                             CsrBtConnId btConnId);


#define CsrBtCmScoCancelReqSend(_appHandle, _btConnId) {                \
        CsrBtCmScoCancelAcceptConnectReq *msg__;                        \
        msg__=CsrBtCmScoCancelReq_struct(_appHandle, _btConnId);        \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDataReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        payloadLength:        length of the payload
 *        *payload:            pointer to the data
 *----------------------------------------------------------------------------*/
CsrBtCmDataReq *CsrBtCmDataReq_struct(CsrBtConnId btConnId,
                                      CsrUint16      payloadLength,
                                      CsrUint8      *payload );

#define CsrBtCmDataReqSend(_btConnId,_payloadLength,_payload) {         \
        CsrBtCmDataReq *msg__;                                          \
        msg__=CsrBtCmDataReq_struct(_btConnId,_payloadLength,_payload); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDataResSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *----------------------------------------------------------------------------*/
CsrBtCmDataRes *CsrBtCmDataRes_struct(CsrBtConnId btConnId);

#define CsrBtCmDataResSend(_btConnId) {         \
        CsrBtCmDataRes *msg__;                  \
        msg__=CsrBtCmDataRes_struct(_btConnId); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmControlReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        modemstatus:        modemstatus
 *        break_signal        break signal (7 LSBs)
 *----------------------------------------------------------------------------*/
CsrBtCmControlReq *CsrBtCmControlReq_struct(CsrBtConnId btConnId,
                                            CsrUint8        theModemstatus,
                                            CsrUint8        theBreakSignal);

#define CsrBtCmControlReqSend(_btConnId,_modemStatus,_breakSignal) {    \
        CsrBtCmControlReq *msg__;                                       \
        msg__=CsrBtCmControlReq_struct(_btConnId,_modemStatus,_breakSignal); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRfcModeChangeReqSend
 *      CsrBtCmRfcForceModeChangeReqSend
 *
 *  DESCRIPTION
 *        If a connection exists, CSR_BT_CM_RFC_MODE_CHANGE_REQ message 
 *        requests a change of the link mode to Active (no power saving mode) or 
 *        Sniff mode. The application would receive CSR_BT_CM_MODE_CHANGE_IND if
 *        the message is successfully processed else CSR_BT_CM_RFC_MODE_CHANGE_IND 
 *        is sent to the application with result. The message will be ignored 
 *        if connection does not exist.
 *
 *  PARAMETERS
 *        btConnId:           connection ID
 *        requestedMode:      Requested link policy mode (CSR_BT_ACTIVE_MODE / CSR_BT_SNIFF_MODE)
 *        forceSniff:         Currently unused
 *----------------------------------------------------------------------------*/
CsrBtCmRfcModeChangeReq *CsrBtCmRfcModeChangeReq_struct(CsrBtConnId btConnId,
                                                        CsrUint8 theMode,
                                                        CsrBool forceSniff);

#define CsrBtCmRfcModeChangeReqSend(_btConnId,_modemStatus) {           \
        CsrBtCmRfcModeChangeReq *msg__;                                 \
        msg__=CsrBtCmRfcModeChangeReq_struct(_btConnId,_modemStatus, FALSE); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDmModeSettingsReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        theMode:            Requested link policy mode
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS
CsrBtCmDmModeSettingsReq *CsrBtCmDmModeSettingsReq_struct(CsrBtConnId btConnId,
                                                          CsrBtSniffSettings               *theSniffSettings,
                                                          CsrBtSsrSettingsDownstream      *theSsrSettings,
                                                          CsrUint8                        lowPowerPriority);

#if CSR_BT_BT_VERSION >= CSR_BT_BLUETOOTH_VERSION_2P1
#define CsrBtCmDmModeSettingsReqSend(_btConnId,_sniffSettings,_ssrSettings,_lowPowerPriority) { \
        CsrBtCmDmModeSettingsReq *msg__;                                \
        msg__=CsrBtCmDmModeSettingsReq_struct(_btConnId,_sniffSettings,_ssrSettings,_lowPowerPriority); \
        CsrBtCmPutMessageDownstream(msg__);}
#else
#define CsrBtCmDmModeSettingsReqSend(_btConnId,_sniffSettings,_ssrSettings,_lowPowerPriority)
#endif
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmPortnegResSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        PortVar:            Port parameter
 *----------------------------------------------------------------------------*/
CsrBtCmPortnegRes *CsrBtCmPortnegRes_struct(CsrBtConnId btConnId,
                                            RFC_PORTNEG_VALUES_T *thePortVar);

#define CsrBtCmPortnegResSend(_btConnId,_portPar) {             \
        CsrBtCmPortnegRes *msg__;                               \
        msg__=CsrBtCmPortnegRes_struct(_btConnId,_portPar);     \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *
 *      CsrBtCml2caCancelConnectReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        CsrBtDeviceAddr:        deviceAddr
 *      localPsm:            The local PSM channel
 *----------------------------------------------------------------------------*/
CsrBtCmCancelL2caConnectReq *CsrBtCml2caCancelConnectReq_struct(CsrSchedQid    appHandle,
                                                                CsrBtDeviceAddr deviceAddr,
                                                                psm_t        localPsm);

#define CsrBtCml2caCancelConnectReqSend(_appHandle,_deviceAddr,_localPsm) { \
        CsrBtCmCancelL2caConnectReq *msg__;                             \
        msg__=CsrBtCml2caCancelConnectReq_struct(_appHandle,_deviceAddr,_localPsm); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caConnectReqSend
 *      CsrBtCmJsr82l2caConnectReqSend
 *      CsrBtCml2caConnectHighDataPriorityReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      appHandle:          Protocol handle
 *      deviceAddr:         BT address of device to bonded with
 *      localPsm:           The local PSM channel
 *      remotePsm:          Remote PSM channel to connect to
 *      framesize:          Maximum frame size (bytes)
 *      secLevel:           Level of security to be applied
 *      transmitMtu         Maximum payload size proposed by the jsr82.
 *                          In the case that transmitMtu < than the Mtu
 *                          received from the remote device in a L2CA_CONFIG_IND
 *      minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectReq *CsrBtCml2caConnectReq_struct(CsrSchedQid           appHandle,
                                                    CsrBtDeviceAddr        theDeviceAddr,
                                                    psm_t               localPsm,
                                                    psm_t               remotePsm,
                                                    l2ca_mtu_t          framesize,
                                                    dm_security_level_t secLevel,
                                                    l2ca_mtu_t          transmitMtu,
                                                    CsrUint16           dataPriority,
                                                    CsrUint8            minEncKeySize);

#ifndef EXCLUDE_CSR_BT_JSR82_MODULE
#define CsrBtCmJsr82l2caConnectReqSend(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel,_transmitMtu, _minEncKeySize) { \
        CsrBtCmL2caConnectReq *msg__;                                   \
        msg__=CsrBtCml2caConnectReq_struct(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel,_transmitMtu, CSR_BT_CM_PRIORITY_NORMAL, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#define CsrBtCml2caConnectReqSend(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel, _minEncKeySize) { \
        CsrBtCmL2caConnectReq *msg__;                                   \
        msg__=CsrBtCml2caConnectReq_struct(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel,0, CSR_BT_CM_PRIORITY_NORMAL, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCml2caConnectHighDataPriorityReqSend(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel,_dataPriority, _minEncKeySize) { \
        CsrBtCmL2caConnectReq *msg__;                                   \
        msg__=CsrBtCml2caConnectReq_struct(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_secLevel,0, _dataPriority, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caConnectReqConftabSend
 *
 *  DESCRIPTION
 *      CM L2CAP connect request using conftabs
 *
 *  PARAMETERS
 *      minEncKeySize       Minimum encryption key size
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_CONNECT_CONFTAB
CsrBtCmL2caConnectReq *CsrBtCmL2caConnectReqConftab_struct(CsrSchedQid appHandle,
                                                           CsrBtDeviceAddr deviceAddr,
                                                           psm_t localPsm,
                                                           psm_t remotePsm,
                                                           dm_security_level_t secLevel,
                                                           CsrUint16 context,
                                                           CsrUint16 conftabCount,
                                                           CsrUint16 *conftab,
                                                           CsrUint8 minEncKeySize);

#define CsrBtCmL2caConnectReqConftabSend(_appHandle,_deviceAddr,_localPsm,_remotePsm,_secLevel,_context,_conftabCount,_conftab,_minEncKeySize) { \
        CsrBtCmL2caConnectReq *msg__;                                   \
        msg__=CsrBtCmL2caConnectReqConftab_struct(_appHandle,_deviceAddr,_localPsm,_remotePsm,_secLevel,_context,_conftabCount,_conftab,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caFecConnectReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            Protocol handle
 *        deviceAddr:            BT address of device to bonded with
 *        localPsm:            The local PSM channel
 *        remotePsm:            Remote PSM channel to connect to
 *        framesize:            Maximum frame size (bytes)
 *        transmitMtu           Maximum payload size proposed by the jsr82.
 *                              In the case that transmitMtu < than the Mtu
 *                              received from the remote device in a L2CA_CONFIG_IND
 *        flushTimeout:        Flush timeout value
 *        serviceQuality:        pointer to QoS flow parameters
 *        secLevel:           Level of security to be applied
 *        minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectReq *CsrBtCml2caConnectReqSend_struct(CsrSchedQid appHandle,
                                                           CsrBtDeviceAddr     deviceAddr,
                                                           psm_t               localPsm,
                                                           psm_t               remotePsm,
                                                           l2ca_mtu_t          framesize,
                                                           l2ca_mtu_t          transmitMtu,
                                                           CsrUint16           flushTimeout,
                                                           L2CA_QOS_T         *serviceQuality,
                                                           L2CA_FLOW_T        *flow,
                                                           CsrBool             fallbackBasicMode,
                                                           dm_security_level_t secLevel,
                                                           CsrUint8            minEncKeySize);

#define CsrBtCml2caFecConnectReqSend(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,_flushTimeout,_serviceQuality, _flow,_fallbackBasicMode,_secLevel,_minEncKeySize) { \
        CsrBtCmL2caConnectReq *msg__;                                   \
        msg__=CsrBtCml2caConnectReqSend_struct(_appHandle,_deviceAddr,_localPsm,_remotePsm,_frameSize,0,_flushTimeout,_serviceQuality, _flow,_fallbackBasicMode,_secLevel,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmJsr82l2caConnectAcceptReqSend
 *      CsrBtCml2caConnectAcceptSecondaryReqSend
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      appHandle:          Protocol handle
 *      localPsm:           The local PSM channel
 *      classOfDevice:      The profile COD
 *      secLevel:           Level of security to be applied
 *      framesize:          Maximum frame size (bytes)
 *      profileUuid:        The local profile Uuid
 *      primaryAcceptor     If TRUE it makes the host controller connectable,
 *                          e.g write the cod value and set it in page scan.
 *                          If FALSE the host controller is not set connecable
 *                          and the SM queue is not used. E.g this message is
 *                          handle at once by the CM.
 *      minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectAcceptReq *CsrBtCml2caConnectAcceptReq_struct(CsrSchedQid              theAppHandle,
                                                                psm_t               theLocalPsm,
                                                                CsrUint24           theClassOfDevice,
                                                                dm_security_level_t theSecLevel,
                                                                l2ca_mtu_t          theFramesize,
                                                                uuid16_t            theProfileUuid,
                                                                l2ca_mtu_t          transmitMtu,
                                                                CsrBool             primaryAcceptor,
                                                                CsrUint16           dataPriority,
                                                                CsrUint8            minEncKeySize);
#ifndef EXCLUDE_CSR_BT_JSR82_MODULE
#define CsrBtCmJsr82l2caConnectAcceptReqSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_profileUuid,_transmitMtu, _minEncKeySize) { \
        CsrBtCmL2caConnectAcceptReq *msg__;                             \
        msg__=CsrBtCml2caConnectAcceptReq_struct(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_profileUuid,_transmitMtu,TRUE, CSR_BT_CM_PRIORITY_NORMAL, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#define CsrBtCml2caConnectAcceptSecondaryReqSend(_appHandle,_localPsm,_secLevel,_frameSize,_profileUuid,_dataPriority, _minEncKeySize) { \
        CsrBtCmL2caConnectAcceptReq *msg__;                             \
        msg__=CsrBtCml2caConnectAcceptReq_struct(_appHandle,_localPsm,0,_secLevel,_frameSize,_profileUuid,0,FALSE,_dataPriority, _minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caConnectAcceptReqConftabSend
 *
 *  DESCRIPTION
 *      CM L2CAP connect accept request using conftabs
 *
 *  PARAMETERS
 *      minEncKeySize       Minimum encryption key size
 *
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_CONNECT_CONFTAB
CsrBtCmL2caConnectAcceptReq *CsrBtCmL2caConnectAcceptReqConftab_struct(CsrSchedQid appHandle,
                                                                       psm_t localPsm,
                                                                       CsrUint24 cod,
                                                                       dm_security_level_t secLevel,
                                                                       uuid16_t uuid,
                                                                       CsrBool primaryAcceptor,
                                                                       CsrUint16 context,
                                                                       CsrUint16 conftabLength,
                                                                       CsrUint16 *conftab, 
                                                                       CsrBtDeviceAddr deviceAddr,
                                                                       CsrUint8 minEncKeySize);

#define CsrBtCmL2caConnectAcceptReqConftabSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_profileUuid,_primaryAcceptor,_context,_conftabCount,_conftab,_devAddr,_minEncKeySize) { \
        CsrBtCmL2caConnectAcceptReq *msg__;                             \
        msg__=CsrBtCmL2caConnectAcceptReqConftab_struct(_appHandle,_localPsm,_classOfDevice,_secLevel,_profileUuid,_primaryAcceptor,_context,_conftabCount,_conftab,_devAddr,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caFecConnectAcceptReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      appHandle:      Protocol handle
 *      localPsm:       The local PSM channel
 *      classOfDevice:  The profile COD
 *      secLevel:       Level of security to be applied
 *      framesize:      Maximum frame size (bytes)
 *      flushTimeout:   Flush timeout value
 *      serviceQuality: QoS flow parameters
 *      profileUuid:    The local profile Uuid
 *      minEncKeySize   Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectAcceptReq *CsrBtCml2caConnectAcceptReqSend_struct(CsrSchedQid appHandle,
                                                                    psm_t localPsm,
                                                                    CsrUint24 classOfDevice,
                                                                    dm_security_level_t secLevel,
                                                                    l2ca_mtu_t framesize,
                                                                    CsrUint16 flushTimeout,
                                                                    L2CA_QOS_T *serviceQuality,
                                                                    L2CA_FLOW_T *flow,
                                                                    CsrBool fallbackBasicMode,
                                                                    uuid16_t profileUuid,
                                                                    CsrBool primaryAcceptor,
                                                                    CsrUint8 minEncKeySize);

#define CsrBtCml2caFecConnectAcceptReqSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality,_flow,_fallbackBasicMode,_profileUuid,_minEncKeySize) { \
        CsrBtCmL2caConnectAcceptReq *msg__;                             \
        msg__=CsrBtCml2caConnectAcceptReqSend_struct(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality, _flow,_fallbackBasicMode,_profileUuid, TRUE,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCml2caFecConnectAcceptSecondaryReqSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality,_flow,_fallbackBasicMode,_profileUuid,_minEncKeySize) { \
        CsrBtCmL2caConnectAcceptReq *msg__;                             \
        msg__=CsrBtCml2caConnectAcceptReqSend_struct(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality, _flow,_fallbackBasicMode,_profileUuid, FALSE,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caConnectAcceptReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            Protocol handle
 *        localPsm:             The local PSM channel
 *        classOfDevice:        The profile COD
 *        secLevel:             Level of security to be applied
 *        framesize:            Maximum frame size (bytes)
 *        flushTimeout:         Flush timeout value
 *        serviceQuality:       QoS flow parameters
 *        profileUuid:          The local profile Uuid
 *        minEncKeySize         Minimum encryption key size
 *----------------------------------------------------------------------------*/
#define CsrBtCml2caConnectAcceptReqSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality,_profileUuid,_minEncKeySize) \
    CsrBtCml2caFecConnectAcceptReqSend(_appHandle,_localPsm,_classOfDevice,_secLevel,_frameSize,_flushTimeout,_serviceQuality,NULL,TRUE,_profileUuid,_minEncKeySize)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caConnectAcceptDeviceReqSend: Connect Accept from a specific device "deviceAddr".
 *      CsrBtCmL2caConnectAcceptReqExSend: Connect Accept from a specific device "deviceAddr" as secondary (i.e. do not make DUT connectable).
 *
 *  DESCRIPTION
 *  Allow Synergy Connection Manager to accept L2CAP connection from peer device on behalf of the client. The requesting module
 *  would receive CSR_BT_CM_L2CA_CONNECT_ACCEPT_CFM message on successful establishment of L2CAP connection.
 *
 *  PARAMETERS
 *        appHandle:            Protocol handle
 *        localPsm:             The local PSM channel
 *        secLevel:             Level of security to be applied
 *        uuid:                 The local profile Uuid
 *        framesize:            Maximum frame size (bytes)
 *        transmitMtu:          Outgoing MTU
 *        deviceAddr:           Remote device address
 *        context:              Context
 *        dataPriority:         Data priority
 *        minEncKeySize         Minimum encryption key size
 *        primaryAcceptor       If TRUE it makes the host controller connectable,
 *                              e.g write the cod value and set it in page scan.
 *                              If FALSE the host controller is not set connecable
 *                              and the SM queue is not used. E.g this message is
 *                              handle at once by the CM.
 *----------------------------------------------------------------------------*/
CsrBtCmL2caConnectAcceptReq *CsrBtCmL2caConnectAcceptReqEx_struct(CsrSchedQid appHandle,
                                                                  psm_t localPsm,
                                                                  dm_security_level_t secLevel,
                                                                  uuid16_t uuid,
                                                                  l2ca_mtu_t theFramesize,
                                                                  l2ca_mtu_t transmitMtu,
                                                                  CsrBtDeviceAddr deviceAddr,
                                                                  CsrUint16 context,
                                                                  CsrUint16 dataPriority,
                                                                  CsrUint8 minEncKeySize,
                                                                  CsrBool primaryAcceptor);

#define CsrBtCmL2caConnectAcceptDeviceReqSend(_appHandle,                       \
                                          _localPsm,                            \
                                          _secLevel,                            \
                                          _uuid,                                \
                                          _theFramesize,                        \
                                          _transmitMtu,                         \
                                          _deviceAddr,                          \
                                          _context,                             \
                                          _dataPriority,                        \
                                          _minEncKeySize,                       \
                                          _primaryAcceptor)                     \
    do                                                                          \
    {                                                                           \
        CsrBtCmL2caConnectAcceptReq *_msg;                                      \
        _msg = CsrBtCmL2caConnectAcceptReqEx_struct(_appHandle,                 \
                                                    _localPsm,                  \
                                                    _secLevel,                  \
                                                    _uuid,                      \
                                                    _theFramesize,              \
                                                    _transmitMtu,               \
                                                    _deviceAddr,                \
                                                    _context,                   \
                                                    _dataPriority,              \
                                                    _minEncKeySize,             \
                                                    _primaryAcceptor);          \
        CsrBtCmPutMessageDownstream(_msg);                                      \
    } while (0)

#define CsrBtCmL2caConnectAcceptReqExSend(_appHandle, _localPsm,_secLevel,_uuid, _theFramesize,_transmitMtu,_deviceAddr,_context,_dataPriority,_minEncKeySize)      \
    CsrBtCmL2caConnectAcceptDeviceReqSend(_appHandle, _localPsm,_secLevel,_uuid, _theFramesize,_transmitMtu,_deviceAddr,_context,_dataPriority,_minEncKeySize, FALSE)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caDataAbortReqSend
 *
 *  DESCRIPTION
 *      Flush the L2CAP transmit queue
 *
 *  PARAMETERS
 *      cid:                Channel identifier
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_L2CA_DATA_ABORT
CsrBtCmL2caDataAbortReq *CsrBtCmL2caDataAbortReq_struct(CsrBtConnId btConnId);

#define CsrBtCmL2caDataAbortReqSend(_btConnId) {                \
        CsrBtCmL2caDataAbortReq *msg__;                         \
        msg__=CsrBtCmL2caDataAbortReq_struct(_btConnId);        \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCommonScoConnectPrepare
 *
 *  DESCRIPTION
 *      Allocate a buffer '*parms' that points to some memory large enought to contain
 *      'count' number of eSCO parameters.
 *
 *  PARAMETERS
 *      parms
 *      parmsOffset
 *      count
 *----------------------------------------------------------------------------*/
void CsrBtCmCommonScoConnectPrepare(CsrBtCmScoCommonParms                      **parms,
                                    CsrUint16        *parmsOffset,
                                    CsrUint16        count);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCommonScoConnectBuild
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      parms
 *      parmsOffset
 *      audioQuality:
 *      txBandwidth
 *      rxBandwidth
 *      maxLatency
 *      voiceSettings
 *      reTxEffort
 *----------------------------------------------------------------------------*/
void CsrBtCmCommonScoConnectBuild(CsrBtCmScoCommonParms                    *parms,
                                  CsrUint16           *parmsOffset,
                                  hci_pkt_type_t     theAudioQuality,
                                  CsrUint32           theTxBandwidth,
                                  CsrUint32           theRxBandwidth,
                                  CsrUint16           theMaxLatency,
                                  CsrUint16           theVoiceSettings,
                                  CsrUint8            theReTxEffort);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caCancelConnectAcceptReqSend
 *      CsrBtCmContextl2caCancelConnectAcceptReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        appHandle:            Protocol handle
 *        localPsm:            The local PSM channel
 *----------------------------------------------------------------------------*/
CsrBtCmL2caCancelConnectAcceptReq *CsrBtCml2caCancelConnectAcceptReq_struct(CsrSchedQid    appHandle,
                                                                            psm_t        localPsm,
                                                                            CsrUint16   context);

#define CsrBtCmContextl2caCancelConnectAcceptReqSend(_appHandle,_localPsm,_context) { \
        CsrBtCmL2caCancelConnectAcceptReq *msg__;                       \
        msg__=CsrBtCml2caCancelConnectAcceptReq_struct(_appHandle,_localPsm,_context); \
        CsrBtCmPutMessageDownstream(msg__);}

#define CsrBtCml2caCancelConnectAcceptReqSend(_a,_l) CsrBtCmContextl2caCancelConnectAcceptReqSend(_a,_l,CSR_BT_CM_CONTEXT_UNUSED)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCml2caModeChangeReqSend
 *      CsrBtCml2caAmpForceModeChangeReqSend
 *
 *  DESCRIPTION
 *        If a connection exists, CSR_BT_CM_L2CA_MODE_CHANGE_REQ message 
 *        requests a change of the link mode to Active (no power saving mode) or 
 *        Sniff mode. The application would receive CSR_BT_CM_MODE_CHANGE_IND if
 *        the message is successfully processed else CSR_BT_CM_L2CA_MODE_CHANGE_IND 
 *        is sent to the application with result. The message will be ignored 
 *        if connection does not exist.
 *
 *  PARAMETERS
 *        btConnId:           connection ID
 *        requestedMode:      Requested link policy mode (CSR_BT_ACTIVE_MODE / CSR_BT_SNIFF_MODE)
 *        forceSniff:         Currently unused
 *----------------------------------------------------------------------------*/
CsrBtCmL2caModeChangeReq *CsrBtCml2caModeChangeReq_struct(CsrBtConnId btConnId,
                                                          CsrUint8    mode,
                                                          CsrBool     forceSniff);

#define CsrBtCml2caModeChangeReqSend(_btConnId,_mode) {                 \
        CsrBtCmL2caModeChangeReq *msg__;                                \
        msg__=CsrBtCml2caModeChangeReq_struct(_btConnId,_mode, FALSE);  \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CmDml2caModeSettingsReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS
CsrBtCmDmL2caModeSettingsReq *CsrBtCmDmL2caModeSettingsReq_struct(CsrBtConnId                  btConnId,
                                                                  CsrBtSniffSettings              *sniffSettings,
                                                                  CsrBtSsrSettingsDownstream     *ssrSettings,
                                                                  CsrUint8                       lowPowerPriority);

#define CsrBtCmDmL2caModeSettingsReqSend(_btConnId,_sniffSettings,_ssrSettings,_lowPowerPriority) { \
        CsrBtCmDmL2caModeSettingsReq *msg__;                            \
        msg__=CsrBtCmDmL2caModeSettingsReq_struct(_btConnId,_sniffSettings,_ssrSettings,_lowPowerPriority); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caConnectionlessDataReqSend
 *
 *  DESCRIPTION
 *      Send broadcast or unicast connectionless data to device.
 *      To broadcast, set address to all 0xFF's.
 *
 *   Note:
 *   If unicast connectionless data(UCD) requires encryption, then the profile or application shall
 *   ensure that authentication is performed and encryption is enabled prior to sending any unicast
 *   data on the connectionless L2CAP channel.
 *
 *  PARAMETERS
 *      connectionlessPsm   Broadcast PSM. Must be registered by the app.
 *      length              Length of 'payload' in bytes
 *      payload             Pointer to data to send
 *      deviceAddr          Address of recipient, use FFFF:FF:FFFFFF to broadcast
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_L2CAP_CONNLESS_SUPPORT
CsrBtCmL2caConnectionlessDataReq *CsrBtCmL2caConnectionlessDataReq_struct(psm_t                     connectionlessPsm,
                                                                          CsrUint16                 length,
                                                                          CsrUint8                 *payload,
                                                                          const CsrBtDeviceAddr    *deviceAddr);

#define CsrBtCmL2caConnectionlessDataReqSend(_connectionLessPsm, _length, _payload, _deviceAddr) { \
        CsrBtCmL2caConnectionlessDataReq *msg__;                        \
        msg__=CsrBtCmL2caConnectionlessDataReq_struct(_connectionLessPsm,_length,_payload,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#ifndef EXCLUDE_CSR_BT_BNEP_MODULE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepRegisterReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        disableExtended:
 *        disableStack:
 *        manualBridge:
 *        phandle:
 *        deviceAddr:
 *        serviceList:        A list of Services (UUID) to search for
 *      serviceListSize:    Number of services to search for
 *----------------------------------------------------------------------------*/
CsrBtCmBnepRegisterReq *CsrBtCmBnepRegisterReq_struct(CsrBool          disableExtended,
                                                      CsrBool          disableStack,
                                                      CsrBool          manualBridge,
                                                      CsrSchedQid      phandle,
                                                      CsrBtDeviceAddr deviceAddr);

#define CsrBtCmBnepRegisterReqSend(_disableExtended,_disableStack,_manualBridge,_phandle,_deviceAddr) { \
        CsrBtCmBnepRegisterReq *msg__;                                  \
        msg__=CsrBtCmBnepRegisterReq_struct(_disableExtended,_disableStack,_manualBridge,_phandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepConnectReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        flags:
 *        remoteAddr:
 *        maxFrameSize:
 *        secLevel
 *        minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmBnepConnectReq *CsrBtCmBnepConnectReq_struct(BNEP_CONNECT_REQ_FLAGS    flags,
                                                    ETHER_ADDR                remoteAddr,
                                                    CsrUint16                    maxFrameSize,
                                                    dm_security_level_t      secLevel,
                                                    CsrUint8                 minEncKeySize);

#define CsrBtCmBnepConnectReqSend(_flags,_remoteAddr,_maxFrameSize,_secLevel,_minEncKeySize) { \
        CsrBtCmBnepConnectReq *msg__;                                   \
        msg__=CsrBtCmBnepConnectReq_struct(_flags,_remoteAddr,_maxFrameSize,_secLevel,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmCancelBnepConnectReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *    PARAMETERS
 *        remoteAddr:
 *----------------------------------------------------------------------------*/
CsrBtCmCancelBnepConnectReq *CsrBtCmCancelBnepConnectReq_struct(ETHER_ADDR remoteAddr);

#define CsrBtCmCancelBnepConnectReqSend(_remoteAddr) {          \
        CsrBtCmCancelBnepConnectReq *msg__;                     \
        msg__=CsrBtCmCancelBnepConnectReq_struct(_remoteAddr);  \
        CsrBtCmMsgTransport( msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepSetConnectReqFlagsFromRoles
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        localRole:
 *        remoteRole:
 *        accept_in:
 *----------------------------------------------------------------------------*/
BNEP_CONNECT_REQ_FLAGS CsrBtCmBnepSetConnectReqFlagsFromRoles(CsrBtBslPanRole localRole, CsrBtBslPanRole remoteRole, CsrBool accept_in, CsrBool single_user);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepConnectAcceptReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        flags:
 *        remoteAddr:
 *        maxFrameSize:
 *        classOfDevice:
 *        secLevel:           Level of security to be applied
 *        minEncKeySize       Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBtCmBnepConnectAcceptReq *CsrBtCmBnepConnectAcceptReq_struct(BNEP_CONNECT_REQ_FLAGS    flags,
                                                                ETHER_ADDR               remoteAddr,
                                                                CsrUint16                maxFrameSize,
                                                                CsrUint24                classOfDevice,
                                                                dm_security_level_t      secLevel,
                                                                CsrUint8                 minEncKeySize);

#define CsrBtCmBnepConnectAcceptReqSend(_flags,_remoteAddr,_maxFrameSize,_classOfDevice,_secLevel,_minEncKeySize) { \
        CsrBtCmBnepConnectAcceptReq *msg__;                             \
        msg__=CsrBtCmBnepConnectAcceptReq_struct(_flags,_remoteAddr,_maxFrameSize,_classOfDevice,_secLevel,_minEncKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepCancelConnectAcceptReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        ......
 *----------------------------------------------------------------------------*/
CsrBtCmBnepCancelConnectAcceptReq *CsrBtCmBnepCancelConnectAcceptReq_struct(void);

#define CsrBtCmBnepCancelConnectAcceptReqSend() {               \
        CsrBtCmBnepCancelConnectAcceptReq *msg__;               \
        msg__=CsrBtCmBnepCancelConnectAcceptReq_struct();       \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepExtendedDataReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        theEtherType:
 *        theId:
 *        theLength:
 *        *thePayload:
 *        theDestEtherAddr:
 *        theSrcEtherAddr:
 *----------------------------------------------------------------------------*/
CsrBtCmBnepExtendedDataReq *CsrBtCmBnepExtendedDataReq_struct(CsrUint16        theEtherType,
                                                              CsrUint16        theId,
                                                              CsrUint16        theLength,
                                                              CsrUint8        *thePayload,
                                                              ETHER_ADDR    theDestEtherAddr,
                                                              ETHER_ADDR    theSrcEtherAddr);

#define CsrBtCmBnepExtendedDataReqSend(_etherType,_id,_length,_payload,_destEtherAddr,_srcEtherAddr) { \
        CsrBtCmBnepExtendedDataReq *msg__;                              \
        msg__=CsrBtCmBnepExtendedDataReq_struct(_etherType,_id,_length,_payload,_destEtherAddr,_srcEtherAddr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepExtendedMultiCastDataReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        theEtherType:
 *        theIdNot:
 *        theLength:
 *        *thePayload:
 *        theDestEtherAddr:
 *        theSrcEtherAddr:
 *----------------------------------------------------------------------------*/
CsrBtCmBnepExtendedMulticastDataReq *CsrBtCmBnepExtendedMultiCastDataReq_struct(CsrUint16    theEtherType,
                                                                                CsrUint16    theIdNot,
                                                                                CsrUint16    theLength,
                                                                                CsrUint8        *thePayload,
                                                                                ETHER_ADDR    theDestEtherAddr,
                                                                                ETHER_ADDR    theSrcEtherAddr);

#define CsrBtCmBnepExtendedMultiCastDataReqSend(_etherType,_id,_length,_payload,_destEtherAddr,_srcEtherAddr) { \
        CsrBtCmBnepExtendedDataReq *msg__;                              \
        msg__=CsrBtCmBnepExtendedMultiCastDataReq_struct(_etherType,_id,_length,_payload,_destEtherAddr,_srcEtherAddr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepDisconnectReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        theFlags:
 *        theId:
 *----------------------------------------------------------------------------*/
CsrBtCmBnepDisconnectReq *CsrBtCmBnepDisconnectReq_struct(CsrUint16 theFlags,
                                                          CsrUint16 theId);

#define CsrBtCmBnepDisconnectReqSend(_flags,_id) {              \
        CsrBtCmBnepDisconnectReq *msg__;                        \
        msg__=CsrBtCmBnepDisconnectReq_struct(_flags,_id);      \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepDisconnectResSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        theFlags:
 *        theId:
 *----------------------------------------------------------------------------*/
CsrBtCmBnepDisconnectRes *CsrBtCmBnepDisconnectRes_struct(CsrUint16 theFlags,
                                                          CsrUint16 theId);


#define CsrBtCmBnepDisconnectResSend(_flags,_id) {              \
        CsrBtCmBnepDisconnectRes *msg__;                        \
        msg__=CsrBtCmBnepDisconnectRes_struct(_flags,_id);      \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepSwitchRoleReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        id:
 *        role:
 *----------------------------------------------------------------------------*/
CsrBtCmBnepSwitchRoleReq *CsrBtCmBnepSwitchRoleReq_struct(CsrUint16        theId,
                                                          CsrUint8        role);

#define CsrBtCmBnepSwitchRoleReqSend(_id,_role) {               \
        CsrBtCmBnepSwitchRoleReq *msg__;                        \
        msg__=CsrBtCmBnepSwitchRoleReq_struct(_id,_role);       \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBnepModeChangeReqSend
 *
 *  DESCRIPTION
 *        If a connection exists, CSR_BT_CM_BNEP_MODE_CHANGE_REQ message 
 *        requests a change of the link mode to Active (no power saving mode) or 
 *        Sniff mode. The application would receive CSR_BT_CM_MODE_CHANGE_IND if
 *        the message is successfully processed else CSR_BT_CM_BNEP_MODE_CHANGE_IND 
 *        is sent to the application. The message will be ignored 
 *        if connection does not exist.
 *
 *  PARAMETERS
 *        id:                 connection ID
 *        requestedMode:      Requested link policy mode (CSR_BT_ACTIVE_MODE / CSR_BT_SNIFF_MODE)
 *----------------------------------------------------------------------------*/
CsrBtCmBnepModeChangeReq *CsrBtCmBnepModeChangeReq_struct(CsrUint16    theId,
                                                          CsrUint8    requestedMode);

#define CsrBtCmBnepModeChangeReqSend(_id,_requestedMode) {              \
        CsrBtCmBnepModeChangeReq *msg__;                                \
        msg__=CsrBtCmBnepModeChangeReq_struct(_id,_requestedMode);      \
        CsrBtCmPutMessageDownstream(msg__);}

#ifdef CSR_BT_INSTALL_CM_PRI_MODE_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDmBnepModeSettingsReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *----------------------------------------------------------------------------*/
CsrBtCmDmBnepModeSettingsReq *CsrBtCmDmBnepModeSettingsReq_struct(CsrUint16         theId,
                                                                  CsrBtSniffSettings               *theSniffSettings,
                                                                  CsrBtSsrSettingsDownstream      *theSsrSettings,
                                                                  CsrUint8                        lowPowerPriority);

#define CsrBtCmDmBnepModeSettingsReqSend(_id,_sniffSettings,_ssrSettings,_lowPowerPriority) { \
        CsrBtCmDmBnepModeSettingsReq *msg__;                            \
        msg__=CsrBtCmDmBnepModeSettingsReq_struct(_id,_sniffSettings,_ssrSettings,_lowPowerPriority); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#endif /* !EXCLUDE_CSR_BT_BNEP_MODULE */

#ifndef CSR_BT_EXCLUDE_HCI_QOS_SETUP
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDmHciQosSetupReqSend
 *
 *  DESCRIPTION
 *      This primitive is used primarily to reserve the bandwidth requirements for any currently running 
 *      high priority data channel for which we are the Master of the ACL link, when a simultaneous 
 *      paging attempt is made. Once paging is done, this api can be used to offer back the complete
 *      bandwidth to high priority acl
 *
 *  EXAMPLE
 *      When inquiry is made that involves RNR (multiple paging attempts to discover remote name 
 *      devices found during discovery), this api shall be used to set up the QOS for the ACL link
 *      that has any high priority data channel running (say AV). 
 *
 *      For instance, before the start of device discovery call the following api to set the QOS of the 
 *      ACL link that has a high priority data channel running if any with its service latency requirements
 *                     CsrBtCmDmHciQosSetupReqSend(phandle, NULL, FALSE);
 *      and once the device discovery has ended call the following api to set the QOS of the ACL link
 *      back to default
 *                     CsrBtCmDmHciQosSetupReqSend(phandle, NULL, TRUE);
 *
 *  PARAMETERS
 *        phandle:            Protocol handle
 *        deviceAddr:       If valid holds the BT address of the device to which paging shall be initiated.
 *                                If invalid implies a procedure that involves paging, say RNR during inquiry.
 *        useDefaultQos:  if set to TRUE use the cm default qos settings, otherwise set it based on high 
 *                                priority channel service latency requirements that are currently running if any.
  *----------------------------------------------------------------------------*/
CsrBtCmDmHciQosSetupReq *CsrBtCmDmHciQosSetupReq_struct(CsrSchedQid phandle,
                                            CsrBtDeviceAddr     *deviceAddr,
                                            CsrBool             useDefaultQos);

#define CsrBtCmDmHciQosSetupReqSend(_phandle, _deviceAddr, _useDefaultQos) { \
        CsrBtCmDmHciQosSetupReq *msg__;                                       \
        msg__=CsrBtCmDmHciQosSetupReq_struct(_phandle, _deviceAddr, _useDefaultQos); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcRfcSearchReqSend
 *
 *  DESCRIPTION
 *      Submits a search request to the SDC sub-system
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *        serviceList:         A list of Services (UUID) to search for
 *      serviceListSize:     Number of services to search for
 *        localServerChannel     The local server Channel
 *----------------------------------------------------------------------------*/
CsrBtCmSdcRfcSearchReq *CsrBtCmSdcRfcSearchReq_struct(CsrSchedQid        appHandle,
                                                      CsrBtDeviceAddr    deviceAddr,
                                                      CsrBtUuid32        * serviceList,
                                                      CsrUint8        serviceListSize,
                                                      CsrUint8    localServerChannel);

#define CsrBtCmSdcRfcSearchReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel) { \
        CsrBtCmSdcRfcSearchReq *msg__;                                  \
        msg__=CsrBtCmSdcRfcSearchReq_struct(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelRfcSearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_RFC_SEARCH_REQ
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
CsrBtCmSdcCancelSearchReq *CsrBtCmSdcCancelRfcSearchReq_struct(CsrSchedQid    appHandle,
                                                               CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcCancelRfcSearchReqSend(_appHandle,_deviceAddr) {      \
        CsrBtCmSdcCancelSearchReq *msg__;                               \
        msg__=CsrBtCmSdcCancelRfcSearchReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcServiceSearchReqSend
 *
 *  DESCRIPTION
 *      Submits a search request to the SDC sub-system
 *
 *  PARAMETERS
 *        appHandle:                protocol handle
 *        deviceAddr:                BT address of the device to search for services
 *        uuidSetLength:          The size of uuidSet in bytes
 *      uuidSet:                A search pattern of UUIDs. All of these are
 *                              contained in each returned service record
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_SDC
CsrBtCmSdcServiceSearchReq *CsrBtCmSdcServiceSearchReq_struct(CsrSchedQid    appHandle,
                                                              CsrBtDeviceAddr deviceAddr,
                                                              CsrUint16     uuidSetLength,
                                                              CsrUint8      *uuidSet);

#define CsrBtCmSdcServiceSearchReqSend(_appHandle,_deviceAddr,_uuidSetLength,_uuidSet) { \
        CsrBtCmSdcServiceSearchReq *msg__;                              \
        msg__=CsrBtCmSdcServiceSearchReq_struct(_appHandle,_deviceAddr,_uuidSetLength,_uuidSet); \
        CsrBtCmPutMessageDownstream(msg__);}
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelServiceSearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_SERVICE_SEARCH_REQ
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
CsrBtCmSdcCancelSearchReq *CsrBtCmSdcCancelServiceSearchReq_struct(CsrSchedQid    appHandle,
                                                                   CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcCancelServiceSearchReqSend(_appHandle,_deviceAddr) {  \
        CsrBtCmSdcCancelSearchReq *msg__;                               \
        msg__=CsrBtCmSdcCancelServiceSearchReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcUuid128RfcSearchReqSend
 *
 *  DESCRIPTION
 *      Submits a 128bit search request to the SDC sub-system
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *        serviceList:         A list of 128 bit Services (UUID128) to search for
 *      serviceListSize:     Number of services to search for
 *        localServerChannel     The local server Channel
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
CsrBtCmSdcUuid128RfcSearchReq *CsrBtCmSdcUuid128RfcSearchReq_struct(CsrSchedQid        appHandle,
                                                                    CsrBtDeviceAddr    deviceAddr,
                                                                    CsrBtUuid128        * serviceList,
                                                                    CsrUint8            serviceListSize,
                                                                    CsrUint8    localServerChannel);

#define CsrBtCmSdcUuid128RfcSearchReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel) { \
        CsrBtCmSdcUuid128RfcSearchReq *msg__;                           \
        msg__=CsrBtCmSdcUuid128RfcSearchReq_struct(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelUuid128RfcSearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
CsrBtCmSdcCancelSearchReq *CsrBtCmSdcCancelUuid128RfcSearchReq_struct(CsrSchedQid    appHandle,
                                                                      CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcCancelUuid128RfcSearchReqSend(_appHandle,_deviceAddr) { \
        CsrBtCmSdcCancelSearchReq *msg__;                               \
        msg__=CsrBtCmSdcCancelUuid128RfcSearchReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcReleaseResourcesReqSend
 *
 *  DESCRIPTION
 *      Releases the SDC search resources, and allow the application with
 *        "apphandle" to perform a new SDC search
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        localServerChannel     The local server Channel
 *----------------------------------------------------------------------------*/
CsrBtCmSdcReleaseResourcesReq *CsrBtCmSdcReleaseResourcesReq_struct(CsrSchedQid     appHandle,
                                                                    CsrBtDeviceAddr  deviceAddr,
                                                                    CsrUint8 localServerChannel);

#define CsrBtCmSdcReleaseResourcesReqSend(_appHandle,_deviceAddr,_localServerChannel) { \
        CsrBtCmSdcReleaseResourcesReq *msg__;                           \
        msg__=CsrBtCmSdcReleaseResourcesReq_struct(_appHandle,_deviceAddr,_localServerChannel); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *    NAME
 *        CsrBtCmSdcRfcExtendedSearchReqSend
 *
 *    DESCRIPTION
 *        Submits a search request to the SDC sub-system for the ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:          BT address of the device to read remote name
 *        serviceList:         A list of Services (UUID) to search for
 *        serviceListSize:     Number of services to search for
 *        localServerChannel     The local server Channel
 *----------------------------------------------------------------------------*/
CsrBtCmSdcRfcExtendedSearchReq *CsrBtCmSdcRfcExtendedSearchReq_struct(CsrSchedQid      appHandle,
                                                                      CsrBtDeviceAddr  deviceAddr,
                                                                      CsrBtUuid32          *serviceList,
                                                                      CsrUint8       serviceListSize,
                                                                      CsrUint8 localServerChannel);

#define CsrBtCmSdcRfcExtendedSearchReqSend(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel) { \
        CsrBtCmSdcRfcExtendedSearchReq *msg__;                          \
        msg__=CsrBtCmSdcRfcExtendedSearchReq_struct(_appHandle,_deviceAddr,_serviceList,_serviceListSize,_localServerChannel); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdcCancelRfcExtendedSearchReqSend
 *
 *  DESCRIPTION
 *      Cancel a CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BT address of the device to read remote name
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_CANCEL_RFC_EXTENDED_SEARCH
CsrBtCmSdcCancelSearchReq *CsrBtCmSdcCancelRfcExtendedSearchReq_struct(CsrSchedQid    appHandle,
                                                                       CsrBtDeviceAddr deviceAddr);

#define CsrBtCmSdcCancelRfcExtendedSearchReqSend(_appHandle,_deviceAddr) { \
        CsrBtCmSdcCancelSearchReq *msg__;                               \
        msg__=CsrBtCmSdcCancelRfcExtendedSearchReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadEncryptionStatusReqSend
 *
 *  DESCRIPTION
 *      Reads the encryption status of an ACL link
 *
 *  PARAMETERS
 *        appHandle:             protocol handle
 *        deviceAddr:             BD address of the connected device
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_SC_ENCRYPTION
CsrBtCmReadEncryptionStatusReq *CsrBtCmReadEncryptionStatusReq_struct(CsrSchedQid    appHandle,
                                                                      CsrBtDeviceAddr deviceAddr);

#define CsrBtCmReadEncryptionStatusReqSend(_appHandle,_deviceAddr) {    \
        CsrBtCmReadEncryptionStatusReq *msg__;                          \
        msg__=CsrBtCmReadEncryptionStatusReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

#ifdef INSTALL_CM_CLEAR_EVENT_FILTER
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmClearEventFilterReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        .....
 *----------------------------------------------------------------------------*/
CsrBtCmClearEventFilterReq *CsrBtCmClearEventFilterReq_struct(CsrSchedQid appHandle,
                                                              CsrUint8   filter);

#define CsrBtCmClearEventFilterReqSend(_appHandle,_filter) {            \
        CsrBtCmClearEventFilterReq *msg__;                              \
        msg__=CsrBtCmClearEventFilterReq_struct(_appHandle,_filter);    \
        CsrBtCmPutMessageDownstream(msg__);}
#endif /* INSTALL_CM_CLEAR_EVENT_FILTER */

#ifdef INSTALL_CM_SET_EVENT_FILTER_COD
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEventFilterCodReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        .....
 *----------------------------------------------------------------------------*/
CsrBtCmSetEventFilterCodReq *CsrBtCmSetEventFilterCodReq_struct(CsrSchedQid appHandle,
                                                                CsrBool    selectInquiryFilter,
                                                                CsrUint8   autoAccept,
                                                                CsrUint24  cod,
                                                                CsrUint24  codMask);

#define CsrBtCmSetEventFilterCodReqSend(_appHandle,_selectInquiryFilter,_autoAccept,_cod,_codMask) { \
        CsrBtCmSetEventFilterCodReq *msg__;                             \
        msg__=CsrBtCmSetEventFilterCodReq_struct(_appHandle,_selectInquiryFilter,_autoAccept,_cod,_codMask); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif /* INSTALL_CM_SET_EVENT_FILTER_COD */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteCodReqSend
 *
 *  DESCRIPTION
 *        ......
 *
 *  PARAMETERS
 *        phandle:    protocol handle
 *        service:    A Class of device value given from the application
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_WRITE_COD
#define CsrBtCmWriteCodReqSend(_appHandle,_serviceClassOfDevice,_majorClassOfDevice,_minorClassOfDevice) { \
        CsrBtCmWriteCodReq *__msg;                                      \
        __msg=CsrBtCmWriteCodReq_struct(_appHandle, CSR_BT_CM_WRITE_COD_UPDATE_FLAG_SERVICE_CLASS|CSR_BT_CM_WRITE_COD_UPDATE_FLAG_MAJOR_MINOR_CLASS, \
                                        (_serviceClassOfDevice) & CSR_BT_CM_SERVICE_CLASS_FILTER, \
                                        (_majorClassOfDevice) & CSR_BT_CM_MAJOR_DEVICE_CLASS_FILTER, \
                                        (_minorClassOfDevice) & CSR_BT_CM_MINOR_DEVICE_CLASS_FILTER); \
        CsrBtCmMsgTransport(__msg);}
#endif
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetEventFilterBdaddrReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        .....
 *----------------------------------------------------------------------------*/
#ifdef CSR_BT_INSTALL_CM_PRI_SET_EVENT_FILTER_BDADDR
CsrBtCmSetEventFilterBdaddrReq *CsrBtCmSetEventFilterBdaddrReq_struct(CsrSchedQid            appHandle,
                                                                      CsrBool           selectInquiryFilter,
                                                                      CsrUint8          autoAccept,
                                                                      CsrBtDeviceAddr   address);

#define CsrBtCmSetEventFilterBdaddrReqSend(_appHandle,_selectInquiryFilter,_autoAccept,_address) { \
        CsrBtCmSetEventFilterBdaddrReq *msg__;                          \
        msg__=CsrBtCmSetEventFilterBdaddrReq_struct(_appHandle,_selectInquiryFilter,_autoAccept,_address); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadRemoteFeaturesReqSend
 *
 *  DESCRIPTION
 *        This API is used to read remote supported features for the device
 *        indicated by deviceAddr parameter.
 *
 *        CSR_BT_CM_READ_REMOTE_EXT_FEATURES_CFM will be received by the caller
 *        as a response to this API. Additionally if 
 *        CSR_BT_INSTALL_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES
 *        is enabled, and if any application has subscribed for
 *        CSR_BT_CM_EVENT_MASK_SUBSCRIBE_REMOTE_FEATURES using the API
 *        CmSetEventMaskReqSend, would receive CSR_BT_CM_REMOTE_FEATURES_IND.
 *
 *  PARAMETERS
 *        appHandle:    Identity of the calling process.
 *        deviceAddr:   Bluetooth device address of the device for which features
 *                      are requested.
 *----------------------------------------------------------------------------*/
CsrBtCmReadRemoteFeaturesReq *CsrBtCmReadRemoteFeaturesReq_struct(CsrSchedQid     thePhandle,
                                                                  CsrBtDeviceAddr  theDeviceAddr);

#define CsrBtCmReadRemoteFeaturesReqSend(_appHandle,_deviceAddr) {        \
        CsrBtCmReadRemoteFeaturesReq *__msg;                            \
        __msg=CsrBtCmReadRemoteFeaturesReq_struct(_appHandle,_deviceAddr); \
        CsrBtCmMsgTransport(__msg);}

#ifdef CSR_BT_INSTALL_CM_WRITE_VOICE_SETTINGS
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteVoiceSettingsReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *      phandle:               protocol handle
 *      voiceSettings:         voice settings
 *----------------------------------------------------------------------------*/
CsrBtCmWriteVoiceSettingsReq *CsrBtCmWriteVoiceSettingsReq_struct(CsrSchedQid       thePhandle,
                                                                  CsrUint16  voiceSettings);

#define CsrBtCmWriteVoiceSettingsReqSend(_phandle, _voiceSettings) {    \
        CsrBtCmWriteVoiceSettingsReq *__msg;                            \
        __msg=CsrBtCmWriteVoiceSettingsReq_struct(_phandle, _voiceSettings); \
        CsrBtCmMsgTransport(__msg);}
#else
#define CsrBtCmWriteVoiceSettingsReqSend(_phandle, _voiceSettings)      \
    do                                                                  \
    {                                                                   \
        CSR_UNUSED(_phandle);                                           \
        CSR_UNUSED(_voiceSettings);                                     \
    }                                                                   \
    while (0)
#endif /* CSR_BT_INSTALL_CM_WRITE_VOICE_SETTINGS */


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmWriteAuthPayloadTimeoutReqSend
 *
 *  DESCRIPTION
 *      Writes Authenticated Payload Timeout value to the controller.
 *
 *  PARAMETERS
 *      phandle:               protocol handle
 *      tpAddrt                TypedBd address with transport type
 *      authPayloadTimeout     Authenticated payload timeout (multiple of 10ms)
 *      aptRoute               Authenticated payload timeout route event
 *----------------------------------------------------------------------------*/
CsrBtCmWriteAuthPayloadTimeoutReq *CsrBtCmWriteAuthPayloadTimeoutReq_struct(CsrSchedQid         thePhandle,
                                                                            CsrBtTpdAddrT       tpAddrt,
                                                                            CsrUint16           authPayloadTimeout,
                                                                            DM_SM_APT_ROUTE_T   aptRoute);

#define CsrBtCmWriteAuthPayloadTimeoutExtReqSend(_phandle, _tpAddrt, _authPayloadTimeout, _aptRoute) {      \
        CsrBtCmWriteAuthPayloadTimeoutReq *__msg;                                                           \
        __msg=CsrBtCmWriteAuthPayloadTimeoutReq_struct(_phandle, _tpAddrt, _authPayloadTimeout, _aptRoute); \
        CsrBtCmMsgTransport(__msg);}

#define CsrBtCmWriteAuthPayloadTimeoutReqSend(_phandle, _tpAddrt, _authPayloadTimeout)                       \
    CsrBtCmWriteAuthPayloadTimeoutExtReqSend(_phandle, _tpAddrt, _authPayloadTimeout, DM_SM_APT_BLUESTACK)
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to register a service with the service discovery server
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        serviceRecord:        The service record
 *        serviceRecordSize:    Size of the service record
 *        context               Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsRegisterReqSend(CsrSchedQid appHandle,
                               CsrUint8 *serviceRecord,
                               CsrUint16 serviceRecordSize,
                               CsrUint16 context);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSdsUnRegisterReqSend
 *
 *  DESCRIPTION
 *      Request to unregister a service with the service discovery server
 *
 *  PARAMETERS
 *        appHandle:            protocol handle
 *        serviceRecHandle:     The service record handle
 *        context               Opaque context number
 *----------------------------------------------------------------------------*/
void CsrBtCmSdsUnRegisterReqSend(CsrSchedQid appHandle,
                                 CsrUint32 serviceRecHandle,
                                 CsrUint16 context);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLogicalChannelTypeReqSend
 *
 *  DESCRIPTION
 *      Information about the type of channel used for the connection.
 *      CM sends the CSR_BT_CM_LOGICAL_CHANNEL_TYPES_IND signal only if the application
 *      is subscribed for this event.
 *
 *  PARAMETERS
 *        logicalChannelTypeMask:       Type of channel of the connection
 *        address:                      bd address connected to
 *        btConnId                      connection ID
 *----------------------------------------------------------------------------*/
CsrBtCmLogicalChannelTypeReq *CsrBtCmLogicalChannelTypeReq_struct(CsrBtLogicalChannelType  logicalChannelTypeMask,
                                                                  CsrBtDeviceAddr address,
                                                                  CsrBtConnId btConnId);

#define CsrBtCmLogicalChannelTypeReqSend(_logicalChannelTypeMask,_address,_btConnId) { \
        CsrBtCmLogicalChannelTypeReq *__msg;                            \
        __msg=CsrBtCmLogicalChannelTypeReq_struct(_logicalChannelTypeMask,_address,_btConnId); \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmPortnegReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        btConnId              connection ID
 *        PortVar:              Port parameter
 *        context:              Opaque context number returned only if there
 *                              is one to negotiateport parameters on otherwise
 *                              the context CsrBtCmPortnegReqSend
 *----------------------------------------------------------------------------*/
CsrBtCmPortnegReq *CsrBtCmPortnegReq_struct(CsrBtConnId btConnId,
                                            RFC_PORTNEG_VALUES_T  *thePortVar,
                                            CsrUint16     context);

#define CsrBtCmPortnegReqSend(_btConnId,_portPar,_context) {            \
        CsrBtCmPortnegReq *msg__;                                       \
        msg__=CsrBtCmPortnegReq_struct(_btConnId,_portPar,_context);    \
        CsrBtCmPutMessageDownstream(msg__);}

#ifdef CSR_AMP_ENABLE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmMoveChannelReqSend
 *
 *  DESCRIPTION
 *      Request the L2CAP channel to be moved to an AMP or back to the BR/EDR
 *      radio.
 *
 *  PARAMETERS
 *      btConnId              L2CAP channel identifier
 *      remote_control   Remote AMP controller ID
 *      local_control    Local AMP controller ID
 *----------------------------------------------------------------------------*/
CsrBtCmMoveChannelReq *CsrBtCmMoveChannelReq_struct(CsrBtConnId btConnId,
                                                    CsrBtAmpController remote_control,
                                                    CsrBtAmpController local_control);

#define CsrBtCmMoveChannelReqSend(_btConnId, _remoteControl, _localControl) { \
        CsrBtCmMoveChannelReq *__msg;                                   \
        __msg=CsrBtCmMoveChannelReq_struct(_btConnId, _remoteControl, _localControl); \
        CsrBtCmMsgTransport(__msg);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmMoveChannelResSend
 *
 *  DESCRIPTION
 *      L2CAP move channel response generator
 *
 *  PARAMETERS
 *      btConnId              L2CAP channel identifier
 *      accept           Proceed with the move, or refuse it
 *----------------------------------------------------------------------------*/
CsrBtCmMoveChannelRes *CsrBtCmMoveChannelRes_struct(CsrBtConnId btConnId,
                                                    CsrBool accept);

#define CsrBtCmMoveChannelResSend(_btConnId, _accept) {         \
        CsrBtCmMoveChannelRes *__msg;                           \
        __msg=CsrBtCmMoveChannelRes_struct(_btConnId, _accept); \
        CsrBtCmMsgTransport(__msg);}

#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmA2dpBitRateReqSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *        deviceAddr:        address of the remote device
 *        streamIdx:         unique Id for the stream in question
 *        bitRate:           bit rate used if known; else CSR_BT_A2DP_BIT_RATE_UNKNOWN
 *
 *----------------------------------------------------------------------------*/
CsrBtCmA2dpBitRateReq *CsrBtCmA2dpBitRateReq_struct(CsrBtDeviceAddr  deviceAddr,
                                                    CsrUint8         streamIdx,
                                                    CsrUint32        bitRate);

#define CsrBtCmA2dpBitRateReqSend(_deviceAddr,_streamIdx,_bitRate) {    \
        CsrBtCmA2dpBitRateReq *msg__;                                   \
        msg__=CsrBtCmA2dpBitRateReq_struct(_deviceAddr,_streamIdx,_bitRate); \
        CsrBtCmPutMessageDownstream(msg__);}

#ifndef EXCLUDE_CSR_BT_CME_BH_FEATURE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSetAvStreamInfoReqSend
 *
 *  DESCRIPTION
 *      Inform AV start/stop to CM
 *
 *  PARAMETERS
*       streamIndex:- Identifies the stream uniquely
*       start:- Identifies start/stop of av stream
*       aclHandle:- Identifies the ACL Link
 *      l2capConnectionId:- Identifies the local L2CAP channel ID
 *      bitRate:- Identifies the bit rate of the codec in kbps
 *      sduSize:- Identifies the L2CAP MTU negotiated for av
 *      period:- Identifies the period in ms of codec data being available for transmission
 *      role:- Identifies the local device role, source or sink
 *      samplingFreq:- Identifies the sampling frequency of audio codec used
 *      codecType:- Identifies the codec type e.g. SBC/aptX etc
 *      codecLocation:- Identifies the location of the codec on/off-chip
 *
 *----------------------------------------------------------------------------*/
CsrBtCmSetAvStreamInfoReq *CsrBtCmSetAvStreamInfoReq_struct(CsrUint8 streamIdx,
                                                                CsrBool start,
                                                                CsrUint16 aclHandle,
                                                                CsrUint16 l2capConId,
                                                                CsrUint16 bitRate,
                                                                CsrUint16 sduSize,
                                                                CsrUint8 period,
                                                                CsrUint8 role,
                                                                CsrUint8 samplingFreq,
                                                                CsrUint8 codecType,
                                                                CsrUint8 codecLoc);
#define CsrBtCmSetAvStreamInfoReqSend(_streamIndex, _start, _aclHandle,_l2capConnectionId,_bitRate, _sduSize, _period, _role, _samplingFreq, _codecType, _codecLocation) { \
        CsrBtCmSetAvStreamInfoReq *msg__;                                   \
        msg__=CsrBtCmSetAvStreamInfoReq_struct(_streamIndex, _start, _aclHandle,_l2capConnectionId,_bitRate, _sduSize, _period, _role, _samplingFreq, _codecType, _codecLocation); \
        CsrBtCmPutMessageDownstream(msg__);}
#endif /* EXCLUDE_CSR_BT_CME_BH_FEATURE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmGetSecurityConfResSend
 *
 *  DESCRIPTION
 *      .....
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmGetSecurityConfRes *CsrBtCmGetSecurityConfRes_struct(CsrUint16           options,
                                                            dm_security_mode_t  securityMode,
                                                            dm_security_level_t securityLevelDefault,
                                                            CsrUint16           config,
                                                            CsrUint16           writeAuthEnable,
                                                            CsrUint8            mode3enc,
                                                            CsrUint16 *leEr, /* NULL, or points to 8*CsrUint16 */
                                                            CsrUint16 *leIr, /* NULL, or points to 8*CsrUint16 */
                                                            CsrUint16 leSmDivState,
                                                            CsrUint32 leSmSignCounter);

#define CsrBtCmGetSecurityConfResSend(_options,_securityMode,_securityLevelDefault,_config,_writeAuthEnable,_mode3enc,_leEr,_leIr,_leSmDivState,_leSmSignCounter) { \
        CsrBtCmGetSecurityConfRes *msg__;                               \
        msg__=CsrBtCmGetSecurityConfRes_struct(_options,_securityMode,_securityLevelDefault,_config,_writeAuthEnable,_mode3enc,_leEr,_leIr,_leSmDivState,_leSmSignCounter); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmBufferEmptyReq
 *
 *  DESCRIPTION
 *      Get notified when data Tx pipeline is empty
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmDataBufferEmptyReq *CsrBtCmDataBufferEmptyReq_struct(CsrBtConnId btConnId);

#define CsrBtCmDataBufferEmptyReqSend(_btConnId) {              \
        CsrBtCmDataBufferEmptyReq *msg__;                       \
        msg__=CsrBtCmDataBufferEmptyReq_struct(_btConnId);      \
        CsrBtCmPutMessageDownstream(msg__);}

#ifdef CSR_BT_LE_ENABLE

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSmLeSecurityReq
 *
 *  DESCRIPTION
 *      Highten security level towards peer.
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmSmLeSecurityReq *CsrBtCmSmLeSecurityReq_struct(CsrBtTypedAddr addr,
                                                      CsrUint16 l2caConFlags,
                                                      CsrUint16 context,
                                                      CsrUint16 securityRequirements);

#define CsrBtCmSmLeSecurityReqSend(_addr,_l2caConFlags,_context,_securityRequirements) { \
        CsrBtCmSmLeSecurityReq *msg__;                                  \
        msg__=CsrBtCmSmLeSecurityReq_struct(_addr,_l2caConFlags,_context,_securityRequirements); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmSmSetEncryptionKeySizeReqSend
 *
 *  DESCRIPTION
 *      Set low energy min/max encryption key sizes
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmSmSetEncryptionKeySizeReq *CsrBtCmSmSetEncryptionKeySizeReq_struct(CsrUint8 minKeySize,
                                                                          CsrUint8 maxKeySize);

#define CsrBtCmSmSetEncryptionKeySizeReqSend(_minKeySize,_maxKeySize) { \
        CsrBtCmSmSetEncryptionKeySizeReq *msg__;                        \
        msg__=CsrBtCmSmSetEncryptionKeySizeReq_struct(_minKeySize,_maxKeySize); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLePhysicalLinkStatusReqSend
 *
 *  DESCRIPTION
 *      Inform CM about LE physical link
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLePhysicalLinkStatusReq *CsrBtCmLePhysicalLinkStatusReq_struct(CsrBtTypedAddr address,
                                                                      CsrBool        radioType,
                                                                      CsrBool        status);

#define CsrBtCmLePhysicalLinkStatusReqSend(_a, _r, _s) {  \
        CsrBtCmLePhysicalLinkStatusReq *msg__; \
        msg__=CsrBtCmLePhysicalLinkStatusReq_struct(_a, _r, _s); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeLockSmQueueReqSend
 *
 *  DESCRIPTION
 *      Make sure that the SM queue is lock. This is done to ensure that
 *      we do not create two l2cap connection at the same time. Today l2cap
 *      cannnot handle this. E.g. called by GATT before it start to create an 
 *      outgoing Bredr connection 
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeLockSmQueueReq *CsrBtCmLeLockSmQueueReq_struct(CsrSchedQid appHandle,CsrBtTypedAddr address);

#define CsrBtCmLeLockSmQueueReqSend(_a,_addr) {  \
        CsrBtCmLeLockSmQueueReq *msg__; \
        msg__=CsrBtCmLeLockSmQueueReq_struct(_a,_addr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeUnlockSmQueueReqSend
 *
 *  DESCRIPTION
 *      Make sure that the SM queue is unlock again. This is done by GATT when is has
 *      create or tried to create an outgoing Bredr connection 
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeUnlockSmQueueReq *CsrBtCmLeUnlockSmQueueReq_struct(CsrBtTypedAddr address);

#define CsrBtCmLeUnlockSmQueueReqSend(_addr) {  \
        CsrBtCmLeUnlockSmQueueReq *msg__; \
        msg__=CsrBtCmLeUnlockSmQueueReq_struct(_addr); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeGetControllerInfoReqSend
 *
 *  DESCRIPTION
 *      Reads the total number of white list entries that can be stored in 
 *      the Controller and reads the states and state combinations that the 
 *      link layer supports
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeGetControllerInfoReq *CsrBtCmLeGetControllerInfoReq_struct(CsrSchedQid appHandle,
                                                                    CsrUint8    whiteListSize);

#define CsrBtCmLeGetControllerInfoReqSend(_a) {  \
        CsrBtCmLeGetControllerInfoReq *msg__; \
        msg__=CsrBtCmLeGetControllerInfoReq_struct(_a, 0); \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadRemoteUsedFeaturesReqSend
 *
 *  DESCRIPTION
 *      Reads the remote device supported LE Features.
 *      Refer LE Controller HCI command : HCI_LE_Read_Remote_Used_Features.
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadRemoteUsedFeaturesReq *CsrBtCmLeReadRemoteUsedFeaturesReq_struct(CsrSchedQid appHandle,
                                                                              CsrBtTypedAddr address);

#define CsrBtCmLeReadRemoteUsedFeaturesReqSend(_a, _addr) {               \
        CsrBtCmLeReadRemoteUsedFeaturesReq *msg__;                         \
        msg__ = CsrBtCmLeReadRemoteUsedFeaturesReq_struct(_a, _addr);      \
        CsrBtCmPutMessageDownstream(msg__);}

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadLocalSupportedFeaturesReqSend
 *
 *  DESCRIPTION
 *      Reads the supported LE feature in local controller
 *      Refer LE Controller HCI command : HCI_LE_Read_Local_Supported_Features.
 *
 * PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadLocalSupportedFeaturesReq *CsrBtCmLeReadLocalSupportedFeaturesReq_struct(CsrSchedQid appHandle);

#define CsrBtCmLeReadLocalSupportedFeaturesReqSend(_a) {                \
        CsrBtCmLeReadLocalSupportedFeaturesReq *msg__;                   \
        msg__ = CsrBtCmLeReadLocalSupportedFeaturesReq_struct(_a);       \
        CsrBtCmPutMessageDownstream(msg__);}

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadResolvingListSizeReqSend
 *
 *  DESCRIPTION
 *      Retrieves the size of local controller's resolving list.
 *      Refer LE Controller HCI command : HCI_LE_Read_Resolving_List_Size.
 *
 * PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadResolvingListSizeReq *CsrBtCmLeReadResolvingListSizeReq_struct(CsrSchedQid appHandle);

#define CsrBtCmLeReadResolvingListSizeReqSend(_a)             \
    do                                                        \
    {                                                         \
        CsrBtCmLeReadResolvingListSizeReq *msg__;             \
        msg__ = CsrBtCmLeReadResolvingListSizeReq_struct(_a); \
        CsrBtCmPutMessageDownstream(msg__);                   \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeSetPrivacyModeReqSend
 *
 *  DESCRIPTION
 *      Sets the privacy mode of the device if LL_PRIVACY feature supported into
 *      controller. Refer LE Controller HCI command:HCI_LE_Set_Privacy_Mode.
 *
 * PARAMETERS
 *      idAddress   : Peer device identity address.
 *      privacyMode : Value of privacy mode for the device to be set as below.
 *                    0x00 : Network privacy mode
 *                    0x01 : Device privacy mode
 *----------------------------------------------------------------------------*/
CsrBtCmLeSetPrivacyModeReq *CsrBtCmLeSetPrivacyModeReq_struct(CsrSchedQid appHandle,
                                                              CsrBtTypedAddr idAddress,
                                                              CsrBtPrivacyMode privacyMode);

#define CsrBtCmLeSetPrivacyModeReqSend(_a,                                  \
                                       _addr,                               \
                                       _privacyMode)                        \
    do                                                                      \
    {                                                                       \
        CsrBtCmLeSetPrivacyModeReq *msg__;                                  \
        msg__ = CsrBtCmLeSetPrivacyModeReq_struct(_a, _addr, _privacyMode); \
        CsrBtCmPutMessageDownstream(msg__);                                 \
    } while(0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmLeReadLocalIrkReqSend
 *
 *  DESCRIPTION
 *      Read local device IRK for a remote device.
 *
 * PARAMETERS
 *      deviceAddr : remote device address.
 *----------------------------------------------------------------------------*/
CsrBtCmLeReadLocalIrkReq *CsrBtCmLeReadLocalIrkReq_struct(CsrSchedQid appHandle,
                                                          CsrBtTypedAddr deviceAddr);

#define CsrBtCmLeReadLocalIrkReqSend(_a,                     \
                                     _addr)                  \
    do                                                       \
    {                                                        \
        CsrBtCmLeReadLocalIrkReq *msg__;                     \
        msg__ = CsrBtCmLeReadLocalIrkReq_struct(_a, _addr);  \
        CsrBtCmPutMessageDownstream(msg__);                  \
    } while(0)
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
#endif /* CSR_BT_LE_ENABLE */

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmReadEncryptionKeySizeReqSend
 *
 *  DESCRIPTION
 *      Read the encryption key size for particular link
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmReadEncryptionKeySizeReq *CsrBtCmReadEncryptionKeySizeReq_struct(CsrSchedQid appHandle,
                                                                        CsrBtTypedAddr addr,
                                                                        CsrUint16 context);

#define CsrBtCmReadEncryptionKeySizeReqSend(_appHandle, _addr, _context) { \
        CsrBtCmReadEncryptionKeySizeReq *msg__;                         \
        msg__=CsrBtCmReadEncryptionKeySizeReq_struct(_appHandle, _addr, _context); \
        CsrBtCmPutMessageDownstream(msg__);}


/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmL2caGetChannelInfoReqSend
 *
 *  DESCRIPTION
 *      Get the ACL channel ID and remote CID
 *
 *  PARAMETERS
 *
 *----------------------------------------------------------------------------*/
CsrBtCmL2caGetChannelInfoReq *CsrBtCmL2caGetChannelInfoReq_struct(CsrBtConnId btConnId,
                                                                  CsrSchedQid appHandle);

#define CsrBtCmL2caGetChannelInfoReqSend(_c, _ah) {  \
        CsrBtCmL2caGetChannelInfoReq *msg__; \
        msg__=CsrBtCmL2caGetChannelInfoReq_struct(_c, _ah); \
        CsrBtCmPutMessageDownstream( msg__);}

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
void CsrBtRespondCmPortNegInd(void *msg);
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */


#ifdef CSR_STREAMS_ENABLE
void *CsrBtCmStreamDataCfmGet(void *streamSink, CsrUint8 protocol, CsrUint16 context);

void *CsrBtCmStreamsDataIndGet(void *streamSource,
                               CsrUint8 protocol,
                               CsrUint16 context);
#endif

#ifdef CSR_TARGET_PRODUCT_VM
#define CsrBtCmUpdateInternalPeerAddrReqSend(_newPeerDeviceAddr, _oldPeerDeviceAddr) \
    do                                                                               \
    {                                                                                \
        CsrBtCmUpdateInternalPeerAddressReq *_req = (CsrBtCmUpdateInternalPeerAddressReq *) CsrPmemAlloc(sizeof(*_req)); \
        _req->type = CSR_BT_CM_UPDATE_INTERNAL_PEER_ADDR_REQ;                        \
        _req->newPeerAddr = _newPeerDeviceAddr;                                      \
        _req->oldPeerAddr = _oldPeerDeviceAddr;                                      \
        CsrBtCmPutMessageDownstream(_req);                                           \
    } while (0)

void CsrBtCmUpdateScoHandle(CsrBtConnId connId,
                            hci_connection_handle_t scoHandle);
#endif /* CSR_TARGET_PRODUCT_VM */

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmRfcDisconnectRspSend
 *
 *  DESCRIPTION
 *      When the flag CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP is enabled, application
 *      uses this API to respond to RFC_DISCONNECT_IND. When the flag is disabled,
 *      synergy CM responds to RFC_DISCONNECT_IND and sends notification to
 *      application.
 *
 *  PARAMETERS
 *      btConnId: connection ID for which RFC_DISCONNECT_IND is received.
 *----------------------------------------------------------------------------*/
#define CsrBtCmRfcDisconnectRspSend(_btConnId)                                  \
    do                                                                          \
    {                                                                           \
        CsrBtCmRfcDisconnectRsp *rsp;                                           \
        rsp = (CsrBtCmRfcDisconnectRsp *) CsrPmemZalloc(sizeof(*rsp));          \
        rsp->type     = CSR_BT_CM_RFC_DISCONNECT_RSP;                           \
        rsp->btConnId = _btConnId;                                              \
        CsrBtCmPutMessageDownstream(rsp);                                       \
    } while (0)
#else
#define CsrBtCmRfcDisconnectRspSend(_btConnId)                                  \
    do                                                                          \
    {                                                                           \
        CSR_UNUSED(_btConnId);                                                  \
    } while (0)
#endif /* CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP */

#ifdef EXCLUDE_CSR_BT_SC_MODULE
void CsrBtScActivateReqSend(CsrSchedQid appHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDatabaseReqSend
 *
 *  DESCRIPTION
 *      Function to build and send a CSR_BT_CM_DATABASE_REQ message to the SC message
 *      queue.
 *
 *  PARAMETERS
 *      appHandle   where the CFM shall be sent to
 *      deviceAddr  what BT device in the database to access
 *      opcode      DB operation code
 *      keyType     key type of data (only used during write operations)
 *      key         Union of CM keys
 *----------------------------------------------------------------------------*/
void CsrBtCmDatabaseReqSend(CsrSchedQid appHandle,
                            CsrBtAddressType addressType,
                            const CsrBtDeviceAddr *deviceAddr,
                            CsrUint8 opcode,
                            CsrBtCmKeyType keyType,
                            CsrBtCmKey *key);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmDatabaseReqSendNow
 *
 *  DESCRIPTION
 *      Synchronous function to process write database request.
 *      Returns CSR_BT_RESULT_CODE_CM_SUCCESS in case of success.
 *
 *  PARAMETERS
 *      deviceAddr  pointer to the bluetooth address of the device
 *      addressType type of the bluetooth address
 *      keyType     type of the key provided
 *      keys        pointer to the bredr/le keys based on the key type
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtCmDatabaseReqSendNow(const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtAddressType addressType,
                                          CsrBtCmKeyType keyType,
                                          const void *keys);

CsrBtResultCode CsrBtScSetSecInLevel(CsrUint16 *secOutLevel,
                                     CsrUint16 secLevel,
                                     CsrUint16 secManLevel,
                                     CsrUint16 secDefLevel,
                                     CsrBtResultCode successCode,
                                     CsrBtResultCode errorCode);
void CsrBtScMapSecInLevel(CsrUint16 secInput, CsrUint16 *secOutput);
CsrBtResultCode CsrBtScSetSecOutLevel(CsrUint16 *secOutLevel,
                                      CsrUint16 secLevel,
                                      CsrUint16 secManLevel,
                                      CsrUint16 secDefLevel,
                                      CsrBtResultCode successCode,
                                      CsrBtResultCode errorCode);
void CsrBtScMapSecOutLevel(CsrUint16 secInput, CsrUint16 *secOutput);
CsrBool CsrBtScDeviceAuthorised(CsrBtAddressType addrType,
                                const CsrBtDeviceAddr *addr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmScRefreshAllDevicesNow
 *
 *  DESCRIPTION
 *      Synchronous API to ask DM to update all auth entries for all devices.
 *
 *  PARAMETERS
 *      None
 *----------------------------------------------------------------------------*/
void CsrBtCmScRefreshAllDevicesNow(void);

#define CsrBtScDeviceBredrPaired(_addressType, _deviceAddr)                       \
    (CsrBtTdDbGetBredrKey(_addressType, _deviceAddr, NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)

#ifdef CSR_BT_LE_ENABLE
#define CsrBtScDeviceLePaired(_addressType, _deviceAddr)                          \
    (CsrBtTdDbGetLeKeys(_addressType, _deviceAddr, NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
#endif

#endif /* EXCLUDE_CSR_BT_SC_MODULE */

#ifdef CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtCmIsQhsPhyConnected
 *
 *  DESCRIPTION
 *      Internal API to check if connection with the given deviceAddr is
 *      using QHS PHY or not.
 *
 *  PARAMETERS
 *      deviceAddr  bd address of the device for which QHS PHY is being checked.
 *----------------------------------------------------------------------------*/
CsrBool CsrBtCmIsQhsPhyConnected(CsrBtDeviceAddr *deviceAddr);
#endif /* CSR_BT_INSTALL_CM_QHS_PHY_SUPPORT */

#ifdef CSR_BT_INSTALL_CM_SWB_DISABLE_STATE
/*----------------------------------------------------------------------------*
 *  NAME
 *      CmIsSWBDisabled
 *
 *  DESCRIPTION
 *      Internal API to check if SWB is disabled for a particular handset.
 *
 *  PARAMETERS
 *      deviceAddr  bd address of the device for which SWB status is checked.
 *----------------------------------------------------------------------------*/
CsrBool CmIsSWBDisabled(CsrBtDeviceAddr *deviceAddr);
#else /* ! CSR_BT_INSTALL_CM_SWB_DISABLE_STATE */
#define CmIsSWBDisabled(deviceAddr)               \
             (CSR_UNUSED(deviceAddr), FALSE)
#endif /* CSR_BT_INSTALL_CM_SWB_DISABLE_STATE */

#ifdef INSTALL_CONTEXT_TRANSFER
void CsrBtCmLeAclOpenedIndExtHandler(DM_ACL_OPENED_IND_T *prim);
#endif

#ifdef __cplusplus
}
#endif

#endif

