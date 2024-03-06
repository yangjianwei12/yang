/****************************************************************************
 * Copyright (c) 2013 - 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_for_adaptors.h
 * \ingroup opmgr
 *
 * Operator Manager header file used by adaptor(s). <br>
 *
 */

#ifndef _OPMGR_FOR_ADAPTORS_H_
#define _OPMGR_FOR_ADAPTORS_H_

#include "types.h"
#include "cap_id_prim.h"
#include "opmgr_common.h"

typedef bool (*get_mips_usage_cback)(CONNECTION_LINK conidx,
                                     unsigned list_length,
                                     uint16 *op_list,
                                     uint16 *usage);

#if defined(PROFILER_ON)
/**
 * Function to get the operator mips usage. Sends a request to P1 if any of the
 * operators are on P1.
 * \param con_id        The connection ID.
 * \param op_list       The list of the operators in the request.
 * \param list_length   The number of operators in the request.
 * \param callback      The callback that sends an ACCMD response.
 * \return Returns True if the request has been dealt with successfully and 
           FALSE otherwise.
 */
extern bool opmgr_handle_get_operator_mips_req(CONNECTION_LINK con_id,
                                               uint16 *op_list,
                                               unsigned list_length,
                                               get_mips_usage_cback callback);
/**
 * Function that executes the callback that sends an ACCMD response and frees up
 * any memory.
 */
extern void opmgr_send_get_mips_usage_resp(void);

/**
 * Function to get the operator mips usage.

 * \return Returns TRUE if any operators in the op_list is on P1.
 */
extern bool opmgr_get_operator_mips(void);
#else
#define opmgr_handle_get_operator_mips_req(a, b, c, d) FALSE
#endif /* defined(PROFILER_ON) */

/**
 * Check the "discardable" status of an unsolicited message.
 * If the OPMSG_REPLY_ID_DISCARDABLE_MESSAGE bit is set, clear it 
 * and return TRUE. Otherwise return FALSE.
 *
 * \return TRUE if the message is a discardable one.
 */
extern bool opmgr_check_discardable_unsolicited_msg(OP_UNSOLICITED_MSG *opmsg);

#endif /* _OPMGR_FOR_ADAPTORS_H_ */

