/**
 * \file  va_pryon_lite_struct.h
 * \ingroup capabilities
 *
 *
 */

#ifndef VA_PRYON_LITE_STRUCT_H
#define VA_PRYON_LITE_STRUCT_H

#include "pryon_lite_PRL1000.h"
#include "wwe/wwe_struct.h"
#include "apva_gen_c.h"
/****************************************************************************
Public Type Definitions
*/
#define MAX_NUM_CLIENT_PROPERTIES 10

typedef struct client_property_data{
    int group_id;
    int property_id;
    int property_value;
} client_property_data;

typedef struct VA_PRYON_LITE_OP_DATA{
    WWE_CLASS_DATA wwe_class;

    PryonLiteWakewordConfig wakewordConfig;
    PryonLiteV2Handle handle;
    PryonLiteV2EventConfig engineEventConfig;
    PryonLiteV2ConfigAttributes configAttributes;
    PryonLiteV2Config engineConfig;
    
    char *engineBuffer;
    size_t sizeofengineBuffer;

    APVA_PARAMETERS apva_cap_params;
    int16 *p_buffer;
    void *f_handle;

    int pryon_metadata_blob_size;  // In bytes
    const char* pryon_metadata_blob;

    int num_client_properties_preserved;
    client_property_data* cpd[MAX_NUM_CLIENT_PROPERTIES];

} VA_PRYON_LITE_OP_DATA;

#endif /* VA_PRYON_LITE_H */
