/*******************************************************************************

Copyright (C) 2007 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

DESCRIPTION:       SDC access library for building SDC downstream primitives.

REVISION:          $Revision: #1 $

*******************************************************************************/


/*=============================================================================*
    ANSI C & System-wide Header Files
 *============================================================================*/

#include "sdplib_private.h"

#if defined(INSTALL_SDP_MODULE) || defined(BUILD_FOR_HOST)

/*============================================================================*
    Public Data
 *============================================================================*/
/* None */

/*============================================================================*
    Private Defines
 *============================================================================*/

/* This code is used both inside BlueStack and by applications above BlueStack.
 * Therefore use of memory functions and task IDs depends on whether we are
 * building for the BlueStack DLL.
 */
#if defined(USE_BLUESTACK_DLL)
#define pmalloc BST_pmalloc
#define pnew BST_pnew
#define zpnew BST_zpnew
#define SDP_PutMsg(prim) BST_putMessage(SDP_IFACEQUEUE, SDP_PRIM, (prim))
#define SDP_IFACEQUEUE bst_sdp_iface_queue
#endif

/*============================================================================*
    Private Data Types
 *============================================================================*/
/* None */

/*============================================================================*
    Private Function Prototypes
 *============================================================================*/
/* None */

/*============================================================================*
    Private Data
 *============================================================================*/
/* None */

/*============================================================================*
    Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_service_search_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_SERVICE_SEARCH_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_service_search_req(
    phandle_t phandle,          /* routing handle */
    BD_ADDR_T *p_bd_addr,       /* remote device */
    uint16_t size_srch_pttrn,   /* size of search pattern */
    uint8_t *srch_pttrn,        /* pointer to the search pattern */
    uint16_t max_num_recs       /* maximum records to return */
    )
{
    SDC_SERVICE_SEARCH_REQ_T *prim;

    prim = zpnew(SDC_SERVICE_SEARCH_REQ_T);

    prim->type = SDC_SERVICE_SEARCH_REQ;
    prim->phandle = phandle;
    prim->bd_addr = *p_bd_addr;
    prim->size_srch_pttrn = size_srch_pttrn;
    prim->srch_pttrn = srch_pttrn;
    prim->max_num_recs = max_num_recs;

    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_service_attribute_req
 *
 *  DESCRIPTION
 *      Build and send a SDC_SERVICE_ATTRIBUTE_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_service_attribute_req(
    phandle_t phandle,          /* routing handle */
    BD_ADDR_T *p_bd_addr,       /* remote device */
    uint32_t svc_rec_hndl,      /* remote service handle */
    uint16_t size_attr_list,    /* size of attribute list */
    uint8_t *attr_list,         /* pointer to the attribute list */
    uint16_t max_num_attr       /* maximum bytes per response */
    )
{
    SDC_SERVICE_ATTRIBUTE_REQ_T *prim;

    prim = zpnew(SDC_SERVICE_ATTRIBUTE_REQ_T);

    prim->type = SDC_SERVICE_ATTRIBUTE_REQ;
    prim->phandle = phandle;
    prim->bd_addr = *p_bd_addr;
    prim->svc_rec_hndl = svc_rec_hndl;
    prim->size_attr_list = size_attr_list;
    prim->attr_list = attr_list;
    prim->max_num_attr = max_num_attr;

    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_service_search_attribute_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_SERVICE_SEARCH_ATTRIBUTE_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_service_search_attribute_req(
    phandle_t phandle,          /* routing handle */
    BD_ADDR_T *p_bd_addr,       /* remote device */
    uint16_t size_srch_pttrn,   /* size of search pattern */
    uint8_t *srch_pttrn,        /* pointer to the search pattern */
    uint16_t size_attr_list,    /* size of attribute list */
    uint8_t *attr_list,         /* pointer to the attribute list */
    uint16_t max_num_attr       /* maximum bytes per response */
    )
{
    SDC_SERVICE_SEARCH_ATTRIBUTE_REQ_T *prim;

    prim = zpnew(SDC_SERVICE_SEARCH_ATTRIBUTE_REQ_T);

    prim->type = SDC_SERVICE_SEARCH_ATTRIBUTE_REQ;
    prim->phandle = phandle;
    prim->bd_addr = *p_bd_addr;
    prim->size_srch_pttrn = size_srch_pttrn;
    prim->srch_pttrn = srch_pttrn;
    prim->size_attr_list = size_attr_list;
    prim->attr_list = attr_list;
    prim->max_num_attr = max_num_attr;

    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_terminate_primitive_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_TERMINATE_PRIMITIVE_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_terminate_primitive_req(
    phandle_t phandle           /* routing handle */
    )
{
    SDC_TERMINATE_PRIMITIVE_REQ_T *prim;

    prim = zpnew(SDC_TERMINATE_PRIMITIVE_REQ_T);

    prim->type = SDC_TERMINATE_PRIMITIVE_REQ;
    prim->phandle = phandle;

    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_open_search_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_OPEN_SEARCH_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_open_search_req(
    phandle_t phandle,          /* routing handle */
    BD_ADDR_T *p_bd_addr        /* remote device */
    )
{
    SDC_OPEN_SEARCH_REQ_T *prim;

    prim = zpnew(SDC_OPEN_SEARCH_REQ_T);

    prim->type = SDC_OPEN_SEARCH_REQ;
    prim->phandle = phandle;
    prim->bd_addr = *p_bd_addr;
    
    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_close_search_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_CLOSE_SEARCH_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_close_search_req(
    phandle_t phandle           /* routing handle */
    )
{
    SDC_CLOSE_SEARCH_REQ_T *prim;

    prim = zpnew(SDC_CLOSE_SEARCH_REQ_T);

    prim->type = SDC_CLOSE_SEARCH_REQ;
    prim->phandle = phandle;

    SDP_PutMsg(prim);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      sdc_config_req
 *
 *  DESCRIPTION
 *      Build and send an SDC_CONFIG_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sdc_config_req(
    phandle_t phandle,
    uint16_t mtu,
    uint16_t flags
    )
{
    SDC_CONFIG_REQ_T *prim;

    prim = zpnew(SDC_CONFIG_REQ_T);

    prim->type = SDC_CONFIG_REQ;
    prim->phandle = phandle;
    prim->l2cap_mtu = mtu;
    prim->flags = flags;

    SDP_PutMsg(prim);
}

#endif /* defined(INSTALL_SDP_MODULE) || defined(BUILD_FOR_HOST) */
