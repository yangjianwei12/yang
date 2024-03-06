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
.MODULE $M.download_support_lib.dsl_ml_engine_lib_get_fn_table;
.CODESEGMENT PM;
.MINIM;
$_dsl_ml_engine_lib_get_fn_table:
    // Set the return value to 0, by default. If the patch version is
    // correct, then the values would be overwritten by the
    // patch function.
    r0 = 0;
#ifdef ENTRY_POINT_DNLD_ML_ENGINE_LIB_GET_FUNCTION_TABLE
    // always check the patch version, if not compatible
    // return NULL. This will be checked in the capability
    // wrapper and the capability creation will FAIL.
    rMAC = M[$_patched_fw_version];
    Null = rMAC - PATCH_BUILD_ID;
    if NZ rts;
    // patch is compatible, so enable/disable the feature.
    jump ENTRY_POINT_DNLD_ML_ENGINE_LIB_GET_FUNCTION_TABLE;
#else
    /* feature isn't available, just return */
    rts;
#endif
.ENDMODULE;
