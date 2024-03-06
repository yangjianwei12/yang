/*!
\copyright  Copyright (c) 2022  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       kymera_msg.h
\brief      Kymera message helper
*/

#ifndef KYMERA_MSG_H_
#define KYMERA_MSG_H_

#include <message.h>

/*! \brief Notify registered clients. */
void appKymera_MsgRegisteredClients(MessageId id, uint16 info);

#endif // KYMERA_MSG_H_
