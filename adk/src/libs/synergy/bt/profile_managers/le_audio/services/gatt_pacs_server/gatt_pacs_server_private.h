/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_PACS_SERVER_PRIVATE_H
#define GATT_PACS_SERVER_PRIVATE_H

#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_core_stack_pmalloc.h"

#include "gatt_pacs_server.h"
#include "gatt_pacs_server_db.h"

/* default client config 0xFFFF means remote client has not written
 * any CCCD which is as good as CCCD disable */
#define GATT_PACS_SERVER_INVALID_CLIENT_CONFIG   0xFFFF

/* Macros for creating messages */
#define MAKE_PACS_SERVER_MESSAGE(TYPE) TYPE##_T* message = (TYPE##_T *)CsrPmemAlloc(sizeof(TYPE##_T))


#define GATT_PACS_SERVER_CCC_NOTIFY                   (0x01)
#define GATT_PACS_SERVER_CCC_INDICATE                 (0x02)

#define GATT_PACS_CLIENT_CONFIG_MASK (GATT_PACS_SERVER_CCC_NOTIFY | GATT_PACS_SERVER_CCC_INDICATE)
#define GET_PACS_CLIENT_CONFIG(config)          (config & GATT_PACS_CLIENT_CONFIG_MASK )

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_PACS_SERVER_CCC_VALUE_SIZE           (sizeof(uint8) * 2)

#define GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE       (sizeof(uint8) * 4)
#define GATT_PACS_AUDIO_LOCATION_VALUE_SIZE       (sizeof(uint8) * 4)

/* Max PAC records supported */
#define MAX_PAC_RECORD_HANDLES          0x000F
#define DEFAULT_PAC_RECORD_HANDLE_MASK  0x8000

#define PACS_PREFERRED_AUDIO_CONTEXTS_TYPE    0x1
#define PACS_VENDOR_SPECIFIC_METATDATA_TYPE   0xFF

/* 3 SINK/SRC handles for LC3
 */
#define NUM_SINK_PAC_RECORD_HANDLES  3
#define NUM_SRC_PAC_RECORD_HANDLES  3

typedef struct
{
    ConnectionIdType                cid;
    GattPacsServerConfigType      client_cfg;
    uint32            selectiveAudioContexts;
} pacs_client_data;

typedef struct pac_record_list
{
   const GattPacsServerRecordType *pac_record;
   uint16                          pac_record_handle;
   bool                            consumed;
   bool                            vendorMetadataPresent;
   struct pac_record_list          *next;
} pac_record_list;

typedef struct pac_record_list_vs
{
   const GattPacsServerVSPacRecord *pac_record;
   uint16                           pac_record_handle;
   struct pac_record_list_vs        *next;
} pac_record_list_vs;

/*! @brief Definition of data required for association.
 */
typedef struct
{
    pac_record_list                *sink_pack_record;
    pac_record_list                *source_pack_record;
    pac_record_list_vs             *vs_aptx_sink_pac_record;
    pac_record_list_vs             *vs_aptx_source_pac_record;
    uint32                         sink_audio_source_location;
    uint32                         source_audio_source_location;
    uint32                         supported_audio_contexts;
    uint32                         available_audio_contexts;
    uint16                         pacs_record_handle_mask;
    bool                           audioContextAvailabiltyControlApp;
    pacs_client_data               connected_clients[GATT_PACS_MAX_CONNECTIONS];
} pacs_data;

/*! @brief The PACS server internal structure for the server role.

    This structure is NOT visible to the application
*/
typedef struct _gatt_pacs_server_t
{
    AppTaskData lib_task;
    AppTask app_task;

    CsrBtGattId  gattId;                               /*! Registered GATT ID */

    /* Service handle provided by the service_handle lib when the server
     * memory instance is created
     */
    PacsServiceHandleType srvc_hndl;
    uint16 start_handle;
    uint16 end_handle;

    /*! Information to be provided in service characteristics. */
    pacs_data data;
} GPACSS_T;

#define MAKE_PACS_MESSAGE(TYPE) \
    TYPE * message = (TYPE *)CsrPmemZalloc(sizeof(TYPE))

#define PacsServerMessageSend(_appTask, _msg) \
                      CsrSchedMessagePut(_appTask, PACS_SERVER_PRIM, ((void *)_msg))

#endif /* GATT_PACS_SERVER_PRIVATE_H */

