/*!
        Copyright (C) 2015 - 2017 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   tbdaddr.h

\brief  Advertise the services of the TYPED_BD_ADDR_T library
*/

#ifndef __BREDRLE_TP_ADDR_H__
#define __BREDRLE_TP_ADDR_H__

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)

#ifdef __cplusplus
extern "C" {
#endif

/* Copy functions */
extern void tpbdaddr_copy_from_bd_addr(TP_BD_ADDR_T *dst,
                               PHYSICAL_TRANSPORT_T src_tp_type, 
                               uint8_t src_type,
                               const BD_ADDR_T *src);

/* Equality functions */

/* Testing functions */

#ifdef __cplusplus
}
#endif

#endif  /* __BREDRLE_TP_ADDR_H__ */
