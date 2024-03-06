/******************************************************************************
 Copyright (c) 2009-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_cmn_sdc_rfc_util.h"
#include "csr_bt_cmn_sdp_lib.h"
#include "csr_bt_sdc_support.h"
#include "sdc_prim.h"

#define CSR_BT_SDC_RFC_UTIL_INST_ID_NOT_USED 0x00

#ifndef INSTALL_CMN_ENHANCED_SDP_FEATURE
#define cmnSdcRfcServiceSearchAttrIndHandler NULL
#define cmnSdcRfcServiceSearchAttrCfmHandler NULL
#endif

/* Static util functions    
   priInst MUST be Zalloc'ed before calling this function */
static void cmnSdcRfcInitPrivateInst(CmnSdcRfcPriInstType * priInst)
{ /* Initialize the private instance data used by the this library  */
/*    priInst->numOfSdrAttr               = 0;
    priInst->numOfSdrOutEntries         = 0;
    priInst->sdpInTagList               = NULL;
    priInst->sdpOutTagList              = NULL;
    priInst->sdrAttrIndex               = 0;
    priInst->sdrEntryIndex              = 0;
*/    
    priInst->uuidType                   = SDP_DATA_ELEMENT_SIZE_1_BYTE;
    priInst->serviceHandle              = CMN_SDR_INVALID_SERVICE_HANDLE;
    priInst->state                      = CMN_SDC_RFC_IDLE_STATE;
    priInst->secLevel                   = CSR_BT_SEC_DEFAULT;
    priInst->localServerCh              = CSR_BT_NO_SERVER;
    priInst->btConnId                   = CSR_BT_CONN_ID_INVALID;
/*
    priInst->reqPortPar                 = FALSE;
    priInst->validPortPar               = FALSE;
    priInst->obtainServer               = FALSE;
    priInst->maxFrameSize               = 0;
    priInst->numOfServiceHandleIndicis  = 0;
    priInst->serviceHandleIndexList     = NULL;
    CsrBtBdAddrZero(&priInst->deviceAddr);
*/
    CsrBtPortParDefault(&(priInst->portPar));
    priInst->modemStatus                = CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT;
    priInst->scTimeout                  = CSR_BT_DEFAULT_MSC_TIMEOUT;
}

static void cmnSdcRfcCleanupPrivateInst(CmnSdcRfcPriInstType * priInst)
{ /* Pfree all pointers used by the library                         */
    if (priInst)
    {
        CsrBtUtilBllFreeLinkedList(&priInst->sdpInTagList, CsrBtUtilBllPfreeWrapper);
        CsrBtUtilBllFreeLinkedList(&priInst->sdpOutTagList, CsrBtUtilBllPfreeWrapper);
        CsrPmemFree(priInst->serviceHandleIndexList);
        CsrPmemFree(priInst);
    }
    else
    { /* No data CsrPmemFree, just ignore                                 */
        ;
    }
}

#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
/* Certain phones expect the attributes to be requested in ascending order of their attribute IDs.
   Below function sorts the attribute list */
static void cmnSdcRfcSortAttrListByAttrIds(CsrUint16 *attrList,
                                           CsrUintFast16 attrCount)
{
    CsrUint16  i, j;
    /* Flag to break the loop if no internal swapping 
     * has happened for this iteration */
    CsrBool proceed = FALSE;

    for (i = 0; i < attrCount - 1; i++)
    {
        proceed = FALSE;

        for (j = 0; j < attrCount - i - 1; j++)
        {
            if (attrList[j] > attrList[j + 1])
            {
                /* Array is unsorted. Perform a swap and
                   set the flag to proceed with sorting */
                CsrUint16 temp;

                temp            = attrList[j];
                attrList[j]     = attrList[j + 1];
                attrList[j + 1] = temp;

                proceed = TRUE;
            }
        }

        if (!proceed)
        {
            /* No swap has been made indicating that the array is sorted */
            break;
        }
    }
}

static CsrBool cmnSdcRfcIsAttributePresent(CmnCsrBtLinkedListStruct *bll_p,
                                           CsrUint16 serviceIndex,
                                           CsrUint16 attrUuid)
{
    CsrUintFast16 numOfAttr;

    if (CsrBtUtilSdrGetNofAttributes(bll_p, serviceIndex, &numOfAttr))
    {
        if (numOfAttr > 0)
        {
            CsrUint16 t, attributeUuid;

            for (t = 0; t < numOfAttr; t++)
            {
                if (CsrBtUtilSdrGetAttributeUuid(bll_p, serviceIndex, t, &attributeUuid))
                {
                    if (attrUuid == attributeUuid)
                    {
                        return (TRUE);
                    }
                }
            }
        }
    }

    return (FALSE);
}

static CsrUintFast16 cmnSdcRfcGetMandatoryAttrCountToAdd(CmnCsrBtLinkedListStruct *sdpTag,
                                                         CsrUint8 serviceIndex,
                                                         CsrBool  obtainChannel)
{
    CsrUintFast16 count = 0;

    if (obtainChannel)
    {
        if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                         serviceIndex,
                                         CSR_BT_SERVICE_RECORD_HANDLE_ATTRIBUTE_IDENTIFIER))
        {
            count++;
        }
        if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                         serviceIndex,
                                         CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER))
        {
            count++;
        }
    }

    if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                     serviceIndex,
                                     CSR_BT_SERVICE_CLASS_ID_LIST))
    {
        count++;
    }

    return (count);
}

static void cmnSdcRfcAddMandatoryAttributes(CmnCsrBtLinkedListStruct * sdpTag,
                                            CsrUint16    *attrList,
                                            CsrUint8      serviceIndex,
                                            CsrUintFast16 totalAttrCount,
                                            CsrBool       obtainChannel)
{
    CsrUint8 tCount = totalAttrCount;

    /* For outgoing connection we need the remote server channel to establish the connection
       and the respective service handle to identify the channel when reading the attribute result.
       So add the required attributes to the sdpTagList if it is not already added */
    if (obtainChannel)
    {
        CsrBool enableExtendedSearch;

        if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                         serviceIndex,
                                         CSR_BT_SERVICE_RECORD_HANDLE_ATTRIBUTE_IDENTIFIER))
        {
            attrList[--tCount] = CSR_BT_SERVICE_RECORD_HANDLE_ATTRIBUTE_IDENTIFIER;
        }

        enableExtendedSearch = CsrBtUtilSdrPerformExtendedSearch(sdpTag, 0);

        if (enableExtendedSearch)
        {
            if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                             serviceIndex,
                                             CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER))
            {
                attrList[--tCount] = CSR_BT_ADDITIONAL_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER;
            }
        }
        else
        {
            if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                             serviceIndex,
                                             CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER))
            {
                attrList[--tCount] = CSR_BT_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_IDENTIFIER;
            }
        }
    }

    if (!cmnSdcRfcIsAttributePresent(sdpTag,
                                     serviceIndex,
                                     CSR_BT_SERVICE_CLASS_ID_LIST))
    {
        attrList[--tCount] = CSR_BT_SERVICE_CLASS_ID_LIST;
    }

    /* Let's sort the attributes list as per their IDs in ascending order */
    cmnSdcRfcSortAttrListByAttrIds(attrList, totalAttrCount);
}

static cmSdcSearchAttrInfo* cmnSdcRfcPrepareSearchAttrInfoList(CmnSdcRfcPriInstType *priInst,
                                                               CsrUint8 entries,
                                                               CsrBool  obtainChannel)
{
    cmSdcSearchAttrInfo *attrInfoList = NULL;
    CsrUint16           *attrList     = NULL;
    CsrUintFast16  i, j, numOfSdrAttr = 0;

    attrInfoList = (cmSdcSearchAttrInfo *) CsrPmemAlloc(sizeof(cmSdcSearchAttrInfo) * entries);

    for (i = 0; i < entries; i++)
    {
        CsrUintFast16 mAttrCount = cmnSdcRfcGetMandatoryAttrCountToAdd(priInst->sdpInTagList,
                                                                       i,
                                                                       obtainChannel);

        CsrBtUtilSdrGetNofAttributes(priInst->sdpInTagList, i, &numOfSdrAttr);

        attrList = (CsrUint16 *) CsrPmemAlloc(sizeof(CsrUint16 *) * (numOfSdrAttr + mAttrCount));

        for (j = 0; j < numOfSdrAttr; j++)
        {
            CsrBtUtilSdrGetAttributeUuid(priInst->sdpInTagList, i, j, &attrList[j]);
        }

        cmnSdcRfcAddMandatoryAttributes(priInst->sdpInTagList,
                                        attrList,
                                        i,
                                        (numOfSdrAttr + mAttrCount),
                                        obtainChannel);

        attrInfoList[i].noOfAttr = numOfSdrAttr + mAttrCount;
        attrInfoList[i].attrList = attrList;
    }

    return (attrInfoList);
}

#else /* ! INSTALL_CMN_ENHANCED_SDP_FEATURE */

static void cmnSdcStartLegacySearch(CmnSdcRfcInstType    *inst,
                                    CmnSdcRfcPriInstType *priInst,
                                    void                 *serviceList,
                                    CsrUint8              entries,
                                    CsrBool               enableExtendedSearch,
                                    CsrUint8              localServerChannel)
{
    if (priInst->uuidType == SDP_DATA_ELEMENT_SIZE_4_BYTES)
    {
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
        {
            if (!enableExtendedSearch)
            {
                CsrBtCmSdcRfcSearchReqSend(inst->appHandle, priInst->deviceAddr,
                                           serviceList, entries,
                                           localServerChannel);
            }
            else
            {
                CsrBtCmSdcRfcExtendedSearchReqSend(inst->appHandle, priInst->deviceAddr,
                                                   serviceList, entries,
                                                   localServerChannel);
            }
        }
        else
#endif /* ! EXCLUDE_CSR_BT_RFC_MODULE */
        {
            if (!enableExtendedSearch)
            {
                CsrBtCmSdcSearchReqSend(inst->appHandle, priInst->deviceAddr,
                                        serviceList, entries);
            }
            else
            {
                CsrBtCmSdcSearchUuidReqSend(inst->appHandle, priInst->deviceAddr,
                                            serviceList, entries);
            }
        }
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else
    { /* The Cm SDP 128bit search procedure must be called.
         The uuid128List must not be CsrPmemFreed, because the functions
         CsrBtCmSdcUuid128SearchReqSend/CsrBtCmSdcUuid128RfcSearchReqSend
         just passes uuid128List on                             */
#ifndef EXCLUDE_CSR_BT_RFC_MODULE
        if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
        {
            CsrBtCmSdcUuid128RfcSearchReqSend(inst->appHandle, priInst->deviceAddr,
                                              serviceList, entries,
                                              localServerChannel);
        }
        else
#endif /* ! EXCLUDE_CSR_BT_RFC_MODULE */
        {
            /* Start the Cm uuid128 Search procedure              */
            CsrBtCmSdcUuid128SearchReqSend(inst->appHandle, priInst->deviceAddr,
                                           serviceList, entries);
        }
    }
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */
}
#endif /* INSTALL_CMN_ENHANCED_SDP_FEATURE */

/* Lint warning 429: Custodial pointer 'serviceUuid128List' and 'serviceUuid32List' has not
   been freed or returned has been suprressed at the declaration of variables
  'serviceUuid128List' and 'serviceUuid32List' as these were freed in every possible scenario */
static CsrBool cmnSdcRfcStartSearch(CmnSdcRfcInstType    *inst,
                                    CmnSdcRfcPriInstType *priInst,
                                    CsrBool               obtainServerChannel)
{ /* Start an 32 or 128 bit CM SDP search if the given tag list,
     sdpInTagList, is correct                                       */

    /* Return the number of entries in the linked list, e.g. how
       many different UUID (32 or 128 bit) define in ServiceClass
       ID List                                                      */
    CsrUintFast16 entries = CsrBtUtilBllGetNofEntriesEx(priInst->sdpInTagList);

    /* Check if there is more different uuid's than the CM library
       can handle or there was not a single uuid. Return FALSE
       to indicate an error                                         */
    if (entries > 0 && entries < CMN_SDC_MAX_NUM_OF_UUID)
    {
        /* Retrieves the first UUID-type                            */
        CsrBool getServiceSuccess = CsrBtUtilSdrGetServiceUuidType(priInst->sdpInTagList,
                                                                   0,
                                                                   &priInst->uuidType);

        if (getServiceSuccess)
        { /* The number of different entries can be handle by the CM
             library                                                */
            CsrUintFast16    i;
            /*lint --e{429} */
            CsrBtUuid32  *serviceUuid32List  = NULL;
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
            CsrBtUuid128 *serviceUuid128List = NULL;
#endif
            void    *serviceList;
            CsrUint8 localServerChannel = CSR_BT_NO_SERVER;
            CsrUint8 uuidType = (priInst->uuidType == SDP_DATA_ELEMENT_SIZE_4_BYTES) ? CSR_BT_CM_SDC_UUID32 : CSR_BT_CM_SDC_UUID128;
            CsrBool  enableExtendedSearch = CsrBtUtilSdrPerformExtendedSearch(priInst->sdpInTagList, 0);
            cmSdcServiceSearchAttrInfo *svcSearchAttrInfoList;

            priInst->numOfSdrOutEntries = entries;

            /* Find out if a UUID 32bit or a UUID 128 bit CM SDP search
               must be started. Note that it is up to the calling
               process to ensure that only one type of UUID's, 32 or
               128 bit, can be present in the sdpInTagList list         */
            if (uuidType == CSR_BT_CM_SDC_UUID32)
            { /* A 32bit CM SDP search must be started.                 */
                serviceUuid32List  = (CsrBtUuid32 *) CsrPmemAlloc(sizeof(CsrBtUuid32) * entries);
            }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
            else
            { /* A 128bit CM SDP search must be started.                */
                serviceUuid128List = (CsrBtUuid128 *) CsrPmemAlloc(sizeof(CsrBtUuid128) * entries);
            }
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */
            for (i = 0; i < entries; i++)
            { /* Take a copy of the sdpInTagList and then copy it in the sdpOutTagList */
                CsrUint16 newIndex;

                priInst->sdpOutTagList = CsrBtUtilSdrCreateServiceHandleEntryFromTemplate(
                                                           priInst->sdpOutTagList,
                                                           &newIndex,
                                                           priInst->sdpInTagList,
                                                           i);
                if (uuidType == CSR_BT_CM_SDC_UUID32)
                { /* The first UUID-type were a 32 bit. Extract the
                     UUID and save it                                   */
                   if (!CsrBtUtilSdrGetServiceUuid32(priInst->sdpInTagList, i, &(serviceUuid32List[i])))
                   { /* Could not find a 32bit UUID-type. Pfree the
                        serviceUuid32List to prevent a memleak and return
                        FALSE to indicate an error                      */
                       CsrPmemFree(serviceUuid32List);
                       return (FALSE);
                   }
                }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
                else
                { /* The first UUID-type were a 128 bit. Extract the
                     UUID and save it                                   */
                   CsrBtUuid128 *uuid128;

                   if (!CsrBtUtilSdrGetServiceUuid128(priInst->sdpInTagList, i, &uuid128))
                   { /* Could not find a 128bit UUID-type. Pfree the
                        serviceUuid128List to prevent a memleak and return
                        FALSE to indicate an error                      */
                       CsrPmemFree(serviceUuid128List);
                       return (FALSE);
                   }
                   else
                   { /* SynMemCpyS uuid128 to the uuid128List           */
                        SynMemCpyS((serviceUuid128List[i]), sizeof(CsrBtUuid128), uuid128, sizeof(CsrBtUuid128));
                   }
                }
#endif /* CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH */
            }

            serviceList = (uuidType == CSR_BT_CM_SDC_UUID32) ? (void*)serviceUuid32List : (void*)serviceUuid128List;

#ifndef EXCLUDE_CSR_BT_RFC_MODULE
            if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
            {
                /* Use localServerCh as a context identifier if inst->instId is not valid */
                localServerChannel = (inst->instId == CSR_BT_SDC_RFC_UTIL_INST_ID_NOT_USED) ?
                                      priInst->localServerCh : (CsrUint8)inst->instId;
            }
#endif

#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
            /* Enhanced SDP : Service search attribute feature is enabled */
            svcSearchAttrInfoList = (cmSdcServiceSearchAttrInfo *) CsrPmemAlloc(sizeof(cmSdcServiceSearchAttrInfo));
            /* Retrieve and fill the structure with the attribute lists and number of attributes
               for the respective service UUIDs from the tag list */
            svcSearchAttrInfoList->attrInfoList    = cmnSdcRfcPrepareSearchAttrInfoList(priInst,
                                                                             entries,
                                                                             obtainServerChannel);
            svcSearchAttrInfoList->serviceList     = serviceList;
            svcSearchAttrInfoList->serviceListSize = entries;

            /* Time to call service search attribute CM API */
            CmSdcServiceSearchAttrReqSend(inst->appHandle,
                                          priInst->deviceAddr,
                                          svcSearchAttrInfoList,
                                          enableExtendedSearch,
                                          uuidType,
                                          localServerChannel);
#else
            /* Fall back to old legacy SDP design */
            cmnSdcStartLegacySearch(inst,
                                    priInst,
                                    serviceList,
                                    entries,
                                    enableExtendedSearch,
                                    localServerChannel);
            CSR_UNUSED(obtainServerChannel);
            CSR_UNUSED(svcSearchAttrInfoList);
#endif /* INSTALL_CMN_ENHANCED_SDP_FEATURE */

            /* Return TRUE to indicate that the CM SDP search is started*/
            return (TRUE);
        }
        else
        {
            /* UUID type was not retreived successfully. Unable to proceed with SDP */
        }
    }

    return (FALSE);
}

static void cmnSdcRfcInsertSearchIndResult(CmnSdcRfcPriInstType    * priInst,
                                           CsrBtUuid32                * serviceHandleList,
                                           CsrUint16                index,
                                           CsrUint16                serviceHandleListCount,
                                           CsrBool                  servicePresent,
                                           CsrBtUuid32                * service32,
                                           CsrBtUuid128               * service128)
{
    if (servicePresent)
    { /* The read service is present in the tag structure           */
        CsrUint16 i;

        for (i = 0; i < (serviceHandleListCount - 1); i++)
        { /* If the remote device support more than one instance
             of this service, make sure that sdpOutTagList contain
             this information                                       */
            CsrUint16 newIndex;
            priInst->sdpOutTagList =
                CsrBtUtilSdrCreateServiceHandleEntryFromTemplate(priInst->sdpOutTagList, &newIndex,
                                                       priInst->sdpOutTagList, index);
        }

        for (i = 0; i < serviceHandleListCount; i++)
        { /* Insert the service handle in the list                  */
            if (service32)
            { /* The service is 32bit UUID-type                     */
                CsrBtUtilSdrInsertServiceHandleAtUuid32(priInst->sdpOutTagList,
                                *service32, serviceHandleList[i]);
            }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
            else
            { /* The service is 128bit UUID-type                    */
                CsrBtUtilSdrInsertServiceHandleAtUuid128(priInst->sdpOutTagList,
                                service128, serviceHandleList[i]);
            }
#else
        CSR_UNUSED(service128);
#endif
        }
    }
    else
    { /* The read service is not present in the tag structure       */
        ;
    }
}

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
static CsrUint8 cmnGetLocalServerChannel(CmnCsrBtLinkedListStruct * sdpTag)
{
    CsrUint16 serverCh   = CSR_BT_NO_SERVER;
    CsrUintFast16 i;
    CsrUintFast16 entries    = CsrBtUtilBllGetNofEntriesEx(sdpTag);

    for (i = 0; i < entries; i++)
    {
        if (CsrBtUtilSdrGetLocalServerChannel(sdpTag, i, &serverCh))
        {
            if (serverCh != CSR_BT_NO_SERVER)
            {
                return ((CsrUint8) serverCh);
            }
        }
    }
    return ((CsrUint8) serverCh);
}

static CsrBool cmnSdcRfcGetSelectedServiceHandle(CmnSdcRfcPriInstType * priInst)
{
    CsrUintFast16 i;

    for (i = priInst->sdrEntryIndex; i < priInst->numOfServiceHandleIndicis; i++)
    {
        CsrUintFast16 index = priInst->serviceHandleIndexList[i];

        if (index < priInst->numOfSdrOutEntries)
        { /* The selected entry number is valid, try to retrieve the
             service handle. If this func returns TRUE a valid
             service record is found, other not                     */
            if (CsrBtUtilSdrGetServiceHandle(priInst->sdpOutTagList, index,
                                                &priInst->serviceHandle))
            {
                CsrUint16 serverCh   = CSR_BT_NO_SERVER;
                if (CsrBtUtilSdrGetLocalServerChannel(priInst->sdpOutTagList, index, &serverCh))
                {
                    if (serverCh != CSR_BT_NO_SERVER)
                    {
                        priInst->localServerCh  = (CsrUint8) serverCh;
                        priInst->sdrEntryIndex  = (CsrUint8) i;
                        return TRUE;
                    }
                }
            }
        }
        else
        { /* The selected entry number is invalid. Just ignore
             the given index                                        */
            ;
        }
    }

    /* Return FALSE to indicate that the connect procedue failed    */
    return FALSE;
}

static CsrBool cmnSdcRfcGetServiceHandle(CmnSdcRfcPriInstType * priInst)
{
    CsrUintFast16 i;

    for (i = priInst->sdrEntryIndex; i < priInst->numOfSdrOutEntries; i++)
    {
        if (CsrBtUtilSdrGetServiceHandle(priInst->sdpOutTagList, i, &priInst->serviceHandle))
        {
            priInst->sdrEntryIndex  = (CsrUint8) i;
            return TRUE;
        }
        else
        { /* Could no find any valid serviceHandle in the
             sdpOutTagList structure. Just ignore this entry        */
            ;
        }
    }
    /* Return FALSE to indicate all service handle has been read    */
    priInst->sdrEntryIndex = (CsrUint8) i;
    return FALSE;
}
#endif /* EXCLUDE_CSR_BT_RFC_MODULE*/

static CsrBool cmnSdcRfcGetAttributeValue(CmnSdcRfcPriInstType * priInst,
                                         CsrBtUuid32             * serviceHandle,
                                         CsrUint16             * attrId)
{ /* Search for a service handle which was a SDP attributes to read */
    CsrBtUuid32 serviceHd;
    CsrUintFast16 i, numOfSdrAttr;
    CsrBool getAtt;

    for (i = priInst->sdrEntryIndex; i < priInst->numOfSdrOutEntries; i++)
    {
        if (CsrBtUtilSdrGetServiceHandle(priInst->sdpOutTagList, i, &serviceHd))
        { /* Find out how many SDP attributes must be read from the
             serviceHandle                                          */
            priInst->sdrAttrIndex = 0;

            numOfSdrAttr = priInst->numOfSdrAttr;
            getAtt = CsrBtUtilSdrGetNofAttributes(priInst->sdpOutTagList, i, &numOfSdrAttr);
            priInst->numOfSdrAttr = (CsrUint8) numOfSdrAttr;

            if (getAtt && priInst->numOfSdrAttr > 0)
            { /* At least one SDP attribute must be read from this
                 serviceHandle                                      */
                CsrUint16 attrValue;
                if (CsrBtUtilSdrGetAttributeUuid(priInst->sdpOutTagList, i, priInst->sdrAttrIndex, &attrValue))
                { /* Found the SDP attribute value. Return TRUE
                     to indicate that an attribute must be read
                     from the servicehandle                         */
                    *serviceHandle          = serviceHd;
                    *attrId                 = attrValue;
                    priInst->sdrEntryIndex  = (CsrUint8) i;
                    return TRUE;
                }
            }
            else
            { /* No attribute, try next entry                       */
                ;
            }
        }
        else
        { /* Could no find any valid serviceHandle in the
             sdpOutTagList structure. Just ignore this entry        */
            ;
        }
    }
    /* Return FALSE to indicate all attributes has been read        */
    priInst->sdrEntryIndex = (CsrUint8) i;
    return FALSE;
}

static void cmnSdcRfcReadAttrOrClose(CmnSdcRfcInstType    * inst,
                                     CmnSdcRfcPriInstType * priInst)
{
    CsrBtUuid32 serviceHandle;
    CsrUint16 attrId;

    if (cmnSdcRfcGetAttributeValue(priInst, &serviceHandle, &attrId))
    { /* Need to read an attribute                                  */
        priInst->state          = CMN_SDC_RFC_ATTR_STATE;
        priInst->serviceHandle  = serviceHandle;
        CsrBtCmSdcAttributeReqSend(serviceHandle, attrId, L2CA_MTU_DEFAULT);
    }
    else
    { /* No attributes to read, close the SDP channel               */
        priInst->state = CMN_SDC_RFC_CLOSE_SEARCH_STATE;
        CsrBtCmSdcCloseReqSend(inst->appHandle);
    }
}

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
static CsrBool cmnSdcRfcErrorResultHandler(CsrBtResultCode      resultCode,
                                           CsrBtSupplier  resultSupplier)
{
    CsrBool initiateConnect = FALSE;

    /* Initiate connect if supplier of error is rfcomm only
       and are not one of the following errors */
    if ( (resultSupplier == CSR_BT_SUPPLIER_RFCOMM) && 
         (resultCode != RFC_CONNECTION_REJ_SECURITY) )
    {
        initiateConnect = TRUE;
    }

    return initiateConnect;
}
#endif

static void cmnSdcRfcCallResultFunc(void                 * instData,
                                    CmnSdcRfcInstType    * inst,
                                    CmnSdcRfcPriInstType * priInst,
                                    CsrBtResultCode      resultCode,
                                    CsrBtSupplier  resultSupplier,
                                    CsrBool               releaseResources)
{
    /* The main SDC instance "inst" can be freed anytime in the callback function. 
     * So decouple it from it's private instance by setting inst->privateInst to NULL 
     * since it is already available as "priInst" and would get freed at the end of this function */
    inst->privateInst = NULL;

    if (inst->cbSupportMask & CSR_SDC_OPT_CB_SEARCH_RESULT_MASK)
    {
        CsrSdcResultFuncType context;

        /* Call the function which indicates that Cm SDP search
                 procedure is finish                                        */

        if (resultCode      == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            resultSupplier  == CSR_BT_SUPPLIER_CM)
        { /* The search procedure is finish with SUCCESS. Set
             priInst->sdpOutTagList NULL because the profiles will
             be the owner of the pointer                            */

            context.instData = instData;
            context.sdpTagList = priInst->sdpOutTagList;
            context.deviceAddr = priInst->deviceAddr;
            context.resultCode = resultCode;
            context.resultSupplier = resultSupplier;

            priInst->sdpOutTagList = NULL;

            inst->sdcRfcResultFunc(CSR_SDC_OPT_CB_SEARCH_RESULT, (void *)&context);
        }
        else
        { /* The search procedure had failed                        */

            context.instData = instData;
            context.sdpTagList = NULL;
            context.deviceAddr = priInst->deviceAddr;
            context.resultCode = resultCode;
            context.resultSupplier = resultSupplier;

            inst->sdcRfcResultFunc(CSR_SDC_OPT_CB_SEARCH_RESULT, (void *)&context);
        }
    }
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
    else
    {
        CsrRfcConResultType context;

        if (releaseResources)
        { /* Request the CM to release its resources                */
            if (inst->instId == CSR_BT_SDC_RFC_UTIL_INST_ID_NOT_USED)
            {
                CsrBtCmSdcReleaseResourcesReqSend(inst->appHandle,
                                                  priInst->deviceAddr,
                                                  priInst->localServerCh);
            }
            else
            {
                CsrBtCmSdcReleaseResourcesReqSend(inst->appHandle,
                                                  priInst->deviceAddr,
                                                  (CsrUint8)inst->instId);
            }
        }

        /* Call the function which indicates that connect procedure
         is finish                                                  */
        if (resultCode      == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            resultSupplier  == CSR_BT_SUPPLIER_CM)
        { /* The connect procedure is finish with success. Copy one
             ready, and copy one sdpTag out of the sdpTagList. The
             owner of the sdpTag                                    */
            CmnCsrBtLinkedListStruct * sdpTag;
            CsrUint8                 * data;
            CsrUint16                dataLen;
            CsrUintFast16            serviceHandleIndex;

            if (inst->cbSupportMask & CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK)
            {
                /* The profile has selected a specific service record
                   handle list. Find the same SDP tag from the list */
                serviceHandleIndex = priInst->serviceHandleIndexList[priInst->sdrEntryIndex];
            }
            else
            {
                serviceHandleIndex = priInst->sdrEntryIndex;
            }
            data    = CsrBtUtilBllGetDataPointerEx(priInst->sdpOutTagList, serviceHandleIndex, &dataLen);
            sdpTag  = CsrBtUtilBllCreateNewEntry(NULL, data, dataLen);
            CsrBtUtilBllSetDataPointerEx(priInst->sdpOutTagList, serviceHandleIndex, NULL, 0);

            context.instData = instData;
            context.localServerCh = priInst->localServerCh;
            context.btConnId = priInst->btConnId;
            context.deviceAddr = priInst->deviceAddr;
            context.maxFrameSize = priInst->maxFrameSize;
            context.validPortPar = priInst->validPortPar;
            context.portPar = &priInst->portPar;
            context.resultCode = resultCode;
            context.resultSupplier = resultSupplier;
            context.sdpTag = sdpTag;

            inst->sdcRfcResultFunc(CSR_SDC_OPT_RFC_CON_RESULT, (void *)&context);
        }
        else
        { /* The connect procedure had failed                       */
            if (priInst->localServerCh != CSR_BT_NO_SERVER &&
                priInst->obtainServer                   )
            { /* Need to unregister the local serverCh again        */
                CsrBtCmUnRegisterReqSend(priInst->localServerCh);
            }
            else
            { /* Did not get any local serverChannel, e.g.
                 nothing to unregister                              */
                ;
            }

            context.instData = instData;
            context.localServerCh = priInst->localServerCh;
            context.btConnId = CSR_BT_CONN_ID_INVALID;
            context.deviceAddr = priInst->deviceAddr;
            context.maxFrameSize = 0;
            context.validPortPar = FALSE;
            context.portPar = &priInst->portPar;
            context.resultCode = resultCode;
            context.resultSupplier = resultSupplier;
            context.sdpTag = NULL;

            inst->sdcRfcResultFunc(CSR_SDC_OPT_RFC_CON_RESULT, (void *)&context);
        }
    }
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

    /* The Search or connect procedure is finished, free
       all data used by intern by the util lib                      */
    cmnSdcRfcCleanupPrivateInst(priInst);
}

static CsrBool cmnReleaseResources(CmnSdcRfcInstType     * inst,
                                  CsrBtResultCode       resultCode,
                                  CsrBtSupplier   resultSupplier)
{
#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
    CSR_UNUSED(resultCode);
    CSR_UNUSED(resultSupplier);
    /* Check for resultCode to be CSR_BT_RESULT_CODE_CM_SUCCESS is not required as in case of
     * INSTALL_CMN_ENHANCED_SDP_FEATURE we don't mask the SDC search failures.
     * In case of outgoing RFC search request, CM doesn't immediately free the resources,
     * instead it expects the common utility to trigger the release of resources.*/
    if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
#else
    /* In case of SDC_OPEN_SEARCH_CFM failure, CM would have already released the resources
     * whereas in case of search completion(success/failure), SDC_CLOSE_SEARCH_IND is received.
     * Here the result of search is masked with CSR_BT_RESULT_CODE_CM_SUCCESS and the CM doesn't 
     * free the resources for outgoing RFC Search request immediately and instead it expects
     * the common utility to trigger the release of resources. */
    if ((inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK) &&
        resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        resultSupplier  == CSR_BT_SUPPLIER_CM)
#endif
    { /* The cm resources must be free                              */
        return TRUE;
    }
    else
    { /* No cm resources are allocated                              */
        return FALSE;
    }
}


static CsrBool cmnSdcRfcStartConnect(void                 * instData,
                                    CmnSdcRfcInstType    * inst,
                                    CmnSdcRfcPriInstType * priInst)
{
    if (priInst->numOfSdrOutEntries > 0)
    { /* At least one entry is valid                                */
        priInst->sdrEntryIndex  = 0;

        if (inst->cbSupportMask & CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK)
        {
            CsrRfcConSelectServiceHandleType context;
            /* The profile has requested to select a service record
             handle index, call the function which allow the
             profile to do this                                     */
            priInst->state = CMN_SDC_RFC_SELECT_SERVICE_RECORD_HANDLE_STATE;

            context.instData = instData;
            context.cmSdcRfcInstData = inst;
            context.deviceAddr = priInst->deviceAddr;
            context.serverChannel = priInst->localServerCh;
            context.entriesInSdpTaglist = priInst->numOfSdrOutEntries;
            context.sdpTagList = priInst->sdpOutTagList;

            inst->sdcRfcResultFunc(CSR_SDC_OPT_CB_CON_SELECT_SERVICE_HANDLE, (void *)&context);

            return TRUE;
        }
        else
        { /* The service record handle must be auto selected by the
             CM RFC connect procedure                               */
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
            if (cmnSdcRfcGetServiceHandle(priInst))
            { /* A service record handle has been found. Request CM
                 to initiate an RFC connection                      */
                priInst->state          = CMN_SDC_RFC_CONNECT_STATE;

                CsrBtCmContextConnectReqSend(inst->appHandle, priInst->localServerCh,
                                             priInst->serviceHandle, priInst->maxFrameSize,
                                             priInst->reqPortPar, priInst->validPortPar,
                                             priInst->portPar, priInst->secLevel, priInst->deviceAddr,
                                             inst->instId, priInst->modemStatus, CSR_BT_DEFAULT_BREAK_SIGNAL,
                                             priInst->scTimeout, priInst->minEncKeySize);

                return TRUE;
            }
            else
#endif /*EXCLUDE_CSR_BT_RFC_MODULE */
            { /* No service record handle has been found, the
                 connect procedure failed                           */
                return FALSE;
            }
        }
    }
    else
    { /* Not a single Sdr entry is valid, the connect
         procedure failed                                           */
        return FALSE;
    }
}

/* Static CM upstream messages handler function                     */
static CsrBool cmnSdcRfcSearchIndHandler(void                 * instData,
                                        CmnSdcRfcInstType    * inst,
                                        CmnSdcRfcPriInstType * priInst,
                                        void                 * msg)
{ /* Has read a uuid32 service, save info and wait for a
     CSR_BT_CM_SDC_SEARCH_CFM message                                      */
    CsrUint16 index;
    CsrBtCmSdcSearchInd * prim = (CsrBtCmSdcSearchInd *) msg;

    /* Find the return index to where the return service is place
       in the priInst->sdpOutTagList                                */
    CsrBool servicePresent = CsrBtUtilSdrGetIndexForServiceUuid32
                                (priInst->sdpOutTagList, prim->service, &index);

    /* Save the return service record handle and if the remote
       device support support more than one instance of the found
       service this information the sdpOutTagList is updated        */
    cmnSdcRfcInsertSearchIndResult(priInst, prim->serviceHandleList,
                                   index, prim->serviceHandleListCount,
                                   servicePresent, &prim->service, NULL);

    CsrPmemFree(prim->serviceHandleList);
    prim->serviceHandleList = NULL;

    CSR_UNUSED(inst);
    CSR_UNUSED(instData);
    return FALSE;
}

#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
static CsrBool cmnSdcRfcUuid128SearchIndHandler(void                 * instData,
                                               CmnSdcRfcInstType    * inst,
                                               CmnSdcRfcPriInstType * priInst,
                                               void                 * msg)
{ /* Has read a uuid128 service, save info and wait for a
     CSR_BT_CM_SDC_SEARCH_CFM message                                      */
    CsrUint16 index;
    CsrBtCmSdcUuid128SearchInd *prim  = (CsrBtCmSdcUuid128SearchInd *) msg;

    /* Find the return index to where the return service is place
       in the priInst->sdpOutTagList                                */
    CsrBool servicePresent = CsrBtUtilSdrGetIndexForServiceUuid128
                              (priInst->sdpOutTagList, &prim->service, &index);

    /* Save the return service record handle and if the remote
       device support support more than one instance of the found
       service this information the sdpOutTagList is updated        */
    cmnSdcRfcInsertSearchIndResult(priInst, prim->serviceHandleList,
                                   index, prim->serviceHandleListCount,
                                   servicePresent, NULL, &prim->service);

    CsrPmemFree(prim->serviceHandleList);
    prim->serviceHandleList = NULL;

    CSR_UNUSED(inst);
    CSR_UNUSED(instData);
    return FALSE;
}
#endif
static CsrBool cmnSdcRfcSearchCfmHandler(void                 * instData,
                                        CmnSdcRfcInstType    * inst,
                                        CmnSdcRfcPriInstType * priInst,
                                        void                 * msg)
{ /* The CM has found all of the requested service that the remote
     device support                                                 */
    if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
    { /* The SDP search has been cancelled, Close the SDP channel   */
        CsrBtCmSdcCloseReqSend(inst->appHandle);
    }
    else
    { /* Remove all none succesful entries. At least one entry must
         be vaild otherwise the CSR_BT_CM_SDC_SEARCH_CFM message is never
         received. Start determined if any of these entries has some
         attributes that must be read. If not the SDP channel is
         closed                                                     */
        priInst->sdrEntryIndex      = 0;
        priInst->sdpOutTagList      = CsrBtUtilSdrRemoveNonSuccessStatusEntries(priInst->sdpOutTagList);
        priInst->numOfSdrOutEntries = (CsrUint8)CsrBtUtilBllGetNofEntriesEx(priInst->sdpOutTagList);
        cmnSdcRfcReadAttrOrClose(inst, priInst);
    }
    CSR_UNUSED(instData);
    CSR_UNUSED(msg);
    return FALSE;
}

static CsrBool cmnSdcRfcAttributeCfmHandler(void                 * instData,
                                           CmnSdcRfcInstType    * inst,
                                           CmnSdcRfcPriInstType * priInst,
                                           void                 * msg)
{
    CsrBtCmSdcAttributeCfm * prim    = (CsrBtCmSdcAttributeCfm *) msg;

    if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
    { /* The SDP search has been  cancelled, Close the SDP channel  */
        CsrBtCmSdcCloseReqSend(inst->appHandle);
    }
    else
    { /* Insert the retrieve attribute information                  */
         CsrBtUtilSdrInsertAttributeDataAtAttributeUuid(priInst->sdpOutTagList, priInst->sdrEntryIndex,
                                              priInst->sdrAttrIndex, prim->attributeListSize, prim->attributeList);

       /* Increment the sdrAttrIndex and check if more attribute
          needs to be read from the servicehandle                   */
        priInst->sdrAttrIndex++;

        if (priInst->sdrAttrIndex < priInst->numOfSdrAttr)
        { /* Need to read more attribute from this entry, e.g
             from this serviceHandle                                */
            CsrUint16 attrId;

            if (CsrBtUtilSdrGetAttributeUuid(priInst->sdpOutTagList,
                        priInst->sdrEntryIndex, priInst->sdrAttrIndex, &attrId))
            { /* Read the SDP attribute                             */
                CsrBtCmSdcAttributeReqSend(priInst->serviceHandle,
                                        attrId,
                                        L2CA_MTU_DEFAULT);
            }
            else
            { /* Could not find any attribute value. Determined if
                 any attributes must be read from another
                 serviceHandle. If not the SDP channel is closed    */
                priInst->sdrEntryIndex++;
                cmnSdcRfcReadAttrOrClose(inst, priInst);
            }
        }
        else
        { /* All atribute has been read from this servicehandle.
             Determined if any attributes must be read from
             another serviceHandle. If not the SDP channel
             is closed                                              */
            priInst->sdrEntryIndex++;
            cmnSdcRfcReadAttrOrClose(inst, priInst);
        }
    }
    CsrPmemFree(prim->attributeList);
    prim->attributeList = NULL;
    CSR_UNUSED(instData);
    return FALSE;
}

#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
static CsrUint16 cmnSdcRfcServiceUnpack16(const CsrUint8 *s)
{
    return ((((CsrUint16)s[0]) << 8) | (CsrUint16)s[1]);
}

static CsrUint32 cmnSdcRfcServiceUnpack32(const CsrUint8 *s)
{
    CsrUint32 r = s[0];

    r <<= 8; r |= s[1];
    r <<= 8; r |= s[2];
    r <<= 8; r |= s[3];

    return (r);
}

/* Returns the size of the region */
static CsrUint16 cmnSdcRfcSdrGetRegionSize(cmnSdcSdrRegion sdr)
{
    return (CsrUint16)(sdr.end - sdr.begin);
}

/* This function parses the attribute list received and splits to
 * get the size of the individual attribute */
static CsrBool cmnSdcRfcParseAndSplitAttrList(cmnSdcSdrRegion *sdr,
                                              CsrBool readAttribute,
                                              CsrUint16 *attrId,
                                              CsrUint16 *bytesToAdd)
{
    CsrUint8  byte, type;
    CsrUint16 size = 0;

    if (cmnSdcRfcSdrGetRegionSize(*sdr) < 1)
        return (FALSE);

    /* Get the first byte from the beginning of region and increment the begin pointer */
    byte = *sdr->begin++;
    type = (byte >> 3);

    /* Determine the number of bytes to consider from the above obtained byte */
    switch (byte & 7)
    {
        default:
        case 0:
            size = (type == 0) ? 0 : 1;
            break;

        case 1:
            size = 2;
            break;

        case 2:
            size = 4;
            break;

        case 3:
            size = 8;
            break;

        case 4:
            size = 16;
            break;

        case 5:
            if (cmnSdcRfcSdrGetRegionSize(*sdr) < 1)
            {
                return (FALSE);
            }
            size = *sdr->begin + 1;
            break;

        case 6:
            if (cmnSdcRfcSdrGetRegionSize(*sdr) < 2)
            {
                return (FALSE);
            }
            size = cmnSdcRfcServiceUnpack16(sdr->begin);
            sdr->begin += 2;
            break;

        case 7:
            if (cmnSdcRfcSdrGetRegionSize(*sdr) < 4)
            {
                return (FALSE);
            }
            size = (CsrUint16)cmnSdcRfcServiceUnpack32(sdr->begin);
            sdr->begin += 4;
            break;
    }

    if (cmnSdcRfcSdrGetRegionSize(*sdr) < 1)
    {
        return (FALSE);
    }

    if (!readAttribute)
    {
        *attrId = CSR_GET_UINT16_FROM_BIG_ENDIAN(sdr->begin);
        *bytesToAdd = size;
    }
    else
    {
        sdr->end = sdr->begin + size;
    }

    return (TRUE);
}

static void cmnSdcRfcUtilParseAndInsertAttributes(CmnSdcRfcPriInstType *priInst,
                                                  CsrUint8 *attributeList,
                                                  CsrUint16 attributeListSize,
                                                  CsrUint8 serviceHandleIndex)
{
    cmnSdcSdrRegion sdr, tempSdr;
    CsrUint16 size;
    CsrUintFast16 i, noOfAttributes;

    CsrBtUtilSdrGetNofAttributes(priInst->sdpOutTagList,
                                 serviceHandleIndex,
                                 &noOfAttributes);

    for (i = 0; i < noOfAttributes; i++)
    {
        CsrUint16 nofBytesToAttribute;
        CsrUint8* att_p;

        sdr.begin = attributeList;
        sdr.end   = attributeList + attributeListSize;

        tempSdr = sdr;

        /* Get the attribute pointer from the sdpOutTagList */
        att_p = CsrBtUtilSdrGetAttributePointer(priInst->sdpOutTagList,
                                                serviceHandleIndex,
                                                i,
                                                &nofBytesToAttribute);

        if (att_p)
        {
            CsrUint16 attrId, tagAttrId, bytesToAdd;

            /* Get the attribute ID from the sdpOutTagList */
            SynMemCpyS(&tagAttrId,
                       SDR_ENTRY_SIZE_SERVICE_UINT16,
                       att_p + SDR_ENTRY_INDEX_ATTRIBUTE_UUID,
                       SDR_ENTRY_SIZE_SERVICE_UINT16);

            /* Go through the entire attribute list received and insert the data if found */
            while (sdr.begin != sdr.end)
            {
                /* Obtain the attribute ID from the received attribute list */
                if (cmnSdcRfcParseAndSplitAttrList(&tempSdr, FALSE, &attrId, &bytesToAdd))
                {
                    /* Increment the begin pointer by the size received */
                    tempSdr.begin += bytesToAdd;

                    /* Retrieve the corresponding attribute data */
                    if (cmnSdcRfcParseAndSplitAttrList(&tempSdr, TRUE, NULL, NULL))
                    {
                        /* We have the end pointer of tempSdr pointing to the end of the specific attribute data.
                           Move the tempSdr begin pointer to the start to get the entire size of the attribute */
                        tempSdr.begin = sdr.begin;
                        size = cmnSdcRfcSdrGetRegionSize(tempSdr);

                        /* Check if the retrieved attribute ID matches with the ID in the outTagList */
                        if (tagAttrId == attrId)
                        {
                            /* Now we have the pointer to the start and end of specific attribute.
                             * Insert it in the sdpOutTagList */
                            CsrBtUtilSdrInsertAttributeDataAtAttributeUuid(priInst->sdpOutTagList,
                                                                           serviceHandleIndex,
                                                                           i,
                                                                           size,
                                                                           sdr.begin);
                            break;
                        }
                        /* Make the begin pointer of tempSdr & sdr point to end pointer of tempSdr
                           for parsing the next attribute in the list */
                        tempSdr.begin = tempSdr.end;
                        sdr.begin     = tempSdr.end;
                        /* Move the end pointer to the end of the actual received attribute list */
                        tempSdr.end   = attributeList + attributeListSize;
                    }
                    else
                    {
                        /* Invalid attribute data */
                        return;
                    }
                }
                else
                {
                    /* Invalid attribute list received */
                    return;
                }
            }
        }
    }
}

static void cmnSdcRfcInsertServiceHandle(CmnSdcRfcPriInstType *priInst,
                                         CsrBtUuid32 serviceHandle,
                                         CsrUint8 serviceHandleIndex)
{
    if (priInst->uuidType == SDP_DATA_ELEMENT_SIZE_4_BYTES)
    {
        CsrBtUuid32 service = 0;

        CsrBtUtilSdrGetServiceUuid32(priInst->sdpOutTagList,
                                     serviceHandleIndex,
                                     &service);

        /* The service is 32bit UUID-type                     */
        CsrBtUtilSdrInsertServiceHandleAtUuid32(priInst->sdpOutTagList,
                                                service,
                                                serviceHandle);
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else
    {
        CsrBtUuid128 *service;

        CsrBtUtilSdrGetServiceUuid128(priInst->sdpOutTagList,
                                      serviceHandleIndex,
                                      &service);

        if (service)
        { /* The service is 128bit UUID-type                     */
            CsrBtUtilSdrInsertServiceHandleAtUuid128(priInst->sdpOutTagList,
                                                     service,
                                                     serviceHandle);
        }
    }
#endif
}

static CsrBool cmnSdcRfcIsResultAttrListValid(CmnSdcRfcPriInstType *priInst,
                                              CmSdcServiceSearchAttrInd *prim)
{
    CsrBool result = FALSE;

    if (prim->attributeList && prim->attributeListSize > 0)
    {
        if (priInst->uuidType == SDP_DATA_ELEMENT_SIZE_4_BYTES)
        {
            CsrBtUuid32 service = 0;

            CsrBtUtilSdrGetServiceUuid32(priInst->sdpOutTagList,
                                         prim->serviceIndex,
                                         &service);

            result = CsrBtSdcFindServiceClassUuid(prim->attributeListSize,
                                                  prim->attributeList,
                                                  service);
        }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
        else
        {
            CsrBtUuid128 *service = NULL;

            CsrBtUtilSdrGetServiceUuid128(priInst->sdpOutTagList,
                                          prim->serviceIndex,
                                          &service);

            if (service)
            {
                result = CsrBtSdcFindServiceClass128Uuid(prim->attributeListSize,
                                                         prim->attributeList,
                                                         *service);
            }
        }
#endif
    }

    return (result);
}

static void cmnSdcRfcHandleServiceSearchAttrResult(CmnSdcRfcPriInstType* priInst,
                                                   CmSdcServiceSearchAttrInd *prim)
{
    CsrBool isValid;
    CsrUint16 newIndex;

    /* Check if the service class ID of the retrieved attribute list
       matches with the service UUID we requested */
    isValid = cmnSdcRfcIsResultAttrListValid(priInst, prim);

    if (isValid)
    {
        /* Check if the entry for this service index has already been made */
        if (TRUE == CmnSdcUtilCheckEntrySuccessful(priInst->sdpOutTagList,
                                                   prim->serviceIndex))
        {
            /* The server supports multiple instance for the same service.
               Create another entry in the sdpOutTagList */
            priInst->sdpOutTagList = CsrBtUtilSdrCreateServiceHandleEntryFromTemplate(priInst->sdpOutTagList,
                                                                                      &newIndex,
                                                                                      priInst->sdpOutTagList,
                                                                                      prim->serviceIndex);
        }
        else
        {
            newIndex = prim->serviceIndex;
        }

        cmnSdcRfcUtilParseAndInsertAttributes(priInst,
                                              prim->attributeList,
                                              prim->attributeListSize,
                                              newIndex);
        CmnSdcUtilMarkEntrySuccessful(priInst->sdpOutTagList,
                                      newIndex);

        if (prim->serviceHandle != INVALID_SERVICE_HANDLE)
        {
            cmnSdcRfcInsertServiceHandle(priInst,
                                         prim->serviceHandle,
                                         newIndex);
        }
    }
}

static CsrBool cmnSdcRfcServiceSearchAttrIndHandler(void                 *instData,
                                                    CmnSdcRfcInstType    *inst,
                                                    CmnSdcRfcPriInstType *priInst,
                                                    void                 *msg)
{
    CmSdcServiceSearchAttrInd * prim = (CmSdcServiceSearchAttrInd *) msg;

    if ((priInst->state != CMN_SDC_RFC_CANCEL_STATE) &&
        (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS))
    {
        cmnSdcRfcHandleServiceSearchAttrResult(priInst, prim);
    }

    CsrPmemFree(prim->attributeList);
    prim->attributeList = NULL;
    CSR_UNUSED(instData);
    CSR_UNUSED(inst);

    /* Set the returnValue to FALSE in order to indicate that the service
       search attribute request procedure is not finished yet */
    return (FALSE);
}

static CsrBool cmnSdcRfcServiceSearchAttrCfmHandler(void                 *instData,
                                                    CmnSdcRfcInstType    *inst,
                                                    CmnSdcRfcPriInstType *priInst,
                                                    void                 *msg)
{
    CsrBool returnValue              = TRUE;
    CmSdcServiceSearchAttrCfm * prim = (CmSdcServiceSearchAttrCfm *) msg;
    CsrBool releaseResources         = cmnReleaseResources(inst,
                                                           prim->resultCode,
                                                           prim->resultSupplier);

    if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
    { /* The SDP search has been cancelled */
        cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                CSR_BT_RESULT_CODE_CM_CANCELLED,
                                CSR_BT_SUPPLIER_CM, releaseResources);
    }
    else
    {
        if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS)
        {
            cmnSdcRfcHandleServiceSearchAttrResult(priInst, prim);
        }

        /* Remove the non-successful entries from the sdpOutTagList */
        priInst->sdpOutTagList      = CsrBtUtilSdrRemoveNonSuccessStatusEntries(priInst->sdpOutTagList);
        priInst->numOfSdrOutEntries = (CsrUint8)CsrBtUtilBllGetNofEntriesEx(priInst->sdpOutTagList);

        if (inst->cbSupportMask & CSR_SDC_OPT_CB_SEARCH_RESULT_MASK)
        { /* Call the function which indicates that service search
             attribute procedure is finished                   */
            cmnSdcRfcCallResultFunc(instData,
                                    inst,
                                    priInst,
                                    prim->resultCode,
                                    prim->resultSupplier,
                                    releaseResources);
        }
        else
        {
            /* The SDP search should either be a success or a failure with the error
               SDC_NO_RESPONSE_DATA. Any error code other than SDC_NO_RESPONSE_DATA
               will indicate an issue with the connection rather than an SDP failure */
            if ((prim->resultSupplier == CSR_BT_SUPPLIER_CM &&
                 prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) ||
                 ((prim->resultSupplier == CSR_BT_SUPPLIER_SDP_SDC ||
                   prim->resultSupplier == CSR_BT_SUPPLIER_SDP_SDC_OPEN_SEARCH) &&
                   prim->resultCode == SDC_NO_RESPONSE_DATA))
            {
                /* The connect procedure is running, check if at least
                   one SDP result is valid                            */
                if (cmnSdcRfcStartConnect(instData, inst, priInst))
                { /* Continue connecting. Set the returnValue to
                     FALSE in order to indicate that the connect
                     procedure is not finish yet                    */
                    returnValue = FALSE;
                }
                else
                { /* The connect procedure has failed */
                    cmnSdcRfcCallResultFunc(instData,
                                            inst,
                                            priInst,
                                            (CsrBtResultCode) SDC_NO_RESPONSE_DATA,
                                            CSR_BT_SUPPLIER_SDP_SDC,
                                            releaseResources);
                }
            }
            else
            {
                /* The SDP procedure has failed due to a reason other than
                   the remote device not supporting the service. Inform the
                   requester with the actual error code and supplier */
                cmnSdcRfcCallResultFunc(instData,
                                        inst,
                                        priInst,
                                        prim->resultCode,
                                        prim->resultSupplier,
                                        releaseResources);
            }
        }
    }

    if (prim->attributeList)
    {
        CsrPmemFree(prim->attributeList);
        prim->attributeList = NULL;
    }

    return (returnValue);
}
#endif /* INSTALL_CMN_ENHANCED_SDP_FEATURE */


static CsrBool cmnSdcRfcCloseIndHandler(void                 * instData,
                                       CmnSdcRfcInstType    * inst,
                                       CmnSdcRfcPriInstType * priInst,
                                       void                 * msg)
{ /* Indicates that the SDC connection has now closed.              */
    CsrBool returnValue          = TRUE;
    CsrBtCmSdcCloseInd * prim   = (CsrBtCmSdcCloseInd *) msg;
    CsrBool releaseResources     = cmnReleaseResources(inst, prim->resultCode, prim->resultSupplier);

    switch (priInst->state)
    {
        case CMN_SDC_RFC_CANCEL_STATE:
        { /* The procedure has been cancelled.                      */
            cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    CSR_BT_RESULT_CODE_CM_CANCELLED,
                                    CSR_BT_SUPPLIER_CM, releaseResources);
            break;
        }
        case CMN_SDC_RFC_CLOSE_SEARCH_STATE:
        { /* The SDP search is closed because the search procedure
             is finish                                              */
            if (inst->cbSupportMask & CSR_SDC_OPT_CB_SEARCH_RESULT_MASK)
            { /* Call the function which indicates that search
                 procedure is finish with SUCCESS                   */
                cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                        CSR_BT_RESULT_CODE_CM_SUCCESS,
                                        CSR_BT_SUPPLIER_CM, releaseResources);
            }
            else
            { /* The connect procedure is running, check if at least
                 one SDP result is valid                            */
                if (cmnSdcRfcStartConnect(instData, inst, priInst))
                { /* Continue connecting. Set the returnValue to
                     FALSE in order to indicate that the connect
                     procedure is not finish yet                    */
                    returnValue = FALSE;
                }
                else
                { /* The sdc search fail, e.g the connect procedure
                     failed                                         */
                    cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                            (CsrBtResultCode) SDC_NO_RESPONSE_DATA,
                                            CSR_BT_SUPPLIER_SDP_SDC, releaseResources);
                }
            }
            break;
        }
        default:
        { /* The SDP channel were close abnormal                    */
            cmnSdcRfcCallResultFunc(instData, inst, priInst, prim->resultCode, prim->resultSupplier, releaseResources);
            break;
        }
    }
    return (returnValue);
}

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
/* Inserts the server channel in the sdp Tag List's entries which has CSR_BT_NO_SERVER as server channel */
static void cmnInsertLocalServerChannel(CmnCsrBtLinkedListStruct * sdpTag, CsrUint16 serverChannel)
{
    CsrUint16 serverCh   = CSR_BT_NO_SERVER;
    CsrUintFast16 i;
    CsrUintFast16 entries    = CsrBtUtilBllGetNofEntriesEx(sdpTag);

    for (i = 0; i < entries; i++)
    {
        if (CsrBtUtilSdrGetLocalServerChannel(sdpTag, i, &serverCh))
        {
            if (CSR_BT_NO_SERVER == serverCh)
            {
                CsrBtUtilSdrInsertLocalServerChannel(sdpTag, i, serverChannel);
            }
        }
    }
}

static CsrBool cmnSdcRfcRegisterCfm(void                 * instData,
                                   CmnSdcRfcInstType    * inst,
                                   CmnSdcRfcPriInstType * priInst,
                                   void                 * msg)
{ /* This event will be used to indicate to the higher layer that
     its previous registration of a protocol handle with an
     CSR_BT_CM_REGISTER_REQ event had been accepted . The server
     channel number assigned is also returned.                      */

    CsrBtCmRegisterCfm * prim    = (CsrBtCmRegisterCfm *) msg;
    priInst->localServerCh       = prim->serverChannel;

    /* Insert the obtained server channel in the sdp Tag List's entries which has
     * CSR_BT_NO_SERVER as server channel.
     */
    cmnInsertLocalServerChannel(priInst->sdpInTagList, prim->serverChannel);

    if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
    { /* The RFC connect procedure has been cancelled,              */
        cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                CSR_BT_RESULT_CODE_CM_CANCELLED,
                                CSR_BT_SUPPLIER_CM, FALSE);

        return TRUE;
    }
    else
    { /* priInst->state must be CMN_SDC_RFC_REGISTER_STATE          */
        if (prim->resultCode     == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        { /* A local server channel has been obtained               */
            if (cmnSdcRfcStartSearch(inst, priInst, TRUE))
            { /* Start the rfc search procedure                     */
                priInst->state          = CMN_SDC_RFC_SEARCH_STATE;
                return FALSE;
            }
            else
            { /* The sdpTag could not be interpret correct          */
                cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                        CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                        CSR_BT_SUPPLIER_CM, FALSE);
            }
        }
        else
        { /* A local server channel could not be obtained           */
            cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    prim->resultCode, prim->resultSupplier, FALSE);
        }
        return TRUE;
    }
}

static CsrBool cmnSdcRfcConnectCfm(void                    * instData,
                                     CmnSdcRfcInstType    * inst,
                                     CmnSdcRfcPriInstType * priInst,
                                     void                 * msg)
{ /* This event is the confirmation of the opening of a RFC
     connection. If the connection has been established the
     result is a success otherwise not                              */
    CsrBtCmConnectCfm * prim    = (CsrBtCmConnectCfm *) msg;

    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    { /* The connection has been established                        */
        if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
        { /* The Rfc connect procedure has been cancelled. Release
             the connection again                                   */
            CsrBtCmContextDisconnectReqSend(prim->btConnId, inst->instId);
            return FALSE;
        }
        else
        { /* Call the function which indicates that connect
             procedure is finish with SUCCESS                       */
            priInst->btConnId       = prim->btConnId;
            priInst->maxFrameSize   = prim->profileMaxFrameSize;
            priInst->portPar        = prim->portPar;
            priInst->validPortPar   = prim->validPortPar;
            cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    CSR_BT_RESULT_CODE_CM_SUCCESS,
                                    CSR_BT_SUPPLIER_CM, TRUE);

            return TRUE;
        }
    }
    else
    { /* The connection could not be establish                      */
        if (priInst->state == CMN_SDC_RFC_CANCEL_STATE)
        { /* The Rfc connect procedure has been cancelled. Call the
             function which indicates that connect procedure is
             finish                                                 */
            cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    CSR_BT_RESULT_CODE_CM_CANCELLED,
                                    CSR_BT_SUPPLIER_CM, TRUE);

            return TRUE;
        }
        else
        { /* The profile don't care which of the service handle is
             being used. Check if it is possible to connect to
             another SDP service handle                             */
            CsrBool initiateConnect = cmnSdcRfcErrorResultHandler(prim->resultCode, prim->resultSupplier);
            
            if (initiateConnect)
            { 
                priInst->sdrEntryIndex++;

                if (inst->cbSupportMask & CSR_SDC_OPT_CB_SELECT_SVC_HANDLE_MASK)
                { /* The profile has selected a specific service record
                     handle list. Check if it is possible to connect to
                     another SDP service handle                         */
                    initiateConnect = cmnSdcRfcGetSelectedServiceHandle(priInst);
                }
                else
                { /* The profile don't care which of the service handle
                     is being used. Check if it is possible to connect
                     to another SDP service handle                      */
                    initiateConnect = cmnSdcRfcGetServiceHandle(priInst);
                }
            }

            if (initiateConnect)
            { /* A service record handle has been found. Request CM
                 to initiate an RFC connection                      */
                priInst->state          = CMN_SDC_RFC_CONNECT_STATE;

                CsrBtCmContextConnectReqSend(inst->appHandle, priInst->localServerCh,
                                             priInst->serviceHandle, (CsrUint16)priInst->btConnId,
                                             priInst->reqPortPar, priInst->validPortPar,
                                             priInst->portPar, priInst->secLevel, priInst->deviceAddr,
                                             inst->instId, priInst->modemStatus, CSR_BT_DEFAULT_BREAK_SIGNAL,
                                             priInst->scTimeout, priInst->minEncKeySize);

                return FALSE;
            }
            else
            { /* Could not connect to any of the obtain service
                 record handles. Call the function which indicates
                 that the connect procedure failed                  */
                cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    prim->resultCode,
                                    prim->resultSupplier, TRUE);

                return TRUE;
            }
        }
    }
}

static CsrBool cmnSdcRfcDisconnectInd(void                 * instData,
                                     CmnSdcRfcInstType    * inst,
                                     CmnSdcRfcPriInstType * priInst,
                                     void                 * msg)
{ /* The connection is release because the request to cancel this
     procedure were crossing with a successful CSR_BT_CM_CONNECT_CFM
     message                                                        */
    CsrBtCmDisconnectInd *prim = (CsrBtCmDisconnectInd *) msg;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
        CsrBtCmRfcDisconnectRspSend(prim->btConnId);
    }
#endif

    if (prim->status)
    { /* The connection is release meaning that this procedure is
         cancelled, call the function which indicates that this
         procedure is finish                                        */
        cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                CSR_BT_RESULT_CODE_CM_CANCELLED,
                                CSR_BT_SUPPLIER_CM, TRUE);
        return TRUE;
    }

    /* Failed to release the connection. Try again                */
    CsrBtCmContextDisconnectReqSend(prim->btConnId, inst->instId);
    return FALSE;
}

static CsrBool cmnSdcRfcPortNegInd(void                 * instData,
                                  CmnSdcRfcInstType    * inst,
                                  CmnSdcRfcPriInstType * priInst,
                                  void                 * msg)
{ /* This event is indicates to the higher port entity that the
     remote port entity requests it to set port parameters for the
     DLCI as given in the Port Parameters. The port entity should
     respond with RFC_PORTNEG_RES.                                  */
    CsrBtCmPortnegInd  * prim    = (CsrBtCmPortnegInd *) msg;

    if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_SET_PORT_PAR_MASK)
    {
        CsrBtUtilRfcConSetPortParType context;

        /* The profile has requested to handle this message, call the
         function which allow the profile to do this                */

        priInst->btConnId = prim->btConnId;
        priInst->state = CMN_SDC_RFC_PORTNEG_PENDING_STATE;

        context.instData = instData;
        context.deviceAddr = priInst->deviceAddr;
        context.serverChannel = priInst->localServerCh;
        context.portPar = &prim->portPar;
        context.request = prim->request;

        inst->sdcRfcResultFunc(CSR_SDC_OPT_UTIL_RFC_CON_SET_PORT_PAR, (void *)&context);
    }
    else
    { /* The message must be handle automatic by this lib           */
        CsrBtRespondCmPortNegInd(msg);
    }
    return FALSE;
}

static CsrBool cmnSdcRfcMicsMsgCfm(void                 * instData,
                                  CmnSdcRfcInstType    * inst,
                                  CmnSdcRfcPriInstType * priInst,
                                  void                 * msg)
{ /* Received a CM upstream that must be ignore. Make sure that
     the message contents is CsrPmemFree to prevent a memory leak         */
    CSR_UNUSED(instData);
    CSR_UNUSED(inst);
    CSR_UNUSED(priInst);
    CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, msg);
    return FALSE;
}

static const CmnSdcRfcType cmnRfcCmMsgHandlers[CSR_BT_CM_RFC_PRIM_UPSTREAM_COUNT] =
{
    NULL,                           /* CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM */
    cmnSdcRfcConnectCfm,            /* CSR_BT_CM_CONNECT_CFM               */
    NULL,                           /* CSR_BT_CM_CONNECT_ACCEPT_CFM        */
    cmnSdcRfcRegisterCfm,           /* CSR_BT_CM_REGISTER_CFM              */
    cmnSdcRfcDisconnectInd,         /* CSR_BT_CM_DISCONNECT_IND            */
    NULL,                           /* CSR_BT_CM_SCO_CONNECT_CFM           */
    NULL,                           /* CSR_BT_CM_SCO_DISCONNECT_IND        */
    NULL,                           /* CSR_BT_CM_SCO_ACCEPT_CONNECT_CFM    */
    cmnSdcRfcMicsMsgCfm,            /* CSR_BT_CM_DATA_IND                  */
    NULL,                           /* CSR_BT_CM_DATA_CFM                  */
    cmnSdcRfcMicsMsgCfm,            /* CSR_BT_CM_CONTROL_IND               */
    cmnSdcRfcMicsMsgCfm,            /* CSR_BT_CM_RFC_MODE_CHANGE_IND       */
    cmnSdcRfcPortNegInd,            /* CSR_BT_CM_PORTNEG_IND               */
    NULL,                           /* CSR_BT_CM_PORTNEG_CFM               */
};
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */

static const CmnSdcRfcType cmnSdcCmMsgHandlers[CSR_BT_CM_SDC_PRIM_UPSTREAM_COUNT] =
{
    cmnSdcRfcSearchIndHandler,                      /* CSR_BT_CM_SDC_SEARCH_IND                   */
    cmnSdcRfcSearchCfmHandler,                      /* CSR_BT_CM_SDC_SEARCH_CFM                   */
    NULL,                                           /* CSR_BT_CM_SDC_SERVICE_SEARCH_CFM           */
    NULL,                                           /* CSR_BT_CM_SDC_OPEN_CFM                     */
    cmnSdcRfcAttributeCfmHandler,                   /* CSR_BT_CM_SDC_ATTRIBUTE_CFM                */
    cmnSdcRfcCloseIndHandler,                       /* CSR_BT_CM_SDC_CLOSE_IND                    */
    NULL,                                           /* CSR_BT_CM_SDC_RELEASE_RESOURCES_CFM        */
    NULL,                                           /* CSR_BT_CM_SDS_REGISTER_CFM                 */
    NULL,                                           /* CSR_BT_CM_SDS_UNREGISTER_CFM               */
    cmnSdcRfcServiceSearchAttrIndHandler,           /* CM_SDC_SERVICE_SEARCH_ATTR_IND             */
    cmnSdcRfcServiceSearchAttrCfmHandler,           /* CM_SDC_SERVICE_SEARCH_ATTR_CFM             */
};

static CsrBool cmnSdcRfcVerifyInstData(void *cmSdcRfcInstData)
{
    if (cmSdcRfcInstData)
    { /* inst data is present                                       */
        CmnSdcRfcInstType * inst = cmSdcRfcInstData;

        if (inst->privateInst)
        { /* All inst data is available, return TRUE                */
            return TRUE;
        }
        else
        { /* No private inst data, return FALSE                     */
            return FALSE;
        }
    }
    else
    { /* No inst data is available, return FALSE                    */
        return FALSE;
    }
}

static CsrBool cmnCmMsgHandler(void *instData, void *cmSdcRfcInstData,
                              void *msg, CsrBool rfcConnect)
{
    if (cmnSdcRfcVerifyInstData(cmSdcRfcInstData))
    {
        CsrBtCmPrim *primType         = (CsrPrim *) msg;
        CmnSdcRfcInstType * inst    = cmSdcRfcInstData;

        CsrUint16  sdcIndex = (CsrUint16) (*primType - CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST);

        if (*primType >= CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST &&
            *primType <= CSR_BT_CM_SDC_PRIM_UPSTREAM_HIGHEST &&
            cmnSdcCmMsgHandlers[sdcIndex] != NULL)
        { /* This library function is able to handle the incoming CM
             message. Call the handler function to the incoming
             message                                                */
            return (cmnSdcCmMsgHandlers[sdcIndex] (instData, inst,
                                             inst->privateInst, msg));
        }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
        else if (*primType == CSR_BT_CM_SDC_UUID128_SEARCH_IND)
        { /* This library function is able to handle the incoming CM
             message. Call the handler function to this incoming
             message                                                */
            return (cmnSdcRfcUuid128SearchIndHandler(instData, inst,
                                                     inst->privateInst, msg));
        }
#endif
        else
        { /* Check if it the Sdc search or the Rfc connect procedure
             that is running                                        */
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
            if (rfcConnect)
            { /* It is the Cmn Rfc Connect procedure that is running*/
                CsrUint16  rfcIndex = (CsrUint16) (*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST);
                if (*primType >= CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST &&
                    *primType <= CSR_BT_CM_RFC_PRIM_UPSTREAM_HIGHEST &&
                    cmnRfcCmMsgHandlers[rfcIndex] != NULL)
                {
                    return (cmnRfcCmMsgHandlers[rfcIndex] (instData, inst,
                                             inst->privateInst, msg));
                }
                else
                { /* This library function is not able to handle the
                     incoming CM message. Make sure that the data
                     pointer in this message is CsrPmemFree as it is
                     consider as a state event error has occurred   */
                    CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, msg);
                    return FALSE;
                }
            }
            else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE*/
            { /* This library function is not able to handle the
                 incoming CM message. Make sure that the data pointer
                 in this message is CsrPmemFree as it is consider as a
                 state event error has occurred                     */
                CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, msg);
                return FALSE;
            }
        }
    }
    else
    {  /* No inst data is allocated this function called is not
         allowed.                                                   */
        CsrBtCmFreeUpstreamMessageContents(CSR_BT_CM_PRIM, msg);
        return TRUE;
    }
}

void * CsrBtUtilSdpRfcInit(CmnSdcRfcCallbackFuncType            sdcRfcResultFunc,
                           CsrSdcOptCbSupportMask               cbSupportMask,
                           CsrSchedQid                          appHandle,
                           CsrUint8                             instIdentifier)
{
    CmnSdcRfcInstType * inst = (CmnSdcRfcInstType *) CsrPmemAlloc(sizeof(CmnSdcRfcInstType));

    inst->appHandle                 = appHandle;
    inst->sdcRfcResultFunc          = sdcRfcResultFunc;
    inst->cbSupportMask             = cbSupportMask;
    inst->instId                    = instIdentifier;
    inst->privateInst               = NULL;
    return (inst);
}

/* public function                                                  */
void * CsrBtUtilSdcInit(CmnSdcRfcCallbackFuncType sdcRfcResultFunc,
                        CsrSchedQid               appHandle)
{
    CmnSdcRfcInstType * inst = (CmnSdcRfcInstType *) CsrPmemZalloc(sizeof(CmnSdcRfcInstType));

    inst->appHandle                 = appHandle;
    inst->sdcRfcResultFunc          = sdcRfcResultFunc;
    inst->cbSupportMask             = CSR_SDC_OPT_CB_SEARCH_RESULT_MASK;
    return (inst);
}

void CsrBtUtilSdcRfcDeinit(void ** cmSdcRfcInstData)
{ /* Deregister the callback function register by the CmnSdpInit    */
    CmnSdcRfcInstType * inst = *cmSdcRfcInstData;

    if (inst)
    { /* Pfree priInst and inst                                     */
        cmnSdcRfcCleanupPrivateInst(inst->privateInst);
        CsrPmemFree(inst);
        *cmSdcRfcInstData = NULL;
    }
    else
    { /* Nothing to free                                            */
        ;
    }
}

CsrBool CsrBtUtilSdcSearchStart(void                    * instData,
                         void                    * cmSdcRfcInstData,
                         CmnCsrBtLinkedListStruct * sdpTag,
                         CsrBtDeviceAddr            deviceAddr)
{
    if (cmSdcRfcInstData)
    { /* inst data is present                                       */
        CmnSdcRfcInstType * inst = cmSdcRfcInstData;

        if (!inst->privateInst && (inst->cbSupportMask & CSR_SDC_OPT_CB_SEARCH_RESULT_MASK))
        { /* Start the CM SDP search procedure                      */
            CmnSdcRfcPriInstType * priInst;

            inst->privateInst = (CmnSdcRfcPriInstType *) CsrPmemZalloc(sizeof(CmnSdcRfcPriInstType));

            priInst               = inst->privateInst;
            cmnSdcRfcInitPrivateInst(priInst);
            priInst->deviceAddr   = deviceAddr;
            priInst->sdpInTagList = sdpTag;

            if (cmnSdcRfcStartSearch(inst, priInst, FALSE))
            { /* The CM SDP search is started                       */
                priInst->state    = CMN_SDC_RFC_SEARCH_STATE;
            }
            else
            { /* The sdpTag could not be interpret correct. Call
                 the function which indicates that this procedure
                 is finish                                          */
                cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                        CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                        CSR_BT_SUPPLIER_CM, FALSE);

                return TRUE;
            }
        }
        else
        { /* The procedure is already running. Just ignore this
             request, but make sure that the pointer *sdpTag
             is CsrPmemFree to prevent a memoryleak                       */
            CsrBtUtilBllFreeLinkedList(&sdpTag, CsrBtUtilBllPfreeWrapper);
        }
        return FALSE;
    }
    else
    { /* No cmSdcRfcInstData, make sure that the pointer *sdpTag
         is CsrPmemFree to prevent a memoryleak                           */
        CsrBtUtilBllFreeLinkedList(&sdpTag, CsrBtUtilBllPfreeWrapper);
        return TRUE;
    }
}

CsrBool CsrBtUtilSdcVerifyCmMsg(void *msg)
{
    CsrBtCmPrim *primType = (CsrPrim *) msg;
    CsrUint16  index     = (CsrUint16) (*primType - CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST);

    if (*primType >= CSR_BT_CM_SDC_PRIM_UPSTREAM_LOWEST &&
        *primType <= CSR_BT_CM_SDC_PRIM_UPSTREAM_HIGHEST &&
        cmnSdcCmMsgHandlers[index] != NULL)
    { /* This library is able to handle this message                */
        return TRUE;
    }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
    else if (*primType == CSR_BT_CM_SDC_UUID128_SEARCH_IND)
    { /* This library is able to handle this message                */
        return TRUE;
    }
#endif
    else
    { /* This library is not able to handle this message            */
        return FALSE;
    }
}

CsrBool CsrBtUtilSdcCmMsgHandler(void *instData, void *cmSdcRfcInstData, void *msg)
{ /* This function handles incoming CM messages                     */
    CSR_UNUSED(instData);
    return (cmnCmMsgHandler(instData, cmSdcRfcInstData, msg, FALSE));
}

CsrBool CsrBtUtilSdcSearchCancel(void *instData, void *cmSdcRfcInstData)
{
    CSR_UNUSED(instData);
    if (cmnSdcRfcVerifyInstData(cmSdcRfcInstData))
    {
        CmnSdcRfcInstType    * inst    = cmSdcRfcInstData;
        CmnSdcRfcPriInstType * priInst = inst->privateInst;

        switch(priInst->state)
        {
            case CMN_SDC_RFC_CLOSE_SEARCH_STATE:
            { /* The SDP channel is allready being closed. Change
                 state to indicated that the SDP search is
                 cancelled                                          */
                priInst->state = CMN_SDC_RFC_CANCEL_STATE;
                return FALSE;
            }
            case CMN_SDC_RFC_ATTR_STATE:
            case CMN_SDC_RFC_SEARCH_STATE:
            { /* The SDP search is running, cancel it               */
                priInst->state = CMN_SDC_RFC_CANCEL_STATE;

#ifdef INSTALL_CMN_ENHANCED_SDP_FEATURE
                CmSdcCancelServiceSearchAttrReqSend(inst->appHandle, priInst->deviceAddr);
#else
                if (priInst->uuidType == SDP_DATA_ELEMENT_SIZE_4_BYTES)
                { /* A 32bit CM SDP search must be cancel           */
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
                    if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
                    { /* Cancel the Cm Rfc Search procedure         */
                        CsrBtCmSdcCancelRfcSearchReqSend(inst->appHandle, priInst->deviceAddr);
                    }
                    else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
                    { /* Cancel the CM search procedure             */
                        CsrBtCmSdcCancelSearchReqSend(inst->appHandle, priInst->deviceAddr);
                    }
                }
#ifdef CSR_BT_INSTALL_128_BIT_SERVICE_SEARCH
                else
                { /* A 128bit CM SDP search must be cancel          */
#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
                    if (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK)
                    { /* Cancel the Cm Rfc uuid128 Search procedure */
                        CsrBtCmSdcCancelUuid128RfcSearchReqSend(inst->appHandle, priInst->deviceAddr);
                    }
                    else
#endif /* EXCLUDE_CSR_BT_RFC_MODULE */
                    { /* Cancel the Cm uuid128 Search procedure */
                        CsrBtCmSdcCancelUuid128SearchReqSend(inst->appHandle, priInst->deviceAddr);
                    }
                }
#endif
#endif /* INSTALL_CMN_ENHANCED_SDP_FEATURE */
                return FALSE;
            }
            case CMN_SDC_RFC_CANCEL_STATE:
            { /* The process is allready being cancelled            */
                return FALSE;
            }
            default:
            { /* CMN_SDC_RFC_IDLE_STATE, nothing to cancel          */
                return TRUE;
            }
        }
    }
    else
    { /* No inst data is allocated nothing to cancel                */
        return TRUE;
    }
}

#if !defined(EXCLUDE_CSR_BT_RFC_MODULE)
void * CsrBtUtilSdpRfcConInit(CmnSdcRfcCallbackFuncType            sdcRfcResultFunc,
                              CsrSdcOptCbSupportMask               cbSupportMask,
                              CsrSchedQid                          appHandle)
{
    return (CsrBtUtilSdpRfcInit(sdcRfcResultFunc,
                                cbSupportMask,
                                appHandle,
                                CSR_BT_SDC_RFC_UTIL_INST_ID_NOT_USED));
}

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
                      CsrUint8                 minEncKeySize)
{
    if (cmSdcRfcInstData)
    { /* inst data is present                                       */
        CmnSdcRfcInstType * inst = cmSdcRfcInstData;

        if (!inst->privateInst && (inst->cbSupportMask & CSR_SDC_OPT_CB_RFC_CON_RESULT_MASK))
        { /* Request the CM to register a protocol handle with
             RFCOMM. CM will return a CSR_BT_CM_REGISTER_CFM primitive
             with an assigned server channel number.                */
            CmnSdcRfcPriInstType * priInst;

            inst->privateInst     = (CmnSdcRfcPriInstType *) CsrPmemZalloc(sizeof(CmnSdcRfcPriInstType));
            priInst               = inst->privateInst;
            cmnSdcRfcInitPrivateInst(priInst);

            priInst->deviceAddr   = deviceAddr;
            priInst->sdpInTagList = sdpTag;
            priInst->secLevel     = secLevel;
            priInst->reqPortPar   = requestPortPar;
            if (modemStatus != 0xFF)
            {
                priInst->modemStatus  = modemStatus;
            }
            if (mscTimeout != 0)
            {
                priInst->scTimeout    = mscTimeout;
            }
            priInst->maxFrameSize = mtu;
            priInst->minEncKeySize = minEncKeySize;

            if (portPar)
            {
                priInst->validPortPar = TRUE;
                priInst->portPar      = *portPar;
            }

            priInst->localServerCh = cmnGetLocalServerChannel(priInst->sdpInTagList);

            if (priInst->localServerCh == CSR_BT_NO_SERVER)
            { /* Must obtain a local server channel                 */
                priInst->state          = CMN_SDC_RFC_REGISTER_STATE;
                priInst->obtainServer   = TRUE;
                CsrBtCmContextRegisterReqSend(inst->appHandle, inst->instId);
            }
            else
            { /* A local server channel is already present. Start
                 the CmRfcSearch                                    */
                if (cmnSdcRfcStartSearch(inst, priInst, TRUE))
                { /* The CmRfcSearch is started                     */
                    priInst->state = CMN_SDC_RFC_SEARCH_STATE;
                    return FALSE;
                }
                else
                { /* The sdpTag could not be interpret correct.     */
                    cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                        CSR_BT_RESULT_CODE_CM_INTERNAL_ERROR,
                                        CSR_BT_SUPPLIER_CM, FALSE);

                    return TRUE;
                }
            }
        }
        else
        { /* The procedure is allready running. Just ignore this
             request, but make sure that the pointer *sdpTag
             is CsrPmemFree to prevent a memoryleak                       */
            CsrBtUtilBllFreeLinkedList(&sdpTag, CsrBtUtilBllPfreeWrapper);
        }
        return FALSE;
    }
    else
    { /* No cmSdcRfcInstData, make sure that the pointer *sdpTag
         is CsrPmemFree to prevent a memoryleak                           */
        CsrBtUtilBllFreeLinkedList(&sdpTag, CsrBtUtilBllPfreeWrapper);
        return TRUE;
    }
}

CsrBool CsrBtUtilRfcConVerifyCmMsg(void *msg)
{
    if (CsrBtUtilSdcVerifyCmMsg(msg))
    {
        return TRUE;
    }
    else
    {
        CsrBtCmPrim *primType = (CsrPrim *) msg;
        CsrUint16  index     = (CsrUint16) (*primType - CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST);

        if (*primType >= CSR_BT_CM_RFC_PRIM_UPSTREAM_LOWEST &&
            *primType <= CSR_BT_CM_RFC_PRIM_UPSTREAM_HIGHEST &&
            cmnRfcCmMsgHandlers[index] != NULL)
        { /* This library is able to handle this message            */
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

CsrBool CsrBtUtilRfcConCmMsgHandler(void *instData, void *cmSdcRfcInstData, void *msg)
{
    return (cmnCmMsgHandler(instData, cmSdcRfcInstData, msg, TRUE));
}

CsrBool CsrBtUtilRfcConCancel(void *instData, void *cmSdcRfcInstData)
{
    if (cmnSdcRfcVerifyInstData(cmSdcRfcInstData))
    {
        if (!CsrBtUtilSdcSearchCancel(instData, cmSdcRfcInstData))
        { /* The cancel procedure has been initiated by this
             function. Return FALSE to indicate that the cancelled
             but the connect procedure is not finished yet          */
            return FALSE;
        }
        else
        {
            CmnSdcRfcInstType    * inst    = cmSdcRfcInstData;
            CmnSdcRfcPriInstType * priInst = inst->privateInst;

            switch(priInst->state)
            {
                case CMN_SDC_RFC_REGISTER_STATE:
                { /* The CM has been requested to register a protocol
                     handle with RFCOMM. Change the state to indicate
                     that the connect procedure has been cancelled
                     and wait for the CSR_BT_CM_REGISTER_CFM message       */
                    priInst->state = CMN_SDC_RFC_CANCEL_STATE;
                    return FALSE;
                }
                case CMN_SDC_RFC_SELECT_SERVICE_RECORD_HANDLE_STATE:
                { /* This procedure has been cancelled while waiting
                     for the profile to select a service record
                     handle index. Right now the SDP connection is
                     closed which means that the function which
                     indicates that the connect procedure has been
                     cancelled can be called direct                 */
                    cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                            CSR_BT_RESULT_CODE_CM_CANCELLED,
                                            CSR_BT_SUPPLIER_CM, TRUE);

                    return TRUE;
                }
                case CMN_SDC_RFC_CONNECT_STATE:
                case CMN_SDC_RFC_PORTNEG_PENDING_STATE:
                { /* The CM has been requested to initiate an RFCOMM
                     connection, or is waiting for the profile to
                     response the CSR_BT_CM_PORTNEG_IND message. Request
                     CM to cancel this connect procedure, and
                     change the state to indicate that the connect
                     procedure has been cancelled. Wait for the
                     CSR_BT_CM_CONNECT_CFM message                         */
                    priInst->state = CMN_SDC_RFC_CANCEL_STATE;
                    CsrBtCmCancelConnectReqSend(inst->appHandle,
                                priInst->localServerCh, priInst->deviceAddr);

                    return FALSE;
                }
                default:
                { /* CMN_SDC_RFC_IDLE_STATE, nothing to cancel      */
                    return TRUE;
                }
            }
        }
    }
    else
    { /* No inst data is allocated nothing to cancel                */
        return TRUE;
    }
}

CsrBool CsrBtUtilRfcConSetServiceHandleIndexList(void      * instData,
                                                void      * cmSdcRfcInstData,
                                                CsrUint16  * serviceHandleIndexList,
                                                CsrUint16  nofServiceHandleIndicis)
{
    if (cmnSdcRfcVerifyInstData(cmSdcRfcInstData))
    {
        CmnSdcRfcInstType    * inst         = cmSdcRfcInstData;
        CmnSdcRfcPriInstType * priInst      = inst->privateInst;

        priInst->numOfServiceHandleIndicis  = (CsrUint8)nofServiceHandleIndicis;
        priInst->serviceHandleIndexList     = serviceHandleIndexList;
        priInst->sdrEntryIndex              = 0;

        if (cmnSdcRfcGetSelectedServiceHandle(priInst))
        { /* A service record handle has been found. Request CM
             to initiate an RFC connection                          */
            priInst->state          = CMN_SDC_RFC_CONNECT_STATE;

            CsrBtCmContextConnectReqSend(inst->appHandle, priInst->localServerCh,
                                         priInst->serviceHandle, priInst->maxFrameSize,
                                         priInst->reqPortPar, priInst->validPortPar,
                                         priInst->portPar, priInst->secLevel, priInst->deviceAddr,
                                         inst->instId, priInst->modemStatus, CSR_BT_DEFAULT_BREAK_SIGNAL,
                                         priInst->scTimeout, priInst->minEncKeySize);

            return FALSE;
        }
        else
        { /* No service record handle match the index given from the
             profile, call the function which indicates that connect
             procedure failed                                       */
            cmnSdcRfcCallResultFunc(instData, inst, priInst,
                                    (CsrBtResultCode) SDC_NO_RESPONSE_DATA,
                                    CSR_BT_SUPPLIER_SDP_SDC, TRUE);
            return TRUE;
        }
    }
    else
    { /* No inst data is allocated.                                 */
         return TRUE;
    }
}

#ifdef CSR_BT_INSTALL_SDC_SET_PORT_PAR
CsrBool CsrBtUtilRfcConSetPortPar(void * cmSdcRfcInstData, RFC_PORTNEG_VALUES_T portPar)
{
    if (cmnSdcRfcVerifyInstData(cmSdcRfcInstData))
    {
        CmnSdcRfcInstType    * inst    = cmSdcRfcInstData;
        CmnSdcRfcPriInstType * priInst = inst->privateInst;

        if (priInst->state == CMN_SDC_RFC_PORTNEG_PENDING_STATE)
        { /* Response the portPar given from the profiles and goto
             CMN_SDC_RFC_CONNECT_STATE                              */
            priInst->state = CMN_SDC_RFC_CONNECT_STATE;
            CsrBtCmPortnegResSend(priInst->btConnId, &(portPar));
        }
        else
        { /* This function is called in a state where this util
             function is not ready to received it. Ignore it        */
            ;
        }
        return FALSE;
    }
    else
    { /* No inst data is allocated.                                 */
         return TRUE;
    }
}
#endif
#endif /*EXCLUDE_CSR_BT_RFC_MODULE */

