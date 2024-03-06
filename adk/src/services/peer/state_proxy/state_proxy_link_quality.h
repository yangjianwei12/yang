/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file
    \addtogroup state_proxy
    @{
    \brief      Link quality measurement
*/

#ifndef STATE_PROXY_LINK_QUALITY_H
#define STATE_PROXY_LINK_QUALITY_H

#include "connection_abstraction.h"

/*  \brief Kick link quality code to start/stop measurements based on changes
    to external state. Link quality measurements will when mirroring is active
    and there are clients registered for link quality events.
*/
void stateProxy_LinkQualityKick(void);

/*! \brief Handle interval timer for measuring link quality.
*/
void stateProxy_HandleIntervalTimerLinkQuality(void);

/*! \brief Handle remote link quality message */
void stateProxy_HandleRemoteLinkQuality(const STATE_PROXY_LINK_QUALITY_T *msg);

#ifdef INCLUDE_HDMA_CIS_RSSI_EVENT

/*! \brief Handle interval timer for measuring CIS RSSI quality.
*/
void stateProxy_HandleIntervalTimerCisRssiQuality(void);

/*! \brief Handle remote CIS RSSI message */
void stateProxy_HandleRemoteCisRssiUpdate(const STATE_PROXY_CIS_RSSI_T *msg);

/*  \brief Kick CIS RSSI code to start/stop measurements based on changes
    to external state. CIS RSSI measurements will when mirroring is active
    and there are clients registered for CIS RSSI events.
*/
void stateProxy_CisRssiKick(void);

#endif

#endif /* STATE_PROXY_LINK_QUALITY_H */

/*! @} */