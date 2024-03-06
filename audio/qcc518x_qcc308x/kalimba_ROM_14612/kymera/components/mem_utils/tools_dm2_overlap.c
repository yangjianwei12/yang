/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  tools_dm2_overlap.c
 * \ingroup mem_utils
 *
 * Dummy allocation to prevent any other useful firmware variable
 * being declared at the dm2 overlap address with tools. This dm2
 * overlap address is used as a special marker by tools and cannot
 * be used by any other firmware symbol until B-295425 is fixed in tools.
 * This will not break heap continuity because we do not care
 * about what happens to this dummy variable.
 *
 * Inclusion of this file is controlled by the build variable
 * "BUILD_TOOLS_DM2_OVERLAP_WORK_AROUND"
 *
 */
 
#if !defined(__GNUC__)
_Pragma("datasection TOOLS_DM2_OVERLAP") unsigned tools_dm2_dummy;
#endif