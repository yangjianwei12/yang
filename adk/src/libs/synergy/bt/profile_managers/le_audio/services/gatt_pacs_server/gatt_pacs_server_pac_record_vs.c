/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "gatt_pacs_server_debug.h"
#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"
#include "gatt_pacs_server_notify.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_pac_record_vs.h"

#define PACS_COMPANY_ID_UNKNOWN         ((0x0000))
#define PACS_VENDOR_CODEC_ID_UNKNOWN    ((0x0000))
#define MAX_MTU_VALUE                   ((0x40))

#define PAC_RECORD_NUM_FIELD            ((0x01))
#define PAC_RECORD_CODEC_ID_SIZE        ((0x01))
#define PAC_RECORD_COMPANY_ID_SIZE      ((0x02))
#define PAC_RECORD_VENDOR_CODEC_ID_SIZE ((0x02))
#define PAC_RECORD_VS_CONFIG_LEN_SIZE   ((0x01))
#define PAC_RECORD_METADATA_LEN_SIZE    ((0x01))

#define PACS_RECORD_STANDARD_FIELDS_LEN (PAC_RECORD_CODEC_ID_SIZE +   \
                                          PAC_RECORD_COMPANY_ID_SIZE +   \
                                          PAC_RECORD_VENDOR_CODEC_ID_SIZE + \
                                          PAC_RECORD_VS_CONFIG_LEN_SIZE +  \
                                          PAC_RECORD_METADATA_LEN_SIZE\
                                          )

typedef struct pacs_record_t
{
    uint8 len;
    uint8* gen_pacs_record; /* generated pac record */
} gVsPacsRecord_t;

gVsPacsRecord_t vsAptXSinkPacRecord;
gVsPacsRecord_t vsAptXSourcePacRecord;

static void generateVsAptXPacsRecord(bool audioSink, pac_record_list_vs *list);
static uint16 addVsPacRecord(GPACSS_T *pacs_server, pac_record_list_vs ** list, const GattPacsServerVSPacRecord *pac_record);

static bool isVsPacRecordMetadataPresent(const GattPacsServerVSPacRecord *pac_record)
{
    if(pac_record->metadataLength != 0 &&
        (pac_record->metadata[1] == PACS_PREFERRED_AUDIO_CONTEXTS_TYPE ||
        pac_record->metadata[1] == PACS_VENDOR_SPECIFIC_METATDATA_TYPE))
        return TRUE;

    return FALSE;
}

static bool isVsPacRecordPresent(pac_record_list_vs *list, const GattPacsServerVSPacRecord *pac_record)
{
    pac_record_list_vs *iterRecord = list;

    while( iterRecord != NULL)
    {
        if (!memcmp(iterRecord->pac_record, pac_record, sizeof(GattPacsServerVSPacRecord)))
            return TRUE;

        iterRecord = iterRecord->next;
    }

    return FALSE;
}

static uint16 addVsPacRecord(GPACSS_T *pacs_server, pac_record_list_vs **list, const GattPacsServerVSPacRecord *pac_record)
{
    uint16 pac_record_handle;
    pac_record_list_vs *new_record = NULL;

    /* Ensure PAC record has metadata field */
    if(!isVsPacRecordMetadataPresent(pac_record))
        return (uint16)PACS_RECORD_METATDATA_NOT_ADDED;

    /* Check if the pac_record passed already exists */
    if (isVsPacRecordPresent(*list, pac_record))
        return (uint16)PACS_RECORD_ALREADY_ADDED;

    pac_record_handle = generatePacRecordHandle(pacs_server);

    if (pac_record_handle != PACS_RECORD_HANDLES_EXHAUSTED)
    {
        /* Allocate memory for PAC record in the list */
        new_record = (pac_record_list_vs*)CsrPmemZalloc(sizeof(pac_record_list_vs));

        /* Assign the pac_record pointer passed by application */
        new_record->pac_record = pac_record;
        new_record->next = *list;
        new_record->pac_record_handle = pac_record_handle;
        *list = new_record;
    }
    return pac_record_handle;
}

static int getVsPacRecordsCount( pac_record_list_vs *list, uint8 *len)
{
    uint8 numVsPacCount = 0;
    pac_record_list_vs *tempList = list;

    while(tempList != NULL)
    {
        /* Assuming each record in the VS list is unique */
        numVsPacCount += 1;

        *len += (list->pac_record->vsConfigLen +  list->pac_record->metadataLength +
                 PACS_RECORD_STANDARD_FIELDS_LEN ) * sizeof(uint8);

        tempList = tempList->next;
    }

    return numVsPacCount;
}

static void generateVsAptXPacsRecord(bool audioSink, pac_record_list_vs *list)
{
    uint8 len = 0;
    uint8 *genPacsRecord = NULL;

    if (list == NULL)
    {
       len = 1; /* for no pac record */
       genPacsRecord = (uint8*)(CsrPmemZalloc(len));
       memset(genPacsRecord, 0, len);

        /* Write num of PAC records */
        genPacsRecord[0] = 0;
    }
    else
    {
        uint8 index = 0;
        uint8 numVsPacCount;
        pac_record_list_vs *tempList = list;

        /* Identify number of VS PAC records and total length of PAC record
         * in the pac_record_list_vs list */
        numVsPacCount = getVsPacRecordsCount(tempList, &len);

        /* We need to account for space for "num of pac rec",
         * so add PAC_RECORD_NUM_FIELD(=1 octet) in len
         */
        len += PAC_RECORD_NUM_FIELD;
        genPacsRecord = (uint8*)(CsrPmemZalloc(len));
        memset(genPacsRecord, 0, len);

        /* Write num of PAC records */
        genPacsRecord[index++] = numVsPacCount;

        while(tempList != NULL)
        {
            /* Write CodecId */
            genPacsRecord[index++] = tempList->pac_record->codecId;

            /* Write CompanyId */
            genPacsRecord[index++] = (tempList->pac_record->companyId & 0xFF);
            genPacsRecord[index++] = ((tempList->pac_record->companyId >> 8) & 0xFF);

            /* Write Vendor ID */
            genPacsRecord[index++] = (tempList->pac_record->vendorCodecId & 0xFF);
            genPacsRecord[index++] = ((tempList->pac_record->vendorCodecId >> 8) & 0xFF);

            /* Write Vendor Specific Config*/
            genPacsRecord[index++] = tempList->pac_record->vsConfigLen;

            if(tempList->pac_record->vsConfigLen != 0)
            {
                memcpy(&genPacsRecord[index], tempList->pac_record->vsConfig,
                                              tempList->pac_record->vsConfigLen);
            }

            /* Move index by Vendor config len */
            index = index + tempList->pac_record->vsConfigLen; 

            genPacsRecord[index++] = tempList->pac_record->metadataLength;

            if(tempList->pac_record->metadataLength != 0)
            {
                /* Write Metadata */
                memcpy(&genPacsRecord[index], tempList->pac_record->metadata,
                                     tempList->pac_record->metadataLength);
            }
            index = index + tempList->pac_record->metadataLength;

            /* Done with a VS record, move to next VS record in list */
            tempList = tempList->next;
        }
    }

    if (audioSink)
    {
        if (vsAptXSinkPacRecord.gen_pacs_record != NULL)
        {
            pfree(vsAptXSinkPacRecord.gen_pacs_record);
            vsAptXSinkPacRecord.gen_pacs_record = NULL;
        }
        vsAptXSinkPacRecord.gen_pacs_record = genPacsRecord;
        vsAptXSinkPacRecord.len = len;
    }
    else
    {
        if (vsAptXSourcePacRecord.gen_pacs_record != NULL)
        {
            pfree(vsAptXSourcePacRecord.gen_pacs_record);
            vsAptXSourcePacRecord.gen_pacs_record = NULL;
        }
        vsAptXSourcePacRecord.gen_pacs_record = genPacsRecord;
        vsAptXSourcePacRecord.len = len;
    }
}

static bool isVendorIdsInvalid(const GattPacsServerVSPacRecord *pacRecord)
{
    if(pacRecord->codecId == PACS_CODEC_ID_UNKNOWN ||
       pacRecord->companyId == PACS_COMPANY_ID_UNKNOWN ||
       pacRecord->vendorCodecId == PACS_VENDOR_CODEC_ID_UNKNOWN)
        return TRUE;

    return FALSE;
}

static bool isVendorConfigLengthInvalid(const GattPacsServerVSPacRecord *pacRecord)
{

    /* If length of vendor config is Empty*/
    if(pacRecord->vsConfigLen == 0 ||
            pacRecord->vsConfig == NULL ||
              isVendorIdsInvalid(pacRecord))
        return TRUE;

    return FALSE;
}

static bool removeVSPacRecord(GPACSS_T* pacs_server, pac_record_list_vs** list, uint16 pac_record_handle)
{
    pac_record_list_vs* current = *list;
    pac_record_list_vs* prev = NULL;

    while (current != NULL)
    {
        if (current->pac_record_handle == pac_record_handle)
            break;

        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        return FALSE;
    }
    else if (prev == NULL)
    {
        *list = current->next;
    }
    else
    {
        prev->next = current->next;
    }

    CsrPmemFree(current);

    removePacRecordHandle(pacs_server, pac_record_handle);

    return TRUE;
}

static const GattPacsServerVSPacRecord *getVSPacRecord(pac_record_list_vs *list, uint16 pac_record_handle)
{
    pac_record_list_vs *iterRecord = list;

    while(iterRecord != NULL)
    {
        if (iterRecord->pac_record_handle == pac_record_handle)
            return iterRecord->pac_record;

        iterRecord = iterRecord->next;
    }

    return NULL;
}

static uint8* getVsPacRecord(uint8* len, gVsPacsRecord_t *record)
{
    uint8* genPacsRecordTemp = NULL;
    *len = record->len;

    if (*len > 0)
        genPacsRecordTemp = record->gen_pacs_record;

    return genPacsRecordTemp;
}

uint8* getGeneratedVSPacsRecord(uint8* len, uint16 handle)
{
    if (handle == HANDLE_SINK_PAC_VS_APTX)
    {
        return getVsPacRecord(len, &vsAptXSinkPacRecord);
    }
    else if (handle == HANDLE_SOURCE_PAC_VS_APTX)
    {
        return getVsPacRecord(len, &vsAptXSourcePacRecord);
    }

    return NULL;
}

uint16 GattPacsServerAddVSPacRecord(PacsServiceHandleType handle,
                                         PacsServerDirectionType direction,
                                         const GattPacsServerVSPacRecord *pacRecord)
{
    uint16 pac_record_handle = PACS_RECORD_INVALID_PARAMETERS;
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    /* If pac record is for SOURCE or if vendor configuration
     * is invalid send error code
     */
    if (pacs_server == NULL || pacRecord == NULL ||
           pacRecord->codecId == PACS_LC3_CODEC_ID)
    {
        return pac_record_handle;
    }

    /*
     * If pac record length is Invalid
     * then reject the request right away
     */
    if (isVendorConfigLengthInvalid(pacRecord))
        return PACS_RECORD_INVALID_LENGTH;

    if(isVsPacRecordMetadataPresent(pacRecord))
    {
        pac_record_handle = addVsPacRecord
                              (pacs_server,
                               audioSink?
                               &(pacs_server->data.vs_aptx_sink_pac_record):
                               &(pacs_server->data.vs_aptx_source_pac_record),
                               pacRecord
                               );

        if (!IS_PAC_RECORD_HANDLE_INVALID(pac_record_handle))
        {
            pac_record_handle |= (audioSink ? PAC_RECORD_VS_SINK_HANDLE_OFFSET :
                                              PAC_RECORD_VS_SOURCE_HANDLE_OFFSET);
            generateVsAptXPacsRecord(audioSink,
                                   audioSink?
                                   pacs_server->data.vs_aptx_sink_pac_record:
                                   pacs_server->data.vs_aptx_source_pac_record);

            pacsServerNotifyVsPacRecordChange(pacs_server,
                                              audioSink,
                                              0);
        }

    }
    return pac_record_handle;
}

bool GattPacsServerRemoveVSPacRecord(PacsServiceHandleType handle, uint16 pacRecordHandle)
{
    GPACSS_T* pacs_server = (GPACSS_T*)ServiceHandleGetInstanceData(handle);
    bool status= FALSE;
    bool audioSink = FALSE;

    if (pacs_server == NULL)
        return status;

    if (IS_PAC_RECORD_VS_SINK(pacRecordHandle))
    {

        status = removeVSPacRecord(pacs_server,
                   &(pacs_server->data.vs_aptx_sink_pac_record),
                       (pacRecordHandle & ~PAC_RECORD_VS_SINK_HANDLE_OFFSET));
        audioSink = TRUE;
    }
    else if(IS_PAC_RECORD_VS_SOURCE(pacRecordHandle))
    {
        status = removeVSPacRecord(pacs_server,
                   &(pacs_server->data.vs_aptx_source_pac_record),
                       (pacRecordHandle & ~PAC_RECORD_VS_SOURCE_HANDLE_OFFSET));
    }

    if(status)
    {
        generateVsAptXPacsRecord(audioSink,
                                 audioSink?
                                 (pacs_server->data.vs_aptx_sink_pac_record):
                                 (pacs_server->data.vs_aptx_source_pac_record));

        pacsServerNotifyVsPacRecordChange(pacs_server,
                                          audioSink,
                                          0);
    }
    return status;
}

const GattPacsServerVSPacRecord* GattPacsServerGetVSPacRecord(PacsServiceHandleType handle,
                                              uint16 pacRecordHandle)
{
    const GattPacsServerVSPacRecord *pac_record = NULL;
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return pac_record;

    if(IS_PAC_RECORD_VS_SINK(pacRecordHandle))
    {
        pac_record = getVSPacRecord(pacs_server->data.vs_aptx_sink_pac_record,
                                   (~PAC_RECORD_VS_SINK_HANDLE_OFFSET & pacRecordHandle));
    }
    else if(IS_PAC_RECORD_VS_SOURCE(pacRecordHandle))
    {
        pac_record = getVSPacRecord(pacs_server->data.vs_aptx_source_pac_record,
                                   (~PAC_RECORD_VS_SOURCE_HANDLE_OFFSET & pacRecordHandle));
    }
    return pac_record;
}
