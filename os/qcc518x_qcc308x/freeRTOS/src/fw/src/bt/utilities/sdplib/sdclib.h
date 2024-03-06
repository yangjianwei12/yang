/*******************************************************************************

Copyright (C) 2007 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

DESCRIPTION:       SDC access library for building SDC downstream primitives.

REVISION:          $Revision: #1 $

*******************************************************************************/
#ifndef _SDCLIB_H_
#define _SDCLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
    Public Defines
 *============================================================================*/
/* None */

/*============================================================================*
    Public Data Types
 *============================================================================*/
/* None */

/*============================================================================*
    Public Data
 *============================================================================*/
/* None */

/*============================================================================*
    Public Functions
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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

#ifdef __cplusplus
}
#endif 


#endif

