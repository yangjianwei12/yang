/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_AVRCP_MODULE

#include "sdc_prim.h"
#include "csr_bt_avrcp_main.h"
#include "csr_bt_avrcp_lib.h"

#if defined(CSR_BT_INSTALL_AVRCP_CT_COVER_ART) || defined(CSR_BT_INSTALL_AVRCP_TG_COVER_ART)
#include "csr_bt_avrcp_imaging_private_prim.h"
#include "csr_bt_avrcp_imaging_private_lib.h"
#endif

/* Macros */
#define AVRCP_INSERT_UINT16(dest_addr, uint)     *(dest_addr) = (CsrUint8)((uint)>>8); \
                                                 *((dest_addr) + 1) = (CsrUint8)((uint) & 0x00FF)

/* Service record sizes and indexes */
#define AVRCP_SDP_AVRCP_UUID_INDEX              (6)
#define AVRCP_SDP_AVRCP_VERSION_INDEX           (48)
#define AVRCP_SDP_FEATURES_INDEX                (54)
#define AVRCP_SDP_MINIMUM_RECORD_LENGTH         (56)
#define AVRCP_SDP_OPTIONAL_PREFIX_SIZE          (5)
#define AVRCP_SDP_OPTIONAL_PREFIX_SIZE_OFFSET   (4)
#define AVRCP_SDP_CT_EXTRA_OFFSET               (3)

/* Incomplete AVRCP record */
#define AVRCP_SDP_HEADER_SIZE                   (50)
static const CsrUint8 avrcpServiceRecordHeader[] = /* AVRCP_SDP_HEADER_SIZE */
{
    /* Service class ID list */
    0x09, 0x00, 0x01,      /* AttrId = ServiceClassIdList */
    0x35, 0x03,            /* Data element seq. 3 bytes */
    0x19, 0x00, 0x00,      /* UUID(2 bytes) */                              /* To be modified */

    /* Protocol descriptor list */
    0x09, 0x00, 0x04,      /* AttrId = ProtocolDescriptorList */
    0x35, 0x10,            /* Data element seq. 16 bytes */
    /* L2CAP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x01, 0x00,      /* UUID(2 bytes), Protocol = L2CAP */
    0x09, 0x00, CSR_BT_AVCTP_PSM, /* PSM value = AVCTP = 0x0017 */
    /* AVCTP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x00, 0x17,      /* UUID(2 bytes), Protocol = AVCTP */
    0x09, CSR_MSB16(AVCTP_VERSION), CSR_LSB16(AVCTP_VERSION),    /* AVCTP Version */

    /* BrowseGroupList    */
    0x09, 0x00, 0x05,    /* AttrId = BrowseGroupList */
    0x35, 0x03,          /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,    /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* Bluetooth profile descriptor list */
    0x09, 0x00, 0x09,      /* AttrId = BluetoothProfileDescriptorList */
    0x35, 0x08,            /* Data element seq. 8 bytes */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x11, 0x0E,      /* UUID(2 bytes), Profile = A/V remote Control */
    0x09, 0x00, 0x00      /* AVRCP Version */                              /* To be modified */
};

#ifdef CSR_BT_INSTALL_AVRCP_13_AND_HIGHER
#define AVRCP_SDP_CT_HEADER_SIZE                   (53)
static const CsrUint8 avrcpCtServiceRecordHeader[] = /* AVRCP_SDP_CT_HEADER_SIZE */
{
    /* Service class ID list */
    0x09, 0x00, 0x01,      /* AttrId = ServiceClassIdList */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x00, 0x00,      /* UUID(2 bytes) */                              /* To be modified */
    0x19, 0x00, 0x00,      /* UUID(2 bytes) */                              /* To be modified */

    /* Protocol descriptor list */
    0x09, 0x00, 0x04,      /* AttrId = ProtocolDescriptorList */
    0x35, 0x10,            /* Data element seq. 16 bytes */
    /* L2CAP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x01, 0x00,      /* UUID(2 bytes), Protocol = L2CAP */
    0x09, 0x00, CSR_BT_AVCTP_PSM, /* PSM value = AVCTP = 0x0017 */
    /* AVCTP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x00, 0x17,      /* UUID(2 bytes), Protocol = AVCTP */
    0x09, CSR_MSB16(AVCTP_VERSION), CSR_LSB16(AVCTP_VERSION),    /* AVCTP Version */
    
    /* BrowseGroupList    */
    0x09, 0x00, 0x05,    /* AttrId = BrowseGroupList */
    0x35, 0x03,          /* Data element seq. 3 bytes */
    0x19, 0x10, 0x02,    /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

    /* Bluetooth profile descriptor list */
    0x09, 0x00, 0x09,      /* AttrId = BluetoothProfileDescriptorList */
    0x35, 0x08,            /* Data element seq. 8 bytes */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x11, 0x0E,      /* UUID(2 bytes), Profile = A/V remote Control */
    0x09, 0x00, 0x00      /* AVRCP Version */                              /* To be modified */
};

#define AVRCP_SDP_ADDITIONAL_PDL_SIZE_INDEX     (4)
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
#define AVRCP_SDP_TG_ADDITIONAL_PDL_DESEQ_SIZE  (33)
#endif

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
#define AVRCP_SDP_BROWSING_SIZE                 (23)
static const CsrUint8 avrcpServiceRecordBrowsing[] = /* AVRCP_SDP_BROWSING_SIZE */
{
    /* Additional protocol descriptor list */
    0x09, 0x00, 0x0D,      /* AttrId = AdditionalProtocolDescriptorList */
    0x35, 0x12,            /* Data element seq: 18 bytes for CT; 33 bytes for TG */

    /* Protocol descriptor list */
    0x35, 0x10,            /* Data element seq. 16 bytes */
    /* L2CAP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x01, 0x00,      /* UUID(2 bytes), Protocol = L2CAP */
    0x09, 0x00, CSR_BT_AVCTP_BROWSING_PSM,/* PSM value = AVCTP_BROWSING = 0x001B */
    /* AVCTP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x00, 0x17,      /* UUID(2 bytes), Protocol = AVCTP */
    0x09, CSR_MSB16(AVCTP_VERSION), CSR_LSB16(AVCTP_VERSION),   /* AVCTP Version */
};
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
#define AVRCP_SDP_TG_COVER_ART_SIZE              (15)
#define AVRCP_SDP_TG_COVER_ART_PSM_INDEX         (8)
static const CsrUint8 avrcpTgServiceRecordCoverArt[] = /* AVRCP_SDP_TG_COVER_ART_SIZE */
{
    /* Additional protocol descriptor list extension */
    /* Protocol descriptor list */
    0x35, 0x0D,            /* Data element seq. 13 bytes */
    /* L2CAP */
    0x35, 0x06,            /* Data element seq. 6 bytes */
    0x19, 0x01, 0x00,      /* UUID(2 bytes), Protocol = L2CAP */
    0x09, 0xDE, 0xAD,      /* PSM value = Dynamically assigned */           /* To be modified */
    /* OBEX */
    0x35, 0x03,            /* Data element seq. 3 bytes */
    0x19, 0x00, 0x08,      /* UUID(2 bytes), Protocol = OBEX */
};
#endif
#endif
#define AVRCP_SDP_FOOTER_SIZE                   (6)
static const CsrUint8 avrcpServiceRecordFooter[] = /* AVRCP_SDP_FOOTER_SIZE */
{
    /* Supported features */
    0x09, 0x03, 0x11,      /* AttrId = Supported features */
    0x09, 0x00, 0x00,      /* Supported features */                         /* To be modified */
};

static void csrBtAvrcpSdpDeInit(AvrcpInstanceData_t *instData)
{
    if (instData->sdpSearchData)
    {
        CsrBtUtilSdcRfcDeinit(&(instData->sdpSearchData));
        instData->sdpSearchData = NULL;
    }
}

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
static CsrBool csrBtAvrcpSdpExtractCoverArtServicePsm(CmnCsrBtLinkedListStruct *sdpTag, 
                                                                CsrUint16 serviceHandleIndex, 
                                                                CsrUint16 *psm);
#endif

static CsrBool csrBtAvrcpSdpGetBluetoothProfileDescriptorList(CmnCsrBtLinkedListStruct *bll_p,
                                                        CsrUint16 serviceHandleIndex,
                                                        CsrUint16 *version)
{
    CsrBool    retBool = FALSE;
    CsrUint8  *att_p;
    CsrUint16  attDataLen, nofBytesToAttribute, emptyAttSize, consumedBytes, totalConsumedBytes = 0, tempVar;
    CsrUintFast16 nofAttributes, x;
    CsrUint32  returnValue, protocolValue;

    if (TRUE == CsrBtUtilSdrGetNofAttributes(bll_p, serviceHandleIndex, &nofAttributes))
    {
        for (x=0; x<nofAttributes; x++)
        {
            att_p = CsrBtUtilSdrGetAttributePointer(bll_p, serviceHandleIndex, x, &nofBytesToAttribute);

            if (att_p)
            {
                /* Check if the UUID in the 'outer' attribute struct is correct */
                SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p + SDR_ENTRY_INDEX_ATTRIBUTE_UUID, SDR_ENTRY_SIZE_SERVICE_UINT16);

                if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == tempVar)
                {
                    CsrBtUtilSdrGetEmptyAttributeSize(&emptyAttSize);
                    SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, att_p, SDR_ENTRY_SIZE_SERVICE_UINT16);
                    attDataLen = tempVar - emptyAttSize + SDR_ENTRY_SIZE_TAG_LENGTH;

                    /* First extract the attribute uuid from the attribute SDP data */
                    if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA,
                                                  attDataLen,
                                                  &returnValue,
                                                  &consumedBytes,
                                                  FALSE))
                    {
                        /* Check if the UUID in the 'inner' attribute sdp data struct is correct */
                        if (CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == returnValue)
                        {
                            attDataLen = attDataLen - consumedBytes;
                            totalConsumedBytes += consumedBytes;
                            /* first find the protocol UUID */
                            if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                          attDataLen,
                                                          &protocolValue,
                                                          &consumedBytes,
                                                          TRUE))
                            {
                                attDataLen = attDataLen - consumedBytes;
                                totalConsumedBytes += consumedBytes;
                                /* Now find the value */
                                if (TRUE == CsrBtUtilSdpExtractUint(att_p + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                              attDataLen,
                                                              &returnValue,
                                                              &consumedBytes,
                                                              TRUE))
                                {
                                    attDataLen = attDataLen - consumedBytes;
                                    totalConsumedBytes += consumedBytes;

                                    if (CSR_BT_AV_REMOTE_CONTROL_UUID == protocolValue)
                                    {
                                        *version = (CsrUint16)returnValue;
                                        retBool = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return retBool;
}

static void csrBtAvrcpSdpExtractServiceRecordV13Data(CmnCsrBtLinkedListStruct *bll_p,
                                           CsrUint16                 serviceHandleIndex,
                                           CsrUint16                 *avrcpVersion,
                                           CsrUint16                 *supportedFeatures,
                                           CsrCharString             **providerName,
                                           CsrCharString             **serviceName)
{
    CsrUint8 *string;
    CsrUint16 stringLen;
    CsrUint32 features;

    if (FALSE == csrBtAvrcpSdpGetBluetoothProfileDescriptorList(bll_p, serviceHandleIndex, avrcpVersion))
    {
        *avrcpVersion = CSR_BT_AVRCP_CONFIG_SR_VERSION_10;
    }

    if (TRUE == CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(bll_p, serviceHandleIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, &features))
    {
        *supportedFeatures = (CsrUint16)features;
    }
    else
    {
        *supportedFeatures = 0;
    }

    if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(bll_p, serviceHandleIndex, CSR_BT_SERVICE_PROVIDER_NAME, &string, &stringLen))
    {
        *providerName = CsrPmemAlloc(stringLen + 1);
        SynMemCpyS(*providerName, stringLen + 1, string, stringLen);
        (*providerName)[stringLen] = 0;
    }
    else
    {
        *providerName = NULL;
    }

    if (TRUE == CsrBtUtilSdrGetStringAttributeFromAttributeUuid(bll_p, serviceHandleIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, &string, &stringLen))
    {
        *serviceName = CsrPmemAlloc(stringLen + 1);
        SynMemCpyS(*serviceName, stringLen + 1, string, stringLen);
        (*serviceName)[stringLen] = 0;
    }
    else
    {
        *providerName = NULL;
    }
}

CsrUint16 CsrBtAvrcpSdpGenerateServiceRecord(CsrBtAvrcpRoleDetails *roleDetails, CsrUint8 **sr, CsrUint8 role, CsrUint16 psm)
{
    CsrUint8     providerNameLen = roleDetails->providerName ? (CsrUint8)CsrStrLen(roleDetails->providerName) : 0;
    CsrUint8     serviceNameLen = roleDetails->serviceName ? (CsrUint8)CsrStrLen(roleDetails->serviceName) : 0;
    CsrUint16    srLength = AVRCP_SDP_MINIMUM_RECORD_LENGTH;
    CsrUint8     srOffset = 0;
    CsrUint8     srCtExtraOffset = 0;
    CsrUint8     optionalElementsLen = 0;
    CsrUint8     *tmpSr;
    CsrUint16    mask;
#ifndef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    CSR_UNUSED(psm);
#endif

#ifdef CSR_BT_INSTALL_AVRCP_13_AND_HIGHER
    if (roleDetails->srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)
    {
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
        srOffset        = AVRCP_SDP_BROWSING_SIZE;
        srLength        += AVRCP_SDP_BROWSING_SIZE;
#endif
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
        if ((role == AVRCP_ROLE_TARGET) &&
            (roleDetails->srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
            (roleDetails->srFeatures & CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART))
        {
            srLength    += AVRCP_SDP_TG_COVER_ART_SIZE;
        }
#endif
        if (role == AVRCP_ROLE_CONTROLLER)
        {
            srCtExtraOffset = AVRCP_SDP_CT_EXTRA_OFFSET;
            srLength   += AVRCP_SDP_CT_EXTRA_OFFSET;
        }
    }
#endif

    /* Determine totalt length and allocate memory */
    *sr = CsrPmemAlloc(srLength +
                (roleDetails->providerName ? providerNameLen + AVRCP_SDP_OPTIONAL_PREFIX_SIZE : 0) +
                (roleDetails->serviceName ? serviceNameLen + AVRCP_SDP_OPTIONAL_PREFIX_SIZE : 0));
    tmpSr = *sr;

#ifdef CSR_BT_INSTALL_AVRCP_13_AND_HIGHER
    /* Insert required elements in service record */
    if ((role == AVRCP_ROLE_CONTROLLER) && (roleDetails->srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14))
    {
        SynMemCpyS(tmpSr, sizeof(avrcpCtServiceRecordHeader), avrcpCtServiceRecordHeader, sizeof(avrcpCtServiceRecordHeader));
        tmpSr += sizeof(avrcpCtServiceRecordHeader);
    }
    else
#endif
    {
        SynMemCpyS(tmpSr, sizeof(avrcpServiceRecordHeader), avrcpServiceRecordHeader, sizeof(avrcpServiceRecordHeader));
        tmpSr += sizeof(avrcpServiceRecordHeader);
    }

#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
    if (srOffset)
    {
        SynMemCpyS(tmpSr, sizeof(avrcpServiceRecordBrowsing), avrcpServiceRecordBrowsing, sizeof(avrcpServiceRecordBrowsing));
        tmpSr += sizeof(avrcpServiceRecordBrowsing);
    }
#endif

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
    if ((role == AVRCP_ROLE_TARGET) &&
        (roleDetails->srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
        (roleDetails->srFeatures & CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART))
    {/* cover art */
        *(tmpSr - srOffset + AVRCP_SDP_ADDITIONAL_PDL_SIZE_INDEX) = AVRCP_SDP_TG_ADDITIONAL_PDL_DESEQ_SIZE;

        SynMemCpyS(tmpSr, sizeof(avrcpTgServiceRecordCoverArt), avrcpTgServiceRecordCoverArt, sizeof(avrcpTgServiceRecordCoverArt));
        /* insert cover art dynamic psm */
        *(tmpSr + AVRCP_SDP_TG_COVER_ART_PSM_INDEX) = (CsrUint8)(psm>>8);
        *(tmpSr + AVRCP_SDP_TG_COVER_ART_PSM_INDEX + 1) = (CsrUint8)((psm) & 0x00FF);

        tmpSr += AVRCP_SDP_TG_COVER_ART_SIZE;
        srOffset += AVRCP_SDP_TG_COVER_ART_SIZE;
    }
#endif

    /* Insert optional elements in service record */
    if (roleDetails->serviceName)
    {/* Service Name */
        const CsrUint8 servicePrefix[AVRCP_SDP_OPTIONAL_PREFIX_SIZE] = { 0x09, 0x01, 0x00, 0x25, 0x00 };

        SynMemCpyS(tmpSr, AVRCP_SDP_OPTIONAL_PREFIX_SIZE, servicePrefix, AVRCP_SDP_OPTIONAL_PREFIX_SIZE);
        *(tmpSr + AVRCP_SDP_OPTIONAL_PREFIX_SIZE_OFFSET) = serviceNameLen;
        srLength += AVRCP_SDP_OPTIONAL_PREFIX_SIZE;
        tmpSr += AVRCP_SDP_OPTIONAL_PREFIX_SIZE;

        SynMemCpyS(tmpSr, serviceNameLen, roleDetails->serviceName, serviceNameLen);
        srLength += serviceNameLen;
        tmpSr += serviceNameLen;
        optionalElementsLen += (serviceNameLen + AVRCP_SDP_OPTIONAL_PREFIX_SIZE);
    }

    if (roleDetails->providerName)
    {/* Provider Name */
        const CsrUint8 providerPrefix[AVRCP_SDP_OPTIONAL_PREFIX_SIZE] = { 0x09, 0x01, 0x02, 0x25, 0x00 };

        SynMemCpyS(tmpSr, AVRCP_SDP_OPTIONAL_PREFIX_SIZE, providerPrefix, AVRCP_SDP_OPTIONAL_PREFIX_SIZE);
        *(tmpSr + AVRCP_SDP_OPTIONAL_PREFIX_SIZE_OFFSET) = providerNameLen;
        srLength += AVRCP_SDP_OPTIONAL_PREFIX_SIZE;
        tmpSr += AVRCP_SDP_OPTIONAL_PREFIX_SIZE;

        SynMemCpyS(tmpSr, providerNameLen, roleDetails->providerName, providerNameLen);
        srLength += providerNameLen;
        tmpSr += providerNameLen;
        optionalElementsLen += (providerNameLen + AVRCP_SDP_OPTIONAL_PREFIX_SIZE);
    }

    SynMemCpyS(tmpSr, sizeof(avrcpServiceRecordFooter), avrcpServiceRecordFooter, sizeof(avrcpServiceRecordFooter));

    mask = role == AVRCP_ROLE_TARGET ? CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID : CSR_BT_AV_REMOTE_CONTROL_UUID;

    AVRCP_INSERT_UINT16(*sr + AVRCP_SDP_AVRCP_UUID_INDEX, mask);
    AVRCP_INSERT_UINT16(*sr + AVRCP_SDP_AVRCP_VERSION_INDEX + srCtExtraOffset, roleDetails->srAvrcpVersion);
    if(role == AVRCP_ROLE_CONTROLLER)
    {/* bit 4,5 and 10-15 are RFA: must not be used! */
        mask = roleDetails->srFeatures & AVRCP_CT_SDP_FEATURES_MASK;

        if (roleDetails->srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)
        {
            AVRCP_INSERT_UINT16(*sr + AVRCP_SDP_AVRCP_UUID_INDEX + srCtExtraOffset, CSR_BT_AV_REMOTE_CONTROL_CONTROLLER_UUID);
        }
        AVRCP_INSERT_UINT16(*sr + AVRCP_SDP_FEATURES_INDEX + srOffset + srCtExtraOffset + optionalElementsLen, mask);
    }
    else
    { /* role is AVRCP_ROLE_TARGET */
        mask = roleDetails->srFeatures & AVRCP_TG_SDP_FEATURES_MASK;

        AVRCP_INSERT_UINT16(*sr + AVRCP_SDP_FEATURES_INDEX + srOffset + optionalElementsLen, mask);
    }

    return srLength;
}

/* Determines which SR to register */
CsrBool CsrBtAvrcpSdpRegisterSR(AvrcpInstanceData_t *instData)
{
    if (instData->srPending)
    {
        CsrUint8 *sr;
        CsrUint16 srLength;

        if (instData->srPending->tgDetails.roleSupported)
        {
#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
            srLength = CsrBtAvrcpSdpGenerateServiceRecord(&instData->srPending->tgDetails, &sr, AVRCP_ROLE_TARGET, instData->tgLocal.obexPsm);
#else
            srLength = CsrBtAvrcpSdpGenerateServiceRecord(&instData->srPending->tgDetails, &sr, AVRCP_ROLE_TARGET, 0);
#endif
            CsrBtCmSdsRegisterReqSend(CSR_BT_AVRCP_IFACEQUEUE, sr, srLength, CSR_BT_CM_CONTEXT_UNUSED);
            AVRCP_CHANGE_STATE(instData->srActiveRole, AVRCP_ROLE_TARGET);
            CsrBtAvrcpUtilFreeRoleDetails(&instData->srPending->tgDetails);
            return TRUE;
        }
        else if (instData->srPending->ctDetails.roleSupported)
        {
            srLength = CsrBtAvrcpSdpGenerateServiceRecord(&instData->srPending->ctDetails, &sr, AVRCP_ROLE_CONTROLLER, 0);
            CsrBtCmSdsRegisterReqSend(CSR_BT_AVRCP_IFACEQUEUE, sr, srLength, CSR_BT_CM_CONTEXT_UNUSED);
            AVRCP_CHANGE_STATE(instData->srActiveRole, AVRCP_ROLE_CONTROLLER);
            CsrBtAvrcpUtilFreeConfigReq(&instData->srPending);
            return TRUE;
        }
        else
        {
            CsrBtAvrcpUtilFreeConfigReq(&instData->srPending); /* Make sure everything is released */
        }
    }

    return FALSE;
}

static void csrBtAvrcpSearchResultHandler(CsrSdcOptCallbackType cbType, void *context)
{
    if(cbType == CSR_SDC_OPT_CB_SEARCH_RESULT)
    {
        CsrSdcResultFuncType *params = (CsrSdcResultFuncType *)context;

        CsrBtAvrcpSdpResultHandler(params->instData,
                                   params->sdpTagList,
                                   params->deviceAddr,
                                   params->resultCode,
                                   params->resultSupplier);
    }
}

void CsrBtAvrcpSdpSearchStart(AvrcpInstanceData_t *instData, AvrcpConnInstance_t *connInst)
{
    CmnCsrBtLinkedListStruct *sdpTagList = NULL;
    CsrUint16                shIndex;

    if (AVRCP_STATE_SDP_IDLE == instData->sdpState)
    {
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
        if (instData->tgLocal.srHandle != AVRCP_SDP_INVALID_SR_HANDLE)
        {/* Search for the corresponding controller */
            sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_AV_REMOTE_CONTROL_UUID, &shIndex);
#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_CLASS_ID_LIST, NULL, 0);
#endif
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_PROVIDER_NAME, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
        }
#endif
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
        if (instData->ctLocal.srHandle != AVRCP_SDP_INVALID_SR_HANDLE)
        {/* Search for the corresponding target */
            sdpTagList = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTagList, CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID, &shIndex);
#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_CLASS_ID_LIST, NULL, 0);
#endif
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_NAME_ATTRIBUTE_IDENTIFIER, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SERVICE_PROVIDER_NAME, NULL, 0);
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER, NULL, 0);
#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
            CsrBtUtilSdrCreateAndInsertAttribute(sdpTagList, shIndex, CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER, NULL, 0);
#endif
        }
#endif
        AVRCP_CHANGE_STATE(instData->sdpState, AVRCP_STATE_SDP_ACTIVE);
        AVRCP_CHANGE_STATE_INDEX(connInst->sdpState, AVRCP_STATE_SDP_ACTIVE, connInst->appConnId);

        if (instData->sdpSearchData == NULL)
        {
            /* allocating instance at the start of the search */
            instData->sdpSearchData = CsrBtUtilSdcInit(csrBtAvrcpSearchResultHandler, CSR_BT_AVRCP_IFACEQUEUE);
        }

        instData->searchOngoing = TRUE;
        /* Start the SDP search */
        CsrBtUtilSdcSearchStart((void *)instData, instData->sdpSearchData, sdpTagList, connInst->address);
    }
}

void CsrBtAvrcpSdpRestartSearch(AvrcpInstanceData_t *instData)
{
    AvrcpConnInstance_t *connInst;
    /* Determine if SDP search should be started for other connections */
    connInst = AVRCP_LIST_CONN_GET_SDP_ST((CsrCmnList_t *)&instData->connList, AVRCP_STATE_SDP_PENDING);

    if (connInst && (AVRCP_STATE_CONN_CONNECTED == connInst->control.state))
    {
        /* Start SDP search for pending connection */
        CsrBtAvrcpSdpSearchStart(instData, connInst);
    }
}

void CsrBtAvrcpSdpSearchCancel(AvrcpConnInstance_t *connInst)
{
    AvrcpInstanceData_t *instData = connInst->instData;
    AVRCP_CHANGE_STATE_INDEX(connInst->sdpState, AVRCP_STATE_SDP_DONE, connInst->appConnId);
    CsrBtUtilSdcSearchCancel((void *)instData, instData->sdpSearchData);
}

static void avrcpUpdateRemoteBrowsingSupport(AvrcpInstanceData_t *instData,
                                             AvrcpConnInstance_t *connInst,
                                             CsrBtAvrcpRoleDetails remoteFeatures,
                                             CsrBtUuid32 tmpUuid)
{
#ifdef CSR_BT_INSTALL_AVRCP_BROWSING
     if (tmpUuid == CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID)
     {
         /*  If peer device is Target and
             The local device supports AVRCP 1.4 or higher and
             The peer device supports AVRCP 1.4 or higher and
             The peer device supports Category 1 or Category 3 or Browsing or Multiple Media Players

             Then
             The local device should able to initiate a browsing channel connection.
         */
         if ((instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)     &&
             (remoteFeatures.srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)       &&
             ((CSR_MASK_IS_SET(remoteFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT1_PLAY_REC)) ||
              (CSR_MASK_IS_SET(remoteFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_CAT3_TUNER))    ||
              (CSR_MASK_IS_SET(remoteFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING))      ||
              (CSR_MASK_IS_SET(remoteFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_MULTIPLE_MP))))
         {
             CSR_MASK_SET(connInst->remoteFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING);
         }
     }
     else
     {
         /*  Peer device is Controller and
             The local device supports AVRCP 1.4 or higher and
             The peer device supports AVRCP 1.4 or higher  and
             The peer device supports Browsing

             Then
             The local device should able to initiate a browsing channel connection.
         */
         if ((instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)     &&
             (remoteFeatures.srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_14)       &&
             (CSR_MASK_IS_SET(remoteFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING)))
         {
             CSR_MASK_SET(connInst->remoteFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_BROWSING);
         }
     }
#else /* !CSR_BT_INSTALL_AVRCP_BROWSING */
    CSR_UNUSED(instData);
    CSR_UNUSED(connInst);
    CSR_UNUSED(remoteFeatures);
    CSR_UNUSED(tmpUuid);
#endif /* CSR_BT_INSTALL_AVRCP_BROWSING */
}

static void avrcpSdpResultFailed(AvrcpInstanceData_t *instData,
                                 AvrcpConnInstance_t *connInst,
                                 CsrBtResultCode resultCode,
                                 CsrBtSupplier resultSupplier,
                                 CmnCsrBtLinkedListStruct *sdpTagList)
{
    if ((resultCode     == SDC_NO_RESPONSE_DATA &&
         (resultSupplier == CSR_BT_SUPPLIER_SDP_SDC ||
          resultSupplier == CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH)) ||
        (resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS &&
         resultSupplier == CSR_BT_SUPPLIER_CM &&
         sdpTagList     == NULL))
    {
         resultCode     = CSR_BT_RESULT_CODE_AVRCP_INVALID_SDP_RECORD;
         resultSupplier = CSR_BT_SUPPLIER_AVRCP;
    }

    /* Inform application about failure.*/
    CsrBtAvrcpConnectCfmSend(instData->ctrlHandle,
                             &connInst->address,
                             AVRCP_MTU_INVALID,
                             CSR_BT_AVRCP_CONNECTION_ID_INVALID,
                             NULL,
                             NULL,
                             resultCode,
                             resultSupplier,
                             CSR_BT_CONN_ID_INVALID);
    CsrBtAvrcpUtilConnRemove((CsrCmnListElm_t*)connInst, NULL);
    AVRCP_LIST_CONN_REMOVE((CsrCmnList_t*)&instData->connList, connInst);
    CsrBtAvrcpUtilGo2Idle(instData);
}

void CsrBtAvrcpSdpResultHandler(void             *inst,
                        CmnCsrBtLinkedListStruct *sdpTagList,
                        CsrBtDeviceAddr             deviceAddr,
                        CsrBtResultCode          resultCode,
                        CsrBtSupplier      resultSupplier)
{
    AvrcpInstanceData_t *instData = (AvrcpInstanceData_t *) inst;
    AvrcpConnInstance_t *connInst = AVRCP_LIST_CONN_GET_ADDR((CsrCmnList_t *)&instData->connList, &deviceAddr);

    AVRCP_CHANGE_STATE(instData->sdpState, AVRCP_STATE_SDP_IDLE);

    instData->searchOngoing = FALSE;

    if (connInst)
    {
        if (connInst->sdpState == AVRCP_STATE_SDP_ACTIVE)
        {/* Device is known, connected and doing SDP search */
            CsrBtAvrcpRoleDetails ctFeatures;
            CsrBtAvrcpRoleDetails tgFeatures;

            CsrBtAvrcpConfigRoleNoSupport(&tgFeatures);
            CsrBtAvrcpConfigRoleNoSupport(&ctFeatures);

            if (resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS &&
                resultSupplier == CSR_BT_SUPPLIER_CM &&
                sdpTagList     != NULL)
            {
                CsrUintFast16    numOfSdpRecords = CsrBtUtilBllGetNofEntriesEx(sdpTagList);
                CsrUintFast16    sdpRecordIndex;
                CsrBtUuid32 tmpUuid;
                CsrUint16    tmpResult;
                CsrUint16    dummy1, dummy2; /* Currently CSR_UNUSED */

                for (sdpRecordIndex = 0; sdpRecordIndex < numOfSdpRecords; sdpRecordIndex++)
                {/* Handle each service record */
                    if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTagList,
                                                    sdpRecordIndex,
                                                    &tmpUuid,
                                                    &tmpResult,
                                                    &dummy1,
                                                    &dummy2))
                    {
                        if (tmpResult == SDR_SDC_SEARCH_SUCCESS)
                        {
                            if (tmpUuid == CSR_BT_AV_REMOTE_CONTROL_TARGET_UUID)
                            {
#ifndef EXCLUDE_CSR_BT_AVRCP_CT_MODULE
                                csrBtAvrcpSdpExtractServiceRecordV13Data(sdpTagList,
                                                                    (CsrUint16)sdpRecordIndex,
                                                                    &connInst->ctLocal->tgSdpAvrcpVersion,
                                                                    &connInst->ctLocal->tgSdpSupportedFeatures,
                                                                    &tgFeatures.providerName,
                                                                    &tgFeatures.serviceName);

                                tgFeatures.srAvrcpVersion   = connInst->ctLocal->tgSdpAvrcpVersion;
                                tgFeatures.srFeatures       = connInst->ctLocal->tgSdpSupportedFeatures;

                                avrcpUpdateRemoteBrowsingSupport(instData, connInst, tgFeatures, tmpUuid);
#endif
                                tgFeatures.roleSupported = TRUE;

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                                connInst->ctLocal->obexPsm = L2CA_PSM_INVALID;
                                /* Check if remote device supports Cover Art then it shall have the OBEX PSM */
                                if ((instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
                                    (tgFeatures.srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
                                    (CSR_MASK_IS_SET(tgFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART)))
                                {
                                    CsrUint16 coverArtPsm = 0;

                                    if (csrBtAvrcpSdpExtractCoverArtServicePsm(sdpTagList, (CsrUint16) sdpRecordIndex,
                                                                            &coverArtPsm) != FALSE)
                                    {
                                        CSR_MASK_SET(connInst->remoteFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART);
                                        connInst->ctLocal->obexPsm = coverArtPsm;
                                    }
                                }
#endif
                            }
                            else if (tmpUuid == CSR_BT_AV_REMOTE_CONTROL_UUID)
                            {
#ifndef EXCLUDE_CSR_BT_AVRCP_TG_MODULE
                                csrBtAvrcpSdpExtractServiceRecordV13Data(sdpTagList,
                                                                    (CsrUint16) sdpRecordIndex,
                                                                    &connInst->tgLocal->ctSdpAvrcpVersion,
                                                                    &connInst->tgLocal->ctSdpSupportedFeatures,
                                                                    &ctFeatures.providerName,
                                                                    &ctFeatures.serviceName);

                                ctFeatures.srAvrcpVersion   = connInst->tgLocal->ctSdpAvrcpVersion;
                                ctFeatures.srFeatures       = connInst->tgLocal->ctSdpSupportedFeatures;

                                avrcpUpdateRemoteBrowsingSupport(instData, connInst, ctFeatures, tmpUuid);

#ifdef CSR_BT_INSTALL_AVRCP_TG_COVER_ART
                                if ((connInst->connDirection != AVRCP_CONN_DIR_INCOMING) &&
                                    (instData->srAvrcpVersionHighest >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
                                    (ctFeatures.srAvrcpVersion >= CSR_BT_AVRCP_CONFIG_SR_VERSION_16) &&
                                    ((CSR_MASK_IS_SET(ctFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART_GET_IMG_PROP)) ||
                                     (CSR_MASK_IS_SET(ctFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART_GET_IMG)) ||
                                     (CSR_MASK_IS_SET(ctFeatures.srFeatures, CSR_BT_AVRCP_CONFIG_SR_FEAT_COVER_ART_GET_IMG_THUMB))))
                                {
                                    dm_security_level_t secIncomingCont = 0;

#ifndef INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS
                                    CsrBtScSetSecInLevel(&secIncomingCont, CSR_BT_SEC_DEFAULT,
                                                    CSR_BT_AV_RCP_MANDATORY_SECURITY_INCOMING,
                                                    CSR_BT_AV_RCP_DEFAULT_SECURITY_INCOMING,
                                                    CSR_BT_RESULT_CODE_AVRCP_SUCCESS,
                                                    CSR_BT_RESULT_CODE_AVRCP_UNACCEPTABLE_PARAMETER);
#else
                                    secIncomingCont = instData->secIncomingCont;
#endif /* INSTALL_AVRCP_CUSTOM_SECURITY_SETTINGS */

                                    CsrBtAvrcpImagingServerActivateReqSend(instData->tgLocal.obexPsm,
                                                                           connInst->address,
                                                                           connInst->appConnId,
                                                                           secIncomingCont,
                                                                           connInst->tgLocal->mpBrowsed->mpHandle);
                                }
#endif
#endif
                                ctFeatures.roleSupported    = TRUE;
                            }
                        }
                    }
                }

                if (connInst->control.state == AVRCP_STATE_CONN_PENDING)
                {/* Proceed by connecting to the remote device */
                    instData->tgDetails = tgFeatures.roleSupported ? CsrMemDup(&tgFeatures, sizeof(CsrBtAvrcpRoleDetails)) : NULL;
                    instData->ctDetails = ctFeatures.roleSupported ? CsrMemDup(&ctFeatures, sizeof(CsrBtAvrcpRoleDetails)) : NULL;
                    CsrBtAvrcpUtilConnect(connInst);
                }
                else if (connInst->control.state == AVRCP_STATE_CONN_CONNECTED)
                {/* The remote features has been retrieved due to an incoming connection */
                    CsrBtAvrcpRemoteFeaturesIndSend(instData->ctrlHandle,
                                               connInst->appConnId,
                                               connInst->address,
                                               &tgFeatures,
                                               &ctFeatures);

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
                    if (connInst->ctLocal->obexPsm != L2CA_PSM_INVALID)
                    {
                        /* Establish OBEX connection */
                        connInst->ctLocal->ctObexState = AVRCP_CT_OBEX_CONNECTING_IN;
                        CsrBtAvrcpImagingClientConnectReqSend(connInst->appConnId,
                            connInst->address, connInst->ctLocal->obexPsm,
                            instData->secIncomingCont);
                    }
#else
                    CsrBtAvrcpUtilGo2Idle(instData);

#endif
                }
            }
            else
            {
                /* SDP failed for any reason */
                avrcpSdpResultFailed(instData, connInst, resultCode, resultSupplier, sdpTagList);
            }

            if (connInst)
            {
                AVRCP_CHANGE_STATE_INDEX(connInst->sdpState, AVRCP_STATE_SDP_DONE, connInst->appConnId);
            }
        }
        else if (connInst->sdpState == AVRCP_STATE_SDP_DONE)
        {/* Send confirmation to app; this is a cancel operation! */
            if (connInst->control.state != AVRCP_STATE_CONN_DISCONNECTING)
            {
                avrcpSdpResultFailed(instData,
                                     connInst,
                                     CSR_BT_RESULT_CODE_AVRCP_CONNECT_ATTEMPT_CANCELLED,
                                     CSR_BT_SUPPLIER_AVRCP,
                                     sdpTagList);
            }
            /* else wait for disconnect indication */
        }
    }
    else if((resultSupplier == CSR_BT_SUPPLIER_CM ) && (resultCode == CSR_BT_RESULT_CODE_CM_CANCELLED))
    {
        CsrBtAvrcpUtilGo2Idle(instData);
    }
    CsrBtUtilBllFreeLinkedList(&sdpTagList, CsrBtUtilBllPfreeWrapper);

    /* Send a request for deallocating the SDP instance. this needs to be done since this is
     * callback from SDP and it uses some of the resources after calling this.
     */
    csrBtAvrcpSdpDeInit(instData);
}

#ifdef CSR_BT_INSTALL_AVRCP_CT_COVER_ART
static CsrBool csrBtAvrcpSdpExtractCoverArtServicePsm(CmnCsrBtLinkedListStruct *sdpTag, 
                                                                CsrUint16 serviceHandleIndex, 
                                                                CsrUint16 *psm)
{
    CsrBool    retBool = FALSE;
    CsrUint8  *attribPtr = NULL;
    CsrUint16  attDataLen, nofBytesToAttribute, emptyAttSize, consumedBytes, totalConsumedBytes, tempVar;
    CsrUintFast16 nofAttributes, index;
    CsrUint32  returnValue, protocolValue;

    attDataLen = nofBytesToAttribute = emptyAttSize = consumedBytes = totalConsumedBytes = tempVar = 0;
    nofAttributes = index = 0;
    returnValue = protocolValue = 0;

    if (CsrBtUtilSdrGetNofAttributes(sdpTag, serviceHandleIndex, &nofAttributes))
    {
        for (index = 0; index < nofAttributes; index++)
        {
            attribPtr = CsrBtUtilSdrGetAttributePointer(sdpTag, serviceHandleIndex, index, &nofBytesToAttribute);

            if (attribPtr)
            { /* Check if the UUID in the 'outer' attribute struct is correct */
                SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_UUID, SDR_ENTRY_SIZE_SERVICE_UINT16);

                if (CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == tempVar)
                {
                    CsrBtUtilSdrGetEmptyAttributeSize(&emptyAttSize);
                    SynMemCpyS(&tempVar, SDR_ENTRY_SIZE_SERVICE_UINT16, attribPtr, SDR_ENTRY_SIZE_SERVICE_UINT16);
                    attDataLen = tempVar - emptyAttSize + SDR_ENTRY_SIZE_TAG_LENGTH;

                    /* First extract the attribute uuid from the attribute SDP data */
                    if (CsrBtUtilSdpExtractUint(
                            attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                            attDataLen, &returnValue, &consumedBytes, FALSE))
                    {
                        attDataLen = attDataLen - consumedBytes;
                        totalConsumedBytes += consumedBytes;

                        /* Check if the UUID in the 'inner' attribute sdp data struct is correct */
                        if (CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER == returnValue)
                        {
                            while (attDataLen > 0)
                            {
                                /* first find the protocol UUID */
                                if (CsrBtUtilSdpExtractUint(
                                        attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                        attDataLen, &protocolValue, &consumedBytes, TRUE))
                                {
                                    attDataLen = attDataLen - consumedBytes;
                                    totalConsumedBytes += consumedBytes;

                                    /* Now find the value */
                                    if (CsrBtUtilSdpExtractUint(
                                            attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                            attDataLen, &returnValue, &consumedBytes, TRUE))
                                    {
                                        attDataLen = attDataLen - consumedBytes;
                                        totalConsumedBytes += consumedBytes;

                                        if (CSR_BT_L2CAP_PROTOCOL_UUID16_IDENTIFIER == protocolValue)
                                        {
                                            *psm = (CsrUint16) returnValue;

                                            if (CsrBtUtilSdpExtractUint(
                                                    attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                    attDataLen, &protocolValue, &consumedBytes, TRUE))
                                            {
                                                attDataLen = attDataLen - consumedBytes;
                                                totalConsumedBytes += consumedBytes;

                                                if (CSR_BT_OBEX_PROTOCOL_UUID16_IDENTIFIER == protocolValue)
                                                {
                                                    retBool = TRUE;
                                                    break;
                                                }
                                                else
                                                {
                                                    if (CsrBtUtilSdpExtractUint(
                                                            attribPtr + SDR_ENTRY_INDEX_ATTRIBUTE_DATA + totalConsumedBytes,
                                                            attDataLen, &returnValue, &consumedBytes, TRUE))
                                                    {
                                                        attDataLen = attDataLen - consumedBytes;
                                                        totalConsumedBytes += consumedBytes;
                                                    }
                                                    else
                                                    {
                                                        break;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                break;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return retBool;
}
#endif

#endif

