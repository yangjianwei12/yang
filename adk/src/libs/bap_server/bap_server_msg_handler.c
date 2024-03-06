/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include <gatt_manager.h>


#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_pacs.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_scan_delegator.h"
#include "connection_no_ble.h"

void BapServerMsgHandler(Task task, MessageId id, Message payload)
{
    void* msg = (void*)payload;
    BAP *bapInst = (BAP *)task;

    switch (id)
    {
        /*case ASCS_SERVER_PRIM:
            bapServerHandleGattAscsServerMsg(bapInst, msg);
            break;*/
        case GATT_BASS_SERVER_SCANNING_STATE_IND:
        case GATT_BASS_SERVER_ADD_SOURCE_IND:
        case GATT_BASS_SERVER_MODIFY_SOURCE_IND:
        case GATT_BASS_SERVER_BROADCAST_CODE_IND:
        case GATT_BASS_SERVER_REMOVE_SOURCE_IND:
            bapServerHandleGattBassServerMsg(bapInst, id, msg);
            break;

        case CL_DM_ISOC_REGISTER_CFM:
        case CL_DM_ISOC_SETUP_ISOCHRONOUS_DATA_PATH_CFM:
        case CL_DM_ISOC_REMOVE_ISO_DATA_PATH_CFM:
        case CL_DM_ISOC_BIG_CREATE_SYNC_CFM:
        case CL_DM_BLE_BIGINFO_ADV_REPORT_IND:
        case CL_DM_ISOC_BIG_TERMINATE_SYNC_IND:
            bapServerHandleClMsg(bapInst,id, msg);
            break;

        default:
            BAP_DEBUG_PANIC(("PANIC: BAP Server Profile Msg not handled \n"));
    }
}


