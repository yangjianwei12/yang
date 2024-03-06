/* Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_notification.h"
#include "gatt_ams_client_private.h"
#include "gatt_ams_client_discover.h"
#include "gatt_ams_client_write.h"
#include "gatt_ams_client_external_msg_send.h"
#include <gatt_lib.h>

static bool discoverCharacteristicDescriptors(GAMS *ams, uint8 characteristic)
{
    uint16 startHandle, endHandle;

    startHandle = MIN(gattAmsGetCharacteristicHandle(ams, characteristic) + 1, ams->service_end_handle);
    endHandle = gattAmsfindEndHandleForCharDesc(ams, startHandle, ams->service_end_handle, characteristic);

    if(startHandle && endHandle)
        return gattAmsDiscoverAllCharacteristicDescriptors(ams, startHandle, endHandle);
    else
        return FALSE;
}

void amsSetNotificationRequest(GAMS *ams, bool notifications_enable, uint8 characteristic)
{
    switch(characteristic)
    {
        case GATT_AMS_CLIENT_REMOTE_COMMAND:
            if (CHECK_VALID_HANDLE(ams->remote_command_handle))
            {
                /* First check if ccd handle is found, else find it */
                if (CHECK_VALID_HANDLE(ams->remote_command_ccd))
                {
                    PRINT(("AMS: Write Remote Command config\n"));
                    gattAmsWriteCharacteristicNotifyConfig(ams, notifications_enable, ams->remote_command_ccd);
                    ams->pending_cmd = ams_pending_write_remote_command_cconfig;
                }
                else
                {
                    PRINT(("AMS: DiscoverAllCharacteristicDescriptors for Remote Command\n"));
                    if (discoverCharacteristicDescriptors(ams, GATT_AMS_CLIENT_REMOTE_COMMAND))
                    {
                        if (notifications_enable)
                            ams->pending_cmd = ams_pending_remote_command_notify_enable;
                        else
                            ams->pending_cmd = ams_pending_remote_command_notify_disable;
                    }
                    else
                        gattAmsSendSetRemoteCommandNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR);
                }
            }
            else
                gattAmsSendSetRemoteCommandNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        break;

        case GATT_AMS_CLIENT_ENTITY_UPDATE:
            if (CHECK_VALID_HANDLE(ams->entity_update_handle))
            {
                /* First check if ccd handle is found, else find it */
                if (CHECK_VALID_HANDLE(ams->entity_update_ccd))
                {
                    PRINT(("AMS: Write Entity Update config\n"));
                    gattAmsWriteCharacteristicNotifyConfig(ams, notifications_enable, ams->entity_update_ccd);
                    ams->pending_cmd = ams_pending_write_entity_update_cconfig;
                }
                else
                {
                    PRINT(("AMS: DiscoverAllCharacteristicDescriptors for Entity Update\n"));
                    if (discoverCharacteristicDescriptors(ams, GATT_AMS_CLIENT_ENTITY_UPDATE))
                    {
                        if (notifications_enable)
                            ams->pending_cmd = ams_pending_entity_update_notify_enable;
                        else
                            ams->pending_cmd = ams_pending_entity_update_notify_disable;
                    }
                    else
                        gattAmsSendSetEntityUpdateNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR);
                }
            }
            else
                gattAmsSendSetEntityUpdateNotificationResponse(ams, CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        break;

        default:
            PANIC(("AMS: Not a valid characteristic for notifications\n"));
        break;
    }
}
