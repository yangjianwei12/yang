#ifndef CSR_HYDRA_STREAMS__
#define CSR_HYDRA_STREAMS__

/*****************************************************************************
Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #58 $
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HYDRA) || defined(CAA)

#include "bdaddr/bdaddr.h"
#include "message/system_message.h"
#include "stream.h"
#include "sink.h"
#include "source.h"
#include "transform.h"

#else /* HYDRA */
    
struct __SOURCE;
typedef struct __SOURCE *Source;

struct __SINK;
typedef struct __SINK *Sink;

#define SYSTEM_MESSAGE_BASE_                0x8000
#define MESSAGE_MORE_DATA                   (SYSTEM_MESSAGE_BASE_ + 33)
#define MESSAGE_MORE_SPACE                  (SYSTEM_MESSAGE_BASE_ + 34)


typedef struct
{
    Source source;     /* The source which has more data. */
} MessageMoreData;

typedef struct
{
    Sink sink;        /* The sink which has more space. */
} MessageMoreSpace;

typedef enum
{
    VM_SOURCE_MESSAGES = 0x0002,
    STREAM_SOURCE_HANDOVER_POLICY = 0x1300
} stream_config_key;

typedef enum
{
    VM_MESSAGES_ALL,                /*!< Send all messages to the registered task */
    VM_MESSAGES_SOME,               /*!< Send at most one message at a time to the registered task */
    VM_MESSAGES_NONE                /*!< Send no messages to the registered task */
} vm_messages_settings;

typedef enum
{
    SOURCE_HANDOVER_DISALLOW = 0,        /*!< Disallow handover (default policy) */
    SOURCE_HANDOVER_ALLOW_WITHOUT_DATA,  /*!< Allow handover if source has no pending data */
    SOURCE_HANDOVER_ALLOW                /*!< Allow handover with/without data. Note: Source data 
                                              may be lost during handover with this policy */
}source_handover_policy;

extern Sink StreamRfcommSink(uint16 conn_id);
extern Sink StreamL2capSink(uint16 cid);
extern Source StreamSourceFromSink(Sink sink);
extern Sink StreamSinkFromSource(Source source);

extern uint16 SourceSize(Source source);
extern uint16 SourceBoundary(Source source);
extern const uint8 *SourceMap(Source source);
extern void SourceDrop(Source source, uint16 amount);
extern bool SourceIsValid(Source source);
extern bool SourceConfigure(Source source,
                            stream_config_key key,
                            CsrUint32 value);

extern uint16 SinkGetRfcommConnId(Sink sink);
extern uint16 SinkGetL2capCid(Sink sink);
extern uint16 SinkSlack(Sink sink);
extern uint16 SinkClaim(Sink sink, uint16 extra);
extern uint8 *SinkMap(Sink sink);
extern bool SinkFlush(Sink sink, uint16 amount);
extern bool SinkIsValid(Sink sink);

extern CsrBool StreamConnectDispose(Source source);
extern void *TransformFromSink(Sink sink);
extern void *TransformFromSource(Source source);

extern void stream_msg_free(uint16 id, void *msg);

#endif /* !HYDRA !CAA */

#define INVALID_SINK_OFFSET                 0xFFFF

typedef enum
{
    RFCOMM_ID = 0x01,
    L2CAP_ID,
} transport_id;

extern void SynergyStreamsSinkRegister(CsrUint8 qID, Sink sink);
extern void SynergyStreamsSourceRegister(CsrUint8 qID, Source src);

#define SynergyStreamsSinkUnregister(_sink)     ((void) MessageStreamTaskFromSink(_sink, NULL))
#define SynergyStreamsSourceUnregister(_src)    ((void) MessageStreamTaskFromSource(_src, NULL))

#ifdef __cplusplus
}
#endif

#endif /* CSR_HYDRA_STREAMS__ */
