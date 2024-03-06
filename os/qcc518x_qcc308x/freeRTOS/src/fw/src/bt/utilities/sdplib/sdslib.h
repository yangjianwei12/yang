/*******************************************************************************

Copyright (C) 2007 - 2016 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 2001

DESCRIPTION:       SDS access library for building SDS downstream primitives.

REVISION:          $Revision: #1 $

*******************************************************************************/
#ifndef _SDSLIB_H_
#define _SDSLIB_H_


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
 *      sds_register_req
 *
 *  DESCRIPTION
 *      Build and send an SDS_REGISTER_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sds_register_req(
    phandle_t phandle,      /* routing handle */
    uint8_t *service_rec,   /* Pointer to service record data */
    uint16_t num_rec_bytes  /* Number of bytes in the service record data  */
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      sds_unregister_req
 *
 *  DESCRIPTION
 *      Build and send an SDS_UNREGISTER_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sds_unregister_req(
    phandle_t phandle,      /* routing handle */
    uint32_t svc_rec_hndl   /* service record handle */
    );

/*----------------------------------------------------------------------------*
 *  NAME
 *      sds_config_req
 *
 *  DESCRIPTION
 *      Build and send an SDS_CONFIG_REQ primitive to SDP.
 *
 *  RETURNS
 *      void 
 *
 *----------------------------------------------------------------------------*/

void sds_config_req(
    phandle_t phandle,
    uint16_t mtu,
    uint16_t flags
    );

#ifdef __cplusplus
}
#endif 


#endif

