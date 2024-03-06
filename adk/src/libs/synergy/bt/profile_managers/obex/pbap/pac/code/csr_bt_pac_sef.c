/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_bt_result.h"
#include "csr_bt_pac_handler.h"
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_sdc_support.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_bt_pb_util.h"


/* Default application header parameter for Connect request */
#define CSR_BT_PAC_CONNECT_OBEX_APP_PAR_FLAG           \
    (PbAppParFlag) (CSR_BT_OBEX_PB_SUPP_FEATURES_FLAG)

/* Default application header parameter for Listing request */
#define CSR_BT_PAC_LISTING_OBEX_APP_PAR_FLAG              \
    (PbAppParFlag) (CSR_BT_OBEX_PB_ORDER_FLAG           | \
                    CSR_BT_OBEX_PB_SEARCH_VAL_FLAG      | \
                    CSR_BT_OBEX_PB_SEARCH_ATT_FLAG      | \
                    CSR_BT_OBEX_PB_MAX_LST_CNT_FLAG     | \
                    CSR_BT_OBEX_PB_LST_START_OFF_FLAG   | \
                    CSR_BT_OBEX_PB_VCARD_SEL_FLAG       | \
                    CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG    | \
                    CSR_BT_OBEX_PB_RST_MISSED_CALL_FLAG)

/* Default application header parameter for Pull phonebook request */
#define CSR_BT_PAC_PULL_PB_OBEX_APP_PAR_FLAG              \
    (PbAppParFlag) (CSR_BT_OBEX_PB_PROP_SEL_FLAG        | \
                    CSR_BT_OBEX_PB_FORMAT_FLAG          | \
                    CSR_BT_OBEX_PB_MAX_LST_CNT_FLAG     | \
                    CSR_BT_OBEX_PB_LST_START_OFF_FLAG   | \
                    CSR_BT_OBEX_PB_VCARD_SEL_FLAG       | \
                    CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG    | \
                    CSR_BT_OBEX_PB_RST_MISSED_CALL_FLAG)

/* Default application header parameter for Pull vcard request */
#define CSR_BT_PAC_PULL_VCARD_OBEX_APP_PAR_FLAG           \
    (PbAppParFlag) (CSR_BT_OBEX_PB_PROP_SEL_FLAG        | \
                    CSR_BT_OBEX_PB_FORMAT_FLAG)

/* Mandatory vCard 2.1 properties */
#define CSR_BT_PAC_VCARD2_1_PROPERTIES          \
                CSR_BT_PB_VCARD_PROP_VERSION  | \
                CSR_BT_PB_VCARD_PROP_N        | \
                CSR_BT_PB_VCARD_PROP_TEL

/* Mandatory vCard 2.1 properties */
#define CSR_BT_PAC_VCARD3_0_PROPERTIES          \
                CSR_BT_PB_VCARD_PROP_VERSION  | \
                CSR_BT_PB_VCARD_PROP_N        | \
                CSR_BT_PB_VCARD_PROP_TEL      | \
                CSR_BT_PB_VCARD_PROP_FN


static const CsrUint8 serviceRecord[] =
{
  /* Service class ID list */
  0x09, 0x00, 0x01, /* AttrID , ServiceClassIDList */
  0x35, 0x03, /* 3 bytes in total DataElSeq */
  0x19, 0x11, 0x2E, /* 2 byte UUID, Phonebook Access Uuid16 0x112E PBAP Client Class */

  /* BrowseGroupList */
  0x09, 0x00, 0x05, /* AttrId = BrowseGroupList */
  0x35, 0x03, /* Data element seq. 3 bytes */
  0x19, 0x10, 0x02, /* 2 byte UUID, PublicBrowseGroup = 0x1002 */

  /* bluetooth profile descriptor list */
  0x09, 0x00, 0x09, /* AttrId ProtocolDescriptorList */
  0x35, 0x08, /* 8 bytes in total DataElSeq */

  /* Supported profile */
  0x35, 0x06, /* 6 bytes in DataElSeq */
  0x19, 0x11, 0x30, /* 2 byte UUID, Supported profile 0x1130 Phonebook Access */

  /* Profile version */
  0x09,
  (CSR_BT_PAC_PROFILE_VERSION >> 8),
  (CSR_BT_PAC_PROFILE_VERSION & 0xFF), /* 2 byte UINT - Profile version = 0x0102 = version 1.2 */

  /* service name */
  0x09, 0x01, 0x00, /* AttrId - Service Name. Use language base attribute 0x0100 (primary language) */
  0x25, 20, /* length of service string */
  'P', 'h', 'o', 'n', 'e', 'b', 'o', 'o', 'k', ' ', 'A', 'c', 'c', 'e', 's', 's', ' ', 'P', 'C', 'E', /* string - "Phonebook Access PCE" */
};

void CsrBtPacMessagePut(CsrSchedQid phandle, void *msg)
{
    CsrSchedMessagePut(phandle, CSR_BT_PAC_PRIM, msg);
}

/******************************** Utility functions ********************************/
static void csrBtPacSetSelectors(CsrUint8 (*selector)[8], CsrUint32 property)
{
    CsrUint8 index = 0;
    CsrUint32 val = property;

    while (val >> 8)
    {
        val >>= 8;
        index++;
    }

    (*selector)[index] |= (CsrUint8) val;
}

static void csrBtPacResetSelector(CsrUint8 (*selector)[8], CsrUint32 property)
{
    CsrUint8 index = 0;
    CsrUint32 val = property;

    while (val >> 8)
    {
        val >>= 8;
        index++;
    }

    (*selector)[index] &= ~((CsrUint8) val);
}

static CsrUint16 csrBtPacAppParaSearchValLen(CsrCharString *searachVal)
{
    if (searachVal)
    {
        CsrUint16 len;
        len = (CsrUint16)CsrStrLen(searachVal);
        if (len)
        {
            return (CSR_BT_OBEX_PB_TAG_SIZE + len + 1);
        }
    }
    return (0);
}

static CmnCsrBtLinkedListStruct *csrBtPacCreateSdr(void)
{
    CsrUint16 index;
    CmnCsrBtLinkedListStruct *sdpTag = NULL;

    sdpTag = CsrBtUtilSdrCreateServiceHandleEntryFromUuid32(sdpTag,
                                                            CSR_BT_OBEX_PBA_SERVER_PROFILE_UUID,
                                                            &index);

    /* Read the SupportedRepositories of the remote server. */
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag,
                                         index,
                                         CSR_BT_SUPPORTED_REPOSITORIES_ATTRIBUTE_IDENTIFIER,
                                         NULL,
                                         0);

    /* Read the SupportedFeatures of the remote server. */
    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag,
                                         index,
                                         CSR_BT_OBEX_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER,
                                         NULL,
                                         0);

    CsrBtUtilSdrInsertLocalServerChannel(sdpTag, index, CSR_BT_NO_SERVER);

    CsrBtUtilSdrCreateAndInsertAttribute(sdpTag,
                                         index,
                                         CSR_BT_OBEX_L2CAP_PSM_ATTRIBUTE,
                                         NULL,
                                         0);

    return (sdpTag);
}

static void csrBtPacExtractSdpData(PacInst *pInst,
                                   CmnCsrBtLinkedListStruct *sdpTag)
{
    CsrUint32 supportedRepositories;

    if (CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTag,
                                                               0,
                                                               CSR_BT_OBEX_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER,
                                                               &pInst->supportedFeatures)
        == FALSE)
    {
        pInst->supportedFeatures = 0;
    }

    if (CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTag,
                                                               0,
                                                               CSR_BT_SUPPORTED_REPOSITORIES_ATTRIBUTE_IDENTIFIER,
                                                               &supportedRepositories)
        == FALSE)
    {
        pInst->supportedRepositories = 0;
    }
    else
    {
        pInst->supportedRepositories = (CsrBtPacSrcType) supportedRepositories;
    }
}

static void csrBtPacExtractApplicationHeaders(PacInst *pInst, CsrUint8 *pObexPacket)
{
    if (pObexPacket)
    {
        if (pInst->operation == CSR_BT_PAC_PULL_PB_REQ
            || pInst->operation == CSR_BT_PAC_PULL_VCARD_LIST_REQ
            || pInst->operation == CSR_BT_PAC_PULL_VCARD_ENTRY_REQ)
        { /* Valid state */
            CsrUint16 nLen;
            CsrUint16 nIndex;

            /* Locate application header in obex packet */
            nIndex = CsrBtObexHeaderIndex(CSR_BT_OBEX_REQUEST,
                                          pObexPacket,
                                          CSR_BT_OBEX_APPLICATION_PARAMETERS_HEADER,
                                          &nLen);

            if (nIndex && nLen > CSR_BT_OBEX_HEADER_LENGTH)
            {
                nLen += nIndex - 1; /* End of application header */
                nIndex += CSR_BT_OBEX_HEADER_LENGTH; /* Start of application header parameters */

                while (nIndex < nLen)
                {
                    CsrUint8 size;
                    CsrUint8 *src, *dest;

                    src = pObexPacket + nIndex + CSR_BT_OBEX_PB_TAG_SIZE; /* Application parameter value */

                    /* Determine the type and size of application parameter */
                    switch (pObexPacket[nIndex])
                    {
                        case CSR_BT_OBEX_PB_PHONEBOOK_SIZE_ID:
                        {
                            dest = (CsrUint8 *) &pInst->pbSize;
                            size = CSR_BT_OBEX_PB_PHONEBOOK_SIZE_VAL_LEN;
                            break;
                        }
                        case CSR_BT_OBEX_PB_MISSED_CALLS_ID:
                        {
                            dest = (CsrUint8 *) &pInst->newMissedCall;
                            size = CSR_BT_OBEX_PB_MISSED_CALLS_VAL_LEN;
                            break;
                        }
                        case CSR_BT_OBEX_PB_PRIM_VER_ID:
                        {
                            dest = (CsrUint8 *) pInst->versionInfo.primaryVersionCounter;
                            size = CSR_BT_OBEX_PB_PRIM_VER_VAL_LEN;
                            break;
                        }
                        case CSR_BT_OBEX_PB_SEC_VER_ID:
                        {
                            dest = (CsrUint8 *) pInst->versionInfo.secondaryVersionCounter;
                            size = CSR_BT_OBEX_PB_SEC_VER_VAL_LEN;
                            break;
                        }
                        case CSR_BT_OBEX_PB_DATABASE_ID_ID:
                        {
                            dest = (CsrUint8 *) pInst->versionInfo.databaseIdentifier;
                            size = CSR_BT_OBEX_PB_DATABASE_ID_VAL_LEN;
                            break;
                        }
                        default:
                        {
                            size = 0;
                            dest = NULL;
                            break;
                        }
                    }

                    /* Change Endianess and copy the application parameter value */
                    CsrBtPbReverseCopy(dest, src, size);

                    /* Move to next application parameter */
                    nIndex += pObexPacket[nIndex + 1]
                              + CSR_BT_OBEX_PB_TAG_SIZE;
                }
            }
        }
    }
}

static CsrBool csrBtPacValidFolder(CsrBtPacSrcType supportedRepository,
                                   CsrUint16 folderId)
{
    CsrUint8 id;
    if (folderId == CSR_BT_PB_FOLDER_INVALID_ID)
    {
        /* Invalid folder */
        return (FALSE);
    }

    id = folderId >> 8;
    if (id == CSR_BT_PB_FAV_ID)
    {
        /* We are asking for Favourites folder. Check if it is supported by server */
        if (!(supportedRepository & CSR_BT_PB_REPO_FAV))
        {
            return (FALSE);
        }
    }
    else if (id == CSR_BT_PB_SPD_ID)
    {
        /* We are asking for SpeedDialKey folder. Check if it is supported by server */
        if (!(supportedRepository & CSR_BT_PB_REPO_SPD))
        {
            return (FALSE);
        }
    }

    if (folderId & CSR_BT_PB_FOLDER_SIM1_ID)
    {
        /* We are asking for something from SIM phonebook. Check if it is supported by server */
        if (!(supportedRepository & CSR_BT_PB_REPO_SIM))
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

/* Enable access to properties only if they are supported by both local and remote device */
static void csrBtPacRegulateSelectors(CsrUint8 (*selector)[8], CsrUint32 supportedFeatures)
{
    /* Reset reserved bits */
    (*selector)[4] = 0;
    (*selector)[5] = 0;
    (*selector)[6] = 0;
    (*selector)[7] = 0;

    /* Unique caller identification*/
    if (!(supportedFeatures & CSR_BT_PB_FEATURE_UCI))
    {
        csrBtPacResetSelector(selector, CSR_BT_PB_VCARD_PROP_X_BT_UCI);
    }

    /* Unique Identifier */
    if (!(supportedFeatures & CSR_BT_PB_FEATURE_UID))
    {
        csrBtPacResetSelector(selector, CSR_BT_PB_VCARD_PROP_X_BT_UID);
    }
}

static void csrBtPacVerfiyAppHeaderParams(PacInst *pInst, PbAppParFlag *pFlag)
{
    CsrBool add;
    PbAppParFlag appParamFlag = *pFlag;

    /* No need to send optional application parameters which have default values */

    /* Verify if order parameter is really needed */
    if (pInst->order == CSR_BT_PB_ORDER_INDEXED)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_ORDER_FLAG;
    }

    /* Verify if search attribute and search value parameter are really needed */
    if (pInst->searchVal == NULL || (CsrStrLen(pInst->searchVal) == 0))
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_SEARCH_VAL_FLAG;
        appParamFlag &= ~CSR_BT_OBEX_PB_SEARCH_ATT_FLAG;
    }
    else if(pInst->searchAtt == CSR_BT_PB_SEARCH_ATT_NAME)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_SEARCH_ATT_FLAG;
    }

    /* Verify if max list count parameter is really needed */
    if(pInst->maxLstCnt == CSR_BT_PB_DEFAULT_MAX_LIST_COUNT)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_MAX_LST_CNT_FLAG;
    }

    /* Verify if list start parameter is really needed */
    if (pInst->lstStartOff == 0)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_LST_START_OFF_FLAG;
    }

    /* No need to send PropertySelector if all are zero. */
    if (pInst->propertySel && ((*pInst->propertySel)[0] || (*pInst->propertySel)[1]
                               || (*pInst->propertySel)[2] || (*pInst->propertySel)[3]))
    {
        if (pInst->format == CSR_BT_PB_FORMAT_VCARD3_0)
        {
            /* Adding mandatory properties for vCard 3.0 */
            csrBtPacSetSelectors(pInst->propertySel,
                                 CSR_BT_PAC_VCARD3_0_PROPERTIES);
        }
        else
        {
            /* Adding mandatory properties for vCard 2.1 */
            csrBtPacSetSelectors(pInst->propertySel,
                                 CSR_BT_PAC_VCARD2_1_PROPERTIES);
        }
    }
    else
    { /* No property selected in PropertySelector */
        appParamFlag &= ~CSR_BT_OBEX_PB_PROP_SEL_FLAG;
    }

    /* Verify if vCard format parameter is really needed */
    if(pInst->format == CSR_BT_PB_FORMAT_VCARD2_1)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_FORMAT_FLAG;
    }

    /* Verify if VcardSelector and VcardSelectorOperator parameters are really needed */
    if ((pInst->supportedFeatures & CSR_BT_PB_FEATURE_VCARD_SEL) && pInst->vcardSel
        && ((*pInst->vcardSel)[0] || (*pInst->vcardSel)[1]
            || (*pInst->vcardSel)[2] || (*pInst->vcardSel)[3]))
    {
        /* Remove VcardSelectorOperator if it is not AND */
        if (pInst->vcardSelOp != CSR_BT_PB_VCARD_SELECTOR_OPERATOR_AND)
        {
            appParamFlag &= ~CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG;
        }
    }
    else
    {
        /* No property selected in VcardSelector */
        appParamFlag &= ~CSR_BT_OBEX_PB_VCARD_SEL_FLAG;
        appParamFlag &= ~CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG;
    }


    /* Check if ResetNewMissedCalls is allowed */
    add = FALSE;
    if (pInst->rstMissedCall == CSR_BT_PB_RESET_NEW_MISSED_CALL)
    {
        if (pInst->supportedFeatures & CSR_BT_PB_FEATURE_MISSED_CALL)
        {
            /* Reset missed calls are applicable for both MCH and CCH */
            CsrUint8 folder = pInst->targetFolderId >> 8;
            if ((folder == CSR_BT_PB_MCH_ID) || (folder == CSR_BT_PB_CCH_ID))
            {
                add = TRUE;
            }
        }
    }
    if (add == FALSE)
    {
        appParamFlag &= ~CSR_BT_OBEX_PB_RST_MISSED_CALL_FLAG;
    }

    *pFlag = appParamFlag;
}

static CsrUint16 csrBtPacBuildAppHeaders(PacInst *pInst,
                                         CsrUint8 **pBuf,
                                         PbAppParFlag appParamFlag)
{
    CsrUint8 *buf;
    CsrUint16 appHeaderLength;
    CsrUint16 len = 0;

    /* Remove unnecessary parameters */
    csrBtPacVerfiyAppHeaderParams(pInst, &appParamFlag);

    /* Calculate the size of buffer required for application parameters */
    appHeaderLength = CsrBtPbApplicationHeaderLen(appParamFlag);
    if (appParamFlag & CSR_BT_OBEX_PB_SEARCH_VAL_FLAG)
    {
        /* We are sending search value as well. Increase buffer size */
        appHeaderLength += csrBtPacAppParaSearchValLen(pInst->searchVal);
    }

    buf = CsrPmemZalloc(appHeaderLength);
    *pBuf = buf;

    /* Add application parameter header */
    buf[0] = CSR_BT_OBEX_APPLICATION_PARAMETERS_HEADER;
    len = CSR_BT_OBEX_APP_PAR_HEADER_SIZE;

    /* Add application parameters */

    /* Order */
    if (appParamFlag & CSR_BT_OBEX_PB_ORDER_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_ORDER_ID,
                                     &pInst->order);
    }

    /* Search attribute value */
    if (appParamFlag & CSR_BT_OBEX_PB_SEARCH_VAL_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_SEARCH_VAL_ID,
                                     (CsrUint8 *)pInst->searchVal);
        /* Search attribute cannot be sent without search value. */
        if (appParamFlag & CSR_BT_OBEX_PB_SEARCH_ATT_FLAG)
        {
            len += CsrBtPbPutHeaderParam(buf + len,
            CSR_BT_OBEX_PB_SEARCH_ATT_ID,
                                         &pInst->searchAtt);
        }
    }

    /* Maximum list count */
    if (appParamFlag & CSR_BT_OBEX_PB_MAX_LST_CNT_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_MAX_LST_CNT_ID,
                                     (CsrUint8 *) &pInst->maxLstCnt);
    }

    /* List start offset */
    if (appParamFlag & CSR_BT_OBEX_PB_LST_START_OFF_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_LST_START_OFF_ID,
                                     (CsrUint8 *) &pInst->lstStartOff);
    }

    /* Property selector */
    if (appParamFlag & CSR_BT_OBEX_PB_PROP_SEL_FLAG)
    {
        /* Make sure we are not asking for unsupported properties */
        csrBtPacRegulateSelectors(pInst->propertySel, pInst->supportedFeatures);
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_PROP_SEL_ID,
                                     (CsrUint8 *) (*pInst->propertySel));
    }

    /* Vcard format */
    if (appParamFlag & CSR_BT_OBEX_PB_FORMAT_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_FORMAT_ID,
                                     &pInst->format);
    }

    /* Vcard selector */
    if (appParamFlag & CSR_BT_OBEX_PB_VCARD_SEL_FLAG)
    {
        /* Make sure we are not selecting by unsupported properties */
        csrBtPacRegulateSelectors(pInst->vcardSel, pInst->supportedFeatures);
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_VCARD_SEL_ID,
                                     (CsrUint8 *) (*pInst->vcardSel));

        /* Vcard selector operator can only be included if vcard selector is included. */
        if (appParamFlag & CSR_BT_OBEX_PB_VCARD_SEL_OP_FLAG)
        {
            len += CsrBtPbPutHeaderParam(buf + len,
                                         CSR_BT_OBEX_PB_VCARD_SEL_OP_ID,
                                         &pInst->vcardSelOp);
        }
    }

    /* Reset new missed calls */
    if (appParamFlag & CSR_BT_OBEX_PB_RST_MISSED_CALL_FLAG)
    {
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_RST_MISSED_CALL_ID,
                                     &pInst->rstMissedCall);
    }

    /* Supported features */
    if (appParamFlag & CSR_BT_OBEX_PB_SUPP_FEATURES_FLAG)
    {
        CsrUint32 supportedFeatures = CSR_BT_PAC_SUPPORTED_FEATURES & CSR_BT_PB_FEATURE_ALL;
        len += CsrBtPbPutHeaderParam(buf + len,
                                     CSR_BT_OBEX_PB_SUPP_FEATURES_ID,
                                     (CsrUint8 *) &supportedFeatures);
    }

    /* Add final application header length */
    buf[1] = len >> 8;
    buf[2] = len & 0x00FF; /* write current length */

    return (len);
}

/************************************* Common Ind/Cfm Handlers **************************************/
static void csrBtPacPullIndHandler(PacInst *pInst,
                                   CsrUint16 bodyOffset,
                                   CsrUint16 bodyLength,
                                   CsrUint8 *obexPacket,
                                   CsrUint16 obexPacketLength)
{
    switch (pInst->operation)
    {
        case CSR_BT_PAC_PULL_PB_REQ:
        {
            CsrBtPacPullPbIndSend(pInst,
                                  bodyOffset,
                                  bodyLength,
                                  obexPacket,
                                  obexPacketLength);
            break;
        }
        case CSR_BT_PAC_PULL_VCARD_LIST_REQ:
        {
            CsrBtPacPullvCardListIndSend(pInst,
                                         bodyOffset,
                                         bodyLength,
                                         obexPacket,
                                         obexPacketLength);
            break;
        }
        case CSR_BT_PAC_PULL_VCARD_ENTRY_REQ:
        {
            CsrBtPacPullvCardEntryIndSend(pInst,
                                          bodyOffset,
                                          bodyLength,
                                          obexPacket,
                                          obexPacketLength);
            break;
        }
        default:
        {
            /* Illegal state */
            break;
        }
    }
}

static void csrBtPacPullCfmHandler(PacInst *pInst,
                                   CsrBtObexResponseCode responseCode)
{
    switch (pInst->operation)
    {
        case CSR_BT_PAC_PULL_PB_REQ:
        {
            CsrBtPacPullPbCfmSend(pInst, responseCode);
            break;
        }
        case CSR_BT_PAC_PULL_VCARD_LIST_REQ:
        {
            CsrBtPacPullvCardListCfmSend(pInst, responseCode);
            break;
        }
        case CSR_BT_PAC_PULL_VCARD_ENTRY_REQ:
        {
            CsrBtPacPullvCardEntryCfmSend(pInst, responseCode);
            break;
        }
        default:
        {
            /* Illegal state */
            break;
        }
    }
}

/******************************** OBEX Callback functions **********************************/
static void csrBtPacSdpResultHandler(void *instData,
                                     CsrUint16 entriesInSdpTaglist,
                                     CmnCsrBtLinkedListStruct *sdpTagList)
{
    PacInst *pInst = (PacInst *) instData;
    CsrBool suppFeaturesAttr = FALSE;
    CsrUint16 *srvHandleList;
    CsrUint16 entries = 0;
    CsrUint16 i;
    CmnCsrBtLinkedListStruct *sdpTag;

    /* Select the relevant SDP records */
    sdpTag = sdpTagList;
    srvHandleList = CsrPmemZalloc(sizeof(CsrUint16) * entriesInSdpTaglist);
    for (i = 0; i < entriesInSdpTaglist && sdpTag; i++)
    {
        CsrBtUuid32 service;
        CsrUint16 result, task, taskResult;

        if (CsrBtUtilSdrGetServiceUuid32AndResult(sdpTag,
                                                  0,
                                                  &service,
                                                  &result,
                                                  &task,
                                                  &taskResult))
        {
            if (result == SDR_SDC_SEARCH_SUCCESS)
            {
                if ((suppFeaturesAttr == FALSE)
                    && (CsrBtUtilSdrGetUintAttributeDataValueFromAttributeUuid(sdpTag,
                                                                               0,
                                                                               CSR_BT_OBEX_SUPPORTED_FEATURES_ATTRIBUTE_IDENTIFIER,
                                                                               &pInst->supportedFeatures)
                        != FALSE))
                {
                    suppFeaturesAttr = TRUE;
                }

                /* We will attempt connection on all of them if there are more than one server */
                srvHandleList[entries++] = i;
            }
        }
        sdpTag = sdpTag->nextEntry;
    }

    if (suppFeaturesAttr != FALSE)
    {
        /* PBAP Server version >= 1.2 */
        CsrUint16 appParamHeaderLength = 0;
        CsrUint8 *appParamHeader = NULL;

        appParamHeaderLength = csrBtPacBuildAppHeaders(pInst,
                                                       &appParamHeader,
                                                       CSR_BT_PAC_CONNECT_OBEX_APP_PAR_FLAG);
        ObexUtilUpdateAppHeader(pInst->obexInst,
                                appParamHeaderLength,
                                &appParamHeader);
    }

    ObexUtilSetServiceHandleIndexListResponse(pInst->obexInst,
                                              &srvHandleList,
                                              entries);
}

static void csrBtPacConnectResultHandler(void *instData,
                                         CsrBtResultCode resultCode,
                                         CsrBtSupplier resultSupplier,
                                         CsrBtDeviceAddr deviceAddr,
                                         CsrBtConnId cid,
                                         CsrUint16 maxPeerObexPacketLength,
                                         CmnCsrBtLinkedListStruct *sdpTag,
                                         CsrUint16 obexPacketLength,
                                         CsrUint8 *obexPacket)
{
    PacInst *pInst = instData;

    pInst->curFolderId = 0;
    pInst->targetFolderId = 0;
    CsrBtPacResetLocalAppHeaderPar(pInst);
    CsrBtPacResetRemoteAppHeaderPar(pInst);

    csrBtPacExtractSdpData(pInst, sdpTag);

    CsrBtPacConnectCfmSend(pInst,
                           resultCode,
                           resultSupplier,
                           maxPeerObexPacketLength,
                           cid,
                           pInst->appHandle);

    if (resultCode == CSR_BT_OBEX_SUCCESS_RESPONSE_CODE &&
        resultSupplier == CSR_BT_SUPPLIER_IRDA_OBEX)
    {
        PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_CONNECTED);

        if (pInst->supportedFeatures != 0)
        {
            /* We can only use the features we support */
            pInst->supportedFeatures &= CSR_BT_PAC_SUPPORTED_FEATURES;
        }
        else
        {
            /* Supported features not found in server SDP record.
             * Hence assuming it supports basic browsing and download features */
            pInst->supportedFeatures = CSR_BT_PB_FEATURE_DOWNLOAD |
                                       CSR_BT_PB_FEATURE_BROWSING;
        }
    }
    else
    {
        /* Reset the address as connection was not successful */
        CsrMemSet(&pInst->deviceAddr, 0x00, sizeof(pInst->deviceAddr));
        PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_IDLE);
    }

    CsrBtUtilBllFreeLinkedList(&(sdpTag), CsrBtUtilBllPfreeWrapper);
    CsrPmemFree(obexPacket);

    CSR_UNUSED(obexPacketLength);
    CSR_UNUSED(deviceAddr);
}

static void csrBtPacAuthenticateIndHandler(void *instData,
                                           CsrBtDeviceAddr deviceAddr,
                                           CsrUint8 options,
                                           CsrUint16 realmLength,
                                           CsrUint8 *realm)
{
    PacInst * pInst = instData;

    CsrBtPacAuthenticateIndSend(pInst,
                                &deviceAddr,
                                options,
                                realmLength,
                                realm);
}

static void csrBtPacDisconnectResultHandler(void *instData,
                                            CsrBtReasonCode reasonCode,
                                            CsrBtSupplier reasonSupplier,
                                            CsrUint8 *obexPacket,
                                            CsrUint16 obexPacketLength)
{
    PacInst * pInst = instData;

    CsrBtPacDisconnectIndSend(pInst, reasonCode, reasonSupplier);
    CsrBtPacResetLocalAppHeaderPar(pInst);
    CsrMemSet(&pInst->deviceAddr, 0x00, sizeof(pInst->deviceAddr));
    PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_IDLE);

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static void csrBtPacCommonPullResultHandler(void *instData,
                                            CsrBtObexResponseCode responseCode,
                                            CsrBool bodyHeader,
                                            CsrUint16 bodyLength,
                                            CsrUint16 bodyOffset,
                                            CsrUint8 *obexPacket,
                                            CsrUint16 obexPacketLength)
{
    PacInst * pInst = instData;

    switch (responseCode)
    {
        case CSR_BT_OBEX_CONTINUE_RESPONSE_CODE:
        {
            csrBtPacExtractApplicationHeaders(pInst, obexPacket);
            if (bodyHeader && bodyLength > 0 && bodyOffset > 0)
            {
                csrBtPacPullIndHandler(pInst,
                                       bodyOffset,
                                       bodyLength,
                                       obexPacket,
                                       obexPacketLength);
                obexPacket = NULL;
            }
            else
            {
                ObexUtilGetContinueRequest(pInst->obexInst, pInst->srmp);
            }
            break;
        }
        case CSR_BT_OBEX_SUCCESS_RESPONSE_CODE:
        {
            csrBtPacExtractApplicationHeaders(pInst, obexPacket);
            if (bodyHeader && bodyLength > 0 && bodyOffset > 0)
            {
                csrBtPacPullIndHandler(pInst,
                                       bodyOffset,
                                       bodyLength,
                                       obexPacket,
                                       obexPacketLength);
                obexPacket = NULL;
                pInst->isFinal = TRUE;
            }
            else
            {
                csrBtPacPullCfmHandler(pInst, responseCode);
            }
            break;
        }
        default:
        {
            csrBtPacPullCfmHandler(pInst, responseCode);
            break;
        }
    }
    CsrPmemFree(obexPacket);
}

static void csrBtPacSetResultHandler(void *instData,
                                     CsrBtObexResponseCode responseCode,
                                     CsrUint8 *obexPacket,
                                     CsrUint16 obexPacketLength)
{
    PacInst * pInst = instData;

    if (responseCode == CSR_BT_OBEX_SUCCESS_RESPONSE_CODE)
    {
        /* Update current folder */
        pInst->curFolderId = pInst->targetFolderId;
    }
    switch (pInst->operation)
    {
        case CSR_BT_PAC_SET_FOLDER_REQ:
        {
            CsrBtPacSetFolderCfmSend(pInst, responseCode);
            break;
        }
        case CSR_BT_PAC_SET_BACK_FOLDER_REQ:
        {
            CsrBtPacSetBackFolderCfmSend(pInst, responseCode);
            break;
        }
        case CSR_BT_PAC_SET_ROOT_FOLDER_REQ:
        {
            CsrBtPacSetRootFolderCfmSend(pInst, responseCode);
            break;
        }
        default:
        {
            /* Should not happen */
            break;
        }
    }

    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

static void csrBtPacAbortResultHandler(void *instData,
                                       CsrBtObexResponseCode responseCode,
                                       CsrUint8 *obexPacket,
                                       CsrUint16 obexPacketLength)
{
    PacInst * pInst = instData;

    CsrBtPacAbortCfmSend(pInst);

    CSR_UNUSED(responseCode);
    CSR_UNUSED(obexPacketLength);
    CsrPmemFree(obexPacket);
}

/***** Functions which deliver the callback handlers defined statically in this file ******/
ObexUtilAuthenticateIndFuncType PacDeliverAuthenticateIndCb(void)
{
    return (&csrBtPacAuthenticateIndHandler);
}

ObexUtilDisconnectIndFuncType PacDeliverDisconnectIndCb(void)
{
    return (&csrBtPacDisconnectResultHandler);
}

/****************************** PAC primitive handlers ************************************/
CsrUint8 CsrBtPacConnectReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacConnectReq *req = (CsrBtPacConnectReq *) msg;
    CsrUint8 result         = CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;

    if (pInst->state == PAC_INSTANCE_STATE_IDLE)
    {
        const CsrUint8 targetUuid[] = CSR_BT_OBEX_TARGET_PB_UUID;
        ObexUtilDigestChallengeType *digestChal = NULL;
        CmnCsrBtLinkedListStruct    *sdpTag;
        CsrUint8                    *pTargetUuid;
        dm_security_level_t          secOutgoing;

        sdpTag = csrBtPacCreateSdr();
        pTargetUuid = CsrMemDup(targetUuid, sizeof(targetUuid));

        pInst->appHandle  = req->appHandle;
        pInst->deviceAddr = req->destination;

        PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_CONNECTING);

#ifndef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
        CsrBtScSetSecOutLevel(&secOutgoing,
                              CSR_BT_SEC_DEFAULT,
                              CSR_BT_PBAP_MANDATORY_SECURITY_OUTGOING,
                              CSR_BT_PBAP_DEFAULT_SECURITY_OUTGOING,
                              CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                              CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#else
        secOutgoing = pInst->secOutgoing;
#endif /* INSTALL_PAC_CUSTOM_SECURITY_SETTINGS */
        result = ObexUtilConnectRequest(pInst->obexInst,
                                       &sdpTag,
                                       req->destination,
                                       secOutgoing,
                                       req->maxPacketSize,
                                       CSR_BT_PAC_PROFILE_DEFAULT_MTU_SIZE,
                                       &pTargetUuid,
                                       sizeof(targetUuid),
                                       0,
                                       NULL,
                                       (CSR_BT_PAC_LP_SUPERVISION_TIMEOUT * 1000),
                                       &digestChal,
                                       0,
                                       0,
                                       NULL,
                                       0,
                                       NULL,
                                       csrBtPacConnectResultHandler,
                                       csrBtPacAuthenticateIndHandler,
                                       csrBtPacDisconnectResultHandler,
                                       csrBtPacSdpResultHandler,
                                       req->windowSize,
                                       TRUE, /* SRM is mandatory for PBAP v1.2 over GOEP v2.0 or above */
                                       CSRMAX(CSR_BT_SC_DEFAULT_ENC_KEY_SIZE,
                                              CSR_BT_PBAP_DEFAULT_ENC_KEY_SIZE_VAL));
    }
    else
    {
        /* Bad state: wrong request */
        CsrBtPacConnectCfmSend(pInst,
                               CSR_BT_OBEX_BAD_REQUEST_RESPONSE_CODE,
                               CSR_BT_SUPPLIER_OBEX_PROFILES,
                               0,
                               0,
                               req->appHandle);
    }

    return (result);
}

CsrUint8 CsrBtPacAuthenticateResHandler(PacInst *pInst, void *msg)
{
    CsrBtPacAuthenticateRes *res = (CsrBtPacAuthenticateRes *) msg;
    CsrUint8 userIdLength = 0;

    if (res->userId)
    {
        userIdLength = (CsrUint8) CsrStrLen((const char *) res->userId);
    }

    if (userIdLength > CSR_BT_OBEX_MAX_AUTH_USERID_LENGTH)
    {
        res->userId[CSR_BT_OBEX_MAX_AUTH_USERID_LENGTH] = '\0';
    }

    return (ObexUtilCliAuthenticateResponse(pInst->obexInst,
                                            res->passwordLength,
                                            &res->password,
                                            &res->userId,
                                            NULL));
}

CsrUint8 CsrBtPacPullPbReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullPbReq *req = (CsrBtPacPullPbReq *) msg;
    CsrUint8 result = CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;

    CsrBtPacResetRemoteAppHeaderPar(pInst);

    if (req->ucs2name)
    {
        CsrUint16 targetFolderId;

        if (req->src == CSR_BT_PB_REPO_SIM)
        {
            targetFolderId = CSR_BT_PB_FOLDER_SIM1_TELECOM_ID;
        }
        else
        {
            targetFolderId = CSR_BT_PB_FOLDER_TELECOM_ID;
        }

        targetFolderId = CsrBtPbGetFolderId(targetFolderId,
                                            req->ucs2name);

        if ((csrBtPacValidFolder(pInst->supportedRepositories, targetFolderId)
             != FALSE)
            && (targetFolderId & 0xFF00))
        { /* Valid folder */
            CsrUcs2String *absolutePath;
            CsrUint16 appParamHeaderLength;
            CsrUint8 *appParamHeader;
            CsrUint8 headerPriorityTable[5] =
            {
              CSR_BT_OBEX_UTIL_NAME_HEADER,
              CSR_BT_OBEX_UTIL_TYPE_HEADER,
              CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
              CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
              CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

            absolutePath = CsrBtPbGetFolderStr(CSR_BT_PB_FOLDER_ROOT_ID,
                                               targetFolderId,
                                               TRUE);

            /* Update current state variables */
            pInst->operation = CSR_BT_PAC_PULL_PB_REQ;
            pInst->isFinal = FALSE;
            pInst->targetFolderId = targetFolderId;
            if (req->srmpOn != FALSE)
            {
                pInst->srmp = CSR_BT_OBEX_SRMP_WAIT;
            }
            else
            {
                pInst->srmp = CSR_BT_OBEX_SRMP_INVALID;
            }

            CsrBtPacResetLocalAppHeaderPar(pInst);

            /* Store requested application header parameters.
             * These will be used while building application header */
            pInst->format = req->format;
            pInst->propertySel = (CsrUint8 (*)[8]) req->filter;
            pInst->maxLstCnt = req->maxListCnt;
            pInst->lstStartOff = req->listStartOffset;
            pInst->rstMissedCall = req->resetNewMissedCalls;
            pInst->vcardSel = (CsrUint8 (*)[8]) req->vCardSelector;
            pInst->vcardSelOp = req->vCardSelectorOperator;

            /* Build application header */
            appParamHeaderLength = csrBtPacBuildAppHeaders(pInst,
                                                           &appParamHeader,
                                                           CSR_BT_PAC_PULL_PB_OBEX_APP_PAR_FLAG);

            result = ObexUtilGetRequest(pInst->obexInst,
                                        headerPriorityTable,
                                        (const CsrUint8 *)CSR_BT_OBEX_TYPE_PB_PHONEBOOK,
                                        &absolutePath,
                                        NULL,
                                        appParamHeaderLength,
                                        &appParamHeader,
                                        0,
                                        NULL,
                                        pInst->srmp,
                                        csrBtPacCommonPullResultHandler,
                                        NULL);
        }
        else
        {
            /* Illegal folder */
            CsrBtPacPullPbCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
        }

        CsrPmemFree(req->ucs2name);
        req->ucs2name = NULL;/*Reset to NULL to prevent double free in-case of exceptions*/
    }
    else
    {
        /* Phonebook name missing */
        CsrBtPacPullPbCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
    }
    return (result);
}

CsrUint8 CsrBtPacPullPbResHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullPbRes *res = (CsrBtPacPullPbRes *) msg;

    if (pInst->isFinal)
    {
        CsrBtPacPullPbCfmSend(pInst, CSR_BT_OBEX_SUCCESS_RESPONSE_CODE);
        return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
    }
    else
    {
        pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                             CSR_BT_OBEX_SRMP_INVALID;
        return (ObexUtilGetContinueRequest(pInst->obexInst,
                                           pInst->srmp));
    }
}

CsrUint8 CsrBtPacSetFolderReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacSetFolderReq *req = (CsrBtPacSetFolderReq *) msg;
    CsrUint8 result = CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;
    CsrUint16 folder;
    CsrUint8 headerPriorityTable[3] =
    {
     CSR_BT_OBEX_UTIL_NAME_HEADER,
     CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
     CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

    if (req->ucs2name)
    {
        folder = CsrBtPbGetFolderId(pInst->curFolderId, req->ucs2name);

        if (csrBtPacValidFolder(pInst->supportedRepositories, folder) != FALSE)
        {
            pInst->targetFolderId = folder;
            pInst->operation = CSR_BT_PAC_SET_FOLDER_REQ;

            result = ObexUtilSetPathRequest(pInst->obexInst,
                                            0x02,
                                            headerPriorityTable,
                                            &req->ucs2name,
                                            NULL,
                                            0,
                                            NULL,
                                            csrBtPacSetResultHandler);
        }
        else
        {
            /* Illegal folder */
            CsrPmemFree(req->ucs2name);
            CsrBtPacSetFolderCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
        }
    }
    else
    {
        /* Illegal parameter */
        CsrBtPacSetFolderCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
    }

    return (result);
}

CsrUint8 CsrBtPacSetBackFolderReqHandler(PacInst *pInst, void *msg)
{
    CsrUint16 folder;
    CsrUint8 result = CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;
    CsrUint8 ucs2UpString[] = { 0, '.', 0, '.', 0, 0 };
    CsrUint8 headerPriorityTable[3] =
    {
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

    folder = CsrBtPbGetFolderId(pInst->curFolderId, ucs2UpString);

    if (csrBtPacValidFolder(pInst->supportedRepositories, folder) != FALSE)
    {
        pInst->targetFolderId = folder;
        pInst->operation = CSR_BT_PAC_SET_BACK_FOLDER_REQ;

        result = ObexUtilSetPathRequest(pInst->obexInst,
                                        0x03,
                                        headerPriorityTable,
                                        NULL,
                                        NULL,
                                        0,
                                        NULL,
                                        csrBtPacSetResultHandler);
    }
    else
    {
        /* Illegal folder */
        CsrBtPacSetBackFolderCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
    }
    CSR_UNUSED(msg);
    return (result);
}

CsrUint8 CsrBtPacSetRootFolderReqHandler(PacInst *pInst, void *msg)
{
    CsrUint8 headerPriorityTable[3] =
    {
      CSR_BT_OBEX_UTIL_EMPTY_NAME_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

    pInst->targetFolderId = CSR_BT_PB_FOLDER_ROOT_ID;
    pInst->operation = CSR_BT_PAC_SET_ROOT_FOLDER_REQ;

    CSR_UNUSED(msg);

    return (ObexUtilSetPathRequest(pInst->obexInst,
                                   0x02,
                                   headerPriorityTable,
                                   NULL,
                                   NULL,
                                   0,
                                   NULL,
                                   csrBtPacSetResultHandler));
}

CsrUint8 CsrBtPacPullvCardListReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullVcardListReq *req = (CsrBtPacPullVcardListReq*) msg;
    CsrUint8 result = CSR_BT_OBEX_UTIL_STATUS_ACCEPTED;
    CsrUint16 folderId;
    CsrUint16 appParamHeaderLength;
    CsrUint8 *appParamHeader;
    CsrUint8 headerPriorityTable[5] =
    {
     CSR_BT_OBEX_UTIL_TYPE_HEADER,
     CSR_BT_OBEX_UTIL_NAME_HEADER,
     CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
     CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
     CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

    CsrBtPacResetRemoteAppHeaderPar(pInst);

    folderId = CsrBtPbGetFolderId(pInst->curFolderId, req->ucs2name);

    if (csrBtPacValidFolder(pInst->supportedRepositories, folderId) != FALSE)
    {
        /* Update current state variables */
        pInst->targetFolderId = folderId;
        pInst->isFinal = FALSE;
        pInst->srmp = req->srmpOn != FALSE ?
                        CSR_BT_OBEX_SRMP_WAIT : CSR_BT_OBEX_SRMP_INVALID;

        CsrBtPacResetLocalAppHeaderPar(pInst);

        /* Store requested application header parameters.
         * These will be used while building application header */
        pInst->operation = CSR_BT_PAC_PULL_VCARD_LIST_REQ;
        pInst->order = req->order;
        pInst->searchVal = (CsrCharString *)req->searchVal;
        pInst->searchAtt = req->searchAtt;
        pInst->maxLstCnt = req->maxListCnt;
        pInst->lstStartOff = req->listStartOffset;
        pInst->rstMissedCall = req->resetNewMissedCalls;
        pInst->vcardSel = (CsrUint8 (*)[8]) req->vCardSelector;
        pInst->vcardSelOp = req->vCardSelectorOperator;

        appParamHeaderLength = csrBtPacBuildAppHeaders(pInst,
                                                       &appParamHeader,
                                                       CSR_BT_PAC_LISTING_OBEX_APP_PAR_FLAG);

        result = ObexUtilGetRequest(pInst->obexInst,
                                    headerPriorityTable,
                                    (const CsrUint8 *)CSR_BT_OBEX_TYPE_PB_LISTING,
                                    &req->ucs2name,
                                    NULL,
                                    appParamHeaderLength,
                                    &appParamHeader,
                                    0,
                                    NULL,
                                    pInst->srmp,
                                    csrBtPacCommonPullResultHandler,
                                    NULL);
    }
    else
    {
        /* Illegal folder */
        CsrPmemFree(req->ucs2name);
        CsrBtPacPullvCardListCfmSend(pInst, CSR_BT_OBEX_NOT_FOUND_RESPONSE_CODE);
    }

    /* searchVal needs to be deallocated here since it is copied into application header not referenced.
     * It also needs to be reset to prevent double deallocation in case of exceptions. */
    CsrPmemFree(req->searchVal);
    req->searchVal = NULL;

    return (result);
}

CsrUint8 CsrBtPacPullvCardListResHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullVcardListRes *res = (CsrBtPacPullVcardListRes *) msg;
    if (pInst->isFinal)
    {
        CsrBtPacPullvCardListCfmSend(pInst, CSR_BT_OBEX_SUCCESS_RESPONSE_CODE);
        return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
    }
    else
    {
        pInst->srmp = res->srmpOn != FALSE ?
                        CSR_BT_OBEX_SRMP_WAIT : CSR_BT_OBEX_SRMP_INVALID;
        return (ObexUtilGetContinueRequest(pInst->obexInst,
                                           pInst->srmp));
    }
}

CsrUint8 CsrBtPacPullvCardEntryReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullVcardEntryReq *req = (CsrBtPacPullVcardEntryReq *) msg;
    CsrUint16 appParamHeaderLength;
    CsrUint8 *appParamHeader;
    CsrUint8 headerPriorityTable[5] =
    {
      CSR_BT_OBEX_UTIL_TYPE_HEADER,
      CSR_BT_OBEX_UTIL_NAME_HEADER,
      CSR_BT_OBEX_UTIL_APP_PARAMETERS_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER,
      CSR_BT_OBEX_UTIL_UNDEFINE_HEADER };

    /* Update current state variables */
    pInst->operation = CSR_BT_PAC_PULL_VCARD_ENTRY_REQ;
    pInst->isFinal = FALSE;
    pInst->srmp = req->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                         CSR_BT_OBEX_SRMP_INVALID;

    CsrBtPacResetLocalAppHeaderPar(pInst);
    CsrBtPacResetRemoteAppHeaderPar(pInst);

    /* Store requested application header parameters.
     * These will be used while building application header */
    pInst->format = req->format;
    pInst->propertySel = (CsrUint8 (*)[8]) req->filter;
    pInst->targetFolderId = pInst->curFolderId;

    appParamHeaderLength = csrBtPacBuildAppHeaders(pInst,
                                                   &appParamHeader,
                                                   CSR_BT_PAC_PULL_VCARD_OBEX_APP_PAR_FLAG);

    return (ObexUtilGetRequest(pInst->obexInst,
                               headerPriorityTable,
                               (const CsrUint8 *)CSR_BT_OBEX_TYPE_PB_VCARD,
                               &req->ucs2name,
                               NULL,
                               appParamHeaderLength,
                               &appParamHeader,
                               0,
                               NULL,
                               pInst->srmp,
                               csrBtPacCommonPullResultHandler,
                               NULL));
}

CsrUint8 CsrBtPacPullvCardEntryResHandler(PacInst *pInst, void *msg)
{
    CsrBtPacPullVcardEntryRes *res = (CsrBtPacPullVcardEntryRes *) msg;
    if (pInst->isFinal)
    {
        CsrBtPacPullvCardEntryCfmSend(pInst, CSR_BT_OBEX_SUCCESS_RESPONSE_CODE);
        return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
    }
    else
    {
        pInst->srmp = res->srmpOn != FALSE ? CSR_BT_OBEX_SRMP_WAIT :
                                             CSR_BT_OBEX_SRMP_INVALID;
        return (ObexUtilGetContinueRequest(pInst->obexInst,
                                           pInst->srmp));
    }
}

CsrUint8 CsrBtPacAbortReqHandler(PacInst *pInst, void *msg)
{
    CSR_UNUSED(msg);
    return (ObexUtilAbortRequest(pInst->obexInst,
                                 NULL,
                                 csrBtPacAbortResultHandler));
}

CsrUint8 CsrBtPacDisconnectReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacDisconnectReq *req = (CsrBtPacDisconnectReq *) msg;
    return (ObexUtilDisconnectRequest(pInst->obexInst,
                                      req->normalDisconnect,
                                      NULL));
}

CsrUint8 CsrBtPacCancelConnectReqHandler(PacInst *pInst, void *msg)
{
    CSR_UNUSED(msg);
    return (ObexUtilDisconnectRequest(pInst->obexInst, FALSE, NULL));
}

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
CsrUint8 CsrBtPacSecurityOutReqHandler(PacInst *pInst, void *msg)
{
    CsrBtPacSecurityOutReq *req;
    CsrBtResultCode result;

    req = (CsrBtPacSecurityOutReq *) msg;
    result = CsrBtScSetSecOutLevel(&pInst->secOutgoing,
                                   req->secLevel,
                                   CSR_BT_PBAP_MANDATORY_SECURITY_OUTGOING,
                                   CSR_BT_PBAP_DEFAULT_SECURITY_OUTGOING,
                                   CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                                   CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
    CsrBtPacSecurityOutCfmSend(pInst, req, result);

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}
#endif

/******************************** CM Primitive handlers *************************************/
void CsrBtPacCmSdsRegisterReqHandler(PacInst *pInst)
{
    CsrUint8 *record = (CsrUint8 *) CsrMemDup(serviceRecord,
                                              sizeof(serviceRecord));

    CsrBtCmSdsRegisterReqSend(CSR_BT_PAC_IFACEQUEUE,
                                 record,
                                 sizeof(serviceRecord),
                                 CSR_BT_CM_CONTEXT_UNUSED);
	CSR_UNUSED(pInst);
}

void CsrBtPacCmSdsRegistertCfmHandler(PacInst *pInst, void *msg)
{
    CsrBtCmSdsRegisterCfm *cfm = (CsrBtCmSdsRegisterCfm *) msg;

    if (!(cfm->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS
        && cfm->resultSupplier == CSR_BT_SUPPLIER_CM))
    { /* The service record has not been register with success. */
        CsrGeneralException(CsrBtPacLto, 0, CSR_BT_CM_PRIM, CSR_BT_CM_SDS_REGISTER_CFM, cfm->resultCode, "SDS Registration has failed");
    }
}

#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
CsrUint8 PacRegisterQIDReqHandler(PacInst *pInst, void *msg)
{
    PacRegisterQidReq *prim = (PacRegisterQidReq *) msg;

    if (pInst->pacInstanceId == CSR_BT_PAC_IFACEQUEUE)
    {
        PacInstancePool *ptr = pInst->pacInstances;

        if (ptr == NULL)
        {
            ptr= pInst->pacInstances = CsrPmemZalloc(sizeof(PacInstancePool));
        }

        if (ptr->numberInPool < PAC_MAX_NUM_INSTANCES)
        {
            ptr->phandles[ptr->numberInPool++] = prim->pacInstanceId;
        }
        else
        {
            CsrPanic(CSR_TECH_BT,
                     CSR_BT_PANIC_MYSTERY,
                     "Pac Instance registration failed: Instance overflow");
        }
    }
    else
    {
        CsrPanic(CSR_TECH_BT,
                 CSR_BT_PANIC_MYSTERY,
                 "Pac instance registration failed: No Pac-manager instance");
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}
#endif /* INSTALL_PAC_MULTI_INSTANCE_SUPPORT */

CsrUint8 PacGetInstanceIdsReqHandler(PacInst *pInst, void *msg)
{
    if (pInst->pacInstanceId == CSR_BT_PAC_IFACEQUEUE)
    {
        PacGetInstanceIdsReq *req = (PacGetInstanceIdsReq *) msg;

        PacGetInstanceIdsCfmSend(pInst, req);
    }
    else
    {
        CsrGeneralException(CsrBtPacLto,
                            0,
                            CSR_BT_PAC_PRIM,
                            PAC_GET_INSTANCE_IDS_REQ,
                            0,
                            "It's not PAC-manager task");
    }

    return (CSR_BT_OBEX_UTIL_STATUS_ACCEPTED);
}
#endif /*EXCLUDE_CSR_BT_PAC_MODULE*/

