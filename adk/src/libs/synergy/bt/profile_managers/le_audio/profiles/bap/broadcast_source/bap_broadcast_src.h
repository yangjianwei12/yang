/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Source interface.
 */

/**
 * \defgroup BAP_BROADCAST_SOURCE BAP
 * @{
 */
#ifndef BAP_BROADCAST_SOURCE_H_
#define BAP_BROADCAST_SOURCE_H_

#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "bap_client_list_util_private.h"
#include "bap_client_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE

typedef uint8 bapBigState;
#define BAP_BIG_STATE_IDLE                  (0x00)
#define BAP_BIG_STATE_CONFIGURING           (0x01)
#define BAP_BIG_STATE_CONFIGURED            (0x02)
#define BAP_BIG_STATE_STREAMING             (0x03)
#define BAP_BIG_STATE_INVALID               (0xFF)

#define BAP_BIG_RECONFIG_OPERATION          (0x01)
#define BAP_BIG_METADATA_OPERATION          (0x02)

/* UUID for Broadcast Source */
#define BAP_BROADCAT_AUDIO_ANNOUNCEMENT_UUID                   (0x1852)
#define BAP_BASIC_AUDIO_ANNOUNCEMENT_UUID                      (0x1851)
#define BAP_PUBLIC_BROADCAST_ANNOUNCEMENT_UUID                 (0x1856)
#define BAP_TELEPHONY_MEDIA_AUDIO_BROADCAST_ANNOUNCEMENT_UUID  (0x1855)
#define BAP_GAMING_AUDIO_BROADCAST_ANNOUNCEMENT_UUID           (0x1858)

#define MAX_BIS_INDICES                     6

#define BAP_INVALID_BROADCAST_ID            (0)
#define BAP_BROADCAST_ID_SIZE               (0x03)
#define MAX_BAP_SRC_NAME_LENGTH             0x20
#define MAX_SERVICE_DATA_LENGTH             0x40
#define BAP_BROADCAST_ID_DONT_CARE          ((uint32)0xFFFFFFFFu)

#define BAP_SQ_PUBLIC_BROADCAST     (1 << 1)
#define BAP_HQ_PUBLIC_BROADCAST     (1 << 2)
#define BAP_ENCRYPTION_ENABLED      (1 << 0)

typedef struct
{
    BapClientListElement       listElement;
    phandle_t                  appPhandle;
    BapProfileHandle           profilePhandle; /* assigned by BAP Source */
    uint8                      bigId;
    uint8                      advHandle;
    uint16                     generalSid; /* advertising SID value that can be used by  multiple advertising sets*/
    uint8                      ownAddrType;
    bool                       encryption;
    uint8                      *broadcastCode;
    bapBigState                bigState;
    uint32                     broadcastId;
    BapBroadcastInfo           *broadcastInfo;
    /* Level 1 Group */
    uint32                     presentationDelay;
    uint8                      numSubgroup;
    /* Level 2 Sub Group */
    BapBigSubgroup             *subgroupInfo;
    uint8                      pendingOperation;
    uint8                      bigNameLen;
    char*                      bigName;
    bool                       periodicAdvDataFragmented;
} BapBigStream;

typedef struct BroadcastSrc
{
    uint8     advSid;
    uint8     advHandle;
    uint32    broadcastId;
    uint8     bigNameLen;
    char*     bigName;

    /* Level 1 */
    uint8     numSubgroup;
    /* Level 2 Sub Group and Level 3 */
    BapBigSubgroup  *subgroupInfo;

    struct BroadcastSrc *next;
}BroadcastSrc;

typedef struct BroadcastSrcList
{
    uint8         numBroadcastSources;
    TYPED_BD_ADDR_T sourceAddress;
    BroadcastSrc    *sources;
}BroadcastSrcList;

BapProfileHandle bapBroadcastSrcGetHandle(void);

void bapBroadcastSrcResetHandle(BapProfileHandle profileHandle);

BapBigStream *bapBigStreamNew(struct BAP * const bap,
                              phandle_t phandle,
                              BapProfileHandle handle);

void bapBigStreamDelete(struct BAP * const bap, BapProfileHandle handle);

Bool bapBroadcastSrcFindStreamByHandle(BAP * const bap,
                                       BapProfileHandle handle,
                                       BapBigStream ** const big_stream);

void bapBroadcastSrcStreamConfigureReqHandler(BAP * const bap,
                                              BapInternalBroadcastSrcConfigureStreamReq * const req);

void bapBroadcastSrcStreamEnableReqHandler(BAP * const bap,
                                           BapInternalBroadcastSrcEnableStreamReq * const req);

void bapBroadcastSrcStreamEnableTestReqHandler(BAP * const bap, 
                                               BapInternalBroadcastSrcEnableStreamTestReq * const req);

void bapBroadcastSrcStreamDisableReqHandler(BAP * const bap, 
                                            BapInternalBroadcastSrcDisableStreamReq * const req);

void bapBroadcastSrcStreamReleaseReqHandler(BAP * const bap,
                                            BapInternalBroadcastSrcReleaseStreamReq * const req);

void bapBroadcastSrcStreamMetadataReqHandler(BAP * const bap,
                                             BapInternalBroadcastSrcUpdateMetadataStreamReq * const req);

void bapBroadcastSrcStreamCmPrimitive(BAP * const bap, CsrBtCmPrim * const primitive);

Bool bapBroadcastSrcGetSrcList(BAP * const bap,
                               uint16 filterContext,
                               BroadcastSrcList * broadcastSrcs);

Bool bapBroadcastGetCodefromSrc(BAP * const bap,
                                uint8 advHandle,
                                uint8 *broadcastCode);

Bool bapBroadcastSrcFindStreamByState(BAP* const bap,
                                      bapBigState bigState,
                                      BapBigStream** const bigStream);

Bool bapBroadcastSrcFindStreamByBisHandle(BAP * const bap,
                                      uint16 bisHandle,
                                      BapBigStream ** const bigStream);

void bapBroadcastSrcSetBroadcastId(BAP * const bap, BapProfileHandle handle, uint32 broadcastId);


#endif /* INSTALL_LEA_BROADCAST_SOURCE */
#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_SOURCE_H_ */

/**@}*/


