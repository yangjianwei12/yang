/*!
   \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \version    
   \file       dfu_rule_events.h
   \addtogroup dfu
   \brief      Definition of events which can initiate dfu rule processing.
   @{
*/

#ifndef DFU_RULE_EVENTS_H_
#define DFU_RULE_EVENTS_H_

#define DFU_EVENT_VALIDATION_COMPLETE                        (1ULL << 0)
#define DFU_EVENT_DATA_TRANSFER_COMPLETE                     (1ULL << 1)
#define DFU_EVENT_PEER_END_DATA_TRANSFER                     (1ULL << 2)
#define DFU_EVENT_UPGRADE_PEER_VLDTN_COMPLETE                (1ULL << 3)
#define DFU_EVENT_RESUME_POINT_SYNC                          (1ULL << 4)
#define DFU_EVENT_RESUME_POINT_SYNC_COMPLETED                (1ULL << 5)
#define DFU_EVENT_UPGRADE_START_DATA_IND                     (1ULL << 6)
#define DFU_EVENT_PEER_SIG_CONNECT_IND                       (1ULL << 7)
#define DFU_EVENT_UPGRADE_RESUME                             (1ULL << 8)
#define DFU_EVENT_UPGRADE_PEER_PROCESS_COMPLETE              (1ULL << 9)
#define DFU_EVENT_UPGRADE_TRANSFER_COMPLETE_RES              (1ULL << 10)

#endif /* DFU_RULE_EVENTS_H_ */

/*! @} */
