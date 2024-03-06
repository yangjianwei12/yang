/*******************************************************************************

Copyright (C) 2021 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_isoc_setup_iso_data_path_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ISOC_SETUP_ISO_DATA_PATH_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *      Note:
 *      Ownership of pointers present in the array is transferred to the stack,
 *      however array by itself will still be owned by the caller and it will 
 *      NOT be freed.
 *
 * RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_isoc_setup_iso_data_path_req(
     hci_connection_handle_t handle,
     uint8_t  data_path_direction,
     uint8_t  data_path_id,
     uint8_t  codec_id[],
     uint24_t controller_delay,
     uint8_t  codec_config_length,
     uint8_t  *codec_config_data,
     DM_UPRIM_T **pp_prim
     )
{
    DM_ISOC_SETUP_ISO_DATA_PATH_REQ_T *p_prim;
    uint8_t index, offset, len_remaining;

    p_prim = zpnew(DM_ISOC_SETUP_ISO_DATA_PATH_REQ_T);
    p_prim->type = DM_ISOC_SETUP_ISO_DATA_PATH_REQ;
    p_prim->handle = handle;
    p_prim->data_path_direction = data_path_direction;
    p_prim->data_path_id = data_path_id;

    for (index = 0; index < DM_CODEC_ID_OCTETS_SIZE; index++)
    {
        p_prim->codec_id[index] = codec_id[index];
    }

    p_prim->controller_delay = controller_delay;

    if (codec_config_length > HCI_CODEC_CONFIG_DATA_LENGTH)
    {
        codec_config_length = HCI_CODEC_CONFIG_DATA_LENGTH;
    }

    p_prim->codec_config_length = codec_config_length;

    for (offset = 0, index = 0; offset < codec_config_length;
                                index++, offset += len_remaining)
    {
        len_remaining = codec_config_length - offset;
        if (len_remaining > HCI_CODEC_CONFIG_DATA_PER_PTR)
            len_remaining = HCI_CODEC_CONFIG_DATA_PER_PTR;

        p_prim->codec_config_data[index] = pmalloc(HCI_CODEC_CONFIG_DATA_PER_PTR);
        qbl_memscpy(p_prim->codec_config_data[index], HCI_CODEC_CONFIG_DATA_PER_PTR,
                codec_config_data + offset, len_remaining);
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) p_prim;
    }
    else
    {
        DM_PutMsg(p_prim);
    }
}

