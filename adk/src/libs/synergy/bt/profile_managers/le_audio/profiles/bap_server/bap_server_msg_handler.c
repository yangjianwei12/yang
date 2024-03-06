/****************************************************************************
* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_prim.h"

#include "bap_server_private.h"
#include "bap_server_msg_handler.h"
#include "bap_server_pacs.h"
#include "bap_server_debug.h"
#include "bap_server_init.h"
#include "bap_server_common.h"
#include "bap_server_scan_delegator.h"

void BapServerMsgHandler(void **gash)
{
    uint16 eventClass = 0;
    void* msg = NULL;
    bapProfileHandle profileHandle;
    BAP *bapInst = NULL;
    
    profileHandle = *((bapProfileHandle*)*gash);
    bapInst = ServiceHandleGetInstanceData(profileHandle);

    if (bapInst)
    {
        if (CsrSchedMessageGet(&eventClass, &msg))
        {
            switch (eventClass)
            {
                case CSR_BT_GATT_PRIM:
                    break;
                case ASCS_SERVER_PRIM:
                    bapServerHandleGattAscsServerMsg(bapInst, msg);
                    break;
                case PACS_SERVER_PRIM:
                    bapServerHandleGattPacsServerMsg(bapInst, msg);
                    break;
                case BASS_SERVER_PRIM:
                    bapServerHandleGattBassServerMsg(bapInst, msg);
                    break;
                case CSR_BT_CM_PRIM:
                    bapServerHandleCMMsg(bapInst, msg);
                    break;

                default:
                    BAP_SERVER_PANIC("PANIC: BAP Server Profile Msg not handled \n");
            }
            SynergyMessageFree(eventClass, msg);
        }
    }
}


