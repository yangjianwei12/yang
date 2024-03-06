/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_utils.h"

uint16 generatePacRecordHandle(GPACSS_T *pacs_server)
{
    uint16 index;
    for (index = 1; index < MAX_PAC_RECORD_HANDLES; index++)
    {
        if (pacs_server->data.pacs_record_handle_mask == DEFAULT_PAC_RECORD_HANDLE_MASK)
        {
            pacs_server->data.pacs_record_handle_mask |= 1 << (index-1);
            GATT_PACS_SERVER_DEBUG_INFO(("PACS: generatePacRecordHandle 0x%x \n",pacs_server->data.pacs_record_handle_mask));
            return index;
        }
        else if ((pacs_server->data.pacs_record_handle_mask & (1 << (index-1))) != (1 << (index-1)))
        {
            pacs_server->data.pacs_record_handle_mask |= 1 << (index-1);
            GATT_PACS_SERVER_DEBUG_INFO(("PACS: generatePacRecordHandle 0x%x \n",pacs_server->data.pacs_record_handle_mask));
            return index;
        }
     }
    return (uint16) PACS_RECORD_HANDLES_EXHAUSTED;
}

void removePacRecordHandle(GPACSS_T *pacs_server, uint16 pac_record_handle)
{
    GATT_PACS_SERVER_DEBUG_INFO(("PACS: removePacRecordHandle 0x%x \n",pacs_server->data.pacs_record_handle_mask));
    pacs_server->data.pacs_record_handle_mask &= ~(1UL << (pac_record_handle -1));
    GATT_PACS_SERVER_DEBUG_INFO(("PACS: removePacRecordHandle 0x%x \n",pacs_server->data.pacs_record_handle_mask));
}


