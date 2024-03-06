/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    %%version
\file       headset_rule_events.h
\brief      Definition of events which can initiate headset application rule processing.
*/

#ifndef HEADSET_RULE_EVENTS_H_
#define HEADSET_RULE_EVENTS_H_

#define HS_EVENT_LINK_LOSS                              (1ULL << 0)
#define HS_EVENT_USER_CON_HANDSET                       (1ULL << 1)
#define HS_EVENT_AUTO_CON_HANDSET                       (1ULL << 2)
#define HS_EVENT_DISCON_LRU_HANDSET                     (1ULL << 3)
#define HS_EVENT_DISCON_ALL_HANDSET                     (1ULL << 4)

#endif /* HEADSET_RULE_EVENTS_H_ */
