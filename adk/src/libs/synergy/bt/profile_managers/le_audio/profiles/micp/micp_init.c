/****************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

%%version
*/

#include "gatt_mics_client.h"
#include "gatt_aics_client.h"

#include "micp.h"
#include "micp_debug.h"
#include "micp_init.h"
#include "micp_common.h"
#include "gatt_service_discovery_lib.h"

micp_main_inst *micp_main;

/******************************************************************************/
void micpSendInitCfm(MICP * micp_inst, MicpStatus status)
{
    if(micp_inst)
    {
        MAKE_MICP_MESSAGE(MicpInitCfm);

        message->id = MICP_INIT_CFM;
        message->status = status;
        message->prflHndl = micp_inst->micp_srvc_hdl;
        message->aicsNum = micp_inst->aics_num;
        message->cid = micp_inst->cid;

        MicpMessageSend(micp_inst->app_task, message);

    }
    else
    {
        MICP_PANIC("Invalid MICP Profile instance\n");
    }
}

/****************************************************************************/
static void micpMicsInitReq(MICP *micp_inst,
                          const MicpInitData *client_init_params,
                          const MicpHandles *device_data)
{
    GattMicsClientInitData mics_init_data;
    GattMicsClientDeviceData mics_device_data;

    mics_init_data.cid = client_init_params->cid;
    mics_init_data.startHandle = device_data->micsHandle.startHandle;
    mics_init_data.endHandle = device_data->micsHandle.endHandle;

    mics_device_data.muteHandle = device_data->micsHandle.muteHandle;
    mics_device_data.muteCccHandle = device_data->micsHandle.muteCccHandle;

    GattMicsClientInitReq(micp_inst->lib_task, &mics_init_data, &mics_device_data);
}

static void InitProfileHandleList(CsrCmnListElm_t *elem)
{
    /* Initialize a CsrBtAseCharacElement. This function is called every
     * time a new entry is made on the queue list */
    ProfileHandleListElm_t *cElem = (ProfileHandleListElm_t *) elem;

    cElem->profile_handle = 0;
}

void micp_init(void **gash)
{
    micp_main = CsrPmemZalloc(sizeof(*micp_main));
    *gash = micp_main;

    CsrCmnListInit(&micp_main->profile_handle_list, 0, InitProfileHandleList, NULL);
}

/***************************************************************************/
void MicpInitReq(AppTask appTask,
                const MicpInitData *clientInitParams,
                const MicpHandles *deviceData,
                bool includedServices)
{
    MICP *micp_inst = NULL;
    ProfileHandleListElm_t *elem = NULL;
    MicpProfileHandle profile_hndl = 0;
    uint8 i;

    if (appTask == CSR_SCHED_QID_INVALID)
    {
        MICP_PANIC("Application Task NULL\n");
    }

    elem = MICP_ADD_SERVICE_HANDLE(micp_main->profile_handle_list);
    profile_hndl = ADD_MICP_CLIENT_INST(micp_inst);
    elem->profile_handle = profile_hndl;

    if (micp_inst)
    {
        /* Reset all the service library memory */
        CsrMemSet(micp_inst, 0, sizeof(MICP));

        /* Set up library handler for external messages */
        micp_inst->lib_task = CSR_BT_MICP_IFACEQUEUE;

        /* Store the Task function parameter.
         * All library messages need to be sent here */
        micp_inst->app_task = appTask;

        micp_inst->cid = clientInitParams->cid;

        micp_inst->micp_srvc_hdl = profile_hndl;

        micp_inst->secondary_service_req = includedServices;

        micpSendInitCfm(micp_inst, MICP_STATUS_IN_PROGRESS);

        if(deviceData)
        {
            /* It's a peer device: we already know handles, no need to do discovery */
            micp_inst->is_peer_device = TRUE;

            micp_inst->start_handle = deviceData->micsHandle.startHandle;
            micp_inst->end_handle = deviceData->micsHandle.endHandle;

            micp_inst->aics_num = deviceData->aicsHandleLength;

            /* Check if the application requested the MICP Secondary service,
             * if so after the initialisation of the MICS Client
             * the AICS Clients must also be initialised.
             */
            if (deviceData->aicsHandle)
            {
                /* Same thing for the AICS instances*/
                micp_inst->aics_counter = deviceData->aicsHandleLength;

                for (i=0; i<deviceData->aicsHandleLength; i++)
                {
                    micpAddAicsInst(micp_inst,
                                   &(deviceData->aicsHandle[i].handles),
                                   (deviceData->aicsHandle[i].startHandle),
                                   (deviceData->aicsHandle[i].endHandle));
                }
            }

            /* We can start now the initialisation of all the necessary client:
             * we start with the MICS Client.
             */
            micpMicsInitReq(micp_inst, clientInitParams, deviceData);
        }
        else
        {
            GattSdSrvcId srvcIds = GATT_SD_MICS_SRVC;
            /* Find handle value range for the MICP from GATT SD */
            GattServiceDiscoveryFindServiceRange(CSR_BT_MICP_IFACEQUEUE, micp_inst->cid, srvcIds);
        }
    }
    else
    {
        MICP_PANIC("Memory alllocation of MICP Profile instance failed!\n");
    }
}

/****************************************************************************/
void micpHandleAicsClientInitResp(MICP *micp_inst,
                                 const GattAicsClientInitCfm *message)
{
    if(micp_inst)
    {
        if(message->status == GATT_AICS_CLIENT_STATUS_SUCCESS)
        {
            /* The initialisation of a AICS client instance is done */
            micp_inst->aics_counter -= 1;

            /* We have to save the service handle of the client instance in a list, because when
             * we finish all the MICP clients initialisation we have to send this list to the application.
             * The application need to know there are more instance in order to handle them in all the
             * MICP procedure it wants to perform.
             */
            micpAddElementAicsSrvcHndlList(micp_inst, message->srvcHndl);

            if (!micp_inst->aics_counter)
            {
                /* There are no more other AICS instance to initialise:
                 * - destroy the instance lists
                 * - send MICP_PROFILE_INT_CFM
                 * - destroy the service handles lists
                 */
                MicpDestroyReqAllInstList(micp_inst);

                micpSendInitCfm(micp_inst, MICP_STATUS_SUCCESS);
            }
        }
        else
        {
            /* One of the AICS Client Initialisation failed. */
            /* We have to destroy all the list we have. */
            MicpDestroyReqAllInstList(micp_inst);
            micpDestroyReqAllSrvcHndlList(micp_inst);

            micpSendInitCfm(micp_inst, MICP_STATUS_FAILED);
        }
    }
    else
    {
        MICP_PANIC("Invalid MICP Profile instance\n");
    }
}

/****************************************************************************/
void micpHandleMicsClientInitResp(MICP *micp_inst,
                                const GattMicsClientInitCfm *message)
{
    if(micp_inst)
    {
        if(message->status == GATT_MICS_CLIENT_STATUS_SUCCESS)
        {
            micp_inst->mics_srvc_hdl = message->srvcHndl;

            if (micp_inst->secondary_service_req && !micp_inst->is_peer_device)
            {
                /* ToDo - Use gatt api to discover
                 GattFindIncludedServicesRequest(&micp_inst->lib_task,
                                            micp_inst->cid,
                                            micp_inst->start_handle,
                                            micp_inst->end_handle);
                                            */
            }
            else if (!micp_inst->secondary_service_req)
            {
                /* The applicantion didn't request the discovery of the secondary service:
                 * we can send the confirmation of the initialisation
                 */
                micpSendInitCfm(micp_inst, MICP_STATUS_SUCCESS);
            }
            else if (micp_inst->is_peer_device)
            {
                /* It's a pair device and the application should have already provided
                 * the info to initialise all the AICS Clients. We can start the
                 * initialisations.
                 */
                micpStartScndrSrvcInit(micp_inst);
            }
        }
        else
        {
            /* The initialisation of MICS Client failed:
             * we need to destroy all the existed instance lists.*/
            MicpDestroyReqAllInstList(micp_inst);

            micpSendInitCfm(micp_inst, MICP_STATUS_FAILED);
        }
    }
    else
    {
        MICP_PANIC("Invalid MICP Profile instance\n");
    }
}

/***************************************************************************/
void micpStartScndrSrvcInit(MICP *micp_inst)
{
    if(micp_inst->aics_counter)
    {
        micp_aics_inst_t *ptr = micp_inst->first_aics_inst;

        while(ptr)
        {
            GattAicsClientInitData init_data;

            init_data.cid = ptr->cid;
            init_data.startHandle = ptr->start_handle;
            init_data.endHandle = ptr->end_handle;

            if (micp_inst->is_peer_device)
            {
                GattAicsClientInitReq(micp_inst->lib_task,
                                      &init_data,
                                      &(ptr->device_data));
            }
            else
            {
                GattAicsClientInitReq(micp_inst->lib_task,
                                      &init_data,
                                      NULL);
            }

            ptr = ptr->next;
        }
    }
    else
    {
        /* No secondary services in the remote device */
        micpSendInitCfm(micp_inst, MICP_STATUS_SUCCESS);
    }
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void micp_deinit(void **gash)
{
    CsrCmnListDeinit(&micp_main->profile_handle_list);
    CsrPmemFree(micp_main);
}
#endif
