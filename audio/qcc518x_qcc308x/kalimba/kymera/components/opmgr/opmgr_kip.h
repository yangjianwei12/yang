/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_kip.h
 * \ingroup  opmgr
 *
 * Operator Manager KIP-related parts. <br>
 */

#ifndef OPMGR_KIP_H
#define OPMGR_KIP_H

/****************************************************************************
Include Files
*/
#include "opmgr.h"
#include "kip_msg_prim.h"
#include "cap_id_prim.h"

/****************************************************************************
Function Definitions
*/

/* Send 'list' type command via KIP: stop, reset, start, destroy */
extern bool opmgr_kip_build_send_list_cmd(CONNECTION_LINK con_id,
                                          unsigned num_ops,
                                          unsigned *op_list,
                                          KIP_MSG_ID kip_msg_id);

/**
 * \brief Handle the create_operator_ex request from kip adaptor.
 *
 * \param con_id   Connection id.
 * \param cap_id   The id of the capability the operator is an instance of.
 * \param op_id    The created operator id.
 * \param priority The priority of the task associated with the operator.
 * \param callback The callback function of create operator for KIP.
 */
void opmgr_kip_create_operator_ex_req_handler(CONNECTION_LINK con_id,
                                              CAP_ID cap_id,
                                              EXT_OP_ID op_id,
                                              PRIORITY priority,
                                              OP_CREATE_CBACK callback);

/**
 * \brief Handle the operator message request from kip adaptor
 *
 * \param con_id    Connection id
 * \param status    Generic Kymera status/error code
 * \param op_id     The created operator id
 * \param num_param Length of parameters in operator message
 * \param params    Parameters in the operator message
 * \param callback  The callback function of operator message
 */
void opmgr_kip_operator_message_req_handler(CONNECTION_LINK con_id,
                                            STATUS_KYMERA status,
                                            EXT_OP_ID op_id,
                                            unsigned num_param,
                                            unsigned *params,
                                            OP_MSG_CBACK callback);
                  
/**
 * \brief Handle the create operator response from kip
 *
 * \param con_id  Connection id
 * \param status  Status of the request
 * \param op_id  The created operator id
 * \param callback  The callback function of create operator
 */
extern void opmgr_kip_create_resp_handler(CONNECTION_LINK con_id,
                                          STATUS_KYMERA status,
                                          EXT_OP_ID op_id);

/**
 * \brief Handle the std list operator (start/stop/reset/destroy) response
 *        from kip.
 *
 * \param con_id    Connection id
 * \param status    Status of the request
 * \param count     Number of operators handled successfully
 * \param err_code  Error code upon operator error
 */
extern void opmgr_kip_stdlist_resp_handler(CONNECTION_LINK con_id,
                                           STATUS_KYMERA status,
                                           unsigned count,
                                           unsigned err_code);

/**
 * \brief Handle the operator message response from kip
 *
 * \param con_id           Connection id
 * \param status           Status of the request
 * \param op_id            The created operator id
 * \param num_resp_params  Length of response message
 * \param resp_params      Response message payload
 * \param context          The callback function of operator message from kip
 */
extern void opmgr_kip_operator_msg_resp_handler(CONNECTION_LINK con_id,
                                                STATUS_KYMERA status,
                                                EXT_OP_ID op_id,
                                                unsigned num_resp_params,
                                                unsigned *resp_params);

#ifdef PROFILER_ON
/**
 * \brief Send a KIP request to P1 to get the MIPS usage for the op IDs on P1.
 *
 * \param con_id            Connection id
 *
 * \return True if successful.
 */
extern bool opmgr_kip_send_get_mips_req(CONNECTION_LINK con_id);
#endif /* PROFILER_ON */
#endif /* OPMGR_KIP_H */
