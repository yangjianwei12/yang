/****************************************************************************
 * Copyright (c) 2022 Qualcomm Technologies International, Ltd
****************************************************************************/
/**
 * \file  dls_vagm_clk_lib_if.h
 * \ingroup support_lib/vagm_clk_lib
 *
 * VA Graph manager support lib interface file. <br>
 *
 */
#ifndef _DLS_VAGM_CLK_LIB_IF_H
#define _DLS_VAGM_CLK_LIB_IF_H

/*****************************************************************************
Include Files
*/
#include "types.h"

/*****************************************************************************
Public functions
*/
extern void vagm_clk_change_req(unsigned clk_mode);
#endif