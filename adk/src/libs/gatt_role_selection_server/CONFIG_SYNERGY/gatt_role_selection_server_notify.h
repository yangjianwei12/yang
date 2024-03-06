/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_server.h"
#include "gatt_role_selection_server_private.h"
#include "gatt_role_selection_service.h"


void gattRoleSelectionServerSendStateNotification(GATT_ROLE_SELECTION_SERVER *instance,
                                                  uint32 conn_id,
                                                  GattRoleSelectionServiceMirroringState state);


void gattRoleSelectionServerSendFigureNotification(GATT_ROLE_SELECTION_SERVER *instance,
                                                   uint32 conn_id,
                                                   grss_figure_of_merit_t figure_of_merit);


void handleRoleSelectionServerStateChanged(GATT_ROLE_SELECTION_SERVER *instance,
                                           const ROLE_SELECTION_SERVER_INTERNAL_STATE_UPDATED_T *msg);


void handleRoleSelectionServerFigureChanged(GATT_ROLE_SELECTION_SERVER *instance,
                                           const ROLE_SELECTION_SERVER_INTERNAL_FIGURE_UPDATED_T *update);


void handleRoleSelectionNotificationCfm(GATT_ROLE_SELECTION_SERVER *instance,
                                        const CsrBtGattEventSendCfm *payload);

void sendInternalMirrorStateChanged(GATT_ROLE_SELECTION_SERVER *instance, uint32 cid);


void sendInternalFigureOfMeritChanged(GATT_ROLE_SELECTION_SERVER *instance, uint32 cid);


