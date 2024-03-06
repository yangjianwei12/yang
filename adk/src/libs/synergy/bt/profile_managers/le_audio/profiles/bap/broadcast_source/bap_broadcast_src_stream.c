/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Braodcast Srource stream interface implementation.
 */

/**
 * \defgroup BAP_BROADCAT_SRC_STREAM BAP
 * @{
 */

#ifndef BAP_BROADCAT_SRC_STREAM_H_
#define BAP_BROADCAT_SRC_STREAM_H_

#include <stdio.h>
#include "bap_client_lib.h"
#include "bap_client_list_container_cast.h"
#include "tbdaddr.h"
#include "bap_broadcast_src.h"
#include "bap_broadcast_src_utils.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_tasks.h"
#include "csr_random.h"
#include "../bap_client_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE
#define MAX_PERIODIC_ADV_DATA_BYTES       (CM_PERIODIC_ADV_DATA_LENGTH_MAX * 2) /* Maximum periodic adv bytes that host can send. This can hold upto 12 BISes.
                                                                                   If required it can be increased in the multiple of CM_PERIODIC_ADV_DATA_LENGTH_MAX */
#define MAX_METADATA_LEN                  64

#define BROADCAST_AUDIO_ANNOUNCEMENT_UUID_SIZE            0x03

#define BAP_CLIENT_LTV_LENGTH_OFFSET               0x00
#define BAP_CLIENT_LTV_TYPE_OFFSET                 0x01
#define BAP_CLIENT_LTV_VALUE_OFFSET                0x02

#define BAP_ADV_SET_REGISTERED                     ((uint8)0x01)
#define BAP_INAVALID_ADV_HANDLE                    0xFF

/* Based on the number of BIG supported in controller */
#define BAP_MAX_SUPPORTED_SOURCE                   0x4

BapBroadcastSrcAdvParams BapSrcAdvPaParams = 
{
    0, /* advEventProperties  */
    CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN, /* advIntervalMin */
    CSR_BT_LE_DEFAULT_ADV_INTERVAL_MIN, /* advIntervalMax */
    BAP_LE_1M_PHY, /* primaryAdvPhy */
    HCI_ULP_ADVERT_CHANNEL_DEFAULT, /* primaryAdvChannelMap */
    0, /* secondaryAdvMaxSkip  */
    BAP_LE_1M_PHY, /* secondaryAdvPhy */
    CM_EXT_ADV_SID_ASSIGNED_BY_STACK, /* advSid  */
    0x320, /* periodicAdvIntervalMin 1 sec */
    0x640, /* periodicAdvIntervalMin 2 sec */
    0x7f   /* Host has no preference */
};

uint8 BapSourceHandle[BAP_MAX_SUPPORTED_SOURCE]= {0};

bool BapLtvUtilitiesFindLtvOffset(uint8 * ltvData, uint8 ltvDataLength,
       uint8 type, uint8 * offset)
{
    uint8 ltvIndex = 0;

    *offset = 0;

    if (ltvData == NULL || ltvDataLength == 0)
    {
        return FALSE;
    }

    while(ltvIndex < ltvDataLength && ltvData[ltvIndex + BAP_CLIENT_LTV_LENGTH_OFFSET])
    {
        uint8 length = ltvData[ltvIndex + BAP_CLIENT_LTV_LENGTH_OFFSET];

        if(ltvData[ltvIndex + BAP_CLIENT_LTV_TYPE_OFFSET] == type)
        {
            *offset = ltvIndex;
            return TRUE;
        }
        else
        {
            ltvIndex += (1 + length);
        }
    }

    return FALSE;
}


static uint8* bapGetMetadata(BapBigStream *bigStream, uint16 metadataLen, bool onlyProgramInfo)
{
    uint8 *metadata = CsrPmemZalloc(metadataLen * sizeof(uint8));
    uint8 i,j =0;

    if(metadata == NULL)
        return NULL;

    for (i =0 ; i < bigStream->numSubgroup; i++)
    {
        if(bigStream->subgroupInfo[i].metadata.metadata)
        {
            uint8 offset;

            if (BapLtvUtilitiesFindLtvOffset(
                     bigStream->subgroupInfo[i].metadata.metadata,
                     (uint8)bigStream->subgroupInfo[i].metadata.metadataLen,
                     BAP_CLIENT_METADATA_LTV_TYPE_PROGRAM_INFO,
                     &offset))
             {

                 CsrMemCpy(&(metadata[j]),
                           &(bigStream->subgroupInfo[i].metadata.metadata[offset]),
                           bigStream->subgroupInfo[i].metadata.metadata[offset]+1);

                 j += bigStream->subgroupInfo[i].metadata.metadata[offset]+1;
             }

             if(!onlyProgramInfo)
             {
                 if(BapLtvUtilitiesFindLtvOffset(
                         bigStream->subgroupInfo[i].metadata.metadata,
                         (uint8)bigStream->subgroupInfo[i].metadata.metadataLen,
                         BAP_CLIENT_METADATA_LTV_TYPE_LANGUAGE,
                         &offset))
                 {
                     CsrMemCpy(&(metadata[j]),
                         &(bigStream->subgroupInfo[i].metadata.metadata[offset]),
                         bigStream->subgroupInfo[i].metadata.metadata[offset]+1);

                     j += bigStream->subgroupInfo[i].metadata.metadata[offset]+1;
                 }

                 if(BapLtvUtilitiesFindLtvOffset(
                         bigStream->subgroupInfo[i].metadata.metadata,
                         (uint8)bigStream->subgroupInfo[i].metadata.metadataLen,
                         BAP_CLIENT_METADATA_LTV_TYPE_PARENTAL_RATING,
                         &offset))
                 {
                      CsrMemCpy(&(metadata[j]),
                          &(bigStream->subgroupInfo[i].metadata.metadata[offset]),
                          bigStream->subgroupInfo[i].metadata.metadata[offset]+1);

                      j += bigStream->subgroupInfo[i].metadata.metadata[offset]+1;
                 }
            }
        }
    }

    if (metadataLen == j)
    {
        /* there is match of metadataLen calculated and copied */
        return metadata;
    }
    else
        CsrPmemFree(metadata);

    return NULL;
}

static uint8 bapGetMetadataLength(uint8 *metadata,
                                              uint16 metadataLen,
                                              bool onlyProgramInfo)
{
    uint8 length, offset;

    (void)BapLtvUtilitiesFindLtvOffset(metadata,
                           (uint8)metadataLen,
                            BAP_CLIENT_METADATA_LTV_TYPE_PROGRAM_INFO,
                            &offset);

    length = metadata[offset] + 1 /* length field */;

    if(!onlyProgramInfo)
    {
        if (BapLtvUtilitiesFindLtvOffset(metadata,
                                (uint8)metadataLen,
                                BAP_CLIENT_METADATA_LTV_TYPE_LANGUAGE,
                                &offset))
        {
            length += metadata[offset] + 1 /* length field */;
        }

        if (BapLtvUtilitiesFindLtvOffset(metadata,
                                (uint8)metadataLen,
                                BAP_CLIENT_METADATA_LTV_TYPE_PARENTAL_RATING,
                                &offset))
        {
            length += metadata[offset] + 1 /* length field */;
        }
    }

    return length;
}

BapProfileHandle bapBroadcastSrcGetHandle(void)
{
    uint8 i = 0;
    BapProfileHandle profileHandle = 0;

    for (i = 0; i<BAP_MAX_SUPPORTED_SOURCE; i++)
    {
        if(BapSourceHandle[i] == 0)
        {
            BapSourceHandle[i] = 1;
            profileHandle = i+1;
            break;
        }
    }
    return profileHandle;
}

void bapBroadcastSrcResetHandle(BapProfileHandle profileHandle)
{
    if((profileHandle > 0) && (profileHandle <= BAP_MAX_SUPPORTED_SOURCE))
        BapSourceHandle[profileHandle -1] = 0;
}


BapBigStream *bapBigStreamNew(struct BAP * const bap,
                              phandle_t phandle,
                              BapProfileHandle handle)
{
    BapBigStream *bigStream;
    bigStream = CsrPmemZalloc(sizeof(BapBigStream));

    if (bigStream != NULL)
    {
        bapClientListElementInitialise(&bigStream->listElement);

        bigStream->appPhandle = phandle;
        bigStream->profilePhandle = handle;
        bigStream->bigId = 0;
        bigStream->advHandle = BAP_INAVALID_ADV_HANDLE;
        bigStream->generalSid = 0;
        bigStream->ownAddrType = CSR_BT_ADDR_PUBLIC;
        bigStream->bigState = BAP_BIG_STATE_IDLE;
        bigStream->presentationDelay = 0;
        bigStream->numSubgroup = 0;
        bigStream->subgroupInfo = NULL;
        bigStream->pendingOperation = 0;
        bigStream->encryption = FALSE;
        bigStream->broadcastCode = NULL;
        bigStream->bigNameLen = 0;
        bigStream->bigName = NULL;
        bigStream->broadcastInfo = NULL;
        bigStream->broadcastId = 0;
    }

    CSR_UNUSED(bap);
    return bigStream;
}

void bapBigStreamDelete(struct BAP * const bap, BapProfileHandle handle)
{
    BapClientListElement *listElement;
    BapBigStream *bigStream = NULL;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
        bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

        if (bigStream->profilePhandle == handle)
        {
            uint8 i = 0;

            if((bigStream->encryption == TRUE) && (bigStream->broadcastCode))
            {
                CsrPmemFree(bigStream->broadcastCode);
            }

            bigStream->presentationDelay = 0;

            if(bigStream->subgroupInfo)
            {
                for( i= 0; i < bigStream->numSubgroup; i++)
                {

                    if(bigStream->subgroupInfo[i].metadata.metadata)
                        CsrPmemFree(bigStream->subgroupInfo[i].metadata.metadata);

                    if(bigStream->subgroupInfo[i].bisInfo)
                        CsrPmemFree(bigStream->subgroupInfo[i].bisInfo);

                }
                CsrPmemFree(bigStream->subgroupInfo);
                bigStream->subgroupInfo = NULL;
            }

            if (bigStream->bigNameLen && bigStream->bigName)
            {
                CsrPmemFree(bigStream->bigName);
                bigStream->bigNameLen = 0;
                bigStream->bigName = NULL;
            }

            if (bigStream->broadcastInfo)
            {
                CsrPmemFree(bigStream->broadcastInfo);
                bigStream->broadcastInfo = NULL;
            }

            bapClientListRemove(&bap->bigStreamList, listElement);
            CsrPmemFree(bigStream);
            bigStream = NULL;
            return;
         }
    }
}

Bool bapBroadcastSrcFindStreamByHandle(BAP * const bap,
                                      BapProfileHandle handle,
                                      BapBigStream ** const bigStream)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

         if ((*bigStream)->profilePhandle == handle)
         {
             return TRUE;
         }
    }
    return FALSE;
}


static Bool bapBroadcastSrcFindStreamByBigId(BAP * const bap,
                                             uint8 bigId,
                                             BapBigStream ** const bigStream)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

         if ((*bigStream)->bigId == bigId)
         {
             return TRUE;
         }
    }
    return FALSE;
}

Bool bapBroadcastSrcFindStreamByState(BAP * const bap,
                                      bapBigState bigState,
                                      BapBigStream ** const bigStream)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);
         if (((*bigStream)->bigState == bigState) && 
                ((*bigStream)->advHandle == BAP_INAVALID_ADV_HANDLE))
         {
             return TRUE;
         }
    }
    return FALSE;
}

static Bool bapBroadcastSrcFindStreamByAdvHandle(BAP * const bap,
                                             uint8 advHandle,
                                             BapBigStream ** const bigStream)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

         if ((*bigStream)->advHandle == advHandle)
         {
             return TRUE;
         }
    }
    return FALSE;
}

Bool bapBroadcastSrcFindStreamByBisHandle(BAP * const bap,
                                      uint16 bisHandle,
                                      BapBigStream ** const bigStream)
{
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         *bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);
         if ((*bigStream)->bigState == BAP_BIG_STATE_STREAMING)
         {
            uint8 i = 0;
            uint8 j = 0;
            for(i = 0; i<(*bigStream)->numSubgroup; i++)
            {
                for(j = 0; j<(*bigStream)->subgroupInfo[i].numBis; j++)
                {
                    if((*bigStream)->subgroupInfo[i].bisInfo[j].bisHandle == bisHandle)
                        return TRUE;
                }
            }
         }
    }
    return FALSE;
}

static void bapBroadcastSrcStreamExtAdvertSetParams(BapBigStream *bigStream)
{
    uint8 ownAddrType = bigStream->ownAddrType;
    TYPED_BD_ADDR_T peerAddr = {0, {0,0,0}};
    uint8 advFilterPolicy = HCI_ULP_ADV_FP_ALLOW_ANY;
    uint32 reserved = 0;

    CmExtAdvSetParamsV2ReqSend(CSR_BT_BAP_IFACEQUEUE,
                             bigStream->advHandle,
                             BapSrcAdvPaParams.advEventProperties,
                             BapSrcAdvPaParams.advIntervalMin,
                             BapSrcAdvPaParams.advIntervalMax,
                             BapSrcAdvPaParams.primaryAdvChannelMap,
                             ownAddrType,
                             peerAddr,
                             advFilterPolicy,
                             BapSrcAdvPaParams.primaryAdvPhy,
                             BapSrcAdvPaParams.secondaryAdvMaxSkip,
                             BapSrcAdvPaParams.secondaryAdvPhy,
                             BapSrcAdvPaParams.advSid,
                             BapSrcAdvPaParams.advertisingTransmitPower,
                             reserved,
                             reserved,
                             reserved);
}

static uint8* bapBroadcastSrcGenerateBroadcastId(BapBigStream* bigStream)
{
    void*   seed;
    uint32  num;
    uint8   *brcstId;

    brcstId = (uint8*)CsrPmemZalloc(BAP_BROADCAST_ID_SIZE);

    if( bigStream->broadcastId )
    {
        brcstId[0] = (uint8)bigStream->broadcastId;
        brcstId[1] = (uint8)(bigStream->broadcastId >> 8);
        brcstId[2] = (uint8)(bigStream->broadcastId >> 16);

        return brcstId;
    }

    /* generate seed*/
    seed = CsrRandomSeed();

    /* generate 32 bit uniform random number */
    num = CsrRandom(seed);

    /* Save the broadacstId in broadcast Stream*/
    bigStream->broadcastId = (uint32)(0x00FFFFFF & num);

    /* Get 24 bit Broadcast ID from 32 bit random number
    *
    * Here we use lower 24 bit of 32 bit random number 
    * as broadcast ID and ignore Most significant byte
    *
    */
    brcstId[0] = (uint8)num;
    brcstId[1] = (uint8)(num >> 8);
    brcstId[2] = (uint8)(num >> 16);

    CsrPmemFree(seed);

    return brcstId;
}

static void bapBroadcastSrcStreamExtAdvertSetData(BapBigStream *bigStream)
{
    uint8 i;
    uint8 position;
    uint8 *metadata;
    uint16 appearanceValue;
    uint8 metadataLen;
    uint8 operation = 3;
        /*
        0x0 = Intermediate fragment of fragmented extended advertising data
        0x1 = First fragment of fragmented extended advertising data
        0x2 = Last fragment of fragmented extended advertising data
        0x3 = Complete extended advertising data
        0x4 = Unchanged data (just update the Advertising DID)
        0x5-0xFF = Reserved for future use*/
    uint8 fragPreference = 0;/* 0x0 = The Controller may fragment all Host advertising data
                                  0x1 = The Controller should not fragment nor minimize fragmentation of Host advertising  */
    uint8 length = 0;
    uint8 *data[CM_EXT_ADV_DATA_BYTES_PTRS_MAX] = {0};
    uint8 *advData = NULL;
    bool_t onlyProgramInfo = FALSE;
    uint8 size = 3 * CM_EXT_ADV_DATA_BLOCK_SIZE * sizeof(uint8);

    advData = CsrPmemZalloc(size);
    if(advData)
    {
        uint8* broadcastId;

        advData[length++] = 0x02;  /* Length */
        advData[length++] = 0x01;  /* ble ad type flags */
        advData[length++] = 0x06;  /* GENERAL_DISCOVERABLE_MODE | SINGLE_MODE */

        /* TBD Do we need Local name in Broadcast Audio announcement */
        if (bigStream->bigNameLen) /* Length */
            advData[length++] = (0x01 + bigStream->bigNameLen);
        else
            advData[length++] = 0x013;

        advData[length++] = 0x30;  /* Broadcast Name */

        if (bigStream->bigNameLen) /* Name */
        {
            for (i = 0; i < bigStream->bigNameLen; i++)
                advData[length++] = bigStream->bigName[i];
        }
        else
        {
            advData[length++] = 0x51;  /* QCOM BROADCAST SRC */
            advData[length++] = 0x43;
            advData[length++] = 0x4f;
            advData[length++] = 0x4d;
            advData[length++] = 0x20;
            advData[length++] = 0x42;
            advData[length++] = 0x52;
            advData[length++] = 0x4f;
            advData[length++] = 0x41;
            advData[length++] = 0x44;
            advData[length++] = 0x43;
            advData[length++] = 0x41;
            advData[length++] = 0x53;
            advData[length++] = 0x54;
            advData[length++] = 0x20;
            advData[length++] = 0x53;
            advData[length++] = 0x52;
            advData[length++] = 0x43;
        }

        advData[length++] = BAP_BROADCAST_ID_SIZE
                   + BROADCAST_AUDIO_ANNOUNCEMENT_UUID_SIZE;  /* Length */

        advData[length++] = 0x16;  /* Service Data - 16-bit UUID */
        advData[length++] = (uint8)(BAP_BROADCAT_AUDIO_ANNOUNCEMENT_UUID & 0xff);
        advData[length++] = (uint8)((BAP_BROADCAT_AUDIO_ANNOUNCEMENT_UUID >> 8) & 0xff);

        /* Generate Broadcast ID */
        broadcastId = bapBroadcastSrcGenerateBroadcastId(bigStream);

        advData[length++] = broadcastId[0];  /* Broadcast_ID */
        advData[length++] = broadcastId[1];
        advData[length++] = broadcastId[2];

        CsrPmemFree(broadcastId);

        if (bigStream->broadcastInfo != NULL )
        {
            advData[length++] = 0x03;  /* Length */
            advData[length++] = 0x19;  /* ble_ad_type_appearance */

            appearanceValue = bigStream->broadcastInfo->appearanceValue?
                              bigStream->broadcastInfo->appearanceValue:
                              CSR_BT_APPEARANCE_UNKNOWN;

            advData[length++] = (uint8) (appearanceValue & 0xff);
            advData[length++] = (uint8) ((appearanceValue >> 8) & 0xff);

            if ((bigStream->broadcastInfo->broadcast & TMAP_BROADCAST) == TMAP_BROADCAST)
            {
                uint16 tmapRole = 0x0015;  /* (TMAP_ROLE_CALL_GATEWAY | TMAP_ROLE_UNICAST_MEDIA_SENDER | TMAP_ROLE_BROADCAST_MEDIA_SENDER) */
                advData[length++] += 5;
    
                advData[length++] = 0x16;  /* Service Data - 16-bit UUID */
    
                advData[length++] = (uint8)( BAP_TELEPHONY_MEDIA_AUDIO_BROADCAST_ANNOUNCEMENT_UUID & 0xff);
                advData[length++] = (uint8)((BAP_TELEPHONY_MEDIA_AUDIO_BROADCAST_ANNOUNCEMENT_UUID >> 8) & 0xff);
    
                advData[length++] = (uint8)(tmapRole & 0xff);
                advData[length++] = (uint8)((tmapRole >> 8) & 0xff);
            }

            if ((bigStream->broadcastInfo->broadcast & GMAP_BROADCAST) == GMAP_BROADCAST)
            {
                uint8 gmapRole = 0x05;  /* (GMAP_ROLE_UNICAST_GAME_GATEWAY| GMAP_ROLE_BROADCAST_GAME_SENDER) */
                advData[length++] += 4;
    
                advData[length++] = 0x16;  /* Service Data - 16-bit UUID */
    
                advData[length++] = (uint8)( BAP_GAMING_AUDIO_BROADCAST_ANNOUNCEMENT_UUID & 0xff);
                advData[length++] = (uint8)((BAP_GAMING_AUDIO_BROADCAST_ANNOUNCEMENT_UUID >> 8) & 0xff);
    
                advData[length++] = gmapRole;
            }

            if (((bigStream->broadcastInfo->broadcast & SQ_PUBLIC_BROADCAST ) == SQ_PUBLIC_BROADCAST) ||
                ((bigStream->broadcastInfo->broadcast & HQ_PUBLIC_BROADCAST ) == HQ_PUBLIC_BROADCAST))
            {
                /* Identify if application wants full metadata or just mandatory 
                 * ProgramInfo LTV to be put in EA
                 */
                onlyProgramInfo = ((bigStream->broadcastInfo->flags & PUBLIC_BROADCAST_METADATA_ALL)
                                  == PUBLIC_BROADCAST_METADATA_ALL) ? FALSE: TRUE;
    
                 /* We will add all metadata(if requested) in the EA from all subgroups */
                 /* For example, lets say SQ Broadcast with 2 subgroups enabled
                    Subgrp 1             Subgrp 2
                    PI: XXX               PI: XXX
                    PR: Y                 PR: Y
                    Language: Eng     Language: Spanish,
                    then we would want to club metadata in EA for both the subgroups as
                    Subgroup1 + Subgroup2 
                  */
                for (i =0 ; i < bigStream->numSubgroup; i++)
                {
                    if (bigStream->subgroupInfo[i].metadata.metadata != NULL)
                    {
                        advData[length] += bapGetMetadataLength(
                                   bigStream->subgroupInfo[i].metadata.metadata,
                                   bigStream->subgroupInfo[i].metadata.metadataLen,
                                   onlyProgramInfo);
                    }
                }
    
                /* Store the length in local variable for deciding advDataX set later */
                metadataLen = advData[length];
    
                /* Store position in case of overflow, we may have to deduct metadataLen*/
                position = length;
    
                /*Increment length for the mandatory PBP annoucments elements */
                advData[length++] += 5;
    
                advData[length++] = 0x16;  /* Service Data - 16-bit UUID */
    
                advData[length++] = (uint8)(BAP_PUBLIC_BROADCAST_ANNOUNCEMENT_UUID & 0xff);
                advData[length++] = (uint8)((BAP_PUBLIC_BROADCAST_ANNOUNCEMENT_UUID >> 8) & 0xff);
    
                /* Single Octet for updating broadcast type and encryption field*/
                if (bigStream->broadcastInfo->broadcast & SQ_PUBLIC_BROADCAST)
                {
                    advData[length] = (uint8)BAP_SQ_PUBLIC_BROADCAST;
                }
                if (bigStream->broadcastInfo->broadcast & HQ_PUBLIC_BROADCAST)
                {
                    advData[length] |= (uint8)BAP_HQ_PUBLIC_BROADCAST;
                }
    
                advData[length++] |= (bigStream->broadcastInfo->flags & BAP_ENCRYPTION_ENABLED )?
                                       (uint8)BAP_ENCRYPTION_ENABLED : 0;
    
    
               if (metadataLen > 0)
               {
                   metadata = bapGetMetadata(bigStream, metadataLen, onlyProgramInfo);
    
                   if (metadata)
                   {
                       advData[length++] = metadataLen;
    
                       if(metadataLen < (size - length))
                       {
                           /* We can accomodate everything in advData */
                           CsrMemCpy(&(advData[length]), metadata, metadataLen);
    
                           length += metadataLen;
                       }
                       else
                       {
                               CsrMemCpy(&(advData[length]), metadata, (size - length));
    
                               length += (size - length);
    
                               /* Adjust the actual metadata length copied in EA*/
                               advData[position] -= (metadataLen - size - length);
                       }
    
                   }
                   else
                   {
                       /* Resize total length as metadata is NULL */
                       advData[position] -= metadataLen;
                       
                       /* Set metdata length as 0 in advert */
                       advData[length++] = 0;
                   }
               }
               else
               {
                  /* Set metdata length as 0 in advert */
                  advData[length++] = 0;
               }
            }
        }
    }

    if (length)
    {
        uint8 index = 0, offset = 0, lenToCpy = 0;

        for(offset = 0, index = 0; offset < length;
                               index++, offset += lenToCpy)
        {
            data[index] = CsrPmemZalloc(CM_EXT_ADV_DATA_BLOCK_SIZE * sizeof(uint8));
            lenToCpy = length - offset;
            if(lenToCpy > CM_EXT_ADV_DATA_BLOCK_SIZE)
                lenToCpy = CM_EXT_ADV_DATA_BLOCK_SIZE;

            SynMemCpyS((data[index]), CM_EXT_ADV_DATA_BLOCK_SIZE, &(advData[offset]), lenToCpy);
        }
    }

    if (advData)
        CsrPmemFree(advData);

    BAP_CLIENT_DEBUG("BAP: bapBroadcastSrcStreamExtAdvertSetData : Adv handle 0x%x : BroadcastID: %d \n", bigStream->advHandle, bigStream->broadcastId);

    CmExtAdvSetDataReqSend(CSR_BT_BAP_IFACEQUEUE,
                           bigStream->advHandle,
                           operation,
                           fragPreference,
                           length,
                           data);
}

#if SCAN_RSP
static void bapBroadcastSrcStreamExtAdvertSetScanRespData(BapBigStream *bigStream)
{
    uint8 operation = 3;
    uint8 fragPreference = 0;
    uint8 dataLength = 35;
    uint8 *data[CM_EXT_ADV_SCAN_RESP_DATA_BYTES_PTRS_MAX] = {0};
    static uint8 scanRspData1[] = { 0x02, 0x01, 0x02, 0x17, 0x09, 0x51, 0x43, 0x4F,
                                       0x4D, 0x20, 0x41, 0x55, 0x52, 0x41, 0x50, 0x4C,
                                       0x55, 0x53, 0x20, 0x4C, 0x45, 0x20, 0x41, 0x55,
                                       0x44, 0x49, 0x4F, 0x03, 0x03, 0x00, 0x18, 0x03 };
    static uint8 scanRspData2[CM_EXT_ADV_SCAN_RESP_DATA_BLOCK_SIZE] = {0x03, 0x01, 0x18};
    data[0] = scanRspData1;
    data[1] = scanRspData2;

    /* TODO scanRspData1 to be filled from big_stream */

    CmExtAdvSetScanRespDataReqSend(bigStream->advHandle,
                                   operation,
                                   fragPreference,
                                   dataLength,
                                   data);
}
#endif

Bool bapBroadcastSrcGetSrcList(BAP * const bap,
                               uint16 filterContext,
                               BroadcastSrcList * broadcastSrcs)
{
    BapClientListElement *listElement;
    BapBigStream *bigStream = NULL;
    uint8 count = 0;
    uint8 i = 0;
    uint8 j = 0;
    uint8 filtSubGrpIndex = 0;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

         if (bigStream->numSubgroup)
         {
             bool_t allocSrc = FALSE;
             BroadcastSrc *newSource = NULL;
             for (i = 0 ; i < bigStream->numSubgroup; i ++)
             {
                 if(bigStream->subgroupInfo[i].metadata.streamingAudioContext == filterContext && !allocSrc)
                 {
                     count++;
                     broadcastSrcs->numBroadcastSources = count;
                     newSource = CsrPmemZalloc(sizeof(BroadcastSrc));
                     allocSrc = TRUE;

                     newSource->next = NULL;

                     newSource->advSid = (uint8)bigStream->generalSid;
                     newSource->advHandle = bigStream->advHandle;
                     newSource->broadcastId = bigStream->broadcastId;

                     if(bigStream->bigNameLen && bigStream->bigName)
                     {
                         newSource->bigNameLen = bigStream->bigNameLen;
                         newSource->bigName = bigStream->bigName;
                     }

                     if( broadcastSrcs->sources == NULL)
                     {
                         broadcastSrcs->sources = newSource;
                     }
                     else
                     {
                         BroadcastSrc *tempSource = broadcastSrcs->sources;
                         while(tempSource->next != NULL)
                         {
                             tempSource = tempSource->next;
                         }
                         tempSource->next = newSource;
                     }
                     for (j = 0 ; j < bigStream->numSubgroup; j++)
                     {
                         if(bigStream->subgroupInfo[j].metadata.streamingAudioContext == filterContext)
                             newSource->numSubgroup += 1;
                     }
                     newSource->subgroupInfo = CsrPmemZalloc(newSource->numSubgroup * sizeof(BapBigSubgroup));
                 }

                 if((bigStream->subgroupInfo[i].metadata.streamingAudioContext == filterContext) && newSource
                     && (filtSubGrpIndex < newSource->numSubgroup))
                 {
                     uint16 metadataLen = bigStream->subgroupInfo[i].metadata.metadataLen;
     
                     CsrMemCpy(&(newSource->subgroupInfo[filtSubGrpIndex].codecId),
                            &(bigStream->subgroupInfo[i].codecId),
                            sizeof(BapCodecId));
     
                     CsrMemCpy(&(newSource->subgroupInfo[filtSubGrpIndex].codecConfigurations),
                            &(bigStream->subgroupInfo[i].codecConfigurations),
                            sizeof(BapCodecConfiguration));
     
                     newSource->subgroupInfo[filtSubGrpIndex].metadata.streamingAudioContext = bigStream->subgroupInfo[i].metadata.streamingAudioContext;
                     newSource->subgroupInfo[filtSubGrpIndex].metadata.metadataLen = metadataLen;
     
                     if (metadataLen != 0)
                     {
                         newSource->subgroupInfo[filtSubGrpIndex].metadata.metadata = CsrPmemAlloc(metadataLen);
     
                         CsrMemCpy(newSource->subgroupInfo[filtSubGrpIndex].metadata.metadata,
                                        bigStream->subgroupInfo[i].metadata.metadata, metadataLen);
                     }
     
                     newSource->subgroupInfo[filtSubGrpIndex].numBis = bigStream->subgroupInfo[i].numBis;
     
                     if(bigStream->subgroupInfo[i].numBis > 0)
                     {
                         newSource->subgroupInfo[filtSubGrpIndex].bisInfo =  CsrPmemZalloc(bigStream->subgroupInfo[i].numBis * sizeof(BapBisInfo));
     
                         for(j = 0; j < bigStream->subgroupInfo[i].numBis; j++)
                         {
                            newSource->subgroupInfo[filtSubGrpIndex].bisInfo[j].bisIndex = bigStream->subgroupInfo[i].bisInfo[j].bisIndex;
                            newSource->subgroupInfo[filtSubGrpIndex].bisInfo[j].bisHandle = bigStream->subgroupInfo[i].bisInfo[j].bisHandle;
                            CsrMemCpy(&(newSource->subgroupInfo[filtSubGrpIndex].bisInfo[j].codecConfigurations),
                                 &(bigStream->subgroupInfo[i].bisInfo[j].codecConfigurations), sizeof(BapCodecConfiguration));
                         }
                     }
                     ++filtSubGrpIndex;
                 }
             }
         }
     }

    if (count)
        return TRUE;
    else
        return FALSE;
}

Bool bapBroadcastGetCodefromSrc(BAP * const bap,
                                uint8 advHandle,
                                uint8 *broadcastCode)
{
    BapClientListElement *listElement;
    BapBigStream *bigStream = NULL;

    for (listElement = bapClientListPeekFront(&bap->bigStreamList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         bigStream = CONTAINER_CAST(listElement, BapBigStream, listElement);

         if (bigStream->advHandle == advHandle)
         {
            if(bigStream->encryption)
            {
                memcpy(broadcastCode, bigStream->broadcastCode, CM_DM_BROADCAST_CODE_SIZE);
                return TRUE;
            }
         }
    }
    return FALSE;
}

static void bapBroadcastSrcStreamPASetData(BapBigStream *bigStream)
{
    uint8 operation = 3;
    uint8 dataLength = 0;
    uint8 *data[CM_PERIODIC_ADV_DATA_BYTES_PTRS_MAX] = {0};
    static uint8 bigInfo[MAX_PERIODIC_ADV_DATA_BYTES] = {0};
    short size = 0;
    uint8 tmpSize = 0;
    uint8 i = 0;
    short j = 0;

    bigInfo[size++] = 0; /* update at the end */
    /* Service Data AD data type */
    bigInfo[size++] = 0x16;  /* 0x16 Service data ad tag */
    bigInfo[size++] = (uint8)(BAP_BASIC_AUDIO_ANNOUNCEMENT_UUID & 0xff);
    bigInfo[size++] = (uint8)((BAP_BASIC_AUDIO_ANNOUNCEMENT_UUID >> 8) & 0xff);

    /* Level 1 : Group */
    /* Presentation Delay */
    bigInfo[size++] = (uint8)(bigStream->presentationDelay & 0xff);
    bigInfo[size++] = (uint8)((bigStream->presentationDelay >> 8) & 0xff);
    bigInfo[size++] = (uint8)((bigStream->presentationDelay >>16) & 0xff);

    /* Num Sungroup */
    bigInfo[size++] = bigStream->numSubgroup;

    for(i = 0; i < bigStream->numSubgroup; i++)
    {
        j = 0;
        /* Level 2 : Sub Group */

        /* Num BIS */
        bigInfo[size++] = bigStream->subgroupInfo[i].numBis;

        /* Codec Id */
        size += bapBroadcastSrcGetCodecId(&bigInfo[size], &bigStream->subgroupInfo[i].codecId);

        /* Codec specific Length */
        bigInfo[size++] = 0;
        /* Codec specific Configurations */
        tmpSize = bapBroadcastSrcGetCodecConfigParam(&bigInfo[size], &bigStream->subgroupInfo[i].codecConfigurations);
        /* update Codec specific length */
        bigInfo[size-1] = tmpSize;
        size += tmpSize;

        /* Metadata length */
        bigInfo[size++] = 0;
        /* Metadata */
        tmpSize = bapBroadcastSrcGetMetadata(&bigInfo[size], &bigStream->subgroupInfo[i].metadata);
        /* update Codec specific length */
        bigInfo[size-1] = tmpSize;
        size += tmpSize;

        /* Level 3: BIS */
        for(j = 0; j < bigStream->subgroupInfo[i].numBis; j++)
        {
            bigInfo[size++] = bigStream->subgroupInfo[i].bisInfo[j].bisIndex;

            /* Codec specific Length */
            bigInfo[size++] = 0;
            /* Codec specific Configurations */
            tmpSize = bapBroadcastSrcGetCodecConfigParam(&bigInfo[size], &bigStream->subgroupInfo[i].bisInfo[j].codecConfigurations);
            /* update Codec specific length */
            bigInfo[size-1] = tmpSize;
            size += tmpSize;
        }

    }
    /* update Service data length */
    bigInfo[0] = size-1;

    BAP_CLIENT_DEBUG("BigInfo data- dataLength %d size %d : \n", dataLength, size);
    j = 0;
    do
    {
        for(i = 0; i < CM_PERIODIC_ADV_DATA_BYTES_PTRS_MAX; i++)
        {
            data[i] = &bigInfo[(i*CM_PERIODIC_ADV_DATA_BLOCK_SIZE) + (j * CM_PERIODIC_ADV_DATA_LENGTH_MAX)];
        }

        if (size > CM_PERIODIC_ADV_DATA_LENGTH_MAX)
        {
            dataLength = CM_PERIODIC_ADV_DATA_LENGTH_MAX;
			size -= CM_PERIODIC_ADV_DATA_LENGTH_MAX;
            if (j == 0x00)
                operation = 0x01;
            else
                operation = 0x00;

            bigStream->periodicAdvDataFragmented = TRUE;
        }
        else
        {
            dataLength = (uint8)size;
            size = 0;
            if (j == 0x00)
                operation = 0x03;
            else
                operation = 0x02;

            bigStream->periodicAdvDataFragmented = FALSE;
        }
        CmPeriodicAdvSetDataReqSend(CSR_BT_BAP_IFACEQUEUE,
                                    bigStream->advHandle,
                                    operation,
                                    dataLength,
                                    data);
        j++;
    }while(size != 0);
}

void bapBroadcastSrcStreamConfigureReqHandler(BAP * const bap,
                                              BapInternalBroadcastSrcConfigureStreamReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;
    uint8 i = 0;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if((bigStream->bigState == BAP_BIG_STATE_IDLE) ||
            (bigStream->bigState == BAP_BIG_STATE_CONFIGURED))
        {
            bigStream->bigId = req->bigId;
            bigStream->ownAddrType = req->ownAddrType;
            bigStream->presentationDelay = req->presentationDelay;

            if (bigStream->numSubgroup)
            {
                for(i = 0; i < bigStream->numSubgroup ; i++)
                {
                    if(bigStream->subgroupInfo[i].metadata.metadataLen && bigStream->subgroupInfo[i].metadata.metadata)
                    {
                        CsrPmemFree(bigStream->subgroupInfo[i].metadata.metadata);
                    }
        
                    CsrPmemFree(bigStream->subgroupInfo[i].bisInfo);
                }
                CsrPmemFree(bigStream->subgroupInfo);
                bigStream->subgroupInfo = NULL;
            }
            if (bigStream->bigNameLen)
            {
                CsrPmemFree(bigStream->bigName);
                bigStream->bigNameLen = 0;
                bigStream->bigName = NULL;
            }

            if (bigStream->broadcastInfo)
            {
                CsrPmemFree(bigStream->broadcastInfo);
                bigStream->broadcastInfo = NULL;
            }

            bigStream->numSubgroup = req->numSubgroup;
            bigStream->subgroupInfo = req->subgroupInfo;

            bigStream->broadcastInfo = req->broadcastInfo;
            bigStream->bigNameLen = req->bigNameLen;
            bigStream->bigName = req->bigName;

            /* call EA and PA update procedure */

            if(bigStream->bigState == BAP_BIG_STATE_IDLE)
            {
                /* Get available advHandle */
                CmExtAdvSetsInfoReqSend(CSR_BT_BAP_IFACEQUEUE);
                bigStream->bigState = BAP_BIG_STATE_CONFIGURING;
            }
            else
            {
                bigStream->pendingOperation = BAP_BIG_RECONFIG_OPERATION;
                bapBroadcastSrcStreamExtAdvertSetData(bigStream);
            }
            return;
        }
        else
        {
            result = BAP_RESULT_INVALID_STATE;
        }
    }
    bapBroadcastSrcConfigureStreamCfmSend(bap->appPhandle, req->handle, result);
}

void bapBroadcastSrcStreamEnableReqHandler(BAP * const bap,
                                           BapInternalBroadcastSrcEnableStreamReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if(bigStream->bigState == BAP_BIG_STATE_CONFIGURED)
        {
            if((bigStream->encryption == TRUE) && (bigStream->broadcastCode))
            {
                /* Free existing broadcastCode if any*/
                CsrPmemFree(bigStream->broadcastCode);
                bigStream->broadcastCode = NULL;
            }

            bigStream->encryption = req->encryption;
            if(req->encryption == TRUE)
            {
                bigStream->broadcastCode = CsrPmemAlloc(CM_DM_BROADCAST_CODE_SIZE);
                memcpy(bigStream->broadcastCode, req->broadcastCode, CM_DM_BROADCAST_CODE_SIZE);
            }
            bigStream->bigState = BAP_BIG_STATE_STREAMING;
            /* Create BIG procedure */
            CmIsocCreateBigReqSend(CSR_BT_BAP_IFACEQUEUE,
                            req->bigId,
                            bigStream->advHandle,
                            (CmBigConfigParam *)(req->bigConfigParameters),
                            req->numBis,
                            req->encryption,
                            req->broadcastCode);
            return;
        }
        else
            result = BAP_RESULT_INVALID_STATE;
    }

    bapBroadcastSrcEnableStreamCfmSend(bap->appPhandle, req->handle, result,
        0, 0, NULL, 0, NULL);
}

void bapBroadcastSrcStreamEnableTestReqHandler(BAP * const bap,
                                               BapInternalBroadcastSrcEnableStreamTestReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if(bigStream->bigState == BAP_BIG_STATE_CONFIGURED)
        {
            bigStream->encryption = req->encryption;
            if(req->encryption == TRUE)
            {
                bigStream->broadcastCode = CsrPmemAlloc(CM_DM_BROADCAST_CODE_SIZE);
                memcpy(bigStream->broadcastCode, req->broadcastCode, CM_DM_BROADCAST_CODE_SIZE);
            }
            bigStream->bigState = BAP_BIG_STATE_STREAMING;
            /* Create BIG procedure */
            CmIsocCreateBigTestReqSend(CSR_BT_BAP_IFACEQUEUE,
                                       req->bigId,
                                       bigStream->advHandle,
                                       (CmBigTestConfigParam *)(req->bigTestConfigParameters),
                                       req->numBis,
                                       req->encryption,
                                       req->broadcastCode);
            return;
        }
        else
            result = BAP_RESULT_INVALID_STATE;
    }

    bapBroadcastSrcEnableStreamTestCfmSend(bap->appPhandle, req->handle, result,
        0, 0, NULL, 0, NULL);
}

void bapBroadcastSrcStreamDisableReqHandler(BAP * const bap,
                                            BapInternalBroadcastSrcDisableStreamReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if(bigStream->bigState == BAP_BIG_STATE_STREAMING)
        {
            bigStream->bigState = BAP_BIG_STATE_CONFIGURED;
            /* BIG Terminate procedure */
            CmIsocTerminateBigReqSend(CSR_BT_BAP_IFACEQUEUE, req->bigId, 0x13 /*User terminated */);
            return;
        }
        else
            result = BAP_RESULT_INVALID_STATE;
    }

    bapBroadcastSrcDisableStreamCfmSend(bap->appPhandle, req->handle, result, req->bigId);
}

void bapBroadcastSrcStreamReleaseReqHandler(BAP * const bap,
                                            BapInternalBroadcastSrcReleaseStreamReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if(bigStream->bigState == BAP_BIG_STATE_CONFIGURED)
        {
            /* Call Terminate PA mode procedure */
            CmPeriodicAdvStopReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle, 1);
            return;
        }
        else
            result = BAP_RESULT_INVALID_STATE;
    }

    bapBroadcastSrcReleaseStreamCfmSend(bap->appPhandle, req->handle, result, req->bigId);
}

void bapBroadcastSrcStreamMetadataReqHandler(BAP * const bap,
                                             BapInternalBroadcastSrcUpdateMetadataStreamReq * const req)
{
    BapBigStream *bigStream = NULL;
    BapResult result = BAP_RESULT_ERROR;
    uint8 i=0;

    if (bapBroadcastSrcFindStreamByHandle(bap, req->handle, &bigStream))
    {
        if(bigStream->bigState == BAP_BIG_STATE_STREAMING)
        {
            bigStream->numSubgroup = req->numSubgroup;

            for(i = 0; i<req->numSubgroup; i++)
            {
                if(bigStream->subgroupInfo[i].metadata.metadata)
                    CsrPmemFree(bigStream->subgroupInfo[i].metadata.metadata);

                memcpy(&bigStream->subgroupInfo[i].metadata, &req->subgroupMetadata[i], sizeof(BapMetadata));

                bigStream->subgroupInfo[i].metadata.metadata = NULL;

                if(req->subgroupMetadata[i].metadataLen && req->subgroupMetadata[i].metadata)
                {
                    bigStream->subgroupInfo[i].metadata.metadataLen= req->subgroupMetadata[i].metadataLen;
                    bigStream->subgroupInfo[i].metadata.metadata = req->subgroupMetadata[i].metadata;
                }
            }

            bapBroadcastSrcStreamPASetData(bigStream);
            return;
        }
        else
            result = BAP_RESULT_INVALID_STATE;
    }

    bapBroadcastSrcUpdateMetadataCfmSend(bap->appPhandle, req->handle, result, bigStream->bigId);
}

void BapBroadcastSrcSetAdvParams(BapProfileHandle handle,
                          const BapBroadcastSrcAdvParams *srcAdvPaParams)
{
    if(handle && srcAdvPaParams)
        memcpy(&BapSrcAdvPaParams, srcAdvPaParams, sizeof(BapBroadcastSrcAdvParams));
}

void BapBroadcastSrcSetBroadcastId(BapProfileHandle handle, uint32 broadcastId)
{
    BapInternalBroadcastSrcSetBroadcastId *pPrim = CsrPmemZalloc(sizeof(BapInternalBroadcastSrcSetBroadcastId));

    pPrim->type = BAP_INTERNAL_BROADCAST_SRC_SET_BROADCAST_ID;
    pPrim->handle = handle;
    pPrim->broadcastId = broadcastId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void bapBroadcastSrcSetBroadcastId(BAP * const bap, BapProfileHandle handle, uint32 broadcastId)
{
    BapBigStream *bigStream = NULL;

    if (bapBroadcastSrcFindStreamByHandle(bap, handle, &bigStream))
    {
        if(broadcastId != BAP_BROADCAST_ID_DONT_CARE)
        {
            bigStream->broadcastId = (uint32)(0x00FFFFFF & broadcastId);
        }
    }
}

void bapBroadcastSrcStreamCmPrimitive(BAP * const bap, CsrBtCmPrim * const primitive)
{
    BapResult result = BAP_RESULT_ERROR;
    BapBigStream *bigStream = NULL;

    BAP_CLIENT_INFO("BAP: bapBroadcastSrcStreamCmPrimitive prim:CsrBtCmPrim 0x%x\n", *primitive);

    switch (*primitive)
    {
        case CSR_BT_CM_EXT_ADV_SETS_INFO_CFM:
        {
            CsrBtCmExtAdvSetsInfoCfm *prim = (CsrBtCmExtAdvSetsInfoCfm *) primitive;

            if((bapBroadcastSrcFindStreamByState(bap, BAP_BIG_STATE_CONFIGURING, &bigStream)))
            {
                uint8 i = 0;

                BAP_CLIENT_DEBUG("CSR_BT_CM_EXT_ADV_SETS_INFO_CFM flags %d NumSet %d\n",prim->flags, prim->numAdvSets);
                /* skipping the index 0 is for legacy */
                for(i = 1; i< prim->numAdvSets; i++)
                {
                    BAP_CLIENT_DEBUG(" advSets.registered %d, advertising %d info %d \n",prim->advSets[i].registered, prim->advSets[i].advertising,
                        prim->advSets[i].info);
                    if(( prim->advSets[i].registered & BAP_ADV_SET_REGISTERED)!= BAP_ADV_SET_REGISTERED)
                    {
                        /* Set next available ADV handle */
                        bigStream->advHandle = i;
                        BAP_CLIENT_DEBUG("bigStream->advHandle %d\n", bigStream->advHandle);
                        break;
                    }
                }
                
                if(bigStream->advHandle != BAP_INAVALID_ADV_HANDLE )
                {
                    result = BAP_RESULT_SUCCESS;
                    CmExtAdvRegisterAppAdvSetReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle, 0);
                }
                else
                {
                    bigStream->bigState = BAP_BIG_STATE_IDLE;
                    result = BAP_RESULT_INSUFFICIENT_RESOURCES;
                }
            }
            
        }
        break;
        case CSR_BT_CM_EXT_ADV_REGISTER_APP_ADV_SET_CFM:
        {
            CmExtAdvRegisterAppAdvSetCfm *prim = (CmExtAdvRegisterAppAdvSetCfm *) primitive;

            if((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                bapBroadcastSrcStreamExtAdvertSetParams(bigStream);
            }
            else if(bigStream && (prim->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                bigStream->bigState = BAP_BIG_STATE_IDLE;
                result = BAP_RESULT_INSUFFICIENT_RESOURCES;
            }
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_PARAMS_CFM:
        {
            CmExtAdvSetParamsCfm *prim = (CmExtAdvSetParamsCfm *) primitive;
            if((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                bigStream->generalSid = prim->advSid;

                if (bigStream->ownAddrType == CSR_BT_ADDR_TYPE_PUBLIC)
                {
                    /* Update the adv sid */
                    bapBroadcastSrcStreamExtAdvertSetData(bigStream);
                }
                else
                {
                    CsrBtDeviceAddr randomAddr = {0};

                    /* Send random address req */
                    CmExtAdvSetRandomAddrReqSend(CSR_BT_BAP_IFACEQUEUE,
                                                 bigStream->advHandle,
                                                 CM_EXT_ADV_ADDRESS_GENERATE_RESOLVABLE,
                                                 randomAddr);
                }
            }
        }
        break;

        case CM_DM_EXT_ADV_SET_PARAMS_V2_CFM:
        {
            CmDmExtAdvSetParamsV2Cfm *prim = (CmDmExtAdvSetParamsV2Cfm *) primitive;

            if((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                bigStream->generalSid = prim->advSid;

                if (bigStream->ownAddrType == CSR_BT_ADDR_TYPE_PUBLIC)
                {
                    /* Update the adv sid */
                    bapBroadcastSrcStreamExtAdvertSetData(bigStream);
                }
                else
                {
                    CsrBtDeviceAddr randomAddr = {0};

                    /* Send random address req */
                    CmExtAdvSetRandomAddrReqSend(CSR_BT_BAP_IFACEQUEUE,
                                                 bigStream->advHandle,
                                                 CM_EXT_ADV_ADDRESS_GENERATE_RESOLVABLE,
                                                 randomAddr);
                }
            }
            else
            {
                BAP_CLIENT_DEBUG(" CM_DM_EXT_ADV_SET_PARAMS_V2_CFM received with status %d \n", prim->resultCode);
                bapBroadcastSrcConfigureStreamCfmSend(bap->appPhandle,
                    bigStream->profilePhandle, BAP_RESULT_INVALID_PARAMETER);
            }
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_RANDOM_ADDR_CFM:
        {
            CmExtAdvSetRandomAddrCfm *prim = (CmExtAdvSetRandomAddrCfm *) primitive;
            if((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                bapBroadcastSrcStreamExtAdvertSetData(bigStream);
            }
        }
        break;

        case CSR_BT_CM_EXT_ADV_SET_DATA_CFM:
        {
            CmExtAdvSetDataCfm *prim = (CmExtAdvSetDataCfm *) primitive;

            if(bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            {
                if((bigStream->bigState == BAP_BIG_STATE_CONFIGURING)
                      && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
                {
                    result = BAP_RESULT_SUCCESS;
                    CmPeriodicAdvSetParamsReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle, 0,
                    BapSrcAdvPaParams.periodicAdvIntervalMin,BapSrcAdvPaParams.periodicAdvIntervalMax, 0);
                }
                else if ((bigStream->bigState == BAP_BIG_STATE_CONFIGURED)
                        && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
                {
                    if (bigStream->pendingOperation == BAP_BIG_RECONFIG_OPERATION)
                    {
                        result = BAP_RESULT_SUCCESS;
                        bapBroadcastSrcStreamPASetData(bigStream);
                    }
                }
                else
                {
                    BAP_CLIENT_DEBUG(" CSR_BT_CM_EXT_ADV_SET_DATA_CFM received with status %d \n", prim->resultCode);
                    bapBroadcastSrcConfigureStreamCfmSend(bap->appPhandle,
                        bigStream->profilePhandle, BAP_RESULT_INVALID_PARAMETER);
                }
            }
        }
        break;
        case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_CFM:
        {
            CmExtAdvSetScanRespDataCfm *prim = (CmExtAdvSetScanRespDataCfm *) primitive;
            if((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                /*CmExtAdvEnableReqSend(big_stream->advHandle, 1);*/
                CmPeriodicAdvSetParamsReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle, 0,
                BapSrcAdvPaParams.periodicAdvIntervalMin,BapSrcAdvPaParams.periodicAdvIntervalMax, 0);
            }
        }
        break;
        case CSR_BT_CM_EXT_ADV_ENABLE_CFM:
        {
            CmExtAdvEnableCfm *prim = (CmExtAdvEnableCfm *) primitive;

            if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                result = BAP_RESULT_SUCCESS;
            }
        }
        break;

        case CSR_BT_CM_EXT_ADV_UNREGISTER_APP_ADV_SET_CFM:
        {
            CmExtAdvUnregisterAppAdvSetCfm *prim = (CmExtAdvUnregisterAppAdvSetCfm *) primitive;

            if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                result = BAP_RESULT_SUCCESS;
            }

            if ((bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
                && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                bigStream->advHandle = BAP_INAVALID_ADV_HANDLE;
            }
        }
        break;

        case CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM:
        {
            CmPeriodicAdvSetParamsCfm *prim = (CmPeriodicAdvSetParamsCfm *) primitive;
            if(bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream)
                && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                bapBroadcastSrcStreamPASetData(bigStream);
            }
            else
                BAP_CLIENT_ERROR(" CSR_BT_CM_PERIODIC_ADV_SET_PARAMS_CFM received failed %d \n",prim->resultCode);

        }
        break;
        case CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM:
        {
            CmPeriodicAdvSetDataCfm *prim = (CmPeriodicAdvSetDataCfm *) primitive;

            if(bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            {
                if(bigStream->bigState == BAP_BIG_STATE_IDLE)
                {
                    BAP_CLIENT_ERROR("BAP: No broadcast source found with config/Streaming state\n");
                }
            }

            if( bigStream )
            {
                if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    result = BAP_RESULT_SUCCESS;

                    if((bigStream->bigState == BAP_BIG_STATE_CONFIGURING) && (!bigStream->periodicAdvDataFragmented))
                    {
                        CmPeriodicAdvStartReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle);
                        bigStream->periodicAdvDataFragmented = TRUE;
                    }
                }
                else
                {
                    result = BAP_RESULT_ERROR;
                    BAP_CLIENT_ERROR(" CSR_BT_CM_PERIODIC_ADV_SET_DATA_CFM failed 0x%x\n", prim->resultCode);
                }

                if((bigStream->bigState == BAP_BIG_STATE_STREAMING) ||
                        (bigStream->bigState == BAP_BIG_STATE_CONFIGURED))
                {
                    if(bigStream->pendingOperation == BAP_BIG_RECONFIG_OPERATION)
                    {
                        bapBroadcastSrcConfigureStreamCfmSend(bap->appPhandle,
                                        bigStream->profilePhandle, result);
                        bigStream->pendingOperation = 0;
                    }
                    else
                    {
                        bapBroadcastSrcUpdateMetadataCfmSend(bap->appPhandle, bigStream->profilePhandle,
                                                             result, bigStream->bigId);
                    }
                }
            }
        }
        break;
        case CSR_BT_CM_PERIODIC_ADV_START_CFM:
        {
            CmPeriodicAdvStartCfm *prim = (CmPeriodicAdvStartCfm *) primitive;
            if(bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream)
                           && (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
            {
                result = BAP_RESULT_SUCCESS;
                /* Move the state to CONFIGURED */
                bigStream->bigState = BAP_BIG_STATE_CONFIGURED;
                if(bigStream->pendingOperation == BAP_BIG_RECONFIG_OPERATION)
                    bigStream->pendingOperation = 0;
                bapBroadcastSrcConfigureStreamCfmSend(bigStream->appPhandle,
                                bigStream->profilePhandle, BAP_RESULT_SUCCESS);
            }
        }
        break;
        case CSR_BT_CM_PERIODIC_ADV_STOP_CFM:
        {
            CmPeriodicAdvStopCfm *prim = (CmPeriodicAdvStopCfm *) primitive;

            if(bapBroadcastSrcFindStreamByAdvHandle(bap, prim->advHandle, &bigStream))
            {
                if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
                {
                    result = BAP_RESULT_SUCCESS;
                    /* Move the state to IDLE */
                    bigStream->bigState = BAP_BIG_STATE_IDLE;
                    bapBroadcastSrcReleaseStreamCfmSend(bigStream->appPhandle, bigStream->profilePhandle,
                                    BAP_RESULT_SUCCESS, bigStream->bigId);
                    CmExtAdvUnregisterAppAdvSetReqSend(CSR_BT_BAP_IFACEQUEUE, bigStream->advHandle);
                }
                else
                {
                    bapBroadcastSrcReleaseStreamCfmSend(bigStream->appPhandle, bigStream->profilePhandle,
                                                        result, bigStream->bigId);
                }
            }
            /* cleanup the BigInfo */
            if(bigStream)
            {
                if(bigStream->subgroupInfo)
                {
                    uint8 i = 0;
                    for(i = 0; i < bigStream->numSubgroup; i++)
                    {
                        if(bigStream->subgroupInfo[i].metadata.metadata)
                        {
                            CsrPmemFree(bigStream->subgroupInfo[i].metadata.metadata);
                            bigStream->subgroupInfo[i].metadata.metadata = NULL;
                        }

                        if(bigStream->subgroupInfo[i].bisInfo)
                        {
                            CsrPmemFree(bigStream->subgroupInfo[i].bisInfo);
                            bigStream->subgroupInfo[i].bisInfo = NULL;
                        }
                    }
                    CsrPmemFree(bigStream->subgroupInfo);
                    bigStream->subgroupInfo = NULL;
					bigStream->numSubgroup = 0;
                }
                if (bigStream->bigNameLen)
                {
                    CsrPmemFree(bigStream->bigName);
                    bigStream->bigNameLen = 0;
                    bigStream->bigName = NULL;
                }
                if (bigStream->broadcastInfo)
                {
                    CsrPmemFree(bigStream->broadcastInfo);
                    bigStream->broadcastInfo = NULL;
                }
                if(bigStream->broadcastCode)
                {
                    CsrPmemFree(bigStream->broadcastCode);
                    bigStream->broadcastCode = NULL; 
                }
            }
        }
        break;
        case CSR_BT_CM_ISOC_CREATE_BIG_CFM:
        {
            CmIsocCreateBigCfm *prim = (CmIsocCreateBigCfm *) primitive;

            BAP_CLIENT_DEBUG("** CSR_BT_CM_ISOC_CREATE_BIG_CFM: BigHandle %x Result 0x%x num_bis %d  big_sync_delay %d transport_latency_big %d max_pdu %d iso_interval %d  phy %d nse %d bn %d \n",
            prim->big_handle,
            prim->resultCode,
            prim->num_bis,
            prim->big_sync_delay,
            prim->big_params.transport_latency_big,
            prim->big_params.max_pdu,
            prim->big_params.iso_interval,
            prim->big_params.phy,
            prim->big_params.nse,
            prim->big_params.bn
            );
            
            if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                uint8 i = 0;
                uint8 j = 0;
                uint8 k = 0;

                result = BAP_RESULT_SUCCESS;

                if(bapBroadcastSrcFindStreamByBigId(bap, prim->big_handle, &bigStream))
                {
                    /* Move the state to Streaming */
                    bigStream->bigState = BAP_BIG_STATE_STREAMING;
                    /* Fill the bisHandle */
                    for(i=0; i<bigStream->numSubgroup; i++)
                    {
                        for(j=0; j<bigStream->subgroupInfo[i].numBis; j++)
                        {
                            if(k < prim->num_bis)
                            {
                                bigStream->subgroupInfo[i].bisInfo[j].bisHandle = prim->bis_handles[k++];
                            }
                        }
                    }

                    bapBroadcastSrcEnableStreamCfmSend(bigStream->appPhandle, bigStream->profilePhandle,
                                    BAP_RESULT_SUCCESS, bigStream->bigId, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                        prim->num_bis, prim->bis_handles);
                }
            }
            else
            {
                if(bigStream)
                {
                    bapBroadcastSrcEnableStreamCfmSend(bap->appPhandle, bigStream->profilePhandle,
                                    result, bigStream->bigId, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                     prim->num_bis, prim->bis_handles);
                }
                else
                {
                    bapBroadcastSrcEnableStreamCfmSend(bap->appPhandle, 0,
                                    result, 0, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                     prim->num_bis, prim->bis_handles);
                }
            }
        }
        break;
       case CSR_BT_CM_ISOC_CREATE_BIG_TEST_CFM:
        {
            CmIsocCreateBigTestCfm *prim = (CmIsocCreateBigTestCfm *) primitive;

            if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                uint8 i = 0;
                uint8 j = 0;
                uint8 k = 0;

                result = BAP_RESULT_SUCCESS;

                if(bapBroadcastSrcFindStreamByBigId(bap, prim->big_handle, &bigStream))
                {
                    /* Move the state to IDLE */
                    bigStream->bigState = BAP_BIG_STATE_STREAMING;
                    /* Fill the bisHandle */
                    for(i=0; i<bigStream->numSubgroup; i++)
                    {
                        for(j=0; j<bigStream->subgroupInfo[i].numBis; j++)
                        {
                            if(k < prim->num_bis)
                            {
                                bigStream->subgroupInfo[i].bisInfo[j].bisHandle = prim->bis_handles[k++];
                            }
                        }
                    }

                    bapBroadcastSrcEnableStreamTestCfmSend(bigStream->appPhandle, bigStream->profilePhandle,
                                    BAP_RESULT_SUCCESS, bigStream->bigId, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                        prim->num_bis, prim->bis_handles);
                }
            }
            else
            {
                if(bigStream)
                {
                    bapBroadcastSrcEnableStreamTestCfmSend(bap->appPhandle, bigStream->profilePhandle,
                                    result, bigStream->bigId, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                     prim->num_bis, prim->bis_handles);
                }
                else
                {
                    bapBroadcastSrcEnableStreamTestCfmSend(bap->appPhandle, 0,
                                    result, 0, prim->big_sync_delay,
                                    (BapBigParam *)&prim->big_params,
                                     prim->num_bis, prim->bis_handles);
                }
            }
        }
        break;
        case CSR_BT_CM_ISOC_TERMINATE_BIG_CFM:
        {
            CmIsocTerminateBigCfm *prim = (CmIsocTerminateBigCfm *) primitive;

            if(bapBroadcastSrcFindStreamByBigId(bap, prim->big_handle, &bigStream))
            {
                /* Move the state to IDLE */
                result = BAP_RESULT_SUCCESS;
                bigStream->bigState = BAP_BIG_STATE_CONFIGURED;
                bapBroadcastSrcDisableStreamCfmSend(bigStream->appPhandle, bigStream->profilePhandle,
                                result, bigStream->bigId);
            }
        }
        break;

        default:
            BAP_CLIENT_WARNING(" BAP: CM broadcast mesg not handled\n");
        break;
        }

    if((bigStream ) && (result != BAP_RESULT_SUCCESS))
    {
        if((bigStream->bigState == BAP_BIG_STATE_CONFIGURING) 
                  ||(bigStream->pendingOperation == BAP_BIG_RECONFIG_OPERATION))
        {
            bapBroadcastSrcConfigureStreamCfmSend(bigStream->appPhandle,
                                                 bigStream->profilePhandle, result);
        }
    }
}
#endif /* INSTALL_LEA_BROADCAST_SOURCE */

#ifdef __cplusplus
}
#endif
#endif /* BAP_BROADCAT_SRC_STREAM_H_ */

/**@}*/
