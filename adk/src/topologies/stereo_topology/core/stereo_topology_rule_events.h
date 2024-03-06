/*!
\copyright  Copyright (c) 2020 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of events which can initiate stereo topology rule processing.
*/

#ifndef STEREO_TOPOLOGY_RULE_EVENTS_H_
#define STEREO_TOPOLOGY_RULE_EVENTS_H_

#define STEREOTOP_RULE_EVENT_START                                (1ULL << 0)
#define STEREOTOP_RULE_EVENT_STOP                                 (1ULL << 1)
#define STEREOTOP_RULE_EVENT_PEER_PAIR                            (1ULL << 2)
#define STEREOTOP_RULE_EVENT_PEER_FIND_ROLE                       (1ULL << 3)
#define STEREOTOP_RULE_EVENT_PRIMARY_CONN_PEER                    (1ULL << 4)
#define STEREOTOP_RULE_EVENT_SECONDARY_CONN_PEER                  (1ULL << 5)
#define STEREOTOP_RULE_EVENT_ENABLE_STEREO_STANDALONE             (1ULL << 6)

#endif /* STEREO_TOPOLOGY_RULE_EVENTS_H_ */
