/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_cap
    \brief      Private defines and types for CAP client profile.
    @{
*/

#ifndef CAP_PROFILE_CLIENT_INSTANCE_H
#define CAP_PROFILE_CLIENT_INSTANCE_H

#include "cap_profile_client.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "cap_client_prim.h"
#include <task_list.h>
#include "gatt.h"

/*! Number of CAP groups supported */
#define MAX_CAP_GROUP_SUPPORTED        (1)

/*! Number of devices supported within a group */
#define MAX_CAP_DEVICES_SUPPORTED      (2)

#define CAP_LTV_LENGTH_OFFSET               0x00
#define CAP_LTV_TYPE_OFFSET                 0x01
#define CAP_LTV_VALUE_OFFSET                0x02

#define CAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC                  0xFF
#define CAP_VS_METADATA_COMPANY_ID_QUALCOMM_SIZE               0x02
#define CAP_VS_METADATA_TYPE_FT_REQUESTED_SETTING              0xFE
#define CAP_VS_METADATA_FT_REQUEST_VALUE_LENGTH                5

#define CAP_NUMBER_OF_BIS_SUPPORTED        0x02

/*! \brief CAP client instance state */
typedef enum
{
    /*! CAP client instance in idle/free state */
    cap_profile_client_state_idle,

    /*! Discovery in progress state */
    cap_profile_client_state_discovery,

    /*! CAP client in connected state */
    cap_profile_client_state_connected,

    /*! CAP client in disconnecting state */
    cap_profile_client_state_disconnecting,
} cap_profile_client_state_t;

/*! \brief CAP client instance state */
typedef enum
{
    /*! Get CAP Client instance based on connection identifier */
    cap_profile_client_compare_by_cid,

    /*! Get CAP Client instance by state */
    cap_profile_client_compare_by_state,

    /*! Get CAP Client instance by bdaddr */
    cap_profile_client_compare_by_bdaddr,

    /*! Get CAP Client instance by valid/invalid cid */
    cap_profile_client_compare_by_valid_invalid_cid
} cap_profile_client_instance_compare_by_type_t;

typedef struct
{
    /*! Number of Audio capabilities supported */
    uint8                   stream_cap_count;

    /*! number of devices supported */
    uint8                   device_count;

    /*! Supported Audio Context */
    uint32                  supported_context;

    /*! Audio Stream capabilities list */
    CapClientStreamCapability     *capability;

    /*! Device List data */
    CapClientDeviceInfo           *device_list;
} cap_profile_client_capabilties_info_t;

/*! \brief CAP device information */
typedef struct
{
    /*! Connection Identifier for this CAP client instance */
    gatt_cid_t                 cid;

    /*! Instance present state */
    cap_profile_client_state_t  state;
} cap_profile_client_device_info_t;

/*! \brief CAP Client Instance information */
typedef struct
{
    /*! Connected device's count */
    int8                                    device_count;

    /*! Size of the coordinated set */
    uint8                                   coordinated_set_size;

    /*! Cig ID */
    uint8                                   cig_id;

    /*! CAP client role */
    uint8                                   role;

    /*! SIRK */
    uint8                                   sirk[CSIP_SIRK_SIZE];

    /*! CAP Service Handle */
    ServiceHandle                           cap_group_handle;

    /*! Available audio context */
    uint32                                  available_audio_context;

    /*! Remote CAP Server capabilties */
    cap_profile_client_capabilties_info_t   cap_info;

    /*! CAP Service Handle */
    cap_profile_client_device_info_t        device_info[MAX_CAP_DEVICES_SUPPORTED];
} cap_profile_group_instance_t;

typedef struct
{
    /*! TRUE if Assistant is scanning for collocated sources else FALSE */
    bool                                    scan_for_colllocated;

    /*! TRUE if Source discovered on scan is collocated else FALSE */
    bool                                    source_found;
    uint8                                   adv_sid;

    /*! Source Identifier */
    uint8                                   source_id;
    CapClientPaSyncState                    pa_sync_state;
    uint8                                   big_encryption;

    /*! Handle for PA sync */
    uint16                                  sync_handle;

    /*! Handle for Broadcast source scanning */
    uint16                                  scan_handle;
    uint16                                  bis_handles[CAP_NUMBER_OF_BIS_SUPPORTED];

    /*! Handle for Broadcast Source role */
    uint32                                  bcast_handle;
    uint32                                  broadcast_id;

    /*! Broadcast Source address */
    TYPED_BD_ADDR_T                         source_addr;
    BapBigSubgroup                          *subgroup_info;
}cap_profile_client_broadcast_data_t;

/*! \brief CAP profile task data */
typedef struct
{
    /*! CAP Client profile task */
    TaskData                                    task_data;

   /*! CAP Client profile callback handler */
    cap_profile_client_callback_handler_t       callback_handler;

   /*! CAP control client instance */
    cap_profile_group_instance_t                cap_group_instance[MAX_CAP_GROUP_SUPPORTED];

   /*! Parameters associatd with Broadcast */
    cap_profile_client_broadcast_data_t         cap_broadcast_data;
} cap_profile_client_task_data_t;

/*! \brief CAP client task Data */
extern cap_profile_client_task_data_t cap_client_taskdata;

/*! \brief Returns the CAP client context */
#define CapProfileClient_GetContext()           (&cap_client_taskdata)

/*! \brief Returns the CAP client context */
#define CapProfileClient_GetTask()              (&cap_client_taskdata.task_data)

/*! \brief Get the CAP device count */
#define capProfileClient_GetDeviceCount()         (cap_client_taskdata.device_count)

/*! \brief Get the size of coordinated set */
#define capProfileClient_GetCoordinatedSetSize()  (cap_client_taskdata.coordinated_set_size)

/*! \brief Method used to store discovered CAA Server handle data to NVM */
bool CapProfileClient_StoreClientHandles(gatt_cid_t cid, void *config, uint8 size);

/*! \brief Method used to retrieve CAS handle data from NVM */
void* CapProfileClient_RetrieveClientHandles(gatt_cid_t cid);

/*! \brief Get the CAP client instance based on the compare type */
cap_profile_client_device_info_t * CapProfileClient_GetDeviceInstance(cap_profile_group_instance_t *group_instance,
                                                                      cap_profile_client_instance_compare_by_type_t type, unsigned cmp_value);

/*! \brief Reset the provided cap client instance */
void CapProfileClient_ResetCapClientInstance(cap_profile_client_device_info_t *device_instance);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* CAP_PROFILE_CLIENT_INSTANCE_H */

/*! @} */