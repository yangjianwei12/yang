/*!
    \copyright Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd. 
    \version  
    \file
    \addtogroup tx_power
    \brief
    @{
*/

#ifndef TX_POWER_MSG_HANDLER_H_
#define TX_POWER_MSG_HANDLER_H_

/*!
    \brief Handler for external messages sent to the tx power module.
*/
void TxPowerTaskHandler(Task task, MessageId id, Message message);

Task TxPower_GetTaskData(void);

#endif
/*! @} */