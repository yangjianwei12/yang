/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_bt_util.h"
#include "csr_bt_pac_handler.h"
#include "csr_bt_obex_util.h"
#include "obex_handover.h"

extern PacInst csrBtPacInstData[PAC_MAX_NUM_INSTANCES];
CsrBtMarshalUtilInst *pacConverter;

/* Retrieve the list of connected PAC instance IDs and their count by
 * searching the connected instances either through state or address passed.
 */
static CsrUint8 pacGetConnectedInstanceIds(const CsrBtDeviceAddr *addr,
                                           CsrSchedQid          **pInstIdsList)
{
    CsrUint8 instIdx;
    CsrUint8 count = 0;

    /* Traverse the PAC instances to find the count of connected instances */
    for (instIdx = 0; instIdx < PAC_MAX_NUM_INSTANCES; instIdx++)
    {
        PacInst *pInst = &(csrBtPacInstData[instIdx]);

        if (pInst->state == PAC_INSTANCE_STATE_CONNECTED &&
            ((addr && CSR_BT_BD_ADDR_EQ(pInst->deviceAddr, *addr)) || addr == NULL))
        {
            count++;
        }
    }

    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacGetConnectedInstanceIds: Connected Instance Count(%d)", count));

    /* Prepare the list now if there is any connected instance found */
    if (count)
    {
        CsrUint8 connIdx = 0;

        *pInstIdsList = (CsrSchedQid *)CsrPmemZalloc(sizeof(CsrSchedQid) * count);

        for (instIdx = 0; instIdx < PAC_MAX_NUM_INSTANCES; instIdx++)
        {
            PacInst *pInst = &(csrBtPacInstData[instIdx]);

            if (pInst->state == PAC_INSTANCE_STATE_CONNECTED &&
                ((addr && CSR_BT_BD_ADDR_EQ(pInst->deviceAddr, *addr)) || addr == NULL))
            {
                pInstIdsList[0][connIdx++] = pInst->pacInstanceId;
            }
        }
    }

    return (count);
}

/* Retrieve the PAC instance pointer for the instance ID requested */
static PacInst *pacGetInstanceForInstanceId(CsrSchedQid pInstId)
{
    CsrUint8 pInstIdx;
    PacInst *pInst = NULL;

    for (pInstIdx = 0; pInstIdx < PAC_MAX_NUM_INSTANCES; pInstIdx++)
    {
        PacInst *curInst = &(csrBtPacInstData[pInstIdx]);

        if (curInst->pacInstanceId == pInstId)
        {
            /* Found the matching instance, break and return the instance */
            pInst = curInst;
            break;
        }
    }

    return (pInst);
}

static void convPacInstance(CsrBtMarshalUtilInst *conv,
                            PacInst *pInst)
{
    CsrBtMarshalUtilConvertObj(conv, pInst->state);
#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
    CsrBtMarshalUtilConvertObj(conv, pInst->secOutgoing);
#endif
    CsrBtMarshalUtilConvertObj(conv, pInst->supportedFeatures);
    CsrBtMarshalUtilConvertObj(conv, pInst->supportedRepositories);
    CsrBtMarshalUtilConvertObj(conv, pInst->curFolderId);
}

/* Serializes all the PAC instances connected with the given address */
static CsrBool serPacInstanceData(const CsrBtDeviceAddr *deviceAddr, CsrUint8 *buf,
                                  CsrUint16 length, CsrUint16 *written)
{
    CsrSchedQid *pInstIdsList  = NULL;
    CsrUint8     pInstIdsCount = pacGetConnectedInstanceIds(deviceAddr, &pInstIdsList);

    if (pInstIdsCount)
    {
        CsrUint8 pInstIdx;

        if (!pacConverter)
        {
            pacConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(pacConverter, length, buf, TRUE);
        /* Serialize the total num of connected instances count first */
        CsrBtMarshalUtilConvertObj(pacConverter, pInstIdsCount);
        /* Serialize each connected instances one after one */
        for (pInstIdx = 0; pInstIdx < pInstIdsCount; pInstIdx++)
        {
            PacInst *pInst = pacGetInstanceForInstanceId(pInstIdsList[pInstIdx]);

            CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "serPacInstanceData: Serializing InstanceId(%d)", pInst->pacInstanceId));
            /* Serialize the PAC instance id explicitly */
            CsrBtMarshalUtilConvertObj(pacConverter, pInst->pacInstanceId);
            /* Serialize rest of the PAC instance fields */
            convPacInstance(pacConverter, pInst);
        }
        /* Use the same PAC converter to serialize obex instance by passing
         * the list of connected instance IDs and their count */
        ObexHandoverSerInstData(pacConverter, pInstIdsList, pInstIdsCount);
        CsrPmemFree(pInstIdsList);
        *written = length - CsrBtMarshalUtilRemainingLengthGet(pacConverter);

        return (CsrBtMarshalUtilStatus(pacConverter));
    }

    /* Nothing to marshal, just return TRUE */
    return (TRUE);
}

static CsrBool deserPacInstanceData(const CsrBtDeviceAddr *deviceAddr, CsrUint8 *buf,
                                    CsrUint16 length, CsrUint16 *written)
{
    CsrUint8    pInstIdsCount = 0;

    if (!pacConverter)
    {
        pacConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(pacConverter, length, (CsrUint8 *) buf, TRUE);
    /* De-serialize the total num of connected instances count first */
    CsrBtMarshalUtilConvertObj(pacConverter, pInstIdsCount);

    if (pInstIdsCount)
    {
        CsrUint8     pInstIdx;
        CsrSchedQid *pInstIdsList  = NULL;
        CsrBool      continueDeser = TRUE;

        /* De-serialize each connected instances one after one */
        for (pInstIdx = 0; pInstIdx < pInstIdsCount; pInstIdx++)
        {
            PacInst     *pInst   = NULL;
            CsrSchedQid  pInstId = 0;

            /* De-serialize the PAC instance id explicitly */
            CsrBtMarshalUtilConvertObj(pacConverter, pInstId);
            CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "deserPacInstanceData: De-serializing InstanceId(%d)", pInstId));
            /* Find out the corresponding PAC instance */
            pInst = pacGetInstanceForInstanceId(pInstId);

            if (pInst)
            {
                /* Reset old copy of local & remote app header fields if any */
                CsrBtPacResetLocalAppHeaderPar(pInst);
                CsrBtPacResetRemoteAppHeaderPar(pInst);
                /* De-serialize rest of the PAC instance fields */
                convPacInstance(pacConverter, pInst);
                pInst->deviceAddr = *deviceAddr;
            }
            else
            {
                continueDeser = FALSE;
                CSR_LOG_TEXT_ERROR((CsrBtPacLto, 0, "No PAC Instance found !"));
            }
        }

        /* Get the list of all connected instance IDs and count if deserializing to be continued */
        if (continueDeser)
        {
            pInstIdsCount = pacGetConnectedInstanceIds(deviceAddr, &pInstIdsList);

            if (pInstIdsList)
            {
                /* Use the same PAC converter to de-serialize obex instance by passing
                 * the list of connected instance IDs and their count */
                ObexHandoverDeserInstData(pacConverter, pInstIdsList, pInstIdsCount);
                CsrPmemFree(pInstIdsList);
            }
        }
    }

    *written = length - CsrBtMarshalUtilRemainingLengthGet(pacConverter);

    return (CsrBtMarshalUtilStatus(pacConverter));
}

static bool pacVeto(void)
{
    CsrUint8 pInstIdx;
    bool     veto = FALSE;

    for (pInstIdx = 0; pInstIdx < PAC_MAX_NUM_INSTANCES; pInstIdx++)
    {
        PacInst *pInst = &(csrBtPacInstData[pInstIdx]);

        if (pInst->state == PAC_INSTANCE_STATE_CONNECTING)
        {
            veto = TRUE;
            CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacVeto (%d) : state(%d)", veto, pInst->state));
            break;
        }
   }

    if (veto == FALSE)
    {
        CsrSchedQid *pInstIdsList  = NULL;
        CsrUint8     pInstIdsCount = pacGetConnectedInstanceIds(NULL, &pInstIdsList);

        if (pInstIdsList)
        {
            veto = ObexHandoverVeto(pInstIdsList, pInstIdsCount);
            CsrPmemFree(pInstIdsList);
        }
    }

    return (veto);
}

static bool pacMarshal(const tp_bdaddr *vmTpAddrt,
                          CsrUint8 *buf,
                          CsrUint16 length,
                          CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacMarshal"));

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    return (serPacInstanceData(&tpAddrt.addrt.addr, buf, length, written));
}


static bool pacUnmarshal(const tp_bdaddr *vmTpAddrt,
                         const CsrUint8 *buf,
                         CsrUint16 length,
                         CsrUint16 *written)
{
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacUnmarshal:length(%d)", length));

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    return (deserPacInstanceData(&tpAddrt.addrt.addr, (CsrUint8 *)buf, length, written));
}


static void pacHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                   const bool newPrimary)
{
    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacHandoverCommit: newPrimary(%d)", newPrimary));

    if (newPrimary)
    {
        CsrSchedQid  *pInstIdsList  = NULL;
        CsrUint8      pInstIdsCount;
        CsrBtTpdAddrT tpAddrt = { 0 };

        BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

        pInstIdsCount = pacGetConnectedInstanceIds(&tpAddrt.addrt.addr, &pInstIdsList);

        /* Commit the obex instance now */
        if (pInstIdsList)
        {
            ObexHandoverCommit(pInstIdsList, pInstIdsCount,
                               PacDeliverAuthenticateIndCb(), PacDeliverDisconnectIndCb());
            CsrPmemFree(pInstIdsList);
        }
    }
}

static void pacHandoverComplete(const bool newPrimary)
{
    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacHandoverComplete: newPrimary(%d)", newPrimary));

    if (pacConverter)
    {
        CsrBtMarshalUtilDestroy(pacConverter);
        pacConverter = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void pacHandoverAbort(void)
{
    CSR_LOG_TEXT_DEBUG((CsrBtPacLto, 0, "pacHandoverAbort"));

    if (pacConverter && CsrBtMarshalUtilTypeGet(pacConverter) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    { /* Secondary */
        CsrSchedQid  *pInstIdsList  = NULL;
        CsrUint8      pInstIdsCount = pacGetConnectedInstanceIds(NULL, &pInstIdsList);

        if (pInstIdsCount)
        {
            CsrUint8 pInstIdx;

            /* Reset all the un-marshalled PAC instance fields first */
            for (pInstIdx = 0; pInstIdx < PAC_MAX_NUM_INSTANCES; pInstIdx++)
            {
                PacInst *pInst = &csrBtPacInstData[pInstIdx];

                if (pInst->state == PAC_INSTANCE_STATE_CONNECTED)
                {
                    pInst->supportedFeatures     = 0;
                    pInst->supportedRepositories = 0;
                    pInst->curFolderId           = 0;

                    CsrBtBdAddrZero(&pInst->deviceAddr);
                    PAC_INSTANCE_STATE_CHANGE(pInst->state, PAC_INSTANCE_STATE_IDLE);

#ifdef INSTALL_PAC_CUSTOM_SECURITY_SETTINGS
                    CsrBtScSetSecOutLevel(&pInst->secOutgoing,
                                          CSR_BT_SEC_DEFAULT,
                                          CSR_BT_PBAP_MANDATORY_SECURITY_OUTGOING,
                                          CSR_BT_PBAP_DEFAULT_SECURITY_OUTGOING,
                                          CSR_BT_RESULT_CODE_OBEX_SUCCESS,
                                          CSR_BT_RESULT_CODE_OBEX_UNACCEPTABLE_PARAMETER);
#endif
                }
            }

            /* Reset all corresponding obex instances now */
            ObexHandoverAbort(pInstIdsList, pInstIdsCount);
            CsrPmemFree(pInstIdsList);
        }
    }

    pacHandoverComplete(FALSE);
}

const handover_interface pac_handover_if =
        MAKE_BREDR_HANDOVER_IF(&pacVeto,
                               &pacMarshal,
                               &pacUnmarshal,
                               &pacHandoverCommit,
                               &pacHandoverComplete,
                               &pacHandoverAbort);

