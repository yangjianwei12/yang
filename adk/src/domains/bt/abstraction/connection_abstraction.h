#ifndef CONNECTION_ABSTRACTION_H
#define CONNECTION_ABSTRACTION_H
#ifdef USE_SYNERGY
#include <cm_lib.h>
#include <csr_bt_profiles.h>
#include <csr_bt_sdc_support.h>
#include <csr_bt_cmn_sdc_rfc_util.h>
#include <csr_bt_td_db.h>
#include <csr_bt_cm_private_prim.h>
#include <l2cap_prim.h>
#else
#include <connection.h>
#include <connection_no_ble.h>
#define isCLMessageId(id) ((id>=CL_MESSAGE_BASE) && (id <= CL_MESSAGE_TOP))
#endif
#endif // CONNECTION_ABSTRACTION_H
