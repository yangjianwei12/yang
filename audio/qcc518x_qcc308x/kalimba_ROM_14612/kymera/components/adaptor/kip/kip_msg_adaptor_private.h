/****************************************************************************
 * Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file kip_msg_adaptor_private.h
 * \ingroup kip
 *
 * This file contains data types and API private to the kip adaptor component.
 */

#ifndef _KIP_MSG_ADAPTOR_PRIVATE_H_
#define _KIP_MSG_ADAPTOR_PRIVATE_H_

#include "kip_msg_adaptor.h"

#define KIP_CONID_SEND_CLIENT_MASK    0xF800
#define KIP_CONID_RECV_CLIENT_MASK    0x00F8

static inline bool kip_msg_is_request(KIP_MSG_ID msg_id)
{
    return KIP_MSG_ID_IS_REQ(msg_id);
}

static inline KIP_MSG_ID kip_msg_to_request(KIP_MSG_ID msg_id)
{
    uint16 id;
    id = msg_id & ~KIP_MSG_ID_RESP_MASK;
    return (KIP_MSG_ID) id;
}

#endif /* _KIP_MSG_ADAPTOR_PRIVATE_H_ */
