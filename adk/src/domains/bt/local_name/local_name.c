/*!
\copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       local_name.c
\ingroup    local_name
\brief      Bluetooth Local Name component

*/

#include "connection_abstraction.h"
#include <connection.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <task_list.h>

#include "local_name.h"
#include <bredr_scan_manager.h>

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_TYPE(local_name_message_t)
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(LOCAL_NAME, LOCAL_NAME_MESSAGE_END)

#ifdef INCLUDE_LOCAL_NAME_LE_PREFIX
#define LOCAL_NAME_LE_PREFIX ("LE-")
#define LOCAL_NAME_SIZE_LE_PREFIX (3)
#else
#define LOCAL_NAME_LE_PREFIX ("")
#define LOCAL_NAME_SIZE_LE_PREFIX (0)
#endif

static void localName_MessageHandler(Task task, MessageId id, Message message);

static const TaskData local_name_task = {.handler = localName_MessageHandler};

static struct
{
    Task   client_task;
    uint8* name;
    uint16 name_len;
    uint16 suffix_len;
    bool include_suffix_ble;
    bool include_suffix_bt;
    task_list_t *clients;
} local_name_task_data;


static void localName_StoreName(uint8* local_name, uint16 size_local_name, hci_status status)
{
    if (status == hci_success)
    {
        uint16 len;
        local_name_task_data.name_len = size_local_name + LOCAL_NAME_SIZE_LE_PREFIX;

        len = local_name_task_data.name_len + local_name_task_data.suffix_len;
        char *new_name = PanicUnlessMalloc(len + 1);
        memcpy(new_name, LOCAL_NAME_LE_PREFIX, LOCAL_NAME_SIZE_LE_PREFIX);
        memcpy(new_name + LOCAL_NAME_SIZE_LE_PREFIX, local_name, size_local_name);
        memcpy(new_name + local_name_task_data.name_len,
               &local_name_task_data.name[local_name_task_data.name_len],
               local_name_task_data.suffix_len);
        new_name[local_name_task_data.name_len + local_name_task_data.suffix_len] = '\0';

        free(local_name_task_data.name);
        local_name_task_data.name = (uint8_t *)new_name;

        MessageSend(local_name_task_data.client_task, LOCAL_NAME_INIT_CFM, NULL);
    }
    else
    {
        DEBUG_LOG("localName_StoreName: failed");
        Panic();
    }
}


static void localName_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
#ifdef USE_SYNERGY
	case CM_PRIM:
	{
        if (*(CsrBtCmPrim *) message == CSR_BT_CM_READ_LOCAL_NAME_CFM)
        {
            CsrBtCmReadLocalNameCfm * name_msg = (CsrBtCmReadLocalNameCfm*)message;
            if (name_msg->localName)
            {
                localName_StoreName(name_msg->localName, CsrStrLen((char *)name_msg->localName), hci_success);
            }
        }
        if (*(CsrBtCmPrim *) message == CSR_BT_CM_SET_LOCAL_NAME_CFM)
        {
            ScanManager_ConfigureEirData(NULL);
        }
        CmFreeUpstreamMessageContents(message);
	}
	break;
#else
    case CL_DM_LOCAL_NAME_COMPLETE:
    {
        CL_DM_LOCAL_NAME_COMPLETE_T *name_msg = (CL_DM_LOCAL_NAME_COMPLETE_T*)message;
        localName_StoreName(name_msg->local_name, name_msg->size_local_name, name_msg->status);
    }
        break;
#endif
    default:
        DEBUG_LOG("localName_MessageHandler: unhandled MESSAGE:0x%04X", id);
        break;
    }
}

static void LocalName_RefreshBluetoothName(void)
{
    uint16 len = local_name_task_data.name_len - LOCAL_NAME_SIZE_LE_PREFIX;

    if (local_name_task_data.include_suffix_bt)
        len += local_name_task_data.suffix_len;

    char *new_name = PanicUnlessMalloc(len + 1);
    strncpy(new_name, (char *)local_name_task_data.name + LOCAL_NAME_SIZE_LE_PREFIX, len);
    new_name[len] = 0;

    DEBUG_LOG_INFO("Current local BT name: %s [%d]",
                   local_name_task_data.name + LOCAL_NAME_SIZE_LE_PREFIX,
                   len);
    CmSetLocalNameReqSend((Task)&local_name_task, (CsrUtf8String*)new_name);
}

bool LocalName_Init(Task init_task)
{
    DEBUG_LOG("LocalName_Init");

    local_name_task_data.client_task = init_task;
    local_name_task_data.clients = TaskList_Create();
    ConnectionReadLocalName((Task) &local_name_task);
    return TRUE;
}


const uint8 *LocalName_GetName(uint16* name_len)
{
    const uint8* name = LocalName_GetPrefixedName(name_len);
    name += LOCAL_NAME_SIZE_LE_PREFIX;
    *name_len -= LOCAL_NAME_SIZE_LE_PREFIX;

    if (local_name_task_data.include_suffix_ble)
    {
        *name_len += local_name_task_data.suffix_len;
    }

    return name;
}


const uint8 *LocalName_GetPrefixedName(uint16* name_len)
{
    PanicNull(name_len);
    *name_len = local_name_task_data.name_len;

    if (local_name_task_data.include_suffix_ble)
    {
        *name_len += local_name_task_data.suffix_len;
    }

    return PanicNull(local_name_task_data.name);
}

void LocalName_SetSuffix(const char *suffix)
{
    PanicNull((void *)suffix);
    PanicFalse(local_name_task_data.suffix_len == 0);

    local_name_task_data.suffix_len = strlen(suffix);
    if (!local_name_task_data.name)
    {
        local_name_task_data.name = PanicUnlessMalloc(local_name_task_data.suffix_len + 1);
        local_name_task_data.name_len = 0;
        strcpy((char *)local_name_task_data.name, suffix);
    }
    else
    {
        char *new_name = PanicUnlessMalloc(local_name_task_data.suffix_len + local_name_task_data.name_len + 1);
        strncpy(new_name, (char *)local_name_task_data.name, local_name_task_data.name_len);
        strncpy(new_name + local_name_task_data.name_len, suffix, local_name_task_data.suffix_len);
        new_name[local_name_task_data.name_len + local_name_task_data.suffix_len] = 0;
        free(local_name_task_data.name);
        local_name_task_data.name = (uint8_t *)new_name;
    }
}


void LocalName_SuffixEnable(bool bt_enable, bool ble_enable)
{
    local_name_task_data.include_suffix_bt = bt_enable;
    local_name_task_data.include_suffix_ble = ble_enable;

    LocalName_RefreshBluetoothName();
    TaskList_MessageSendId(local_name_task_data.clients, LOCAL_NAME_NAME_CHANGED_IND);
}


void LocalName_RegisterNotifications(Task client)
{
    TaskList_AddTask(local_name_task_data.clients, client);
}
