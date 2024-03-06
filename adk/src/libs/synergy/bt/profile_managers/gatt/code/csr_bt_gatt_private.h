#ifndef _CSR_BT_GATT_PRIVATE_H_
#define _CSR_BT_GATT_PRIVATE_H_
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_sched.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_message_queue.h"
#include "csr_log_text_2.h"
#include "csr_bt_util.h"
#include "csr_bt_tasks.h"
#include "att_prim.h"
#include "attlib.h"
#include "tbdaddr.h"
#include "l2cap_prim.h"
#include "l2caplib.h"
#include "dm_prim.h"
#include "dmlib.h"
#ifndef EXCLUDE_CSR_BT_CM_MODULE
#include "csr_bt_cm_private_lib.h"
#endif
#include "csr_bt_gatt_config.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "csr_bt_gatt_utils.h"
#include "csr_bt_gatt_private_prim.h"
#include "csr_bt_uuids.h"
#include "csr_list.h"
#include "csr_bt_gatt_main.h"

#include "csr_bt_gatt_private_utils.h"
#ifndef CSR_TARGET_PRODUCT_VM
#include "csr_bt_core_stack_fsm.h"
/*#include "csr_bt_gatt_conn_genfsm.h"*/
#endif

#include "csr_bt_gatt_sef.h"
#include "csr_bt_gatt_att_sef.h"
#include "csr_bt_gatt_cm_sef.h"
#include "csr_bt_gatt_sef.h"
#include "csr_bt_gatt_upstream.h"


#include "csr_bt_gatt_dm_sef.h"
#if defined(CSR_BT_GATT_CACHING) || defined(CSR_BT_GATT_INSTALL_EATT)
#include "csr_bt_gatt_tddb_utils.h"
#endif
#include "csr_bt_td_db_gatt.h"

#ifdef GATT_DATA_LOGGER
#include "gatt_data_logger_prim.h"
#endif

/* Maximum number of connections supported by connection manager is 5 */
#define GATT_MAX_CONNECTIONS 5

#define SERVICE_CHANGED_INDICATION_IDLE      0x00
#define SERVICE_CHANGED_INDICATION_SERVED    0x01
#define SERVICE_CHANGED_INDICATION_DISABLED  0x02

#ifdef __cplusplus
extern "C" {
#endif

/* Empty */

#ifdef __cplusplus
}
#endif

#endif
