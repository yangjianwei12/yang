/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup leabm
    \brief      Data types common to Broadcast Manager functions
    @{
*/

#ifndef LE_BROADCAST_MANAGER_TYPES_H_
#define LE_BROADCAST_MANAGER_TYPES_H_

#include <bdaddr.h>
#include <bt_types.h>

#ifdef USE_SYNERGY
#include <bap_server_prim.h>
#else
#include <bap_server.h>
#endif

#define LE_BM_PA_INTERVAL_UNKNOWN (0xFFFF)

typedef enum
{
    le_bm_bass_status_success = BAP_SERVER_STATUS_SUCCESS,
    le_bm_bass_status_in_progress = BAP_SERVER_STATUS_IN_PROGRESS,
    le_bm_bass_status_invalid_parameter = BAP_SERVER_STATUS_INVALID_PARAMETER,
    le_bm_bass_status_not_allowed = BAP_SERVER_STATUS_NOT_ALLOWED,
    le_bm_bass_status_failed = BAP_SERVER_STATUS_FAILED,
    le_bm_bass_status_bc_source_in_sync = BAP_SERVER_STATUS_BC_SOURCE_IN_SYNC,
    le_bm_bass_status_invalid_source_id = BAP_SERVER_STATUS_INVALID_SOURCE_ID,

} le_bm_bass_status_t;

/*! \brief Periodic Advertising synchronization options */
typedef enum
{
    le_bm_pa_sync_none = 0x00,              /*! Do not synchronize to PA */
    le_bm_pa_sync_past_available = 0x01,    /*! Synchronize to PA - PAST available */
    le_bm_pa_sync_past_not_available = 0x02 /*! Synchronize to PA - PAST not available */

} le_bm_pa_sync_t;

/*! \brief Structure containing information regarding a Broadcast Source subgroup*/
typedef struct
{
/*! Bitfield indicating to which streams to synchronize */
    uint32 bis_sync;
/*! Length of metadata for the subgroup */
    uint8 metadata_length;
/*! Metadata for the subgroup */
    uint8 *metadata;

} le_bm_source_subgroup_t;

#ifndef HOSTED_TEST_ENVIRONMENT
/* The le_bm_source_subgroup_t and GattBassServerSubGroupsData structures should contain the same elements. */
#include <gatt_bass_server.h>
COMPILE_TIME_ASSERT(sizeof(le_bm_source_subgroup_t) == sizeof(GattBassServerSubGroupsData), le_bm_source_subgroup_t_mismatched);
COMPILE_TIME_ASSERT(offsetof(le_bm_source_subgroup_t, bis_sync) == offsetof(GattBassServerSubGroupsData, bisSync), le_bm_source_subgroup_t_mismatched_bis_sync);
COMPILE_TIME_ASSERT(offsetof(le_bm_source_subgroup_t, metadata_length) == offsetof(GattBassServerSubGroupsData, metadataLen), le_bm_source_subgroup_t_mismatched_metadata_length);
COMPILE_TIME_ASSERT(offsetof(le_bm_source_subgroup_t, metadata) == offsetof(GattBassServerSubGroupsData, metadata), le_bm_source_subgroup_t_mismatched_metadata);
#endif


/*! \brief Structure containing information regarding a Broadcast Source to be added */
typedef struct
{
/*! #typed_bdaddr of the Broadcast Source */
    typed_bdaddr advertiser_address;
/*! #typed_bdaddr of the Broadcast Assistant */
    typed_bdaddr  assistant_address;
/*! Advertising Set identifier */
    uint8 advertising_sid;
/*! Broadcast Identifier of the Broadcast Source */
    uint32 broadcast_id;
/*! #scan_delegator_client_pa_sync_t determining if the server should sync with the PA*/
    le_bm_pa_sync_t pa_sync;
/*! Periodic Advertising interval in 1.25 ms units, #LE_BM_PA_INTERVAL_UNKNOWN if unknown */
    uint16 pa_interval;
/*! Number of subgroups */
    uint8 num_subgroups;
/*! Array of #le_bm_broadcast_source_subgroup_t structures for each subgroup */
    le_bm_source_subgroup_t *subgroups;

} le_bm_add_source_info_t;


/*! \brief Structure containing information regarding a Broadcast Source to be modified */
typedef struct
{
/*! #scan_delegator_client_pa_sync_t determining if the server should sync with the PA*/
    le_bm_pa_sync_t pa_sync;
/*! Periodic Advertising interval in 1.25 ms units, 0xFFFF if unknown */
    uint16 pa_interval;
/*! Number of subgroups */
    uint8 num_subgroups;
/*! Array of #le_bm_broadcast_source_subgroup_t structures for each subgroup */
    le_bm_source_subgroup_t *subgroups;

} le_bm_modify_source_info_t;

#endif /* LE_BROADCAST_MANAGER_TYPES_H_ */
/*! @} */