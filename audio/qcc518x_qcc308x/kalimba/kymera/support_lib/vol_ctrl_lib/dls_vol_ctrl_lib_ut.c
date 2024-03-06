/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  dls_vol_ctrl_lib_ut.c
 * \ingroup support_lib/vol_ctrl_lib
 *
 * VA Graph manager support lib C file. <br>
 *
 */

/*****************************************************************************
Include Files
*/
#include "dls_vol_ctrl_lib_if.h"

/*****************************************************************************
Public functions
*/
#if defined(UNIT_TEST_BUILD) || defined(CACHE_MIB_DEFAULT_2WAY) || defined(INSTALL_2WAY_CACHE)
/* This is a dummy function to keep the UT/SQIF builds happy.
* The actual implementation of this function is in
* dls_vol_ctrl_lib_asm.asm. This file shall not be
* exposed in an audio package
*/
unsigned vol_ctrl_low_event_get(void)
{
    return 1000; /*dummy value for UT*/
}
#endif /* UNIT_TEST_BUILD */
