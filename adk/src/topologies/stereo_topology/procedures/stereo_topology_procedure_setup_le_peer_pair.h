 /*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to setup LE for peer pair 
            
*/
#ifndef STEREO_TOPOLOGY_PROC_SETUP_LE_PEER_PAIR_H
#define STEREO_TOPOLOGY_PROC_SETUP_LE_PEER_PAIR_H

#include "stereo_topology_procedures.h"

/*! Structure definining the parameters for the procedure to enable/disable
    LE connectability to handset.
*/
typedef struct
{
    bool enable;
} SETUP_LE_PEER_PAIR_PARAMS_T;

/*! Parameter definition for connectable enable */
extern const SETUP_LE_PEER_PAIR_PARAMS_T stereo_topology_procedure_setup_le_peer_pair_enable;
#define PROC_SETUP_LE_PEER_PAIR_DATA_ENABLE  ((Message)&stereo_topology_procedure_setup_le_peer_pair_enable)

extern const procedure_fns_t stereo_proc_setup_le_peer_pair_fns;

#endif /* STEREO_TOPOLOGY_PROC_SETUP_LE_PEER_PAIR_H */
