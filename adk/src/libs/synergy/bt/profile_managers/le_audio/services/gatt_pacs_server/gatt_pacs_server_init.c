/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"

PacsServiceHandleType pacs_service_handle;

/****************************************************************************/
PacsServiceHandleType GattPacsServerInit(AppTask appTask, uint16 startHandle, uint16 endHandle)
{
     GPACSS_T *pacs_server = NULL;
     PacsServiceHandleType srvc_hndl = 0;
     uint8 i;

    /* validate the input parameters */
    if (appTask == CSR_SCHED_QID_INVALID)
    {
        GATT_PACS_SERVER_PANIC(
                    "PACS: Invalid Initialization parameters!"
                    );
    }

    pacs_service_handle = ServiceHandleNewInstance((void **) &pacs_server, sizeof(GPACSS_T));

    if (pacs_server)
    {
        /* Reset all the service library memory */
        memset(pacs_server, 0, sizeof(GPACSS_T));

        /* Set up library handler for external messages */
        pacs_server->lib_task = CSR_BT_PACS_SERVER_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        pacs_server->app_task = appTask;

        /* Save the service handle */
        pacs_server->srvc_hndl = pacs_service_handle;

        /* Initiliasation of the PACS Server Charateristics */
        pacs_server->data.sink_pack_record = NULL;
        pacs_server->data.source_pack_record = NULL;
 
		/* application will pass the actual audio location using GattPacsServerAddAudioLocation ()*/ 
        pacs_server->data.sink_audio_source_location = 0;
        pacs_server->data.source_audio_source_location = 0;

        pacs_server->data.supported_audio_contexts = PACS_CONTEXT_TYPE_PROHIBITED;
        pacs_server->data.available_audio_contexts = PACS_CONTEXT_TYPE_PROHIBITED;
        pacs_server->data.pacs_record_handle_mask = DEFAULT_PAC_RECORD_HANDLE_MASK;
        pacs_server->data.audioContextAvailabiltyControlApp = FALSE;

        memset(pacs_server->data.connected_clients, 0, (sizeof(pacs_client_data) * GATT_PACS_MAX_CONNECTIONS));

        /* Reset the client config value to default GATT_PACS_SERVER_INVALID_CLIENT_CONFIG */
        for(i = 0; i < GATT_PACS_MAX_CONNECTIONS; i++)
        {
           pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg1 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg2 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sinkPacClientCfg3 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg1 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg2 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sourcePacClientCfg3 = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sinkAudioLocationsClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.sourceAudioLocationsClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.availableAudioContextsClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.supportedAudioContextsClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.vsAptXSinkPacClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
           pacs_server->data.connected_clients[i].client_cfg.vsAptXSourcePacClientCfg = GATT_PACS_SERVER_INVALID_CLIENT_CONFIG;
        }

        /* Setup data required for Published Audio Capabilities Service
         * to be registered with the GATT
         */
        pacs_server->start_handle = startHandle;
        pacs_server->end_handle = endHandle;

        /* Register with the GATT  and verify the result */
        pacs_server->gattId = CsrBtGattRegister(pacs_server->lib_task);

        if (pacs_server->gattId != CSR_BT_GATT_INVALID_GATT_ID)
        {
            CsrBtGattFlatDbRegisterHandleRangeReqSend(pacs_server->gattId,
                                                     startHandle,
                                                     endHandle);
            return pacs_service_handle;
        }
        else
        {
            /* If the registration with GATT Manager fails and we have allocated memory
             * for the new instance successfully (service handle not zero), we have to free
             * the memory of that instance.
             */
            if (srvc_hndl)
            {
                ServiceHandleFreeInstanceData(srvc_hndl);
            }
            return 0;
        }
    }
    else
    {
        GATT_PACS_SERVER_PANIC("Memory alllocation of PACS Server instance failed!\n");
        return 0;
    }
}

void gatt_pacs_server_init(void** gash)
{
    *gash = &pacs_service_handle;
    GATT_PACS_SERVER_INFO("Pacs: gatt_pacs_server_init\n\n");

}

#ifdef ENABLE_SHUTDOWN
void gatt_pacs_server_deinit(void** gash)
{
    ServiceHandle serviceHandle = *((ServiceHandle*)*gash);

    if (serviceHandle)
    {
        if(ServiceHandleFreeInstanceData(serviceHandle))
        {
            GATT_PACS_SERVER_INFO("PACS: gatt_pacs_server_deinit\n\n");
        }
        else
        {
            GATT_PACS_SERVER_PANIC("PACS: deinit - Unable to free PACS server instance.\n");
        }
    }
    else
    {
        GATT_PACS_SERVER_INFO("PACS: deinit - Invalid Service Handle\n\n");
    }
}
#endif
