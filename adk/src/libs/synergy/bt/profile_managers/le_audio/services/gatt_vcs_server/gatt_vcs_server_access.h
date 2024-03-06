/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_VCS_SERVER_ACCESS_H_
#define GATT_VCS_SERVER_ACCESS_H_

#include "gatt_vcs_server.h"
#include "gatt_vcs_server_db.h"
#include "gatt_vcs_server_common.h"

#define GATT_VCS_SERVER_MUTE_VALUE    (1)
#define GATT_VCS_SERVER_UNMUTE_VALUE  (0)

/* Values for Volume Flags Characteristic */
#define GATT_VCS_SERVER_VOLUME_SETTING_NOT_PERSISTED  (0)
#define GATT_VCS_SERVER_VOLUME_SETTING_PERSISTED      (1)

/**************************************************************************
NAME
    vcsServerSetVolumeFlag

DESCRIPTION
    Set the Volume Flag characteristic to GATT_VCS_SERVER_VOLUME_SETTING_PERSISTED
    when the Volume setting is changed the first time.
*/
void vcsServerSetVolumeFlag(GVCS *volume_control_server);

/***************************************************************************
NAME
    vcsServerHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the VCS Server library.
*/
void vcsServerHandleAccessIndication(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        );

#endif /* GATT_VCS_SERVER_ACCESS_H_ */
