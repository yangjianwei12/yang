/******************************************************************************
 Copyright (c) 2012-2022 Qualcomm Technologies International, Ltd.
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
#include "csr_bt_cm_free_handcoded.h"

void CsrBtCmFreeUpstreamMessageContents(CsrUint16 eventClass, void *message)
{
    if (eventClass == CSR_BT_CM_PRIM)
    {
        CsrBtCmPrim *prim = (CsrBtCmPrim *) message;
        switch (*prim)
        {
#ifndef EXCLUDE_CSR_BT_CM_SDC_UUID128_SEARCH_IND
            case CSR_BT_CM_SDC_UUID128_SEARCH_IND:
            {
                CsrBtCmSdcUuid128SearchInd *p = message;
                CsrPmemFree(p->serviceHandleList);
                p->serviceHandleList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_UUID128_SEARCH_IND */
#ifndef EXCLUDE_CSR_BT_CM_READ_LOCAL_NAME_CFM
            case CSR_BT_CM_READ_LOCAL_NAME_CFM:
            {
                CsrBtCmReadLocalNameCfm *p = message;
                CsrPmemFree(p->localName);
                p->localName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_READ_LOCAL_NAME_CFM */
#ifndef EXCLUDE_CSR_BT_CM_DATA_IND
            case CSR_BT_CM_DATA_IND:
            {
                CsrBtCmDataInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_DATA_IND */
#ifndef EXCLUDE_CSR_BT_CM_SDC_ATTRIBUTE_CFM
            case CSR_BT_CM_SDC_ATTRIBUTE_CFM:
            {
                CsrBtCmSdcAttributeCfm *p = message;
                CsrPmemFree(p->attributeList);
                p->attributeList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_ATTRIBUTE_CFM */
#ifndef EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_CFM
            case CM_SDC_SERVICE_SEARCH_ATTR_CFM:
            {
                CmSdcServiceSearchAttrCfm *p = message;
                CsrPmemFree(p->attributeList);
                p->attributeList = NULL;
                break;
            }
#endif /* EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_CFM */
#ifndef EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_IND
            case CM_SDC_SERVICE_SEARCH_ATTR_IND:
            {
                CmSdcServiceSearchAttrInd *p = message;
                CsrPmemFree(p->attributeList);
                p->attributeList = NULL;
                break;
            }
#endif /* EXCLUDE_CM_SDC_SERVICE_SEARCH_ATTR_IND */
#ifndef EXCLUDE_CSR_BT_CM_LOCAL_NAME_CHANGE_IND
            case CSR_BT_CM_LOCAL_NAME_CHANGE_IND:
            {
                CsrBtCmLocalNameChangeInd *p = message;
                CsrPmemFree(p->localName);
                p->localName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_LOCAL_NAME_CHANGE_IND */
#ifndef EXCLUDE_CSR_BT_CM_READ_REMOTE_NAME_IND
            case CSR_BT_CM_READ_REMOTE_NAME_IND:
            {
                CsrBtCmReadRemoteNameInd *p = message;
                CsrPmemFree(p->friendlyName);
                p->friendlyName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_READ_REMOTE_NAME_IND */
#ifndef EXCLUDE_CSR_BT_CM_SDC_SEARCH_IND
            case CSR_BT_CM_SDC_SEARCH_IND:
            {
                CsrBtCmSdcSearchInd *p = message;
                CsrPmemFree(p->serviceHandleList);
                p->serviceHandleList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_SEARCH_IND */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_DATA_IND
            case CSR_BT_CM_L2CA_DATA_IND:
            {
                CsrBtCmL2caDataInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_DATA_IND */
#ifndef EXCLUDE_CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_IND
            case CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_IND:
            {
                CsrBtCmL2caConnectionlessDataInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_L2CA_CONNECTIONLESS_DATA_IND */
#ifndef EXCLUDE_CSR_BT_CM_INQUIRY_RESULT_IND
            case CSR_BT_CM_INQUIRY_RESULT_IND:
            {
                CsrBtCmInquiryResultInd *p = message;
                CsrPmemFree(p->eirData);
                p->eirData = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_INQUIRY_RESULT_IND */
#ifndef EXCLUDE_CSR_BT_CM_SDC_SERVICE_SEARCH_CFM
            case CSR_BT_CM_SDC_SERVICE_SEARCH_CFM:
            {
                CsrBtCmSdcServiceSearchCfm *p = message;
                CsrPmemFree(p->recList);
                p->recList = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_SDC_SERVICE_SEARCH_CFM */
#ifndef EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_IND
            case CSR_BT_CM_BNEP_EXTENDED_DATA_IND:
            {
                CsrBtCmBnepExtendedDataInd *p = message;
                CsrPmemFree(p->payload);
                p->payload = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_BNEP_EXTENDED_DATA_IND */
#ifndef EXCLUDE_CSR_BT_CM_READ_REMOTE_NAME_CFM
            case CSR_BT_CM_READ_REMOTE_NAME_CFM:
            {
                CsrBtCmReadRemoteNameCfm *p = message;
                CsrPmemFree(p->friendlyName);
                p->friendlyName = NULL;
                break;
            }
#endif /* EXCLUDE_CSR_BT_CM_READ_REMOTE_NAME_CFM */
#ifdef CSR_BT_INSTALL_EXTENDED_SCANNING
            case CSR_BT_CM_EXT_SCAN_FILTERED_ADV_REPORT_IND:
            {
                CmExtScanFilteredAdvReportInd *p = message;
#ifndef CSR_STREAMS_ENABLE
                CsrPmemFree(p->data);
#else
                /* Data will be freed in CM_EXT_SCAN_FILTERED_ADV_REPORT_DONE_IND message handling.
                 * Application should not use this data pointer after this. So, setting it to NULL. */
#endif
                p->data = NULL;
                break;
            }
#endif /* CSR_BT_INSTALL_EXTENDED_SCANNING */
#ifdef CSR_BT_INSTALL_PERIODIC_SCANNING
            case CSR_BT_CM_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            {
                CmPeriodicScanSyncAdvReportInd *p = message;
                /* Data will be freed in CM_PERIODIC_SCAN_SYNC_ADV_REPORT_DONE_IND message handling.
                 * Application should not use this data pointer after this. So, setting it to NULL. */
                p->data = NULL;
                break;
            }
#endif /* CSR_BT_INSTALL_PERIODIC_SCANNING */
#ifdef CSR_BT_INSTALL_ISOC_SUPPORT
            case CSR_BT_CM_ISOC_BIG_CREATE_SYNC_CFM:
            {
                CmIsocBigCreateSyncCfm *p = message;
                CsrPmemFree(p->bis_handles);
                p->bis_handles = NULL;
                break;
            }
#endif /* CSR_BT_INSTALL_ISOC_SUPPORT */
#ifdef CSR_BT_INSTALL_CRYPTO_SUPPORT
            case CSR_BT_CM_CRYPTO_AES_CTR_CFM:
            {
                CsrBtCmCryptoAesCtrCfm *p = message;
                CsrPmemFree(p->data);
                p->data = NULL;
                break;    
            }
#endif /* CSR_BT_INSTALL_CRYPTO_SUPPORT */
#ifdef INSTALL_CM_READ_EIR_DATA
            case CM_DM_READ_EIR_DATA_CFM:
            {
                CmDmReadEIRDataCfm *p = message;
                CsrPmemFree(p->eirData);
                p->eirData = NULL;
                break;
            }
#endif /* INSTALL_CM_READ_EIR_DATA */
            case CSR_BT_CM_SM_READ_LOCAL_OOB_DATA_CFM:
            {
                CsrBtCmSmReadLocalOobDataCfm *p = message;
                if(p->status == HCI_SUCCESS)
                {
                    CsrPmemFree(p->oob_hash_c);
                    CsrPmemFree(p->oob_rand_r);
                    p->oob_hash_c = NULL;
                    p->oob_rand_r = NULL;
                }
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
