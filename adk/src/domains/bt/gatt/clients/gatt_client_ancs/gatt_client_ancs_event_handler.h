/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup gatt_client_ancs
    \brief      Application support for GATT ANCS client
    @{
*/

#ifndef GATT_CLIENT_ANCS_EVENT_HANDLER
#define GATT_CLIENT_ANCS_EVENT_HANDLER

/*! \brief GATT ANCs Events */
typedef enum
{
    EventInvalid,
    EventSysAncsOtherAlert,
    EventSysAncsIncomingCallAlert,
    EventSysAncsMissedCallAlert,
    EventSysAncsVoiceMailAlert,
    EventSysAncsSocialAlert,
    EventSysAncsScheduleAlert,
    EventSysAncsNewsAlert,
    EventSysAncsHealthNFittnessAlert,
    EventSysAncsBusinessNFinanceAlert,
    EventSysAncsLocationAlert,
    EventSysAncsEntertainmentAlert,
    EventSysAncsEmailAlert,
}gatt_ancs_event;

/*!
*   \brief  Message handler for when receiving an update, invokes this function.
*                 
*   \param  Task    The task registered for event updates. (unused)
*   \param  id      The message ID containing enum value to be handled in function.        
*   \param  message The message itself. (unused)
*/
void GattClientAncs_eventHandler(Task task, MessageId id, Message message);

#endif  /* GATT_CLIENT_ANCS_EVENT_HANDLER */
/*! @} */