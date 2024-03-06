/******************************************************************************
 Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "csr_synergy.h"

#include "csr_bt_handover_if.h"
#include "csr_bt_marshal_util.h"
#include "csr_pmem.h"
#include "csr_bt_panic.h"
#include "gatt_service_discovery_init.h"
#include "gatt_service_discovery_handler.h"

#ifdef CSR_LOG_ENABLE
#include "csr_log_text_2.h"
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(CsrBtGattSdLto);
#endif

#ifdef CSR_LOG_ENABLE
#define CSR_BT_GATT_SD_LTSO_HANDOVER             0
#define CSR_BT_GATT_SD_HANDOVER_LOG_INFO(...)    CSR_LOG_TEXT_INFO((CsrBtGattSdLto, CSR_BT_GATT_SD_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_GATT_SD_HANDOVER_LOG_WARNING(...) CSR_LOG_TEXT_WARNING((CsrBtGattSdLto, CSR_BT_GATT_SD_LTSO_HANDOVER, __VA_ARGS__))
#define CSR_BT_GATT_SD_HANDOVER_LOG_ERROR(...)   CSR_LOG_TEXT_ERROR((CsrBtGattSdLto, CSR_BT_GATT_SD_LTSO_HANDOVER, __VA_ARGS__))
#else
#define CSR_BT_GATT_SD_HANDOVER_LOG_INFO(...)
#define CSR_BT_GATT_SD_HANDOVER_LOG_WARNING(...)
#define CSR_BT_GATT_SD_HANDOVER_LOG_ERROR(...)
#endif

static bool csrBtGattSdVeto(void)
{
    GGSD *gatt_sd = gattServiceDiscoveryGetInstance();
    bool veto = FALSE;

    if (gatt_sd->state == GATT_SRVC_DISC_STATE_INPROGRESS)
    {
        veto = TRUE;
    }

    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdVeto %d", veto);

    return veto;
}

static bool csrBtGattSdMarshal(const tp_bdaddr *vmTpAddrt,
                               CsrUint8 *buf,
                               CsrUint16 length,
                               CsrUint16 *written)
{
    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdMarshal");

    CSR_UNUSED(vmTpAddrt);
    CSR_UNUSED(buf);
    CSR_UNUSED(length);
    CSR_UNUSED(written);

    return TRUE;
}

static bool csrBtGattSdUnmarshal(const tp_bdaddr *vmTpAddrt,
                             const CsrUint8 *buf,
                             CsrUint16 length,
                             CsrUint16 *written)
{
    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdUnmarshal");

    CSR_UNUSED(vmTpAddrt);
    CSR_UNUSED(buf);
    CSR_UNUSED(length);
    CSR_UNUSED(written);

    return TRUE;
}

static void csrBtGattSdHandoverCommit(const tp_bdaddr *vmTpAddrt,
                                  const bool newPrimary)
{
    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdHandoverCommit");

    CSR_UNUSED(vmTpAddrt);
    CSR_UNUSED(newPrimary);
}

static void csrBtGattSdHandoverComplete(const bool newPrimary)
{
    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdHandoverComplete");
    GGSD *gatt_sd = gattServiceDiscoveryGetInstance();

    if (!newPrimary)
    {   /* Free GATT Service Discovery device list */
        GATT_SD_DL_CLEANUP(gatt_sd->deviceList);
        gatt_sd->deviceList = NULL;
    }
}

static void csrBtGattSdHandoverAbort(void)
{
    CSR_BT_GATT_SD_HANDOVER_LOG_INFO("csrBtGattSdHandoverAbort");
}

const handover_interface csr_bt_gatt_sd_handover_if =
    MAKE_BLE_HANDOVER_IF(&csrBtGattSdVeto,
                         &csrBtGattSdMarshal,
                         &csrBtGattSdUnmarshal,
                         &csrBtGattSdHandoverCommit,
                         &csrBtGattSdHandoverComplete,
                         &csrBtGattSdHandoverAbort);

