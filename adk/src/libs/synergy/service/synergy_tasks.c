/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy task information
*/

#include "synergy_tasks.h"
#include "synergy_private.h"

extern void CsrBtCmInit(void **gash);
extern void CsrBtCmHandler(void **gash);

extern void CsrBtGattInit(void **gash);
extern void CsrBtGattInterfaceHandler(void **gash);

extern void CsrBtGattClientUtilInit(void **gash);
extern void CsrBtGattClientUtilHandler(void **gash);

extern void GattServiceDiscoveryInit(void **gash);
extern void GattServiceDiscoveryMsgHandler(void **gash);

#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
extern void gatt_pacs_server_init(void **gash);
extern void pacs_server_msg_handler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
extern void gatt_ascs_server_task_init(void **gash);
extern void ascs_server_msg_handler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
extern void gatt_bass_server_init(void **gash);
extern void GattBassServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
extern void gatt_csis_server_init(void **gash);
extern void csis_server_msg_handler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
extern void gatt_vcs_server_init(void **gash);
extern void vcs_server_msg_handler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
extern void gattMcsClientInit(void **gash);
extern void gattMcsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MCP_MODULE
extern void mcpInit(void **gash);
extern void mcpMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
extern void BapServerInit(void **gash);
extern void BapServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
extern void gatt_tbs_client_init(void **gash);
extern void gattTbsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
extern void gattTbsServerInit(void **gash);
extern void tbsServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CCP_MODULE
extern void ccp_init(void **gash);
extern void ccpMsgHandler(void **gash);
#endif

/* VOCS and AICS client are currently not supported. Uncomment below block if in future they are supported */
/*
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
extern void gatt_vocs_client_init(void **gash);
extern void gattVocsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
extern void gatt_aics_client_init(void **gash);
extern void gattAicsClientMsgHandler(void **gash);
#endif
*/

#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
extern void gatt_mics_server_init(void **gash);
extern void mics_server_msg_handler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE
extern void gattTmasServerInit(void **gash);
extern void gattTmasServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE
extern void tmapClientInit(void **gash);
extern void tmapClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE
extern void gattTmasClientInit(void **gash);
extern void gattTmasClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
extern void gattMcsServerInit(void **gash);
extern void gattMcsServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
extern void CapClientTaskInit(void **gash);
extern void CapClientMsgHandler(void **gash);
#endif

#if (!defined(EXCLUDE_CSR_BT_BAP_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE))
extern void bapClientInit(void **gash);
extern void bapClientHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
extern void GattPacsClientInit(void** gash);
extern void PacsClientMsgHandler(void** gash);
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
extern void gatt_ascs_client_init(void **gash);
extern void AscsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
extern void gattCsisClientInit(void **gash);
extern void gattCsisClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
extern void gatt_bass_client_init(void **gash);
extern void gattBassClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
extern void CsipInit(void **gash);
extern void csipMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_VCP_MODULE
extern void vcp_init(void **gash);
extern void vcpMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
extern void gatt_vcs_client_init(void **gash);
extern void gattVcsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
extern void micp_init(void **gash);
extern void micpMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
extern void GattMicsClientInit(void **gash);
extern void gattMicsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
extern void gattGmasServerInit(void **gash);
extern void gattGmasServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE
extern void gattGmasClientInit(void **gash);
extern void gattGmasClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
extern void gmapClientInit(void **gash);
extern void gmapClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_PBP_MODULE
extern void pbpInit(void **gash);
extern void pbpMsgHandler(void **gash);
#endif

#endif /*CSR_ENABLE_LEA*/

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
extern void gattHidsServerInit(void **gash);
extern void gattHidsServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_HF_MODULE
extern void CsrBtHfInit(void **gash);
extern void CsrBtHfHandler(void **gash);
#endif /* EXCLUDE_CSR_BT_HF_MODULE */

extern void CsrBtAvInit(void **gash);
extern void CsrBtAvHandler(void **gash);

extern void CsrBtAvrcpInit(void **gash);
extern void CsrBtAvrcpHandler(void **gash);

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
extern void CsrBtSppInit(void **gash);
extern void CsrBtSppHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
extern void CsrBtHiddInit(void **gash);
extern void CsrBtHiddHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_PAC_MODULE
extern void CsrBtPacInit(void **gash);
extern void CsrBtPacHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_BAS_SERVER_MODULE
extern void CsrBtBasInit(void **gash);
extern void CsrBtBasHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_HFG_MODULE
extern void CsrBtHfgInit(void **gash);
extern void CsrBtHfgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_DIS_SERVER_MODULE
extern void CsrBtDisInit(void **gash);
extern void CsrBtDisHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TDS_SERVER_MODULE
extern void GattTdsServerTaskInit(void **gash);
extern void TdsServerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
extern void gattTdsClientInit(void **gash);
extern void gattTdsClientMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
extern void chpSeekerInit(void **gash);
extern void chpSeekerMsgHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_TPS_SERVER_MODULE
extern void GattTpsServerTaskInit(void **gash);
extern void TpsServerMsgHandler(void **gash);
#endif

#ifdef CSR_TARGET_PRODUCT_VM
extern void VscInit(void **gash);
extern void VscHandler(void **gash);
#endif

#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
extern void CsrBtMapcInit(void **gash);
extern void CsrBtMapcHandler(void **gash);
#endif

#ifndef EXCLUDE_GATT_QSS_SERVER_MODULE
extern void gattQssServerTaskInit(void **gash);
extern void gattQssServerMsgHandler(void **gash);
#endif

/******************************************************************************
 * Global and Local Declarations
 ******************************************************************************/

schedEntryFunction_t SynergyTaskInit[SYNERGY_TASK_ID_MAX] =
{
    CsrBtCmInit,
#ifndef EXCLUDE_CSR_BT_HF_MODULE
    CsrBtHfInit,
#endif
    CsrBtAvInit,
    CsrBtAvrcpInit,
    CsrBtGattInit,
    CsrBtGattClientUtilInit,
    GattServiceDiscoveryInit,
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE
    gattTmasServerInit,
#endif
#ifndef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE
    tmapClientInit,
#endif
#ifndef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE
    gattTmasClientInit,
#endif

#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
    NULL,
#endif

#if (!defined(EXCLUDE_CSR_BT_BAP_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE))
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
    gattGmasServerInit,
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE
    gattGmasClientInit,
#endif
#ifndef EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
    gmapClientInit,
#endif
#ifndef EXCLUDE_CSR_BT_PBP_MODULE
    pbpInit,
#endif

#endif /*CSR_ENABLE_LEA*/

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
    CsrBtSppInit,
#ifndef INSTALL_SPP_ADDITIONAL_INSTANCES
    NULL,             /* SYNERGY_TASK_ID_BT_SPP2 */
#else
    CsrBtSppInit,     /* SYNERGY_TASK_ID_BT_SPP2 */
    CsrBtSppInit,     /* SYNERGY_TASK_ID_BT_SPP3 */
    CsrBtSppInit,     /* SYNERGY_TASK_ID_BT_SPP4 */
    CsrBtSppInit,     /* SYNERGY_TASK_ID_BT_SPP5 */
    CsrBtSppInit,     /* SYNERGY_TASK_ID_BT_SPP6 */
#endif /* !INSTALL_SPP_ADDITIONAL_INSTANCES */
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
    NULL,  /*SYNERGY_TASK_ID_BT_HIDD,       */
#endif
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
    CsrBtPacInit,
#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    CsrBtPacInit,
#endif
#endif
#ifndef EXCLUDE_CSR_BT_BAS_SERVER_MODULE
    CsrBtBasInit,
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
    CsrBtHfgInit,
#endif
#ifndef EXCLUDE_CSR_BT_DIS_SERVER_MODULE
    CsrBtDisInit,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_SERVER_MODULE
    GattTdsServerTaskInit,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
    gattTdsClientInit,
#endif
#ifndef EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
    chpSeekerInit,
#endif
#ifndef EXCLUDE_CSR_BT_TPS_SERVER_MODULE
    GattTpsServerTaskInit,
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
    CsrBtMapcInit,
#ifdef CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT
    CsrBtMapcInit,
#endif
#endif
#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
    NULL,
#endif

#ifdef CSR_TARGET_PRODUCT_VM
    VscInit,
#endif

#ifndef EXCLUDE_GATT_QSS_SERVER_MODULE
    gattQssServerTaskInit,
#endif
};

#if defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)
CsrLogTextHandle g_log_registrations[SYNERGY_TASK_ID_MAX] = {0};
const CsrUint16 g_log_registrations_size = SYNERGY_TASK_ID_MAX;
#endif /*defined(CSR_LOG_ENABLE) && defined(CSR_LOG_ENABLE_REGISTRATION)*/

schedEntryFunction_t SynergyTaskHandler[SYNERGY_TASK_ID_MAX] =
{
    CsrBtCmHandler,
#ifndef EXCLUDE_CSR_BT_HF_MODULE
    CsrBtHfHandler,
#endif
    CsrBtAvHandler,
    CsrBtAvrcpHandler,
    CsrBtGattInterfaceHandler,
    CsrBtGattClientUtilHandler,
    GattServiceDiscoveryMsgHandler,
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE
    gattTmasServerMsgHandler,
#endif

#ifndef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE
    tmapClientMsgHandler,
#endif

#ifndef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE
    gattTmasClientMsgHandler,
#endif

#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
    NULL,
#endif

#if (!defined(EXCLUDE_CSR_BT_BAP_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE))
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
    NULL,
#endif

#ifndef EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
    gattGmasServerMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE
    gattGmasClientMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
    gmapClientMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_PBP_MODULE
    pbpMsgHandler,
#endif

#endif /*CSR_ENABLE_LEA*/

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
    CsrBtSppHandler,
#ifndef INSTALL_SPP_ADDITIONAL_INSTANCES
    NULL,               /* SYNERGY_TASK_ID_BT_SPP2 */
#else
    CsrBtSppHandler,    /* SYNERGY_TASK_ID_BT_SPP2 */
    CsrBtSppHandler,    /* SYNERGY_TASK_ID_BT_SPP3 */
    CsrBtSppHandler,    /* SYNERGY_TASK_ID_BT_SPP4 */
    CsrBtSppHandler,    /* SYNERGY_TASK_ID_BT_SPP5 */
    CsrBtSppHandler,    /* SYNERGY_TASK_ID_BT_SPP6 */
#endif /* !INSTALL_SPP_ADDITIONAL_INSTANCES */
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
    NULL,  /*SYNERGY_TASK_ID_BT_HIDD,       */
#endif
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
    CsrBtPacHandler,
#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
    CsrBtPacHandler,
#endif
#endif
#ifndef EXCLUDE_CSR_BT_BAS_SERVER_MODULE
    CsrBtBasHandler,
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
    CsrBtHfgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_DIS_SERVER_MODULE
    CsrBtDisHandler,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_SERVER_MODULE
    TdsServerMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
    gattTdsClientMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
    chpSeekerMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_TPS_SERVER_MODULE
    TpsServerMsgHandler,
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
    CsrBtMapcHandler,
#ifdef CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT
    CsrBtMapcHandler,
#endif
#endif
#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
    NULL,  /*SYNERGY_TASK_ID_BT_HIDS*/
#endif

#ifdef CSR_TARGET_PRODUCT_VM
    VscHandler,
#endif

#ifndef EXCLUDE_GATT_QSS_SERVER_MODULE
    gattQssServerMsgHandler,
#endif
};

CsrSchedQid CSR_BT_CM_IFACEQUEUE    = SYNERGY_TASK_ID_BT_CM;
#ifndef EXCLUDE_CSR_BT_HF_MODULE
CsrSchedQid CSR_BT_HF_IFACEQUEUE    = SYNERGY_TASK_ID_BT_HF;
#endif
CsrSchedQid CSR_BT_AV_IFACEQUEUE    = SYNERGY_TASK_ID_BT_AV;
CsrSchedQid CSR_BT_AVRCP_IFACEQUEUE = SYNERGY_TASK_ID_BT_AVRCP;
CsrSchedQid CSR_BT_GATT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_GATT;
CsrSchedQid CSR_BT_GATT_CLIENT_UTIL_IFACEQUEUE  = SYNERGY_TASK_ID_BT_GATT_CLIENT_UTIL;
CsrSchedQid CSR_BT_GATT_SRVC_DISC_IFACEQUEUE = SYNERGY_TASK_ID_BT_SRVC_DISC;
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
CsrSchedQid CSR_BT_PACS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_PACS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
CsrSchedQid CSR_BT_ASCS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_ASCS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
CsrSchedQid CSR_BT_BASS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_BASS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
CsrSchedQid CSR_BT_CSIS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_CSIS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
CsrSchedQid CSR_BT_VCS_SERVER_IFACEQUEUE   = SYNERGY_TASK_ID_BT_VCS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
CsrSchedQid CSR_BT_MCS_CLIENT_IFACEQUEUE   = SYNERGY_TASK_ID_BT_MCS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_MCP_MODULE
CsrSchedQid CSR_BT_MCP_IFACEQUEUE = SYNERGY_TASK_ID_BT_MCP;
#endif
#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
CsrSchedQid CSR_BT_BAP_SERVER_IFACEQUEUE   = SYNERGY_TASK_ID_BT_BAP_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
CsrSchedQid CSR_BT_TBS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_TBS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_CCP_MODULE
CsrSchedQid CSR_BT_CCP_IFACEQUEUE  = SYNERGY_TASK_ID_BT_CCP;
#endif
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
CsrSchedQid CSR_BT_VOCS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_VOCS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
CsrSchedQid CSR_BT_AICS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_AICS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
CsrSchedQid CSR_BT_TBS_SERVER_IFACEQUEUE = SYNERGY_TASK_ID_BT_TBS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
CsrSchedQid CSR_BT_MICS_SERVER_IFACEQUEUE   = SYNERGY_TASK_ID_BT_MICS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_TMAS_SERVER_MODULE
CsrSchedQid CSR_BT_TMAS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TMAS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_TMAP_CLIENT_MODULE
CsrSchedQid CSR_BT_TMAP_CLIENT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TMAP_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_TMAS_CLIENT_MODULE
CsrSchedQid CSR_BT_TMAS_CLIENT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TMAS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
CsrSchedQid CSR_BT_MCS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_MCS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
CsrSchedQid CSR_BT_CAP_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_CAP_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
CsrSchedQid CSR_BT_PACS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_PACS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
CsrSchedQid CSR_BT_ASCS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_ASCS_CLIENT;
#endif
#if (!defined(EXCLUDE_CSR_BT_BAP_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE)) || (!defined(EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE))
CsrSchedQid CSR_BT_BAP_IFACEQUEUE = SYNERGY_TASK_ID_BT_BAP_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
CsrSchedQid CSR_BT_CSIS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_CSIS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
CsrSchedQid CSR_BT_BASS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_BASS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
CsrSchedQid CSR_BT_CSIP_IFACEQUEUE = SYNERGY_TASK_ID_BT_CSIP_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_VCP_MODULE
CsrSchedQid CSR_BT_VCP_IFACEQUEUE = SYNERGY_TASK_ID_BT_VCP;
#endif
#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
CsrSchedQid CSR_BT_VCS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_VCS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
CsrSchedQid CSR_BT_MICP_IFACEQUEUE = SYNERGY_TASK_ID_BT_MICP;
#endif
#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
CsrSchedQid CSR_BT_MICS_CLIENT_IFACEQUEUE = SYNERGY_TASK_ID_BT_MICS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_SERVER_MODULE
CsrSchedQid CSR_BT_GMAS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_GMAS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_GMAS_CLIENT_MODULE
CsrSchedQid CSR_BT_GMAS_CLIENT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_GMAS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_GMAP_CLIENT_MODULE
CsrSchedQid CSR_BT_GMAP_CLIENT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_GMAP_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_PBP_MODULE
CsrSchedQid CSR_BT_PBP_IFACEQUEUE  = SYNERGY_TASK_ID_BT_PBP;
#endif
#endif /*CSR_ENABLE_LEA*/

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
CsrSchedQid CSR_BT_HIDS_SERVER_IFACEQUEUE   = SYNERGY_TASK_ID_BT_HIDS;
#endif

#ifndef EXCLUDE_CSR_BT_SPP_MODULE
CsrSchedQid CSR_BT_SPP_IFACEQUEUE    = SYNERGY_TASK_ID_BT_SPP;
#endif
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
CsrSchedQid CSR_BT_HIDD_IFACEQUEUE = SYNERGY_TASK_ID_BT_HIDD;
#endif /*EXCLUDE_CSR_BT_HIDD_MODULE*/
#ifndef EXCLUDE_CSR_BT_PAC_MODULE
CsrSchedQid CSR_BT_PAC_IFACEQUEUE  = SYNERGY_TASK_ID_BT_PAC_1;
#ifdef INSTALL_PAC_MULTI_INSTANCE_SUPPORT
CsrSchedQid PAC_IFACEQUEUE_SEC_INSTANCE = SYNERGY_TASK_ID_BT_PAC_2;
#endif
#endif
#ifndef EXCLUDE_CSR_BT_BAS_SERVER_MODULE
CsrSchedQid CSR_BT_BAS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_BAS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_HFG_MODULE
CsrSchedQid CSR_BT_HFG_IFACEQUEUE  = SYNERGY_TASK_ID_BT_HFG;
#endif
#ifndef EXCLUDE_CSR_BT_DIS_SERVER_MODULE
CsrSchedQid CSR_BT_DIS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_DIS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_TDS_SERVER_MODULE
CsrSchedQid CSR_BT_TDS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TDS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_TDS_CLIENT_MODULE
CsrSchedQid CSR_BT_TDS_CLIENT_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TDS_CLIENT;
#endif
#ifndef EXCLUDE_CSR_BT_CHP_SEEKER_MODULE
CsrSchedQid CSR_BT_CHP_SEEKER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_CHP_SEEKER;
#endif
#ifndef EXCLUDE_CSR_BT_TPS_SERVER_MODULE
CsrSchedQid CSR_BT_TPS_SERVER_IFACEQUEUE  = SYNERGY_TASK_ID_BT_TPS_SERVER;
#endif
#ifndef EXCLUDE_CSR_BT_MAPC_MODULE
CsrSchedQid CSR_BT_MAPC_IFACEQUEUE  = SYNERGY_TASK_ID_BT_MAP_CLIENT;
#ifdef CSR_BT_INSTALL_MULTI_MAPC_INSTANCE_SUPPORT
CsrSchedQid CSR_BT_MAPC_EXTRA_IFACEQUEUE = SYNERGY_TASK_ID_BT_EXTRA_MAP_CLIENT;
#endif
#endif
#ifdef CSR_TARGET_PRODUCT_VM
CsrSchedQid CSR_BT_VSDM_IFACEQUEUE = SYNERGY_TASK_ID_VSC;
#endif
#ifndef EXCLUDE_GATT_QSS_SERVER_MODULE
CsrSchedQid CSR_BT_QSS_SERVER_IFACEQUEUE = SYNERGY_TASK_ID_BT_QSS_SERVER;
#endif

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/
void SynergyEnableLEAUnicastServerTasks(void)
{
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_PACS_SERVER] = gatt_pacs_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_PACS_SERVER] = pacs_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_ASCS_SERVER] = gatt_ascs_server_task_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_ASCS_SERVER] = ascs_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIS_SERVER] = gatt_csis_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIS_SERVER] = csis_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCS_SERVER] = gatt_vcs_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCS_SERVER] = vcs_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_MCS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MCS_CLIENT] = gattMcsClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MCS_CLIENT] = gattMcsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_MCP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MCP] = mcpInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MCP] = mcpMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BAP_SERVER] = BapServerInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BAP_SERVER] = BapServerMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_TBS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_TBS_CLIENT] = gatt_tbs_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_TBS_CLIENT] = gattTbsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CCP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CCP] = ccp_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CCP] = ccpMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_MICS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MICS_SERVER] = gatt_mics_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MICS_SERVER] = mics_server_msg_handler;
#endif
#endif /*CSR_ENABLE_LEA*/
}

void SynergyEnableLEAUnicastClientTasks(void)
{
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientTaskInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_PACS_CLIENT] = GattPacsClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_PACS_CLIENT] = PacsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_ASCS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_ASCS_CLIENT] = gatt_ascs_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_ASCS_CLIENT] = AscsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_BAP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIS_CLIENT] = gattCsisClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIS_CLIENT] = gattCsisClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIP_CLIENT] = CsipInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIP_CLIENT] = csipMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_VCP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCP] = vcp_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCP] = vcpMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCS_CLIENT] = gatt_vcs_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCS_CLIENT] = gattVcsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MICP] = micp_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MICP] = micpMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_MICS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MICS_CLIENT] = GattMicsClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MICS_CLIENT] = gattMicsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_TBS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_TBS_SERVER] = gattTbsServerInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_TBS_SERVER] = tbsServerMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_MCS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_MCS_SERVER] = gattMcsServerInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_MCS_SERVER] = gattMcsServerMsgHandler;
#endif

/* VOCS and AICS client are currently not supported. Uncomment below block if in future they are supported */
/*
#ifndef EXCLUDE_CSR_BT_VOCS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VOCS_CLIENT] = gatt_vocs_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VOCS_CLIENT] = gattVocsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_AICS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_AICS_CLIENT] = gatt_aics_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_AICS_CLIENT] = gattAicsClientMsgHandler;
#endif
*/
#endif /*CSR_ENABLE_LEA*/
}

void SynergyEnableLEABroadcastSourceTask(void)
{
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientTaskInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_BAP_BROADCAST_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientHandler;
#endif

#ifndef EXCLUDE_CSR_BT_PBP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_PBP] = pbpInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_PBP] = pbpMsgHandler;
#endif
#endif /*CSR_ENABLE_LEA*/
}

void SynergyEnableLEABroadcastAssistantTasks(void)
{
#ifdef CSR_ENABLE_LEA
#ifndef EXCLUDE_CSR_BT_CAP_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientTaskInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CAP_CLIENT] = CapClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_PACS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_PACS_CLIENT] = GattPacsClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_PACS_CLIENT] = PacsClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_BAP_BROADCAST_ASSISTANT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BAP_CLIENT] = bapClientHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIS_CLIENT] = gattCsisClientInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIS_CLIENT] = gattCsisClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_BASS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BASS_CLIENT] = gatt_bass_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BASS_CLIENT] = gattBassClientMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIP_CLIENT] = CsipInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIP_CLIENT] = csipMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_VCP_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCP] = vcp_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCP] = vcpMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_VCS_CLIENT_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCS_CLIENT] = gatt_vcs_client_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCS_CLIENT] = gattVcsClientMsgHandler;
#endif
#endif /*CSR_ENABLE_LEA*/

}

void SynergyEnableLEABroadcastSinkTasks(void)
{
#ifdef CSR_ENABLE_LEA

#ifndef EXCLUDE_CSR_BT_PACS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_PACS_SERVER] = gatt_pacs_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_PACS_SERVER] = pacs_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_BASS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BASS_SERVER] = gatt_bass_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BASS_SERVER] = GattBassServerMsgHandler;
#endif

#ifndef EXCLUDE_CSR_BT_CSIS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_CSIS_SERVER] = gatt_csis_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_CSIS_SERVER] = csis_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_VCS_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_VCS_SERVER] = gatt_vcs_server_init;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_VCS_SERVER] = vcs_server_msg_handler;
#endif

#ifndef EXCLUDE_CSR_BT_BAP_SERVER_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_BAP_SERVER] = BapServerInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_BAP_SERVER] = BapServerMsgHandler;
#endif
#endif /*CSR_ENABLE_LEA*/
}

void SynergyEnableLEATasks(void)
{
    SynergyEnableLEAUnicastServerTasks();
    SynergyEnableLEABroadcastSinkTasks();
    SynergyEnableLEABroadcastAssistantTasks();
    SynergyEnableLEAUnicastClientTasks();
}

void SynergyEnableHIDDTask(void)
{
#ifndef EXCLUDE_CSR_BT_HIDD_MODULE
    SynergyTaskInit[SYNERGY_TASK_ID_BT_HIDD] = CsrBtHiddInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_HIDD] = CsrBtHiddHandler;
#endif
}

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
void SynergyEnableHIDSTask(void)
{
    SynergyTaskInit[SYNERGY_TASK_ID_BT_HIDS] = gattHidsServerInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_HIDS] = gattHidsServerMsgHandler;
}
#endif

void SynergyEnableSPPMultiInstanceTasks(void)
{
#ifndef EXCLUDE_CSR_BT_SPP_MODULE
    /* SPP Instance 2 */
    SynergyTaskInit[SYNERGY_TASK_ID_BT_SPP2] = CsrBtSppInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_SPP2] = CsrBtSppHandler;
#ifdef INSTALL_SPP_ADDITIONAL_INSTANCES
    /* SPP Instance 3-5 */
    SynergyTaskInit[SYNERGY_TASK_ID_BT_SPP3] = CsrBtSppInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_SPP3] = CsrBtSppHandler;
    SynergyTaskInit[SYNERGY_TASK_ID_BT_SPP4] = CsrBtSppInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_SPP4] = CsrBtSppHandler;
    SynergyTaskInit[SYNERGY_TASK_ID_BT_SPP5] = CsrBtSppInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_SPP5] = CsrBtSppHandler;
    SynergyTaskInit[SYNERGY_TASK_ID_BT_SPP6] = CsrBtSppInit;
    SynergyTaskHandler[SYNERGY_TASK_ID_BT_SPP6] = CsrBtSppHandler;
#endif /* INSTALL_SPP_ADDITIONAL_INSTANCES */
#endif
}

void SynergyTaskBluestackRegister(void)
{
    SYNERGY_TASK_T *synergy_tasks = synergy_service_inst.synergy_tasks;

    MessageAttTask(&synergy_tasks[SYNERGY_TASK_ID_BT_GATT].task);
    MessageBlueStackTask(&synergy_tasks[SYNERGY_TASK_ID_BT_CM].task);
    #ifdef CSR_TARGET_PRODUCT_VM
    MessageVsdmTask(&synergy_tasks[SYNERGY_TASK_ID_VSC].task);
    #endif

}
