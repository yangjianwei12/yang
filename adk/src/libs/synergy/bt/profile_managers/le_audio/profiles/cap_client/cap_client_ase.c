/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "cap_client_ase.h"
#include "cap_client_util.h"
#include "cap_client_discover_audio_capabilities_req.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define CAP_MAX_SUPPORTED_SINK_ASES    CAP_CLIENT_MAX_SUPPORTED_ASES
#define CAP_MAX_SUPPORTED_SOURCE_ASES  CAP_CLIENT_MAX_SUPPORTED_ASES

#define VALIDATE_ASE_STATE(STATE, ASE_STATE)     (STATE == ASE_STATE)

CapClientBool capClientGetbapAseFromAseId(CsrCmnListElm_t* elem, void *value)
{
    uint8 id = *(uint8*)value;
    BapAseElement* ase = (BapAseElement *)elem;
    return (ase->aseId == id) ? TRUE : FALSE;
}

CapClientBool capClientGetbapAseFromCisHandle(CsrCmnListElm_t* elem, void *value)
{
    uint16 cisHandle = *(uint16*)value;
    BapAseElement* ase = (BapAseElement *)elem;
    return (ase->cisHandle == cisHandle) ? TRUE : FALSE;
}

CapClientBool capClientGetbapAseFromCisId(CsrCmnListElm_t* elem, void* value)
{
    uint8 cisId = *(uint8*)value;
    BapAseElement* ase = (BapAseElement*)elem;
    return (ase->cisId == cisId) ? TRUE : FALSE;
}

CapClientBool capClientIsCisHandleUnique(uint16* cisHandles,
                                         uint8 count, 
                                         uint16 cisHandle)
{
    uint8 i;

    if (cisHandle == 0 || cisHandles == NULL)
        return FALSE;

    for (i = 0; i < count; i++)
        if (cisHandles[i] == cisHandle)
            return FALSE;

    return TRUE;
}

uint8* capClientGetCodecConfigurableAses(uint8 direction,
                                   uint8 aseCount,
                                   uint8 state,
                                   BapInstElement* bap,
                                   CapClientContext useCase)
{
    uint8 index;
    uint8* ase = NULL;
    BapAseElement* aseElem = NULL;
    bool isSink = (direction == BAP_ASE_SINK);

    aseElem = isSink ? ((BapAseElement*)(bap->sinkAseList).first) :
                                  ((BapAseElement*)(bap->sourceAseList).first);

    if (aseCount <= 0)
    {
        return ase;
    }

    ase = (uint8*)CsrPmemZalloc((aseCount) * sizeof(uint8));

    for (index = 0; index < aseCount && aseElem; )
    {
        if (aseElem->state <= state)
        {
            if ((aseElem->useCase == CAP_CLIENT_CONTEXT_TYPE_PROHIBITED) 
                      || (useCase == aseElem->useCase))
            {
                ase[index] = aseElem->aseId;
                index++;
            }
        }

        aseElem = aseElem->next;
    }

    /* If available ASE's are fewer than required aseCount then return NULL*/

    if (index < aseCount)
    {
        CsrPmemFree(ase);
        ase = NULL;
    }

    return ase;
}


uint8 *capClientGetAseIdForGivenCidAndState(uint32 cid,
                                      uint8 direction,
                                      uint8 aseCount,
                                      uint8 state,
                                      BapInstElement *bap)
{
    CSR_UNUSED(cid);
    uint8 index;
    uint8* ase = NULL;
    BapAseElement *aseElem = NULL;
    bool isSink = (direction == BAP_ASE_SINK);

    aseElem = isSink?((BapAseElement*)(bap->sinkAseList).first) :
                                    ((BapAseElement*)(bap->sourceAseList).first);

    ase = (uint8*)CsrPmemZalloc((aseCount)*sizeof(uint8));

    for(index = 0; index < aseCount && aseElem;  )
    {
        if (VALIDATE_ASE_STATE(state, aseElem->state))
        {
            ase[index] = aseElem->aseId;
            index++;
        }

        aseElem = aseElem->next;
    }
    return ase;
}

uint8 capClientGetCisCount(BapInstElement *bap)
{
    uint8 count = 0;
    CsrCmnListElm_t *elem = NULL;
    uint16 prevCisId = 0;

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientGetCisCount: Encountered Null instance \n ");
        return count;
    }

    elem = (CsrCmnListElm_t*)(bap->sinkAseList.first);

    if(elem == NULL)
        elem = (CsrCmnListElm_t*)(bap->sourceAseList.first);

    for(;elem; elem = elem->next)
    {
        BapAseElement *aseElem = (BapAseElement*)elem;
        if((aseElem->state >= BAP_ASE_STATE_QOS_CONFIGURED)
               && (aseElem->cisId != 0)
               && (aseElem->cisId != prevCisId))
        {
            count++;
        }
        prevCisId = aseElem->cisId;
    }

    return count;
}

uint8 capClientGetAseCountInUse(BapInstElement *bap)
{
    uint8 count;
    CsrCmnListElm_t *elem = NULL;
    BapAseElement *aseElem = NULL;
    count = 0;

    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientGetAseCountInUse: Encountered Null instance \n ");
        return count;
    }

    elem = (CsrCmnListElm_t*)(bap->sinkAseList.first);

    while(elem)
    {
        aseElem = (BapAseElement*)elem;

        if (aseElem->inUse)
            count++;

        elem = elem->next;
    }

    elem = (CsrCmnListElm_t*)(bap->sourceAseList.first);

    while(elem)
    {
        aseElem = (BapAseElement*)elem;

        if (aseElem->inUse)
            count++;

        elem = elem->next;
    }

    return count;
}

uint8 *capClientGetAseIdsInUse(BapInstElement *bap, uint8 aseCount,CapClientContext useCase)
{
    uint8 index;
    uint8* ase = NULL;
    BapAseElement *aseElem = NULL;

    ase = (uint8*)CsrPmemZalloc((aseCount)*sizeof(uint8));

    aseElem = (BapAseElement*)(bap->sinkAseList).first;

    for(index = 0; index < aseCount && aseElem; aseElem = aseElem->next)
    {
        if(aseElem->inUse && aseElem->useCase == useCase)
        {
            ase[index] = aseElem->aseId;
            index++;
        }
    }

    aseElem = (BapAseElement*)(bap->sourceAseList).first;

    for(; index < aseCount && aseElem; aseElem = aseElem->next)
    {
        if(aseElem->inUse && aseElem->useCase == useCase)
        {
            ase[index] = aseElem->aseId;
            index++;
        }
    }
    return ase;
}


uint8 capClientGetAseCountInState(uint8 state,
                           BapInstElement *bap,
                           uint8 direction)
{
    uint8 count, i, tempCount;
    CsrCmnListElm_t *elem = NULL;
    BapAseElement *aseElem = NULL;
    count = 0;


    if(bap == NULL)
    {
        CAP_CLIENT_ERROR("\n capClientGetAseCountInState: Encountered Null instance \n ");
        return count;
    }

    if (direction & BAP_ASE_SINK)
    {
        tempCount = bap->sinkAseCount;
        elem = (CsrCmnListElm_t*)(bap->sinkAseList.first);

        for (i = 0; i < tempCount && elem; elem = elem->next)
        {
            aseElem = (BapAseElement*)elem;

            if (VALIDATE_ASE_STATE(state,aseElem->state))
            {
                count++;
            }
        }
    }

    if (direction & BAP_ASE_SOURCE)
    {

        tempCount = bap->sourceAseCount;
        elem = (CsrCmnListElm_t*)(bap->sourceAseList.first);

        for (i = 0; i < tempCount && elem; elem = elem->next)
        {
            aseElem = (BapAseElement*)elem;

            if (VALIDATE_ASE_STATE(state, aseElem->state))
            {
                count++;
            }
        }
    }

    return count;
}

void capClientFlushAseIdFromBapInstance(BapInstElement* bap, uint8 aseType)
{
    CsrCmnList_t* list = NULL;
    CsrCmnListElm_t* elem = NULL;
    CsrCmnListElm_t* temp = NULL;
    bool isSink = (aseType == BAP_ASE_SINK);
    uint8 count = 0;

    uint8 i = 0;
    
    if (bap)
    {
        if (isSink)
        {
            list = &(bap->sinkAseList);
            bap->sinkAseCount = 0;
        }
        else
        {
            list = &(bap->sourceAseList);
            bap->sourceAseCount = 0;
        }
        
        elem = (CsrCmnListElm_t*)list->first;

        /* Before discovery the count will be zero, hence flushing doesnot happen */
        count = list->count;

        for (i = 0; i < count  && elem; i++)
        {
            temp = elem->next;
            CAP_CLIENT_BAP_REMOVE_ASE_ELEM(list, elem);
            elem = temp;
        }
    }
}


uint16 *capClientGetCisHandlesForContext(CapClientContext useCase,
                                  CapClientGroupInstance *cap,
                                  uint8 cisCount)
{

    BapAseElement *aseElem = NULL;
    uint16 *cisHandles;
    uint8 index = 0;
    cisHandles = (uint16*)CsrPmemZalloc((cisCount)*sizeof(uint16));

    BapInstElement* bap = (BapInstElement*)cap->bapList.first;

    while (bap)
    {
        if (CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection)
                || CAP_CLIENT_CIS_IS_UNI_SINK(cap->cigDirection))
        {
            aseElem = (BapAseElement*)(bap->sinkAseList).first;

            for (; index < cisCount && aseElem; aseElem = aseElem->next)
            {
                if (aseElem->inUse && aseElem->useCase == useCase)
                {
                    cisHandles[index] = aseElem->cisHandle;
                    index++;
                }
            }
        }

        if ((CAP_CLIENT_CIS_IS_BIDIRECTIONAL(cap->cigDirection) && cap->numOfSourceAses)
                                           || CAP_CLIENT_CIS_IS_UNI_SRC(cap->cigDirection))
        {
            aseElem = (BapAseElement*)(bap->sourceAseList).first;

            for (; index < cisCount && aseElem; aseElem = aseElem->next)
            {
                if (aseElem->inUse &&
                       aseElem->useCase == useCase
                          && capClientIsCisHandleUnique(cisHandles, index, aseElem->cisHandle))
                {
                    cisHandles[index] = aseElem->cisHandle;
                    index++;
                }
            }
        }

        if (cisCount > index)
            bap = bap->next;
        else
            break;
    }

    return cisHandles;
}

void capClientHandleReadAseInfoCfm(CAP_INST *inst,
          BapUnicastClientReadAseInfoCfm* cfm,
          CapClientGroupInstance *gInst)
{
    uint8 index;
    bool isSink = (cfm->aseType == BAP_ASE_SINK);
    CsrCmnList_t *list;
    BapInstElement *bap = (BapInstElement*)
                CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(gInst->bapList, cfm->handle);

    inst->bapRequestCount--;

    if (cfm->aseType == BAP_ASE_SOURCE && CAP_CLIENT_GET_BAP_STATE_PER_CIG_ID(bap, CAP_CLIENT_NO_CIG_ID_MASK) == CAP_CLIENT_BAP_STATE_IDLE)
    {
        /* Update the bap state here as ase read for source is the last operation of the discovery procedure */
        setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_DISCOVER_COMPLETE, CAP_CLIENT_BAP_STATE_INVALID);
        CAP_CLIENT_INFO("\n(CAP): capClientHandleReadAseInfoCfm:bap->bapCurrentState : %x \n", bap->bapCurrentState);
    }

    if(cfm->result != BAP_RESULT_SUCCESS)
    {
        /* LOG: Read unsuccessful?*/
        CAP_CLIENT_INFO("\n(CAP) :capClientHandleReadAseInfoCfm: Read ASE Info Unsuccessful with Result: 0x%x \n", cfm->result);

    }

    list = isSink ? (&bap->sinkAseList) : (&bap->sourceAseList);

    if (isSink)
    {
        CAP_CLIENT_INFO("\n(CAP) : Sink ASE Discovered \n");
    }
    else
    {
        CAP_CLIENT_INFO("\n(CAP) : Source ASE Discovered \n");
    }

    for(index = 0; index < cfm->numAses;  index++)
    {
        BapAseElement *ase = NULL;
        ase = (BapAseElement*)CAP_CLIENT_BAP_ADD_ASE_ELEM(list);
        isSink ? bap->sinkAseCount++ : bap->sourceAseCount++;

        ase->aseId = cfm->aseIds[index];
        ase->state = cfm->aseState;

        CAP_CLIENT_INFO("(CAP) : ASE ID Found: %d \n", ase->aseId);
    }

    gInst->pendingCap = (gInst->pendingCap & ~CAP_CLIENT_DISCOVER_ASE_STATE);

    CAP_CLIENT_INFO("\n(CAP) :capClientHandleReadAseInfoCfm: bapRequestCount: 0x%x \n", inst->bapRequestCount);

    if(inst->bapRequestCount == 0)
    {
        capClientSendDiscoverCapabilityReq(gInst->pendingCap, bap, isSink, inst, gInst->setSize, gInst);
    }

    /* free memory for aseInfo */
    if (cfm->aseInfoLen)
        CsrPmemFree(cfm->aseInfo);

    /* free memory for ase_id */
    if (cfm->aseIds)
        CsrPmemFree(cfm->aseIds);
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
