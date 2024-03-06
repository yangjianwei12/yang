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
#include "csr_bt_panic.h"
#include "csr_bt_spp_main.h"
#include "csr_bt_spp_sef.h"

#ifdef CSR_LOG_ENABLE
#define CSR_BT_SPP_LTSO_HANDOVER              0
#define CSR_BT_SPP_HANDOVER_LOG_INFO(...)     CSR_LOG_TEXT_INFO((CsrBtSppLto, CSR_BT_SPP_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_SPP_HANDOVER_LOG_WARNING(...)  CSR_LOG_TEXT_WARNING((CsrBtSppLto, CSR_BT_SPP_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_SPP_HANDOVER_LOG_ERROR(...)    CSR_LOG_TEXT_ERROR((CsrBtSppLto, CSR_BT_SPP_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_SPP_HANDOVER_LOG_INFO(...)
#define CSR_BT_SPP_HANDOVER_LOG_WARNING(...)
#define CSR_BT_SPP_HANDOVER_LOG_ERROR(...)
#endif

static CsrBtMarshalUtilInst *sppConverter;

static CsrUint8 sppGetConnectedConnectionCountForBdAddr(const CsrBtDeviceAddr deviceAddr)
{
    CsrUint8 i, count = 0;
    for (i = 0; i < sppInstanceData.numberOfSppInstances; i++)
    {
        SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];
        if (instData)
        {
            if (CSR_BT_BD_ADDR_EQ(instData->bdAddr, deviceAddr))
            {
                if (instData->state == Connected_s)
                {
                    count++;
                }
            }
        }
    }
    return count;
}

static void serSppInstData(CsrBtMarshalUtilInst *conv,
                           SppInstanceData_t *sppInst)
{
    CsrBtMarshalUtilConvertObj(conv, sppInst->myAppHandle);
    CsrBtMarshalUtilConvertObj(conv, sppInst->sppConnId);
}

static void deserSppInstData(CsrBtMarshalUtilInst *conv,
                             SppInstanceData_t *sppInst)
{
    CsrBtMarshalUtilConvertObj(conv, sppInst->sppConnId);
}

static bool sppVeto(void)
{
    CsrUint8 i;
    bool veto = FALSE;

    for (i = 0; i < sppInstanceData.numberOfSppInstances && veto != TRUE; i++)
    {
        SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];

        if (instData)
        {
            if (instData->state    != Connected_s &&
                instData->state    != Activated_s &&
                instData->state    != Idle_s &&
                instData->subState != SPP_SUB_IDLE_STATE)
            {
                 veto = TRUE;
            }
            else if (instData->saveQueue ||
                     SynergySchedMessagesPendingForTask(instData->myAppHandle, NULL) != 0)
            {
                /* If there are pending messages in either the synergy queue or savequeue of SPP, veto handover. */
                veto = TRUE;
            }
            else if (instData->sdsUnregInProgress == TRUE)
            {
                veto = TRUE;
            }
        }
    }
    CSR_BT_SPP_HANDOVER_LOG_INFO("SppVeto %d", veto);
    return veto;
}

static bool sppMarshal(const tp_bdaddr *vmTpAddrt,
                              CsrUint8 *buf,
                              CsrUint16 length,
                              CsrUint16 *written)
{
    CsrUint8 i, noOfConnections = 0;
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_SPP_HANDOVER_LOG_INFO("SppMarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!sppConverter)
    {
        sppConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(sppConverter, length, buf, TRUE);

    noOfConnections = sppGetConnectedConnectionCountForBdAddr(tpAddrt.addrt.addr);
    if (noOfConnections > 0)
    {
        CsrBtMarshalUtilConvertObj(sppConverter, noOfConnections);
    }

    for (i = 0; i < sppInstanceData.numberOfSppInstances && noOfConnections; i++)
    {
        /* Since a single device can have more than one instance, loop through
         * the SPP manager and marshal all the matching instances */
        SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];
        if (instData)
        {
            CsrBool instExists = CSR_BT_BD_ADDR_EQ(tpAddrt.addrt.addr, instData->bdAddr);
            if (instExists)
            {
                serSppInstData(sppConverter, instData);
                noOfConnections--;
            }
        }
    }

    *written = length - CsrBtMarshalUtilRemainingLengthGet(sppConverter);

    return CsrBtMarshalUtilStatus(sppConverter);
}

static bool sppUnmarshal(const tp_bdaddr *vmTpAddrt,
                                const CsrUint8 *buf,
                                CsrUint16 length,
                                CsrUint16 *written)
{
    CsrUint8 i, noOfConnections = 0;
    CsrSchedQid queueId = 0;
    CsrBtTpdAddrT tpAddrt = { 0 };

    CSR_BT_SPP_HANDOVER_LOG_INFO("SppUnmarshal");

    BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

    if (!sppConverter)
    {
        sppConverter = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(sppConverter, length, (CsrUint8 *) buf, TRUE);

    /* Get the number of SPP connections with this device */
    CsrBtMarshalUtilConvertObj(sppConverter, noOfConnections);
    /* Get the queueId/appHandle for the instance to be unmarshalled */
    CsrBtMarshalUtilConvertObj(sppConverter, queueId);

    for (i = 0; i < sppInstanceData.numberOfSppInstances && noOfConnections; i++)
    {
        SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];
        if (instData)
        {
            CsrBtDeviceAddr zeroBdAddr = { 0,0,0 };
            if (CSR_BT_BD_ADDR_EQ(zeroBdAddr, instData->bdAddr) && instData->myAppHandle == queueId)
            {
                deserSppInstData(sppConverter, instData);
                instData->bdAddr = tpAddrt.addrt.addr;
                noOfConnections--;
                if (noOfConnections)
                {
                    /* Get the queueId of the next instance */
                    CsrBtMarshalUtilConvertObj(sppConverter, queueId);
                }
            }
        }
    }

    *written = length - CsrBtMarshalUtilRemainingLengthGet(sppConverter);

    return CsrBtMarshalUtilStatus(sppConverter);
}

static void sppHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                   bool newPrimary)
{
    if (newPrimary)
    {
        CsrUint8 i;
        CsrBtTpdAddrT tpAddrt = { 0 };

        BdaddrConvertTpVmToBluestack(&tpAddrt, vmTpAddrt);

        for (i = 0; i < sppInstanceData.numberOfSppInstances; i++)
        {
            /* Since a single device can have more than one instance, loop through
             * the SPP manager and find all the matching instances */
            SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];
            if (instData)
            {
                CsrBool instExists = CSR_BT_BD_ADDR_EQ(tpAddrt.addrt.addr, instData->bdAddr);
                if (instExists)
                {
                    /* Since handover is done only for connected instances, move the state to connected */
                    instData->state = Connected_s;
                    instData->numberOfConnections = MAX_NUMBER_OF_CONNECTIONS;
                    CsrBtCmCancelAcceptConnectReqSend(instData->myAppHandle, instData->serverChannel);
                    CsrBtCmSdsUnRegisterReqSend(instData->myAppHandle, instData->sdsRecHandle, CSR_BT_CM_CONTEXT_UNUSED);
                    instData->sdsUnregInProgress = TRUE;
                }
            }
        }
    }
}

static void sppHandoverComplete(bool newPrimary)
{
    CSR_BT_SPP_HANDOVER_LOG_INFO("SppHandoverComplete");

    if (sppConverter)
    {
        CsrBtMarshalUtilDestroy(sppConverter);
        sppConverter = NULL;
    }

    CSR_UNUSED(newPrimary);
}

static void sppHandoverAbort(void)
{
    CsrUint8 i;
    CSR_BT_SPP_HANDOVER_LOG_INFO("SppHandoverAbort");
    if (sppConverter && CsrBtMarshalUtilTypeGet(sppConverter) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
    {
        for (i = 0; i < sppInstanceData.numberOfSppInstances; i++)
        {
            SppInstanceData_t *instData = (SppInstanceData_t *) sppInstanceData.sppInstances->connInstPtrs[i];
            if (instData)
            {
                instData->sppConnId = SPP_NO_CONNID;
                CsrBtBdAddrZero(&instData->bdAddr);
            }
        }
    }
    sppHandoverComplete(FALSE);
}

const handover_interface spp_handover_if =
        MAKE_BREDR_HANDOVER_IF(&sppVeto,
                               &sppMarshal,
                               &sppUnmarshal,
                               &sppHandoverCommit,
                               &sppHandoverComplete,
                               &sppHandoverAbort);
