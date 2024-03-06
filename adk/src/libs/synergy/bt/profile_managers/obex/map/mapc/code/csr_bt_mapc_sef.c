/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
#include "csr_bt_mapc_handler.h"
#include "csr_bt_mapc_sef.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

#include "csr_bt_sdc_support.h"
#include "csr_log_text_2.h"
#include "csr_bt_assert.h"
#include "csr_bt_mapc_private_lib.h"


void CsrBtMapcMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_MAPC_PRIM, msg);
}

static void csrBtMapcNotiServiceConnectIndHandler(void   *instData,
                                             CsrBtDeviceAddr         deviceAddr,
                                             CsrBtConnId             cid,
                                             CsrUint16               maxPeerObexPacketLength,
                                             CsrUint16               obexResponsePacketLength,
                                             CsrUint32               length,
                                             CsrUint32               count,
                                             CsrUint16               targetHeaderLength,
                                             CsrUint8                *target,
                                             CsrBool                 authenticated,
                                             CsrBool                 challenged,
                                             CsrUint16               obexPacketLength,
                                             CsrUint8                *obexPacket);
static void csrBtMapcNotiServicePutIndHandler
                (
                    void      *instData,
                    CsrUint16  obexResponsePacketLength,
                    CsrBool    authenticated,
                    CsrBool    finalBitSet,
                    CsrUint16  bodyLength,
                    CsrUint16  bodyOffset,
                    CsrUint16  obexPacketLength,
                    CsrUint8  *obexPacket
                );
static void csrBtMapcNotiServiceAbortIndHandler
                (
                    void      *instData,
                    CsrUint16  descriptionOffset,
                    CsrUint16  descriptionLength,
                    CsrUint16  obexPacketLength,
                    CsrUint8  *obexPacket
                );
static void csrBtMapcNotiServiceDisconnectIndHandler
                (
                    void            *instData,
                    CsrBtDeviceAddr  deviceAddr,
                    CsrBtReasonCode  reasonCode,
                    CsrBtSupplier    reasonSupplier,
                    CsrUint16        obexPacketLength,
                    CsrUint8        *obexPacket
                );
static void csrBtMapcNotiServiceActivateCfmHandler
                (
                    void            *instData,
                    CsrUint8         serverChannel,
                    psm_t            psm,
                    CsrBtResultCode  resultCode,
                    CsrBtSupplier    resultSupplier
                );

#define MAPC_MAS_TARGET {0xbb,0x58,0x2b,0x40,0x42,0x0c,0x11,0xdb,0xb0,0xde,0x08,0x00,0x20,0x0c,0x9a,0x66}
#define MAPC_MNS_TARGET {0xbb,0x58,0x2b,0x41,0x42,0x0c,0x11,0xdb,0xb0,0xde,0x08,0x00,0x20,0x0c,0x9a,0x66}
#define MAPC_TARGET_LENGTH 0x10

static CsrBool csrBtMapcExtractMasSdpParameters(MapcInstanceData *pInst,
                                                CmnCsrBtLinkedListStruct *sdpTagList, 
                                                CsrUint16 sdpRecordIndex,
                                                CsrBtDeviceName *serviceName, 
                                                CsrUint8 *masInstanceId, 
                                                CsrBtMapMesSupport *supportedMessages,
                                                CsrBtMapSupportedFeatures *supportedFeatures)
{
    CsrBool      gotValidSdpRecord = FALSE;
    CsrBtUuid32  tmpUuid = 0;
    CsrUint16    tmpResult;
    CsrUint16    dummy1,dummy2;
    CsrUint8     *string;
    CsrUint16    stringLen;
    CsrUint32    returnValue;

    CSR_UNUSED(pInst);

    CsrMemSet(*serviceName, 0, sizeof(*serviceName));
    *masInstanceId = 0;
    *supportedMessages = CSR_BT_MAP_NO_TYPE_SUPPORT;
    *supportedFeatures = 0;

    if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,sdpRecordIndex, &tmpUuid, &tmpResult, &dummy1, &dummy2))
    {
        if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
        {
            if (tmpUuid == CSR_BT_OBEX_MESSAGE_ACCESS_SERVER_UUID)
            {
                gotValidSdpRecord = TRUE;
                if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(sdpTagList, sdpRecordIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, &string, &stringLen))
                {
                    if (stringLen > CSR_BT_MAX_FRIENDLY_NAME_LEN)
                    {
                        stringLen = CSR_BT_MAX_FRIENDLY_NAME_LEN;
                    }
                    SynMemCpyS((CsrUint8 *) *serviceName, sizeof(*serviceName), string, stringLen);
                    CsrUtf8StrTruncate(*serviceName, stringLen);
                }

                if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                            CSR_BT_MAS_INSTANCE_ID_ATTRIBUTE_IDENTIFIER, &returnValue))
                {
                    *masInstanceId = (CsrUint8) returnValue;
                }

                if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                            CSR_BT_SUPPORTED_MESSAGE_TYPES_ATTRIBUTE_IDENTIFIER, &returnValue))
                {
                    *supportedMessages = (CsrUint8) returnValue;
                }

                if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTagList, sdpRecordIndex,
                                            CSR_BT_OBEX_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, &returnValue))
                {
                    *supportedFeatures = returnValue;
                }
                else
                {
                    *supportedFeatures = CSR_BT_MAP_SF_DEFAULT;
                }
            }
        }
    }
    return gotValidSdpRecord;
}

static void csrBtMapcConnectResultHandler(void                       *instData,
                                            CsrBtResultCode          resultCode,
                                            CsrBtSupplier            resultSupplier,
                                            CsrBtDeviceAddr          deviceAddr,
                                            CsrBtConnId              cid,
                                            CsrUint16                maxPeerObexPacketLength,
                                            CmnCsrBtLinkedListStruct *sdpTag,
                                            CsrUint16                obexPacketLength,
                                            CsrUint8                 *obexPacket)
{
    MapcInstanceData * pInst    = instData;

    CsrBtMapcConnectCfm *pMsg   = (CsrBtMapcConnectCfm *) CsrPmemAlloc(sizeof(CsrBtMapcConnectCfm));
    pMsg->type                  = CSR_BT_MAPC_CONNECT_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;

    /* Index would be zero since SDC Common would always provide one SDP Tag */
    csrBtMapcExtractMasSdpParameters(pInst,sdpTag,0,&pMsg->serviceName,&pMsg->masInstanceId,&pMsg->supportedMessages, &pMsg->features);
    pMsg->obexPeerMaxPacketSize = maxPeerObexPacketLength;
    pMsg->resultCode            = resultCode;
    pMsg->resultSupplier        = resultSupplier;
    pMsg->btConnId              = cid;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);

    if((resultCode == CSR_BT_OBEX_SUCCESS_RESPONSE_CODE) && (resultSupplier == CSR_BT_SUPPLIER_IRDA_OBEX))
    {
        pInst->masState = MAPC_MAS_STATE_CONNECTED;
        pInst->masConnId = cid;
    }
    else
    {
        pInst->masState = MAPC_MAS_STATE_IDLE;
    }
    /* MNS registration is always OFF by default */
    pInst->notificationRegistrationOn = FALSE;

    CsrBtUtilBllFreeLinkedList(&(sdpTag), CsrBtUtilBllPfreeWrapper);

    CsrPmemFree(pInst->masInstToServiceMap);
    pInst->masInstToServiceMap = NULL;

    CSR_UNUSED(obexPacketLength);
    CSR_UNUSED(deviceAddr);
    CsrPmemFree(obexPacket);
}

static void csrBtMapcDisconnectResultHandler(void           *instData,
                                            CsrBtReasonCode reasonCode,
                                            CsrBtSupplier   reasonSupplier,
                                            CsrUint8        *obexPacket,
                                            CsrUint16       obexPacketLength)
{
    MapcInstanceData * pInst        = instData;
    CsrBtMapcDisconnectInd *pMsg    = CsrPmemAlloc(sizeof(*pMsg));
    pMsg->type                      = CSR_BT_MAPC_DISCONNECT_IND;
    pMsg->instanceId                = pInst->mapcInstanceId;
    pMsg->reasonCode                = reasonCode; 
    pMsg->reasonSupplier            = reasonSupplier;
    CsrBtMapcMessagePut(pInst->appHandle, pMsg);

    pInst->masState = MAPC_MAS_STATE_IDLE;
    pInst->masConnId = 0;

    /* Force remove the notification service on mas disconnection */
    CsrBtMapcRemoveNotiReqSend(pInst->deviceAddr, 
                                pInst->appHandle, 
                                pInst->masInstanceId, 
                                TRUE);

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static void csrBtMapcSendSelectMasInstanceInd(MapcInstanceData *pInst,
                                              CsrBtMapcMasInstance *masInstanceList,
                                              CsrUint16           masInstanceListSize)
{
    CsrBtMapcSelectMasInstanceInd *pMsg   = (CsrBtMapcSelectMasInstanceInd *) CsrPmemAlloc(sizeof(CsrBtMapcSelectMasInstanceInd));
    pMsg->type                  = CSR_BT_MAPC_SELECT_MAS_INSTANCE_IND;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->masInstanceList       = masInstanceList;
    pMsg->masInstanceListSize   = masInstanceListSize;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
}

static void csrBtMapcSelectMasInstanceIdHandler(void                     *instData,
                                               CsrUint16                entriesInSdpTaglist,
                                               CmnCsrBtLinkedListStruct *sdpTagList)
{
    MapcInstanceData * pInst    = instData;
    CsrUint16 sdpRecordIndex;
    CsrUint16    idx = 0;
    CsrBool      gotValidSdpRecord = FALSE;
    CsrBtMapcMasInstance *masInstanceList =  (CsrBtMapcMasInstance *) CsrPmemAlloc(sizeof(CsrBtMapcMasInstance) * entriesInSdpTaglist);

    pInst->masInstToServiceMapLength = entriesInSdpTaglist;
    pInst->masInstToServiceMap =   (CsrBtMapcMasInstIdMap *)CsrPmemAlloc(sizeof(CsrBtMapcMasInstIdMap) * entriesInSdpTaglist);

    for (sdpRecordIndex=0; sdpRecordIndex < entriesInSdpTaglist; sdpRecordIndex++ )
    {
        if (csrBtMapcExtractMasSdpParameters(pInst,sdpTagList,sdpRecordIndex,&masInstanceList[idx].serviceName,&masInstanceList[idx].masInstanceId,&masInstanceList[idx].supportedMessages, &masInstanceList[idx].features))
        {
            gotValidSdpRecord = TRUE;
            pInst->masInstToServiceMap[idx].serviceIdx = sdpRecordIndex;
            pInst->masInstToServiceMap[idx].masInstanceId = masInstanceList[idx].masInstanceId;
            pInst->masInstToServiceMap[idx].features= masInstanceList[idx].features;
            idx++;
        }
    }

    if(gotValidSdpRecord)
    {
        csrBtMapcSendSelectMasInstanceInd(pInst, masInstanceList, entriesInSdpTaglist);
    }
    else
    {
        ObexUtilSetServiceHandleIndexListResponse(pInst->obexClientInst, NULL, 0);
        CsrPmemFree(masInstanceList);
    }
}

static void csrBtMapcSetFolderResultHandler(void  *    instData,
                                      CsrUint8    responseCode,
                                      CsrUint8 * obexPacket,
                                      CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst = instData;
    CsrBtMapcSetFolderCfm *pMsg   = (CsrBtMapcSetFolderCfm *) CsrPmemAlloc(sizeof(CsrBtMapcSetFolderCfm));
    pMsg->type              = CSR_BT_MAPC_SET_FOLDER_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result            = responseCode;
    CsrBtMapcMessagePut(pInst->appHandle, pMsg);

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static void csrBtMapcSetBackFolderResultHandler(void  *    instData,
                                          CsrUint8    responseCode,
                                          CsrUint8 * obexPacket,
                                          CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst     = instData;
    CsrBtMapcSetBackFolderCfm *pMsg   = CsrPmemAlloc(sizeof(CsrBtMapcSetBackFolderCfm));
    pMsg->type                  = CSR_BT_MAPC_SET_BACK_FOLDER_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;
    CsrBtMapcMessagePut(pInst->appHandle, pMsg);

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static void csrBtMapcSetRootFolderResultHandler(void  *    instData,
                                          CsrUint8    responseCode,
                                          CsrUint8 * obexPacket,
                                          CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst     = instData;
    CsrBtMapcSetRootFolderCfm *pMsg   = (CsrBtMapcSetRootFolderCfm *) CsrPmemAlloc(sizeof(CsrBtMapcSetRootFolderCfm));
    pMsg->type                  = CSR_BT_MAPC_SET_ROOT_FOLDER_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;
    CsrBtMapcMessagePut(pInst->appHandle, pMsg);

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static CsrUint16 csrBtObexUtilAddApplicationParametersHeaderField(CsrUint8 **appHeader, CsrUint16 offset, CsrUint8 tagId, CsrUint8 tagLength, CsrUint8 *tagValue)
{
    CsrUint16 retLength = 0;
    
    if(*appHeader == NULL)
    { /* First entry in the header */
        offset = 3; 
        retLength = 3 + 2 + tagLength;
        *appHeader = CsrPmemAlloc(retLength);
        (*appHeader)[0] = CSR_BT_OBEX_APPLICATION_PARAMETERS_HEADER;
    }
    else
    { /* Not the first entry in the header, we'll append this new tag to the end of the existing header */
        CsrUint8 *tempAppHeader = *appHeader;
        retLength = 2 + tagLength + offset;
        *appHeader = CsrPmemAlloc(retLength);
        SynMemCpyS(*appHeader, retLength, tempAppHeader, offset);
        CsrPmemFree(tempAppHeader);
    }

    /* Update full header length */
    CSR_BT_OBEX_SET_WORD_AT(*appHeader, 1, retLength);

    /* Insert tagId */
    (*appHeader)[offset] = tagId;

    /* Insert tagLength */
    (*appHeader)[offset + 1] = tagLength;

    if(tagLength > 0)
    {
        /* Insert tagValue */
        SynMemCpyS(&(*appHeader)[offset + 2], tagLength, tagValue, tagLength);
    }
    
    return retLength;
}

static CsrBool csrBtObexUtilExtractApplicationParametersHeaderValueField(CsrUint8 *obexPacket,
                                                                  CsrUint16 packetLength,
                                                                  CsrUint8 tagId,
                                                                  CsrUint8 *tagLength,
                                                                  CsrUint8 **tagValue)
{
    *tagLength = 0;
    *tagValue = NULL;

    if (obexPacket)
    {
        CsrUint16 appParamLen, appParamOffset;

        if (CsrBtObexGetAppParametersOffset(CSR_BT_OBEX_REQUEST,
                                            obexPacket,
                                            packetLength,
                                            &appParamOffset,
                                            &appParamLen))
        {
            CsrUint16 i;
            CsrUint8 *appParams = &obexPacket[appParamOffset];

            for (i = 0; i + 1 < appParamLen; i += appParams[i + 1] + 2)
            {
                if (appParams[i] == tagId)
                {
                    CsrUint8 len = appParams[i + 1];

                    if (len && i + 1 + len < appParamLen)
                    {
                        *tagLength = len;
                        *tagValue = &appParams[i + 2];
                    }

                    return TRUE;
                }
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                 0,
                                 "Application response header is not found"));
        }
    }

    return FALSE;
}


static void csrBtMapcGetFolderListingResultHandler(void        *instData,
                                                   CsrUint8    responseCode,
                                                   CsrBool     bodyHeader,
                                                   CsrUint16   bodyLength,
                                                   CsrUint16   bodyOffset,
                                                   CsrUint8  *obexPacket,
                                                   CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst = instData;

    if(pInst->fullSize == 0)
    {
        CsrUint8 tagLength;
        CsrUint8 *tagValue;
        
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_FOLDER_LISTING_SIZE, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_FOLDER_LISTING_SIZE && tagValue)
            {
                pInst->fullSize = CSR_BT_OBEX_GET_WORD_AT(tagValue, 0);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                     0,
                                     "FOLDER_LISTING_SIZE tag is found but length field is wrong"));
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                 0,
                                 "Application response header is not found"));
        }
    }
    
    pInst->partialSize += bodyLength;

    if (responseCode != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
    {
        CsrBtMapcGetFolderListingCfm *pMsg = (CsrBtMapcGetFolderListingCfm *) CsrPmemAlloc(sizeof(CsrBtMapcGetFolderListingCfm));

        pMsg->type                    = CSR_BT_MAPC_GET_FOLDER_LISTING_CFM;
        pMsg->instanceId              = pInst->mapcInstanceId;
        pMsg->result                  = responseCode;
        pMsg->fullFolderListingSize   = pInst->partialSize; 
        pMsg->bodyLength              = bodyLength; 
        pMsg->bodyOffset              = bodyOffset;  
        pMsg->payload                 = obexPacket;
        pMsg->payloadLength           = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }
    else
    {
        CsrBtMapcGetFolderListingInd *pMsg = (CsrBtMapcGetFolderListingInd *) CsrPmemAlloc(sizeof(CsrBtMapcGetFolderListingInd));

        pMsg->type                    = CSR_BT_MAPC_GET_FOLDER_LISTING_IND;
        pMsg->instanceId              = pInst->mapcInstanceId;
        pMsg->fullFolderListingSize   = pInst->fullSize;
        pMsg->folderListingRetrieved  = pInst->partialSize;
        pMsg->bodyLength              = bodyLength; 
        pMsg->bodyOffset              = bodyOffset;  
        pMsg->payload                 = obexPacket;
        pMsg->payloadLength           = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }

    CSR_UNUSED(bodyHeader);
    
}

static void csrBtMapcGetMessageListingResultHandler(void        *instData,
                                                    CsrUint8    responseCode,
                                                    CsrBool     bodyHeader,
                                                    CsrUint16   bodyLength,
                                                    CsrUint16   bodyOffset,
                                                    CsrUint8  *obexPacket,
                                                    CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst = instData;
    CsrUint16 size = 0;
    CsrUint8 tagLength;
    CsrUint8 *tagValue;

    if(pInst->fullSize == 0)
    {
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_MESSAGES_LISTING_SIZE, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_MESSAGES_LISTING_SIZE && tagValue)
            {
                pInst->fullSize = CSR_BT_OBEX_GET_WORD_AT(tagValue, 0);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                     0,
                                     "MESSAGES_LISTING_SIZE tag is found but length field is wrong"));
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                 0,
                                 "Application response header is not found"));
        }
    }

    if(pInst->newMessage == 0xFF)
    {
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_NEW_MESSAGE, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_NEW_MESSAGE && tagValue)
            {
                pInst->newMessage = CSR_BT_OBEX_GET_BYTE_AT(tagValue, 0);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                     0,
                                     "NEW_MESSAGE tag is found but length field is wrong"));
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                 0,
                                 "Application response header is not found"));
        }
    }

    if (CsrStrCmp((char*)pInst->mseTime,"") == 0) /* If 0 is returned -> First time the obex packet contain MSETime */
    {
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_MSE_TIME, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->mseTime);
                pInst->mseTime = CsrPmemAlloc(tagLength+1);
                SynMemCpyS(pInst->mseTime, (tagLength+1), tagValue, tagLength);
                pInst->mseTime[tagLength] = 0;
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                     0,
                                     "MSE tag is found but length field is wrong"));
            }
        }
        else
        {
            CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                 0,
                                 "Application response header is not found"));
        }
    }

    if (CsrStrCmp((char*) pInst->databaseId, "") == 0)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_DATABASE_ID, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->databaseId);
                pInst->databaseId = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->databaseId, (tagLength + 1), tagValue, tagLength);
                pInst->databaseId[tagLength] = 0;
            }
        }
    }

    if (CsrStrCmp((char*) pInst->folderVersionCounter, "") == 0)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_FOLDER_VER_COUNTER, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->folderVersionCounter);
                pInst->folderVersionCounter = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->folderVersionCounter, (tagLength + 1), tagValue, tagLength);
                pInst->folderVersionCounter[tagLength] = 0;
            }
        }
    }

    pInst->partialSize += bodyLength;

    if (responseCode != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
    {       
        CsrBtMapcGetMessageListingCfm *pMsg = (CsrBtMapcGetMessageListingCfm *) CsrPmemAlloc(sizeof(CsrBtMapcGetMessageListingCfm));

        if(pInst->partialSize == 0)
        {
            size = pInst->fullSize;
        }
        else
        {
           size = pInst->partialSize;
        }

        pMsg->type                    = CSR_BT_MAPC_GET_MESSAGE_LISTING_CFM;
        pMsg->instanceId              = pInst->mapcInstanceId;
        pMsg->result                  = responseCode;
        pMsg->newMessages             = pInst->newMessage;
        pMsg->fullMessageListingSize  = size;
        pMsg->mseTime                 = pInst->mseTime;
        pMsg->bodyLength              = bodyLength; 
        pMsg->bodyOffset              = bodyOffset;  
        pMsg->payload                 = obexPacket;
        pMsg->payloadLength           = obexPacketLength;
        pMsg->databaseId              = pInst->databaseId;
        pMsg->folderVersionCounter    = pInst->folderVersionCounter;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);

        pInst->mseTime = NULL;
        pInst->databaseId = NULL;
        pInst->folderVersionCounter = NULL;
    }
    else
    {
        CsrBtMapcGetMessageListingInd *pMsg = (CsrBtMapcGetMessageListingInd *) CsrPmemAlloc(sizeof(CsrBtMapcGetMessageListingInd));

        pMsg->type                    = CSR_BT_MAPC_GET_MESSAGE_LISTING_IND;
        pMsg->instanceId              = pInst->mapcInstanceId;
        pMsg->newMessages             = pInst->newMessage;
        pMsg->fullMessageListingSize  = pInst->fullSize;
        pMsg->messageListingRetrieved = pInst->partialSize;
        pMsg->mseTime                 = CsrUtf8StrDup((CsrUtf8String*) pInst->mseTime);
        pMsg->bodyLength              = bodyLength;
        pMsg->bodyOffset              = bodyOffset;  
        pMsg->payload                 = obexPacket;
        pMsg->payloadLength           = obexPacketLength;
        pMsg->databaseId              = CsrUtf8StrDup((CsrUtf8String*) pInst->databaseId);
        pMsg->folderVersionCounter    = CsrUtf8StrDup((CsrUtf8String*) pInst->folderVersionCounter);
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }

    CSR_UNUSED(bodyHeader);
}

static void csrBtMapcGetMessageResultHandler(void       *instData,
                                             CsrUint8   responseCode,
                                             CsrBool    bodyHeader,
                                             CsrUint16  bodyLength,
                                             CsrUint16  bodyOffset,
                                             CsrUint8 *obexPacket,
                                             CsrUint16  obexPacketLength)
{
    MapcInstanceData * pInst = instData;

    if(pInst->fractionDeliver == 0xFE)
    {
        CsrUint8 tagLength;
        CsrUint8 *tagValue;
        
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_FRACTION_DELIVER, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_FRACTION_DELIVER && tagValue)
            {
                pInst->fractionDeliver = CSR_BT_OBEX_GET_BYTE_AT(tagValue, 0);
            }
            else
            {
                CSR_LOG_TEXT_WARNING((CsrBtMapcLth,
                                     0,
                                     "FRACTION_DELIVER tag is found but length field is wrong so we now set it as unspecified"));
                pInst->fractionDeliver = CSR_BT_MAP_FRACTION_DEL_NOT_SPECIFIED;
            }
        }
        else
        {
            pInst->fractionDeliver = CSR_BT_MAP_FRACTION_DEL_NOT_SPECIFIED;
        }
    }

    if (responseCode != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
    {
        CsrBtMapcGetMessageCfm *pMsg      = (CsrBtMapcGetMessageCfm *) CsrPmemAlloc(sizeof(CsrBtMapcGetMessageCfm));
        pMsg->type              = CSR_BT_MAPC_GET_MESSAGE_CFM;
        pMsg->instanceId        = pInst->mapcInstanceId;
        pMsg->result            = responseCode;
        pMsg->fractionDeliver   = pInst->fractionDeliver;
        pMsg->bodyLength        = bodyLength; 
        pMsg->bodyOffset        = bodyOffset;  
        pMsg->payload           = obexPacket;
        pMsg->payloadLength     = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
        
    }
    else
    {
        CsrBtMapcGetMessageInd *pMsg  = (CsrBtMapcGetMessageInd *) CsrPmemAlloc(sizeof(CsrBtMapcGetMessageInd));
        pMsg->type              = CSR_BT_MAPC_GET_MESSAGE_IND;
        pMsg->instanceId        = pInst->mapcInstanceId;
        pMsg->bodyLength        = bodyLength; 
        pMsg->bodyOffset        = bodyOffset;  
        pMsg->payload           = obexPacket;
        pMsg->payloadLength     = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }
    CSR_UNUSED(bodyHeader);
}

static void csrBtMapcSetMessageStatusResultHandler(void  *    instData,
                                        CsrUint8    responseCode,
                                        CsrUint8 * obexPacket,
                                        CsrUint16   obexPacketLength)
{
    MapcInstanceData *pInst    = instData;
    CsrBtMapcSetMessageStatusCfm *pMsg     = (CsrBtMapcSetMessageStatusCfm *) CsrPmemAlloc(sizeof(CsrBtMapcSetMessageStatusCfm));

    pMsg->type                  = CSR_BT_MAPC_SET_MESSAGE_STATUS_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}

/* Continue handler for handing over filler byte to obex library */
static void csrBtMapcFillerBytePutContinueHandler(  void        *instData,
                                                    CsrUint16   obexPacketLength)
{
    MapcInstanceData *pInst    = instData;
    CsrUint8 *payload = CsrPmemAlloc(1);
    payload[0] = 0x30; /* Filler byte */

    ObexUtilPutContinueRequest(pInst->obexClientInst, TRUE, &payload, 1);
    CSR_UNUSED(obexPacketLength);
}

static void csrBtMapcPushMessageResultHandler(void  *    instData,
                                           CsrUint8    responseCode,
                                           CsrUint8 * obexPacket,
                                           CsrUint16   obexPacketLength)
{
    MapcInstanceData *pInst    = instData;
    CsrBtMapcPushMessageCfm *pMsg  = (CsrBtMapcPushMessageCfm *) CsrPmemAlloc(sizeof(CsrBtMapcPushMessageCfm));
    CsrUint16  nameOffset;

    pMsg->type                  = CSR_BT_MAPC_PUSH_MESSAGE_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;

    if(CsrBtObexGetNameOffset(CSR_BT_OBEX_RESPONSE_NORMAL, obexPacket, &nameOffset))
    {
        pMsg->messageHandle = (CsrUtf8String*) CsrUcs2ByteString2Utf8((const CsrUcs2String *) &obexPacket[nameOffset]);
    }
    else
    {
        pMsg->messageHandle = NULL;
    }

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}

static void csrBtMapcPushMessageContinueHandler(void  *    instData,
                                             CsrUint16   obexPacketLength)
{
    MapcInstanceData *pInst    = instData;
    CsrBtMapcPushMessageInd *pMsg  = (CsrBtMapcPushMessageInd *) CsrPmemAlloc(sizeof(CsrBtMapcPushMessageInd));

    pMsg->type                  = CSR_BT_MAPC_PUSH_MESSAGE_IND;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->maxAllowedPayloadSize = obexPacketLength;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
}

static void csrBtMapcUpdateInboxResultHandler(void  *    instData,
                                           CsrUint8    responseCode,
                                           CsrUint8 * obexPacket,
                                           CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst   = instData;
    CsrBtMapcUpdateInboxCfm *pMsg  = (CsrBtMapcUpdateInboxCfm *) CsrPmemAlloc(sizeof(CsrBtMapcUpdateInboxCfm));
    pMsg->type                  = CSR_BT_MAPC_UPDATE_INBOX_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}

static void csrBtMapcNotificationRegistrationResultHandler(void  *instData,
                                           CsrUint8    responseCode,
                                           CsrUint8   *obexPacket,
                                           CsrUint16   obexPacketLength)
{
    MapcInstanceData *pInst   = instData;

    /* If any registration fails then force remove the notification instance */
    if (responseCode != CSR_BT_OBEX_SUCCESS_RESPONSE_CODE)
    {
        pInst->notificationRegistrationOn = FALSE;
        CsrBtMapcRemoveNotiReqSend(pInst->deviceAddr, 
                                    pInst->appHandle, 
                                    pInst->masInstanceId, 
                                    TRUE);
    }
    else if (!pInst->notificationRegistrationOn)
    {
        /* If response was success then lets wait for the remote device to 
            initiate the disconnect */
        CsrBtMapcRemoveNotiReqSend(pInst->deviceAddr, 
                                    pInst->appHandle, 
                                    pInst->masInstanceId, 
                                    FALSE);
    }

    if (pInst->masState != MAPC_MAS_STATE_DISCONNECT_DEREGISTER)
    {
        CsrBtMapcNotificationRegistrationCfm *pMsg;

        /* Send the Noti Registration confirmation */
        pMsg  = CsrPmemAlloc(sizeof(*pMsg));
        pMsg->type                  = CSR_BT_MAPC_NOTIFICATION_REGISTRATION_CFM;
        pMsg->instanceId            = pInst->mapcInstanceId;
        pMsg->result                = responseCode;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }
    else
    {
        /* Do not send remove notification here since remove would be sent 
            on disconnect indication regardless of whether it was local or 
            remote initiated */
        ObexUtilDisconnectRequest(pInst->obexClientInst, pInst->normalDisconnect, NULL);
    }

    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}

static void csrBtMapcAbortResultHandler(void  *    instData,
                                    CsrUint8    responseCode,
                                    CsrUint8 * obexPacket,
                                    CsrUint16   obexPacketLength)
{
    MapcInstanceData * pInst     = instData;
    CsrBtMapcAbortCfm *pMsg      = (CsrBtMapcAbortCfm*) CsrPmemAlloc(sizeof(CsrBtMapcAbortCfm));
    pMsg->type                   = CSR_BT_MAPC_ABORT_CFM;
    pMsg->instanceId             = pInst->mapcInstanceId;
    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    
    CSR_UNUSED(responseCode);
    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
static void csrBtMapcSecurityInCfmSend(CsrSchedQid appHandle, CsrSchedQid instanceId, 
    CsrBtResultCode resultCode,  CsrBtSupplier resultSupplier)
{
    CsrBtMapcSecurityInCfm *pMsg    = (CsrBtMapcSecurityInCfm*)CsrPmemAlloc(sizeof(CsrBtMapcSecurityInCfm));
    pMsg->type                  = CSR_BT_MAPC_SECURITY_IN_CFM;
    pMsg->instanceId            = instanceId;
    pMsg->resultCode            = resultCode;
    pMsg->resultSupplier        = resultSupplier;
    CsrBtMapcMessagePut(appHandle, pMsg);
}

static void csrBtMapcSecurityOutCfmSend(CsrSchedQid appHandle, CsrSchedQid instanceId, 
    CsrBtResultCode resultCode, CsrBtSupplier resultSupplier)
{
    CsrBtMapcSecurityOutCfm *pMsg   = (CsrBtMapcSecurityOutCfm*)CsrPmemAlloc(sizeof(CsrBtMapcSecurityOutCfm));
    pMsg->type                  = CSR_BT_MAPC_SECURITY_OUT_CFM;
    pMsg->instanceId            = instanceId;
    pMsg->resultCode            = resultCode;
    pMsg->resultSupplier        = resultSupplier;
    CsrBtMapcMessagePut(appHandle, pMsg);
}
#endif 

static void csrBtMapcTagCopy(CsrUint8* dest, CsrUint8* src, CsrUint8 size)
{
    CsrUint8 i;
    for (i = 0; i < size; i++)
    {
        dest[i] = src[size - i - 1];
    }
}


static CsrUint8 csrBtObexMapcNotificationPutReqSend(MapcInstanceData *pInst)
{
    CsrUint8 headerPriorityTable[6]  = {CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                        CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrUint8 *appHeader = NULL;
    CsrUint16 appHeaderLength = 0;
    CsrUint8 temp[1];
    
    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pInst->notificationRegistrationOn);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_NOTIFICATION_STATUS, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_NOTIFICATION_STATUS, 
                                                                        temp);
    return ObexUtilPutRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              0, 
                              (CsrUint8*)(MAPC_TEXT_X_BT_NOTIFICATION_REGISTRATION), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL, 
                              csrBtMapcFillerBytePutContinueHandler, 
                              csrBtMapcNotificationRegistrationResultHandler);

}

CsrUint8 CsrBtMapcGetInstanceIdsReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcGetInstanceIdsReq *prim_req;
    CsrBtMapcGetInstanceIdsCfm *prim_cfm;
    CsrBtMapcInstancePool         *ptr;
    CsrUint8                    offset;

    if (pInst->mapcInstanceId == CSR_BT_MAPC_IFACEQUEUE)
    {
        prim_req=(CsrBtMapcGetInstanceIdsReq *) msg;

        prim_cfm = CsrPmemAlloc(sizeof(CsrBtMapcGetInstanceIdsCfm));

        prim_cfm->type = CSR_BT_MAPC_GET_INSTANCE_IDS_CFM;
        prim_cfm->instanceIdsListSize = pInst->numberOfMapcInstances;

        if (pInst->numberOfMapcInstances == 0)
        {
            prim_cfm->instanceIdsList = NULL;
        }
        else
        {
            prim_cfm->instanceIdsList = CsrPmemAlloc(sizeof(CsrSchedQid) *
                                              pInst->numberOfMapcInstances);

            ptr = pInst->mapcInstances;
            offset = 0;

            while(ptr)
            {
                SynMemCpyS(&prim_cfm->instanceIdsList[offset],
                       sizeof(CsrSchedQid) * ptr->numberInPool, 
                       ptr->phandles,
                       sizeof(CsrSchedQid) * ptr->numberInPool);

                offset += ptr->numberInPool;
                ptr = ptr->next;
            }
        }

        CsrBtMapcMessagePut(prim_req->appHandle,prim_cfm);
    }
    else
    {
        CsrGeneralException(CsrBtMapcLth,
                            0,
                            CSR_BT_MAPC_PRIM,
                            CSR_BT_MAPC_GET_INSTANCE_IDS_REQ,
                            0,
                            "Task not MAPC-manager");
    }
    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}


CsrUint8 CsrBtMapcNotificationRegistrationReqHandler(MapcInstanceData *pInst, void *message)
{
    CsrBtMapcNotificationRegistrationReq *pMsg = (CsrBtMapcNotificationRegistrationReq*)message;
   dm_security_level_t secIncoming = 0;

    if(pInst->masState == MAPC_MAS_STATE_CONNECTED)
    {
        pInst->notificationRegistrationOn = pMsg->enableNotifications;

        if (pInst->notificationRegistrationOn != FALSE)
        {
#ifndef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
            CsrBtScSetSecInLevel(&secIncoming, CSR_BT_SEC_DEFAULT,
                CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_INCOMING,
                CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_INCOMING,
                CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#else
            secIncoming  = pInst->secIncoming;
#endif /* INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS */

            CsrBtMapcAddNotiReqSend(pInst->deviceAddr, pInst->appHandle, pInst->masInstanceId, 
                                        secIncoming, pInst->maxFrameSize, pInst->windowSize);
        }
        else
        {
            csrBtObexMapcNotificationPutReqSend(pInst);
        }
        return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
    }
    else
    {
        return (CSR_BT_OBEX_UTIL_STATUS_EXCEPTION);
    }

}

CsrUint8 CsrBtMapcConnectReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint16  index;
    CsrUint8   headerPriorityTable[2]       = {CSR_BT_OBEX_UTIL_LENGTH_HEADER, CSR_BT_OBEX_UTIL_COUNT_HEADER};
    CsrUint8 tempTarget[MAPC_TARGET_LENGTH] = MAPC_MAS_TARGET;
    CsrBtMapcConnectReq *pMsg               = (CsrBtMapcConnectReq *) msg;
    CmnCsrBtLinkedListStruct * sdpTag       = NULL;
    CsrUint8 *target                        = (CsrUint8 *) CsrPmemAlloc(MAPC_TARGET_LENGTH);
    dm_security_level_t secOutgoing = 0;


    pInst->appHandle                        = pMsg->appHandle;
    pInst->windowSize = pMsg->windowSize;

    SynMemCpyS(target, MAPC_TARGET_LENGTH, tempTarget, MAPC_TARGET_LENGTH);
    sdpTag = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTag, CSR_BT_OBEX_MESSAGE_ACCESS_SERVER_UUID, &index);
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag, index, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag, index, CSR_BT_MAS_INSTANCE_ID_ATTRIBUTE_IDENTIFIER, NULL, 0);
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag, index, CSR_BT_SUPPORTED_MESSAGE_TYPES_ATTRIBUTE_IDENTIFIER, NULL, 0);
    CsrBtUtilSdrInsertLocalServerChannel(sdpTag, index, CSR_BT_NO_SERVER);
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag, index, CSR_BT_OBEX_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag, index, CSR_BT_OBEX_L2CAP_PSM_ATTRIBUTE, NULL, 0);
#endif 

    /* Only for the OBEX server begin */
    pInst->maxFrameSize = pMsg->maxPacketSize;

    /* Only for the OBEX server end*/
    CsrPmemFree(pInst->masInstToServiceMap);
    pInst->masInstToServiceMap = NULL;

    pInst->deviceAddr = pMsg->deviceAddr;

#ifndef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
    CsrBtScSetSecOutLevel(&secOutgoing, CSR_BT_SEC_DEFAULT,
                CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_OUTGOING,
                CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_OUTGOING,
                CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#else
    secOutgoing  = pInst->secOutgoing;
#endif /* INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS */

    return(ObexUtilConnectRequest(pInst->obexClientInst, 
                                  &sdpTag, 
                                  pMsg->deviceAddr, 
                                  secOutgoing, 
                                  pMsg->maxPacketSize, 
                                  CSR_BT_MAPC_PROFILE_DEFAULT_MTU_SIZE, 
                                  &target, 
                                  MAPC_TARGET_LENGTH, 
                                  0, /* Number of headers */
                                  headerPriorityTable, 
                                  CSR_BT_MAPC_LP_SUPERVISION_TIMEOUT, 
                                  NULL, 
                                  0, /* Length */
                                  0, /* Count */
                                  NULL,
                                  0,
                                  NULL,
                                  csrBtMapcConnectResultHandler, 
                                  NULL, 
                                  csrBtMapcDisconnectResultHandler, 
                                  csrBtMapcSelectMasInstanceIdHandler,
                                  pInst->windowSize,
                                  TRUE,
                                  CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                         CSR_BT_MAP_DEFAULT_ENC_KEY_SIZE_VAL)));
}

CsrUint8 CsrBtMapcCancelConnectReqHandler(MapcInstanceData *pInst, void *msg)
{
    CSR_UNUSED(msg);
    return (ObexUtilDisconnectRequest(pInst->obexClientInst, FALSE, NULL));
}

CsrUint8 CsrBtMapcDisconnectReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcDisconnectReq *pMsg = (CsrBtMapcDisconnectReq *) msg;
    CsrUint8 status;

    if (pInst->notificationRegistrationOn)
    {
        /* Note: we would have to serialize the notification off and then do 
         * disconnect for the reason that response to notification would be
         * received on the MAS connection.
         */
        pInst->notificationRegistrationOn = FALSE;
        pInst->masState = MAPC_MAS_STATE_DISCONNECT_DEREGISTER;
        pInst->normalDisconnect = pMsg->normalObexDisconnect;
        status = csrBtObexMapcNotificationPutReqSend(pInst);

		if (status == CSR_BT_OBEX_UTIL_STATUS_EXCEPTION)
		{
			/*Obex has not accepted the put request
			Send ind with status to application*/
			CsrBtMapcDisconnectInd *pMsg = CsrPmemAlloc(sizeof(*pMsg));

			pMsg->type = CSR_BT_MAPC_DISCONNECT_IND;
			pMsg->instanceId = pInst->mapcInstanceId;
			pMsg->reasonCode = CSR_BT_RESULT_CODE_OBEX_REQUEST_RECEIVED_IN_INVALID_STATE;
			pMsg->reasonSupplier = CSR_BT_SUPPLIER_OBEX_PROFILES;

			CsrBtMapcMessagePut(pInst->appHandle, pMsg);
		}
    }
    else
    {
        status = ObexUtilDisconnectRequest(pInst->obexClientInst, pMsg->normalObexDisconnect, NULL);
    }

    return status;
}

static void csrBtMapcFindServiceHandleIndexFromMasInstanceId(MapcInstanceData *pInst,
                                                                  CsrUint16 masInstanceId,
                                                                  CsrUint16 *serviceHandle,
                                                                  CsrBtMapSupportedFeatures *features)
{
    CsrUint16 i;
    *serviceHandle = 0xFFFF;
    *features = 0;
    if(pInst->masInstToServiceMap)
    {
        for(i=0;i<pInst->masInstToServiceMapLength;i++)
        {
            if(pInst->masInstToServiceMap[i].masInstanceId == masInstanceId)
            {
                *serviceHandle = pInst->masInstToServiceMap[i].serviceIdx;
                *features = pInst->masInstToServiceMap[i].features;
                break;
            }
        }

        CsrPmemFree(pInst->masInstToServiceMap);
        pInst->masInstToServiceMap = NULL;
    }
}


CsrUint8 CsrBtMapcSelectMasInstanceResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcSelectMasInstanceRes *pMsg = (CsrBtMapcSelectMasInstanceRes *) msg;
    CsrUint16 *serviceHandleIndexList = NULL;
    CsrUint16 nofServiceHandleIndicis = 0;

    if(pMsg->proceedWithConnection)
    {
        CsrUint16 tempServiceHandle;
        CsrBtMapSupportedFeatures features;

        csrBtMapcFindServiceHandleIndexFromMasInstanceId(pInst, pMsg->masInstanceId, &tempServiceHandle, &features);
        if(tempServiceHandle != 0xFFFF)
        {
            nofServiceHandleIndicis = 1;
            serviceHandleIndexList = CsrPmemAlloc(sizeof(CsrUint16));
            *serviceHandleIndexList = tempServiceHandle;
        }
        if (features & CSR_BT_MAP_SF_SUPP_FEATURES_IN_CONNECT_REQ)
        {
            CsrUint8 *buf;
            CsrUint16 appHeaderLength;
            CsrUint32 supportedFeatures;

            /* 4 bytes for supported features */
            appHeaderLength = CSR_BT_OBEX_APP_PAR_HEADER_SIZE + CSR_BT_OBEX_MAP_TAG_SIZE + 4;

            buf = CsrPmemZalloc(appHeaderLength);
            /* Add application parameter header */
            buf[0] = CSR_BT_OBEX_APPLICATION_PARAMETERS_HEADER;
            buf[1] = appHeaderLength >> 8;
            buf[2] = appHeaderLength & 0x00FF;
            buf[CSR_BT_OBEX_APP_PAR_HEADER_SIZE] = CSR_BT_MAP_TAG_ID_MAP_SUPPORTED_FEATURES;
            buf[CSR_BT_OBEX_APP_PAR_HEADER_SIZE + 1] = 4;

            supportedFeatures = CSR_BT_MAPC_SUPPORTED_FEATURES & CSR_BT_MAP_SUPPORTED_FEATURE_ALL;
            csrBtMapcTagCopy(buf + CSR_BT_OBEX_APP_PAR_HEADER_SIZE + CSR_BT_OBEX_MAP_TAG_SIZE, 
                                (CsrUint8 *) &supportedFeatures, 4);

            /* include the supported features header in connect request */
            ObexUtilUpdateAppHeader(pInst->obexClientInst, appHeaderLength, &buf);
        }
    }
    else
    {
        CsrPmemFree(pInst->masInstToServiceMap);
        pInst->masInstToServiceMap = NULL;
    }

    pInst->masInstanceId = pMsg->masInstanceId;

    return (ObexUtilSetServiceHandleIndexListResponse(pInst->obexClientInst, &serviceHandleIndexList, nofServiceHandleIndicis));
}

/* Browsing */
CsrUint8 CsrBtMapcSetFolderReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[3]  = {CSR_BT_OBEX_UTIL_NAME_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcSetFolderReq *pMsg           = (CsrBtMapcSetFolderReq *) msg;
    CsrUcs2String *folderName;

    folderName = CsrUtf82Ucs2ByteString((CsrUint8*)pMsg->folderName);

    CsrPmemFree(pMsg->folderName);
	pMsg->folderName = NULL;
    return (ObexUtilSetPathRequest(pInst->obexClientInst, 0x02, headerPriorityTable, &folderName, NULL, 0, NULL, csrBtMapcSetFolderResultHandler));
}

CsrUint8 CsrBtMapcSetBackFolderReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[3]  = {CSR_BT_OBEX_UTIL_EMPTY_NAME_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CSR_UNUSED(msg);
    return (ObexUtilSetPathRequest(pInst->obexClientInst, 0x03, headerPriorityTable, NULL, NULL, 0, NULL, csrBtMapcSetBackFolderResultHandler));
}

CsrUint8 CsrBtMapcSetRootFolderReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[3]  = {CSR_BT_OBEX_UTIL_EMPTY_NAME_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};

    CSR_UNUSED(msg);
    return (ObexUtilSetPathRequest(pInst->obexClientInst, 0x02, headerPriorityTable, NULL, NULL, 0, NULL, csrBtMapcSetRootFolderResultHandler));
}

CsrUint8 CsrBtMapcGetFolderListingReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[5]  = {CSR_BT_OBEX_UTIL_TYPE_HEADER, CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcGetFolderListingReq *pMsg       = (CsrBtMapcGetFolderListingReq *) msg;
    CsrUint8 *appHeader = NULL;
    CsrUint16 appHeaderLength = 0;
    CsrUint8 temp[2];

    /* Build applications parameter header */
    CSR_BT_OBEX_SET_WORD_AT(temp,0,pMsg->maxListCount);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_MAX_LIST_COUNT, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_MAX_LIST_COUNT, 
                                                                        temp);

    CSR_BT_OBEX_SET_WORD_AT(temp,0,pMsg->listStartOffset);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_START_OFFSET, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_START_OFFSET, 
                                                                        temp);
    pInst->fullSize = 0;
    pInst->partialSize = 0;

    pInst->srmp = pMsg->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*)("x-obex/folder-listing"), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL, 
                              pInst->srmp,
                              csrBtMapcGetFolderListingResultHandler, 
                              NULL));
}

CsrUint8 CsrBtMapcGetFolderListingResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcGetFolderListingRes *res = msg;

    pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetContinueRequest(pInst->obexClientInst, pInst->srmp));
}

CsrUint8 CsrBtMapcGetMessageListingReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                        headerPriorityTable[5]  = { CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_NAME_HEADER, 
                                                                CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcGetMessageListingReq   *pMsg           = msg;
    CsrUint8                        *appHeader      = NULL;
    CsrUint16                       appHeaderLength = 0;
    CsrUint8                        temp[4];
    CsrUcs2String                   *folderName     = NULL;

    /* If filterMessageHandle application parameter holds a value, all other application 
        parameters aside from ParameterMask and SubjectLength shall not be present. */
    if (!pMsg->filterMessageHandle)
    {
        /* Build applications parameter header */
        CSR_BT_OBEX_SET_WORD_AT(temp, 0, pMsg->maxListCount);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                            appHeaderLength,
                                                                            CSR_BT_MAP_TAG_ID_MAX_LIST_COUNT, 
                                                                            CSR_BT_MAP_TAG_ID_LENGTH_MAX_LIST_COUNT, 
                                                                            temp);

        CSR_BT_OBEX_SET_WORD_AT(temp, 0, pMsg->listStartOffset);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                            appHeaderLength,
                                                                            CSR_BT_MAP_TAG_ID_START_OFFSET, 
                                                                            CSR_BT_MAP_TAG_ID_LENGTH_START_OFFSET, 
                                                                            temp);
    }

    if (pMsg->maxSubjectLength > 0)
    {
        CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->maxSubjectLength);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                            appHeaderLength,
                                                                            CSR_BT_MAP_TAG_ID_SUBJECT_LENGTH, 
                                                                            CSR_BT_MAP_TAG_ID_LENGTH_SUBJECT_LENGTH, 
                                                                            temp);
    }
    
    CSR_BT_OBEX_SET_D_WORD_AT(temp, 0, pMsg->parameterMask);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_PARAMETER_MASK, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_PARAMETER_MASK, 
                                                                        temp);

    if (!pMsg->filterMessageHandle)
    {
        CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->filterMessageType);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                            appHeaderLength,
                                                                            CSR_BT_MAP_TAG_ID_FILTER_MESSAGE_TYPE, 
                                                                            CSR_BT_MAP_TAG_ID_LENGTH_FILTER_MESSAGE_TYPE, 
                                                                            temp);

        if(pMsg->filterPeriodBegin)
        {
            appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
                &appHeader, 
                appHeaderLength,
                CSR_BT_MAP_TAG_ID_FILTER_PERIOD_BEGIN,
                (CsrUint8) CsrStrLen((const char *) pMsg->filterPeriodBegin), 
                (CsrUint8*) pMsg->filterPeriodBegin);
        }
        
        if(pMsg->filterPeriodEnd)
        {
            appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
                &appHeader, 
                appHeaderLength,
                CSR_BT_MAP_TAG_ID_FILTER_PERIOD_END,
                (CsrUint8) CsrStrLen((const char *) pMsg->filterPeriodEnd), 
                (CsrUint8*) pMsg->filterPeriodEnd);
        }

        CSR_BT_OBEX_SET_BYTE_AT(temp,0,pMsg->filterReadStatus);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                            appHeaderLength,
                                                                            CSR_BT_MAP_TAG_ID_FILTER_READ_STATUS, 
                                                                            CSR_BT_MAP_TAG_ID_LENGTH_FILTER_READ_STATUS, 
                                                                            temp);

        if(pMsg->filterRecipient)
        {
            appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
                &appHeader, 
                appHeaderLength,
                CSR_BT_MAP_TAG_ID_FILTER_RECIPIENT, 
                (CsrUint8) CsrStrLen((const char *) pMsg->filterRecipient),
                (CsrUint8*) pMsg->filterRecipient);
        }
        
        if(pMsg->filterOriginator)
        {
            appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
                &appHeader, 
                appHeaderLength,
                CSR_BT_MAP_TAG_ID_FILTER_ORIGINATOR, 
                (CsrUint8) CsrStrLen((const char *) pMsg->filterOriginator), 
                (CsrUint8*) pMsg->filterOriginator);
        }

        CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->filterPriority);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FILTER_PRIORITY, 
            CSR_BT_MAP_TAG_ID_LENGTH_FILTER_PRIORITY, 
            (CsrUint8*) temp);

        if (pMsg->conversationId)
        {
            appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
                &appHeader, 
                appHeaderLength,
                CSR_BT_MAP_TAG_ID_CONVERSATION_ID, 
                (CsrUint8) CsrStrLen((const char *) pMsg->conversationId), 
                (CsrUint8*) pMsg->conversationId);
        }
        else
        {
            /* Name shall be hold a valid value only if there is no 
                conversation id to be queried */
            folderName = CsrUtf82Ucs2ByteString((CsrUint8*) pMsg->folderName);
        }
    }
    else
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FILTER_MSG_HANDLE,
            (CsrUint8) CsrStrLen((const char *) pMsg->filterMessageHandle), 
            (CsrUint8*) pMsg->filterMessageHandle);
    }

    /* Check values used in the result handler */
    pInst->newMessage = 0xFF;
    pInst->fullSize = 0;
    pInst->partialSize = 0;

    CsrPmemFree(pInst->mseTime);
    pInst->mseTime = (CsrUtf8String*) CsrPmemAlloc(sizeof(CsrUtf8String));    
    *pInst->mseTime = '\0';

    CsrPmemFree(pInst->databaseId);
    pInst->databaseId = (CsrUtf8String*) CsrPmemAlloc(sizeof(CsrUtf8String));    
    *pInst->databaseId = '\0';

    CsrPmemFree(pInst->folderVersionCounter);
    pInst->folderVersionCounter = (CsrUtf8String*) CsrPmemAlloc(sizeof(CsrUtf8String));    
    *pInst->folderVersionCounter = '\0';

    CsrPmemFree(pMsg->folderName);
    CsrPmemFree(pMsg->filterPeriodBegin);
    CsrPmemFree(pMsg->filterPeriodEnd);
    CsrPmemFree(pMsg->filterRecipient);
    CsrPmemFree(pMsg->filterOriginator);
    CsrPmemFree(pMsg->conversationId);
    CsrPmemFree(pMsg->filterMessageHandle);
	pMsg->folderName = NULL;
	pMsg->filterPeriodBegin = NULL;
	pMsg->filterPeriodEnd = NULL;
	pMsg->filterRecipient = NULL;
	pMsg->filterOriginator = NULL;
	pMsg->conversationId = NULL;
	pMsg->filterMessageHandle = NULL;

    pInst->srmp = pMsg->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*) (MAPC_TEXT_X_BT_MAP_MSG_LISTING), 
                              &folderName, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              pInst->srmp,
                              csrBtMapcGetMessageListingResultHandler, 
                              NULL));
}

CsrUint8 CsrBtMapcGetMessageListingResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcGetMessageListingRes *res = msg;

    pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetContinueRequest(pInst->obexClientInst, pInst->srmp));
}

CsrUint8 CsrBtMapcGetMessageReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[5]  = {CSR_BT_OBEX_UTIL_NAME_HEADER, 
                                        CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                        CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcGetMessageReq *pMsg    = (CsrBtMapcGetMessageReq *) msg;
    CsrUint8 *appHeader = NULL;
    CsrUint16 appHeaderLength = 0;
    CsrUint8 temp[1];
    CsrUcs2String *messageHandle = CsrUtf82Ucs2ByteString((CsrUint8*)pMsg->messageHandle);

    CSR_BT_OBEX_SET_BYTE_AT(temp,0,pMsg->attachment);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_ATTACHMENT, 
        CSR_BT_MAP_TAG_ID_LENGTH_ATTACHMENT, 
        temp);

    CSR_BT_OBEX_SET_BYTE_AT(temp,0,pMsg->charset);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_CHARSET, 
        CSR_BT_MAP_TAG_ID_LENGTH_CHARSET, 
        temp);

    if(pMsg->fractionRequest != CSR_BT_MAP_FRACTION_REQ_NOT_USED)
    {
        CSR_BT_OBEX_SET_BYTE_AT(temp,0,pMsg->fractionRequest);
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FRACTION_REQUEST, 
            CSR_BT_MAP_TAG_ID_LENGTH_FRACTION_REQUEST, 
            temp);
    }

    pInst->fractionDeliver = 0xFE;   
    CsrPmemFree(pMsg->messageHandle);
	pMsg->messageHandle = NULL;	
    pInst->srmp = pMsg->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*)(MAPC_TEXT_X_BT_MESSAGE), 
                              &messageHandle, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              pInst->srmp,
                              csrBtMapcGetMessageResultHandler, 
                              NULL));
}

CsrUint8 CsrBtMapcGetMessageResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcGetMessageRes *res = msg;

    pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetContinueRequest(pInst->obexClientInst, pInst->srmp));
}

/* !!! Check if this includes srm/srmp header  it shouldn't be !!! */
CsrUint8 CsrBtMapcSetMessageStatusReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                        headerPriorityTable[6]  = { CSR_BT_OBEX_UTIL_NAME_HEADER, 
                                                                CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcSetMessageStatusReq    *pMsg                   = msg;
    CsrUint8                        *appHeader              = NULL;
    CsrUint16                       appHeaderLength         = 0;
    CsrUint8                        temp[1];
    CsrUcs2String                   *messageHandle = CsrUtf82Ucs2ByteString((CsrUint8*)pMsg->messageHandle);

    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->statusIndicator);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_STATUS_INDICATOR, 
        CSR_BT_MAP_TAG_ID_LENGTH_STATUS_INDICATOR, 
        temp);

    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->statusValue);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_STATUS_VALUE, 
        CSR_BT_MAP_TAG_ID_LENGTH_STATUS_VALUE, 
        temp);

    if (pMsg->extendedData)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_EXTENDED_DATA, 
            (CsrUint8) CsrStrLen((const char *) pMsg->extendedData), 
            (CsrUint8*) pMsg->extendedData);
    }

    CsrPmemFree(pMsg->messageHandle);
    CsrPmemFree(pMsg->extendedData);
	pMsg->messageHandle = NULL;
	pMsg->extendedData = NULL;

    return(ObexUtilPutRequest(pInst->obexClientInst, 
                              headerPriorityTable, 0,
                              (CsrUint8*) (MAPC_TEXT_X_BT_MESSAGE_STATUS), 
                              &messageHandle, NULL, 
                              appHeaderLength, &appHeader, 0, 
                              NULL,
                              csrBtMapcFillerBytePutContinueHandler, 
                              csrBtMapcSetMessageStatusResultHandler));
}

CsrUint8 CsrBtMapcPushMessageReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[6] = {CSR_BT_OBEX_UTIL_TYPE_HEADER,
                                       CSR_BT_OBEX_UTIL_NAME_HEADER,
                                       CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
                                       CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                       CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                       CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcPushMessageReq *pMsg = (CsrBtMapcPushMessageReq *) msg;
    CsrUint8 *appHeader = NULL;
    CsrUint16 appHeaderLength = 0;
    CsrUcs2String *folderName = CsrUtf82Ucs2ByteString((CsrUint8*)pMsg->folderName);

    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                       appHeaderLength,
                                                                       CSR_BT_MAP_TAG_ID_TRANSPARENT,
                                                                       CSR_BT_MAP_TAG_ID_LENGTH_TRANSPARENT,
                                                                       &(pMsg->transparent));

    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                       appHeaderLength,
                                                                       CSR_BT_MAP_TAG_ID_RETRY,
                                                                       CSR_BT_MAP_TAG_ID_LENGTH_RETRY,
                                                                       &(pMsg->retry));

    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                       appHeaderLength,
                                                                       CSR_BT_MAP_TAG_ID_CHARSET,
                                                                       CSR_BT_MAP_TAG_ID_LENGTH_CHARSET,
                                                                       &(pMsg->charset));

    if(pMsg->lengthOfObject > 0)
    {
        headerPriorityTable[3] = CSR_BT_OBEX_UTIL_LENGTH_HEADER;
    }

    if (pMsg->conversationId)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                           appHeaderLength,
                                                                           CSR_BT_MAP_TAG_ID_CONVERSATION_ID, 
                                                                           (CsrUint8) CsrStrLen((const CsrCharString *) pMsg->conversationId), 
                                                                           (CsrUint8*) pMsg->conversationId);
    }

    if (pMsg->messageHandle)
    {
        /* if messagehandle is available, then attachment and modifytext will be used */
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                           appHeaderLength,
                                                                           CSR_BT_MAP_TAG_ID_MESSAGE_HANDLE,
                                                                           (CsrUint8) CsrStrLen((const CsrCharString *) pMsg->messageHandle),
                                                                           (CsrUint8*) pMsg->messageHandle);

        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                           appHeaderLength,
                                                                           CSR_BT_MAP_TAG_ID_ATTACHMENT,
                                                                           CSR_BT_MAP_TAG_ID_LENGTH_ATTACHMENT,
                                                                           &(pMsg->attachment));

        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader,
                                                                           appHeaderLength,
                                                                           CSR_BT_MAP_TAG_ID_MODIFY_TEXT,
                                                                           CSR_BT_MAP_TAG_ID_LENGTH_MODIFY_TEXT,
                                                                           &(pMsg->modifyText));
    }

    CsrPmemFree(pMsg->messageHandle);
    CsrPmemFree(pMsg->folderName);
    CsrPmemFree(pMsg->conversationId);
	pMsg->messageHandle = NULL;
	pMsg->folderName = NULL;
	pMsg->conversationId = NULL;

    return (ObexUtilPutRequest(pInst->obexClientInst,
                               headerPriorityTable,
                               pMsg->lengthOfObject,
                               (CsrUint8*) (MAPC_TEXT_X_BT_MESSAGE),
                               &folderName,
                               NULL,
                               appHeaderLength,
                               &appHeader,
                               0,
                               NULL,
                               csrBtMapcPushMessageContinueHandler,
                               csrBtMapcPushMessageResultHandler));
}

CsrUint8 CsrBtMapcPushMessageResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcPushMessageRes *pMsg      = (CsrBtMapcPushMessageRes *) msg;

    return ObexUtilPutContinueRequestEx(pInst->obexClientInst, pMsg->finalFlag, &pMsg->payload, pMsg->payloadLength, TRUE);
}

CsrUint8 CsrBtMapcUpdateInboxReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[6]  = {CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                        CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CSR_UNUSED(msg);

    return(ObexUtilPutRequest(pInst->obexClientInst, headerPriorityTable, 0, (CsrUint8*) (MAPC_TEXT_X_BT_MESSAGE_UPDATE), NULL, NULL, 0, NULL, 0, NULL, csrBtMapcFillerBytePutContinueHandler, csrBtMapcUpdateInboxResultHandler));
}

CsrUint8 CsrBtMapcAbortReqHandler(MapcInstanceData *pInst, void *msg)
{
    CSR_UNUSED(msg);

    return(ObexUtilAbortRequest(pInst->obexClientInst, NULL, csrBtMapcAbortResultHandler));
}


/* OBEX Server part of MAPC */
#define RFCOMM_SERVER_CHANNEL_SERVICE_RECORD_INDEX      24
#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
#define L2CAP_PSM_SERVICE_RECORD_INDEX                  72
#define SUPPORTED_FEATURES_RECORD_INDEX                 78
#else
#define SUPPORTED_FEATURES_RECORD_INDEX                 72
#endif

static const CsrUint8 mapcObexMapcMNSServiceRecord[] =
{
    /* Service class ID list */
    0x09,0x00,0x01,        /* AttrID , ServiceClassIDList */
    0x35,0x03,             /* 3 bytes in total DataElSeq */
    0x19,0x11,0x33,        /* 2 byte UUID, Service class = Message Notification Server */

    /* protocol descriptor list */
    0x09,0x00,0x04,        /* AttrId ProtocolDescriptorList */
    0x35,0x11,             /* 17 bytes in total DataElSeq */
    0x35,0x03,             /* 3 bytes in DataElSeq */
    0x19,0x01,0x00,        /* 2 byte UUID, Protocol = L2CAP */

    0x35,0x05,             /* 5 bytes in DataElSeq */
    0x19,0x00,0x03,        /* 2 byte UUID Protocol = RFCOMM */
    0x08,0x00,             /* 1 byte UINT - server channel template value 0 - to be filled in by app (index:27) */

    0x35,0x03,             /* 3 bytes in DataElSeq */
    0x19, 0x00, 0x08,      /* 2 byte UUID, Protocol = OBEX */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,      /* AttrId = BrowseGroupList */
    0x35, 0x03,            /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,      /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* profile descriptor list */
    0x09,0x00,0x09,        /* AttrId, ProfileDescriptorList */
    0x35,0x08,
    0x35,0x06,             /* 6 bytes in total DataElSeq */
    0x19,0x11,0x34,        /* 2 byte UUID, Service class = Message Access Profile */
    0x09,0x01,0x04,        /* 2 byte uint, version = 0x0104 */
    
    /* service name */
    0x09,0x01,0x00,        /* AttrId - Service Name */
    0x25,0x0C,             /* 12 bytes string */
    'M','A','P',' ','M','N','S','-','n','a','m','e',

#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    /* OBEX-over-L2CAP */
    0x09,0x02,0x00,        /* 16 bit attribute */
    0x09,0x00,0x00,        /* 16 bit L2CAP PSM - filled in by obex-utils */
#endif

    /* Supported Features */
    0x09,0x03,0x17,        /* AttrId - Supported Features */
    0x0a,                   /* 4 byte UINT */
    0x00,0x00,0x00,0x00
};

void csrBtMapcRegisterMnsRecord(MapcInstanceData *pInst)
{
    CsrUint8 *record;
    CsrUint16 recordLength = sizeof(mapcObexMapcMNSServiceRecord);
    
    /* Prepare to register Service Record */
    record = CsrPmemAlloc(recordLength);
    SynMemCpyS(record, recordLength, mapcObexMapcMNSServiceRecord, recordLength);

    record[RFCOMM_SERVER_CHANNEL_SERVICE_RECORD_INDEX] = pInst->mnsServerChannel;

#ifdef CSR_BT_INSTALL_OBEX_GOEP_20
    record[L2CAP_PSM_SERVICE_RECORD_INDEX]     = (CsrUint8)((pInst->mnsPsm) >> 8);
    record[L2CAP_PSM_SERVICE_RECORD_INDEX + 1] = (CsrUint8)((pInst->mnsPsm) & 0x00FF);
#endif

    record[SUPPORTED_FEATURES_RECORD_INDEX]     = (CsrUint8)(CSR_BT_MAPC_SUPPORTED_FEATURES >> 24);
    record[SUPPORTED_FEATURES_RECORD_INDEX + 1] = (CsrUint8)(CSR_BT_MAPC_SUPPORTED_FEATURES >> 16);
    record[SUPPORTED_FEATURES_RECORD_INDEX + 2] = (CsrUint8)(CSR_BT_MAPC_SUPPORTED_FEATURES >> 8);
    record[SUPPORTED_FEATURES_RECORD_INDEX + 3] = (CsrUint8)(CSR_BT_MAPC_SUPPORTED_FEATURES);

    CsrBtCmSdsRegisterReqSend(CSR_BT_MAPC_IFACEQUEUE,
                              record,
                              recordLength,
                              MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID);
}


static CsrUint8 csrBtMapcGetNewNotiInstanceId(MapcInstanceData *instData)
{ /* Use instance IDs CSR_BT_MAPC_CLIENT_INST_ID+1 to 0xFE */
    CsrUint8 currInstId = CSR_BT_MAPC_CLIENT_INST_ID+1;
    MapcNotiService_t *srvInst = NOTI_SERVICE_LIST_GET_FIRST(&instData->notiServiceList);

    while (srvInst)
    {
        if (srvInst->obexInstId == currInstId)
        {/* Instance ID already in use - skip to next number and restart */
            currInstId++;
            srvInst = NOTI_SERVICE_LIST_GET_FIRST(&instData->notiServiceList);
        }
        else
        {
            srvInst = srvInst->next;
        }
    }
    /* Lowest possible instance ID found */
    return currInstId;
}

static CsrBool csrBtMapcCheckTargetHeader(CsrUint8   *target, 
                                          CsrUint16  targetHeaderLength)
{
    if (target && (targetHeaderLength == MAPC_TARGET_LENGTH))
    {
        CsrUint8 tempTarget[MAPC_TARGET_LENGTH] = MAPC_MNS_TARGET;
    
        if (!CsrMemCmp(target, tempTarget, MAPC_TARGET_LENGTH))
        {
            return (TRUE);
        }
    }
    return (FALSE);       
}

#ifdef INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS
CsrUint8 CsrBtMapcSecurityInReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcSecurityInReq *pMsg    = (CsrBtMapcSecurityInReq*)msg;

    csrBtMapcSecurityInCfmSend(pMsg->appHandle, pInst->mapcInstanceId,
                            CsrBtScSetSecInLevel(&pInst->secIncoming, pMsg->secLevel,
                                            CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_INCOMING,
                                            CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_INCOMING,
                                            CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                                            CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER),
                            CSR_BT_SUPPLIER_OBEX_PROFILES);
    
    /* ObexUtilServChangeIncomingSecurity(pInst->obexServerInst, pInst->secIncoming); */
    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}

CsrUint8 CsrBtMapcSecurityOutReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcSecurityOutReq *pMsg = (CsrBtMapcSecurityOutReq*)msg;

    csrBtMapcSecurityOutCfmSend(pMsg->appHandle, pInst->mapcInstanceId,
                            CsrBtScSetSecOutLevel(&pInst->secOutgoing, pMsg->secLevel,
                                             CSR_BT_OBEX_MESSAGE_ACCESS_MANDATORY_SECURITY_OUTGOING,
                                             CSR_BT_OBEX_MESSAGE_ACCESS_DEFAULT_SECURITY_OUTGOING,
                                            CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                                            CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER),
                            CSR_BT_SUPPLIER_OBEX_PROFILES);

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}
#endif /* INSTALL_MAPC_CUSTOM_SECURITY_SETTINGS */

CsrUint8 CsrBtMapcRegisterQIDReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcRegisterQidReq *prim;
    CsrBtMapcInstancePool *ptr;
    CsrBtMapcInstancePool *prev;

    prim = (CsrBtMapcRegisterQidReq *) msg;

    if (pInst->mapcInstanceId == CSR_BT_MAPC_IFACEQUEUE)
    {
        ptr = pInst->mapcInstances;
        prev = NULL;

        while((ptr) && (ptr->numberInPool == MAPC_INSTANCES_POOL_SIZE))
        {
            prev = ptr;
            ptr = ptr->next;
        }

        if (ptr)
        {
            /* Do nothing */
        }
        else
        {
            ptr = CsrPmemZalloc(sizeof(CsrBtMapcInstancePool));
            ptr->numberInPool = 0;
            ptr->next=NULL;

            if (prev)
            {
                prev->next = ptr;
            }
            else
            {
                pInst->mapcInstances = ptr;
            }
        }

        if(ptr->numberInPool < MAPC_INSTANCES_POOL_SIZE)
        {
            ptr->phandles[ptr->numberInPool++] = prim->mapcInstanceId;
            pInst->numberOfMapcInstances++;
        }
        else
        {
            return CSR_BT_OBEX_UTIL_STATUS_EXCEPTION;
        }
    }
    else
    {
        CsrGeneralException(CsrBtMapcLth,
                            0,
                            CSR_BT_MAPC_PRIM,
                            CSR_BT_MAPC_REGISTER_QID_REQ,
                            0,
                            "Task not MAPC-manager");
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}


static void CsrBtMapcResetNotiService(MapcNotiService_t* notiService)
{
    CsrMemSet(&notiService->notiInstanceList, 0, sizeof(CsrCmnList_t));
    notiService->obexInstId = MAPC_NOTI_CHANNEL_REGISTER_CONTEXT_ID;
    notiService->obexInst = NULL;
    notiService->connId = CSR_BT_CONN_ID_INVALID;
    notiService->tempMasInstanceId = MAS_INVALID_INSTANCE;
    notiService->mapcInst = NULL;
}

CsrUint8 CsrBtMapcAddNotiReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcAddNotiReq *prim = (CsrBtMapcAddNotiReq *) msg;
    MapcNotiService_t *notiService = \
        NOTI_SERVICE_LIST_GET_ADDR(&pInst->notiServiceList, &prim->deviceAddr);
    notiInstance_t *notiInst;
    CsrBtMapcAddNotiCfm *cfm = NULL;

    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "CsrBtMapcAddNotiReqHandler"));

    if (!notiService)
    {
        notiService = NOTI_SERVICE_LIST_ADD_FIRST(&pInst->notiServiceList);

        CsrBtMapcResetNotiService(notiService);

        notiService->deviceAddr = prim->deviceAddr;
        notiService->putOp = FALSE;
        notiService->security = prim->security;
        /* Let the authorisation be removed for the notification */
        notiService->security &= ~SECL_IN_AUTHORISATION;
        notiService->maxFrameSize = prim->maxFrameSize;
        notiService->mapcInst = pInst;
    }

    /* Make sure there is no such instance already exist */ 
    notiInst = NOTI_INSTANCE_LIST_GET_INSTID(&notiService->notiInstanceList, prim->masInstanceId);
    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "instance found = %p, id = %d", notiInst, prim->masInstanceId));
    if (!notiInst)
    {
        notiInst = NOTI_INSTANCE_LIST_ADD_FIRST(&notiService->notiInstanceList);
        notiInst->masInstanceId = prim->masInstanceId;
        notiInst->mapHandle = prim->mapHandle;
        notiInst->appHandle = prim->appHandle;
        notiInst->awaitingDisc = FALSE;

        /* Do the init for the 1st instance alone */
        if (notiService->notiInstanceList.count == 1)
        {
            notiService->obexInstId = csrBtMapcGetNewNotiInstanceId(pInst);
            notiService->obexInst = ObexUtilInit(CSR_BT_MAPC_IFACEQUEUE, notiService, notiService->obexInstId);
            ObexUtilServAddAddress(notiService->obexInst, prim->deviceAddr);
            ObexUtilServActivateStart(notiService->obexInst,
                                      notiService->security,
                                      CSR_BT_OBEX_MESSAGE_NOTIFICATION_SERVER_UUID,
                                      CSR_BT_MAPC_LP_SUPERVISION_TIMEOUT,
                                      pInst->mnsServerChannel,
                                      pInst->mnsPsm,
                                      0,
                                      0xFFFE,
                                      0xFFFE,
                                      CSR_BT_OBEX_UTIL_INVALID_SDP_RECORD_INDEX,
                                      NULL,
                                      CSR_BT_OBJECT_TRANSFER_COD,
                                      csrBtMapcNotiServiceConnectIndHandler,
                                      csrBtMapcNotiServicePutIndHandler,
                                      NULL,
                                      NULL,
                                      csrBtMapcNotiServiceAbortIndHandler,
                                      csrBtMapcNotiServiceDisconnectIndHandler,
                                      NULL,
                                      csrBtMapcNotiServiceActivateCfmHandler,
                                      NULL,
                                      prim->maxFrameSize,
                                      prim->windowSize,
                                      TRUE,
                                      CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                             CSR_BT_MAP_DEFAULT_ENC_KEY_SIZE_VAL));
        }

        cfm = CsrPmemAlloc(sizeof(*cfm));
        cfm->type = CSR_BT_MAPC_ADD_NOTI_CFM;
        cfm->resultCode = CSR_BT_OBEX_SUCCESS_RESPONSE_CODE;
        CsrMsgTransport(prim->mapHandle, CSR_BT_MAPC_PRIM, cfm);
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtMapcLth, 0, "Notification instance already exist"));
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}

static void CsrBtObexMapcNotiServiceDeactivateCfmHandler(void *instData, 
                                              CsrUint8 serverChannel,
                                              psm_t         psm)
{
    MapcNotiService_t *notiService = instData;
    MapcInstanceData  *mapcInst = notiService->mapcInst;

    ObexUtilDeinit(&notiService->obexInst);
    /* There shouldn't be any pending noti instances at this moment. 
        So this would just be a sanity */
    CSR_LOG_TEXT_CONDITIONAL_ERROR(notiService->notiInstanceList.count,
                                   (CsrBtMapcLth,
                                   0,
                                   "Assertion \"notiService->notiInstanceList.count\" failed at %s:%u",
                                   __FILE__,
                                   __LINE__));

    CsrCmnListDeinit(&notiService->notiInstanceList);

    NOTI_SERVICE_LIST_REMOVE(&mapcInst->notiServiceList, notiService);

    CSR_UNUSED(psm);
    CSR_UNUSED(serverChannel);
}

CsrUint8 CsrBtMapcRemoveNotiReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcRemoveNotiReq *prim = (CsrBtMapcRemoveNotiReq *) msg;
    MapcNotiService_t *notiService = \
        NOTI_SERVICE_LIST_GET_ADDR(&pInst->notiServiceList, &prim->deviceAddr);
    notiInstance_t *notiInst;
    CsrBtDeviceAddr zeroBdAddr = { 0,0,0 };
    CsrBtMapcRemoveNotiCfm *cfm;

    if (notiService && ((notiInst = NOTI_INSTANCE_LIST_GET_INSTID(&notiService->notiInstanceList, prim->masInstanceId)) != NULL))
    {
        if (notiService->notiInstanceList.count == 1)
        {
            if (prim->forceDisc)
            {
                /* Invalidate the address in case of cross the add request does not find 
                    the same entry */
                notiService->deviceAddr = zeroBdAddr;
                ObexUtilServDeactivateStart(notiService->obexInst,
                                            pInst->mnsServerChannel,
                                            pInst->mnsPsm,
                                            CsrBtObexMapcNotiServiceDeactivateCfmHandler);
                NOTI_INSTANCE_LIST_REMOVE(&notiService->notiInstanceList, notiInst);
            }
            else
            {
                /* This is ensure we don't send NOTI OFF IND on receiving MNS disconnection */
                notiInst->awaitingDisc = TRUE;
            }
        }
        else
        {
            /* Let other instance use the service. */
            NOTI_INSTANCE_LIST_REMOVE(&notiService->notiInstanceList, notiInst);
        }

        cfm = CsrPmemAlloc(sizeof(*cfm));
        cfm->type = CSR_BT_MAPC_REMOVE_NOTI_CFM;
        cfm->resultCode = CSR_BT_OBEX_SUCCESS_RESPONSE_CODE;
        CsrMsgTransport(prim->mapHandle, CSR_BT_MAPC_PRIM, cfm);
    }
    else
    {
        CSR_LOG_TEXT_WARNING((CsrBtMapcLth, 0, "No service or instance found"));
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}


static CsrUint8 csrBtMapcNotiServerConnectResponseSend(MapcNotiService_t *notiService,
                                                   CsrBtObexResponseCode        responseCode, 
                                                   CsrUcs2String                *description)
{
    CsrUint8  numOfHeaders           = 0;  
    CsrUint8  headerPriorityTable[1] = {CSR_BT_OBEX_UTIL_DESCRIPTION_HEADER};
    CsrUint16 whoHeaderLength        = MAPC_TARGET_LENGTH;
    CsrUint8  *who                   = (CsrUint8 *) CsrPmemAlloc(whoHeaderLength);
    CsrUint8  tempTarget[MAPC_TARGET_LENGTH] = MAPC_MNS_TARGET;
    responseCode                     = (CsrBtObexResponseCode)(responseCode | CSR_BT_OBEX_FINAL_BIT_MASK);
    SynMemCpyS(who, whoHeaderLength, tempTarget, whoHeaderLength);

    if (description)
    {
        numOfHeaders                 = 1;
    }
    
    return (ObexUtilServConnectResponse(notiService->obexInst, 
                                        responseCode, 
                                        whoHeaderLength, 
                                        &who, 
                                        (CsrUint32) notiService->obexInstId, 
                                        numOfHeaders, 
                                        headerPriorityTable, 
                                        NULL, 
                                        &description, 
                                        0, 
                                        NULL,
                                        FALSE));
}

static void csrBtMapcNotiServiceConnectIndHandler(void   *instData,
                                             CsrBtDeviceAddr         deviceAddr,
                                             CsrBtConnId             cid,
                                             CsrUint16               maxPeerObexPacketLength,
                                             CsrUint16               obexResponsePacketLength,
                                             CsrUint32               length,
                                             CsrUint32               count,
                                             CsrUint16               targetHeaderLength,
                                             CsrUint8                *target,
                                             CsrBool                 authenticated,
                                             CsrBool                 challenged,
                                             CsrUint16               obexPacketLength,
                                             CsrUint8                *obexPacket)
{
    MapcNotiService_t *notiService = (MapcNotiService_t *)instData;

    if (csrBtMapcCheckTargetHeader(target, targetHeaderLength))
    {   
        notiService->connId = cid;
        csrBtMapcNotiServerConnectResponseSend(notiService, 
                                            CSR_BT_OBEX_SUCCESS_RESPONSE_CODE, 
                                            NULL);
    }
    else
    {   
        CsrUcs2String *description = CsrUtf82Ucs2ByteString((CsrUint8 *) "Target missing");
        csrBtMapcNotiServerConnectResponseSend(notiService, CSR_BT_OBEX_BAD_REQUEST_RESPONSE_CODE,
                                           description);
    }
    CSR_UNUSED(length);
    CSR_UNUSED(count);
    CSR_UNUSED(authenticated);
    CSR_UNUSED(obexPacketLength);
    CSR_UNUSED(obexResponsePacketLength);
    CSR_UNUSED(challenged);
    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(maxPeerObexPacketLength);
    
    CsrPmemFree(obexPacket);
    CsrPmemFree(target);
    
}

static void csrBtMapcNotiServiceEventHandler(MapcNotiService_t *notiService,
                                          notiInstance_t *notiInst,
                                          CsrUint16   bodyOffset,
                                          CsrUint16   bodyLength,
                                          CsrUint8   *payload,
                                          CsrUint16   payloadLength)
{
    CsrBtMapcEventNotificationInd *pMsg = (CsrBtMapcEventNotificationInd *)CsrPmemAlloc(sizeof(CsrBtMapcEventNotificationInd));

    pMsg->type              = CSR_BT_MAPC_EVENT_NOTIFICATION_IND;
    pMsg->instanceId        = notiInst->mapHandle;
    pMsg->finalFlag         = notiService->putFinalFlag;
    pMsg->bodyOffset        = bodyOffset;  
    pMsg->bodyLength        = bodyLength; 
    pMsg->payload           = payload;
    pMsg->payloadLength     = payloadLength;
    
    CsrBtMapcMessagePut(notiInst->appHandle, pMsg);
}

static CsrBool csrBtGetMasInstanceTagFromAppHeader(CsrUint8 *pObexPacket, CsrUint8 *masInstanceId)
{
    CsrBool   found = FALSE;
    CsrUint16 nLen;
    CsrUint16 nIndex = CsrBtObexHeaderIndex(CSR_BT_OBEX_REQUEST,
                                            pObexPacket,
                                            CSR_BT_OBEX_APPLICATION_PARAMETERS_HEADER,
                                            &nLen);
    *masInstanceId = 0xff;

    if (nIndex && nLen > CSR_BT_OBEX_HEADER_LENGTH)
    {
        nLen += nIndex - 1;
        nIndex += CSR_BT_OBEX_HEADER_LENGTH;

        while ((nIndex < nLen) && !found)
        {
            CsrUint8 *src;

            src = pObexPacket + nIndex + CSR_BT_OBEX_MAPC_TAG_SIZE;

            switch (pObexPacket[nIndex])
            {
                case CSR_BT_OBEX_MAPC_TAG_INSTANCE_ID:
                {
                    *masInstanceId = *src;
                    found = TRUE;
                    break;
                }
                default:
                {
                    break;
                }
            }

            nIndex += pObexPacket[nIndex + 1]
                      + CSR_BT_OBEX_MAPC_TAG_SIZE;
        }
    }

    return found;
}

static void csrBtMapcNotiServicePutIndHandler
                (
                    void      *instData,
                    CsrUint16  obexResponsePacketLength,
                    CsrBool    authenticated,
                    CsrBool    finalBitSet,
                    CsrUint16  bodyLength,
                    CsrUint16  bodyOffset,
                    CsrUint16  obexPacketLength,
                    CsrUint8  *pObexPacket
                )
{ /* The first Put Request packet is received */
    MapcNotiService_t *notiService = (MapcNotiService_t *)instData;
    CsrUint8           masInstanceId = MAS_INVALID_INSTANCE;
    CsrUint16          eobodyLen;
    CsrBool            masInstanceFound = TRUE;
    notiInstance_t    *notiInst = NULL;

    if (csrBtGetMasInstanceTagFromAppHeader(pObexPacket, &masInstanceId) != FALSE)
    {
        /* This is the 1st pkt received for this operation */
        notiService->tempMasInstanceId = masInstanceId;
        notiService->putOp = TRUE;
    }
    else if (notiService->putOp)
    {
        /* Use the id from temp id since the following pkts would not hold the inst id */
        masInstanceId = notiService->tempMasInstanceId;
    }
    else
    {
        /* Could not find the instance */
        masInstanceFound = FALSE;
        notiService->tempMasInstanceId = MAS_INVALID_INSTANCE;
    }

    CSR_LOG_TEXT_INFO((CsrBtMapcLth, 0, "instance found = %d, id = %d", masInstanceFound, masInstanceId));

    if (masInstanceFound && ((notiInst = NOTI_INSTANCE_LIST_GET_INSTID(&notiService->notiInstanceList, masInstanceId)) != NULL))
    {
        notiService->putFinalFlag = finalBitSet;

        csrBtMapcNotiServiceEventHandler(notiService,
                                      notiInst,
                                      bodyOffset,
                                      bodyLength,
                                      pObexPacket,
                                      obexPacketLength);

        /* Find the EOB hdr to know if this is the end of the operation */
        if (CsrBtObexHeaderIndex(CSR_BT_OBEX_PUT_OPCODE, pObexPacket, CSR_BT_OBEX_END_OF_BODY_HEADER, &eobodyLen))
        {
            notiService->putOp = FALSE;
        }
    }
    else
    {
        /* Could not find associated instance probably wrong instance number sent by remote device! */
        CsrPmemFree(pObexPacket);
        CSR_LOG_TEXT_ERROR((CsrBtMapcLth, 0, "Could not find associated instance"));
    }

    CSR_UNUSED(authenticated);
    CSR_UNUSED(obexResponsePacketLength);
}

static void csrBtMapcNotiServiceAbortIndHandler
                (
                    void      *instData,
                    CsrUint16  descriptionOffset,
                    CsrUint16  descriptionLength,
                    CsrUint16  obexPacketLength,
                    CsrUint8  *obexPacket
                )
{
    MapcNotiService_t *notiService = (MapcNotiService_t *)instData;
    CsrUint8           masInstanceId = MAS_INVALID_INSTANCE;
    notiInstance_t    *notiInst = NULL;

    CsrBtMapcEventNotificationAbortInd *pMsg;    

    if (notiService->putOp)
    {
        /* Use the id from temp id since the pkts following 1st PUT including Abort 
            would not hold the inst id */
        masInstanceId = notiService->tempMasInstanceId;
        notiService->putOp = FALSE;
    }

    notiInst = NOTI_INSTANCE_LIST_GET_INSTID(&notiService->notiInstanceList, masInstanceId);
    if (notiInst)
    {
        if (descriptionOffset == 0)
        { /* An OBEX Descriptor header is not included, CsrPmemFree obexPacket         */
            CsrPmemFree(obexPacket);
            obexPacket          = NULL;
            obexPacketLength    = 0;     
        }

        pMsg = (CsrBtMapcEventNotificationAbortInd *)CsrPmemAlloc(sizeof(CsrBtMapcEventNotificationAbortInd));
        pMsg->type = CSR_BT_MAPC_EVENT_NOTIFICATION_ABORT_IND;
        pMsg->descriptionOffset   = descriptionOffset;
        pMsg->descriptionLength   = descriptionLength;
        pMsg->payload             = obexPacket;
        pMsg->payloadLength       = obexPacketLength;
        CsrBtMapcMessagePut(notiInst->appHandle, pMsg);
    }
    else
    {
        CsrPmemFree(obexPacket);
        CSR_LOG_TEXT_ERROR((CsrBtMapcLth, 0, "Could not find associated instance"));
    }
}

/* These functions are handled by the manager task */
static CsrUint8 csrBtMapcObexServerResponseSend(MapcNotiService_t *notiService, CsrUint8 responseCode, CsrUint8 srmp)
{
    return (ObexUtilServPutResponse(notiService->obexInst,
                                    (CsrBtObexResponseCode) responseCode,
                                    srmp)); 
}

static MapcNotiService_t *CsrBtMapcFindNotiServFromMasTaskId(MapcInstanceData *pInst, CsrSchedQid mapTaskId)
{
    MapcNotiService_t *notiService;
    notiInstance_t *notiInst;

    notiService = (MapcNotiService_t *)CsrCmnListElementGetFirst(&pInst->notiServiceList);

    for (;notiService;notiService = notiService->next)
    {
        notiInst = (notiInstance_t *)CsrCmnListElementGetFirst(&notiService->notiInstanceList);

        for (;notiInst;notiInst = notiInst->next)
        {
            if (notiInst->mapHandle == mapTaskId)
            {
                return notiService;
            }
        }
    }
    
    return notiService;
}

CsrUint8 CsrBtMapcEventNotificationResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcEventNotificationRes *pMsg = msg;
    CsrUint8 result;
    MapcNotiService_t *notiService;

    pInst->srmp = pMsg->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    notiService = CsrBtMapcFindNotiServFromMasTaskId(pInst, pMsg->instanceId);
    if (!notiService)
    {
        /* This should never happen return exception as we cannot tell
            anything to application */
        return CSR_BT_OBEX_UTIL_STATUS_EXCEPTION;
    }

    if (notiService->putFinalFlag)
    {
        if (pMsg->response != CSR_BT_OBEX_SUCCESS_RESPONSE_CODE && pMsg->response != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
        {
            result = pMsg->response;
        }
        else
        {
            result = CSR_BT_OBEX_SUCCESS_RESPONSE_CODE;
        }
    }
    else
    {
        if (pMsg->response != CSR_BT_OBEX_SUCCESS_RESPONSE_CODE && pMsg->response != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
        {
            result = pMsg->response;
        }
        else
        {
            result = CSR_BT_OBEX_CONTINUE_RESPONSE_CODE;
        }
    }

    switch (result & ~CSR_BT_OBEX_FINAL_BIT_MASK)
    {
        case CSR_BT_OBEX_SUCCESS_RESPONSE_CODE:
            if (notiService->putFinalFlag)
            {
                csrBtMapcObexServerResponseSend(notiService,CSR_BT_OBEX_SUCCESS_RESPONSE_CODE | CSR_BT_OBEX_FINAL_BIT_MASK, pInst->srmp);
            }
            else
            {
                csrBtMapcObexServerResponseSend(notiService,CSR_BT_OBEX_CONTINUE_RESPONSE_CODE | CSR_BT_OBEX_FINAL_BIT_MASK, pInst->srmp);
            }
            break;
        case CSR_BT_OBEX_CONTINUE_RESPONSE_CODE:
            csrBtMapcObexServerResponseSend(notiService,CSR_BT_OBEX_CONTINUE_RESPONSE_CODE | CSR_BT_OBEX_FINAL_BIT_MASK, pInst->srmp);
            break;
        default:
            csrBtMapcObexServerResponseSend(notiService, (CsrUint8)(result | (CsrUint8)CSR_BT_OBEX_FINAL_BIT_MASK), pInst->srmp);
            break;
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}

static CsrBool csrBtMapcObexServerSendNotificationOffInd(CsrCmnListElm_t *elem, void *data)
{
    notiInstance_t *notiInst = (notiInstance_t *)elem;

    /* Do not send if this is part of the notification registration OFF 
        which client had initiated */
    if (!notiInst->awaitingDisc)
    {
        CsrBtMapcNotificationRegistrationOffInd *pMsg = CsrPmemAlloc(sizeof(*pMsg));

        pMsg->type = CSR_BT_MAPC_NOTIFICATION_REGISTRATION_OFF_IND;
        pMsg->instanceId = notiInst->mapHandle;
        CsrBtMapcMessagePut(notiInst->appHandle, pMsg);
    }
    CSR_UNUSED(data);

    return TRUE;
}

static void csrBtMapcNotiServiceDisconnectIndHandler
                (
                    void            *instData,
                    CsrBtDeviceAddr  deviceAddr,
                    CsrBtReasonCode  reasonCode,
                    CsrBtSupplier    reasonSupplier,
                    CsrUint16        obexPacketLength,
                    CsrUint8        *obexPacket
                )
{
    MapcNotiService_t *notiService = (MapcNotiService_t*)instData;

    CsrBtMapcServiceCleanupSend(notiService->obexInstId);

    CSR_UNUSED(instData);
    CSR_UNUSED(deviceAddr);
    CSR_UNUSED(obexPacketLength);
    CSR_UNUSED(reasonCode);
    CSR_UNUSED(reasonSupplier);
    CsrPmemFree(obexPacket);
}

static void csrBtMapcNotiServiceActivateCfmHandler
                (
                    void            *instData,
                    CsrUint8         serverChannel,
                    psm_t            psm,
                    CsrBtResultCode  resultCode,
                    CsrBtSupplier    resultSupplier
                )
{
    MapcNotiService_t *notiService = (MapcNotiService_t*)instData;
    
    if (resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS && 
        resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        ObexUtilServConnectableStart(notiService->obexInst, 
                                     CSR_BT_MAPC_PROFILE_DEFAULT_MTU_SIZE,
                                     notiService->maxFrameSize);
    }
    else
    {
        CSR_LOG_TEXT_ERROR((CsrBtMapcLth,
                           0,
                           "Unexpected result code = %u and result supplier = %u",
                           resultCode,
                           resultSupplier));
    }
    CSR_UNUSED(psm);
    CSR_UNUSED(serverChannel);
}

CsrUint8 CsrBtMapcAddNotiCfmHandler(MapcInstanceData *pInst, void *msg)
{
    csrBtObexMapcNotificationPutReqSend(pInst);
    CSR_UNUSED(msg);
    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}

CsrUint8 CsrBtMapcRemoveNotiCfmHandler(MapcInstanceData *pInst, void *msg)
{
    /* Nothing to be done */
    CSR_UNUSED(pInst);
    CSR_UNUSED(msg);
    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}

static void csrBtMapcGetMasInstanceInformationResultHandler(void *instData,
                                                    CsrUint8    responseCode,
                                                    CsrBool     bodyHeader,
                                                    CsrUint16   bodyLength,
                                                    CsrUint16   bodyOffset,
                                                    CsrUint8  *obexPacket,
                                                    CsrUint16   obexPacketLength)
{
        MapcInstanceData *pInst = instData;
        CsrBtMapcGetMasInstanceInformationCfm *pMsg = CsrPmemAlloc(sizeof(*pMsg));
        CsrUint8 tagLength;
        CsrUint8 *tagValue;

        pMsg->ownerUci = NULL;
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_OWNER_UCI, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                pMsg->ownerUci = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pMsg->ownerUci, (tagLength + 1), tagValue, tagLength);
                pMsg->ownerUci[tagLength] = 0;
            }
        }

        pMsg->type                    = CSR_BT_MAPC_GET_MAS_INSTANCE_INFORMATION_CFM;
        pMsg->instanceId              = pInst->mapcInstanceId;
        pMsg->result                  = responseCode;
        pMsg->bodyLength              = bodyLength; 
        pMsg->bodyOffset              = bodyOffset;  
        pMsg->payload                 = obexPacket;
        pMsg->payloadLength           = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);

        CSR_UNUSED(bodyHeader);
}

CsrUint8 CsrBtMapcGetMasInstanceInformationReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[5]  = {CSR_BT_OBEX_UTIL_TYPE_HEADER, CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER};
    CsrUint8 *appHeader = NULL;
    CsrUint16 appHeaderLength = 0;
    CsrUint8 temp[CSR_BT_MAP_TAG_ID_LENGTH_MAS_INSTANCE_ID];

    CSR_UNUSED(msg);

    /* Build applications parameter header */
    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pInst->masInstanceId);
    
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_MAS_INSTANCE_ID, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_MAS_INSTANCE_ID, 
                                                                        temp);

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*)(MAPC_TEXT_X_BT_MAS_INSTANCE_INFORMATION), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              CSR_BT_OBEX_SRMP_INVALID,
                              csrBtMapcGetMasInstanceInformationResultHandler, 
                              NULL));
}

CsrUint8 CsrBtMapcServiceCleanupHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcServiceCleanup *prim = msg;
    MapcNotiService_t *notiService = \
        NOTI_SERVICE_LIST_GET_INSTID(&pInst->notiServiceList, prim->obexInstId);

    if (notiService)
    {
        CsrCmnListIterateAllowRemove(&notiService->notiInstanceList, 
            csrBtMapcObexServerSendNotificationOffInd, NULL);

        ObexUtilDeinit(&notiService->obexInst);

        NOTI_SERVICE_LIST_REMOVE(&pInst->notiServiceList, notiService);
    }

    return CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;
}

/****************************************************************************/
/*********************** Get Onwer Status ***********************************/ 
/****************************************************************************/
static void csrBtMapcGetOwnerStatusResultHandler(void       *instData,
                                                 CsrUint8   responseCode,
                                                 CsrBool    bodyHeader,
                                                 CsrUint16  bodyLength,
                                                 CsrUint16  bodyOffset,
                                                 CsrUint8   *obexPacket,
                                                 CsrUint16  obexPacketLength);

CsrUint8 CsrBtMapcGetOwnerStatusReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                    headerPriorityTable[5]  = { CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                            CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
                                                            CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                            CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                            CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcGetOwnerStatusReq  *pMsg                   = msg;
    CsrUint8                    *appHeader              = NULL;
    CsrUint16                   appHeaderLength         = 0;

    if (pMsg->conversationId)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_CONVERSATION_ID, 
            (CsrUint8) CsrStrLen((const char *)pMsg->conversationId), 
            (CsrUint8*) pMsg->conversationId);
    }
    else
    {
        headerPriorityTable[1] = CSR_BT_OBEX_UTIL_UNDEFINE_HEADER;
    }
    CsrPmemFree(pInst->presenceText);
    CsrPmemFree(pInst->lastActivity);

    pInst->presenceAvailability = CSR_BT_MAP_PRESENCE_UNKNOWN;
    pInst->presenceText = NULL;
    pInst->lastActivity = NULL;
    pInst->chatState = CSR_BT_MAP_CHAT_STATE_UNKNOWN;

    CsrPmemFree(pMsg->conversationId);
	pMsg->conversationId = NULL;

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*) ("x-bt/ownerStatus"), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              CSR_BT_OBEX_SRMP_INVALID,
                              csrBtMapcGetOwnerStatusResultHandler, 
                              NULL));
}

static void csrBtMapcGetOwnerStatusResultHandler(void       *instData,
                                                 CsrUint8   responseCode,
                                                 CsrBool    bodyHeader,
                                                 CsrUint16  bodyLength,
                                                 CsrUint16  bodyOffset,
                                                 CsrUint8   *obexPacket,
                                                 CsrUint16  obexPacketLength)
{
    MapcInstanceData *pInst = instData;
    CsrUint8 tagLength;
    CsrUint8 *tagValue;

    if (pInst->presenceAvailability == CSR_BT_MAP_PRESENCE_UNKNOWN)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_PRESENCE_AVAILABILITY, &tagLength, &tagValue))
        {
            if (tagLength == CSR_BT_MAP_TAG_ID_LENGTH_PRESENCE && tagValue)
            {
                pInst->presenceAvailability = CSR_BT_OBEX_GET_BYTE_AT(tagValue, 0);
            }
        }
    }

    if (!pInst->presenceText)
    { 
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_PRESENCE_TEXT, &tagLength, &tagValue))
        {
            if (tagLength != 0 && tagValue)
            {
                pInst->presenceText = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->presenceText, (tagLength + 1), tagValue, tagLength);
                pInst->presenceText[tagLength] = 0;
            }
        }
    }

    if (!pInst->lastActivity)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_LAST_ACTIVTY, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                pInst->lastActivity = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->lastActivity, (tagLength + 1), tagValue, tagLength);
                pInst->lastActivity[tagLength] = 0;
            }
        }
    }

    if (pInst->chatState == CSR_BT_MAP_CHAT_STATE_UNKNOWN)
    {
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_CHAT_STATE, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_CHAT_STATE && tagValue)
            {
                pInst->chatState = CSR_BT_OBEX_GET_BYTE_AT(tagValue, 0);
            }
        }
    }

    if (responseCode != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
    {       
        CsrBtMapcGetOwnerStatusCfm *pMsg = CsrPmemAlloc(sizeof(*pMsg));

        pMsg->type                      = CSR_BT_MAPC_GET_OWNER_STATUS_CFM;
        pMsg->instanceId                = pInst->mapcInstanceId;
        pMsg->result                    = responseCode;
        pMsg->presenceAvailability      = pInst->presenceAvailability;
        pMsg->presenceText              = pInst->presenceText;
        pMsg->lastActivity              = pInst->lastActivity;
        pMsg->chatState                 = pInst->chatState;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);

        pInst->presenceText              = NULL;
        pInst->lastActivity              = NULL;
    }
    else
    {
        ObexUtilGetContinueRequest(pInst->obexClientInst, CSR_BT_OBEX_SRMP_INVALID);
    }
    CsrPmemFree(obexPacket);

    CSR_UNUSED(bodyHeader);
    CSR_UNUSED(bodyLength);
    CSR_UNUSED(bodyOffset);
}

/****************************************************************************/
/*********************** Set Onwer Status ***********************************/ 
/****************************************************************************/
static void csrBtMapcSetOwnerStatusResultHandler(void       *instData,
                                                 CsrUint8   responseCode,
                                                 CsrUint8   *obexPacket,
                                                 CsrUint16  obexPacketLength);

CsrUint8 CsrBtMapcSetOwnerStatusReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                        headerPriorityTable[6]  = { CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcSetOwnerStatusReq      *pMsg                   = msg;
    CsrUint8                        *appHeader              = NULL;
    CsrUint16                       appHeaderLength         = 0;
    CsrUint8                        temp[1];

    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->presenceAvailability);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_PRESENCE_AVAILABILITY, 
        CSR_BT_MAP_TAG_ID_LENGTH_PRESENCE, 
        temp);

    if (pMsg->presenceText)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_PRESENCE_TEXT, 
            (CsrUint8) CsrStrLen((const char *) pMsg->presenceText), 
            (CsrUint8*) pMsg->presenceText);
    }

    if (pMsg->lastActivity)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_LAST_ACTIVTY, 
            (CsrUint8) CsrStrLen((const char *) pMsg->lastActivity), 
            (CsrUint8*) pMsg->lastActivity);
    }

    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->chatState);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
        &appHeader, 
        appHeaderLength,
        CSR_BT_MAP_TAG_ID_CHAT_STATE, 
        CSR_BT_MAP_TAG_ID_LENGTH_CHAT_STATE, 
        temp);

    if (pMsg->conversationId)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_CONVERSATION_ID, 
            (CsrUint8) CsrStrLen((const char *) pMsg->conversationId), 
            (CsrUint8*) pMsg->conversationId);
    }

    CsrPmemFree(pMsg->presenceText);
    CsrPmemFree(pMsg->lastActivity);
    CsrPmemFree(pMsg->conversationId);
	pMsg->presenceText = NULL;
	pMsg->lastActivity = NULL;
	pMsg->conversationId = NULL;

    return(ObexUtilPutRequest(pInst->obexClientInst, 
                              headerPriorityTable, 0,
                              (CsrUint8*) ("x-bt/ownerStatus"), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              csrBtMapcFillerBytePutContinueHandler, 
                              csrBtMapcSetOwnerStatusResultHandler));
}

static void csrBtMapcSetOwnerStatusResultHandler(void       *instData,
                                                 CsrUint8   responseCode,
                                                 CsrUint8   *obexPacket,
                                                 CsrUint16  obexPacketLength)
{
    MapcInstanceData *pInst    = instData;
    CsrBtMapcSetMessageStatusCfm *pMsg     = CsrPmemAlloc(sizeof(*pMsg));

    pMsg->type                  = CSR_BT_MAPC_SET_OWNER_STATUS_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}

/****************************************************************************/
/*********************** Get Conversation Listing ***************************/ 
/****************************************************************************/
CsrUint8 CsrBtMapcGetConversationListingResHandler(MapcInstanceData *pInst, void *msg);

static void CsrBtMapcGetConversationListingResultHandler(void        *instData,
                                                         CsrUint8    responseCode,
                                                         CsrBool     bodyHeader,
                                                         CsrUint16   bodyLength,
                                                         CsrUint16   bodyOffset,
                                                         CsrUint8    *obexPacket,
                                                         CsrUint16   obexPacketLength);

CsrUint8 CsrBtMapcGetConversationListingReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                        headerPriorityTable[5]  = { CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_NAME_HEADER, 
                                                                CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER, 
                                                                CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcGetConversationListingReq  *pMsg           = msg;
    CsrUint8                            *appHeader      = NULL;
    CsrUint16                           appHeaderLength = 0;
    CsrUint8                            temp[4];

    /* Build applications parameter header */
    CSR_BT_OBEX_SET_WORD_AT(temp, 0, pMsg->maxListCount);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_MAX_LIST_COUNT, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_MAX_LIST_COUNT, 
                                                                        temp);
    
    CSR_BT_OBEX_SET_WORD_AT(temp, 0, pMsg->listStartOffset);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_START_OFFSET, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_START_OFFSET, 
                                                                        temp);

    if (pMsg->filterLastActivityBegin)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FILTER_LAST_ACTIVITY_BEGIN,
            (CsrUint8) CsrStrLen((const char *) pMsg->filterLastActivityBegin), 
            (CsrUint8*) pMsg->filterLastActivityBegin);
    }
    if (pMsg->filterLastActivityEnd)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FILTER_LAST_ACTIVITY_END,
            (CsrUint8) CsrStrLen((const char *) pMsg->filterLastActivityEnd), 
            (CsrUint8*) pMsg->filterLastActivityEnd);
    }

    CSR_BT_OBEX_SET_BYTE_AT(temp, 0, pMsg->filterReadStatus);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, appHeaderLength,
        CSR_BT_MAP_TAG_ID_FILTER_READ_STATUS, CSR_BT_MAP_TAG_ID_LENGTH_FILTER_READ_STATUS, temp);

    if (pMsg->filterRecipient)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_FILTER_RECIPIENT,
            (CsrUint8) CsrStrLen((const char *) pMsg->filterRecipient), 
            (CsrUint8*) pMsg->filterRecipient);
    }
    if (pMsg->conversationId)
    {
        appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(
            &appHeader, 
            appHeaderLength,
            CSR_BT_MAP_TAG_ID_CONVERSATION_ID,
            (CsrUint8) CsrStrLen((const char *) pMsg->conversationId), 
            (CsrUint8*) pMsg->conversationId);
    }

    CSR_BT_OBEX_SET_D_WORD_AT(temp, 0, pMsg->convParameterMask);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_CONV_PARAMETER_MASK, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_CONV_PARAMETER_MASK, 
                                                                        temp);

    /* Check values used in the result handler */
    CsrPmemFree(pInst->mseTime);
    pInst->mseTime                   = CsrPmemAlloc(sizeof(CsrCharString));    
    *pInst->mseTime                  = '\0';

    pInst->numberOfConversations     = 0;

    CsrPmemFree(pInst->databaseId);
    pInst->databaseId = (CsrUtf8String*)CsrPmemAlloc(sizeof(CsrUtf8String));    
    *pInst->databaseId = '\0';

    CsrPmemFree(pInst->convListingVersionCounter);
    pInst->convListingVersionCounter = (CsrUtf8String*)CsrPmemAlloc(sizeof(CsrUtf8String));    
    *pInst->convListingVersionCounter = '\0';

    CsrPmemFree(pMsg->filterLastActivityBegin);
    CsrPmemFree(pMsg->filterLastActivityEnd);
    CsrPmemFree(pMsg->filterRecipient);
    CsrPmemFree(pMsg->conversationId);
	pMsg->filterLastActivityBegin = NULL;
	pMsg->filterLastActivityEnd = NULL;
	pMsg->filterRecipient = NULL;
	pMsg->conversationId = NULL;

    pInst->srmp = pMsg->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetRequest(pInst->obexClientInst, 
                              headerPriorityTable, 
                              (const CsrUint8*) ("x-bt/MAP-convo-listing"), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              pInst->srmp,
                              CsrBtMapcGetConversationListingResultHandler, 
                              NULL));
}

CsrUint8 CsrBtMapcGetConversationListingResHandler(MapcInstanceData *pInst, void *msg)
{
    CsrBtMapcGetMessageListingRes *res = msg;

    pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    return(ObexUtilGetContinueRequest(pInst->obexClientInst, pInst->srmp));
}

static void CsrBtMapcGetConversationListingResultHandler(void        *instData,
                                                         CsrUint8    responseCode,
                                                         CsrBool     bodyHeader,
                                                         CsrUint16   bodyLength,
                                                         CsrUint16   bodyOffset,
                                                         CsrUint8    *obexPacket,
                                                         CsrUint16   obexPacketLength)
{
    MapcInstanceData    *pInst = instData;
    CsrUint8            tagLength;
    CsrUint8            *tagValue;

    if (CsrStrCmp((char*)pInst->convListingVersionCounter, "") == 0)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_CONV_LIST_VER_COUNTER, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->convListingVersionCounter);
                pInst->convListingVersionCounter = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->convListingVersionCounter, (tagLength + 1), tagValue, tagLength);
                pInst->convListingVersionCounter[tagLength] = 0;
            }
        }
    }

    if(pInst->numberOfConversations == 0)
    {
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_MESSAGES_LISTING_SIZE, &tagLength, &tagValue))
        {
            if(tagLength == CSR_BT_MAP_TAG_ID_LENGTH_MESSAGES_LISTING_SIZE && tagValue)
            {
                pInst->numberOfConversations = CSR_BT_OBEX_GET_WORD_AT(tagValue, 0);
            }
        }
    }

    if (CsrStrCmp((char*)pInst->databaseId, "") == 0)
    {
        if (csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength, 
                CSR_BT_MAP_TAG_ID_DATABASE_ID, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->databaseId);
                pInst->databaseId = CsrPmemAlloc(tagLength + 1);
                SynMemCpyS(pInst->databaseId, (tagLength + 1), tagValue, tagLength);
                pInst->databaseId[tagLength] = 0;
            }
        }
    }

    if (CsrStrCmp((char*)pInst->mseTime,"") == 0)
    { 
        if(csrBtObexUtilExtractApplicationParametersHeaderValueField(obexPacket, obexPacketLength,
            CSR_BT_MAP_TAG_ID_MSE_TIME, &tagLength, &tagValue))
        {
            if(tagLength != 0 && tagValue)
            {
                CsrPmemFree(pInst->mseTime);
                pInst->mseTime = CsrPmemAlloc(tagLength+1);
                SynMemCpyS(pInst->mseTime, (tagLength+1), tagValue, tagLength);
                pInst->mseTime[tagLength] = 0;
            }
        }
    }

    if (responseCode != CSR_BT_OBEX_CONTINUE_RESPONSE_CODE)
    {       
        CsrBtMapcGetConversationListingCfm *pMsg = CsrPmemAlloc(sizeof(*pMsg));

        pMsg->type                      = CSR_BT_MAPC_GET_CONVERSATION_LISTING_CFM;
        pMsg->instanceId                = pInst->mapcInstanceId;
        pMsg->result                    = responseCode;
        pMsg->convListingVersionCounter = pInst->convListingVersionCounter;
        pMsg->numberOfConversations     = pInst->numberOfConversations;
        pMsg->databaseId                = pInst->databaseId;
        pMsg->mseTime                   = pInst->mseTime;
        pInst->mseTime                  = NULL;
        pMsg->listingLength             = bodyLength; 
        pMsg->listingOffset             = bodyOffset;  
        pMsg->payload                   = obexPacket;
        pMsg->payloadLength             = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);

        pInst->convListingVersionCounter = NULL;
        pInst->databaseId = NULL;
        pInst->mseTime = NULL;
    }
    else
    {
        CsrBtMapcGetConversationListingInd *pMsg = CsrPmemAlloc(sizeof(*pMsg));

        pMsg->type                      = CSR_BT_MAPC_GET_CONVERSATION_LISTING_IND;
        pMsg->instanceId                = pInst->mapcInstanceId;
        pMsg->convListingVersionCounter = CsrUtf8StrDup((CsrUtf8String*) pInst->convListingVersionCounter);
        pMsg->numberOfConversations     = pInst->numberOfConversations;
        pMsg->databaseId                = CsrUtf8StrDup((CsrUtf8String*) pInst->databaseId);
        pMsg->mseTime                   = CsrUtf8StrDup((CsrUtf8String*) pInst->mseTime);
        pMsg->listingLength             = bodyLength; 
        pMsg->listingOffset             = bodyOffset;  
        pMsg->payload                   = obexPacket;
        pMsg->payloadLength             = obexPacketLength;
        CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    }

    CSR_UNUSED(bodyHeader);
}

/****************************************************************************/
/*********************** Set Notification Filter ****************************/ 
/****************************************************************************/
static void csrBtMapcSetNotificationFilterResultHandler(void       *instData,
                                                        CsrUint8   responseCode,
                                                        CsrUint8   *obexPacket,
                                                        CsrUint16  obexPacketLength);

CsrUint8 CsrBtMapcSetNotificationFilterReqHandler(MapcInstanceData *pInst, void *msg)
{
    CsrUint8                            headerPriorityTable[6]  = { CSR_BT_OBEX_UTIL_TYPE_HEADER, 
                                                                    CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
                                                                    CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                    CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                    CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
                                                                    CSR_BT_OBEX_UTIL_UNDEFINE_HEADER};
    CsrBtMapcSetNotificationFilterReq   *pMsg                   = msg;
    CsrUint8                            *appHeader              = NULL;
    CsrUint16                           appHeaderLength         = 0;
    CsrUint8                            temp[4];

    CSR_BT_OBEX_SET_D_WORD_AT(temp, 0, pMsg->notiFilterMask);
    appHeaderLength = csrBtObexUtilAddApplicationParametersHeaderField(&appHeader, 
                                                                        appHeaderLength,
                                                                        CSR_BT_MAP_TAG_ID_NOTI_FILTER_MASK, 
                                                                        CSR_BT_MAP_TAG_ID_LENGTH_NOTI_FILTER_MASK, 
                                                                        temp);

    return(ObexUtilPutRequest(pInst->obexClientInst, 
                              headerPriorityTable, 0,
                              (CsrUint8*) ("x-bt/MAP-notification-filter"), 
                              NULL, 
                              NULL, 
                              appHeaderLength, 
                              &appHeader, 
                              0, 
                              NULL,
                              csrBtMapcFillerBytePutContinueHandler, 
                              csrBtMapcSetNotificationFilterResultHandler));
}

static void csrBtMapcSetNotificationFilterResultHandler(void       *instData,
                                                        CsrUint8   responseCode,
                                                        CsrUint8   *obexPacket,
                                                        CsrUint16  obexPacketLength)
{
    MapcInstanceData                  *pInst    = instData;
    CsrBtMapcSetNotificationFilterCfm *pMsg     = CsrPmemAlloc(sizeof(*pMsg));

    pMsg->type                  = CSR_BT_MAPC_SET_NOTIFICATION_FILTER_CFM;
    pMsg->instanceId            = pInst->mapcInstanceId;
    pMsg->result                = responseCode;

    CsrBtMapcMessagePut(pInst->appHandle, pMsg);
    CsrPmemFree(obexPacket);
    CSR_UNUSED(obexPacketLength);
}
#endif /*EXCLUDE_CSR_BT_MAPC_MODULE*/
