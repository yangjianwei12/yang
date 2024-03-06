/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "gatt_mcs_client_private.h"
#include "gatt_mcs_client_common_util.h"

GattMcsClient *mcsClientMain;

bool gattRegisterMcsClient(GattClientRegistrationParams *regParam, GMCSC *gattMcsClient)
{
    CsrBtTypedAddr addr;

    gattMcsClient->srvcElem->gattId = CsrBtGattRegister(CSR_BT_MCS_CLIENT_IFACEQUEUE);

    if (gattMcsClient->srvcElem->gattId)
    {
        if (CsrBtGattClientUtilFindAddrByConnId(regParam->cid,
                                                &addr))
        {
            CsrBtGattClientRegisterServiceReqSend(gattMcsClient->srvcElem->gattId,
                                                  regParam->startHandle,
                                                  regParam->endHandle,
                                                  addr);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
        return FALSE;
}

ServiceHandle getMcsServiceHandle(GMCSC **gattMcsClient, CsrCmnList_t *list)
{
    ServiceHandleListElm_t *elem = MCS_ADD_SERVICE_HANDLE(*list);

    elem->service_handle = ServiceHandleNewInstance((void **) gattMcsClient, sizeof(GMCSC));

    if ((*gattMcsClient) != NULL)
        (*gattMcsClient)->srvcElem = elem;

    return elem->service_handle;
}

void initMcsServiceHandleList(CsrCmnListElm_t *elem)
{
    ServiceHandleListElm_t *cElem = (ServiceHandleListElm_t *) elem;

    cElem->service_handle = 0;
}

void gattMcsClientInit(void **gash)
{
    mcsClientMain = CsrPmemAlloc(sizeof(*mcsClientMain));
    *gash = mcsClientMain;

    CsrCmnListInit(&mcsClientMain->serviceHandleList, 0, initMcsServiceHandleList, NULL);
}

GattMcsClient *mcsClientGetMainInstance(void)
{
    return mcsClientMain;
}

/****************************************************************************/
#ifdef ENABLE_SHUTDOWN
void gattMcsClientDeInit(void **gash)
{
    CsrCmnListDeinit(&mcsClientMain->serviceHandleList);
    CsrPmemFree(mcsClientMain);
}
#endif

GattMcsClientStatus getMcsClientStatusFromGattStatus(status_t status)
{
    GattMcsClientStatus mcsStatus;

    switch(status)
    {
        case CSR_BT_GATT_RESULT_SUCCESS:
            mcsStatus = GATT_MCS_CLIENT_STATUS_SUCCESS;
            break;
        case CSR_BT_GATT_RESULT_TRUNCATED_DATA:
            mcsStatus = GATT_MCS_CLIENT_STATUS_TRUNCATED_DATA;
            break;
        default:
            mcsStatus = GATT_MCS_CLIENT_STATUS_FAILED;
            break;
    }
    return mcsStatus;
}

/* Gets info type from characteristic handle */
MediaPlayerAttribute getMcsCharacFromHandle(GMCSC *mcsClient, uint16 handle)
{
    if (handle == 0)
        return 0;
    else if(handle == mcsClient->handles.contentControlIdHandle)
        return MCS_CONTENT_CONTROL_ID;
    else if (handle == mcsClient->handles.currentGroupObjIdHandle)
        return MCS_CURRENT_GROUP_OBJ_ID;
    else if (handle == mcsClient->handles.currentTrackObjIdHandle)
        return MCS_CURRENT_TRACK_OBJ_ID;
    else if (handle == mcsClient->handles.currentTrackSegmentsObjIdHandle)
        return MCS_CURRENT_TRACK_SEGMENTS_OBJ_ID;
    else if (handle == mcsClient->handles.mediaControlPointHandle)
        return MCS_MEDIA_CONTROL_POINT;
    else if (handle == mcsClient->handles.mediaControlPointOpSuppHandle)
        return MCS_MEDIA_CONTROL_POINT_OP_SUPP;
    else if (handle == mcsClient->handles.mediaPlayerIconObjIdHandle)
        return MCS_MEDIA_PLAYER_ICON_OBJ_ID;
    else if (handle == mcsClient->handles.mediaPlayerIconUrlHandle)
        return MCS_MEDIA_PLAYER_ICON_URL;
    else if (handle == mcsClient->handles.mediaPlayerNameHandle)
        return MCS_MEDIA_PLAYER_NAME;
    else if (handle == mcsClient->handles.mediaStateHandle)
        return MCS_MEDIA_STATE;
    else if (handle == mcsClient->handles.nextTrackObjIdHandle)
        return MCS_NEXT_TRACK_OBJ_ID;
    else if (handle == mcsClient->handles.parentGroupObjIdHandle)
        return MCS_PARENT_GROUP_OBJ_ID;
    else if (handle == mcsClient->handles.playbackSpeedHandle)
        return MCS_PLAYBACK_SPEED;
    else if (handle == mcsClient->handles.playingOrderHandle)
        return MCS_PLAYING_ORDER;
    else if (handle == mcsClient->handles.playingOrderSuppHandle)
        return MCS_PLAYING_ORDER_SUPP;
    else if (handle == mcsClient->handles.searchControlPointHandle)
        return MCS_SEARCH_CONTROL_POINT;
    else if (handle == mcsClient->handles.searchResultsObjIdHandle)
        return MCS_SEARCH_RESULTS_OBJ_ID;
    else if (handle == mcsClient->handles.seekingSpeedHandle)
        return MCS_SEEKING_SPEED;
    else if (handle == mcsClient->handles.trackChangedHandle)
        return MCS_TRACK_CHANGED;
    else if (handle == mcsClient->handles.trackDurationHandle)
        return MCS_TRACK_DURATION;
    else if (handle == mcsClient->handles.trackPositionHandle)
        return MCS_TRACK_POSITION;
    else if (handle == mcsClient->handles.trackTitleHandle)
        return MCS_TRACK_TITLE;
    else
        return 0;
}

/* Gets info type from characteristic handle */
MediaPlayerAttribute getMcsCharacFromCccHandle(GMCSC *mcsClient, uint16 handle)
{
    if (handle == 0)
        return 0;
    else if (handle == mcsClient->handles.currentGroupObjIdCccHandle)
        return MCS_CURRENT_GROUP_OBJ_ID;
    else if (handle == mcsClient->handles.currentTrackObjIdCccHandle)
        return MCS_CURRENT_TRACK_OBJ_ID;
    else if (handle == mcsClient->handles.mediaControlPointCccHandle)
        return MCS_MEDIA_CONTROL_POINT;
    else if (handle == mcsClient->handles.mediaControlPointOpSuppCccHandle)
        return MCS_MEDIA_CONTROL_POINT_OP_SUPP;
    else if (handle == mcsClient->handles.mediaPlayerNameCccHandle)
        return MCS_MEDIA_PLAYER_NAME;
    else if (handle == mcsClient->handles.mediaStateCccHandle)
        return MCS_MEDIA_STATE;
    else if (handle == mcsClient->handles.nextTrackObjIdCccHandle)
        return MCS_NEXT_TRACK_OBJ_ID;
    else if (handle == mcsClient->handles.parentGroupObjIdCccHandle)
        return MCS_PARENT_GROUP_OBJ_ID;
    else if (handle == mcsClient->handles.playbackSpeedCccHandle)
        return MCS_PLAYBACK_SPEED;
    else if (handle == mcsClient->handles.playingOrderCccHandle)
        return MCS_PLAYING_ORDER;
    else if (handle == mcsClient->handles.searchControlPointCccHandle)
        return MCS_SEARCH_CONTROL_POINT;
    else if (handle == mcsClient->handles.searchResultsObjIdCccHandle)
        return MCS_SEARCH_RESULTS_OBJ_ID;
    else if (handle == mcsClient->handles.seekingSpeedCccHandle)
        return MCS_SEEKING_SPEED;
    else if (handle == mcsClient->handles.trackChangedCccHandle)
        return MCS_TRACK_CHANGED;
    else if (handle == mcsClient->handles.trackDurationCccHandle)
        return MCS_TRACK_DURATION;
    else if (handle == mcsClient->handles.trackPositionCccHandle)
        return MCS_TRACK_POSITION;
    else if (handle == mcsClient->handles.trackTitleCccHandle)
        return MCS_TRACK_TITLE;
    else
        return 0;
}

GattMcsClientDeviceData *GattMcsClientGetHandlesReq(ServiceHandle clntHndl)
{
    GMCSC *mcsClient = ServiceHandleGetInstanceData(clntHndl);

    if (mcsClient)
    {
        GattMcsClientDeviceData *mcsHandles = CsrPmemZalloc(sizeof(GattMcsClientDeviceData));

        memcpy(mcsHandles, &(mcsClient->handles), sizeof(GattMcsClientDeviceData));

        return mcsHandles;
    }

    return NULL;
}
