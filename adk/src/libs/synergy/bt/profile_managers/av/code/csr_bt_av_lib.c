/******************************************************************************
 Copyright (c) 2009-2017 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_pmem.h"
#include "csr_bt_av_lib.h"
#include "csr_msg_transport.h"
#include "csr_bt_av_main.h"

void CsrBtAvMsgTransport(void* msg__)
{
    CsrMsgTransport(CSR_BT_AV_IFACEQUEUE, CSR_BT_AV_PRIM, msg__);
}

#ifdef CSR_TARGET_PRODUCT_VM
void CsrBtAvGetConId(CsrBtDeviceAddr *addr, CsrUint8 *conId)
{
    CsrUint8 index = 0;

    for(index = 0; index < CSR_BT_AV_MAX_NUM_CONNECTIONS; index++)
    {
        if(CsrBtBdAddrEq(&csrBtAvInstData.con[index].remoteDevAddr, addr))
        {
            *conId = index;
            break;
        }
    }
}

void CsrBtAvUseLargeMtu(CsrUint8 conId, CsrBool useLargeMtu)
{
    csrBtAvInstData.con[conId].useLargeMtu = useLargeMtu;
}
#endif /* CSR_TARGET_PRODUCT_VM */

/*----------------------------------------------------------------------------*
 *  NAME
 *  CsrBtAvGetServiceCap
 *
 *  DESCRIPTION
 *  Search a list of service capabilities for a given serviceCap category.
 *
 *  PARAMETERS
 *    serviceCap: service capability category to search for (or simply
 *        the next service capability)
 *    list:   pointer to service capability list
 *    length:     length of service capability list
 *    index:  index for current position in list
 *----------------------------------------------------------------------------*/
CsrUint8 *CsrBtAvGetServiceCap(CsrBtAvServCap  serviceCap,
                               CsrUint8    *list,
                               CsrUint16   length,
                               CsrUint16   *index)
{
    CsrUint16 tmp = *index;
    CsrUint8  *ptr = NULL;

    if(serviceCap == CSR_BT_AV_SC_NEXT)
    {
        if((tmp + *(list + tmp + 1) + 2) < length)
        {
            *index = (CsrUint16)(*index + *(list+tmp+1) + 2);
            ptr = list + *index;
        }
    }
    else
    {
        while (tmp < length)
        {
            if(*(list+tmp) == serviceCap)
            {
                ptr = list + tmp;
                *index = (CsrUint16)(tmp + *(ptr + 1) + 2);
                break;
            }
            else
            {
                tmp = (CsrUint16)(tmp + *(list+tmp+1) + 2);
            }
        }
    }

    return ptr;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *  CsrBtAvValidateServiceCap
 *
 *  DESCRIPTION
 *  Validate a service capability category.
 *
 *  PARAMETERS
 *    serviceCap_ptr:    pointer to service capability category
 *----------------------------------------------------------------------------*/
CsrBtAvResult CsrBtAvValidateServiceCap(CsrUint8 *serviceCap_ptr)
{
    CsrBtAvResult result;

    switch(*serviceCap_ptr)
    {
        case CSR_BT_AV_SC_MEDIA_TRANSPORT:
        {
            result = CSR_BT_RESULT_CODE_A2DP_BAD_MEDIA_TRANSPORT_FORMAT;
            if(*(serviceCap_ptr+1) == 0)
            {
                result = CSR_BT_AV_ACCEPT;
            }
        }
        break;

        case CSR_BT_AV_SC_CONTENT_PROTECTION:
        case CSR_BT_AV_SC_MEDIA_CODEC:
        case CSR_BT_AV_SC_DELAY_REPORTING:
        {
            /* just return OK, the app. should check this, not the AV profile */
            result = CSR_BT_AV_ACCEPT;
        }
        break;

        case CSR_BT_AV_SC_REPORTING:
        case CSR_BT_AV_SC_RECOVERY:
        case CSR_BT_AV_SC_HDR_COMPRESSION:
        case CSR_BT_AV_SC_MULTIPLEXING:
        {
            result = CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION;
        }
        break;

        default:
        {
            result = CSR_BT_RESULT_CODE_A2DP_BAD_SERV_CATEGORY;
        }
        break;
    }

    return result;
}




