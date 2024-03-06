/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_bap
    \brief      Implementations for the LE BAP Unicast server interfaces.
*/

#include "unicast_server_role.h"
#include "unicast_server_role_advertising.h"
#include "pacs_utilities.h"

#include <gatt_handler_db_if.h>
#include <logging.h>

#ifdef USE_SYNERGY
#include "cm_lib.h"
#include "bap_server_lib.h"
#else
#include "bap_server.h"
#endif
#include "le_bap_profile.h"

#define UNICAST_SERVER_ROLE_LOG     DEBUG_LOG

/*! Defines for configure data path request vendor specific Types */
#define CONFIG_DATA_PATH_VS_LENGTH      7
#define CONNECTION_HANDLE_TYPE          0x06
#define CONNECTION_HANDLE_TYPE_LENGTH   3

#define RELAY_INFO_TYPE                 0x0c
#define RELAY_INFO_TYPE_LENGTH          2

#define RELAY_INFO_FLAG_FOR_RELAY       0x01
#define RELAY_INFO_FLAG_FOR_HANDOVER    0x02

ServiceHandle bapUnicastServiceHandle = 0;

void LeBapUnicastServer_Init(uint8 number_of_ases, Task bap_handler)
{
    BapServerHandleRange pacsServiceHandle = {HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE,
                                              HANDLE_PUBLISHED_AUDIO_CAPABILITIES_SERVICE_END};
    BapServerHandleRange ascsServiceHandle = {HANDLE_ASCS_SERVICE, HANDLE_ASCS_SERVICE_END};

    PanicNull((void *) bap_handler);

#ifdef USE_SYNERGY
    bapUnicastServiceHandle = BapServerUnicastInit(TrapToOxygenTask(bap_handler),
                               number_of_ases, &pacsServiceHandle, &ascsServiceHandle);
    UNICAST_SERVER_ROLE_LOG("LeBapUnicastServer_Init bapUnicastServiceHandle=0x%x", bapUnicastServiceHandle);
#else
    bapUnicastServiceHandle = BapServerUnicastInit(bap_handler, number_of_ases,
                                                   &pacsServiceHandle, &ascsServiceHandle);
#endif
    LeBapPacsSetBapHandle(bapUnicastServiceHandle);

    LeBapPacsUtilities_Init();

    LeBapUnicastServer_SetupLeAdvertisingData();
}

#ifdef INCLUDE_CIS_MIRRORING
static void leBapUnicastServer_ConfigureDataPath(Task bap_handler, BapServerSetupDataPathReq *data_path_req)
{
    uint8 vendor_specific_length = CONFIG_DATA_PATH_VS_LENGTH;
    uint8 *data[CM_DM_CONFIGURE_DATA_PATH_MAX_INDEX] = {0};
    uint8* vendor_specific_data = (uint8 *) PanicUnlessMalloc(vendor_specific_length * sizeof(uint8));

    vendor_specific_data[0] = CONNECTION_HANDLE_TYPE_LENGTH;
    vendor_specific_data[1] = CONNECTION_HANDLE_TYPE;
    vendor_specific_data[2] = (uint8)data_path_req->isoHandle;
    vendor_specific_data[3] = (uint8)(data_path_req->isoHandle >> 8);
    vendor_specific_data[4] = RELAY_INFO_TYPE_LENGTH;
    vendor_specific_data[5] = RELAY_INFO_TYPE;
    vendor_specific_data[6] = (RELAY_INFO_FLAG_FOR_RELAY | RELAY_INFO_FLAG_FOR_HANDOVER);

    UNICAST_SERVER_ROLE_LOG("leBapUnicastServer_ConfigureDataPath: cis_handle=0x%x Direction =%u data_path_id=%u",
                            data_path_req->isoHandle, data_path_req->dataPathDirection, data_path_req->dataPathId);

    data[0] = vendor_specific_data;
    CmDmConfigureDataPathReqSend(TrapToOxygenTask(bap_handler),
                                data_path_req->dataPathDirection,
                                data_path_req->dataPathId,
                                vendor_specific_length,
                                &data[0]);

}
#endif

void LeBapUnicastServer_CreateDataPath(Task bap_handler, uint16 cis_handle, bool host_to_controller, bool controller_to_host,
                                       bool is_cis_delegated)
{
    BapServerSetupDataPathReq dataPathParameters = {0};

    dataPathParameters.isoHandle = cis_handle,
    dataPathParameters.dataPathId = is_cis_delegated ? ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL:
                                                       ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;

    UNICAST_SERVER_ROLE_LOG("LeBapUnicastServer_CreateDataPath: cis_handle=0x%x host_to_controller=%u controller_to_host=%u is_cis_delegated=%u",
                            cis_handle, host_to_controller, controller_to_host, is_cis_delegated);

    if (host_to_controller)
    {
        dataPathParameters.dataPathDirection = ISOC_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER;
#ifdef INCLUDE_CIS_MIRRORING
        leBapUnicastServer_ConfigureDataPath(bap_handler, &dataPathParameters);
#else
        UNUSED(bap_handler);
#endif
        BapServerSetupIsoDataPathReq(bapUnicastServiceHandle, BAP_ISO_UNICAST, &dataPathParameters);
    }

    if (controller_to_host)
    {
        dataPathParameters.dataPathDirection = ISOC_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST;
#ifdef INCLUDE_CIS_MIRRORING
        leBapUnicastServer_ConfigureDataPath(bap_handler, &dataPathParameters);
#else
        UNUSED(bap_handler);
#endif
        BapServerSetupIsoDataPathReq(bapUnicastServiceHandle, BAP_ISO_UNICAST, &dataPathParameters);
    }
}

void LeBapUnicastServer_RemoveDataPath(uint16 cis_handle, uint8 direction_mask)
{
    UNICAST_SERVER_ROLE_LOG("LeBapUnicastServer_RemoveDataPath: cis_handle=0x%x direction_mask=0x%02x", cis_handle, direction_mask);

    BapServerRemoveIsoDataPathReq(bapUnicastServiceHandle, cis_handle, BAP_ISO_UNICAST, direction_mask);
}
