/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/
#include "gatt_gmas_server_access.h"
#include "gatt_gmas_server_private.h"

/***************************************************************************
NAME
    gattGmasServerHandleRoleAccess

DESCRIPTION
    Deals with access of the SIG_GMAS_HANDLE_ROLE handle.
*/
static void gattGmasServerHandleRoleAccess(GGMAS *const gmasServer,
                                           GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint8* value = NULL;
        value = (uint8*) CsrPmemZalloc(GATT_GMAS_SERVER_CHAR_SIZE);

        value[0] = gmasServer->data.role;

        gattGmasServerSendAccessRsp(gmasServer->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    GATT_GMAS_SERVER_CHAR_SIZE,
                                    value);
    }
    else
    {
        gattGmasServerSendAccessErrorRsp(gmasServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

static void gattGmasServerHandleUnicastFeaturesAccess(GGMAS *const gmasServer,
                                                      GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint8* value = NULL;
        value = (uint8*) CsrPmemZalloc(GATT_GMAS_SERVER_CHAR_SIZE);

#if defined(ENABLE_GMAP_UGG_BGS)
        if (accessInd->handle == HANDLE_GMAS_UGG_FEATURES)
            value[0] = gmasServer->data.uggFeatures;
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        if (accessInd->handle == HANDLE_GMAS_UGT_FEATURES)
            value[0] = gmasServer->data.ugtFeatures;
#endif

        gattGmasServerSendAccessRsp(gmasServer->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    GATT_GMAS_SERVER_CHAR_SIZE,
                                    value);
    }
    else
    {
        gattGmasServerSendAccessErrorRsp(gmasServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

static void gattGmasServerHandleBroadcastFeaturesAccess(GGMAS *const gmasServer,
                                                        GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint8* value = NULL;
        value = (uint8*) CsrPmemZalloc(GATT_GMAS_SERVER_CHAR_SIZE);

#if defined(ENABLE_GMAP_UGG_BGS)
        if (accessInd->handle == HANDLE_GMAS_BGS_FEATURES)
            value[0] = gmasServer->data.bgsFeatures;
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        if (accessInd->handle == HANDLE_GMAS_BGR_FEATURES)
            value[0] = gmasServer->data.bgrFeatures;
#endif
        gattGmasServerSendAccessRsp(gmasServer->gattId,
                                    accessInd->cid,
                                    accessInd->handle,
                                    CSR_BT_GATT_RESULT_SUCCESS,
                                    GATT_GMAS_SERVER_CHAR_SIZE,
                                    value);
    }
    else
    {
        gattGmasServerSendAccessErrorRsp(gmasServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

/***************************************************************************/
void gattGmasServerHandleAccessIndication(GGMAS *gmasServer,
                                          GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd)
{
    switch (accessInd->handle)
    {
        case HANDLE_GMAS_ROLE:
        {
            GATT_GMAS_SERVER_DEBUG("HANDLE_GMAS_ROLE\n");
            gattGmasServerHandleRoleAccess(gmasServer,
                                           accessInd);
        }
        break;

#if defined(ENABLE_GMAP_UGG_BGS)
        case HANDLE_GMAS_UGG_FEATURES:
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        case HANDLE_GMAS_UGT_FEATURES:
#endif
        {
            GATT_GMAS_SERVER_DEBUG("HANDLE Unicast Features\n");
            gattGmasServerHandleUnicastFeaturesAccess(gmasServer,
                                                      accessInd);
        }
        break;

#if defined(ENABLE_GMAP_UGG_BGS)
        case HANDLE_GMAS_BGS_FEATURES:
#endif
#if defined(ENABLE_GMAP_UGT_BGR)
        case HANDLE_GMAS_BGR_FEATURES:
#endif
        {
            GATT_GMAS_SERVER_DEBUG("HANDLE Broadcast Features\n");
            gattGmasServerHandleBroadcastFeaturesAccess(gmasServer,
                                                        accessInd);
        }
        break;

        default:
        {
            GATT_GMAS_SERVER_DEBUG("Unhandled access indication handle:%d\n", accessInd->handle);
        }
        break;
    }
}
