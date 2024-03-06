/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Default values for product behaviour config.
*/

#ifndef TWS_TOPOLOGY_DEFAULTS_H
#define TWS_TOPOLOGY_DEFAULTS_H

/* See topology_timeouts_t for description of context */
#define TWSTOPOLOGY_DEFAULT_STOP_TIMEOUT_SEC                            (5)
#define TWSTOPOLOGY_DEFAULT_PEER_FINDROLE_TIMEOUT_SEC                   (3)
#define TWSTOPOLOGY_DEFAULT_PRIMARY_PEER_CONNECT_TIMEOUT_SEC            (11)
#define TWSTOPOLOGY_DEFAULT_SECONDARY_PEER_CONNECT_TIMEOUT_SEC          (12)
#define TWSTOPOLOGY_DEFAULT_POST_IDLE_STATIC_HANDOVER_TIMEOUT_SEC       (6)
#define TWSTOPOLOGY_DEFAULT_POST_IDLE_SECONDARY_WAIT_FOR_PRIMARY_SEC    (2)
#define TWSTOPOLOGY_DEFAULT_POST_IDLE_SECONDARY_OUTOFCASE_TIMEOUT_SEC   (10)
#define TWSTOPOLOGY_DEFAULT_HANDOVER_RETRY_TIMEOUT_mS                   (200)
#define TWSTOPOLOGY_DEFAULT_HANDOVER_ACTIVE_WINDOW_TIMEOUT_SEC          (31)
#define TWSTOPOLOGY_DEFAULT_IDLESTATE_DEVICE_RESET_TIMEOUT_SEC          (10)

/* Default timeout values for initialisation of timeouts memeber of product behaviour config*/
#define TWS_TOPOLOGY_DEFAULT_TIMEOUTS \
{\
    .send_stop_cmd_sec = TWSTOPOLOGY_DEFAULT_STOP_TIMEOUT_SEC,\
    .peer_find_role_sec = TWSTOPOLOGY_DEFAULT_PEER_FINDROLE_TIMEOUT_SEC,\
    .primary_wait_for_secondary_sec = TWSTOPOLOGY_DEFAULT_PRIMARY_PEER_CONNECT_TIMEOUT_SEC,\
    .secondary_wait_for_primary_sec = TWSTOPOLOGY_DEFAULT_SECONDARY_PEER_CONNECT_TIMEOUT_SEC,\
    .post_idle_static_handover_timeout_sec = TWSTOPOLOGY_DEFAULT_POST_IDLE_STATIC_HANDOVER_TIMEOUT_SEC,\
    .post_idle_secondary_wait_for_primary_sec = TWSTOPOLOGY_DEFAULT_POST_IDLE_SECONDARY_WAIT_FOR_PRIMARY_SEC,\
    .post_idle_secondary_out_of_case_timeout_sec = TWSTOPOLOGY_DEFAULT_POST_IDLE_SECONDARY_OUTOFCASE_TIMEOUT_SEC,\
    .max_handover_window_sec = TWSTOPOLOGY_DEFAULT_HANDOVER_ACTIVE_WINDOW_TIMEOUT_SEC,\
    .reset_device_sec = TWSTOPOLOGY_DEFAULT_IDLESTATE_DEVICE_RESET_TIMEOUT_SEC,\
    .handover_retry_ms = TWSTOPOLOGY_DEFAULT_HANDOVER_RETRY_TIMEOUT_mS\
}

#endif // TWS_TOPOLOGY_DEFAULTS_H
