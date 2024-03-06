/*!
\copyright  Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for application volume observer
*/

#ifndef CHARGER_CASE_VOLUME_OBSERVER_H
#define CHARGER_CASE_VOLUME_OBSERVER_H

#include <av_typedef.h>

void ChargerCase_VolumeObserverInit(void);
void ChargerCase_OnAvrcpConnection(avInstanceTaskData *av);
void ChargerCase_OnAvrcpDisconnection(void);

#endif // CHARGER_CASE_VOLUME_OBSERVER_H
