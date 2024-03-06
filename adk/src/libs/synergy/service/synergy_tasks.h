/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy task information
*/

#ifndef SYNERGY_TASKS_H_
#define SYNERGY_TASKS_H_

#include "csr_sched.h"
#include "csr_log_text_2.h"
/*!
    Synergy task IDs
 */
typedef enum
{
    SYNERGY_TASK_ID_BT_CM,
#ifndef EXCLUDE_CSR_BT_HF_MODULE
    SYNERGY_TASK_ID_BT_HF,
#endif
    SYNERGY_TASK_ID_BT_AV,
    SYNERGY_TASK_ID_BT_AVRCP,
    SYNERGY_TASK_ID_BT_GATT,
    SYNERGY_TASK_ID_BT_GATT_CLIENT_UTIL,
    SYNERGY_TASK_ID_BT_SRVC_DISC,
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_PACS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_ASCS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_BASS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_CSIS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_VCS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_MCS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_MCP_MODULE
    SYNERGY_TASK_ID_BT_MCP,
#endif
#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
    SYNERGY_TASK_ID_BT_BAP_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_TBS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_CCP_MODULE
    SYNERGY_TASK_ID_BT_CCP,
#endif
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_VOCS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_AICS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_TBS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_MICS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_TMAS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_TMAP_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_TMAS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_MCS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_CAP_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_PACS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_ASCS_CLIENT,
#endif
#if (!defined(EXCLUDE_CSR_BT_BAP_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE))
    SYNERGY_TASK_ID_BT_BAP_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_CSIS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_BASS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
    SYNERGY_TASK_ID_BT_CSIP_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_VCP_MODULE
    SYNERGY_TASK_ID_BT_VCP,
#endif
#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_VCS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    SYNERGY_TASK_ID_BT_MICP,
#endif
#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_MICS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_GMAS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_GMAS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_GMAP_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_PBP_MODULE
    SYNERGY_TASK_ID_BT_PBP,
#endif
#endif /*CSR_ENABLE_LEA*/
#ifndef EXCLUDE_CSR_BT_SPP_MODULE
    SYNERGY_TASK_ID_BT_SPP, /* Main SPP instance - SPP-manager where
                               all other SPP instances register themselves */
    SYNERGY_TASK_ID_BT_SPP2, /* SPP instance 2 */
#ifdef INSTALL_SPP_ADDITIONAL_INSTANCES
    SYNERGY_TASK_ID_BT_SPP3, /* SPP instance 3 */
    SYNERGY_TASK_ID_BT_SPP4, /* SPP instance 4 */
    SYNERGY_TASK_ID_BT_SPP5, /* SPP instance 5 */
    SYNERGY_TASK_ID_BT_SPP6, /* SPP instance 6 */
#endif /* INSTALL_SPP_ADDITIONAL_INSTANCES */
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE	
    SYNERGY_TASK_ID_BT_HIDD,
#endif
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
    SYNERGY_TASK_ID_BT_PAC_1, /* PAC main instance */
#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    SYNERGY_TASK_ID_BT_PAC_2, /* PAC second instance */
#endif
#endif
#ifndef EXCLUDE_CSR_BT_BAS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_BAS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
        SYNERGY_TASK_ID_BT_HFG,
#endif
#ifndef EXCLUDE_CSR_BT_DIS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_DIS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_TDS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
    SYNERGY_TASK_ID_BT_TDS_CLIENT,
#endif
#ifndef EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
    SYNERGY_TASK_ID_BT_CHP_SEEKER,
#endif
#ifndef EXCLUDE_CSR_BT_TPS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_TPS_SERVER,
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
    SYNERGY_TASK_ID_BT_MAP_CLIENT,
#ifdef CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT
/* To enable support for additional map clients the above macro (CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT) 
 * needs to be enabled, more number of clients can be added by adding additional entries here */
    SYNERGY_TASK_ID_BT_EXTRA_MAP_CLIENT,
#endif
#endif
#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_HIDS,
#endif

#ifdef CSR_TARGET_PRODUCT_VM
    SYNERGY_TASK_ID_VSC,
#endif
#ifndef EXCLUDE_GATT_QSS_SERVER_MODULE
    SYNERGY_TASK_ID_BT_QSS_SERVER,
#endif
    SYNERGY_TASK_ID_MAX,
} SYNERGY_TASK_ID_T;

/*! Synergy task init array */
extern schedEntryFunction_t SynergyTaskInit[SYNERGY_TASK_ID_MAX];

/*! Synergy task handler array */
extern schedEntryFunction_t SynergyTaskHandler[SYNERGY_TASK_ID_MAX];

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
extern CsrLogTextHandle g_log_registrations[SYNERGY_TASK_ID_MAX];
#endif /*defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)*/

/*! \brief Registers synergy task for Bluestack messages.
 */
void SynergyTaskBluestackRegister(void);

#endif /* SYNERGY_TASKS_H_ */