/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file    lea_advertising_policy_config.h
\brief   Header file for the LEA Advertising Policy configurations.
*/

#ifndef LEA_ADVERTISING_POLICY_CONFIG_H_
#define LEA_ADVERTISING_POLICY_CONFIG_H_

#include "rtime.h"

/* Advertising Intervals for quicker connection setup (As Per Table 8.1 in BAP Specification v 1.0) */
#define LEA_ADVERTISING_POLICY_DIRECTED_ADVERT_INTERVAL_MIN        MS_TO_BT_SLOTS(20)
#define LEA_ADVERTISING_POLICY_DIRECTED_ADVERT_INTERVAL_MAX        MS_TO_BT_SLOTS(30)

/* Advertising Intervals for reduced power consumption */
#define LEA_ADVERTISING_POLICY_UNDIRECTED_ADVERT_INTERVAL_MIN      MS_TO_BT_SLOTS(225)
#define LEA_ADVERTISING_POLICY_UNDIRECTED_ADVERT_INTERVAL_MAX      MS_TO_BT_SLOTS(250)

#endif
