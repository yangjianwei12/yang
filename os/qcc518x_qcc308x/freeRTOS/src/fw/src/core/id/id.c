/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */
/*
 * The patched fw version, plus bcd version for usb
 */

#include "id/id.h"

uint32 patched_fw_version = 0;

#ifdef DESKTOP_TEST_BUILD
#define get_build_id_number() (0xFA11FA11)
#else
/* Need this to be a true function so it can be called from Pylib */
uint32 get_build_id_number(void)
{
    return build_id_number;
}
#endif

uint16 id_get_build_bcd(void)
{
    uint16 shift,bcd = 0;
    uint32 version = patched_fw_version != 0 ?
        patched_fw_version : build_id_number;

    /* Pull each decimal digit out and turn it into the literally identical
     * hex digit */
    for (shift = 0; shift < 16; shift += 4)
    {
        bcd |= (uint16)((version % 10) << shift);
        version /= 10;
    }

    return bcd;
}

uint32 get_p1_build_id_number(void)
{
    uint32 *slt_loc;

    /* SLT header is pair of [Key : Pointer], where key is APPS_SLT_FINGERPRINT
    * and value is pointer to the actual data table */
    /* Check the slt fingerprint */
    if (*SLT_LOCATION_HEAD_PTR != APPS_SLT_FINGERPRINT)
    {
        return 0;
    }

    /* SLT is a list of pairs of [Key : Pointer].
     * We need to scan the array find the Key value for the build ID then use
     * the associated pointer to retrieve the build ID.
     * For that first we need to fetch pointer to the actual data table which is
     * next to the header.
     */
    slt_loc = (uint32 *)(*(SLT_LOCATION_HEAD_PTR + 1));

    /* Traverse through the table till either the location for
     * APPS_SLT_ID_VERSION_INTEGER is found or table gets terminated.
     */
    while((*slt_loc != APPS_SLT_ID_VERSION_INTEGER) &&
          (*slt_loc != APPS_SLT_ID_NONE))
    {
        slt_loc += 2;
    }

    /* Check if we found the location for APPS_SLT_ID_VERSION_INTEGER */
    if(*slt_loc == APPS_SLT_ID_VERSION_INTEGER)
    {
        /* Since it is a key, value pair; value pointer is stored at next loc */
        /* Get the value of location pointer */
        uint32 *loc_ver_int = (uint32 *)(*(slt_loc + 1));
        /* Fetch the value of version integer */
        return *loc_ver_int;
    }
    else
    {
        return 0;
    }
}

