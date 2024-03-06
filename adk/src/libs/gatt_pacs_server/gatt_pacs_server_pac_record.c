/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_pacs_server_private.h"
#include "gatt_pacs_server_msg_handler.h"
#include "gatt_pacs_server_utils.h"
#include "gatt_pacs_server_pac_record.h"
#include "gatt_pacs_server_notify.h"

#define PAC_RECORD_SINK_HANDLE_OFFSET    (uint16)0x1000
#define PAC_RECORD_SOURCE_HANDLE_OFFSET  (uint16)0x2000
#define PAC_RECORD_VS_SINK_HANDLE_OFFSET    (uint16)0x4000
#define PAC_RECORD_VS_SOURCE_HANDLE_OFFSET  (uint16)0x8000

#define IS_PAC_RECORD_SINK(handle) ((handle & PAC_RECORD_SINK_HANDLE_OFFSET) == PAC_RECORD_SINK_HANDLE_OFFSET)
#define IS_PAC_RECORD_SOURCE(handle) ((handle & PAC_RECORD_SOURCE_HANDLE_OFFSET) == PAC_RECORD_SOURCE_HANDLE_OFFSET)
#define IS_PAC_RECORD_VS_SINK(handle) ((handle & PAC_RECORD_VS_SINK_HANDLE_OFFSET) == PAC_RECORD_VS_SINK_HANDLE_OFFSET)
#define IS_PAC_RECORD_VS_SOURCE(handle) ((handle & PAC_RECORD_VS_SOURCE_HANDLE_OFFSET) == PAC_RECORD_VS_SOURCE_HANDLE_OFFSET)

#define SUPPORTED_SAMPLING_FREQUENCY_MASK  (PACS_SAMPLING_FREQUENCY_8KHZ | PACS_SAMPLING_FREQUENCY_11_025KHZ | \
                                           PACS_SAMPLING_FREQUENCY_16KHZ | PACS_SAMPLING_FREQUENCY_22_05KHZ  | \
                                           PACS_SAMPLING_FREQUENCY_24KHZ | PACS_SAMPLING_FREQUENCY_32KHZ | \
                                           PACS_SAMPLING_FREQUENCY_44_1KHZ | PACS_SAMPLING_FREQUENCY_48KHZ | \
                                           PACS_SAMPLING_FREQUENCY_88_2KHZ | PACS_SAMPLING_FREQUENCY_96KHZ | \
                                           PACS_SAMPLING_FREQUENCY_176_4KHZ | PACS_SAMPLING_FREQUENCY_192KHZ | \
                                           PACS_SAMPLING_FREQUENCY_384KHZ)
#define IS_SAMPLING_FREQUENCY_SET_INVALID(sampling_frequencies) \
    ((sampling_frequencies & SUPPORTED_SAMPLING_FREQUENCY_MASK ) == 0) 


#define FRAME_DURATION_7P5MS_MASK (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_PREFERRED_FRAME_DURATION_7P5MS)
#define FRAME_DURATION_10MS_MASK (PACS_SUPPORTED_FRAME_DURATION_10MS | PACS_PREFERRED_FRAME_DURATION_10MS)
#define FRAME_DURATION_ALL_MASK (FRAME_DURATION_7P5MS_MASK | FRAME_DURATION_10MS_MASK)
#define IS_FRAME_DURATION_SET_INVALID(frame_duration) \
    ((frame_duration & FRAME_DURATION_ALL_MASK ) == 0) 


#define AUDIO_CHANNEL_COUNTS_MASK (PACS_AUDIO_CHANNEL_1 |  \
                                   PACS_AUDIO_CHANNELS_2 | \
                                   PACS_AUDIO_CHANNELS_3 | \
                                   PACS_AUDIO_CHANNELS_4 | \
                                   PACS_AUDIO_CHANNELS_5 | \
                                   PACS_AUDIO_CHANNELS_6 | \
                                   PACS_AUDIO_CHANNELS_7 | \
                                   PACS_AUDIO_CHANNELS_8 \
                                   )
#define IS_AUDIO_CHANNEL_COUNTS_SET_INVALID(audio_channel_counts) \
        ((audio_channel_counts & AUDIO_CHANNEL_COUNTS_MASK ) == 0)

#define IS_PAC_RECORD_HANDLE_INVALID(handle) \
        ((handle & PACS_RECORD_ERRORCODE_BASE) == PACS_RECORD_ERRORCODE_BASE)

#define LC3_CODEC_SPECIFIC_CAPABILTIES_LENGTH \
        ( 1 + SAMPLING_FREQUENCY_LENGTH + \
          1 + SUPPORTED_FRAME_DURATION_LENGTH + \
          1 + AUDIO_CHANNEL_COUNTS_LENGTH + \
          1 + SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH + \
          1 + MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_LENGTH)

#define SAMPLING_FREQUENCY_LENGTH    0x03
#define SAMPLING_FREQUENCY_TYPE      0x01

#define SUPPORTED_FRAME_DURATION_LENGTH  0x02
#define SUPPORTED_FRAME_DURATION_TYPE    0x02

#define AUDIO_CHANNEL_COUNTS_LENGTH      0x02
#define AUDIO_CHANNEL_COUNTS_TYPE        0x03

#define SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH  0x05
#define SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE    0x04

#define MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_LENGTH  0x02
#define MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_TYPE    0x05

#define PACS_RECORD_SIZE_IN_OCTETS 31  /* metadata length is filled */

#define PACS_RECORD_SIZE  (PACS_RECORD_SIZE_IN_OCTETS*sizeof(uint8))

#define MAX_PACS_RECORD_SIZE  ((2*PACS_RECORD_SIZE) - 1)

/* With LE ATT MTU =64, PACS can accomodate 2 PAC record in 
 * a single handle
 */
#define PACS_RECORD_IN_SINGLE_HANDLE   2

/* 2 SINK handles for LC3. 2 SINK Handle can accomodate 4 PACS
 * records each having unique metadata
 */
#define NUM_SINK_PAC_RECORD_HANDLES  2

#define PACS_CODEC_ID_OFFSET         1

typedef struct pacs_record_t
{
    uint8* gen_pacs_record; /* generated pac record */
    uint8 len;
} gPacsRecord_t;

gPacsRecord_t sinkPacRecord[NUM_SINK_PAC_RECORD_HANDLES];
gPacsRecord_t sourcePacRecord;


#define PACS_RECORD_SIZE_WITHOUT_METADATA_IN_OCTETS     27

/* 1 Vendor specific SINK and 1 Vendor specific SRC PAC record
 * This PAC record will have LC3 codec ID but variable VS Metadata
 * which makes 1 single handle accomodating only 1 SINK PAC or
 * SRC PAC record
 */
gPacsRecord_t vsSinkPacRecord;
gPacsRecord_t vsSourcePacRecord;


/*! \brief Write a uint8 to the place pointed to and increment the pointer.

    \param buf Pointer to pointer to place in buffer.
    \param val Value to be written.
*/
static void write_uint8(uint8 **buf, uint8 val)
{
    *((*buf)++) = val & 0xFF;
}

/*! \brief Write a uint16 to the place pointed to and increment the pointer.
 
    \param buf Pointer to pointer to place in buffer.
    \param val Value to be written.
*/
static void write_uint16(uint8 **buf, uint16 val)
{
    write_uint8(buf, (uint8)(val & 0xFF));
    write_uint8(buf, (uint8)(val >> 8));
}

static uint8 getCodecCount(pac_record_list *list, PacsCodecIdType *codec_id, bool *vendor_codec_supported)
{
    pac_record_list *iterRecord = list;
    uint8 codecCount = 0;
    *vendor_codec_supported = FALSE;

    while( iterRecord != NULL)
    {
        switch (iterRecord->pac_record->codecId)
        {
            case PACS_LC3_CODEC_ID:
                *codec_id |= PACS_LC3_CODEC_ID;
                break;

            case PACS_VENDOR_CODEC_ID:
                *vendor_codec_supported = TRUE;
                break;

            case PACS_CODEC_ID_UNKNOWN:
                break;
        }
        iterRecord = iterRecord->next;
    }

    if ((*codec_id & PACS_LC3_CODEC_ID) == PACS_LC3_CODEC_ID)
        codecCount++;

    if (*vendor_codec_supported)
        codecCount++;

    return codecCount;
}

static void setListPacRecordsNotConsumed(pac_record_list *list)
{
    pac_record_list *iterRecord = list;

    while( iterRecord != NULL)
    {
        iterRecord->consumed = FALSE;

        iterRecord = iterRecord->next;
    }
}

static uint16 getPreferredAudioContext(const GattPacsServerRecordType *pac_record)
{
    uint16 audioContext = 0;

    if(pac_record->metadataLength != 0 &&
        pac_record->metadata[1] == PACS_PREFERRED_AUDIO_CONTEXTS_TYPE)
    {
        audioContext = ((uint16)(pac_record->metadata[3] << 8) | pac_record->metadata[2]);
    }

    return audioContext;
}

static uint8 countNumPacRecord(pac_record_list *list, PacsCodecIdType codec_id)
{
    pac_record_list *iterRecord = list;
    pac_record_list *iterRecord1 = list;
    uint16 audioContext;
    uint8 count = 0;

    setListPacRecordsNotConsumed(list);

    while( iterRecord != NULL)
    {
        if (codec_id == iterRecord->pac_record->codecId && !iterRecord->consumed)
        {
            iterRecord->consumed = TRUE;
            count++;
            audioContext = getPreferredAudioContext(iterRecord->pac_record);

            while(iterRecord1 !=  NULL)
            {
                if (codec_id == iterRecord1->pac_record->codecId &&
                    !iterRecord1->consumed &&
                    iterRecord->pac_record->metadataLength == iterRecord1->pac_record->metadataLength &&
                    audioContext == getPreferredAudioContext(iterRecord1->pac_record))
                {
                    /* Don't count this record as we have already counted this above */
                    iterRecord1->consumed = TRUE;
                }
                iterRecord1 = iterRecord1->next;
            }
        }
        iterRecord = iterRecord->next;
    }

    return count;
}

static uint8 getSupportedFrameDurations(uint8 value1, uint8 value2)
{
    uint8 frameDurations;

    frameDurations = value1 | value2;

    if ((frameDurations & (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS)) ==
        (PACS_SUPPORTED_FRAME_DURATION_7P5MS | PACS_SUPPORTED_FRAME_DURATION_10MS))
    {
        if ((frameDurations & PACS_PREFERRED_FRAME_DURATION_10MS) == PACS_PREFERRED_FRAME_DURATION_10MS)
           frameDurations &= ~(1UL << 4);  /* Clear bit4*/
        else
           frameDurations &= ~(1UL << 5);  /* Clear bit5*/
    }
    else
    {
        frameDurations &= ~(1UL << 4);  /* Clear bit4*/
        frameDurations &= ~(1UL << 5);  /* Clear bit5*/
    }

    return frameDurations;
}

static GattPacsServerRecordType* getConcatenatedPacRecordByCodecId(pac_record_list *list,
    PacsCodecIdType codec_id)
{
    pac_record_list *iterRecord = list;
    pac_record_list *iterRecord1 = list;
    uint16 audioContext;
    uint8 supportedFrameDuration;
    GattPacsServerRecordType *pac_record;

    /*Allocate pac_record */
    pac_record = (GattPacsServerRecordType*)PanicUnlessMalloc(sizeof(GattPacsServerRecordType));
    memset(pac_record, 0, sizeof(GattPacsServerRecordType));

    while( iterRecord != NULL)
    {
        if (codec_id == iterRecord->pac_record->codecId && !iterRecord->consumed)
        {
            iterRecord->consumed = TRUE;

            pac_record->codecId = iterRecord->pac_record->codecId;
            pac_record->companyId = iterRecord->pac_record->companyId;
            pac_record->vendorCodecId = iterRecord->pac_record->vendorCodecId;
            pac_record->supportedSamplingFrequencies = iterRecord->pac_record->supportedSamplingFrequencies;
            pac_record->supportedFrameDuration = iterRecord->pac_record->supportedFrameDuration;
            pac_record->minSupportedOctetsPerCodecFrame = iterRecord->pac_record->minSupportedOctetsPerCodecFrame;
            pac_record->maxSupportedOctetsPerCodecFrame = iterRecord->pac_record->maxSupportedOctetsPerCodecFrame;
            pac_record->audioChannelCounts = iterRecord->pac_record->audioChannelCounts;
            pac_record->supportedMaxCodecFramePerSdu = iterRecord->pac_record->supportedMaxCodecFramePerSdu;

            pac_record->metadataLength = iterRecord->pac_record->metadataLength;

            if(pac_record->metadataLength != 0)
            {
                pac_record->metadata = iterRecord->pac_record->metadata;
            }

            /* Get preferred audio context from metadata  for this record */
            audioContext = getPreferredAudioContext(iterRecord->pac_record);

            while(iterRecord1 != NULL)
            {
                if (codec_id == iterRecord1->pac_record->codecId &&
                    !iterRecord1->consumed &&
                     pac_record->metadataLength == iterRecord1->pac_record->metadataLength &&
                    audioContext == getPreferredAudioContext(iterRecord1->pac_record))
                {
                    /* This means we have one more PAC record in list which has same metadata */
                    iterRecord1->consumed = TRUE;

                    pac_record->supportedSamplingFrequencies |= iterRecord->pac_record->supportedSamplingFrequencies;
                    pac_record->audioChannelCounts |= iterRecord->pac_record->audioChannelCounts;

                    supportedFrameDuration = getSupportedFrameDurations(
                                                pac_record->supportedFrameDuration,
                                                iterRecord->pac_record->supportedFrameDuration);
                    pac_record->supportedFrameDuration = supportedFrameDuration;

                    if (pac_record->minSupportedOctetsPerCodecFrame > iterRecord->pac_record->minSupportedOctetsPerCodecFrame)
                        pac_record->minSupportedOctetsPerCodecFrame = iterRecord->pac_record->minSupportedOctetsPerCodecFrame;

                    if (pac_record->maxSupportedOctetsPerCodecFrame < iterRecord->pac_record->maxSupportedOctetsPerCodecFrame)
                        pac_record->maxSupportedOctetsPerCodecFrame = iterRecord->pac_record->maxSupportedOctetsPerCodecFrame;

                    if (pac_record->supportedMaxCodecFramePerSdu != 0 &&
                        pac_record->supportedMaxCodecFramePerSdu < iterRecord->pac_record->supportedMaxCodecFramePerSdu)
                    {
                        pac_record->supportedMaxCodecFramePerSdu = iterRecord->pac_record->supportedMaxCodecFramePerSdu;
                    }
                }

                iterRecord1 = iterRecord1->next;
            }

            /* Done, return as we have constructed concatenated PAC record*/
            break;
        }

        iterRecord = iterRecord->next;
    }

    return pac_record;
}

static void allocateLC3PacsRecord(bool audioSink)
{
    uint16 i;

    if(audioSink)
    {
        GATT_PACS_SERVER_DEBUG_INFO(("PACS: num of PAC record handles supported for Sink 0x%x \n", NUM_SINK_PAC_RECORD_HANDLES));

        /* Allocate memory for Sink PAC record */
        for ( i = 0 ; i < NUM_SINK_PAC_RECORD_HANDLES; i++)
        {
            if (sinkPacRecord[i].gen_pacs_record == NULL)
            {
                /* We can accomodate 2 PAC records in a single SINK handle */
                sinkPacRecord[i].gen_pacs_record = (uint8*)(PanicUnlessMalloc(MAX_PACS_RECORD_SIZE));
            }
            memset(sinkPacRecord[i].gen_pacs_record, 0, MAX_PACS_RECORD_SIZE);
        }
    }
    else
    {
        GATT_PACS_SERVER_DEBUG_INFO(("PACS: num of PAC record handles supported for Source is 1\n"));

        if (sourcePacRecord.gen_pacs_record == NULL)
        {
            /* We can accomodate 2 PAC records in a single SOURCE PAC handle */
            sourcePacRecord.gen_pacs_record = (uint8*)(PanicUnlessMalloc(MAX_PACS_RECORD_SIZE));
        }
        memset(sourcePacRecord.gen_pacs_record, 0, MAX_PACS_RECORD_SIZE);
    }
}

static void addRecord(bool audioSink, GattPacsServerRecordType *pac_record, uint16 index, uint16 offset)
{
    uint8 *genPacsRecord = audioSink? sinkPacRecord[index].gen_pacs_record + offset:
                                      sourcePacRecord.gen_pacs_record + offset;

    write_uint8(&genPacsRecord, pac_record->codecId);
    write_uint16(&genPacsRecord, pac_record->companyId);    /* Company ID */
    write_uint16(&genPacsRecord, pac_record->vendorCodecId);     /* Vendor ID */

    write_uint8(&genPacsRecord, LC3_CODEC_SPECIFIC_CAPABILTIES_LENGTH);

    write_uint8(&genPacsRecord, SAMPLING_FREQUENCY_LENGTH);  /* Length of Supported Sampling frequency field */
    write_uint8(&genPacsRecord, SAMPLING_FREQUENCY_TYPE);  /* Type defined by PACS Spec */
    write_uint16(&genPacsRecord, pac_record->supportedSamplingFrequencies);

    write_uint8(&genPacsRecord, SUPPORTED_FRAME_DURATION_LENGTH);  /* Length of Supported Frame Duration field */
    write_uint8(&genPacsRecord, SUPPORTED_FRAME_DURATION_TYPE);  /* Type defined by PACS Spec */
    write_uint8(&genPacsRecord, pac_record->supportedFrameDuration);

    write_uint8(&genPacsRecord, AUDIO_CHANNEL_COUNTS_LENGTH);  /* Length of Audio Channel counts field */
    write_uint8(&genPacsRecord, AUDIO_CHANNEL_COUNTS_TYPE);    /* Type defined by PACS Spec */
    write_uint8(&genPacsRecord, pac_record->audioChannelCounts);

    write_uint8(&genPacsRecord, SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH);  /* Length of min & max supported
                                                                            octets per codec frame field */
    write_uint8(&genPacsRecord, SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE);   /* Type defined by PACS Spec */
    write_uint16(&genPacsRecord, pac_record->minSupportedOctetsPerCodecFrame);
    write_uint16(&genPacsRecord, pac_record->maxSupportedOctetsPerCodecFrame);

    write_uint8(&genPacsRecord, MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_LENGTH);  /* Length of supported codec frame field */
    write_uint8(&genPacsRecord, MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_TYPE);    /* Type defined by PACS Spec */
    write_uint8(&genPacsRecord, pac_record->supportedMaxCodecFramePerSdu);

    write_uint8(&genPacsRecord, pac_record->metadataLength);

    if(pac_record->metadataLength != 0)
    {
        memcpy(genPacsRecord, pac_record->metadata, pac_record->metadataLength);
    }
}

static void constructRecord(pac_record_list *list, uint16 index, uint16 offset, uint8 numOfPacRecord, bool audioSink)
{
    uint8 i;
    uint8 *genPacsRecord = NULL;
    GattPacsServerRecordType *pac_record = NULL;

    genPacsRecord = audioSink ? sinkPacRecord[index].gen_pacs_record:
                                sourcePacRecord.gen_pacs_record;

    /* Write num of PAC records */
    write_uint8(&genPacsRecord, numOfPacRecord);

    for (i = 0 ; i < numOfPacRecord; i++)
    {
        pac_record = getConcatenatedPacRecordByCodecId(list, PACS_LC3_CODEC_ID);

        if(pac_record)
            addRecord(audioSink, pac_record, index, offset);

        offset = PACS_RECORD_SIZE;

        if (pac_record)
            free(pac_record);
    }
}

static void generatePacsRecordLC3(bool audioSink, pac_record_list *list, uint8 numPacRecords)
{
    uint16 index;

     /* Mark all the Records of the list as not consumed for generation */
    setListPacRecordsNotConsumed(list);

    if(audioSink)
    {
        for (index = 0; index < NUM_SINK_PAC_RECORD_HANDLES; index ++)
        {
            if (numPacRecords >= PACS_RECORD_IN_SINGLE_HANDLE)
            {
                constructRecord(list, index, PACS_CODEC_ID_OFFSET, PACS_RECORD_IN_SINGLE_HANDLE, audioSink);
                sinkPacRecord[index].len = MAX_PACS_RECORD_SIZE;

                numPacRecords = numPacRecords - PACS_RECORD_IN_SINGLE_HANDLE;
            }
            else if (numPacRecords == 1)
            {
                /* First handle will have just 1 record entry */
                constructRecord(list, index, PACS_CODEC_ID_OFFSET, numPacRecords, audioSink);
                sinkPacRecord[index].len = PACS_RECORD_SIZE;

                numPacRecords = numPacRecords - 1;
            }
            else
            {
                /* index will have no entry */
                constructRecord(list, index, 0, numPacRecords, audioSink);
                sinkPacRecord[index].len = 1; /* No Pac record */
            }
       }
    }
    else
    {
        index = 0; /* index = 0 corresponds to first SOURCE handle */
        if (numPacRecords >= PACS_RECORD_IN_SINGLE_HANDLE)
        {
            /* We can accomodate only 2 PAC records, rest will be discarded */
            constructRecord(list, index, PACS_CODEC_ID_OFFSET, PACS_RECORD_IN_SINGLE_HANDLE, audioSink);
            sourcePacRecord.len = MAX_PACS_RECORD_SIZE;
        }
        else if (numPacRecords == 1)
        {
            /* We can accomodate this PAC record in Source Handle */
            constructRecord(list, index, PACS_CODEC_ID_OFFSET, numPacRecords, audioSink);
            sourcePacRecord.len = PACS_RECORD_SIZE;
        }
    }
}

static void generatePacsRecord(bool audioSink, pac_record_list *list)
{
    uint8 numPacRecords = 0;
    PacsCodecIdType codec_id = PACS_CODEC_ID_UNKNOWN;
    bool vendorCodecSupported;

    /* Parse the list for identifying num of codec id */
    if (getCodecCount(list, &codec_id, &vendorCodecSupported) > 1)
    {
        GATT_PACS_SERVER_DEBUG_INFO(("Only LC3 codec is supported. Vendor Codec NOT Supported for PAC Record generation\n"));
    }

    /* Calculate num of unique PAC records for LC3.
     * PAC records having same codec id and metadata shall be counted as 1
     */
    numPacRecords = countNumPacRecord(list, PACS_LC3_CODEC_ID);
    GATT_PACS_SERVER_DEBUG_INFO(("PACS: num of PAC record Handles required 0x%x \n",(numPacRecords +1)/2));

    /* Allocate memory for LC3 codec PACS support */
    allocateLC3PacsRecord(audioSink);

    generatePacsRecordLC3(audioSink, list, numPacRecords);
}

static bool isAudioContextAlreadyUsed(const GattPacsServerRecordType *pac_record,
    pac_record_list *list)
{
    pac_record_list *iterRecord = list;
    uint16 audioContext = getPreferredAudioContext(pac_record);

    while( iterRecord != NULL)
    {
        if (pac_record->codecId == iterRecord->pac_record->codecId &&
            pac_record->metadataLength == iterRecord->pac_record->metadataLength &&
            audioContext == getPreferredAudioContext(iterRecord->pac_record))
        {
            return TRUE;
        }

        iterRecord = iterRecord->next;
    }
    return FALSE;
}

static bool isPacRecordValid(const GattPacsServerRecordType *pac_record)
{
    if(pac_record->codecId == PACS_CODEC_ID_UNKNOWN)
        return FALSE;

    if(pac_record->codecId == PACS_LC3_CODEC_ID &&
        (pac_record->companyId != 0 ||
         pac_record->vendorCodecId != 0))
    {
        return FALSE;
    }

    if (IS_SAMPLING_FREQUENCY_SET_INVALID(pac_record->supportedSamplingFrequencies))
        return FALSE;

    if (IS_FRAME_DURATION_SET_INVALID(pac_record->supportedFrameDuration))
        return FALSE;

    if (IS_AUDIO_CHANNEL_COUNTS_SET_INVALID(pac_record->audioChannelCounts))
        return FALSE;

    if (pac_record->supportedMaxCodecFramePerSdu == 0)
        return FALSE;

    if (pac_record->minSupportedOctetsPerCodecFrame == 0 ||
        pac_record->maxSupportedOctetsPerCodecFrame == 0 ||
        pac_record->minSupportedOctetsPerCodecFrame >
        pac_record->maxSupportedOctetsPerCodecFrame)
     {
        return FALSE;
     }

    return TRUE;
}

static const GattPacsServerRecordType *getPacRecord(pac_record_list *list, uint16 pac_record_handle)
{
    pac_record_list *iterRecord = list;

    while( iterRecord != NULL)
    {
        if (iterRecord->pac_record_handle == pac_record_handle)
            return iterRecord->pac_record;

        iterRecord = iterRecord->next;
    }

    return NULL;
}

static bool isPacRecordPresent(pac_record_list * list, const GattPacsServerRecordType *pac_record)
{
    pac_record_list *iterRecord = list;

    while( iterRecord != NULL)
    {
        if (!memcmp(iterRecord->pac_record, pac_record, sizeof(GattPacsServerRecordType)))
            return TRUE;

        iterRecord = iterRecord->next;
    }

    return FALSE;
}

static bool isPacRecordMetadataPresent(const GattPacsServerRecordType *pac_record)
{
    if(pac_record->metadataLength != 0 &&
        (pac_record->metadata[1] == PACS_PREFERRED_AUDIO_CONTEXTS_TYPE ||
        pac_record->metadata[1] == PACS_VENDOR_SPECIFIC_METATDATA_TYPE))
        return TRUE;

    return FALSE;
}

static bool isPacRecordHandleExhausted(PacsServerDirectionType direction)
{

    if (direction == PACS_SERVER_IS_AUDIO_SINK)
    {

        if (sinkPacRecord[NUM_SINK_PAC_RECORD_HANDLES - 1].len == MAX_PACS_RECORD_SIZE)
            return TRUE;
    }
    else
    {

        if (sourcePacRecord.len == MAX_PACS_RECORD_SIZE)
            return TRUE;
    }

    return FALSE;
}

static bool isVsPacRecordHandleExhausted(PacsServerDirectionType direction)
{
    if (direction == PACS_SERVER_IS_AUDIO_SINK)
    {
        if (vsSinkPacRecord.len > 1)
            return TRUE;
    }
    else
    {
        if (vsSourcePacRecord.len > 1)
            return TRUE;
    }

    return FALSE;
}

static bool isVsMetadataTypePresentInPacRecord(const GattPacsServerRecordType *pac_record)
{
    bool vsMetadataPresent = FALSE;
    uint8 len = 0;

    if(pac_record->metadataLength == 0)
    {
        GATT_PACS_SERVER_DEBUG_INFO(("PACS: isVsMetadataTypePresentInPacRecord vsMetadataPresent=%x\n", vsMetadataPresent));
        return vsMetadataPresent;
    }

    while(len <= pac_record->metadataLength)
    {
        /* Check if type from LTV pair has PACS_VENDOR_SPECIFIC_METATDATA_TYPE */
        if(pac_record->metadata[len + 1] == PACS_VENDOR_SPECIFIC_METATDATA_TYPE)
        {
            vsMetadataPresent = TRUE;
            break;
        }
        len += pac_record->metadata[len] + 1;
    }

    GATT_PACS_SERVER_DEBUG_INFO(("PACS: isVsMetadataTypePresentInPacRecord vsMetadataPresent=%x\n", vsMetadataPresent));

    return vsMetadataPresent;
}

static void generateVsPacsRecord(bool audioSink, pac_record_list *list)
{
    uint8 len;
    uint8 *genPacsRecord = NULL;
    uint8 *genPacsRecordTemp = NULL;

    if (list == NULL)
    {
       len = 1; /* for no pac record */
       genPacsRecord = (uint8*)(PanicUnlessMalloc(len));
       memset(genPacsRecord, 0, len);

        genPacsRecordTemp = genPacsRecord;
        /* Write num of PAC records */
        write_uint8(&genPacsRecord, 0);
    }
	else
    {
        /* Parse the list for identifying num of codec id */
        len = (PACS_RECORD_SIZE_WITHOUT_METADATA_IN_OCTETS + list->pac_record->metadataLength) * sizeof(uint8);

        genPacsRecord = (uint8*)(PanicUnlessMalloc(len));
        memset(genPacsRecord, 0, len);

        genPacsRecordTemp = genPacsRecord;
        /* Write num of PAC records */
        write_uint8(&genPacsRecord, 1);

        write_uint8(&genPacsRecord, list->pac_record->codecId);
        write_uint16(&genPacsRecord, list->pac_record->companyId);    /* Company ID */
        write_uint16(&genPacsRecord, list->pac_record->vendorCodecId);     /* Vendor ID */

        write_uint8(&genPacsRecord, LC3_CODEC_SPECIFIC_CAPABILTIES_LENGTH);

        write_uint8(&genPacsRecord, SAMPLING_FREQUENCY_LENGTH);  /* Length of Supported Sampling frequency field */
        write_uint8(&genPacsRecord, SAMPLING_FREQUENCY_TYPE);  /* Type defined by PACS Spec */
        write_uint16(&genPacsRecord, list->pac_record->supportedSamplingFrequencies);

        write_uint8(&genPacsRecord, SUPPORTED_FRAME_DURATION_LENGTH);  /* Length of Supported Frame Duration field */
        write_uint8(&genPacsRecord, SUPPORTED_FRAME_DURATION_TYPE);  /* Type defined by PACS Spec */
        write_uint8(&genPacsRecord, list->pac_record->supportedFrameDuration);

        write_uint8(&genPacsRecord, AUDIO_CHANNEL_COUNTS_LENGTH);  /* Length of Audio Channel counts field */
        write_uint8(&genPacsRecord, AUDIO_CHANNEL_COUNTS_TYPE);    /* Type defined by PACS Spec */
        write_uint8(&genPacsRecord, list->pac_record->audioChannelCounts);

        write_uint8(&genPacsRecord, SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH);  /* Length of min & max supported
                                                                              octets per codec frame field */
        write_uint8(&genPacsRecord, SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE);   /* Type defined by PACS Spec */
        write_uint16(&genPacsRecord, list->pac_record->minSupportedOctetsPerCodecFrame);
        write_uint16(&genPacsRecord, list->pac_record->maxSupportedOctetsPerCodecFrame);

        write_uint8(&genPacsRecord, MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_LENGTH);  /* Length of supported codec frame field */
        write_uint8(&genPacsRecord, MAX_SUPPORTED_CODEC_FRAMES_PER_SDU_TYPE);    /* Type defined by PACS Spec */
        write_uint8(&genPacsRecord, list->pac_record->supportedMaxCodecFramePerSdu);

        write_uint8(&genPacsRecord, list->pac_record->metadataLength);

        if(list->pac_record->metadataLength != 0)
        {
            memcpy(genPacsRecord, list->pac_record->metadata, list->pac_record->metadataLength);
        }
	}

    if (audioSink)
    {
        if (vsSinkPacRecord.gen_pacs_record != NULL)
        {
            free(vsSinkPacRecord.gen_pacs_record);
            vsSinkPacRecord.gen_pacs_record = NULL;
        }
        vsSinkPacRecord.gen_pacs_record = genPacsRecordTemp;
        vsSinkPacRecord.len = len;
    }
    else
    {
        if (vsSourcePacRecord.gen_pacs_record != NULL)
        {
            free(vsSourcePacRecord.gen_pacs_record);
            vsSourcePacRecord.gen_pacs_record = NULL;
        }
        vsSourcePacRecord.gen_pacs_record = genPacsRecordTemp;
        vsSourcePacRecord.len = len;
    }
}

static uint16 addPacRecord(GPACSS_T *pacs_server, pac_record_list ** list, const GattPacsServerRecordType *pac_record)
{
    pac_record_list *new_record = NULL;
    uint16 pac_record_handle;

    /* Ensure PAC record has metadata field */
    if(!isPacRecordMetadataPresent(pac_record))
        return (uint16)PACS_RECORD_METATDATA_NOT_ADDED;

    /* Check if the pac_record passed already exists */
    if (isPacRecordPresent(*list, pac_record))
        return (uint16)PACS_RECORD_ALREADY_ADDED;

    pac_record_handle = generatePacRecordHandle(pacs_server);

    if (pac_record_handle != PACS_RECORD_HANDLES_EXHAUSTED)
    {
        /* Allocate memory for PAC record in the list */
        new_record = (pac_record_list*)PanicUnlessMalloc(sizeof(pac_record_list));

        /* Assign the pac_record pointer passed by application */
        new_record->pac_record = pac_record;
        new_record->next = *list;
        new_record->pac_record_handle = pac_record_handle;
        new_record->consumed = FALSE;
        *list = new_record;
    }

    return pac_record_handle;
}

static bool removePacRecord(GPACSS_T *pacs_server, pac_record_list ** list, uint16 pac_record_handle)
{
    pac_record_list *current = *list;
    pac_record_list *prev = NULL;


    while(current != NULL)
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

    free(current);

    removePacRecordHandle(pacs_server, pac_record_handle);

    return TRUE;
}


uint8* getGeneratedPacsRecord(bool audioSink, uint8 *len, uint16 handle)
{
    uint8 *genPacRecord = NULL;
    uint8 *genPacsRecordTemp = NULL;
    if (audioSink)
    {
        if (handle == HANDLE_SINK_PAC_1)
        {
            genPacRecord = sinkPacRecord[0].gen_pacs_record;
            *len = sinkPacRecord[0].len;
        }
        else if(handle == HANDLE_SINK_PAC_2)
        {
            genPacRecord = sinkPacRecord[NUM_SINK_PAC_RECORD_HANDLES - 1].gen_pacs_record;
            *len = sinkPacRecord[NUM_SINK_PAC_RECORD_HANDLES - 1].len;
        }
        else if(handle == HANDLE_SINK_PAC_VS)
        {
            *len = vsSinkPacRecord.len;
            if (*len == 0)
            {
                vsSinkPacRecord.len = 1;
                *len = vsSinkPacRecord.len; /* for no pac record */
                genPacsRecordTemp = (uint8*)(PanicUnlessMalloc(*len));
                memset(genPacsRecordTemp, 0, *len);

                vsSinkPacRecord.gen_pacs_record = genPacsRecordTemp;

                /* Write num of PAC records */
                write_uint8(&genPacsRecordTemp, 0);
            }
            genPacRecord = vsSinkPacRecord.gen_pacs_record;
        }
    }
    else
    {
        if (handle == HANDLE_SOURCE_PAC)
        {
            genPacRecord = sourcePacRecord.gen_pacs_record;
            *len = sourcePacRecord.len;
        }
        else if(handle == HANDLE_SOURCE_PAC_VS)
        {
            *len = vsSourcePacRecord.len;
            if (*len == 0)
            {
                vsSourcePacRecord.len = 1;
                *len = vsSourcePacRecord.len; /* for no pac record */
                genPacsRecordTemp = (uint8*)(PanicUnlessMalloc(*len));
                memset(genPacsRecordTemp, 0, *len);

                vsSourcePacRecord.gen_pacs_record = genPacsRecordTemp;

                /* Write num of PAC records */
                write_uint8(&genPacsRecordTemp, 0);
            }
            genPacRecord = vsSourcePacRecord.gen_pacs_record;
        }
    }

    return genPacRecord;
}


/****************************************************************************/
uint16 GattPacsServerAddPacRecord(ServiceHandleType handle,
                                         PacsServerDirectionType direction,
                                         const GattPacsServerRecordType *pacRecord)
{
    uint16 pac_record_handle = PACS_RECORD_INVALID_PARAMETERS;
    bool audioSink = (direction == PACS_SERVER_IS_AUDIO_SINK? TRUE: FALSE);
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL ||
        pacRecord == NULL ||
        !isPacRecordValid(pacRecord))
    {
        return pac_record_handle;
    }

    /* vendor codec id and paramters are not available yet*/
    if(pacRecord->codecId == PACS_VENDOR_CODEC_ID)
        return PACS_RECORD_VENDOR_CODEC_NOT_SUPPORTED;

    /* Handle VS metadata for LC3 codecId. These are treated seperately
     * from standard PAC record because of variable length
     */
    if(isVsMetadataTypePresentInPacRecord(pacRecord))
    {
        if(isVsPacRecordHandleExhausted(direction))
            return PACS_RECORD_HANDLES_EXHAUSTED;

        pac_record_handle = addPacRecord
                              (pacs_server,
                               audioSink?
                               &(pacs_server->data.vs_sink_pack_record):
                               &(pacs_server->data.vs_source_pack_record),
                               pacRecord
                               ); 

        GATT_PACS_SERVER_DEBUG_INFO(("PACS: pac_record_handle=%x\n", pac_record_handle));


        if (!IS_PAC_RECORD_HANDLE_INVALID(pac_record_handle))
        {
            pac_record_handle |= (audioSink ? PAC_RECORD_VS_SINK_HANDLE_OFFSET :
                                              PAC_RECORD_VS_SOURCE_HANDLE_OFFSET);
            generateVsPacsRecord(audioSink,
                                 audioSink?
                                 pacs_server->data.vs_sink_pack_record:
                                 pacs_server->data.vs_source_pack_record);

            pacsServerNotifyVsPacRecordChange(pacs_server, audioSink, 0);
        }

        return pac_record_handle;
    }


    /* identify if the PAC record requested has preferred audio context 
     * which was already added earlier. This would mean PACS have to 
     * concatenate this configuration with the existing preferred audio
     * context PAC record for the same codec id*/
    /* identify if SINK or SRC PAC handle are exhausted or not */
    if (isPacRecordHandleExhausted(direction) && 
        !isAudioContextAlreadyUsed(pacRecord, audioSink?
                           pacs_server->data.sink_pack_record:
                           pacs_server->data.source_pack_record))
    {
        return PACS_RECORD_HANDLES_EXHAUSTED;
    }

    pac_record_handle = addPacRecord
                          (pacs_server,
                           audioSink?
                           &(pacs_server->data.sink_pack_record):
                           &(pacs_server->data.source_pack_record),
                           pacRecord
                           );

    if (!IS_PAC_RECORD_HANDLE_INVALID(pac_record_handle))
    {
        pac_record_handle |= (audioSink ? PAC_RECORD_SINK_HANDLE_OFFSET :
                                          PAC_RECORD_SOURCE_HANDLE_OFFSET);
        generatePacsRecord(audioSink,
                           audioSink?
                           pacs_server->data.sink_pack_record:
                           pacs_server->data.source_pack_record);

        pacsServerNotifyPacRecordChange(pacs_server, audioSink, 0);
    }

    return pac_record_handle;
}

bool GattPacsServerRemovePacRecord(ServiceHandleType handle,
                                      uint16 pacRecordHandle)
{
    bool status = FALSE;
    bool audioSink = FALSE;
    bool vendorPac = FALSE;
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return status;

    if (IS_PAC_RECORD_SINK(pacRecordHandle))
    {
        status = removePacRecord(pacs_server,
                                 &(pacs_server->data.sink_pack_record),
                                 (pacRecordHandle & ~PAC_RECORD_SINK_HANDLE_OFFSET));
        audioSink = TRUE;
    }
    else if (IS_PAC_RECORD_SOURCE(pacRecordHandle))
    {
        status = removePacRecord(pacs_server,
                                 &(pacs_server->data.source_pack_record),
                                (pacRecordHandle & ~PAC_RECORD_SOURCE_HANDLE_OFFSET));
    }
    else if (IS_PAC_RECORD_VS_SINK(pacRecordHandle))
    {
        status = removePacRecord(pacs_server,
                                 &(pacs_server->data.vs_sink_pack_record),
                                 (pacRecordHandle & ~PAC_RECORD_VS_SINK_HANDLE_OFFSET));
        audioSink = TRUE;
        vendorPac = TRUE;
    }
    else if (IS_PAC_RECORD_VS_SOURCE(pacRecordHandle))
    {
        status = removePacRecord(pacs_server,
                                 &(pacs_server->data.vs_source_pack_record),
                                (pacRecordHandle & ~PAC_RECORD_VS_SOURCE_HANDLE_OFFSET));
        vendorPac = TRUE;
    }

    if (status)
    {
        if (vendorPac)
        {
             generateVsPacsRecord(audioSink,
                                  audioSink?
                                  pacs_server->data.vs_sink_pack_record:
                                  pacs_server->data.vs_source_pack_record);

             pacsServerNotifyVsPacRecordChange(pacs_server, audioSink, 0);
        }
        else
        {
             generatePacsRecord(audioSink,
                                audioSink?
                                pacs_server->data.sink_pack_record:
                                pacs_server->data.source_pack_record);

             pacsServerNotifyPacRecordChange(pacs_server, audioSink, 0);
        }
    }

    return status;
}

const GattPacsServerRecordType* GattPacsServerGetPacRecord(ServiceHandleType handle,
                                                          uint16 pacRecordHandle)
{
    const GattPacsServerRecordType *pac_record = NULL;
    GPACSS_T *pacs_server = (GPACSS_T *) ServiceHandleGetInstanceData(handle);

    if (pacs_server == NULL)
        return pac_record;

    if (IS_PAC_RECORD_SINK(pacRecordHandle))
    {
        pac_record = getPacRecord(pacs_server->data.sink_pack_record,
                                  (pacRecordHandle & ~PAC_RECORD_SINK_HANDLE_OFFSET));
    }
    else if (IS_PAC_RECORD_SOURCE(pacRecordHandle))
    {
        pac_record = getPacRecord(pacs_server->data.source_pack_record,
                                 (pacRecordHandle & ~PAC_RECORD_SOURCE_HANDLE_OFFSET));
    }
    else if (IS_PAC_RECORD_VS_SINK(pacRecordHandle))
    {
        pac_record = getPacRecord(pacs_server->data.vs_sink_pack_record,
                                  (pacRecordHandle & ~PAC_RECORD_VS_SINK_HANDLE_OFFSET));
    }
    else if (IS_PAC_RECORD_VS_SOURCE(pacRecordHandle))
    {
        pac_record = getPacRecord(pacs_server->data.vs_source_pack_record,
                                 (pacRecordHandle & ~PAC_RECORD_VS_SOURCE_HANDLE_OFFSET));
    }

    return pac_record;
}

