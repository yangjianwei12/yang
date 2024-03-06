/***************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
**************************************************************/
/* CACHE_MIB_DEFAULT_2WAY signals a SQIF config, which implies a KATS 
 * test patch. INSTALL_SWITCHABLE_CACHE is undefined for kalsim builds.
 * In both cases, don't try to include the ROM build specific exported
 * symbols.
 */
#if defined(INSTALL_SWITCHABLE_CACHE) && !defined(CACHE_MIB_DEFAULT_2WAY) && defined(RUN_CAPABILITIES_FROM_SQIF)
#include "subsys3_patch0_fw00003914_map_public.h"
#endif
.MODULE $M.download_support_lib.vol_ctrl_low_event_get;
.CODESEGMENT PM;
.MINIM;
$_vol_ctrl_low_event_get:
#ifdef ENTRY_POINT_VOL_CTRL
    rMAC = M[$_patched_fw_version];
    Null = rMAC - PATCH_BUILD_ID;
    if NZ rts;
    // patch is compatible, so enable/disable the feature.
    jump ENTRY_POINT_VOL_CTRL;
#else
    /* feature isn't available, just return */
    rts;
#endif
.ENDMODULE;
