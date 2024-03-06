/****************************************************************************
* Copyright (c) 2021 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_SERVER_PACS_H_
#define BAP_SERVER_PACS_H_

#include "gatt_pacs_server.h"
#include "gatt_ascs_server.h"


typedef struct
{
    uint8 numberOfRegisteredRecords;
    uint16 pacRecordHandles[];
} registeredPacsRecords;

/***************************************************************************
NAME
    bapMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the Server role.
*/
void bapServerHandleGattPacsServerMsg(BAP *bapInst, void *message);

bool bapServerIsCodecConfigSuppported(ServiceHandle handle, GattAscsServerConfigureCodecClientInfo * client_codec,
                                    uint8 *samplingValue, uint8 *frameDurValue);

uint16 bapServerPacsUtilitiesGetSinkAudioContextAvailability(void);

uint16 bapServerPacsUtilitiesGetSourceAudioContextAvailability(void);

#endif
