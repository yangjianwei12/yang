#ifndef CSR_BT_CMN_SDC_RFC_UTIL_H__
#define CSR_BT_CMN_SDC_RFC_UTIL_H__
/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_bt_result.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_private_lib.h"
#include "csr_bt_util.h"
#include "csr_bt_cmn_sdr_tagbased_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Defines the states                                                           */
#define CMN_SDC_RFC_IDLE_STATE                          0x00
#define CMN_SDC_RFC_SEARCH_STATE                        0x01
#define CMN_SDC_RFC_ATTR_STATE                          0x02
#define CMN_SDC_RFC_CLOSE_SEARCH_STATE                  0x03
#define CMN_SDC_RFC_CANCEL_STATE                        0x04
#define CMN_SDC_RFC_REGISTER_STATE                      0x05
#define CMN_SDC_RFC_SELECT_SERVICE_RECORD_HANDLE_STATE  0x06
#define CMN_SDC_RFC_CONNECT_STATE                       0x07
#define CMN_SDC_RFC_PORTNEG_PENDING_STATE               0x08

/* Defines the maximum number of different uuid that can be requested in one
   request                                                                      */
#define CMN_SDC_MAX_NUM_OF_UUID                     0xff

typedef CsrUint8 CsrSdcOptCbSupportMask;

#define CSR_SDC_OPT_CB_SEARCH_RESULT_MASK           ((CsrSdcOptCbSupportMask) (1 << CSR_SDC_OPT_CB_SEARCH_RESULT))
#define CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK       ((CsrSdcOptCbSupportMask) (1 << CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE))
#define CSR_SDC_OPT_CB_RFC_CON_SET_PORT_PAR_MASK    ((CsrSdcOptCbSupportMask) (1 << CSR_SDC_OPT_UTIL_RFC_CON_SET_PORT_PAR))
#define CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK          ((CsrSdcOptCbSupportMask) (1 << CSR_SDC_OPT_RFC_CON_RESULT))

typedef enum
{
    CSR_SDC_OPT_CB_SEARCH_RESULT,             /* CmnSdcResultFuncType */
    CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE, /* CmnRfcConSelectServiceHandleFuncType */
    CSR_SDC_OPT_UTIL_RFC_CON_SET_PORT_PAR,    /* CsrBtUtilRfcConSetPortParFuncType */
    CSR_SDC_OPT_RFC_CON_RESULT,               /* CmnRfcConResultFuncType */
} CsrSdcOptCallbackType;

typedef struct
{
    CsrUint8    *begin; /* Contains the pointer to the beginning of a region in the received attribute list */
    CsrUint8    *end;   /* Contains the pointer to the end of a region in the received attribute list */
} cmnSdcSdrRegion;

typedef struct
{
    void                        *instData;
    void                        *cmSdcRfcInstData;
    CsrBtDeviceAddr             deviceAddr;
    CsrUint8                    serverChannel;
    CsrUint16                   entriesInSdpTaglist;
    CmnCsrBtLinkedListStruct    *sdpTagList;
} CsrRfcConSelectServiceHandleType;

typedef struct
{
    void                     *instData;
    CsrBtDeviceAddr          deviceAddr;
    CsrUint8                 serverChannel;
    RFC_PORTNEG_VALUES_T     *portPar;
    CsrBool                  request;
} CsrBtUtilRfcConSetPortParType;

typedef struct
{
    void                        *instData;
    CsrUint8                    localServerCh;
    CsrUint32                   btConnId;
    CsrBtDeviceAddr             deviceAddr;
    CsrUint16                   maxFrameSize;
    CsrBool                     validPortPar;
    RFC_PORTNEG_VALUES_T        *portPar;
    CsrBtResultCode             resultCode;
    CsrBtSupplier               resultSupplier;
    CmnCsrBtLinkedListStruct    *sdpTag;
} CsrRfcConResultType;

typedef struct
{
    void                        *instData;
    CmnCsrBtLinkedListStruct    *sdpTagList;
    CsrBtDeviceAddr             deviceAddr;
    CsrBtResultCode             resultCode;
    CsrBtSupplier               resultSupplier;
} CsrSdcResultFuncType;

typedef void (*CmnSdcRfcCallbackFuncType)(CsrSdcOptCallbackType cbType, void *context);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtUtilRfcConStart
 *
 *  DESCRIPTION
 *      This function will start a CM RFC SEARCH and RFCOMM connect operation
 *
 *    PARAMETERS
 *      *instData               : The overloaded void pointer to the
 *                                profiles own instance data
 *
 *      *cmSdcRfcInstData       : The overloaded void pointer to this
 *                                library private instance data
 *
 *      *sdpTag                 : The info that the profile must read from the
 *                                peer device service record
 *
 *      deviceAddr              : The Bluetooth address of the device to
 *                                connect to
 *
 *      secLevel                : Sets up the security level for new outgoing
 *                                connection
 *
 *      requestPortPar          : If TRUE, this command is a request for the remote
 *                                device to report its current port parameter values.
 *                                Please note that this parameter is only valid if
 *                                the *portPar is not NULL
 *
 *      *portPar                : If *portPar is not NULL it is requested to set port
 *                                parameters doing the connect procedure. If *portPar
 *                                is set to NULL the value of requestPortPar is not
 *                                important. Please note the normally this parameter
 *                                is set to NULL.
 *
 *      mtu                     : Specify MTU size for the RFCOMM connection
 *
 *      modemStatus             : Modem signal values
 *
 *      mscTimeout              : Time in msec that the RFC shall wait for MSC at connection
 *                                time.
 *      minEncKeySize             Minimum encryption key size
 *----------------------------------------------------------------------------*/
CsrBool CsrBtUtilRfcConStart(void                    * instData,
                                      void                    * cmSdcRfcInstData,
                                      CmnCsrBtLinkedListStruct * sdpTag,
                                      CsrBtDeviceAddr            deviceAddr,
                                      dm_security_level_t     secLevel,
                                      CsrBool                  requestPortPar,
                                      RFC_PORTNEG_VALUES_T     * portPar,
                                      CsrUint16                mtu,
                                      CsrUint8                 modemStatus,
                                      CsrUint8                 mscTimeout,
                                      CsrUint8                 minEncKeySize);



/* Function to cancel the CM RFC connect procedure search                       */
CsrBool CsrBtUtilRfcConCancel(void *instData, void *cmSdcRfcInstData);


/* Function to select one or more service record handle(s) doing the CM RFC
   connect procedure. Please note that in the case that nofServiceHandleIndicis
   > 1 then will this library try to connect to serviceHandleIndexList[0] first,
   if this fails then serviceHandleIndexList[1] and so on. In this way the
   profile gives priority of which service recod handle it perfers              */
CsrBool CsrBtUtilRfcConSetServiceHandleIndexList(void           * instData,
                                                       void           * cmSdcRfcInstData,
                                                       CsrUint16       * serviceHandleIndexList,
                                                       CsrUint16       nofServiceHandleIndicis);

#ifdef CSR_BT_INSTALL_SDC_SET_PORT_PAR
/* Function to respond a portneg ind msg received doing the CM RFC connect
   procedure                                                                    */
CsrBool CsrBtUtilRfcConSetPortPar(void * cmSdcRfcInstData, RFC_PORTNEG_VALUES_T portPar);
#endif

/* Function to register the callback functions that is needed to used the Cm Rfc
   connect procedure. Note by setting the callback function of type
   CmnRfcConSelectServiceHandleFuncType to NULL the service record handle will
   be auto selected by the CM RFC connect procedure. Also note that by setting
   the callback function of type CsrBtUtilRfcConSetPortParFuncType to NULL the portPar
   will be automatic return with default parameters by the CM RFC connect
   procedure                                                                    */
void * CsrBtUtilSdpRfcConInit(CmnSdcRfcCallbackFuncType            sdcRfcResultFunc,
                                     CsrSdcOptCbSupportMask               cbSupportMask,
                                     CsrSchedQid                          appHandle);

/* Function to does the same thing as CsrBtUtilSdpRfcConInit. The different part
   is that the instIdentifier is return in the CM upstream messages. Note this
   is an CsrUint8 because in the CM_SDC_XXX message the instIdentifier is the
   local server channel                                                         */
void * CsrBtUtilSdpRfcInit(CmnSdcRfcCallbackFuncType            sdcRfcResultFunc,
                                  CsrSdcOptCbSupportMask               cbSupportMask,
                                  CsrSchedQid                          appHandle,
                                  CsrUint8                             instIdentifier);


/* Function to handle upstream CM messages, in the group
   NUM_OF_CM_BASIC_PLUS_SDC_SEND_PRIMS, doing the Cm Rfc Connect procedure      */
CsrBool CsrBtUtilRfcConCmMsgHandler(void * instData,
                                          void * cmSdcRfcInstData,
                                          void * msg);


/* Function to check if the CsrBtUtilRfcConCmMsgHandler is able to handle the
   incoming CM message. This function will return TRUE if it can handle the
   message otherwise FALSE                                                      */
CsrBool CsrBtUtilRfcConVerifyCmMsg(void *msg);

/* Function to register the callback function that is needed to used the Cm SDP
   search procedure.                                                            */
void * CsrBtUtilSdcInit(CmnSdcRfcCallbackFuncType sdcRfcResultFunc,
                               CsrSchedQid               appHandle);

/* Function to deregister the callback functions again. Note that this
   function must only be called when the Cm SDP search
   procedure is idle                                                            */
void CsrBtUtilSdcRfcDeinit(void ** cmSdcRfcInstData);

/* Function to start the Cm SDP search procedure                                */
CsrBool CsrBtUtilSdcSearchStart(void               * instData,
                                void                     * cmSdcRfcInstData,
                                CmnCsrBtLinkedListStruct * sdpTag,
                                CsrBtDeviceAddr            deviceAddr);

/* Function to cancel a SDP search                                              */
CsrBool CsrBtUtilSdcSearchCancel(void *instData, void *cmSdcRfcInstData);

/* Function to handle upstream CM messages, in the group
   NUM_OF_CM_PROFILE_SDC_SEND_PRIMS, doing the Cm SDP search                    */
CsrBool CsrBtUtilSdcCmMsgHandler(void * instData,
                                       void * cmSdcRfcInstData,
                                       void * msg);


/* Function to check if the CsrBtUtilSdcCmMsgHandler is able to handle the
   incoming CM message. This function will return TRUE if it can handle the
   message otherwise FALSE                                                      */
CsrBool CsrBtUtilSdcVerifyCmMsg(void *msg);

/* Instance data used intern by cmn_sdp_connect_util functions                  */
typedef struct
{
    CsrBtDeviceAddr          deviceAddr;
    CsrUint8                 numOfSdrOutEntries;
    CsrUint8                 numOfSdrAttr;
    CsrUint8                 sdrEntryIndex;
    CsrUint8                 sdrAttrIndex;
    CsrUint16                uuidType;
    CsrUint16                maxFrameSize;
    CsrBtUuid32              serviceHandle;
    dm_security_level_t      secLevel;
    CsrUint8                 localServerCh;
    CsrBtConnId              btConnId;
    CsrUint8                 state:4;
    CsrUint8                 minEncKeySize:4;
    CsrBool                  reqPortPar:1;
    CsrBool                  validPortPar:1;
    CsrBool                  obtainServer:1;
    CsrBool                  reqControl:1;
    RFC_PORTNEG_VALUES_T     portPar;
    CsrUint8                 numOfServiceHandleIndicis;
    CsrUint16                * serviceHandleIndexList;
    CmnCsrBtLinkedListStruct * sdpInTagList; /* Contains the service UUID and attributes requested by the profile */
    CmnCsrBtLinkedListStruct * sdpOutTagList; /* Contains the attribute values given as response by the remote device for the attributes requested */
    CsrUint8                 modemStatus; 
    CsrUint8                 scTimeout; /* check for scope l2cap and rfcomm */
} CmnSdcRfcPriInstType;

typedef struct
{
    CsrSchedQid                             appHandle;
    CmnSdcRfcCallbackFuncType               sdcRfcResultFunc;
    CmnSdcRfcPriInstType                    *privateInst;
    CsrUint8                                instId;
    CsrSdcOptCbSupportMask                  cbSupportMask:4;
} CmnSdcRfcInstType;

typedef CsrBool (*CmnSdcRfcType)(void *instData, CmnSdcRfcInstType *inst,
                                CmnSdcRfcPriInstType *priInst, void * msg);


#ifdef __cplusplus
}
#endif

#endif /*_CMN_SDP_RFC_UTIL_H */

