/*!
    \copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup bap_profile_client
    \brief      Header file for private defines and functions for BAP Profile client
    @{
*/

#ifndef BAP_PROFILE_CLIENT_PRIVATE_H
#define BAP_PROFILE_CLIENT_PRIVATE_H

#include "bap_profile_client.h"

#if defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE)

#include "bap_client_prim.h"
#include <logging.h>
#include <task_list.h>
#include "bt_types.h"
#include "csip.h"
#include "gatt.h"

#include "kymera_adaptation_audio_protected.h"

/*! Number of BAP devices supported */
#define MAX_BAP_DEVICES_SUPPORTED      (2)

/*! Maximum number of supported CIS */
#define MAX_BAP_CIS_SUPPORTED          (2)

/*! Maximum number of ASE's supported for a use case */
#define MAX_BAP_ASE_SUPPORTED          (4)


/*! \brief BAP Profile client instance state */
typedef enum
{
    /*! BAP client instance in idle/free state */
    bap_profile_client_state_idle,

    /*! Discovery in progress state */
    bap_profile_client_state_discovery,

    /*! BAP client in connected state */
    bap_profile_client_state_connected,

    /*! CSIP client in connected state */
    bap_profile_client_state_disconnecting,
} bap_profile_client_state_t;

/*! \brief Types for getting the BAP instance by comparison */
typedef enum
{
    /*! Get BAP Client instance based on connection identifier */
    bap_profile_client_compare_by_cid,

    /*! Get BAP Client instance by state */
    bap_profile_client_compare_by_state,

    /*! Get BAP Client instance by profile handle */
    bap_profile_client_compare_by_profile_handle,

    /*! Get BAP Client instance by valid/invalid cid */
    bap_profile_client_compare_by_valid_invalid_cid
} bap_profile_client_instance_compare_by_type_t;

/*! \brief BAP CIS Info */
typedef struct
{
    uint16   cis_id;
    uint16   cis_handle;
    uint8    direction;
    bool     is_data_path_ready;
} bap_profile_client_cis_info_t;

/*! \brief BAP CIG Info */
typedef struct
{
    uint8                           cig_id;
    bap_profile_client_cis_info_t   cis_info[MAX_BAP_CIS_SUPPORTED];
} bap_profile_client_cig_info_t;

/*! \brief BAP ASE Info */
typedef struct
{
    uint8                          ase_id;               /*!< ASE Identifier */
    BapAseState                    ase_state;            /*!< ASE State */
    uint8                          framing;              /* qos: framing */
    uint8                          phy;                  /* 1PHY, 2PHY or coded PHY. Depends on the audio stream */
    uint8                          rtn;                  /* qos: retransmission_effort */
    uint16                         transportLatency;     /* qos: transport_latency */
    uint32                         presentationDelayMin; /*!< Presentation delay min */
    uint32                         presentationDelayMax; /*!< Presentation delay min */
    BapCodecConfiguration          codec_config;
    bap_profile_client_cis_info_t  cis;
} bap_profile_ase_info_t;

/*! \brief BAP device instance */
typedef struct
{
    /*! TRUE if specified broadcast source is discovered */
    bool source_found;

    /*! TRUE if collocated source need to be added */
    bool add_collocated;

    /*! Broadcast Source ID */
    uint8  source_id;

    /*! ADV_SID for Broadcast Source */
    uint8 adv_sid;

    /*! BAP Role */
    BapRole role;

    /*! Instance present state */
    bap_profile_client_state_t state;

    /*! Connection Identifier for this BAP client instance */
    gatt_cid_t cid;

    /*! Handle for Broadcast Source Scanning */
    uint16 scan_handle;
    uint16 adv_handle;

    /*! BAP profile Handle */
    BapProfileHandle bap_profile_handle;

    /*! Broadcast ID */
    uint32 bcast_id;
    uint32 bis_index;

    bap_microphone_config_t mic_audio_path;
    bap_media_config_t spkr_audio_path;

    /*! Broadcast Source Address */
    TYPED_BD_ADDR_T source_addr;

    /*! Maximum ASE's supported for a session */
    bap_profile_ase_info_t ase_info[MAX_BAP_ASE_SUPPORTED];
} bap_profile_client_device_instance_t;

/*! \brief BAP profile task data */
typedef struct
{
    /*! BAP Client profile task */
    TaskData   task_data;

    /*! Current CIG information */
    bap_profile_client_cig_info_t cig_info;

    /*! BAP individual device instances */
    bap_profile_client_device_instance_t device_instance[MAX_BAP_DEVICES_SUPPORTED];

    /*! BAP Client callback handler */
    bap_profile_client_callback_handler_t       callback_handler;

    /*! Number of BAP Server currently connected to */
    uint8 number_of_connected_servers;
} bap_profile_client_task_data_t;

/*! \brief BAP client task Data */
extern bap_profile_client_task_data_t *bap_profile_client_taskdata;

/*! \brief Returns the CSIP client context */
#define BapProfileClient_GetContext()           (bap_profile_client_taskdata)

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST_SOURCE) || defined(INCLUDE_LE_AUDIO_BROADCAST_SOURCE) */

#endif /* BAP_CLIENT_PRIVATE_H */

/*! @} */