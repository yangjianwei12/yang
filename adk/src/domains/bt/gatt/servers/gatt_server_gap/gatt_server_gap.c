/*!
    \copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \ingroup    gatt_server_gap
    \brief      Implementation of the GAP Server module.
*/

#include "gatt_server_gap.h"

#include "gatt_server_gap_advertising.h"
#include "gatt_handler_db_if.h"
#include "local_name.h"

#ifdef USE_SYNERGY
#include "gatt_lib.h"
#else
#include <gatt_gap_server.h>
#endif
#include <logging.h>

#include <panic.h>

#define GAP_DEVICE_APPEARANCE_CHARACTERISTIC_LENGTH 2

/*! Structure holding information for the application handling of GAP Server */
typedef struct
{
    /*! Task for handling GAP related messages */
    TaskData    gap_task;
#ifdef USE_SYNERGY
    /*! Unique GATT id provided by GATT lib */
    CsrBtGattId    gattId;
#else
    /*! GAP server library data */
    GGAPS       gap_server;
#endif
    /*! Complete local name flag */
    bool        complete_local_name;

    /*! Appearance value for this device */
    uint16      appearance_value;
} gattServerGapData;

static gattServerGapData gatt_server_gap = {0};

/*! Get pointer to the main GAP server Task */
#define GetGattServerGapTask() (&gatt_server_gap.gap_task)

/*! Get pointer to the GAP server data passed to the library */
#define GetGattServerGapGgaps() (&gatt_server_gap.gap_server)

/*! Get the complete local name flag */
#define GetGattServerGapCompleteName() (gatt_server_gap.complete_local_name)

#ifdef USE_SYNERGY
static void gattServerGap_RegisterCfm(const CsrBtGattRegisterCfm *cfm)
{
    DEBUG_LOG("gattServerGap_RegisterCfm: resultCode(0x%04x) resultSupplier(0x%04x) ", cfm->resultCode, cfm->resultSupplier);

    if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
        cfm->resultSupplier == CSR_BT_SUPPLIER_GATT)
    {
        gatt_server_gap.gattId = cfm->gattId;
        GattFlatDbRegisterHandleRangeReqSend(gatt_server_gap.gattId, HANDLE_GAP_SERVICE, HANDLE_GAP_SERVICE_END);
    }
    else
    {
        DEBUG_LOG("gattServerGap_RegisterCfm: Gatt Registration failed");
        Panic();
    }
}

static void gattServerGap_DbRegisterHandleRangeCfm(const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    DEBUG_LOG("gattServerGap_DbRegisterHandleRangeCfm: resultCode(0x%04x) resultSupplier(0x%04x) ", cfm->resultCode, cfm->resultSupplier);
}

static void gattServerGap_HandleReadAccessInd(const CsrBtGattDbAccessReadInd *access_ind)
{
    DEBUG_LOG("gattServerGap_HandleReadAccessInd: handle %d",access_ind->attrHandle);
    uint16 handle = HANDLE_GAP_SERVICE + (access_ind->attrHandle - 1);
    uint16 size_value = 0;
    uint8  *value = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_SUCCESS;

    if(handle == HANDLE_DEVICE_NAME)
    {
        uint16 name_len;
        const uint8 *name = LocalName_GetPrefixedName(&name_len);
        PanicNull((void*)name);

        /* The indication can request a portion of our name by specifying the start offset */
        if (access_ind->offset)
        {
            /* Check that we haven't been asked for an entry off the end of the name */
            if (access_ind->offset >= name_len)
            {
                name_len = 0;
                name = NULL;
                status = CSR_BT_GATT_ACCESS_RES_INVALID_OFFSET;
            }
            else
            {
                name_len -= access_ind->offset;
                name += access_ind->offset;
            }
        }
        if(name_len)
        {
            value = (uint8*) CsrPmemAlloc(name_len);
            memcpy(value, name, name_len);
            size_value = name_len;
        }
    }
    else if(handle == HANDLE_DEVICE_APPEARANCE)
    {
        value = (uint8*) CsrPmemAlloc(GAP_DEVICE_APPEARANCE_CHARACTERISTIC_LENGTH);
        value[0] = (uint8) (gatt_server_gap.appearance_value & 0xFF);
        value[1] = (uint8) ((gatt_server_gap.appearance_value >> 8) & 0xFF);

        size_value = GAP_DEVICE_APPEARANCE_CHARACTERISTIC_LENGTH;
    }
    else
    {
        status = CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE;
    }

    GattDbReadAccessResSend(gatt_server_gap.gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            status,
                            size_value,
                            value);
}

static void gattServerGap_handleGattPrim(Message message)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *)message;

    switch (*prim)
    {
        case CSR_BT_GATT_REGISTER_CFM:
            gattServerGap_RegisterCfm((const CsrBtGattRegisterCfm*) message);
            break;

        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            gattServerGap_DbRegisterHandleRangeCfm((const CsrBtGattFlatDbRegisterHandleRangeCfm*)message);
            break;

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            gattServerGap_HandleReadAccessInd((const CsrBtGattDbAccessReadInd *)message);
        break;

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
        {
            CsrBtGattDbAccessWriteInd *access_ind = (CsrBtGattDbAccessWriteInd *)message;
            GattDbWriteAccessResSend(gatt_server_gap.gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED);
        }
        break;

        default:
            DEBUG_LOG("gattServerGap_handleGattPrim. Unhandled message MESSAGE:0x%04x", *prim);
            break;
    }
    GattFreeUpstreamMessageContents((void *)message);
}
#else
static void gattServerGap_HandleReadAppearanceInd(const GATT_GAP_SERVER_READ_APPEARANCE_IND_T *ind)
{
    uint8 appearance[GAP_DEVICE_APPEARANCE_CHARACTERISTIC_LENGTH];

    appearance[0] = (uint8) (gatt_server_gap.appearance_value & 0xFF);
    appearance[1] = (uint8) ((gatt_server_gap.appearance_value >> 8) & 0xFF);

    PanicFalse(GetGattServerGapGgaps() == ind->gap_server);

    GattGapServerReadAppearanceResponse(GetGattServerGapGgaps(), ind->cid,
                                        (GAP_DEVICE_APPEARANCE_CHARACTERISTIC_LENGTH * sizeof(uint8)), appearance);
}

static void gattServerGap_HandleReadDeviceNameInd(const GATT_GAP_SERVER_READ_DEVICE_NAME_IND_T *ind)
{
    uint16 name_len;
    const uint8 *name = LocalName_GetPrefixedName(&name_len);
    PanicNull((void*)name);

    /* The indication can request a portion of our name by specifying the start offset */
    if (ind->name_offset)
    {
        /* Check that we haven't been asked for an entry off the end of the name */

        if (ind->name_offset >= name_len)
        {
            name_len = 0;
            name = NULL;
        /*  \todo return gatt_status_invalid_offset  */
        }
        else
        {
            name_len -= ind->name_offset;
            name += ind->name_offset;
        /*  \todo return gatt_status_success  */
        }
    }

    PanicFalse(GetGattServerGapGgaps() == ind->gap_server);

    GattGapServerReadDeviceNameResponse(GetGattServerGapGgaps(), ind->cid,
                                        name_len, (uint8*)name);
}
#endif

static void gattServerGap_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gattServerGap_MessageHandler MESSAGE:0x%x", id);

    switch (id)
    {
        case LOCAL_NAME_NAME_CHANGED_IND:
            GattServerGap_UpdateLeAdvertisingData();
            break;

#ifdef USE_SYNERGY
        case GATT_PRIM:
            gattServerGap_handleGattPrim(message);
            break;
#else
        case GATT_GAP_SERVER_READ_DEVICE_NAME_IND:
            gattServerGap_HandleReadDeviceNameInd((const GATT_GAP_SERVER_READ_DEVICE_NAME_IND_T*)message);
            break;
        case GATT_GAP_SERVER_READ_APPEARANCE_IND:
            gattServerGap_HandleReadAppearanceInd((const GATT_GAP_SERVER_READ_APPEARANCE_IND_T*)message);
            break;
#endif

        default:
            DEBUG_LOG("gattServerGap_MessageHandler. Unhandled message MESSAGE:0x%x", id);
            break;
    }
}

/*****************************************************************************/
bool GattServerGap_Init(Task init_task)
{
    UNUSED(init_task);

    memset(&gatt_server_gap, 0, sizeof(gatt_server_gap));

    gatt_server_gap.gap_task.handler = gattServerGap_MessageHandler;

#ifdef USE_SYNERGY
    GattRegisterReqSend(GetGattServerGapTask(), 0);
#else
    if (GattGapServerInit(GetGattServerGapGgaps(), GetGattServerGapTask(),
            HANDLE_GAP_SERVICE, HANDLE_GAP_SERVICE_END)
                            != gatt_gap_server_status_success)
    {
        DEBUG_LOG("gattServerGap_init Server failed");
        Panic();
    }
#endif
    GattServerGap_SetupLeAdvertisingData();

    DEBUG_LOG("GattServerGap_Init. Server initialised");
    LocalName_RegisterNotifications(&gatt_server_gap.gap_task);

    return TRUE;
}

void GattServerGap_UseCompleteLocalName(bool complete)
{
    gatt_server_gap.complete_local_name = complete;
}

bool GattServerGap_IsCompleteLocalNameBeingUsed(void)
{
    return gatt_server_gap.complete_local_name;
}

void GattServerGap_SetAppearanceValue(uint16 appearance_value)
{
    gatt_server_gap.appearance_value = appearance_value;
}

uint16 GattServerGap_GetAppearanceValue(void)
{
    return gatt_server_gap.appearance_value;
}
