/*!
\copyright  Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Script to transition to standalone primary role.
*/

#include "script_engine.h"

#include "tws_topology_procedure_find_role.h"

/* This macro selects the FN element of an entry */
const procedure_fns_t *const standalone_primary_role_procs[] = {
    &proc_find_role_fns
};

const Message standalone_primary_role_procs_data[] = {
    PROC_FIND_ROLE_TIMEOUT_DATA_CONTINUOUS
};

const procedure_script_t standalone_primary_role_script = {
    standalone_primary_role_procs, standalone_primary_role_procs_data,
    ARRAY_DIM(standalone_primary_role_procs),
};
