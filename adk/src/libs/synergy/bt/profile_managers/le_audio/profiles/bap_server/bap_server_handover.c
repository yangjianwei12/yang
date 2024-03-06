/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "gatt_pacs_server_handover.h"
#include "gatt_ascs_server_handover.h"
#include "gatt_bass_server_handover.h"
#include "csr_bt_gatt_client_util_lib.h"

#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_debug.h"
#include "csr_bt_marshal_util.h"
#include "bap_server_handover.h"
#include "csr_bt_handover_if.h"

#define HANDOVER_DATA_PRESENT_ASCS 0x01
#define HANDOVER_DATA_PRESENT_BASS 0x02

static struct
{
    CsrBtMarshalUtilInst *conv;
    uint32 cid;
    bool uPresent;
    uint8 svcPresent;
} *bapServerConverter;

typedef enum
{
    BAP_SERVER_HANDOVER_STATE_IDLE,
    BAP_SERVER_HANDOVER_STATE_BAP,
    BAP_SERVER_HANDOVER_STATE_ASCS,
    BAP_SERVER_HANDOVER_STATE_PACS,
    BAP_SERVER_HANDOVER_STATE_BASS,
}BapServerHandoverState;

BapServerHandoverState bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_IDLE;

static void serBapServerData(CsrBtMarshalUtilInst *conv,
                             uint32 cid)
{
    bool present = TRUE;

    /* Signifies connection instance is present */
    CsrBtMarshalUtilConvertObj(conv, present);

    CsrBtMarshalUtilConvertObj(conv, cid);
    bapServerConverter->cid = cid;
    bapServerConverter->uPresent = present;
}

static void serHandoverServiceDataPresent(CsrBtMarshalUtilInst *conv,
                                          uint8 servicesPresent)
{
    BAP_SERVER_INFO("serHandoverServiceDataPresent: servicesPresent (%d)", servicesPresent);
    CsrBtMarshalUtilConvertObj(conv, servicesPresent);
    bapServerConverter->svcPresent = servicesPresent;
}

static void deserBapServerData(CsrBtMarshalUtilInst *conv)
{
    bool present;
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());
    uint32 cidInfo;

    present = bapServerConverter->uPresent;

    /* Find if connection instance is present or not */
    CsrBtMarshalUtilConvertObj(conv, present);

    if (present && bapInst)
    {
        uint8 servicesPresent;
        bapServerConverter->uPresent = TRUE;

        CsrBtMarshalUtilConvertObj(conv, cidInfo);

        bapServerAddConnectionIdToList(bapInst, cidInfo);

        /* Store cidinfo for handoverabort if in case happens */
        bapServerConverter->cid = cidInfo;

        CsrBtMarshalUtilConvertObj(conv, servicesPresent);

        /* Store the bitmask of services whose marshalled data is present */
        bapServerConverter->svcPresent = servicesPresent;
        BAP_SERVER_INFO("deserBapServerData: servicesPresent (%d)", servicesPresent);
    }
    else
    {
        bapServerConverter->uPresent = FALSE;
        bapServerConverter->cid = 0;
        bapServerConverter->svcPresent = 0;
    }
}

static void bapServerUtilconvertToTypedBdAddr(const tp_bdaddr *tpBdAddr, CsrBtTypedAddr *typedAddr)
{
    typedAddr->type = tpBdAddr->taddr.type;
    typedAddr->addr.lap = tpBdAddr->taddr.addr.lap;
    typedAddr->addr.uap = tpBdAddr->taddr.addr.uap;
    typedAddr->addr.nap = tpBdAddr->taddr.addr.nap;
}

bool BapServerHandoverVeto(void)
{

    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());

    if(bapInst)
    {
        if(gattAscsServerHandoverVeto(bapInst->ascsHandle) || gattPacsServerHandoverVeto()
            || gattBassServerHandoverVeto(bapInst->bassHandle))
        {
            BAP_SERVER_DEBUG("BapServerHandoverVeto");
            return TRUE;
        }
    }

    return FALSE;
}

bool BapServerHandoverMarshal(const tp_bdaddr *tpBdAddr,
                              uint8 *buf,
                              uint16 length,
                              uint16 *written)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());
    bool present = FALSE;
    uint16 writtenCount = 0;
    CsrBtTypedAddr typedAddr = {0};
    uint32 cid = 0;
    uint8 status = 0;
    bapServerUtilconvertToTypedBdAddr(tpBdAddr, &typedAddr);
    cid = CsrBtGattClientUtilFindConnIdByAddr(&typedAddr);

    if (!bapServerConverter)
    {
        bapServerConverter = CsrPmemZalloc(sizeof(*bapServerConverter));
        bapServerConverter->conv= CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_SERIALIZER);
    }

    CsrBtMarshalUtilResetBuffer(bapServerConverter->conv, length, buf, TRUE);

    if(bapInst)
    {
        if ((cid != INVALID_BTCONNID) && bapServerIsValidConectionId(bapInst, cid))
        {
            if ( bapServerHandoverNextState <= BAP_SERVER_HANDOVER_STATE_BAP)
            {
                uint8 svcPresent = 0;
                bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_BAP;
                serBapServerData(bapServerConverter->conv, cid);

                if (gattAscsServerHasValidConnection(bapInst->ascsHandle, cid))
                    svcPresent |= HANDOVER_DATA_PRESENT_ASCS;

                if (gattBassServerHasValidConnection(bapInst->bassHandle, cid))
                    svcPresent |= HANDOVER_DATA_PRESENT_BASS;

                serHandoverServiceDataPresent(bapServerConverter->conv, svcPresent);

                *written = length - CsrBtMarshalUtilRemainingLengthGet(bapServerConverter->conv);

                if (!CsrBtMarshalUtilStatus(bapServerConverter->conv))
                    return FALSE;

                writtenCount += *written;

                bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_ASCS;
            }

            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_ASCS)
            {
                status = gattAscsServerHandoverMarshal(bapInst->ascsHandle, cid, (buf + writtenCount), (length - writtenCount), written);

                writtenCount += *written;
                *written = writtenCount;

                if (status)
                    bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_PACS;
                else
                    return FALSE;
            }

            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_PACS)
            {
                status = gattPacsServerHandoverMarshal(bapInst->pacsHandle, cid, (buf + writtenCount), (length - writtenCount), written);

                writtenCount += *written;
                *written = writtenCount;

                if (status)
                    bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_BASS;
                else
                    return FALSE;
            }

            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_BASS)
            {
                status = gattBassServerHandoverMarshal(bapInst->bassHandle, cid, (buf + writtenCount), (length - writtenCount), written);

                writtenCount += *written;
                *written = writtenCount;

                if (!status)
                    return FALSE;
            }
        }
        else
        {
            /* Signifies connection instance is not present */
            CsrBtMarshalUtilConvertObj(bapServerConverter->conv, present);
            *written = length - CsrBtMarshalUtilRemainingLengthGet(bapServerConverter->conv);
            return CsrBtMarshalUtilStatus(bapServerConverter->conv);
        }
    }
    else
    {
        return FALSE;
    }

    bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_IDLE;
    return TRUE;
}

bool BapServerHandoverUnmarshal(const tp_bdaddr *tpBdAddr,
                                const uint8 *buf,
                                uint16 length,
                                uint16 *consume)
{
    uint16 consumedCount = 0;
    bool status = TRUE;
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());

    CSR_UNUSED(tpBdAddr);

    if(bapInst)
    {
        if (!bapServerConverter)
        {
            bapServerConverter = CsrPmemZalloc(sizeof(*bapServerConverter));
            bapServerConverter->conv = CsrBtMarshalUtilCreate(CSR_BT_MARSHAL_UTIL_DESERIALIZER);
        }

        CsrBtMarshalUtilResetBuffer(bapServerConverter->conv, length, (void *) buf, TRUE);

        if ( bapServerHandoverNextState <= BAP_SERVER_HANDOVER_STATE_BAP)
        {
            bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_BAP;
            deserBapServerData(bapServerConverter->conv);

            *consume = length - CsrBtMarshalUtilRemainingLengthGet(bapServerConverter->conv);

            /* Signifies connection instance is not present */
            if (bapServerConverter->uPresent == FALSE)
                return TRUE;

            if (!CsrBtMarshalUtilStatus(bapServerConverter->conv))
                return FALSE;

            consumedCount += *consume;
            bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_ASCS;
        }
        if (bapServerConverter->cid)
        {
            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_ASCS)
            {
                if ((bapServerConverter->svcPresent & HANDOVER_DATA_PRESENT_ASCS) == HANDOVER_DATA_PRESENT_ASCS)
                {
                    status = gattAscsServerHandoverUnmarshal(bapInst->ascsHandle, bapServerConverter->cid, (buf + consumedCount), (length - consumedCount), consume);

                    consumedCount += *consume;
                    *consume = consumedCount;
                }

                if (status)
                    bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_PACS;
                else
                    return FALSE;
            }

            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_PACS)
            {
                status = gattPacsServerHandoverUnmarshal(bapInst->pacsHandle, bapServerConverter->cid, (buf + consumedCount), (length - consumedCount), consume);

                consumedCount += *consume;
                *consume = consumedCount;

                if (status)
                    bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_BASS;
                else
                    return FALSE;
            }

            if ( bapServerHandoverNextState == BAP_SERVER_HANDOVER_STATE_BASS)
            {
                if ((bapServerConverter->svcPresent & HANDOVER_DATA_PRESENT_BASS) == HANDOVER_DATA_PRESENT_BASS)
                {
                    status = gattBassServerHandoverUnmarshal(bapInst->bassHandle, bapServerConverter->cid, (buf + consumedCount), (length - consumedCount), consume);
                    consumedCount += *consume;
                    *consume = consumedCount;
                }

                if (!status)
                    return FALSE;
            }
        }
        bapServerHandoverNextState = BAP_SERVER_HANDOVER_STATE_IDLE;
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void BapServerHandoverCommit(const tp_bdaddr *tpBdAddr,
                             const bool newPrimary)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());
    CsrBtTypedAddr typedAddr = {0};
    uint32 cid = 0;
    bapServerUtilconvertToTypedBdAddr(tpBdAddr, &typedAddr);
    cid = CsrBtGattClientUtilFindConnIdByAddr(&typedAddr);

    if(bapInst && bapServerIsValidConectionId(bapInst, cid))
    {
        gattAscsServerHandoverCommit(bapInst->ascsHandle, cid, newPrimary);
        gattPacsServerHandoverCommit(bapInst->pacsHandle, cid, newPrimary);
        gattBassServerHandoverCommit(bapInst->bassHandle, cid, newPrimary);
    }
    BAP_SERVER_DEBUG("BapServerHandoverCommit");
}

void BapServerHandoverComplete(const bool newPrimary)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());

    if (bapServerConverter)
    {
        CsrBtMarshalUtilDestroy(bapServerConverter->conv);
        CsrPmemFree(bapServerConverter);
        bapServerConverter = NULL;
    }

    if(bapInst)
    {
        gattAscsServerHandoverComplete(bapInst->ascsHandle, newPrimary);
        gattPacsServerHandoverComplete(bapInst->pacsHandle, newPrimary);
        gattBassServerHandoverComplete(bapInst->bassHandle, newPrimary);
        BAP_SERVER_DEBUG("BapServerHandoverComplete");
    }
}

void BapServerHandoverAbort(void)
{
    BAP *bapInst = FIND_BAP_SERVER_INST_BY_PROFILE_HANDLE(bapServerGetBapInstance());

    if(bapInst)
    {
        if (bapServerConverter)
        {
            if (CsrBtMarshalUtilTypeGet(bapServerConverter->conv) == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
                bapServerRemoveConnectionIdFromList(bapInst, bapServerConverter->cid);

            CsrBtMarshalUtilDestroy(bapServerConverter->conv);
            CsrPmemFree(bapServerConverter);
            bapServerConverter = NULL;
        }


        gattAscsServerHandoverAbort(bapInst->ascsHandle);
        gattPacsServerHandoverAbort(bapInst->pacsHandle);
        gattBassServerHandoverAbort(bapInst->bassHandle);
        BAP_SERVER_DEBUG("BapServerHandoverAbort");
    }
}
