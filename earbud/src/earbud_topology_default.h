#ifndef EARBUD_TOPOLOGY_DEFAULT_H
#define EARBUD_TOPOLOGY_DEFAULT_H

#include "tws_topology.h"

const tws_topology_product_behaviour_t* EarbudTopologyDefault_GetConfig(void);

#ifdef INCLUDE_HDMA_MIC_QUALITY_EVENT
#define earbudTopologyConfig_StateProxyRegisterMicQuality          (state_proxy_event_type_mic_quality)
#else
#define earbudTopologyConfig_StateProxyRegisterMicQuality          (0)
#endif

#if defined(INCLUDE_HDMA_RSSI_EVENT) || defined(INCLUDE_HDMA_LINK_QUALITY_EVENT)
#define earbudTopologyConfig_StateProxyRegisterRssiQuality         (state_proxy_event_type_link_quality)
#else
#define earbudTopologyConfig_StateProxyRegisterRssiQuality         (0)
#endif

#ifdef INCLUDE_HDMA_CIS_RSSI_EVENT
#define earbudTopologyConfig_StateProxyRegisterCisRssiQuality      (state_proxy_event_type_cis_rssi)
#else
#define earbudTopologyConfig_StateProxyRegisterCisRssiQuality      (0)
#endif

#define earbudTopology_StateProxyEventsOfInterestMask()            (earbudTopologyConfig_StateProxyRegisterMicQuality    | \
                                                                    earbudTopologyConfig_StateProxyRegisterRssiQuality   | \
                                                                    earbudTopologyConfig_StateProxyRegisterCisRssiQuality)

#endif // EARBUD_TOPOLOGY_DEFAULT_H
