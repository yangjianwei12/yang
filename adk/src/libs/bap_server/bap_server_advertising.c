/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* 
************************************************************************* ***/

#include "bap_server_private.h"
#include "bap_server_debug.h"
#include "bap_server_common.h"
#include "bap_server.h"
#include "bap_server_pacs.h"

#define BAP_UNICAST_SERVER_NUMBER_ADVERT_DATA_ITEMS     1
#define BAP_UNICAST_SERVER_SIZE_ADVERT                  10
#define BAP_UNICAST_SERVER_ANNOUNCEMENT_TYPE_OCTET      4
#define BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT      5
#define BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET     7

#define BLE_AD_TYPE_SERVICE_DATA                0x16
#define AUDIO_STREAM_CONTROL_SERVICE_UUID       0x184E /* UUID_ASCS_SERVICE */

#define BAP_SCAN_DELEGATOR_NUMBER_ADVERT_DATA_ITEMS     1
#define BAP_SCAN_DELEGATOR_SIZE_ADVERT                  4


#define BROADCAST_AUDIO_SCAN_SERVICE_UUID       0x184F /* UUID_BASS_SERVICE */

typedef enum
{
    announcement_type_general,
    announcement_type_targeted
} BapAnnouncementType;

static leAdvDataItem bapUnicastServerAdvert;
static leAdvDataItem bapScanDelegatorAdvert;

static const uint8 bapUnicastServerAdvertData[BAP_UNICAST_SERVER_SIZE_ADVERT] = {
    BAP_UNICAST_SERVER_SIZE_ADVERT - 1,
    BLE_AD_TYPE_SERVICE_DATA,
    /* ASCS UUID */
    AUDIO_STREAM_CONTROL_SERVICE_UUID & 0xFF,
    AUDIO_STREAM_CONTROL_SERVICE_UUID >> 8,
    /* 1 octet for Announcement Type */
    announcement_type_targeted,
    /* 4 octets for Available Audio Contexts, need to be filled in with current availability */
    0,
    0,
    0,
    0,
    /* 1 octet for MetaData Length */
    0
};

static const uint8 bapScanDelegatorAdvertData[BAP_SCAN_DELEGATOR_SIZE_ADVERT] = {
    BAP_SCAN_DELEGATOR_SIZE_ADVERT - 1,
    BLE_AD_TYPE_SERVICE_DATA,
    /* BASS UUID */
    BROADCAST_AUDIO_SCAN_SERVICE_UUID & 0xFF,
    BROADCAST_AUDIO_SCAN_SERVICE_UUID >> 8
};

static BapAnnouncementType bapServerGetAnnouncementType(void)
{
    return announcement_type_general;
}

uint16 BapServerGetNumberOfAdvertisingItems( BapServerRole ServerRole)
{
    if(ServerRole == BAP_SERVER_UNICAST_ROLE)
    {
        BAP_DEBUG_INFO(("BapServerGetNumberOfAdvertisingItems"));

        return BAP_UNICAST_SERVER_NUMBER_ADVERT_DATA_ITEMS;
    }
    else if(ServerRole == BAP_SERVER_BROADCAST_ROLE)
    {
        return BAP_SCAN_DELEGATOR_NUMBER_ADVERT_DATA_ITEMS;
    }

    return 0;
}

leAdvDataItem * BapServerGetAdvertisingData( BapServerRole ServerRole)
{
    uint8 * data = NULL;
    uint16 availability;

    BAP_DEBUG_INFO(("BapServerGetAdvertisingData"));

    if (ServerRole == BAP_SERVER_UNICAST_ROLE)
    {
        data = PanicUnlessMalloc(BAP_UNICAST_SERVER_SIZE_ADVERT);
        memcpy(data, bapUnicastServerAdvertData, BAP_UNICAST_SERVER_SIZE_ADVERT);
        
        data[BAP_UNICAST_SERVER_ANNOUNCEMENT_TYPE_OCTET] = bapServerGetAnnouncementType();

        availability = bapServerPacsUtilitiesGetSinkAudioContextAvailability();
        data[BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT] = availability & 0xFF;
        data[BAP_UNICAST_SERVER_SINK_AVAIL_START_OCTECT+1] = availability >> 8;

        availability = bapServerPacsUtilitiesGetSourceAudioContextAvailability();
        data[BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET] = availability & 0xFF;
        data[BAP_UNICAST_SERVER_SOURCE_AVAIL_START_OCTET+1] = availability >> 8;

        bapUnicastServerAdvert.data = data;
        bapUnicastServerAdvert.size = BAP_UNICAST_SERVER_SIZE_ADVERT;
        
        BAP_DEBUG_INFO(("BapServerGetAdvertisingData size:%d",bapUnicastServerAdvert.size));

        return &bapUnicastServerAdvert;
    }
    else if(ServerRole == BAP_SERVER_BROADCAST_ROLE)
    {
        data = PanicUnlessMalloc(BAP_SCAN_DELEGATOR_SIZE_ADVERT);
        memcpy(data, bapScanDelegatorAdvertData, BAP_SCAN_DELEGATOR_SIZE_ADVERT);

        bapScanDelegatorAdvert.data = data;
        bapScanDelegatorAdvert.size = BAP_SCAN_DELEGATOR_SIZE_ADVERT;

        BAP_DEBUG_INFO(("BapServerGetAdvertisingData size:%d", bapScanDelegatorAdvert.size));

        return &bapScanDelegatorAdvert;
    }
    
    BAP_DEBUG_PANIC(("BapServerGetAdvertisingData Panic\n"));
    return &bapUnicastServerAdvert;
}

void BapServerReleaseAdvertisingItems( BapServerRole ServerRole)
{
    if ( ServerRole == BAP_SERVER_UNICAST_ROLE )
    {
        if (bapUnicastServerAdvert.data)
        {
            free((void *)bapUnicastServerAdvert.data);
            bapUnicastServerAdvert.data = NULL;
        }
    }
    else if ( ServerRole == BAP_SERVER_BROADCAST_ROLE )
    {
        if (bapScanDelegatorAdvert.data)
        {
            free((void *)bapScanDelegatorAdvert.data);
            bapScanDelegatorAdvert.data = NULL;
        }
    }
}

unsigned BapServerGetAdvertisingDataSize(BapServerRole server_role)
{
    unsigned size = 0;
    if(server_role == BAP_SERVER_UNICAST_ROLE)
    {
        size = BAP_UNICAST_SERVER_SIZE_ADVERT;
    }
    else if(server_role == BAP_SERVER_BROADCAST_ROLE)
    {
        size = BAP_SCAN_DELEGATOR_SIZE_ADVERT;
    }
    return size;
}


