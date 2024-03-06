/******************************************************************************
 Copyright (c) 2015-2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#include "csr_synergy.h"

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
#include "csr_types.h"
#include "csr_bt_profiles.h"
#include "csr_bt_spp_prim.h"
#ifndef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_sc_private_lib.h"
#endif
#include "csr_bt_spp_common.h"


#ifdef CSR_BT_INSTALL_SPP_EXTENDED

dm_security_level_t CsrBtSppMapSecurityInLevel(CsrUint16 secInputLevel)
{
    dm_security_level_t rval = 0;

    CsrBtScSetSecInLevel(&rval, secInputLevel,
                          CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_INCOMING,
                          CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_INCOMING,
                          CSR_BT_RESULT_CODE_SPP_SUCCESS,
                          CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
    
    return rval;
}

dm_security_level_t CsrBtSppMapSecurityOutLevel(CsrUint16 secOutputLevel)
{
    dm_security_level_t rval = 0;

    CsrBtScSetSecOutLevel(&rval, secOutputLevel,
                           CSR_BT_SERIAL_PORT_MANDATORY_SECURITY_OUTGOING,
                           CSR_BT_SERIAL_PORT_DEFAULT_SECURITY_OUTGOING,
                           CSR_BT_RESULT_CODE_SPP_SUCCESS,
                           CSR_BT_RESULT_CODE_SPP_UNACCEPTABLE_PARAMETER);
    return rval;
}

#endif /* CSR_BT_INSTALL_SPP_EXTENDED */
#endif /* !EXCLUDE_CSR_BT_SPP_MODULE */