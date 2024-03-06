/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"


/****************************************************************************/
ServiceHandleType GattPacsServerInit(Task appTask, uint16 startHandle, uint16 endHandle)
{
     GPACSS_T *pacs_server = NULL;
    /*Registration parameters for PACS library to GATT manager  */
    gatt_manager_server_registration_params_t reg_params;
    ServiceHandle srvc_hndl = 0;

    /* validate the input parameters */
    if (appTask == NULL)
    {
        GATT_PACS_SERVER_PANIC((
                    "PACS: Invalid Initialization parameters!"
                    ));
    }

    srvc_hndl = ServiceHandleNewInstance((void **) &pacs_server, sizeof(GPACSS_T));

    if (pacs_server)
    {
        /* Reset all the service library memory */
        memset(pacs_server, 0, sizeof(GPACSS_T));

        /* Set up library handler for external messages */
        pacs_server->lib_task.handler = pacsServerMsgHandler;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        pacs_server->app_task = appTask;

        /* Save the service handle */
        pacs_server->srvc_hndl = srvc_hndl;

        /* Initiliasation of the PACS Server Charateristics */
        pacs_server->data.sink_pack_record = NULL;
        pacs_server->data.source_pack_record = NULL;
        pacs_server->data.vs_sink_pack_record = NULL;
        pacs_server->data.vs_source_pack_record = NULL;
        
		/* application will pass the actual audio location using GattPacsServerAddAudioLocation ()*/
        pacs_server->data.sink_audio_source_location = 0;
        pacs_server->data.source_audio_source_location = 0;

        pacs_server->data.supported_audio_contexts = PACS_CONTEXT_TYPE_PROHIBITED;
        pacs_server->data.available_audio_contexts = PACS_CONTEXT_TYPE_PROHIBITED;
        pacs_server->data.pacs_record_handle_mask = DEFAULT_PAC_RECORD_HANDLE_MASK;

        memset(pacs_server->data.connected_clients, 0, (sizeof(pacs_client_data) * GATT_PACS_MAX_CONNECTIONS));

        /* Setup data required for Published Audio Capabilities Service
         * to be registered with the GATT Manager
         */
        reg_params.start_handle = startHandle;
        reg_params.end_handle = endHandle;
        reg_params.task = &pacs_server->lib_task;

        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterServer(&reg_params) == gatt_manager_status_success)
        {
            return srvc_hndl;
        }
        else
        {
            /* If the registration with GATT Manager fails and we have allocated memory
             * for the new instance successfully (service handle not zero), we have to free
             * the memnory of that instance.
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
        GATT_PACS_SERVER_DEBUG_PANIC(("Memory alllocation of PACS Server instance failed!\n"));
        return 0;
    }
}

