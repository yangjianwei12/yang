/*******************************************************************************

Copyright (C) 2019 - 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_ext_scan_register_scanner_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_ext_scan_register_scanner_req(
    phandle_t phandle,
    uint32_t  flags,
    uint16_t  adv_filter,
    uint16_t  adv_filter_sub_field1,
    uint32_t  adv_filter_sub_field2,
    uint16_t  ad_structure_filter,
    uint16_t  ad_structure_filter_sub_field1,
    uint32_t  ad_structure_filter_sub_field2,
    uint16_t  ad_structure_info_len,
    uint8_t  *ad_structure_info,
    DM_UPRIM_T **pp_prim
    )
{
    uint8_t index, offset, element_len;
    DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ_T *prim =
            zpnew(DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ_T);

    if (ad_structure_info_len > DM_ULP_AD_STRUCT_INFO_LENGTH)
    {
        BLUESTACK_PANIC(PANIC_INVALID_BLUESTACK_PRIMITIVE);
    }

    prim->type = DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ;
    prim->phandle = phandle;
    prim->flags = flags;
    prim->adv_filter = adv_filter;
    prim->adv_filter_sub_field1 = adv_filter_sub_field1;
    prim->adv_filter_sub_field2 = adv_filter_sub_field2;
    prim->ad_structure_filter = ad_structure_filter;
    prim->ad_structure_filter_sub_field1 = ad_structure_filter_sub_field1;
    prim->ad_structure_filter_sub_field2 = ad_structure_filter_sub_field2;
    prim->ad_structure_info_len = ad_structure_info_len;

    for(offset = 0, index = 0; offset < prim->ad_structure_info_len;
                               index++, offset += element_len)
    {
        element_len = prim->ad_structure_info_len - offset;
        if(element_len > DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR)
            element_len = DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR;

        prim->ad_structure_info[index] = pmalloc(DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR);
        qbl_memscpy(prim->ad_structure_info[index], DM_ULP_AD_STRUCT_INFO_BYTES_PER_PTR,
                ad_structure_info + offset, element_len);
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}
