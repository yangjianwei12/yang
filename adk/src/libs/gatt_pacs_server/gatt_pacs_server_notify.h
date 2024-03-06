/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_NOTIFY_H_
#define GATT_PACS_SERVER_NOTIFY_H_

/***************************************************************************
NAME
    pacsServerNotifyAudioLocationChange

DESCRIPTION
    Notify Sink or Source AudioLocation change to connected client(s).
*/
void pacsServerNotifyAudioLocationChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid);

/***************************************************************************
NAME
    pacsServerNotifyAudioContextsChange

DESCRIPTION
    Notify Sink or Source AudioContexts change to connected client(s).
*/
void pacsServerNotifyAudioContextsChange(GPACSS_T *pacs_server, bool supportedAudioContexts, connection_id_t cid);

/***************************************************************************
NAME
    pacsServerNotifyPacRecordChange

DESCRIPTION
    Notify Sink or Source PAC record change to connected client(s).
*/
void pacsServerNotifyPacRecordChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid);

/***************************************************************************
NAME
    pacsServerNotifyVsPacRecordChange

DESCRIPTION
    Notify Sink or Source VS PAC record change to connected client(s).
*/
void pacsServerNotifyVsPacRecordChange(GPACSS_T *pacs_server, bool audioSink, connection_id_t cid);

#endif

