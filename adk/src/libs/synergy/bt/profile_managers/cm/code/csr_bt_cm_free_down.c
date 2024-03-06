/******************************************************************************
 Copyright (c) 2012-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/* Note: this is an auto-generated file. */

#ifndef EXCLUDE_CSR_BT_CM_MODULE
#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_mblk.h"
#include "csr_bt_autogen.h"
#include "csr_bt_cm_lib.h"
#include "csr_bt_cm_prim.h"
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_free_handcoded.h"

void CsrBtCmFreeDownstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_CM_PRIM)
    {
        CsrBtCmPrim *prim = (CsrBtCmPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ
            case CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ:
            {
                CsrBtCmEirUpdateManufacturerDataReq *p = message;
                CsrPmemFree(p->manufacturerData);
                p->manufacturerData = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_EIR_UPDATE_MANUFACTURER_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_MAP_SCO_PCM_RES
            case CSR_BT_CM_MAP_SCO_PCM_RES:
            {
                CsrBtCmMapScoPcmRes *p = message;
                CsrPmemFree(p->parms);
                p->parms = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_MAP_SCO_PCM_RES */
#ifndef EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ
            case CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ:
            {
                CsrBtCmBnepExtendedMulticastDataReq *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_MULTICAST_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SET_LOCAL_NAME_REQ
            case CSR_BT_CM_SET_LOCAL_NAME_REQ:
            {
                CsrBtCmSetLocalNameReq *p = message;
                CsrPmemFree(p->friendlyName);
                p->friendlyName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SET_LOCAL_NAME_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_RFC_SEARCH_REQ
            case CSR_BT_CM_SDC_RFC_SEARCH_REQ:
            {
                CsrBtCmSdcRfcSearchReq *p = message;
                CsrPmemFree(p->serviceList);
                p->serviceList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_RFC_SEARCH_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_SEARCH_REQ
            case CSR_BT_CM_SDC_SEARCH_REQ:
            {
                CsrBtCmSdcSearchReq *p = message;
                CsrPmemFree(p->serviceList);
                p->serviceList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_SEARCH_REQ */
#ifndef EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_REQ
            case CM_SDC_SERVICE_SEARCH_ATTR_REQ:
            {
                CmSdcServiceSearchAttrReq *p = message;
                CsrPmemFree(p->svcSearchAttrInfoList);
                p->svcSearchAttrInfoList = NULL;
                break;
            }
#endif /* EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_REQ */
#ifndef EXCLUDE_CM_DM_WRITE_LINK_POLICY_REQ
            case CM_DM_WRITE_LINK_POLICY_REQ:
            {
                CsrBtCmWriteLinkPolicyReq *p = message;
                CsrPmemFree(p->sniffSettings);
                p->sniffSettings = NULL;
                break;
            }
#endif /* EXCLUDE_CM_DM_WRITE_LINK_POLICY_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDS_REGISTER_REQ
            case CSR_BT_CM_SDS_REGISTER_REQ:
            {
                CsrBtCmSdsRegisterReq *p = message;
                CsrPmemFree(p->serviceRecord);
                p->serviceRecord = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDS_REGISTER_REQ */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_DATA_REQ
            case CSR_BT_CM_L2CA_DATA_REQ:
            {
                CsrBtCmL2caDataReq *p = message;
                CsrMblkDestroy(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ
            case CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ:
            {
                CsrBtCmSdcRfcExtendedSearchReq *p = message;
                CsrPmemFree(p->serviceList);
                p->serviceList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_RFC_EXTENDED_SEARCH_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ
            case CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ:
            {
                CsrBtCmSdcUuid128RfcSearchReq *p = message;
                CsrPmemFree(p->serviceList);
                p->serviceList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_UUID128_RFC_SEARCH_REQ */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ
            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ:
            {
                CsrBtCmL2caConnectAcceptReq *p = message;
                CsrPmemFree(p->conftab);
                p->conftab = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_CONNECT_ACCEPT_REQ */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_REQ
            case CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_REQ:
            {
                CsrBtCmL2caConnectionlessDataReq *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SCO_CONNECT_REQ
            case CSR_BT_CM_SCO_CONNECT_REQ:
            {
                CsrBtCmScoConnectReq *p = message;
                CsrPmemFree(p->parms);
                p->parms = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SCO_CONNECT_REQ */
#ifndef EXCLUDE_CSR_BT_CM_LE_WHITELIST_SET_REQ
            case CSR_BT_CM_LE_WHITELIST_SET_REQ:
            {
                CsrBtCmLeWhitelistSetReq *p = message;
                CsrPmemFree(p->addressList);
                p->addressList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_LE_WHITELIST_SET_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_UUID128_SEARCH_REQ
            case CSR_BT_CM_SDC_UUID128_SEARCH_REQ:
            {
                CsrBtCmSdcUuid128SearchReq *p = message;
                CsrPmemFree(p->serviceList);
                p->serviceList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_UUID128_SEARCH_REQ */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_CONNECT_REQ
            case CSR_BT_CM_L2CA_CONNECT_REQ:
            {
                CsrBtCmL2caConnectReq *p = message;
                CsrPmemFree(p->conftab);
                p->conftab = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_CONNECT_REQ */
#ifndef EXCLUDE_CM_L2CA_TP_CONNECT_REQ
            case CM_L2CA_TP_CONNECT_REQ:
            {
                CmL2caTpConnectReq *p = message;
                CsrPmemFree(p->conftab);
                p->conftab = NULL;
                break;
            }
#endif /* EXCLUDE_CM_L2CA_CONNECT_REQ */
#ifndef EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_REQ
            case CSR_BT_CM_BNEP_EXTENDED_DATA_REQ:
            {
                CsrBtCmBnepExtendedDataReq *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SDC_SERVICE_SEARCH_REQ
            case CSR_BT_CM_SDC_SERVICE_SEARCH_REQ:
            {
                CsrBtCmSdcServiceSearchReq *p = message;
                CsrPmemFree(p->uuidSet);
                p->uuidSet = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_SERVICE_SEARCH_REQ */
#ifndef EXCLUDE_CSR_BT_CM_LE_ADVERTISE_REQ
            case CSR_BT_CM_LE_ADVERTISE_REQ:
            {
                CsrBtCmLeAdvertiseReq *p = message;
                CsrPmemFree(p->advertisingData);
                p->advertisingData = NULL;
                CsrPmemFree(p->scanResponseData);
                p->scanResponseData = NULL;
                CsrPmemFree(p->whitelistAddrList);
                p->whitelistAddrList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_LE_ADVERTISE_REQ */
#ifndef EXCLUDE_CSR_BT_CM_LE_SCAN_REQ
            case CSR_BT_CM_LE_SCAN_REQ:
            {
                CsrBtCmLeScanReq *p = message;
                CsrPmemFree(p->addressList);
                p->addressList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_LE_SCAN_REQ */
#ifndef EXCLUDE_CSR_BT_CM_DM_L2CA_MODE_SETTINGS_REQ
            case CSR_BT_CM_DM_L2CA_MODE_SETTINGS_REQ:
            {
                CsrBtCmDmL2caModeSettingsReq *p = message;
                CsrPmemFree(p->sniffSettings);
                p->sniffSettings = NULL;
                CsrPmemFree(p->ssrSettings);
                p->ssrSettings = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DM_L2CA_MODE_SETTINGS_REQ */
#ifndef EXCLUDE_CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ
            case CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ:
            {
                CsrBtCmDmBnepModeSettingsReq *p = message;
                CsrPmemFree(p->sniffSettings);
                p->sniffSettings = NULL;
                CsrPmemFree(p->ssrSettings);
                p->ssrSettings = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DM_BNEP_MODE_SETTINGS_REQ */
#ifndef EXCLUDE_CSR_BT_CM_GET_SECURITY_CONF_RES
            case CSR_BT_CM_GET_SECURITY_CONF_RES:
            {
                CsrBtCmGetSecurityConfRes *p = message;
                CsrPmemFree(p->leEr);
                p->leEr = NULL;
                CsrPmemFree(p->leIr);
                p->leIr = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_GET_SECURITY_CONF_RES */
#ifndef EXCLUDE_CSR_BT_CM_DATA_REQ
            case CSR_BT_CM_DATA_REQ:
            {
                CsrBtCmDataReq *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_DM_MODE_SETTINGS_REQ
            case CSR_BT_CM_DM_MODE_SETTINGS_REQ:
            {
                CsrBtCmDmModeSettingsReq *p = message;
                CsrPmemFree(p->sniffSettings);
                p->sniffSettings = NULL;
                CsrPmemFree(p->ssrSettings);
                p->ssrSettings = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DM_MODE_SETTINGS_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SM_ADD_DEVICE_REQ
            case CSR_BT_CM_SM_ADD_DEVICE_REQ:
            {
                CsrBtCmSmAddDeviceReq *p = message;
                CsrPmemFree(p->keys);
                p->keys = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SM_ADD_DEVICE_REQ */
#ifndef EXCLUDE_CSR_BT_CM_SET_EIR_DATA_REQ
            case CSR_BT_CM_SET_EIR_DATA_REQ:
            {
                CsrBtCmSetEirDataReq *p = message;
                CsrPmemFree(p->data);
                p->data = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SET_EIR_DATA_REQ */
#ifndef EXCLUDE_CSR_BT_CM_DM_POWER_SETTINGS_REQ
            case CSR_BT_CM_DM_POWER_SETTINGS_REQ:
            {
                CsrBtCmDmPowerSettingsReq *p = message;
                CsrPmemFree(p->powerTable);
                p->powerTable = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DM_POWER_SETTINGS_REQ */
#ifdef CSR_BT_ISOC_ENABLE
            case CSR_BT_CM_ISOC_BIG_CREATE_SYNC_REQ:
            {
                CmIsocBigCreateSyncReq *p = message;
                CsrPmemFree(p->bis);
                p->bis = NULL;
                break;
            }
#endif /* CSR_BT_ISOC_ENABLE */
#ifdef CSR_BT_INSTALL_EXTENDED_ADVERTISING
            case CSR_BT_CM_EXT_ADV_SET_DATA_REQ:
            {
                CmExtAdvSetDataReq *p = message;
                CsrUint8 i;

                for (i = 0; i < CSR_ARRAY_SIZE(p->data); i++)
                {
                    CsrPmemFree(p->data[i]);
                    p->data[i] = NULL;
                }
                p->dataLength = 0;
                break;
            }
            
            case CSR_BT_CM_EXT_ADV_SET_SCAN_RESP_DATA_REQ:
            {
                CmExtAdvSetScanRespDataReq *p = message;
                CsrUint8 i;

                for (i = 0; i < CSR_ARRAY_SIZE(p->data); i++)
                {
                    CsrPmemFree(p->data[i]);
                    p->data[i] = NULL;
                }
                p->dataLength = 0;
                break;
            }
#endif /* CSR_BT_INSTALL_EXTENDED_ADVERTISING */
#ifdef CSR_BT_ISOC_ENABLE
            case CSR_BT_CM_ISOC_SETUP_ISO_DATA_PATH_REQ:
            {
                CmIsocSetupIsoDataPathReq* p = message;

                if (p->codec_config_length && p->codec_config_data)
                {
                    CsrPmemFree(p->codec_config_data);
                    p->codec_config_data = NULL;
                }
                p->codec_config_length = 0;
                break;
            }
#endif

            case CSR_BT_CM_L2CA_CONNECT_ACCEPT_RSP:
            {
                CsrBtCmL2caConnectAcceptRsp *p = message;
                CsrPmemFree(p->conftab);
                p->conftab = NULL;
                break;
            }

            case CM_L2CA_TP_CONNECT_ACCEPT_RSP:
            {
                CmL2caTpConnectAcceptRsp *p = message;
                CsrPmemFree(p->conftab);
                p->conftab = NULL;
                break;
            }

            default:
            {
                CsrBtCmFreeHandcoded(prim);
                break;
            }
        } /* End switch */
    } /* End if */
    else
    {
        /* Unknown primitive type, exception handling */
    }
}
#endif /* EXCLUDE_CSR_BT_CM_MODULE */
