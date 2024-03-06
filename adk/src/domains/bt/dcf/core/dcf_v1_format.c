/*!
\copyright  Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of DCF v1 Format framework
*/

#include "dcf_v1_format.h"

#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <util.h>

#define DEBUG_LOG_DCF       DEBUG_LOG
#define DEBUG_LOG_DATA_DCF  DEBUG_LOG_DATA

#define DCF_V1_HEADER_SIZE                          1
#define DCF_V1_HEADER_VERSION                       0x1
#define DCF_V1_HEADER_VISIBILITY_ENCRYPTED          0x0
#define DCF_V1_HEADER_VISIBILITY_UNENCRYPTED        0x1

#define DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE  16
#define DCF_V1_IDENTITY_ADDITIONAL_DATA_SIZE        2
#define DCF_V1_IDENTITY_PUBLIC_SIZE                 DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE
#define DCF_V1_IDENTITY_PRIVATE_SIZE                (DCF_V1_IDENTITY_ADDITIONAL_DATA_SIZE + DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE)
#define DCF_V1_IDENTITY_TRUSTED_SIZE                (DCF_V1_IDENTITY_ADDITIONAL_DATA_SIZE + DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE)

#define DCF_V1_DATA_SET_LENGTH_SIZE                 2

#define DCF_V1_MIC_SIZE                             16

#define DCF_V1_LENGTH_TYPE_MIN_LENGTH               1

#define MAX_DATA_ELEMENT_SIZE   126

static inline bool dcfV1Format_DoesIdentityUseEncryption(dcf_v1_identity_t identity)
{
    return (identity == dcf_v1_public_identity ? FALSE : TRUE);
}

static inline uint8 dcfV1Format_PopulateHeader(uint8 * data, uint8 max_data_size, dcf_v1_identity_t identity)
{
    PanicFalse(DCF_V1_HEADER_SIZE <= max_data_size);
    data[0] = (DCF_V1_HEADER_VERSION << 5) | ((dcfV1Format_DoesIdentityUseEncryption(identity) ? DCF_V1_HEADER_VISIBILITY_ENCRYPTED : DCF_V1_HEADER_VISIBILITY_UNENCRYPTED) << 4);
    DEBUG_LOG_DCF("dcfV1Format_PopulateHeader length=%d", DCF_V1_HEADER_SIZE);
    DEBUG_LOG_DATA_DCF(data, DCF_V1_HEADER_SIZE);
    return DCF_V1_HEADER_SIZE;
}

static inline uint8 dcfV1Format_GenerateInitialisationVector(uint8 * data, uint8 max_data_size)
{
    PanicFalse(DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE <= max_data_size);
    for(uint8 i = 0; i < DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE; i++)
    {
        data[i] = (UtilRandom() & 0xFF);
    }
    DEBUG_LOG_DCF("dcfV1Format_GenerateInitialisationVector length=%d", DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE);
    DEBUG_LOG_DATA_DCF(data, DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE);
    return DCF_V1_IDENTITY_INITIALISATION_VECTOR_SIZE;
}

static inline uint8 dcfV1Format_GenerateAdditionalData(uint8 * data, uint8 max_data_size)
{
    PanicFalse(DCF_V1_IDENTITY_ADDITIONAL_DATA_SIZE <= max_data_size);
    // trusted/private still to be implemented
    UNUSED(data);
    Panic();
    return DCF_V1_IDENTITY_ADDITIONAL_DATA_SIZE;
}

static inline uint8 dcfV1Format_PopulateIdentity(uint8 * data, uint8 max_data_size, dcf_v1_identity_t identity)
{
    uint8 identity_size = 0;
    if(identity != dcf_v1_public_identity)
    {
        identity_size += dcfV1Format_GenerateAdditionalData(&data[identity_size], max_data_size);
    }
    identity_size += dcfV1Format_GenerateInitialisationVector(&data[identity_size], (max_data_size - identity_size));
    DEBUG_LOG_DCF("dcfV1Format_PopulateIdentity length=%d", identity_size);
    DEBUG_LOG_DATA_DCF(data, identity_size);
    return identity_size;
}

static inline uint8 dcfV1Format_PopulateMic(uint8 * data, uint8 max_data_size, dcf_v1_identity_t identity)
{
    uint8 mic_size = 0;
    if(dcfV1Format_DoesIdentityUseEncryption(identity))
    {
        PanicFalse(DCF_V1_MIC_SIZE <= max_data_size);
        // trusted/private still to be implemented
        UNUSED(data);
        Panic();
    }
    return mic_size;
}

static inline uint8 dcfV1Format_CalculateTagFieldLength(uint32 data_element_type)
{
    uint8 tag_field_length = 0;
    if(data_element_type <= 0x000F)
    {
        tag_field_length = 0;
    }
    else if(data_element_type <= 0x007F)
    {
        tag_field_length = 1;
    }
    else if(data_element_type <= 0x3FFF)
    {
        tag_field_length = 2;
    }
    else if(data_element_type <= 0x1FFFFF)
    {
        tag_field_length = 3;
    }
    else
    {
        Panic();
    }
    return tag_field_length;
}

static inline uint8 dcfV1Format_PopulateLengthTypeForDataElement(uint8 * data, uint8 max_data_size, uint32 data_element_type, uint8 data_element_length)
{
    uint8 tag_field_length = dcfV1Format_CalculateTagFieldLength(data_element_type);
    uint8 length_type_length = (tag_field_length + DCF_V1_LENGTH_TYPE_MIN_LENGTH);
    PanicFalse(max_data_size >= length_type_length);
    PanicFalse(data_element_length <= (127 - tag_field_length));
    switch(length_type_length)
    {
        case 1:
            PanicFalse(data_element_length <= 0x07);
            data[0] = (((data_element_length << 4) & 0x70) | (data_element_type & 0x0F));
            break;
        case 2:
            data[0] = (0x80 | (data_element_length + tag_field_length));
            data[1] = (data_element_type & 0x7F);
            break;
        case 3:
            data[0] = (0x80 | (data_element_length + tag_field_length));
            data[1] = (0x80 | data_element_type);
            data[2] = ((data_element_type >> 7) & 0x7F);
            break;
        case 4:
            data[0] = (0x80 | (data_element_length + tag_field_length));
            data[1] = (0x80 | data_element_type);
            data[2] = (0x80 | (data_element_type >> 7));
            data[3] = ((data_element_type >> 14) & 0x7F);
            break;
        default:
            Panic();
            break;
    }
    DEBUG_LOG_DCF("dcfV1Format_PopulateLengthTypeForDataElement tag=%d tag_field_length=%d length=%d", data_element_type, tag_field_length, length_type_length);
    DEBUG_LOG_DATA_DCF(data, length_type_length);
    return length_type_length;
}

static inline uint8 dcfV1Format_PopulateDataElements(uint8 * data, uint8 max_data_size, const dcf_data_element_set_t * data_element_set)
{
    uint8 * data_element = PanicUnlessMalloc(MAX_DATA_ELEMENT_SIZE);
    uint8 data_element_set_size = 0;
    for(int i = 0; i < data_element_set->number_of_constructors; i++)
    {
        uint8 data_element_size = data_element_set->constructors[i].constructor_function(data_element, MAX_DATA_ELEMENT_SIZE);
        PanicFalse(data_element_size <= MAX_DATA_ELEMENT_SIZE);
        data_element_set_size += dcfV1Format_PopulateLengthTypeForDataElement(&data[data_element_set_size], (max_data_size - data_element_set_size), data_element_set->constructors[i].type, data_element_size);
        PanicFalse(data_element_size <= (max_data_size - data_element_set_size));
        memcpy(&data[data_element_set_size], data_element, data_element_size);
        data_element_set_size += data_element_size;
    }
    free(data_element);
    return data_element_set_size;
}

static inline uint8 dcfV1Format_PopulateDataElementSetSize(uint8 * data, uint8 max_data_size, uint16 data_element_set_size)
{
    PanicFalse(DCF_V1_DATA_SET_LENGTH_SIZE <= max_data_size);
    data[0] = data_element_set_size >> 8;
    data[1] = data_element_set_size;
    return DCF_V1_DATA_SET_LENGTH_SIZE;
}

uint8 Dcfv1format_ConstructDataElementSet(uint8 * data, uint8 max_data_size, const dcf_data_element_set_t * data_element_set)
{
    PanicNull((void *)data_element_set);
    DEBUG_LOG_DCF("Dcfv1format_ConstructServiceData data=%p max_size=%d set=%p ident=%d num_cons=%d cons=%p", data, max_data_size, data_element_set, data_element_set->identity, data_element_set->number_of_constructors, data_element_set->constructors);
    PanicNull(data);
    PanicNull(data_element_set->constructors);
    PanicFalse(data_element_set->number_of_constructors);

    uint8 constructed_data_size = 0;
    constructed_data_size += dcfV1Format_PopulateHeader(&data[constructed_data_size], (max_data_size - constructed_data_size), data_element_set->identity);
    constructed_data_size += dcfV1Format_PopulateIdentity(&data[constructed_data_size], (max_data_size - constructed_data_size), data_element_set->identity);
    uint8 data_element_set_size_index = constructed_data_size;
    constructed_data_size += dcfV1Format_PopulateDataElementSetSize(&data[constructed_data_size], (max_data_size - constructed_data_size), 0);
    uint8 data_element_set_size = dcfV1Format_PopulateDataElements(&data[constructed_data_size], (max_data_size - (constructed_data_size)), data_element_set);
    constructed_data_size += data_element_set_size;
    dcfV1Format_PopulateDataElementSetSize(&data[data_element_set_size_index], (max_data_size - data_element_set_size_index), data_element_set_size);
    constructed_data_size += dcfV1Format_PopulateMic(&data[constructed_data_size], (max_data_size - constructed_data_size), data_element_set->identity);
    return constructed_data_size;
}
