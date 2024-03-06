#ifndef CSR_BT_MAPC_PRIM_H__
#define CSR_BT_MAPC_PRIM_H__
/******************************************************************************
 Copyright (c) 2009-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "csr_bt_obex.h"
#include "csr_bt_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* search_string="CsrBtMapcPrim" */
/* conversion_rule="UPPERCASE_START_AND_REMOVE_UNDERSCORES" */

typedef CsrPrim    CsrBtMapcPrim;

/*******************************************************************************
 * Primitive definitions
 *******************************************************************************/
#define CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST              (0x0000)

#define CSR_BT_MAPC_GET_INSTANCE_IDS_REQ                ((CsrBtMapcPrim) (0x0000 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_CONNECT_REQ                         ((CsrBtMapcPrim) (0x0001 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_CANCEL_CONNECT_REQ                  ((CsrBtMapcPrim) (0x0002 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_DISCONNECT_REQ                      ((CsrBtMapcPrim) (0x0003 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SELECT_MAS_INSTANCE_RES             ((CsrBtMapcPrim) (0x0004 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_FOLDER_REQ                      ((CsrBtMapcPrim) (0x0005 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_BACK_FOLDER_REQ                 ((CsrBtMapcPrim) (0x0006 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_ROOT_FOLDER_REQ                 ((CsrBtMapcPrim) (0x0007 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_FOLDER_LISTING_REQ              ((CsrBtMapcPrim) (0x0008 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_FOLDER_LISTING_RES              ((CsrBtMapcPrim) (0x0009 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_LISTING_REQ             ((CsrBtMapcPrim) (0x000A + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_LISTING_RES             ((CsrBtMapcPrim) (0x000B + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_REQ                     ((CsrBtMapcPrim) (0x000C + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_RES                     ((CsrBtMapcPrim) (0x000D + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_MESSAGE_STATUS_REQ              ((CsrBtMapcPrim) (0x000E + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_PUSH_MESSAGE_REQ                    ((CsrBtMapcPrim) (0x000F + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_PUSH_MESSAGE_RES                    ((CsrBtMapcPrim) (0x0010 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_UPDATE_INBOX_REQ                    ((CsrBtMapcPrim) (0x0011 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_ABORT_REQ                           ((CsrBtMapcPrim) (0x0012 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_NOTIFICATION_REGISTRATION_REQ       ((CsrBtMapcPrim) (0x0013 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_EVENT_NOTIFICATION_RES              ((CsrBtMapcPrim) (0x0014 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SECURITY_IN_REQ                     ((CsrBtMapcPrim) (0x0015 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SECURITY_OUT_REQ                    ((CsrBtMapcPrim) (0x0016 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_REGISTER_QID_REQ                    ((CsrBtMapcPrim) (0x0017 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_REQ    ((CsrBtMapcPrim) (0x0018 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_CONVERSATION_LISTING_REQ        ((CsrBtMapcPrim) (0x0019 + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_CONVERSATION_LISTING_RES        ((CsrBtMapcPrim) (0x001A + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_OWNER_STATUS_REQ                ((CsrBtMapcPrim) (0x001B + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_OWNER_STATUS_REQ                ((CsrBtMapcPrim) (0x001C + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_NOTIFICATION_FILTER_REQ         ((CsrBtMapcPrim) (0x001D + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST))

#define CSR_BT_MAPC_PRIM_DOWNSTREAM_HIGHEST                              (0x001D + CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST)

/*******************************************************************************/

#define CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST                                 (0x0000 + CSR_PRIM_UPSTREAM)

#define CSR_BT_MAPC_GET_INSTANCE_IDS_CFM                ((CsrBtMapcPrim) (0x0000 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_CONNECT_CFM                         ((CsrBtMapcPrim) (0x0001 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_DISCONNECT_IND                      ((CsrBtMapcPrim) (0x0002 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SELECT_MAS_INSTANCE_IND             ((CsrBtMapcPrim) (0x0003 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_FOLDER_CFM                      ((CsrBtMapcPrim) (0x0004 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_BACK_FOLDER_CFM                 ((CsrBtMapcPrim) (0x0005 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_ROOT_FOLDER_CFM                 ((CsrBtMapcPrim) (0x0006 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_FOLDER_LISTING_IND              ((CsrBtMapcPrim) (0x0007 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_FOLDER_LISTING_CFM              ((CsrBtMapcPrim) (0x0008 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_LISTING_IND             ((CsrBtMapcPrim) (0x0009 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_LISTING_CFM             ((CsrBtMapcPrim) (0x000A + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_IND                     ((CsrBtMapcPrim) (0x000B + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MESSAGE_CFM                     ((CsrBtMapcPrim) (0x000C + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_MESSAGE_STATUS_CFM              ((CsrBtMapcPrim) (0x000D + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_PUSH_MESSAGE_IND                    ((CsrBtMapcPrim) (0x000E + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_PUSH_MESSAGE_CFM                    ((CsrBtMapcPrim) (0x000F + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_UPDATE_INBOX_CFM                    ((CsrBtMapcPrim) (0x0010 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_ABORT_CFM                           ((CsrBtMapcPrim) (0x0011 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_NOTIFICATION_REGISTRATION_CFM       ((CsrBtMapcPrim) (0x0012 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_NOTIFICATION_REGISTRATION_OFF_IND   ((CsrBtMapcPrim) (0x0013 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_EVENT_NOTIFICATION_IND              ((CsrBtMapcPrim) (0x0014 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_EVENT_NOTIFICATION_ABORT_IND        ((CsrBtMapcPrim) (0x0015 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SECURITY_IN_CFM                     ((CsrBtMapcPrim) (0x0016 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SECURITY_OUT_CFM                    ((CsrBtMapcPrim) (0x0017 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_CFM    ((CsrBtMapcPrim) (0x0018 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_CONVERSATION_LISTING_IND        ((CsrBtMapcPrim) (0x0019 + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_CONVERSATION_LISTING_CFM        ((CsrBtMapcPrim) (0x001A + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_GET_OWNER_STATUS_CFM                ((CsrBtMapcPrim) (0x001B + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_OWNER_STATUS_CFM                ((CsrBtMapcPrim) (0x001C + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))
#define CSR_BT_MAPC_SET_NOTIFICATION_FILTER_CFM         ((CsrBtMapcPrim) (0x001D + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST))

#define CSR_BT_MAPC_PRIM_UPSTREAM_HIGHEST                                (0x001D + CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST)

#define CSR_BT_MAPC_PRIM_DOWNSTREAM_COUNT               (CSR_BT_MAPC_PRIM_DOWNSTREAM_HIGHEST + 1 - CSR_BT_MAPC_PRIM_DOWNSTREAM_LOWEST)
#define CSR_BT_MAPC_PRIM_UPSTREAM_COUNT                 (CSR_BT_MAPC_PRIM_UPSTREAM_HIGHEST + 1 - CSR_BT_MAPC_PRIM_UPSTREAM_LOWEST)
/*******************************************************************************
 * End primitive definitions
 *******************************************************************************/
/*** MAPC MULTI INSTANCE HANDLING ***/
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid        appHandle;                       /* application handle */
} CsrBtMapcGetInstanceIdsReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUint8            instanceIdsListSize;            /* number of items in instanceIdsList, not length in bytes */
    CsrSchedQid        *instanceIdsList;                /* list of instances  */
} CsrBtMapcGetInstanceIdsCfm;

/*** CONNECTION HANDLING ***/
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         appHandle;                      /* application handle */
    CsrUint16           maxPacketSize;                  /* maximum OBEX packet size intended to be used */
    CsrBtDeviceAddr     deviceAddr;                     /* device address to connect to */
    CsrUint16           windowSize;                     /* size of the tx queue. Controls how many packets the 
                                                           OBEX profile (and lower protocol layers) is allowed 
                                                           to cache on the data receive side. A value of 0
                                                           causes the system to auto-detect this value.*/
} CsrBtMapcConnectReq;

typedef struct
{
    CsrBtMapcPrim               type;                   /* primitive type */
    CsrSchedQid                 instanceId;             /* id of the MAPC instance that generated this event */
    CsrUint8                    masInstanceId;          /* MASInstanceID from SDP record */
    CsrBtMapMesSupport          supportedMessages;      /* Bit pattern of supported message types in the server */
    CsrUint16                   obexPeerMaxPacketSize;  /* Max OBEX packet size supported by the server */
    CsrBtResultCode             resultCode;             /* Result code of the operation. Possible values depend on 
                                                           the value of resultSupplier. If the resultSupplier is 
                                                           CSR_BT_SUPPLIER_IRDA_OBEX,  then the possible result codes 
                                                           can be found in csr_bt_obex.h. There could be other result 
                                                           suppliers also whose error code could be supplied as part 
                                                           of the csr_bt_xxx_prim.h file corresponding to each supplier. 
                                                           All values which are currently not specified in the respective 
                                                           prim.h file are regarded as reserved and the application should 
                                                           consider them as errors. */
    CsrBtSupplier               resultSupplier;         /* Specifies the supplier of the result given in resultCode. Possible 
                                                           values can be found in csr_bt_result.h. */
    CsrBtConnId                 btConnId;               /* Global Bluetooth connection ID */
    CsrBtMapSupportedFeatures   features;               /* Remote supported features */
    CsrBtDeviceName             serviceName;            /* ServiceName from SDP record */
} CsrBtMapcConnectCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
} CsrBtMapcCancelConnectReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             normalObexDisconnect;           /* if TRUE an OBEX disconnect is issued, if FALSE the transport channel 
                                                           is torn down without sending the OBEX disconnect first */
} CsrBtMapcDisconnectReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrBtReasonCode     reasonCode;                     /* Reason code of the operation. Possible values depend on the value 
                                                           of reasonSupplier. If the reasonSupplier is CSR_BT_SUPPLIER_CM, 
                                                           then the possible reason codes can be found in csr_bt_cm_prim.h. 
                                                           If the reasonSupplier == CSR_BT_SUPPLIER_OBEX, then the possible 
                                                           result codes can be found in csr_bt_obex.h. All values which are 
                                                           currently not specified are the respective prim.h files or csr_bt_obex.h 
                                                           are considered as reserved and the application should consider them as errors. */
    CsrBtSupplier       reasonSupplier;                 /* Specifies the supplier of the reason given in reasonCode. Possible 
                                                           values can be found in csr_bt_result.h. */
} CsrBtMapcDisconnectInd;

typedef struct
{
    CsrBtDeviceName     serviceName;                    /* ServiceName from SDP record */
    CsrUint8            masInstanceId;                  /* MASInstanceID from SDP record */
    CsrBtMapMesSupport  supportedMessages;              /* bit pattern of supported message types in this MAS instance */
    CsrBtMapSupportedFeatures features;                 /* remote supported features */
} CsrBtMapcMasInstance;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrBtMapcMasInstance *masInstanceList;              /* pointer to the list of available MAS instances on the peer */
    CsrUint16           masInstanceListSize;            /* Number of items in masInstanceList */
} CsrBtMapcSelectMasInstanceInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             proceedWithConnection;          /* if TRUE the connection establishment is continued with the given 
                                                           masInstanceId. If FALSE the profile will abort the connection 
                                                           establishment and send a CSR_BT_MAPC_CONNECT_CFM with error */
    CsrUint8            masInstanceId;                  /* MASInstanceID to connect */
} CsrBtMapcSelectMasInstanceRes;

/*** BROWSING ***/
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *folderName;                     /* null terminated name string of the folder to set */
} CsrBtMapcSetFolderReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
} CsrBtMapcSetBackFolderReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
} CsrBtMapcSetRootFolderReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the SetFolder operation */
} CsrBtMapcSetFolderCfm;

typedef CsrBtMapcSetFolderCfm CsrBtMapcSetBackFolderCfm;
typedef CsrBtMapcSetFolderCfm CsrBtMapcSetRootFolderCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUint16           maxListCount;                   /* maximum number of folders in the listing */
    CsrUint16           listStartOffset;                /* offset of where to start the listing */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetFolderListingReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetFolderListingRes;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint16           fullFolderListingSize;          /* number of bytes in the complete folder listing */
    CsrUint16           folderListingRetrieved;         /* number of bytes of the folder listing received so far */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. 
                                                           NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8            *payload;                       /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetFolderListingInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the GET request */
    CsrUint16           fullFolderListingSize;          /* number of bytes in the complete folder listing */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. 
                                                           NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetFolderListingCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *folderName;                     /* null terminated name string of the folder from where the current message listing is to be retreived: NB: if NULL this means current folder */
    CsrUint16           maxListCount;                   /* maximum number of messages in the listing */
    CsrUint16           listStartOffset;                /* offset of where to start the listing */
    CsrUint8            maxSubjectLength;               /* maximum string length allowed of the subject field */
    CsrBtMapMesParms    parameterMask;                  /* bitmask of relevant parameters for the message listing. NB: a bit value of 1 means that the parameter should be present and a value of 0 means it should be filtered out */
    CsrBtMapMesTypes    filterMessageType;              /* bitmask specifying which message types to include/exclude in the listing. NB: a bit value of 1 means that the message type should be filtered and a value of 0 means that it should be present */
    CsrUtf8String      *filterPeriodBegin;              /* null terminated time string */
    CsrUtf8String      *filterPeriodEnd;                /* null terminated time string */
    CsrBtMapReadStatus  filterReadStatus;               /* bitmask specifying if filtering should be done on behalf of the read status */
    CsrUtf8String      *filterRecipient;                /* null terminated recipient string */
    CsrUtf8String      *filterOriginator;               /* null terminated originator string */
    CsrBtMapPriority    filterPriority;                 /* bitmask specifying which priority type to include in the listing */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
    CsrUtf8String      *conversationId;                 /* Conversation ID (optional). Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUtf8String      *filterMessageHandle;            /* null terminated message handle string */
} CsrBtMapcGetMessageListingReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetMessageListingRes;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrBtMapNewMessage  newMessages;                    /* specifies if there's unread messages on the MSE or not */
    CsrUtf8String      *mseTime;                        /* current time basis and UTC-offset. Null terminated time string */    
    CsrUint16           fullMessageListingSize;         /* number of bytes in the complete message listing */
    CsrUint16           messageListingRetrieved;        /* number of bytes of the message listing received so far */
    CsrUtf8String      *databaseId;                     /* Unique database identifier. Should be allocated a total of 32 bytes + 1 for NULL */
    CsrUtf8String      *folderVersionCounter;           /* Version counter. Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetMessageListingInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the GET request */
    CsrBtMapNewMessage  newMessages;                    /* specifies if there's unread messages on the MSE or not */
    CsrUtf8String      *mseTime;                        /* current time basis and UTC-offset. Null terminated time string */    
    CsrUint16           fullMessageListingSize;         /* number of bytes in the complete message listing */
    CsrUtf8String      *databaseId;                     /* Unique database identifier. Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUtf8String      *folderVersionCounter;           /* Version counter. Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetMessageListingCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *messageHandle;                  /* null terminated message handle string */
    CsrBtMapAttachment  attachment;                     /* bitmask specifying whether to include attachment or not */
    CsrBtMapCharset     charset;                        /* bitmask used to specifying trans-coding of the message */
    CsrBtMapFracReq     fractionRequest;                /* bitmask which fragment of the message to get _if any_ */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetMessageReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetMessageRes;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8          *payload;                         /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetMessageInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the GET request */
    CsrBtMapFracDel     fractionDeliver;                /* bitmask specifying the fragment status of the message retrieved if sent by peer */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8          *payload;                         /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetMessageCfm;


typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *messageHandle;                  /* null terminated message handle string */
    CsrBtMapStatusInd   statusIndicator;                /* specifies which status information to modify */
    CsrBtMapStatusVal   statusValue;                    /* specifies the new value of the status indication in question */
    CsrUtf8String      *extendedData;                   /* Extended data in "0:18;2:486;3:11;" string format; 
                                                            refer unsigned numbers page :
                                                            https://www.bluetooth.com/specifications/assigned-numbers/message-access-profile
                                                         */
} CsrBtMapcSetMessageStatusReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the add operation */
} CsrBtMapcSetMessageStatusCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *folderName;                     /* null terminated string specifying the name of the folder where the message should be pushed */
    CsrUint32           lengthOfObject;                 /* total length of the message to send, NB: if set to zero this informative OBEX header field will not be included in the packet*/
    CsrBtMapTrans       transparent;                    /* specifies if the MSE should keep a copu of the message in the sent folder */
    CsrBtMapRetry       retry;                          /* specifies if the MSE should try to resent if first delivery to the network fails */
    CsrBtMapCharset     charset;                        /* used to specify the format of the content delivered */
    CsrUtf8String      *conversationId;                 /* Conversation ID (optional). Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUtf8String      *messageHandle;                  /* unique handle of the message */
    CsrBtMapAttachment  attachment;                     /* specifies whether to include attachment while forwarding the message */
    CsrBtMapModifyText  modifyText;                     /* specifies whether to prepend (0) or replace (1) the message to be forwarded */
} CsrBtMapcPushMessageReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint16           maxAllowedPayloadSize;          /* the maximum allowed payload size to include in the CsrBtMapcPushMessageRes */
} CsrBtMapcPushMessageInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             finalFlag;                      /* set this to TRUE if this response carries the final part of the message */
    CsrUint8            *payload;                       /* payload to send */
    CsrUint16           payloadLength;                  /* length of the payload to send */
} CsrBtMapcPushMessageRes;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the push operation */
    CsrUtf8String      *messageHandle;                  /* null terminated string specifying message handle assigned by the MSE */
} CsrBtMapcPushMessageCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
} CsrBtMapcUpdateInboxReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the update inbox request */
} CsrBtMapcUpdateInboxCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
} CsrBtMapcAbortReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
} CsrBtMapcAbortCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             enableNotifications;            /* specifies whether to enable or disable notifications from the MSE */
} CsrBtMapcNotificationRegistrationReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the notification register request */
} CsrBtMapcNotificationRegistrationCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
} CsrBtMapcNotificationRegistrationOffInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;
    CsrUint8            finalFlag;                      /* if TRUE this is the complete obex packet from the peer, if FALSE the rest of the packet can be retrieved by sending a OBEX_CONTINUE_RESPONSE_CODE in CsrBtMapcSyncCommandRes */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcEventNotificationInd;

/* since this response is directed to manager task instanceId is req 
   to identify the right instance to which this response belongs to.
   TBD: Remove comment post review.
    */
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            response;                       /* OBEX response to the sync command */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcEventNotificationRes;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint16           descriptionOffset;              /* payload relative offset to where the description part starts. NB: Only valid if descriptionLength > 0 */
    CsrUint16           descriptionLength;              /* length of the description carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcEventNotificationAbortInd;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         appHandle;                      /* application handle */
    CsrUint16           secLevel;                       /* outgoing security level. The supported values can be found in csr_bt_profiles.h*/
} CsrBtMapcSecurityOutReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrBtResultCode     resultCode;                     /* Result code of the operation. Possible values depend on 
                                                           the value of resultSupplier. If the resultSupplier is 
                                                           CSR_BT_SUPPLIER_IRDA_OBEX,  then the possible result codes 
                                                           can be found in csr_bt_obex.h. There could be other result 
                                                           suppliers also whose error code could be supplied as part 
                                                           of the csr_bt_xxx_prim.h file corresponding to each supplier. 
                                                           All values which are currently not specified in the respective 
                                                           prim.h file are regarded as reserved and the application should 
                                                           consider them as errors. */
    CsrBtSupplier       resultSupplier;                 /* Specifies the supplier of the result given in resultCode. 
                                                           Possible values can be found in csr_bt_result.h. */
} CsrBtMapcSecurityOutCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         appHandle;                      /* application handle */
    CsrUint16           secLevel;                       /* ingoing security level, The supported values can be found in csr_bt_profiles.h*/
} CsrBtMapcSecurityInReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrBtResultCode     resultCode;                     /* Result code of the operation. Possible values depend on 
                                                           the value of resultSupplier. If the resultSupplier is 
                                                           CSR_BT_SUPPLIER_IRDA_OBEX,  then the possible result codes 
                                                           can be found in csr_bt_obex.h. There could be other result 
                                                           suppliers also whose error code could be supplied as part 
                                                           of the csr_bt_xxx_prim.h file corresponding to each supplier. 
                                                           All values which are currently not specified in the respective 
                                                           prim.h file are regarded as reserved and the application should 
                                                           consider them as errors. */
    CsrBtSupplier       resultSupplier;                 /* Specifies the supplier of the result given in resultCode. 
                                                           Possible values can be found in csr_bt_result.h. */
} CsrBtMapcSecurityInCfm;

/*** MAPC Internal Qid registration ***/
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         mapcInstanceId;                 /* application handle */
} CsrBtMapcRegisterQidReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    /* remove after serialization */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetMasInstanceInformationReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the GET request */
    CsrUtf8String      *ownerUci;                       /* Null terminated UCI string of the MAS-instance for which 
                                                           the information is requested.     Example: "lync:x.why@whatever.com" */
    CsrUint16           bodyOffset;                     /* payload relative offset to where the body part starts. NB: Only valid if bodyLength > 0 */
    CsrUint16           bodyLength;                     /* length of the object body carried with this payload */
    CsrUint8           *payload;                        /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                  /* total length of the payload */
} CsrBtMapcGetMasInstanceInformationCfm;


/**********************************************************************/
typedef struct
{
    CsrBtMapcPrim               type;                           /* primitive type */
    CsrUint16                   maxListCount;                   /* maximum number of messages in the listing */
    CsrUint16                   listStartOffset;                /* offset of where to start the listing */
    CsrUtf8String              *filterLastActivityBegin;        /* "YYYYMMDDTHHMMSS+-HHMM" if remote supports UTC. "YYYYMMDDTHHMMSS" otherwise */
    CsrUtf8String              *filterLastActivityEnd;          /* "YYYYMMDDTHHMMSS+-HHMM" if remote supports UTC. "YYYYMMDDTHHMMSS" otherwise */     
    CsrBtMapReadStatus          filterReadStatus;               /* bitmask specifying if filtering should be done on behalf of the read status */
    CsrUtf8String              *filterRecipient;                /* null terminated recipient string */
    CsrUtf8String              *conversationId;                 /* Conversation ID (optional). Should be allocated a total of 32 bytes + 1 for NUL */
    CsrBtMapConvParams          convParameterMask;              /* Conversation parameter mask */
    CsrBool                     srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                                   only if peer device supports SRM. */
} CsrBtMapcGetConversationListingReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBool             srmpOn;                         /* Temporarily suspends the SRM, if TRUE. It is applicable 
                                                           only if peer device supports SRM. */
} CsrBtMapcGetConversationListingRes;

typedef struct
{
    CsrBtMapcPrim       type;                            /* primitive type */
    CsrSchedQid         instanceId;                      /* id of the MAPC instance that generated this event */
    CsrUtf8String      *convListingVersionCounter;       /* conversation listing version counter */
    CsrUint16           numberOfConversations;           /* listingSize - number of conversations in the Conversation-Listing. 
                                                            Note: If maxListCount is zero then this would be valid. 
                                                                  But there will not be any body */
    CsrUtf8String      *databaseId;                      /* Unique database identifier. Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUtf8String      *mseTime;                         /* current time basis and UTC-offset. Null terminated time string */
    CsrUint16           listingOffset;                   /* payload relative offset to where the Conversation Listing object part starts. 
                                                            NB: Only valid if bodyLength > 0 */
    CsrUint16           listingLength;                   /* length of the Conversation Listing object carried in the payload */
    CsrUint8           *payload;                         /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                   /* total length of the payload */
} CsrBtMapcGetConversationListingInd;

typedef struct
{
    CsrBtMapcPrim       type;                            /* primitive type */
    CsrSchedQid         instanceId;                      /* id of the MAPC instance that generated this event */
    CsrUint8            result;                          /* OBEX result of the GET request */
    CsrUtf8String      *convListingVersionCounter;       /* conversation listing version counter */
    CsrUint16           numberOfConversations;           /* listingSize - number of conversations in the Conversation-Listing. 
                                                            Note: If maxListCount is zero then this would be valid. 
                                                                  But there will not be any body
                                                          */
    CsrUtf8String      *databaseId;                      /* Unique database identifier. Should be allocated a total of 32 bytes + 1 for NUL */
    CsrUtf8String      *mseTime;                         /* current time basis and UTC-offset. Null terminated time string */
    CsrUint16           listingOffset;                   /* payload relative offset to where the Conversation Listing object part starts. 
                                                            NB: Only valid if bodyLength > 0 */
    CsrUint16           listingLength;                   /* length of the Conversation Listing object carried in the payload */
    CsrUint8           *payload;                         /* pointer to the complete OBEX payload received from the server */
    CsrUint16           payloadLength;                   /* total length of the payload */
} CsrBtMapcGetConversationListingCfm;

/* All of these may be optionaly included */
typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrBtMapPresence    presenceAvailability;           /* MAP Presence availability state */
    CsrUtf8String      *presenceText;                   /* a description of the current status of the user */
    CsrUtf8String      *lastActivity;                   /* the date time of the owner's last activity.
                                                           "YYYYMMDDTHHMMSS+-HHMM" if remote supports UTC.
                                                           "YYYYMMDDTHHMMSS" otherwise */
    CsrBtMapChatState   chatState;                      /* the chat state of the owner */
    CsrUtf8String      *conversationId;                 /* Conversation ID (optional). Should be allocated a total of 32 bytes + 1 for NUL */
} CsrBtMapcSetOwnerStatusReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the set owner status */
} CsrBtMapcSetOwnerStatusCfm;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrUtf8String      *conversationId;                 /* Conversation ID (optional). Should be allocated a total of 32 bytes + 1 for NUL */
} CsrBtMapcGetOwnerStatusReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the set owner status */
    CsrBtMapPresence    presenceAvailability;           /* Contains the presence availability of the owner. Refer presenceAvailability definition for more details.*/
    CsrUtf8String      *presenceText;                   /* Contains the presence text of the owner. NOTE  The Application should ensure that this does not exceed 255 octets.*/
    CsrUtf8String      *lastActivity;                   /* Indicates the datetime of the owner’s last activity. */
    CsrBtMapChatState   chatState;                      /* Contains the chat state of the owner. Refer chatState definition for more details. */
} CsrBtMapcGetOwnerStatusCfm;

typedef struct
{
    CsrBtMapcPrim                   type;               /* primitive type */
    CsrBtMapNotificationFilterMask  notiFilterMask;     /* Indicates which notifications the applications want from the MSE when notifications are turned “on” or “off”. */
} CsrBtMapcSetNotificationFilterReq;

typedef struct
{
    CsrBtMapcPrim       type;                           /* primitive type */
    CsrSchedQid         instanceId;                     /* id of the MAPC instance that generated this event */
    CsrUint8            result;                         /* OBEX result of the set owner status */
} CsrBtMapcSetNotificationFilterCfm;


#ifdef __cplusplus
}
#endif

#endif

