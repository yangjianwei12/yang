/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_service_discovery_init.h"
#include "gatt_service_discovery_handler.h"
#include "gatt_service_discovery_debug.h"
#include "csr_bt_gatt_lib.h"

/* GATT Service Discovery UUID Info List size */
#define GATT_SD_SRVC_UUID_INFO_LIST_SIZE  (0x16)

/* List of GATT Service UUIDs that could be discovered by Gatt Service module
 *
 * Based on gatt_uuid_type_t value, it will decides 16 bit or 128 bit UUID value 
 * used in UUID based Primary Service Discovery for the GATT Service
 *
 */
GattSdSrvcUuidInfo_t   GattSdSrvcUuidInfo[GATT_SD_SRVC_UUID_INFO_LIST_SIZE] = {
    {GATT_SD_GATT_SRVC, {gatt_uuid16, {0x00001801u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_GAP_SRVC,  {gatt_uuid16, {0x00001800u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_CSIS_SRVC, {gatt_uuid16, {0x00001846u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_PACS_SRVC, {gatt_uuid16, {0x00001850u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_ASCS_SRVC, {gatt_uuid16, {0x0000184Eu, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_VCS_SRVC,  {gatt_uuid16, {0x00001844u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_GMCS_SRVC, {gatt_uuid16, {0x00001849u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_BASS_SRVC, {gatt_uuid16, {0x0000184Fu, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_WMAP_SRVC, {gatt_uuid128, {0x2587DB3Cu, 0xCE704FC9u, 0x935F777Au, 0xB4188FD7u}}},
    {GATT_SD_TBS_SRVC,  {gatt_uuid16, {0x0000184Cu, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_TDS_SRVC,  {gatt_uuid16, {0x00001824u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_TMAS_SRVC, {gatt_uuid16, {0x00001855u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
	{GATT_SD_CAS_SRVC,  {gatt_uuid16, {0x00001853u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_HID_SRVC,  {gatt_uuid16, {0x00001812u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_DI_SRVC,   {gatt_uuid16, {0x0000180Au, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_BATTERY_SRVC,  {gatt_uuid16, {0x0000180Fu, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
	{GATT_SD_SCAN_PARAM_SRVC,  {gatt_uuid16, {0x00001813u, 0x0001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_AMS_SRVC,  {gatt_uuid128, {0x89D3502Bu, 0x0F36433Au, 0x8EF4C502u, 0xAD55F8DCu}}},
    {GATT_SD_ANCS_SRVC, {gatt_uuid128, {0x7905f431u, 0xb5ce4e99u, 0xa40f4b1eu, 0x122d00d0u}}},
    {GATT_SD_MICS_SRVC, {gatt_uuid16, {0x0000184Du, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_GMAS_SRVC, {gatt_uuid16, {0x00001858u, 0x00001000u, 0x80000080u, 0x5F9B34FBu}}},
    {GATT_SD_QGMAS_SRVC, {gatt_uuid128, {0x12994B7Eu, 0x6D474215u, 0x8C9EAAE9u, 0xA1095BA3u}}}
};

GattSdDeviceElement* gattSdDlAddDevice(GattSdDeviceElement** list)
{
    GattSdDeviceElement* elem = NULL;
    GattSdDeviceElement* temp = *list;
    if (temp == NULL) 
    {
        /* list is empty */
        elem = (GattSdDeviceElement*) GATT_SD_MALLOC(sizeof(GattSdDeviceElement));
        memset(elem, 0, sizeof(GattSdDeviceElement));
        elem->next = NULL;
        *list = elem;
    }
    else 
    {
        /* transverse list to the last element */
        while(temp->next != NULL)
            temp = temp->next;
        elem = (GattSdDeviceElement*) GATT_SD_MALLOC(sizeof(GattSdDeviceElement));
        memset(elem, 0, sizeof(GattSdDeviceElement));
        elem->next = NULL;
        temp->next = elem;
    }
    return elem;
}


void gattSdDlRemoveDevice(GattSdDeviceElement** list,
                                GattSdDeviceElement* elem)
{
    GattSdDeviceElement* temp1 = *list;
    GattSdDeviceElement* temp2 = *list;
    if (temp1 != NULL)
    {
        /* element is at the head of the list */
        if (temp1 == elem)
        {
            *list = temp1->next;
        }
        else
        {
            /* transverse the list to find the element */
            temp2 = temp1->next;
            while (temp2 != NULL)
            {
                if (temp2 == elem) {
                    temp1->next = temp2->next;
                    break;
                }
                else
                {
                    temp1 = temp1->next;
                    temp2 = temp2->next;
                }
            }
        }
    }
}

void gattSdDlCleanup(GattSdDeviceElement* list)
{
    while (list)
    {
        GattSdDeviceElement* devElem = list;
        /* Free Device service list */
        gattSdSlCleanup(devElem->serviceList);
        devElem->serviceList = NULL;
        list = list->next;
        free(devElem);
    }
}


GattSdDeviceElement* gattSdDlFindByConnid(GattSdDeviceElement* list, 
                                connection_id_t cid)
{
    GattSdDeviceElement* temp = list;
    while (temp != NULL) {
        if (temp->cid == cid) {
            break;
        }
        temp = temp->next;
    }
    return temp;
}

GattSdServiceElement* gattSdSlFindBySdsrvcId(GattSdServiceElement* list,
                                          GattSdSrvcId sdSrvcId)
{
    GattSdServiceElement* temp = list;
    while (temp != NULL) {
        if (temp->srvcId == sdSrvcId) {
            break;
        }
        temp = temp->next;
    }
    return temp;
}

GattSdIncludedServiceElement* gattSdIslFindBySdsrvcId(GattSdIncludedServiceElement* list,
                                            GattSdSrvcId sdSrvcId)
{
    GattSdIncludedServiceElement* temp = list;
    while (temp != NULL) {
        if (temp->srvcId == sdSrvcId) {
            break;
        }
        temp = temp->next;
    }
    return temp;
}


bool gattSdSlFindDuplicate(GattSdServiceElement* list, 
                           const GattSdServiceElement elem)
{
    GattSdServiceElement* temp = list;
    while (temp != NULL) 
    {
        if ((temp->srvcId == elem.srvcId) &&
            (temp->startHandle == elem.startHandle) &&
            (temp->endHandle == elem.endHandle) &&
            (temp->serviceType == elem.serviceType))
        {
            return TRUE;
        }
        temp = temp->next;
    }
    return FALSE;
}

GattSdServiceElement* gattSdSlAddService(GattSdServiceElement** list)
{
    GattSdServiceElement* elem = NULL;
    GattSdServiceElement* temp = *list;

    /* Check if list is empty */
    if (temp == NULL) 
    {
        /* Alloc new element */
        elem = (GattSdServiceElement*) GATT_SD_MALLOC(sizeof(GattSdServiceElement));
        memset(elem, 0, sizeof(GattSdServiceElement));
        elem->next = NULL;
        /* Set new element as head of the list */
        *list = elem;
    }
    else 
    {
        /* Transverse thorugh list to the last element */
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        /* Alloc memory for the new element */
        elem = (GattSdServiceElement*) GATT_SD_MALLOC(sizeof(GattSdServiceElement));
        memset(elem, 0, sizeof(GattSdServiceElement));
        elem->next = NULL;
        /* Append new element to the end of the list */
        temp->next = elem;
    }
    return elem;
}

GattSdIncludedServiceElement* gattSdIncLSrvAddService(GattSdIncludedServiceElement** list)
{
    GattSdIncludedServiceElement* elem = NULL;
    GattSdIncludedServiceElement* temp = *list;

    /* Check if list is empty */
    if (temp == NULL)
    {
        /* Alloc new element */
        elem = (GattSdIncludedServiceElement*)GATT_SD_MALLOC(sizeof(GattSdIncludedServiceElement));
        memset(elem, 0, sizeof(GattSdIncludedServiceElement));
        elem->next = NULL;
        /* Set new element as head of the list */
        *list = elem;
    }
    else
    {
        /* Transverse thorugh list to the last element */
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        /* Alloc memory for the new element */
        elem = (GattSdIncludedServiceElement*)GATT_SD_MALLOC(sizeof(GattSdIncludedServiceElement));
        memset(elem, 0, sizeof(GattSdIncludedServiceElement));
        elem->next = NULL;
        /* Append new element to the end of the list */
        temp->next = elem;
    }
    return elem;
}

void gattSdSlCleanup(GattSdServiceElement* list)
{
    while (list)
    {
        GattSdServiceElement* srvcElem = list;

        GATT_SD_ISL_CLEANUP(srvcElem->includedSrvcList);
        srvcElem->includedSrvcList = NULL;
        list = list->next;
        free(srvcElem);
    }
}

void gattSdIslCleanup(GattSdIncludedServiceElement* list)
{
    while (list)
    {
        GattSdIncludedServiceElement* srvcElem = list;
        list = list->next;
        free(srvcElem);
    }
}


void gattServiceDiscoveryRegisterSupportedServicesCfm(AppTask task,
                                  GattSdResult result)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM);

    message->result = result;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_REGISTER_SUPPORTED_SERVICES_CFM, message);
}


void gattServiceDiscoveryStartCfm(AppTask task,
                          GattSdResult result,
                          connection_id_t cid)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_START_CFM);

    message->result = result;
    message->cid = cid;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_START_CFM, message);
}

void gattServiceDiscoveryStopCfm(AppTask task,
                          GattSdResult result,
                          connection_id_t cid)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_STOP_CFM);

    message->result = result;
    message->cid = cid;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_STOP_CFM, message);
}

void gattServiceDiscoveryGetDeviceConfigCfm(AppTask task,
                                  GattSdResult result,
                                  connection_id_t cid,
                                  uint16 srvcInfoCount,
                                  GattSdSrvcInfo_t *srvcInfo)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_GET_DEVICE_CONFIG_CFM);

    message->result = result;
    message->cid = cid;
    message->srvcInfo = srvcInfo;
    message->srvcInfoCount = srvcInfoCount;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_GET_DEVICE_CONFIG_CFM, message);
}

void gattServiceDiscoveryAddDeviceConfigCfm(AppTask task,
                                  GattSdResult result)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_ADD_DEVICE_CONFIG_CFM);

    message->result = result;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_ADD_DEVICE_CONFIG_CFM, message);
}

void gattServiceDiscoveryRemoveDeviceConfigCfm(AppTask task,
                                  GattSdResult result,
                                  connection_id_t cid)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_REMOVE_DEVICE_CONFIG_CFM);

    message->result = result;
    message->cid = cid;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_REMOVE_DEVICE_CONFIG_CFM, message);
}

void gattServiceDiscoveryFindServiceRangeCfm(AppTask task,
                                  GattSdResult result,
                                  connection_id_t cid,
                                  uint16 srvcInfoCount,
                                  GattSdSrvcInfo_t *srvcInfo)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM);

    message->result = result;
    message->cid = cid;
    message->srvcInfoCount = srvcInfoCount;
    message->srvcInfo = srvcInfo;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM, message);
}

void gattServiceDiscoveryFindInclServiceRangeCfm(AppTask task,
                                          GattSdResult result,
                                          connection_id_t cid,
                                          GattSdSrvcId    srvcId,
                                          uint16     srvcInfoCount,
                                          GattSdSrvcInfo_t* srvcInfo)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM);

    message->result = result;
    message->cid = cid;
    message->srvcInfoCount = srvcInfoCount;
    message->srvcInfo = srvcInfo;
    message->primarySrvcId = srvcId;

    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_FIND_INCL_SRVC_RANGE_CFM, message);
}

void gattServiceDiscoveryFindInclSrvcStartCfm(AppTask task,
                                          GattSdResult result,
                                          connection_id_t cid)
{
    MAKE_GATT_SD_MESSAGE(GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM);

    message->result = result;
    message->cid = cid;
    GATT_SD_MESSAGE_SEND(task, GATT_SERVICE_DISCOVERY_INCL_SRVC_START_CFM, message);
}

void gattSdAddDevice(GGSD *gatt_sd,
                                   connection_id_t cid)
{
    GattSdDeviceElement *devElem = 
        GATT_SD_DL_FIND_BY_CONNID(gatt_sd->deviceList, cid);
    if (!devElem)
    {
        devElem = GATT_SD_DL_ADD_DEVICE(&gatt_sd->deviceList);
        devElem->cid = cid;
        devElem->serviceListCount = 0;
    }
}

void gattSdAddService(GattSdDeviceElement *devElem,
                                   GattSdSrvcId srvcId,
                                   uint16 startHandle,
                                   uint16 endHandle,
                                   GattSdServiceType serviceType)
{
    GattSdServiceElement* srvcElem = NULL;

    /* Look for duplicate element in service list */
    GattSdServiceElement elem;

    elem.srvcId = srvcId;
    elem.startHandle = startHandle;
    elem.endHandle = endHandle;
    elem.serviceType = serviceType;

    /* Look for duplicate element in service list */
    if (GATT_SD_SL_FIND_DUPLICATE(devElem->serviceList, elem) == FALSE)
    {
        srvcElem = GATT_SD_SL_ADD_SRVC(&devElem->serviceList);
        srvcElem->srvcId = srvcId;
        srvcElem->startHandle = startHandle;
        srvcElem->endHandle = endHandle;
        srvcElem->serviceType = serviceType;
        srvcElem->includedSrvcList = NULL;
        srvcElem->includedSrvcListCount = 0;
    }
    else
    {
        GATT_SD_DEBUG_INFO(("Duplicate Element SrvcId 0x%x, SH 0x%x EH 0x%x, SrvcType 0x%x\n",
                        srvcId, startHandle, endHandle, serviceType));
    }
}

GattSdSrvcId gattSdGetSrvcIdFromUuid128(const uint32* uuid)
{
    GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
    uint8 index;

    for (index = 0; index < GATT_SD_SRVC_UUID_INFO_LIST_SIZE; index++)
    {
        if ((uuid[0] == GattSdSrvcUuidInfo[index].srvcUuid.uuid[0]) &&
            (uuid[1] == GattSdSrvcUuidInfo[index].srvcUuid.uuid[1]) &&
            (uuid[2] == GattSdSrvcUuidInfo[index].srvcUuid.uuid[2]) &&
            (uuid[3] == GattSdSrvcUuidInfo[index].srvcUuid.uuid[3]))
        {
            return GattSdSrvcUuidInfo[index].srvcId;
        }
    }
    return srvcId;
}

GattSdSrvcId gattSdGetSrvcIdFromUuid32(uint32 uuid)
{
    GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
    uint8 index;

    for (index = 0; index < GATT_SD_SRVC_UUID_INFO_LIST_SIZE; index++)
    {
        if (uuid == GattSdSrvcUuidInfo[index].srvcUuid.uuid[0])
        {
            return GattSdSrvcUuidInfo[index].srvcId;
        }
    }
    return srvcId;
}

GattSdSrvcId gattSdGetSrvcIdFromUuid16(uint16 uuid)
{
    GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
    uint8 index;

    for (index = 0; index < GATT_SD_SRVC_UUID_INFO_LIST_SIZE; index++)
    {
        uint16 srvcUuid = (uint16) GattSdSrvcUuidInfo[index].srvcUuid.uuid[0];
        if (srvcUuid == uuid)
        {
            return GattSdSrvcUuidInfo[index].srvcId;
        }
    }
    return srvcId;
}

static uint8 gattSdGetSetBitIndex(GattSdSrvcId srvcId)
{
    uint8 i = 0;
    while(srvcId)
    {
        srvcId = srvcId >> 1;
        i++;
    }
    return i;
}

static GattSdSrvcId gattSdGetNextSrvcUuid(const GGSD *gatt_sd,
                                                    GattSdSrvcUuid *uuid)
{
    GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
    /* Get the current set bit index */
    uint8 idx = gattSdGetSetBitIndex(gatt_sd->curSrvcId);

    /* Look for next service id to trigger the discovery */
    for(; idx < GATT_SD_SRVC_UUID_INFO_LIST_SIZE; idx++)
    {
        if (GattSdSrvcUuidInfo[idx].srvcId & gatt_sd->srvcIds)
        {
            break;
        }
    }

    /* Update the service id and service uuid */
    if (idx < GATT_SD_SRVC_UUID_INFO_LIST_SIZE)
    {
        srvcId = GattSdSrvcUuidInfo[idx].srvcId;
        uuid->type = GattSdSrvcUuidInfo[idx].srvcUuid.type;
        memcpy(uuid->uuid, GattSdSrvcUuidInfo[idx].srvcUuid.uuid, GATT_SD_UUID_SIZE);
    }

    return srvcId;
}

bool gattSdDiscoverPrimaryServiceByUuid(GGSD *gatt_sd)
{
    bool status = FALSE;
    GattSdSrvcUuid uuid;

    /* Get the Next Service uuid */
    gatt_sd->curSrvcId = gattSdGetNextSrvcUuid(gatt_sd, (GattSdSrvcUuid *)&uuid);
    GATT_SD_DEBUG_INFO(("Current  Service Id : 0x%x \n", gatt_sd->curSrvcId));

    if (gatt_sd->curSrvcId)
    {
        if (uuid.type == gatt_uuid16)
        {
            uint16 uuid16 = GATT_SD_UUID_GET_16(uuid.uuid);
            CsrBtGattDiscoverPrimaryServicesBy16BitUuidReqSend(gatt_sd->gattId,
                                                           gatt_sd->curCid,
                                                           uuid16);
        }
        else if ((uuid.type == gatt_uuid32) || (uuid.type == gatt_uuid128))
        {
            uint32 uuidValue[4];
            /* Change the UUID formate as required by GATT layer */
            GATT_SD_UUID_LITTLE_ENDIAN_FORMAT(uuid.uuid, uuidValue);
            CsrBtGattDiscoverPrimaryServicesBy128BitUuidReqSend(gatt_sd->gattId,
                                                gatt_sd->curCid,
                                                (uint8 *)uuidValue);
        }
        /* Update Gatt Service discovery state */
        gatt_sd->state = GATT_SRVC_DISC_STATE_INPROGRESS;
        status = TRUE;
    }
    return status;
}

static void gattSdHandlePrimaryServiceDiscovery(GGSD* gatt_sd,
                                                connection_id_t cid,
                                                gatt_uuid_type_t uuid_type,
                                                const uint8 *uuid,
                                                uint16 start_handle,
                                                uint16 end_handle)
{
    GattSdDeviceElement *devElem = 
        GATT_SD_DL_FIND_BY_CONNID(gatt_sd->deviceList, cid);

    if (devElem == NULL)
    {
        /* If device element is not present, then create the device 
         * element and add to the GATT SD device list.
         * Also, set the service list count for the device to 0 */
        devElem = GATT_SD_DL_ADD_DEVICE(&gatt_sd->deviceList);
        devElem->cid = cid;
        devElem->serviceListCount = 0;
    }

    GATT_SD_DEBUG_INFO(("Handle Primary Service Discovery\n"));
    GATT_SD_DEBUG_INFO(("CID 0x%x, SH 0x%x EH 0x%x, UUID_TYPE 0x%x\n",
                        cid, start_handle, end_handle, uuid_type));

    if (devElem)
    {
        GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
        if (uuid_type == gatt_uuid16)
        {
            uint16 uuid16 = GATT_SD_UUID_GET_16(uuid);
            srvcId = gattSdGetSrvcIdFromUuid16(uuid16);
        }
        else if (uuid_type == gatt_uuid128)
        {
            srvcId = gattSdGetSrvcIdFromUuid128((uint32 *)uuid);
        }
        else if (uuid_type == gatt_uuid32)
        {
            uint32 uuid32 = GATT_SD_UUID_GET_32(uuid);
            srvcId = gattSdGetSrvcIdFromUuid32(uuid32);
        }
 
        if (srvcId != GATT_SD_INVALID_SRVC)
        {
            gattSdAddService(devElem, srvcId,
                           start_handle, end_handle,
                           GATT_SD_SERVICE_TYPE_PRIMARY);
            devElem->serviceListCount++;
        }
    }
}

static void gattSdHandleFindInclSrvc(GGSD* gatt_sd,
                                   connection_id_t cid,
                                   gatt_uuid_type_t uuid_type,
                                   const uint8* uuid,
                                   uint16 start_handle,
                                   uint16 end_handle)
{
    GattSdSrvcId srvcId = GATT_SD_INVALID_SRVC;
    GattSdServiceElement* srvcElem = NULL;
    GattSdIncludedServiceElement* inclSrvcElem = NULL;
    GattSdDeviceElement* devElem =
        GATT_SD_DL_FIND_BY_CONNID(gatt_sd->deviceList, cid);

    GATT_SD_DEBUG_INFO(("Handle Finf Included Service Discovery\n"));
    GATT_SD_DEBUG_INFO(("CID 0x%x, SH 0x%x EH 0x%x, UUID_TYPE 0x%x\n",
        cid, start_handle, end_handle, uuid_type));

    if (devElem == NULL)
    {
        /* Device Has to be Present */
        return;

    }
    
    srvcElem = GATT_SD_SL_FIND_BY_SDSRVCID(devElem->serviceList, gatt_sd->curSrvcId);


    if (srvcElem == NULL)
    {
        /* Service needs to be Present */
        return;
    }

    if (uuid_type == gatt_uuid16)
    {
        uint16 uuid16 = GATT_SD_UUID_GET_16(uuid);
        srvcId = gattSdGetSrvcIdFromUuid16(uuid16);
    }
    else if (uuid_type == gatt_uuid128)
    {
        srvcId = gattSdGetSrvcIdFromUuid128((uint32*)uuid);
    }
    else if (uuid_type == gatt_uuid32)
    {
        uint32 uuid32 = GATT_SD_UUID_GET_32(uuid);
        srvcId = gattSdGetSrvcIdFromUuid32(uuid32);
    }

    inclSrvcElem = GATT_SD_ISL_FIND_BY_SRVCID(srvcElem->includedSrvcList, srvcId);

    if (inclSrvcElem)
    {
        /* Include Service already in the Included Service List */
        return;
    }

    inclSrvcElem = GATT_SD_ISL_ADD_SRVC(&srvcElem->includedSrvcList);

    if (inclSrvcElem)
    {
        inclSrvcElem->endHandle = end_handle;
        inclSrvcElem->startHandle = start_handle;
        inclSrvcElem->srvcId = srvcId;
        inclSrvcElem->serviceType = GATT_SD_SERVICE_TYPE_INCLUDED;
        srvcElem->includedSrvcListCount++;
    }
}


static void gattSdInternalMsgHandler(AppTask task, MsgId id, Msg msg)
{
    CSR_UNUSED(task);

    switch (id)
    {
        case GATT_SD_INTERNAL_MSG_DISCOVERY_START:
        {
            gattSdDiscoveryStartInternal((const GATT_SD_INTERNAL_MSG_DISCOVERY_START_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_DISCOVERY_STOP:
        {
            gattSdDiscoveryStopInternal((const GATT_SD_INTERNAL_MSG_DISCOVERY_START_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_GET_DEVICE_CONFIG:
        {
            gattSdGetDeviceConfigInternal((const GATT_SD_INTERNAL_MSG_GET_DEVICE_CONFIG_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_ADD_DEVICE_CONFIG:
        {
            gattSdAddDeviceConfigInternal((const GATT_SD_INTERNAL_MSG_ADD_DEVICE_CONFIG_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_REMOVE_DEVICE_CONFIG:
        {
            gattSdRemoveDeviceConfigInternal((const GATT_SD_INTERNAL_MSG_REMOVE_DEVICE_CONFIG_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_FIND_SERVICE_RANGE:
        {
            gattSdFindServiceRangeInternal((const GATT_SD_INTERNAL_MSG_FIND_SERVICE_RANGE_T*) msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_FIND_INCL_SRVC_START:
        {
            gattSdInclSrvcDiscoveryStartInternal((const GATT_SD_INTERNAL_MSG_FIND_INCL_SRVC_START_T*)msg);
        }
        break;
        case GATT_SD_INTERNAL_MSG_FIND_INCL_SRVC_RANGE:
        {
            gattSdFindInclSrvcRangeInternal((const GATT_SD_INTERNAL_MSG_FIND_INCL_SRVC_RANGE_T*)msg);
        }
        break;
        default:
        {
            /* Internal unrecognised messages */
            GATT_SD_DEBUG_PANIC(("Gatt SD Internal Msg not handled [0x%x]\n", id));
            break;
        }
    }
}

static void gattSdGetUuidFromBuffer(const uint8* buf, gatt_uuid_type_t uuidType, uint32 *uuid)
{
    if (uuidType == gatt_uuid16)
    {
        uuid[0] = GATT_SD_UUID_GET_16(buf);
    }
    else if (uuidType == gatt_uuid32)
    {
        uuid[0] = GATT_SD_UUID_GET_32(buf);
    }
    else if (uuidType == gatt_uuid128)
    {
        /* Reverse the Service UUID */
        uuid[0] = (uint32) (buf[12] | (buf[13] << 8) | (buf[14] << 16) | (buf[15] << 24));
        uuid[1] = (uint32) (buf[8] | (buf[9] << 8) | (buf[10] << 16) | (buf[11] << 24));
        uuid[2] = (uint32) (buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24));
        uuid[3] = (uint32) (buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
    }
}

static void gattSdGattMsgHandler(AppTask task, MsgId id, Msg msg)
{
    GGSD *gatt_sd = gattServiceDiscoveryGetInstance();
    GATT_SD_UNUSED(task);

    CSR_UNUSED(task);

    switch (id)
    {
        case CSR_BT_GATT_REGISTER_CFM:
        {
            /* GATT Register confirmation received with the gattId */
            CsrBtGattRegisterCfm* cfm = (CsrBtGattRegisterCfm *) msg;
            if(cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
            {
                gatt_sd->gattId = cfm->gattId;
            }
            break;
        }
        case CSR_BT_GATT_DISCOVER_SERVICES_IND:
        {
            /* Primary service discovery indication received */
            CsrBtGattDiscoverServicesInd* ind = 
                    (CsrBtGattDiscoverServicesInd *) msg;
            uint32 uuidValue[4];

            /* Get the service uuid type */
            gatt_uuid_type_t uuidType = GATT_SD_GET_UUID_TYPE(ind->uuid.length);
            /* Get the Service UUID from the buffer */
            gattSdGetUuidFromBuffer(ind->uuid.uuid, uuidType, uuidValue);

            gattSdHandlePrimaryServiceDiscovery(gatt_sd, ind->btConnId,
                    uuidType,(uint8 *)uuidValue, ind->startHandle, ind->endHandle);
            break;
        }
        case CSR_BT_GATT_DISCOVER_SERVICES_CFM:
        {
            /* Primary service discovery confirmation received */
            if (gatt_sd->discoverByUuid)
            {
                if (!gattSdDiscoverPrimaryServiceByUuid(gatt_sd))
                {
                    GATT_SD_DEBUG_INFO(("Primary Service Discovery : Complete\n"));
                    gatt_sd->state = GATT_SRVC_DISC_STATE_IDLE;
                    gattServiceDiscoveryStartCfm(gatt_sd->app_task,
                        GATT_SD_RESULT_SUCCESS, gatt_sd->curCid);
                }
            }
            else
            {
                GATT_SD_DEBUG_INFO(("All Primary Service Discovery : Complete\n"));
                gatt_sd->state = GATT_SRVC_DISC_STATE_IDLE;
                gattServiceDiscoveryStartCfm(gatt_sd->app_task,
                    GATT_SD_RESULT_SUCCESS, gatt_sd->curCid);
            }

            break;
        }
        case CSR_BT_GATT_FIND_INCL_SERVICES_IND:
        {
            CsrBtGattFindInclServicesInd* ind =
                (CsrBtGattFindInclServicesInd*)msg;
            uint32 uuidValue[4];
            /* Get the service uuid type */
            gatt_uuid_type_t uuidType = GATT_SD_GET_UUID_TYPE(ind->uuid.length);

            gattSdGetUuidFromBuffer(ind->uuid.uuid, uuidType, uuidValue);
            gattSdHandleFindInclSrvc(gatt_sd, ind->btConnId, 
                                 uuidType, (uint8*)uuidValue, ind->startHandle,
                                 ind->endGroupHandle);
        }
            break;
        case CSR_BT_GATT_FIND_INCL_SERVICES_CFM:
        {
            CsrBtGattFindInclServicesCfm* cfm =
                (CsrBtGattFindInclServicesCfm*)msg;

            if (cfm->resultCode == CSR_BT_GATT_RESULT_SUCCESS)
            {
                gatt_sd->state = GATT_SRVC_DISC_STATE_IDLE;
                gattServiceDiscoveryFindInclSrvcStartCfm(gatt_sd->findInclSrvcsTask,
                                                GATT_SD_RESULT_SUCCESS, gatt_sd->curCid);
            }
            else
            {
                gatt_sd->state = GATT_SRVC_DISC_STATE_IDLE;
                gattServiceDiscoveryFindInclSrvcStartCfm(gatt_sd->findInclSrvcsTask,
                                               GATT_SD_RESULT_ERROR, gatt_sd->curCid);
            }
            break;
        }
        default:
        {
            /* Internal unrecognised messages */
            GATT_SD_DEBUG_PANIC(("Gatt Msg not handled [0x%x]\n", id));
            break;
        }
    }
}

void GattServiceDiscoveryMsgHandler(void **gash)
{
    uint16 eventClass = 0;
    AppTask task;
    uint16 id;
    void* msg;

    GGSD *inst = (GGSD *) *gash;
    task = inst->app_task;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        id = *(GattSdPrim *)msg;
        switch (eventClass)
        {
            case CSR_BT_GATT_PRIM:
            {
                gattSdGattMsgHandler(task, id, msg);
                break;
            }
            case GATT_SRVC_DISC_PRIM:
            {
                gattSdInternalMsgHandler(task, id, msg);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    SynergyMessageFree(eventClass, msg);
}
