/******************************************************************************
 Copyright (c) 2008-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_hf_main_sef.h"

#ifndef EXCLUDE_CSR_BT_SD_MODULE
#include "csr_bt_sd_private_lib.h"
#endif

#define CSR_BT_HF_CHANNEL_REGISTER_CONTEXT_RETRY            (CSR_BT_CM_CONTEXT_UNUSED + 1)

static void DeactivateHsHandler(HfMainInstanceData_t * instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    switch (linkPtr->state)
    {
        case Activate_s:
            {
                CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServerChannel, linkPtr->instId);
                break;
            }
        case Connect_s:
            {
                break;
            }
        case Connected_s:
            {
                if (linkPtr->scoHandle != HF_SCO_UNUSED)
                {
                    CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                }
                else
                {
                    CsrBtCmScoCancelReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                }
                break;
            }
    }
    instData->state = Deactivate_s;
}

static void DeactivateHfHandler(HfMainInstanceData_t * instData)
{
    HfInstanceData_t    *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);

    switch (linkPtr->state)
    {
        case Activate_s:
            {
                CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServerChannel, linkPtr->instId);
                instData->state = Deactivate_s;
                break;
            }
        case ServiceSearch_s:
            {
                CsrBtHfSaveMessage(instData);
                instData->deactivated = TRUE;
                break;
            }
        case Connect_s:
            {
                instData->state = Deactivate_s;
                break;
            }
        case Connected_s:
            {
                instData->state = Deactivate_s;
                /* determine if we have a SCO connected */
                if (linkPtr->scoHandle == HF_SCO_UNUSED)
                {
                    CsrBtCmScoCancelReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                    CsrBtCmDisconnectReqSend(linkPtr->hfConnId);
                }
                else
                {
                    CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                }
                break;
            }
        default:
            {
                break;
            }
    }
}

void CsrBtHfFreeInactiveLinkData(HfInstanceData_t *linkData, CsrUint8 count)
{
    CsrUint8 i = 0;
    HfInstanceData_t *linkPtr = NULL;

    if (count == 0)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        linkPtr = (HfInstanceData_t *) &(linkData[i]);
        if (linkPtr->obtainedServerCh != CSR_BT_NO_SERVER)
        {
            CsrBtCmUnRegisterReqSend(linkPtr->obtainedServerCh);
            linkPtr->obtainedServerCh = CSR_BT_NO_SERVER;
        }

        if (linkPtr->sdpSearchData != NULL)
        {
            CsrBtUtilSdcRfcDeinit(&(linkPtr->sdpSearchData));
            linkPtr->sdpSearchData = NULL;
        }

        CsrMemSet(&linkPtr->agIndicators, 0, sizeof(linkPtr->agIndicators));

        if (linkPtr->data != NULL)
        {
            if (linkPtr->data->atResponseTimerId != 0)
            {
                void *mv;
                CsrUint16 mi;
                CsrSchedTimerCancel(linkPtr->data->atResponseTimerId, &mi, &mv);
                linkPtr->data->atResponseTimerId = 0;
            }
            if (linkPtr->data->recvAtCmd != NULL)
            {
                CsrPmemFree(linkPtr->data->recvAtCmd);
                linkPtr->data->recvAtCmd = NULL;
            }
            CsrPmemFree(linkPtr->data);
            linkPtr->data = NULL;
        }

        CsrPmemFree(linkPtr->serviceName);
        linkPtr->serviceName = NULL;

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
        if (linkPtr->audioSetupParams)
        {
            CsrPmemFree(linkPtr->audioSetupParams);
            linkPtr->audioSetupParams = NULL;
        }
#endif
    }
}

void CsrBtHfHsDeactivateHandler(HfMainInstanceData_t * instData)
{
    HfInstanceData_t    *linkPtr;
    CsrUint8 i;

    for (i = 0; (i < (instData->maxHFConnections + instData->maxHSConnections)); i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        if ((linkPtr->accepting) || (linkPtr->state != Activate_s))
        {
            instData->index = i;
            if (linkPtr->linkType == CSR_BT_HF_CONNECTION_HF)
            {
                DeactivateHfHandler(instData);
            }
            else
            {
                DeactivateHsHandler(instData);
            }
            return;
        }
    }

    if (instData->hfServiceRecHandle != 0 )
    {
        instData->numberOfUnregister = 0;
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServiceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        return;
    }
    else if (instData->hsServiceRecHandle != 0 )
    {
        instData->numberOfUnregister = 0;
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServiceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        return;
    }

    instData->state = Null_s;
    if (instData->localHfIndicatorList != NULL)
    {
        CsrPmemFree(instData->localHfIndicatorList);
        instData->localHfIndicatorList = NULL;
        instData->indCount= 0;
    }
    if (instData->doingCleanup)
    {
        /* Purge queue */
        CsrBtHfCleanup_queue(instData);
    }
    else
    {
        CsrBtHfSendHfDeactivateCfm(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
    }

    CsrBtHfFreeInactiveLinkData(instData->linkData, (CsrUint8)(instData->maxHFConnections + instData->maxHSConnections));
    CsrPmemFree(instData->linkData);
    instData->linkData = NULL;
    instData->maxHFConnections = 0;
    instData->maxHSConnections = 0;
}

/*************************************************************************************
    A activate request from a higher layer is received.
************************************************************************************/
static void commonActivateHandler(HfMainInstanceData_t *instData)
{
    CsrIntFast8 i = 0;
    CsrBool msgSent = FALSE;
    HfInstanceData_t *linkPtr;

    if ((instData->maxHFConnections > 0) && (instData->hfServiceRecHandle == 0))
    {
        msgSent = TRUE;
        CsrBtHfSendSdsRegisterReq(instData);
    }
    else if ((instData->maxHSConnections > 0) && (instData->hsServiceRecHandle == 0))
    {
        msgSent = TRUE;
        HsSendSdsRegisterReq(instData);
    }

    if (!msgSent)
    {
        for (i = 0;
                ((i < (instData->allocInactiveHfCons + instData->allocInactiveHsCons ) ) && (!msgSent));
                i++)
        {
            linkPtr = (HfInstanceData_t *) &(instData->inactiveLinkData[i]);

            if (linkPtr->accepting)
            {
                msgSent = TRUE;
                if (CSR_BT_HF_CONNECTION_HF == linkPtr->linkType)
                {
                    CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServerChannel, linkPtr->instId);
                }
                else
                {
                    CsrBtCmContextCancelAcceptConnectReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServerChannel, linkPtr->instId);
                }
            }
        }
    }

    if (!msgSent)
    {
        if ((instData->maxHSConnections == 0) && (instData->hsServiceRecHandle != 0))
        {
            msgSent = TRUE;
            instData->numberOfUnregister = 0;
            CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, instData->hsServiceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        }
        else if ((instData->maxHFConnections == 0) && (instData->hfServiceRecHandle != 0))
        {
            msgSent = TRUE;
            instData->numberOfUnregister = 0;
            CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, instData->hfServiceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
        }
    }

    if (!msgSent)
    {
        CsrBtHfFreeInactiveLinkData(instData->inactiveLinkData, (CsrUint8)(instData->allocInactiveHfCons + instData->allocInactiveHsCons));
        CsrPmemFree(instData->inactiveLinkData);
        instData->inactiveLinkData = NULL;
        instData->allocInactiveHfCons = 0;
        instData->allocInactiveHsCons = 0;

        instData->state = Activated_s;
        instData->reActivating = FALSE;
        CsrBtHfSendHfActivateCfm(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
        CsrBtHfAllowConnectCheck(instData);
        CsrBtHfSendHfHouseCleaning(instData);
    }
}
/************************************************************************************************************************************/
static void copyLinkPointer(HfInstanceData_t *newLinkPtr, HfInstanceData_t *oldLinkptr, HfMainInstanceData_t *instData)
{
    switch (oldLinkptr->state)
    {
        case Activate_s:
        {
            newLinkPtr->instId = oldLinkptr->instId;
            newLinkPtr->sdpSearchData = oldLinkptr->sdpSearchData;
            newLinkPtr->linkType = oldLinkptr->linkType;
            newLinkPtr->accepting = oldLinkptr->accepting;
            newLinkPtr->state = oldLinkptr->state;

            if (oldLinkptr->data != NULL)
            {
                if (oldLinkptr->data->recvAtCmd != NULL)
                {
                    CsrPmemFree(oldLinkptr->data->recvAtCmd);
                    oldLinkptr->data->recvAtCmd = NULL;
                }
                CsrPmemFree(oldLinkptr->data);
                oldLinkptr->data = NULL;
            }

            break;
        }
        case Connect_s:
        case Connected_s:
        case ServiceSearch_s:
        {
            HfHsData_t *tmpData = newLinkPtr->data;
            SynMemCpyS(newLinkPtr, sizeof(HfInstanceData_t), oldLinkptr, sizeof(HfInstanceData_t));
            newLinkPtr->data = tmpData;
            if (oldLinkptr->data != NULL)
            {
                SynMemCpyS(newLinkPtr->data, sizeof(HfHsData_t), oldLinkptr->data, sizeof(HfHsData_t));
                CsrPmemFree(oldLinkptr->data);
                /* oldLinkptr->data->recvAtCmd should not be freed since it will be part of newLinkPtr */
                oldLinkptr->data = NULL;
            }

            break;
        }
        default:
            break;
    }
    CSR_UNUSED(instData);
}

static void ReorganizeLinks(HfInstanceData_t *oldLinkptr,
                         CsrUint8 oldActiveHFCons,
                         CsrUint8 oldActiveHSCons,
                         HfMainInstanceData_t *instData)
{
    HfInstanceData_t *inactiveLinkPtr = instData->inactiveLinkData;
    HfInstanceData_t *newLinkPtr = instData->linkData;
    CsrUint8 newActiveHFCons = instData->maxHFConnections;
    CsrUint8 newActiveHSCons = instData->maxHSConnections;
    CsrIntFast8 i;
    CsrUint8 newInactiveHf = 0;
    CsrUint8 newInactiveHs = 0;

    if (newActiveHFCons >= oldActiveHFCons)
    {
        newInactiveHf = 0;
    }
    else
    {
        newInactiveHf = oldActiveHFCons - newActiveHFCons;
    }

    if (newActiveHSCons >= oldActiveHSCons)
    {
        newInactiveHs = 0;
    }
    else
    {
        newInactiveHs = oldActiveHSCons - newActiveHSCons;
    }

    /* Copy the active HF instances to newLinkPtr and inactive HF instances to inactiveLinkPtr */
    for (i = 0; i < oldActiveHFCons; i++)
    {
        CsrIntFast8 m;
        if (i < newActiveHFCons)
        {
            m = i;
            copyLinkPointer(&newLinkPtr[m], &oldLinkptr[i], instData);
        }
        else
        {
            m = i - newActiveHFCons;
            copyLinkPointer(&inactiveLinkPtr[m], &oldLinkptr[i], instData);
        }
    }

    /* All HF instances are redistributed to newLinkPtr and inactiveLinkPtr.
     * Now do the same for HS instances */
    for (i = 0; i < oldActiveHSCons; i++)
    {
        CsrIntFast8 j;
        CsrIntFast8 m;
        j = i + oldActiveHFCons;
        if (i < newActiveHSCons)
        {
            m = i + newActiveHFCons;
            copyLinkPointer(&newLinkPtr[m], &oldLinkptr[j], instData);
        }
        else
        {
            m = newInactiveHf + i - newActiveHSCons;
            copyLinkPointer(&inactiveLinkPtr[m], &oldLinkptr[j], instData);
        }
    }

    /* Now that all the active links have been run through make sure to keep track
     * of the inactive ones: the ones that were used before, but are not needed now */
    instData->allocInactiveHfCons = newInactiveHf;
    instData->allocInactiveHsCons = newInactiveHs;
}

/********************************************************************************************************
    Organize service records so records engaged in a connection are placed as the first in the list.
********************************************************************************************************/
static void reOrderRecords(HfMainInstanceData_t * instData, CsrBtHfConnectionType type,CsrUint8 usedRecords)
{
    CsrUintFast8 i,j;
    HfInstanceData_t *linkPtr, *tmpPtr;
    HfInstanceData_t copyPtr;
    CsrUint8 start = 0, end = instData->maxHFConnections;
    CsrBool moved = FALSE;

    if (type == CSR_BT_HF_CONNECTION_HS)
    {
        start   = instData->maxHFConnections;
        end     += instData->maxHSConnections;
    }

    for (i=start; ((i < end) && (usedRecords > 0)); i++)
    {
        linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
        if (linkPtr->state != Activate_s)
        {/* service record in use: move to start of array */
            usedRecords--;
            moved = FALSE;
            for (j = start; ((j < i) && (!moved)); j++)
            {
                tmpPtr = (HfInstanceData_t *)&(instData->linkData[j]);
                if (tmpPtr->state == Activate_s)
                {/* Free entry found: do the switch! */
                    SynMemCpyS(&copyPtr, sizeof(HfInstanceData_t), tmpPtr,sizeof(HfInstanceData_t));
                    SynMemCpyS(tmpPtr, sizeof(HfInstanceData_t), linkPtr,sizeof(HfInstanceData_t));
                    SynMemCpyS(linkPtr, sizeof(HfInstanceData_t), &copyPtr,sizeof(HfInstanceData_t));
                    moved = TRUE;
                }
            }
        }
    }
}

/********************************************************************************************************
    Generate a unique instance id which is the lowest.
********************************************************************************************************/
static CsrUint8 csrBtHfGetNewInstanceId(HfMainInstanceData_t * instData, CsrUint8 totalNumber, CsrUint8 totalInactiveNumber)
{ /* Instance IDs to start from 1 since 0 cannot be used as CM's context */
    CsrUint8 currInstId = 1;
    CsrUint8 i = 0;

    while (i < (totalNumber + totalInactiveNumber))
    {
        HfInstanceData_t *linkPtr = NULL;
        if (i < totalNumber)
        {
            linkPtr = &instData->linkData[i];
        }
        else
        {
            linkPtr = &instData->inactiveLinkData[i - totalNumber];
        }

        if (linkPtr->instId == currInstId)
        {/* Instance ID already in use - skip to next number and restart */
            currInstId++;
            i = 0;
        }
        else
        {
            i++;
        }
    }
    /* Lowest possible instance ID found */
    return currInstId;
}

/*************************************************************************************
    Perform registration and allocation of servers towards the CM depending on the
    wishes of the application
************************************************************************************/
void CsrBtHfXStateActivateReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfActivateReq *prim;
    CsrIntFast8 i = 0;
    CsrUint8 totalNumber = 0;
    CsrUint8 totalInactiveNumber = 0;
    CsrUint8 allocatedHfRecords = instData->allocInactiveHfCons + instData->maxHFConnections;
    CsrUint8 allocatedHsRecords = instData->allocInactiveHsCons + instData->maxHSConnections;
    CsrBool  cmRegister = FALSE;
    CsrUint8 oldMaxHfCon = instData->maxHFConnections;
    CsrUint8 oldMaxHsCon = instData->maxHSConnections;
    HfInstanceData_t    *linkPtr, *oldLinkPtr = NULL;
    CsrUint8 nrActiveHf = 0;
    CsrUint8 nrActiveHs = 0;
    CsrUint8 nrOfActiveConnections = CsrBtHfGetNumberOfRecordsInUse(instData,&nrActiveHf,&nrActiveHs);

    prim = (CsrBtHfActivateReq *) instData->recvMsgP;

    instData->appHandle = prim->phandle;

    if (((prim->maxHFConnections + prim->maxHSConnections) == 0) ||
        (prim->maxSimultaneousConnections == 0) ||
        (prim->maxSimultaneousConnections < nrOfActiveConnections) ||
        (prim->maxHFConnections < nrActiveHf) ||
        (prim->maxHSConnections < nrActiveHs))
    {/* Invalid parameters: answer back and ignore! */
        CsrBtHfSendHfActivateCfm(instData,CSR_BT_RESULT_CODE_HF_INVALID_PARAMETER, CSR_BT_SUPPLIER_HF);
        return;
    }
    if (instData->reActivating)
    {/* Already activating.... reject! */
        CsrBtHfSendHfActivateCfm(instData, CSR_BT_RESULT_CODE_HF_REJ_RESOURCES, CSR_BT_SUPPLIER_HF);
        return;
    }
    else
    {
        instData->reActivating = TRUE;
    }

    instData->localSupportedFeatures = prim->supportedFeatures;

    /* Application has to always send supported list of HF indicators during Activation if
       support for HF Indicator feature is enabled */
    if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_HF_INDICATORS)
    {
        if (prim->hfSupportedHfIndicators == NULL)
        {
            /* Should support at least one HF Indicator */
            instData->reActivating = FALSE;
            CsrBtHfSendHfActivateCfm(instData,CSR_BT_RESULT_CODE_HF_INVALID_PARAMETER, CSR_BT_SUPPLIER_HF);
            return;
        }
        else
        {
            
            if(instData->localHfIndicatorList != NULL)
            {/* Handling an activate request in Activated_s state */
                CsrPmemFree(instData->localHfIndicatorList);
                instData->localHfIndicatorList = NULL;
                instData->indCount= 0;
            }

            instData->indCount = (CsrUint8)prim->hfSupportedHfIndicatorsCount;
            instData->localHfIndicatorList = (CsrBtHfpHfIndicatorId *) CsrPmemZalloc(
                sizeof(CsrBtHfpHfIndicatorId) * prim->hfSupportedHfIndicatorsCount);
            SynMemCpyS(instData->localHfIndicatorList,
                sizeof(CsrBtHfpHfIndicatorId) * prim->hfSupportedHfIndicatorsCount,
                prim->hfSupportedHfIndicators,
                sizeof(CsrBtHfpHfIndicatorId) * instData->indCount);

            CsrPmemFree(prim->hfSupportedHfIndicators);
        }
    }

    if ((prim->maxHFConnections) && (CSR_BT_NO_SERVER == instData->hfServerChannel))
    {/* Tell the SD that it must look for the CSR_BT_HFG_PROFILE_UUID service,
       when it perform a SD_READ_AVAILABLE_SERVICE_REQ                  */
        cmRegister = TRUE;
#ifndef EXCLUDE_CSR_BT_SD_MODULE
        CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_HFG_PROFILE_UUID);
#endif
    }
    if ((prim->maxHSConnections) && (CSR_BT_NO_SERVER == instData->hsServerChannel))
    {/* Tell the SD that it must look for the CSR_BT_AG_PROFILE_UUID service,
       when it perform a SD_READ_AVAILABLE_SERVICE_REQ                  */
        cmRegister = TRUE;
#ifndef EXCLUDE_CSR_BT_SD_MODULE
        CsrBtSdRegisterAvailableServiceReqSend(CSR_BT_HEADSET_AG_SERVICE_UUID);
#endif
    }

    if (nrOfActiveConnections > 0)
    {/* Organize first if needed so used records are placed as the first ones in the array */
        if (nrActiveHf > 0)
        {
            reOrderRecords(instData, CSR_BT_HF_CONNECTION_HF, nrActiveHf);
        }
        if (nrActiveHs > 0)
        {
            reOrderRecords(instData, CSR_BT_HF_CONNECTION_HS, nrActiveHs);
        }
    }

    if((prim->hfConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) != 0)
    {
        instData->mainConfig = (prim->hfConfig |
                                CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CLIP_ACTIVATION |
                                CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CCWA_ACTIVATION |
                                CSR_BT_HF_CNF_DISABLE_AUTOMATIC_CMEE_ACTIVATION);
    }
    else
    {
        instData->mainConfig = prim->hfConfig;
    }
    if (prim->atResponseTime > CSR_BT_AT_DEFAULT_RESPONSE_TIME)
    {
        instData->atRespWaitTime = prim->atResponseTime;
    }
    instData->maxHFConnections = prim->maxHFConnections;
    instData->maxHSConnections = prim->maxHSConnections;
    instData->maxTotalSimultaneousConnections = prim->maxSimultaneousConnections;
    totalNumber = instData->maxHSConnections + instData->maxHFConnections;

    if (instData->linkData != NULL)
    {/* Keep the old data to copy it if needed */
        oldLinkPtr = instData->linkData;
    }

    if (allocatedHfRecords > instData->maxHFConnections)
    {
        totalInactiveNumber += allocatedHfRecords - instData->maxHFConnections;
    }
    if (allocatedHsRecords > instData->maxHSConnections)
    {
        totalInactiveNumber += allocatedHsRecords - instData->maxHSConnections;
    }
    /* totalNumber shall indicate how many active records there shall be */
    /* totalInactiveNumber shall now indicate how many inactive records there shall be */

    instData->linkData = (HfInstanceData_t *) CsrPmemAlloc(sizeof(HfInstanceData_t) * totalNumber);
    if (totalInactiveNumber > 0)
    {
        instData->inactiveLinkData = (HfInstanceData_t *) CsrPmemAlloc(sizeof(HfInstanceData_t) * totalInactiveNumber);
    }

    for (i = 0; i < (totalNumber + totalInactiveNumber); i++)
    {
        if (i < totalNumber)
        {
            linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        }
        else
        {
            linkPtr = (HfInstanceData_t *) &(instData->inactiveLinkData[i - totalNumber]);
        }

        CsrMemSet(linkPtr, 0x00, sizeof(HfInstanceData_t));
        linkPtr->data = (HfHsData_t *) CsrPmemZalloc(sizeof(HfHsData_t));

        linkPtr->instId             = CSR_BT_NO_SERVER;
        linkPtr->sdpSearchData      = NULL;
        linkPtr->hfConnId           = CSR_BT_CONN_ID_INVALID;
        linkPtr->accepting          = FALSE;
        CSR_BT_HF_MAIN_INSTANCE_SET(linkPtr, instData);
        CsrBtHfInitInstanceData(linkPtr);
    }

    if (oldLinkPtr != NULL)
    {
        ReorganizeLinks(oldLinkPtr, oldMaxHfCon, oldMaxHsCon, instData);
        /* De-allocate the old heap area. No need to free the data since it is already freed as part of ReorganizeLinks */
        CsrPmemFree(oldLinkPtr);
        oldLinkPtr = NULL;
    }

    for (i = 0; i < totalNumber; i++)
    {
        linkPtr = (HfInstanceData_t *) &(instData->linkData[i]);
        if (CSR_BT_NO_SERVER == linkPtr->instId)
        {
            linkPtr->instId = csrBtHfGetNewInstanceId(instData, totalNumber, totalInactiveNumber);

            if (i < instData->maxHFConnections)
            {
                linkPtr->linkType = CSR_BT_HF_CONNECTION_HF;
            }
            else
            {
                linkPtr->linkType = CSR_BT_HF_CONNECTION_HS;
            }
        }
    }

    /* Send a register to CM to allocate the server channel to be used for incoming connections. */
    if (cmRegister)
    {
        CsrBtCmPublicRegisterExtReqSend(CSR_BT_HF_IFACEQUEUE,
                                        CSR_BT_CM_CONTEXT_UNUSED,
                                        (prim->maxHFConnections ? CSR_BT_HF_DEFAULT_SERVER_HF: CSR_BT_HF_DEFAULT_SERVER_HS),
                                        CM_REGISTER_OPTION_APP_CONNECTION_HANDLING);
    }
    else
    {
        commonActivateHandler(instData);
    }
}


/*************************************************************************************
    The HF is now registered in the CM and a local server channel is allocated. Go on
    and register the allocated server channel in SDS for the HF.
************************************************************************************/
void CsrBtHfNullStateDeactivateReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfSendHfDeactivateCfm(instData, CSR_BT_RESULT_CODE_HF_SUCCESS, CSR_BT_SUPPLIER_HF);
}

void CsrBtHfNullStateCmRegisterCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmRegisterCfm *prim;
    CsrBool waitForNextRegister = FALSE;

    prim = (CsrBtCmRegisterCfm *) instData->recvMsgP;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        /* When both HF and HS need to be registered, first register is for HF and second for HS */
        if ((instData->maxHFConnections) && (CSR_BT_NO_SERVER == instData->hfServerChannel))
        {
            instData->hfServerChannel = prim->serverChannel;
            if ((instData->maxHSConnections) && (CSR_BT_NO_SERVER == instData->hsServerChannel))
            {
                CsrBtCmPublicRegisterExtReqSend(CSR_BT_HF_IFACEQUEUE,
                                                CSR_BT_CM_CONTEXT_UNUSED,
                                                CSR_BT_HF_DEFAULT_SERVER_HS,
                                                CM_REGISTER_OPTION_APP_CONNECTION_HANDLING);
                waitForNextRegister = TRUE;
            }
        }
        else if ((instData->maxHSConnections) && (CSR_BT_NO_SERVER == instData->hsServerChannel))
        {
            instData->hsServerChannel = prim->serverChannel;
        }

        if(!waitForNextRegister)
        {
            CsrBtHfSendHfHouseCleaning(instData);
            /* Now make sure to finish activation process: start SDS record setup */
            commonActivateHandler(instData);
        }
    }
    else if (prim->context != CSR_BT_HF_CHANNEL_REGISTER_CONTEXT_RETRY)
    {
        CsrUint8 serverChannel;

        if (instData->maxHFConnections &&
            instData->hfServerChannel == CSR_BT_NO_SERVER)
        {
            serverChannel = CSR_BT_HF_DEFAULT_SERVER_HF;
        }
        else
        {
            serverChannel = CSR_BT_HF_DEFAULT_SERVER_HS;
        }

        CsrBtCmPublicRegisterExtReqSend(CSR_BT_HF_IFACEQUEUE,
                                        CSR_BT_HF_CHANNEL_REGISTER_CONTEXT_RETRY,
                                        serverChannel,
                                        CM_REGISTER_OPTION_APP_CONNECTION_HANDLING);
    }
    else
    {
        /* Retry attempt also failed, panic */
    }
}

#ifdef CSR_BT_INSTALL_HF_CONFIG_AUDIO
/************************************************************************************
    Configure audio settings to be used when the profile is ready to use them
*************************************************************************************/
void CsrBtHfXStateConfigAudioReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfConfigAudioReq *prim;
    prim = (CsrBtHfConfigAudioReq *) instData->recvMsgP;

    if ((instData->state != Activated_s) || (prim->connectionId == CSR_BT_HF_CONNECTION_ALL))
    {/* If connectionId == 0xFFFFFFFF, all existing connections shall be updated! */
        switch(prim->audioType)
        {
            case CSR_BT_HF_AUDIO_OUTGOING_PARAMETER_LIST_CONFIG:
            {
                CsrBtHfAudioLinkParameterListConfig *audSettings = (CsrBtHfAudioLinkParameterListConfig *)prim->audioSetting;

                instData->generalAudioParams.theAudioQuality  = audSettings->packetType;
                instData->generalAudioParams.theTxBandwidth  = audSettings->txBandwidth;
                instData->generalAudioParams.theRxBandwidth  = audSettings->rxBandwidth;
                instData->generalAudioParams.theMaxLatency  = audSettings->maxLatency;
                instData->generalAudioParams.theVoiceSettings = audSettings->voiceSettings;
                instData->generalAudioParams.theReTxEffort = audSettings->reTxEffort;

                break;
            }
            default:
                break;
        }
    }
    else if ((CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId)) &&
             (instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF))
    {
        HfInstanceData_t  *linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
        switch(prim->audioType)
        {
            case CSR_BT_HF_AUDIO_OUTGOING_PARAMETER_LIST_CONFIG:
            {
                CsrBtHfAudioLinkParameterListConfig *audSettings = (CsrBtHfAudioLinkParameterListConfig *)prim->audioSetting;
                if(!linkPtr->audioSetupParams)
                {
                    linkPtr->audioSetupParams = CsrPmemAlloc(sizeof(audioSetupParams_t));
                }

                linkPtr->audioSetupParams->theAudioQuality  = audSettings->packetType;
                linkPtr->audioSetupParams->theTxBandwidth  = audSettings->txBandwidth;
                linkPtr->audioSetupParams->theRxBandwidth  = audSettings->rxBandwidth;
                linkPtr->audioSetupParams->theMaxLatency  = audSettings->maxLatency;
                linkPtr->audioSetupParams->theVoiceSettings = audSettings->voiceSettings;
                linkPtr->audioSetupParams->theReTxEffort = audSettings->reTxEffort;
                break;
            }
            default:
                break;
        }
        
    }

    /* Send audio configuration confirmation */
    CsrBtHfSendConfirmMessage(instData,CSR_BT_HF_CONFIG_AUDIO_CFM);
    CsrPmemFree(prim->audioSetting);
}
#endif /* CSR_BT_INSTALL_HF_CONFIG_AUDIO */

/************************************************************************************
    Null / Activated State
*************************************************************************************/
void CsrBtHfXStateCmSdsRegisterCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmSdsExtRegisterCfm * prim;
    prim = (CsrBtCmSdsExtRegisterCfm *) instData->recvMsgP;

    if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        if ((CsrUint8) prim->context == instData->hfServerChannel)
        {
            instData->hfServiceRecHandle = prim->serviceRecHandle;
        }
        else
        {
            instData->hsServiceRecHandle = prim->serviceRecHandle;
        }
        commonActivateHandler(instData);
    }
    else
    {
        if ((CsrUint8) prim->context == instData->hsServerChannel)
        {
            HsSendSdsRegisterReq(instData);
        }
        else
        {
            CsrBtHfSendSdsRegisterReq(instData);
        }
    }
}

/****************************************************************************
    Activated State
*****************************************************************************/
void CsrBtHfActivatedStateCmCancelAcceptConnectCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmCancelAcceptConnectCfm * prim;
    prim = (CsrBtCmCancelAcceptConnectCfm *) instData->recvMsgP;

    if (!(CsrBtHfSetCurrentConnIndexFromInstId(instData, (CsrUint8) prim->context)))
    {
        CsrIntFast8 i = 0;
        HfInstanceData_t *linkPtr;

        for (i = 0;
                i < (instData->allocInactiveHfCons + instData->allocInactiveHsCons);
                i++)
        {
            linkPtr = (HfInstanceData_t *) &(instData->inactiveLinkData[i]);

            if (linkPtr->instId == (CsrUint8) prim->context)
            {
                if((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS  || prim->resultCode == CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL)&&
                    prim->resultSupplier == CSR_BT_SUPPLIER_CM)
                {
                    linkPtr->accepting = FALSE;
                    commonActivateHandler(instData);
                }
                break;
            }
        }
    }
}

void CsrBtHfActivatedStateCmConnectAcceptCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmConnectAcceptCfm * prim;
    prim = (CsrBtCmConnectAcceptCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromInstId(instData, (CsrUint8) prim->context))
    {
        if(prim->resultCode != CSR_BT_RESULT_CODE_CM_ALREADY_CONNECTING)
        {
            instData->linkData[instData->index].accepting = FALSE;
        }
        CsrBtHfpHandler(instData);
    }
    else
    {
        if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
           prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
}

void CsrBtHfActivatedStateCmDisconnectIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDisconnectInd * prim;
    prim = (CsrBtCmDisconnectInd *) instData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
        CsrBtCmRfcDisconnectRspSend(prim->btConnId);
    }
#endif

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        if (prim->status)
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                             instData->linkData[instData->index].currentDeviceAddress,
                                             prim->btConnId);

            /* Unregister the CM Server channel obtained by SDC RFC lib for outgoing connection */
            if (instData->linkData[instData->index].obtainedServerCh != CSR_BT_NO_SERVER)
            {
                CsrBtCmUnRegisterReqSend(instData->linkData[instData->index].obtainedServerCh);
                instData->linkData[instData->index].obtainedServerCh = CSR_BT_NO_SERVER;
            }
        }

        CsrBtHfpHandler(instData);
        /* Make sure that the allowed connections can be established now...*/
        CsrBtHfAllowConnectCheck(instData);
    }
}

void CsrBtHfActivatedStateCmScoConnectCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmScoConnectCfm * prim;
    prim = (CsrBtCmScoConnectCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
}

void CsrBtHfActivatedStateCmScoDisconnectIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmScoDisconnectInd * prim;
    prim = (CsrBtCmScoDisconnectInd *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
}

void CsrBtHfActivatedStateCmScoAcceptConnectCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmScoAcceptConnectCfm * prim;
    prim = (CsrBtCmScoAcceptConnectCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
}

void CsrBtHfActivatedStateCmDataIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDataInd * prim;
    prim = (CsrBtCmDataInd *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
}

void CsrBtHfActivatedStateCmDataCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDataCfm * prim;
    prim = (CsrBtCmDataCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
}
/* Just ignore CSR_BT_CM_CONTROL_IND: it is not used at all....*/
void CsrBtHfXStateIgnoreCmControlIndHandler(HfMainInstanceData_t * instData)
{
    CSR_UNUSED(instData);
}

void CsrBtHfActivatedStateCmPortnegIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtRespondCmPortNegInd(instData->recvMsgP);
}

void HfActivatedStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmRfcConnectAcceptInd *prim = (CsrBtCmRfcConnectAcceptInd *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromBdAddr(instData, prim->deviceAddr))
    { /* device is known, let the connection instance handler handle it */
        CsrBtHfpHandler(instData);
    }
    else
    { /* device is unknown, connect ind can be responded here */
        CsrBtCmRfcConnectAcceptRspSend(CSR_BT_HF_IFACEQUEUE,
                                       prim->btConnId,
                                       prim->deviceAddr,
                                       HfIsNewConnectionAllowed(instData, prim->localServerChannel),
                                       prim->localServerChannel,
                                       CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                       CSR_BT_DEFAULT_BREAK_SIGNAL,
                                       CSR_BT_DEFAULT_MSC_TIMEOUT);
    }
}

void CsrBtHfDeactivateStateCmSdsRegisterCfmHandler(HfMainInstanceData_t * instData)
{/* Unregister immediately! */
    CsrBtCmSdsExtRegisterCfm * prim = (CsrBtCmSdsExtRegisterCfm *) instData->recvMsgP;
    if ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS) &&
        (prim->resultSupplier == CSR_BT_SUPPLIER_CM))
    {
        instData->numberOfUnregister = 0;
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, prim->serviceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
    }
}

void CsrBtHfActivatedStateCmSdsUnregisterCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmSdsUnregisterCfm *prim = (CsrBtCmSdsUnregisterCfm *) instData->recvMsgP;

    instData->numberOfUnregister++;
    if ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS && prim->resultSupplier == CSR_BT_SUPPLIER_CM) || instData->numberOfUnregister == HF_NUMBER_OF_UNREGISTER)
    {
        if (instData->hfServiceRecHandle == prim->serviceRecHandle)
        {
            instData->hfServiceRecHandle = 0;
        }
        else if (instData->hsServiceRecHandle == prim->serviceRecHandle)
        {
            instData->hsServiceRecHandle = 0;
        }
        commonActivateHandler(instData);
    }
    else
    {
        /* unregister failed, try again */
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, prim->serviceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
    }
}

/**************************************************************************************
    Deactivate state
***************************************************************************************/
void CsrBtHfDeactivateStateCmCancelAcceptConnectCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmCancelAcceptConnectCfm * prim;
    prim = (CsrBtCmCancelAcceptConnectCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromInstId(instData, (CsrUint8) prim->context) &&
        ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS  || 
         prim->resultCode == CSR_BT_RESULT_CODE_CM_NOTHING_TO_CANCEL) &&
         prim->resultSupplier == CSR_BT_SUPPLIER_CM))
    {
        instData->linkData[instData->index].accepting = FALSE;
        CsrBtHfHsDeactivateHandler(instData);
    }
    /* else wait for CSR_BT_CM_CONNECT_ACCEPT_CFM and disconnect */
}


void CsrBtHfDeactivateStateCmConnectAcceptCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmConnectAcceptCfm * prim;
    prim = (CsrBtCmConnectAcceptCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromInstId(instData, (CsrUint8) prim->context))
    {
        /*Save Connection ID to use while handling Disconnect Indication*/
        instData->linkData[instData->index].hfConnId = prim->btConnId;

        if(prim->resultCode != CSR_BT_RESULT_CODE_CM_ALREADY_CONNECTING)
        {
            instData->linkData[instData->index].accepting = FALSE;
        }

        if (prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
            prim->resultSupplier == CSR_BT_SUPPLIER_CM)
        {
            /* This means that CSR_BT_CM_CANCEL_ACCEPT_CONNECT_CFM has failed, send disconnect. */
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_ACTIVE_CONTROL_CHANNEL, prim->deviceAddr, prim->btConnId);
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
}

void CsrBtHfDeactivateStateCmDisconnectIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDisconnectInd * prim;
    prim = (CsrBtCmDisconnectInd *) instData->recvMsgP;

#ifdef CSR_BT_INSTALL_CM_DISABLE_AUTO_DISC_RESP
    if (!prim->localTerminated)
    {
        /* For remote disconnections, profile needs to respond to RFC_DISCONNECT_IND. */
        CsrBtCmRfcDisconnectRspSend(prim->btConnId);
    }
#endif

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        HfInstanceData_t *linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

        if (prim->status)
        {
            CsrBtCmLogicalChannelTypeReqSend(CSR_BT_NO_ACTIVE_LOGICAL_CHANNEL,
                                             linkPtr->currentDeviceAddress,
                                             prim->btConnId);

            /* Unregister the CM Server channel obtained by SDC RFC lib for outgoing connection */
            if (linkPtr->obtainedServerCh != CSR_BT_NO_SERVER)
            {
                CsrBtCmUnRegisterReqSend(linkPtr->obtainedServerCh);
                linkPtr->obtainedServerCh = CSR_BT_NO_SERVER;
            }
            linkPtr->state = Activate_s;
            CsrBtHfHsDeactivateHandler(instData);
        }
        else
        {
            if (CSR_BT_HF_CONNECTION_UNKNOWN != linkPtr->linkType)
            {
                CsrBtCmDisconnectReqSend(prim->btConnId);
            }
        }
    }
}

void CsrBtHfDeactivateStateCmScoConnectCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmScoConnectCfm * prim = (CsrBtCmScoConnectCfm *) instData->recvMsgP;

    /* this signal should not come in Deactivate_s, but may occur as a result of a race condition */
    if(prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
       prim->resultSupplier == CSR_BT_SUPPLIER_CM)
    {
        CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, prim->btConnId);
    }
}

void CsrBtHfDeactivateStateCmScoDisconnectIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmScoDisconnectInd * prim;
    prim = (CsrBtCmScoDisconnectInd *) instData->recvMsgP;

    CsrBtCmDisconnectReqSend(prim->btConnId);
}

void CsrBtHfDeactivateStateCmScoAcceptConnectCfm(HfMainInstanceData_t * instData)
{
    CsrBtCmScoAcceptConnectCfm * prim;
    CsrIntFast8 i = 0;
    
    prim = (CsrBtCmScoAcceptConnectCfm *) instData->recvMsgP;
    
    CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, prim->btConnId);
    for (i=0;i<(instData->maxHFConnections + instData->maxHSConnections);i++)
    {
        instData->linkData[i].scoConnectAcceptPending = FALSE;
    }
}

void CsrBtHfDeactivateStateCmDataIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDataInd * prim;
    prim = (CsrBtCmDataInd *) instData->recvMsgP;
#ifndef CSR_STREAMS_ENABLE
    CsrBtCmDataResSend(prim->btConnId);
#endif

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        HfInstanceData_t *linkPtr;
        linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

        if ((linkPtr->state == Connect_s) && (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS))
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
    CsrPmemFree (prim->payload);
}

void CsrBtHfDeactivateStateCmDataCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmDataCfm * prim;
    prim = (CsrBtCmDataCfm *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->btConnId))
    {
        HfInstanceData_t *linkPtr;
        linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

        if ((linkPtr->state == Connect_s) && (linkPtr->linkType == CSR_BT_HF_CONNECTION_HS))
        {
            CsrBtCmDisconnectReqSend(prim->btConnId);
        }
    }
}

void CsrBtHfDeactivateStateCmControlIndHandler(HfMainInstanceData_t * instData)
{
    /* ignore in deactivate */
    CSR_UNUSED(instData);
}

void HfDeactivateStateCmRfcConnectAcceptIndHandler(HfMainInstanceData_t *instData)
{
    CsrBtCmRfcConnectAcceptInd *prim = (CsrBtCmRfcConnectAcceptInd *) instData->recvMsgP;

    CsrBtCmRfcConnectAcceptRspSend(CSR_BT_HF_IFACEQUEUE,
                                   prim->btConnId,
                                   prim->deviceAddr,
                                   FALSE,
                                   prim->localServerChannel,
                                   CSR_BT_MODEM_SEND_CTRL_DTE_DEFAULT,
                                   CSR_BT_DEFAULT_BREAK_SIGNAL,
                                   CSR_BT_DEFAULT_MSC_TIMEOUT);
}

void CsrBtHfDeactivateStateCmSdsUnregisterCfmHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmSdsUnregisterCfm * prim;
    prim = (CsrBtCmSdsUnregisterCfm *) instData->recvMsgP;

    instData->numberOfUnregister++;
    if ((prim->resultCode == CSR_BT_RESULT_CODE_CM_SUCCESS &&
          prim->resultSupplier == CSR_BT_SUPPLIER_CM) ||
         instData->numberOfUnregister == HF_NUMBER_OF_UNREGISTER)
    {
        if (instData->hfServiceRecHandle == prim->serviceRecHandle)
        {
            instData->hfServiceRecHandle = 0;
        }
        else if (instData->hsServiceRecHandle == prim->serviceRecHandle)
        {
            instData->hsServiceRecHandle = 0;
        }
        CsrBtHfHsDeactivateHandler(instData);
    }
    else
    {
        /* unregister failed, try again */
        CsrBtCmSdsUnRegisterReqSend(CSR_BT_HF_IFACEQUEUE, prim->serviceRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
    }
}


void CsrBtHfCommonAtCmdPrimReqHandler(HfMainInstanceData_t *instData, CsrBtHfPrim *primType)
{
#if defined (CSR_TARGET_PRODUCT_WEARABLE)
    CSR_LOG_TEXT_INFO((CsrBtHfLto, 0, "CsrBtHfCommonAtCmdPrimReqHandler MESSAGE:CsrBtHfPrim:0x%0x, State:%d, ConnID:0x%08x", 
                      *primType, instData->state, instData->linkData->hfConnId));
#endif
    switch(*primType)
    {
        case CSR_BT_HF_GET_ALL_STATUS_INDICATORS_REQ:
        {
            CsrBtHfXStateHfGetAllStatusReqHandler(instData);
            break;
        }
        case CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_REQ:
        {
            CsrBtHfXStateHfCopsReqHandler(instData);
            break;
        }
        case CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_REQ:
        {
            CsrBtHfXStateHfSubscriberReqHandler(instData);
            break;
        }
        case CSR_BT_HF_GET_CURRENT_CALL_LIST_REQ:
        {
            CsrBtHfXStateHfCallListReqHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_REQ:
        {
            CsrBtHfXStateHfSetExtErrorReqHandler(instData);
            break;
        }
        case  CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_REQ:
        {
            CsrBtHfXStateHfSetClipHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_REQ:
        {
            CsrBtHfXStateHfSetCcwaHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_REQ:
        {
            CsrBtHfActivatedSetStatusIndUpdateReqHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_ECHO_AND_NOISE_REQ:
        {
            CsrBtHfXStateHfSetNrecHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_VOICE_RECOGNITION_REQ:
        {
            CsrBtHfXStateHfSetBvraHandler(instData);
            break;
        }
        case CSR_BT_HF_BT_INPUT_REQ:
        {
            CsrBtHfActivatedBtInputReqHandler(instData);
            break;
        }
        case CSR_BT_HF_GENERATE_DTMF_REQ:
        {
            CsrBtHfXStateDTMFReqHandler(instData);
            break;
        }
        case CSR_BT_HF_SPEAKER_GAIN_STATUS_REQ:
        {
            CsrBtHfActivatedStateSpeakerGainStatusReqHandler(instData);
            break;
        }
        case CSR_BT_HF_MIC_GAIN_STATUS_REQ:
        {
            CsrBtHfActivatedStateMicGainStatusReqHandler(instData);
            break;
        }
        case CSR_BT_HF_DIAL_REQ:
        {
            CsrBtHfActivatedDialRequest(instData);
            break;
        }
        case CSR_BT_HF_CALL_ANSWER_REQ:
        {
            CsrBtHfActivatedStateAnswerReqHandler(instData);
            break;
        }
        case CSR_BT_HF_CALL_END_REQ:
        {
            CsrBtHfActivatedStateRejectReqHandler(instData);
            break;
        }
        case CSR_BT_HF_CALL_HANDLING_REQ:
        {
            CsrBtHfActivatedStateChldReqHandler(instData);
            break;
        }
        case CSR_BT_HF_AT_CMD_REQ:
        {
            CsrBtHfActivatedStateAtCmdReqHandler(instData);
            break;
        }
        case CSR_BT_HF_INDICATOR_ACTIVATION_REQ:
        {
            CsrBtHfXStateIndicatorActivationReqHandler(instData);
            break;
        }
        case CSR_BT_HF_SET_HF_INDICATOR_VALUE_REQ:
        {
            CsrBtHfXStateHfSetHfIndicatorValueHandler(instData);
            break;
        }
    }
}

/* All primitives handled through CsrBtHfXStateHfCommonAtCmdReqHandler must have data structures
   with first 2 parameters as prim type and connection ID as in .CsrBtHfDataPrim */
void CsrBtHfXStateHfCommonAtCmdReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfDataPrim *prim;
    prim = (CsrBtHfDataPrim *) instData->recvMsgP;
    
    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId))
    {
        if (instData->linkData[instData->index].lastAtCmdSent != idleCmd)
        {/* Save the whole message. */
            CsrMessageQueuePush(&instData->linkData[instData->index].data->cmDataReqQueue,
                                CSR_BT_HF_PRIM,
                                instData->recvMsgP);
            instData->recvMsgP = NULL;
        }
        else
        {
            CsrBtHfCommonAtCmdPrimReqHandler(instData, &prim->type);
        }
    }
}

void CsrBtHfXStateHfUpdateCodecSupportReqHandler(HfMainInstanceData_t *instData)
{
    /* First update the supported codec list */
    CsrBtHfUpdateSupportedCodecReq *prim = (CsrBtHfUpdateSupportedCodecReq *) instData->recvMsgP;
    CsrBool changed = FALSE;
    
    if (prim->enable && !(instData->supportedCodecsMask & prim->codecMask) )
    {
        changed = TRUE;
        instData->supportedCodecsMask |= prim->codecMask;
    }
    else if (!prim->enable && (instData->supportedCodecsMask & prim->codecMask))
    {/* Disable */
        changed = TRUE;
        instData->supportedCodecsMask ^= prim->codecMask;
    }

    if (changed && prim->sendUpdate)
    {/* Indicate the change to the remote device if needed */
        CsrUint8 i;
        for (i=0; i < instData->maxHFConnections ;i++)
        {
            HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
            if (linkPtr->state == Connected_s)
            {/* Handsfree connection found: is codec negotiation supported on it? */
                if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION &&
                    linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION)
                {/* Yes: let the remote know about the change */
                    instData->index = i;
                    sendCodecSupport(instData);
                }
            }
        }
    }
    if (instData->state == Activated_s)
    {
        CsrBtHfSendUpdateCodecSupportedCfm(instData);
    }
}

#ifdef HF_ENABLE_OPTIONAL_CODEC_SUPPORT
void HfXStateHfUpdateOptionalCodecReqHandler(HfMainInstanceData_t *instData)
{
    HfUpdateOptionalCodecReq *prim = (HfUpdateOptionalCodecReq *) instData->recvMsgP;
    CsrUint8 i, codecSize = sizeof(HfCodecId) * prim->codecCount;
    CsrBtResultCode result = CSR_BT_RESULT_CODE_HF_SUCCESS;

    if (prim->codecCount > 0 && prim->codecIdList == NULL)
    {
        result = CSR_BT_RESULT_CODE_HF_INVALID_PARAMETER;
    }
    else
    {
        if (instData->optionalCodecList != NULL)
        {
            CsrPmemFree(instData->optionalCodecList);
            instData->optionalCodecList = NULL;
            instData->optionalCodecCount = 0;
        }

        instData->optionalCodecCount = prim->codecCount;
        if (prim->codecIdList != NULL)
        {
            instData->optionalCodecList = (HfCodecId *) CsrPmemZalloc(codecSize);
            SynMemCpyS(instData->optionalCodecList, codecSize, prim->codecIdList, codecSize);
        }
        
        CsrPmemFree(prim->codecIdList);
        prim->codecIdList = NULL;

        for (i=0; i < instData->maxHFConnections ;i++)
        {
            HfInstanceData_t *linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
            if (linkPtr->state == Connected_s)
            {
                if (instData->localSupportedFeatures & CSR_BT_HF_SUPPORT_CODEC_NEGOTIATION &&
                    linkPtr->supportedFeatures & CSR_BT_HFG_SUPPORT_CODEC_NEGOTIATION)
                {
                    instData->index = i;
                    sendCodecSupport(instData);
                }
            }
        }
    }
    if (instData->appHandle != CSR_SCHED_QID_INVALID)
    {
        HfSendUpdateOptionalCodecCfm(instData, result);
    }
}
#endif

#ifdef CSR_BT_HF_ENABLE_SWB_SUPPORT
void CsrBtHfXStateHfUpdateQceSupportReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfUpdateQceSupportReq *prim = (CsrBtHfUpdateQceSupportReq *) instData->recvMsgP;

    if (prim->enable)
    {
        instData->hfQceCodecMask = (instData->hfQceCodecMask == CSR_BT_HF_QCE_UNSUPPORTED) ? 0 : instData->hfQceCodecMask;
        instData->hfQceCodecMask |= prim->codecMask;
    }
    else if (instData->hfQceCodecMask != CSR_BT_HF_QCE_UNSUPPORTED)
    {
        instData->hfQceCodecMask ^= prim->codecMask;

        if (!(instData->hfQceCodecMask & CSR_BT_HF_QCE_UNSUPPORTED))
        {
            instData->hfQceCodecMask = CSR_BT_HF_QCE_UNSUPPORTED;
        }
    }
}
#endif /* CSR_BT_HF_ENABLE_SWB_SUPPORT */

/*************************************************************************************
    startCopsSequence
************************************************************************************/
void CsrBtHfXStateHfCopsReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfGetCurrentOperatorSelectionReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfGetCurrentOperatorSelectionReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if (instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF)
        {
            OpNotAllowed = FALSE;
            if (prim->forceResendingFormat)
            {
                /*instData->linkData[instData->index].atSequenceState = copsQuery;*/
                CsrBtHfAtCopsSetCommandSend(instData,prim->mode,prim->format);
                CsrBtHfAtCopsQuerySend(instData);
            }
            else
            {
                instData->linkData[instData->index].atSequenceState = rest;
                CsrBtHfAtCopsQuerySend(instData);
            }
        }
    }


    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfGetCurrentOperatorSelectionCfm    *primCfm;

        primCfm = (CsrBtHfGetCurrentOperatorSelectionCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetCurrentOperatorSelectionCfm));
        primCfm->type = CSR_BT_HF_GET_CURRENT_OPERATOR_SELECTION_CFM;
        primCfm->connectionId = prim->connectionId;

        primCfm->copsString = NULL;
        primCfm->cmeeResultCode = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);
    }
}

/*****************************************************************************************
    Retrieve all the status indicators from the remote device
******************************************************************************************/
void CsrBtHfXStateHfGetAllStatusReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfGetAllStatusIndicatorsReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfGetAllStatusIndicatorsReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if (instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF)
        {
            OpNotAllowed = FALSE;
            sendCindStatus(instData);
        }
    }


    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfGetAllStatusIndicatorsCfm    *primCfm;

        primCfm = (CsrBtHfGetAllStatusIndicatorsCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetAllStatusIndicatorsCfm));
        primCfm->type = CSR_BT_HF_GET_ALL_STATUS_INDICATORS_CFM;
        primCfm->connectionId = prim->connectionId;
        primCfm->indicatorSupported = NULL;
        primCfm->indicatorValue     = NULL;
        primCfm->cmeeResultCode     = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);
    }
}

/*****************************************************************************************
    Enable or disable the call waiting feature the remote device
******************************************************************************************/
void CsrBtHfXStateHfSetCcwaHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetCallWaitingNotificationReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfSetCallWaitingNotificationReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CCWA=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtCcwaSend(instData,prim->enable);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_CALL_WAITING_NOTIFICATION_CFM);
    }
}


/*****************************************************************************************
    Enable or disable the noise reduction and echo cancelation feature the remote device
******************************************************************************************/
void CsrBtHfXStateHfSetNrecHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetEchoAndNoiseReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfSetEchoAndNoiseReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+NREC=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtNrecSend(instData,prim->enable);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_ECHO_AND_NOISE_CFM);
    }
}

/*****************************************************************************************
    Start or stop the voice recognition feature the remote device
******************************************************************************************/
void CsrBtHfXStateHfSetBvraHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetVoiceRecognitionReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfSetVoiceRecognitionReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+BVRA=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtBvraSend(instData, prim->value);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData, CSR_BT_CME_OPERATION_NOT_ALLOWED, CSR_BT_HF_SET_VOICE_RECOGNITION_CFM);
    }
}

/*****************************************************************************************
    Ask the remote device to generate a DTMF tone
******************************************************************************************/
void CsrBtHfXStateDTMFReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfGenerateDtmfReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfGenerateDtmfReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+VTS=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtVtsSend(instData,prim->dtmf);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_GENERATE_DTMF_CFM);
    }
}

/*****************************************************************************************
    Enable or disable the call notification feature the remote device
******************************************************************************************/
void CsrBtHfXStateHfSetClipHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetCallNotificationIndicationReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfSetCallNotificationIndicationReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CLIP=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtClipSend(instData,prim->enable);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_CALL_NOTIFICATION_INDICATION_CFM);
    }
}

/*****************************************************************************************
    Enable or disable the extended error feature in the remote device
******************************************************************************************/
void CsrBtHfXStateHfSetExtErrorReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetExtendedAgErrorResultCodeReq *prim;
    CsrBool OpNotAllowed = TRUE;
    prim = (CsrBtHfSetExtendedAgErrorResultCodeReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CMEE=X */
            OpNotAllowed = FALSE;
            CsrBtHfAtCmeeSetCommandSend(instData,prim->enable);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_EXTENDED_AG_ERROR_RESULT_CODE_CFM);
    }
}

/*****************************************************************************************
    get the current status of the indicators in the AG (CMER=3,0,?)
******************************************************************************************/
void CsrBtHfActivatedSetStatusIndUpdateReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetStatusIndicatorUpdateReq *prim;
    CsrBool OpNotAllowed = TRUE;
    prim = (CsrBtHfSetStatusIndicatorUpdateReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CMER=3,0,enable */
            OpNotAllowed = FALSE;
            sendSetCmer(instData,prim->enable);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_STATUS_INDICATOR_UPDATE_CFM);
    }
}

/*****************************************************************************************
    request input data from the AG (BINP)
******************************************************************************************/
void CsrBtHfActivatedBtInputReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfBtInputReq *prim;
    CsrBool OpNotAllowed = TRUE;
    prim = (CsrBtHfBtInputReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+BINP */
            OpNotAllowed = FALSE;
            CsrBtHfAtBinpSend(instData,prim->dataRequest);
        }
    }

    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfBtInputCfm   *primCfm;

        primCfm = (CsrBtHfBtInputCfm *)CsrPmemAlloc(sizeof(CsrBtHfBtInputCfm));
        primCfm->type = CSR_BT_HF_BT_INPUT_CFM;
        primCfm->connectionId = prim->connectionId;
        primCfm->dataRespString = NULL;
        primCfm->cmeeResultCode = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);
    }
}


/*****************************************************************************************
    request to dial out (ATD)
******************************************************************************************/
void CsrBtHfActivatedDialRequest(HfMainInstanceData_t *instData)
{
    CsrBtHfDialReq  *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfDialReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send ATD */
            OpNotAllowed = FALSE;
            CsrBtHfAtDialSend(instData,prim->command,prim->number);
        }
    }

    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfDialCfm   *primCfm;

        primCfm = (CsrBtHfDialCfm *)CsrPmemAlloc(sizeof(CsrBtHfDialCfm));
        primCfm->type = CSR_BT_HF_DIAL_CFM;
        primCfm->connectionId = prim->connectionId;
        primCfm->cmeeResultCode = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);

        CsrPmemFree(prim->number);
    }

}

/*****************************************************************************************
    get the current call list (CLCC)
******************************************************************************************/
void CsrBtHfXStateHfCallListReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfGetCurrentCallListReq *prim;
    CsrBool OpNotAllowed = TRUE;
    prim = (CsrBtHfGetCurrentCallListReq *) instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CLCC */
            OpNotAllowed = FALSE;
            CsrBtHfAtClccSend(instData);
        }
    }

    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfGetCurrentCallListCfm   *primCfm;

        primCfm = (CsrBtHfGetCurrentCallListCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetCurrentCallListCfm));
        primCfm->type = CSR_BT_HF_GET_CURRENT_CALL_LIST_CFM;
        primCfm->connectionId = prim->connectionId;
        primCfm->cmeeResultCode = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);
    }
}

/*****************************************************************************************
    get the subscriber number (CNUM)
******************************************************************************************/
void CsrBtHfXStateHfSubscriberReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfGetSubscriberNumberInformationReq *prim;
    CsrBool OpNotAllowed = TRUE;
    prim = (CsrBtHfGetSubscriberNumberInformationReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {/* Send AT+CNUM */
            OpNotAllowed = FALSE;
            CsrBtHfAtCnumSend(instData);
        }
    }

    if (OpNotAllowed)
    {/* Not possible according to HSP spec */
        CsrBtHfGetSubscriberNumberInformationCfm   *primCfm;

        primCfm = (CsrBtHfGetSubscriberNumberInformationCfm *)CsrPmemAlloc(sizeof(CsrBtHfGetSubscriberNumberInformationCfm));
        primCfm->type = CSR_BT_HF_GET_SUBSCRIBER_NUMBER_INFORMATION_CFM;
        primCfm->connectionId = prim->connectionId;
        primCfm->cmeeResultCode = CSR_BT_CME_OPERATION_NOT_ALLOWED;

        CsrBtHfMessagePut(instData->appHandle, primCfm);
    }
}

/**************************************************************************************************
   Current Implementation deactivates HF/HS instances in a sequence.
   Depending on link state it follows below steps:
   1. SCO Disconnect/SCO Cancel Accept 
   2. ACL Disconnect/Cancel Connect Accept 
   3. SDS Un-register 
   
   This continues till all HF/HS instances are deactivated.
 **************************************************************************************************/

void CsrBtHfActivatedStateHfDeactivateReqHandler(HfMainInstanceData_t * instData)
{
      CsrBtHfHsDeactivateHandler(instData);
}

void CsrBtHfActivatedStateHfCancelReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfCancelConnectReq * prim;
    prim = (CsrBtHfCancelConnectReq *) instData->recvMsgP;
    /* First check whether there is a pending connection to that bd_address....*/
    if (CsrBtHfSetCurrentConnIndexFromBdAddr(instData, prim->deviceAddr))
    {/* The function above has found the proper instData->index...
       at this point we might not know what type of connection this is */
        HfInstanceData_t *linkPtr;
        linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);

        if ((linkPtr->state == Connect_s) || (linkPtr->state == ServiceSearch_s))
        {
            CsrBtHfpHandler(instData);
        }
    }
    /* if no pending connection is found, just ignore this request; probably it is due to a
       cross situation bwteen a connection just established and the application trying to
       cancel the operation. The application shall disconnect if it wants to, when the
       it receives the crossing service connect indication */
}

/* With current code logic disconnection request must be received for a valid connectionId.
   It need to be extended to disconnect all connections when coonection Id is set to CSR_BT_HF_CONNECTION_ALL */
void CsrBtHfActivatedStateHfDisconnectReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfDisconnectReq * prim;
    prim = (CsrBtHfDisconnectReq *) instData->recvMsgP;

    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId))
    {
        CsrBtHfpHandler(instData);
    }

}

void CsrBtHfActivatedStateHfServiceConnectReq(HfMainInstanceData_t * instData)
{
    CsrUint8 nrActiveHf = 0;
    CsrUint8 nrActiveHs = 0;
    CsrBtHfServiceConnectReq * prim;
    prim = (CsrBtHfServiceConnectReq *) instData->recvMsgP;

    if (instData->maxTotalSimultaneousConnections > CsrBtHfGetNumberOfRecordsInUse(instData,&nrActiveHf,&nrActiveHs))
    {
        CsrUint8    serviceCount;
        CsrIntFast8    i = 0;
        HfInstanceData_t *linkPtr;

        serviceCount = 0;

        if ((prim->connectionType == CSR_BT_HF_CONNECTION_HF) || (prim->connectionType == CSR_BT_HF_CONNECTION_UNKNOWN))
        {
            for (i=0; i < instData->maxHFConnections; i++)
            {
                linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
                if (linkPtr->state == Activate_s)
                {
                    serviceCount ++;
                }
                else if (CsrBtBdAddrEq(&linkPtr->currentDeviceAddress, &prim->deviceAddr))
                {
                    /* An instance already exists, means the connection is either already present or in progress.
                     * The attempt to establish local connection needs to be rejected.
                     */
                    serviceCount = 0;
                    break;
                }
            }
        }

        if  ((prim->connectionType == CSR_BT_HF_CONNECTION_HS) || (prim->connectionType == CSR_BT_HF_CONNECTION_UNKNOWN))
        {
            for (i=instData->maxHFConnections; i < (instData->maxHSConnections + instData->maxHFConnections); i++)
            {
                linkPtr = (HfInstanceData_t *)&(instData->linkData[i]);
                if (linkPtr->state == Activate_s)
                {
                    serviceCount ++;
                }
            }
        }
        instData->currentDeviceAddress = prim->deviceAddr;
        if (serviceCount != 0)
        {
            startSdcFeatureSearch(instData, TRUE);
        }
        else
        {
            CsrBtHfSendHfFailedServiceConnectCfm(instData, prim->deviceAddr, CSR_BT_RESULT_CODE_HF_REJECTED_BY_PROFILE, CSR_BT_SUPPLIER_HF);
        }
    }
    else
    {
        CsrBtHfSendHfFailedServiceConnectCfm(instData, prim->deviceAddr, CSR_BT_RESULT_CODE_HF_MAX_NUM_OF_CONNECTIONS, CSR_BT_SUPPLIER_HF);
    }
}

void CsrBtHfActivatedStateAudioDisconnectReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfAudioDisconnectReq * prim;
    HfInstanceData_t *linkPtr;
    CsrBtHfConnectionType conType;

    prim = (CsrBtHfAudioDisconnectReq *) instData->recvMsgP;

    if (!CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId))
    {
        return;
    }
    linkPtr  = (HfInstanceData_t *) &(instData->linkData[instData->index]);
    conType = linkPtr->linkType;

    switch (conType)
    {
        case CSR_BT_HF_CONNECTION_HF:
            {
                linkPtr->lastAudioReq  = CSR_BT_HF_AUDIO_OFF;

                if (linkPtr->audioPending == FALSE)
                {
                    /* Check whether audio connection already exists */
                    if (linkPtr->scoHandle != HF_SCO_UNUSED)
                    { /* Just disconnect Audio; no need to get out of LP mode.*/
                        CsrBtCmScoDisconnectReqSend(CSR_BT_HF_IFACEQUEUE, linkPtr->hfConnId);
                        linkPtr->audioPending = TRUE;
                    }
                    else
                    {/* If no audio connection exists, just answer back */
                        CsrBtHfSendHfAudioDisconnectCfm(instData, linkPtr->scoHandle,
                                                        CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED,
                                                        CSR_BT_SUPPLIER_HF);
                    }
                }
                break;
            }
        case CSR_BT_HF_CONNECTION_HS:
            {
                linkPtr->lastAudioReq = CSR_BT_HF_AUDIO_OFF;

                if (linkPtr->audioPending == FALSE)
                {
                    if (linkPtr->state == Connected_s)
                    {
                        if (((linkPtr->scoHandle != HF_SCO_UNUSED)))
                        { /* Send Button pressed signal to indicate we want to open or close audio,
                           * and wait for AG to connect or remove SCO */
                            sendCkpd(instData);
                            linkPtr->audioPending = TRUE;
                        }
                        else
                        {/* If no audio connection exists, just answer back */
                            CsrBtHfSendHfAudioDisconnectCfm(instData, linkPtr->scoHandle,
                                                            CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED,
                                                            CSR_BT_SUPPLIER_HF);
                        }
                    }
                }
                break;
            }
        default:
            {
                break;
            }
    }
}

void CsrBtHfActivatedStateAudioReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfAudioConnectReq * prim;
    HfInstanceData_t *linkPtr;
    CsrBtHfConnectionType conType;

    prim = (CsrBtHfAudioConnectReq *) instData->recvMsgP;
    if (!CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId))
    {
        return;
    }
    linkPtr = (HfInstanceData_t *) &(instData->linkData[instData->index]);
    conType = linkPtr->linkType;


    if (linkPtr->audioAcceptPending)
    {/* Reject: there is an incoming audio ongoing: do not allow outgoing to the same device! */
        CsrBtHfSendHfAudioConnectInd(instData, 0,0,0,0,0,0,0,
                                     CSR_BT_RESULT_CODE_HF_SYNCHRONOUS_CONNECTION_ALREADY_CONNECTING,
                                     0xDEAD,
                                     CSR_BT_SUPPLIER_HF,
                                     CSR_BT_HF_AUDIO_CONNECT_IND);
    }
    else
    {
        switch (conType)
        {
            case CSR_BT_HF_CONNECTION_HF:
                {
                    CsrBtHfpHandler(instData);
                    break;
                }
            case CSR_BT_HF_CONNECTION_HS:
                {
                    linkPtr->pcmSlot = prim->pcmSlot;
                    linkPtr->pcmReassign = prim->pcmRealloc;
                    linkPtr->pcmMappingReceived = TRUE;

                    if (linkPtr->state == Connected_s)
                    {
                        linkPtr->lastAudioReq = CSR_BT_HF_AUDIO_ON;
                        if (!linkPtr->audioPending)
                        {
                            if (linkPtr->scoHandle == HF_SCO_UNUSED)
                            { /* Send Button pressed signal to indicate we want to open or close audio,
                               * and wait for AG to connect or remove SCO */
                                sendCkpd(instData);
                                linkPtr->audioPending = TRUE;
                            }
                        }
                    }
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
}

void CsrBtHfActivatedStateMapScoPcmResHandler(HfMainInstanceData_t * instData)
{
    HfInstanceData_t *linkPtr;
    CsrBtHfAudioAcceptConnectRes *prim;

    prim = (CsrBtHfAudioAcceptConnectRes *) instData->recvMsgP;
    if (!CsrBtHfSetCurrentConnIndexFromConnId(instData, prim->connectionId))
    {
        return;
    }

    linkPtr = (HfInstanceData_t *)&(instData->linkData[instData->index]);
    if (linkPtr->state == Connected_s)
    {
        CsrBtCmScoCommonParms *scoParms = (CsrBtCmScoCommonParms *)CsrPmemAlloc(sizeof(CsrBtCmScoCommonParms));

        linkPtr->pcmSlot            = prim->pcmSlot;
        linkPtr->pcmReassign        = prim->pcmReassign;

        if (prim->acceptResponse == HCI_SUCCESS)
        {
            linkPtr->pcmMappingReceived = TRUE;
        }
        linkPtr->audioAcceptPending = FALSE;

        if (prim->acceptParameters)
        { /* Use Application provided audio accept parameters */
            scoParms->audioQuality  = prim->acceptParameters->packetTypes;
            scoParms->txBandwidth   = prim->acceptParameters->txBandwidth;
            scoParms->rxBandwidth   = prim->acceptParameters->rxBandwidth;
            scoParms->maxLatency    = prim->acceptParameters->maxLatency;
            scoParms->voiceSettings = prim->acceptParameters->contentFormat;
            scoParms->reTxEffort    = prim->acceptParameters->reTxEffort;
            CsrPmemFree(prim->acceptParameters);
        }
        else if ((instData->mainConfig & CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE) == CSR_BT_HF_AT_MODE_TRANSPARENT_ENABLE)
        {
            CsrPmemFree(scoParms);
            scoParms = NULL;
        }
        else
        {
            CsrBtHfSetIncomingScoAudioParams(linkPtr, scoParms);
        }
        CsrBtCmMapScoPcmResSend(linkPtr->hfConnId,
                                   prim->acceptResponse,
                                   scoParms,
                                   linkPtr->pcmSlot,
                                   prim->pcmReassign);
    }
}

void CsrBtHfMainXStateMapScoPcmIndHandler(HfMainInstanceData_t * instData)
{
    CsrBtCmMapScoPcmInd *cmPrim;

    cmPrim = (CsrBtCmMapScoPcmInd *)instData->recvMsgP;
    if (CsrBtHfSetCurrentConnIndexFromConnId(instData, cmPrim->btConnId))
    {
        CsrBtHfpHandler(instData);
    }
    else
    {
        CsrBtCmMapScoPcmResSend(cmPrim->btConnId,
                                HCI_ERROR_REJ_BY_REMOTE_PERS,
                                NULL,
                                CSR_BT_PCM_DONT_CARE,
                                FALSE);
    }
}

void CsrBtHfActivatedStateAnswerReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfpHandler(instData);
}

void CsrBtHfActivatedStateRejectReqHandler(HfMainInstanceData_t * instData)
{
    /* Only for HF */
    CsrBtHfpHandler(instData);
}

void CsrBtHfActivatedStateSpeakerGainStatusReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfpHandler(instData);
}

void CsrBtHfActivatedStateMicGainStatusReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfpHandler(instData);
}

void CsrBtHfActivatedStateAtCmdReqHandler(HfMainInstanceData_t * instData)
{
    CsrBtHfpHandler(instData);
}

void CsrBtHfActivatedStateChldReqHandler(HfMainInstanceData_t * instData)
{
    /* This signal is only for the HF */
    CsrBtHfpHandler(instData);
}

/* The API CsrBtHfSetDeregisterTimeReqSend() will no longer be needed.
 * It will be marked for deprecation. With multipoint HF feature,
 * the HF module will always keep the service records as long as the services are active.
 * Hence CsrBtHfDeregisterTimeReq message is not copied for timed SDS deregister.
 * For backward compatibility reasons we are handling this message and responding
 * with success.
 */
void CsrBtHfXStateSetDeregisterTimeReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfDeregisterTimeCfm *hfPrim;

    /* Send confirmation message */
    hfPrim = (CsrBtHfDeregisterTimeCfm *)CsrPmemAlloc(sizeof(CsrBtHfDeregisterTimeCfm));
    hfPrim->type = CSR_BT_HF_DEREGISTER_TIME_CFM;
    hfPrim->result = CSR_BT_CME_SUCCESS;

    CsrBtHfMessagePut(instData->appHandle, hfPrim );
}

void CsrBtHfMainIgnoreMessage(HfMainInstanceData_t *instData)
{
     CSR_UNUSED(instData);
    /* Just ignore; don't raise exception */
}

void CsrBtHfXStateIndicatorActivationReqHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfIndicatorActivationReq *prim = (CsrBtHfIndicatorActivationReq *)instData->recvMsgP;
    CsrBtHfConnectionType conType = instData->linkData[instData->index].linkType;

    if ((conType != CSR_BT_HF_CONNECTION_HF) ||
        (instData->linkData[instData->index].state != Connected_s) ||
        (instData->linkData[instData->index].agIndicators.cindData.instCount == 0))
    {/* The AT+BIA command shall only be issued on HFP connections; it is meant to be valid only for the SLC established and
        shall not be 'remembered' from connection to connection!
        Send negative confirmation back */
        CsrBtHfIndicatorActivationCfm *hfPrim = (CsrBtHfIndicatorActivationCfm *)CsrPmemAlloc(sizeof(CsrBtHfIndicatorActivationCfm));
        hfPrim->type         = CSR_BT_HF_INDICATOR_ACTIVATION_CFM;
        hfPrim->connectionId = prim->connectionId;
        if (conType == CSR_BT_HF_CONNECTION_HF)
        {/* Correct type link but not in the proper state */
            hfPrim->result       = CSR_BT_CME_NO_CONNECTION_TO_PHONE;
        }
        else
        {/* Wrong type of link: either not existent or HS connection */
            hfPrim->result       = CSR_BT_CME_OPERATION_NOT_ALLOWED;
        }

        CsrBtHfMessagePut(instData->appHandle, hfPrim );
    }
    else
    {/* Get the information, format the AT+BIA command and send it */
        sendBia(instData);
    }

}

/*****************************************************************************************
    Send enabled HF Indicator value update.
******************************************************************************************/
void CsrBtHfXStateHfSetHfIndicatorValueHandler(HfMainInstanceData_t *instData)
{
    CsrBtHfSetHfIndicatorValueReq *prim;
    CsrBool OpNotAllowed = TRUE;

    prim = (CsrBtHfSetHfIndicatorValueReq *)instData->recvMsgP;

    if (instData->state == Activated_s)
    {
        if ((instData->linkData[instData->index].linkType == CSR_BT_HF_CONNECTION_HF) &&
            (instData->linkData[instData->index].state == Connected_s))
        {
            OpNotAllowed = FALSE;
            CsrBtHfSendUpdatedHfIndValue(instData, prim->indId, prim->value);
        }
    }

    if (OpNotAllowed)
    {
        CsrBtHfSendHfGeneralCfmMsg(instData,CSR_BT_CME_OPERATION_NOT_ALLOWED,CSR_BT_HF_SET_HF_INDICATOR_VALUE_CFM);
    }
}

