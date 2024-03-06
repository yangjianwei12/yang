/* Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csr_bt_gatt_lib.h"
#include "gatt_device_info_server_debug.h"
#include "gatt_device_info_server_private.h"
#include "gatt_device_info_server_msg_handler.h"
#include "gatt_device_info_server_db.h"

/* PnP structure in the characteristic value is as follows
 * 
 * Octet    Size    Description
 * 0        uint8   Vendor ID Source 1..2
 * 1..2     uint16  Vendor ID
 * 3..4     uint16  Product ID
 * 5..6     uint16  Product Version
 * 
 * Total value size is 7-octets.
 */
#define DIS_PNP_VAL_SIZE (sizeof(uint8) * 7)
#define DIS_PNP_VENDOR_ID_SRC_OFFSET 0
#define DIS_PNP_VENDOR_ID_OFFSET 1
#define DIS_PNP_PRODUCT_ID_OFFSET 3
#define DIS_PNP_PRODUCT_VER_OFFSET 5

void CsrBtDisHandler(void** gash);

/******************************************************************************/
static void disHandleRegisterHandleRangeCfm(gdiss_t *const dev_info_server,
                                     const CsrBtGattFlatDbRegisterHandleRangeCfm *cfm)
{
    CSR_UNUSED(dev_info_server);
    CSR_UNUSED(cfm);
}

/***************************************************************************
NAME
    disHandleReadAccess

DESCRIPTION
    Deals with access of Device Information Service chanracteristics.
*/

static void disHandleReadAccess(gdiss_t *const dev_info_server,
                                const CsrBtGattDbAccessReadInd *access_ind)
{
    size_t size = 0;
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    uint16 handle = access_ind->attrHandle;
    uint8 *value = NULL;
    gatt_dis_strings_t* dis_strings = dev_info_server->dis_params.dis_strings;

    switch(handle)
    {
        case HANDLE_MANUFACTURER_NAME:
            if((dis_strings != NULL) && (dis_strings->manufacturer_name_string != NULL))
            {
                size = strlen(dis_strings->manufacturer_name_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->manufacturer_name_string, size);
            }
            break;

        case HANDLE_MODEL_NUMBER:
            if((dis_strings != NULL) && (dis_strings->model_num_string != NULL))
            {
                size = strlen(dis_strings->model_num_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->model_num_string, size);
            }
            break;

        case HANDLE_SERIAL_NUMBER:
            if((dis_strings != NULL) && (dis_strings->serial_num_string != NULL))
            {
                size = strlen(dis_strings->serial_num_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->serial_num_string, size);
            }
            break;

        case HANDLE_HARDWARE_REVISION:
            if((dis_strings != NULL) && (dis_strings->hw_revision_string != NULL))
            {
                size = strlen(dis_strings->hw_revision_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->hw_revision_string, size);
            }
            break;

        case HANDLE_FIRMWARE_REVISION:
            if((dis_strings != NULL) && (dis_strings->fw_revision_string != NULL))
            {
                size = strlen(dis_strings->fw_revision_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->fw_revision_string, size);
            }
            break;

        case HANDLE_SOFTWARE_REVISION:
            if((dis_strings != NULL) && (dis_strings->sw_revision_string != NULL))
            {
                size = strlen(dis_strings->sw_revision_string);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dis_strings->sw_revision_string, size);
            }
            break;

        case HANDLE_SYSTEM_ID:
            if(dev_info_server->dis_params.system_id != NULL)
            {
                size = sizeof(dev_info_server->dis_params.system_id);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dev_info_server->dis_params.system_id, size);
            }
            break;

        case HANDLE_IEEE_DATA:
            if(dev_info_server->dis_params.ieee_data != NULL)
            {
                size = sizeof(dev_info_server->dis_params.ieee_data);
                value = (uint8*) CsrPmemAlloc(size);
                memcpy(value,dev_info_server->dis_params.ieee_data, size);
            }
            break;

        case HANDLE_PNP_ID:
            if(dev_info_server->dis_params.pnp_id != NULL)
            {
                size = DIS_PNP_VAL_SIZE;
                value = (uint8*) CsrPmemAlloc(size);
                value[DIS_PNP_VENDOR_ID_SRC_OFFSET] = dev_info_server->dis_params.pnp_id->vendor_id_source;
                CSR_COPY_UINT16_TO_LITTLE_ENDIAN(
                    dev_info_server->dis_params.pnp_id->vendor_id,
                    &value[DIS_PNP_VENDOR_ID_OFFSET]);
                CSR_COPY_UINT16_TO_LITTLE_ENDIAN(
                    dev_info_server->dis_params.pnp_id->product_id,
                    &value[DIS_PNP_PRODUCT_ID_OFFSET]);
                CSR_COPY_UINT16_TO_LITTLE_ENDIAN(
                    dev_info_server->dis_params.pnp_id->product_version,
                    &value[DIS_PNP_PRODUCT_VER_OFFSET]);
            }
            break;

         default:
            GATT_DEVICE_INFO_SERVER_DEBUG_PANIC((
                        "GDIS: GATT message for handle 0x%04x not handled\n",
                        handle
                        ));
            break;
    }
    CsrBtGattDbReadAccessResSend(dev_info_server->gattId,
                            access_ind->btConnId,
                            handle,
                            result,
                            (CsrUint16) size,
                            value);
}

/****************************************************************************/
void deviceInfoServerMsgHandler(void* task, MsgId id, Msg msg)
{
    gdiss_t *const device_info_server = (gdiss_t*)task;
    CsrBtGattPrim *primType = (CsrBtGattPrim *)msg;

    CSR_UNUSED(id);

    switch (*primType)
    {
        case CSR_BT_GATT_FLAT_DB_REGISTER_HANDLE_RANGE_CFM:
            disHandleRegisterHandleRangeCfm(device_info_server, (CsrBtGattFlatDbRegisterHandleRangeCfm*)msg);
            break;

        case CSR_BT_GATT_DB_ACCESS_WRITE_IND:
            /* Write access to characteristic is not permitted */
             CsrBtGattDbWriteAccessResSend(device_info_server->gattId,
                                      ((CsrBtGattDbAccessWriteInd *)msg)->btConnId,
                                      ((CsrBtGattDbAccessWriteInd *)msg)->attrHandle,
                                      CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED);
            break;

        case CSR_BT_GATT_DB_ACCESS_READ_IND:
            /* Read access to characteristic */
            disHandleReadAccess(device_info_server, (CsrBtGattDbAccessReadInd *)msg);
            break;

        default:
        {
            GATT_DEVICE_INFO_SERVER_DEBUG_PANIC((
                        "GDIS: GATT message 0x%04x not handled\n",
                        (*primType)
                        ));
            break;
        }
    } /* switch */
    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, (void *)msg);
}


void CsrBtDisHandler(void** gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    CsrBtGattPrim id;
    GDISS_T *disInst = (GDISS_T*)*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                id = *(CsrBtGattPrim*)msg;
                GATT_DEVICE_INFO_SERVER_DEBUG_INFO(("GDIS: CsrBtDisHandler MsgId 0x%x\n",id));
                deviceInfoServerMsgHandler((void*)disInst->deviceInfo, id, msg);
                break;
            }
            case DIS_SERVER_PRIM:
            default:
                break;
        }
        SynergyMessageFree(eventClass, msg);
    }
}
