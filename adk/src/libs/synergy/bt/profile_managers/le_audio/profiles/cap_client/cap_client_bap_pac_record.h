/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef CAP_CLIENT_BAP_RECORD_H
#define CAP_CLIENT_BAP_RECORD_H

#include "cap_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_MAX_STD_SINK_PACS_RECORD   2
#define CAP_MAX_STD_SOURCE_PACS_RECORD 2

#define CAP_CLIENT_JS_LOC_MAX          (0x02)

#define CAP_MAX_LC3_EPC_RECORDS 2 /* includes both Source and Sink Records */

#define CAP_TOTAL_PACS_RECORDS  CAP_MAX_STD_SINK_PACS_RECORD + \
                                           CAP_MAX_STD_SOURCE_PACS_RECORD + \
                                                           CAP_MAX_LC3_EPC_RECORDS

typedef struct {
    uint32 cid;
    uint32 location;
}CapClientAudioLocationInfo;

typedef struct {
    CapClientSreamCapability capability;
    CapClientContext         context; /* preferred audio context */
    uint16                   minOctetsPerCodecFrame;
    uint16                   maxOctetsPerCodecFrame;
    uint8                    codecVersionNum;
    CapClientAudioChannelCount channelCount;
    uint8                    supportedMaxCodecFramesPerSdu;
    CapClientFrameDuration   frameDuaration;
    uint8*                   metadata;
    uint8                    metadataLen;
}CapClientLocalStreamCapabilities;

void capClientInterpretPacRecord(CapClientGroupInstance *cap, BapPacRecord **record, uint8 numOfRecords,
                                 uint8 recordtype);

void capClientCacheRemoteAudioLocations(uint32 location, uint8 recordType, uint32 cid);

void capClientCacheRemoteSupportedContext(uint32 context);

uint32 capClientGetRemoteSupportedContext(void);

CapClientLocalStreamCapabilities* capClientGetRemotePacRecord(CapClientGroupInstance *cap, uint8* count, bool isSink, CapClientContext useCase);

uint8 capClientGetChannelCountForContext(CapClientGroupInstance *cap, CapClientContext useCase, bool sink);

uint8 capClientIsChannelCountSupported(uint8 channelCount, uint8 supportedCount);

uint8 capClientGetPacRecordCount(CapClientGroupInstance *cap);

uint32 capClientGetRemoteAudioLocationsForCid(uint32 cid, bool isSink);

CapClientAudioLocationInfo* capClientGetRemoteAudioLocationsInfo(uint8 *count, bool isSink);

bool capClientIsRecordPresent(CapClientGroupInstance *cap, bool isSink);

void capClientFlushPacRecordsFromList(CapClientGroupInstance *cap, uint8 recordtype);

void capClientResetCachedAudioLoc(uint32 cid);

#define CAP_CLIENT_LIST_ADD_RECORD(listPtr)  \
           ((CapClientBapPacRecord *)CsrCmnListElementAddLast(listPtr, sizeof(CapClientBapPacRecord)))

#define CAP_CLIENT_LIST_REMOVE_RECORD(listPtr,elem)  \
           (CsrCmnListElementRemove(listPtr, (CsrCmnListElm_t*)elem))

#endif /* INSTALL_LEA_UNICAST_CLIENT */
#endif

