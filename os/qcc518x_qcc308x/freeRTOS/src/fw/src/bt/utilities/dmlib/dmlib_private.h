/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef __DMLIB_PRIVATE_H__
#define __DMLIB_PRIVATE_H__

#include <string.h>
#include "qbl_adapter_panic.h"                   /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_pmalloc.h"                 /* QBLUESTACK_REFACTOR */
#include "qbl_adapter_scheduler.h"               /* QBLUESTACK_REFACTOR */
#include "dmlib.h"
#include INC_DIR(common,bluestacklib.h)
#include INC_DIR(common,common.h)
#include INC_DIR(common,error.h)
#include INC_DIR(mblk,mblk.h)
#include INC_DIR(tbdaddr,tbdaddr.h)

#ifdef __cplusplus
extern "C" {
#endif


/**********************************************/








/*********************************************/


/*lint -sem(dm_put_message, custodial(1)) */
void dm_put_message(void *p_prim, const dm_prim_t type);

#ifdef INSTALL_SM_MODULE
#ifdef SM_HAS_FUNCTION_FOR_AES
extern TYPED_BD_ADDR_T *dm_sm_resolve_address(const TYPED_BD_ADDR_T *const addrt);
#else
TYPED_BD_ADDR_T *sm_get_resolved_addr(const TYPED_BD_ADDR_T *const addrt);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DMLIB_PRIVATE_H__ */
