/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_tds_client_discovery.h"
#include "gatt_tds_client_debug.h"
#include "gatt_tds_client.h"
#include "gatt_tds_client_uuid.h"
#include "gatt_tds_client_init.h"


/****************************************************************************/
void handleDiscoverAllTdsCharacteristicsResp(GTDSC *gattTdsClient,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_TDS_CLIENT_INFO("GTDS: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            switch (cfm->uuid[0])
            {
                case UUID_TRANSPORT_DISCOVERY_CONTROL_POINT:
                {
                    gattTdsClient->handles.tdsControlPointHandle= cfm->handle;
                    GATT_TDS_CLIENT_INFO("GTDS: Transport Disc Control Point: Handle = (%x)\n ", cfm->handle);
                }
                break;

                case UUID_TRANSPORT_DISCOVERY_BREDR_HANDOVER_DATA:
                {
                    gattTdsClient->handles.bredrHandoverDataHandle = cfm->handle;
                    GATT_TDS_CLIENT_INFO("GTDS: Transport Discovery BREDR Handover data cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case UUID_TRANSPORT_DISCOVERY_SIG_DATA:
                {
                    gattTdsClient->handles.bluetoothSigDataHandle = cfm->handle;
                    GATT_TDS_CLIENT_INFO("GTDS: Transport disc SIG data : Handle = (%x)\n ", cfm->handle);
                }
                break;

                case UUID_TRANSPORT_DISCOVERY_BREDR_TRANSPORT_BLOCK_DATA:
                {
                    gattTdsClient->handles.CompleteTransportBlockHandle = cfm->handle;
                    GATT_TDS_CLIENT_INFO("Transport Block Data : Handle = (%x)\n ", cfm->handle);
                }
                break;
                default:
                    break;
            }
        }

        if (!cfm->more_to_come)
        {
            /* all charateristics are optional */
            discoverAllTdsCharacteristicDescriptors(gattTdsClient);
        }
    }
    else
    {
        gattTdsClientSendInitCfm(gattTdsClient, GATT_TDS_CLIENT_STATUS_DISCOVERY_ERR);
    }

}

/****************************************************************************/
void discoverAllTdsCharacteristicDescriptors(GTDSC *gattTdsClient)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gattTdsClient->srvcElem->gattId,
                                                 gattTdsClient->srvcElem->cid,
                                                 gattTdsClient->handles.startHandle + 1,
                                                 gattTdsClient->handles.endHandle);
}

/****************************************************************************/
void handleDiscoverAllTdsCharacteristicDescriptorsResp(GTDSC *gattTdsClient,
            const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_TDS_CLIENT_INFO("GTDSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            switch (cfm->uuid[0])
            {
                case UUID_TRANSPORT_DISCOVERY_CONTROL_POINT:
                {
                    gattTdsClient->pendingHandle = cfm->handle;
                }
                break;

                case CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC:
                {
                    if (gattTdsClient->pendingHandle == gattTdsClient->handles.tdsControlPointHandle)
                    {
                        gattTdsClient->handles.tdsControlPointCCCDHandle = cfm->handle;
                        gattTdsClient->pendingHandle = 0;
                        GATT_TDS_CLIENT_INFO("TDS: tdsControlPointCCCDHandle CCD : Handle = (%x)\n", cfm->handle);
                    }
                }
                break;

                default:
                    break;
            }
        }
    }

    if (!cfm->more_to_come)
    {
        /* One of the Mandatory MCS characteristic's CCCD is not found, initialisation complete */
        if (!gattTdsClient->handles.tdsControlPointCCCDHandle)
        {
            gattTdsClientSendInitCfm(gattTdsClient, GATT_TDS_CLIENT_STATUS_DISCOVERY_ERR);
        }
        else
        {
            gattTdsClientSendInitCfm(gattTdsClient, GATT_TDS_CLIENT_STATUS_SUCCESS);
        }
    }
}

