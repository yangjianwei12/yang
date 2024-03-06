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
    Interface Header Files
 *============================================================================*/


/*============================================================================*
    Local Header File
 *============================================================================*/


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
    )
{
    SDS_REGISTER_REQ_T *prim;

    prim = zpnew(SDS_REGISTER_REQ_T);

    prim->type = SDS_REGISTER_REQ;
    prim->phandle = phandle;
    prim->service_rec = service_rec;
    prim->num_rec_bytes = num_rec_bytes;

    SDP_PutMsg(prim);
}

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
    )
{
    SDS_UNREGISTER_REQ_T *prim;

    prim = zpnew(SDS_UNREGISTER_REQ_T);

    prim->type = SDS_UNREGISTER_REQ;
    prim->phandle = phandle;
    prim->svc_rec_hndl = svc_rec_hndl;

    SDP_PutMsg(prim);
}

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
    )
{
    SDS_CONFIG_REQ_T *prim;

    prim = zpnew(SDS_CONFIG_REQ_T);

    prim->type = SDS_CONFIG_REQ;
    prim->phandle = phandle;
    prim->l2cap_mtu = mtu;
    prim->flags = flags;

    SDP_PutMsg(prim);
}

#endif /* defined(INSTALL_SDP_MODULE) || defined(BUILD_FOR_HOST) */
