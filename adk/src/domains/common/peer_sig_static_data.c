/*
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_sig_static_data.c
\brief      Statically defined data may be peer signalled by using this API. As images on both buds are
            identical, the address of static data is the same and data may be peer signalled by simply
            providing address and size.
            Note: As the peer signallng implementation limits single object within marshalling descriptor
            to max. 255 bytes, any data larger than 255 bytes will be transmitted in multiple
            max. 255 byte chunks and then reassembled on the receiver before storing the static data.
            Usage:
            1. PeerSigStaticData_Register
            2. PeerSigStaticData_Transmit (primary)
            ... Data transmitted will be received on Secondary by the registered peer signalling task.
            ... Task receives a PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND message id and calls...
            3. PeerSigStaticData_Receive (secondary) to write received data into memory
*/

#include "peer_sig_static_data.h"
#include "peer_sig_static_data_descriptor.h"

#include <peer_signalling.h>
#include <panic.h>
#include <logging.h>
#include <limits.h>


#define MARSHAL_TYPE_peer_sig_static_header_t MARSHAL_TYPE_uint16

typedef struct
{
    uint16 num_bytes_consumed;
    peer_sig_static_data_t received;
} peer_sig_static_received_t;

static peer_sig_static_received_t *peer_sig_static_data_received = NULL;

static uint32 peerSigStaticData_GetStaticDataSize(const void *object,
                             const marshal_member_descriptor_t *member,
                             uint32 arrayElement);

static const marshal_member_descriptor_t member_descriptors_peer_sig_static_data[] =
{
    MAKE_MARSHAL_MEMBER(peer_sig_static_data_t, peer_sig_static_header_t, header),
    MAKE_MARSHAL_MEMBER(peer_sig_static_data_t, uint32, data_addr),
    MAKE_MARSHAL_MEMBER(peer_sig_static_data_t, uint8, data_length),
    MAKE_MARSHAL_MEMBER_ARRAY(peer_sig_static_data_t, uint8, data, 1),
};

static const marshal_type_descriptor_dynamic_t marshal_type_descriptor_peer_sig_static_data =
    MAKE_MARSHAL_TYPE_DEFINITION_HAS_DYNAMIC_ARRAY(
        peer_sig_static_data_t,
        member_descriptors_peer_sig_static_data,
        peerSigStaticData_GetStaticDataSize);

/* Use xmacro to expand type table as array of type descriptors */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const descriptor_peer_sig_static_data[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

static uint32 peerSigStaticData_GetStaticDataSize(const void *object,
                             const marshal_member_descriptor_t *member,
                             uint32 arrayElement)
{
    UNUSED(member);
    UNUSED(arrayElement);

    peer_sig_static_data_t* peer_sig_static_data = (peer_sig_static_data_t*)object;
    return peer_sig_static_data->data_length;
}

static void* peerSigStaticData_CreateMessage(uint16 total_data_length, const uint8* data_addr, uint16 data_size)
{
    peer_sig_static_data_t* temp = (peer_sig_static_data_t*)PanicUnlessMalloc(sizeof(peer_sig_static_data_t)+data_size);
    temp->header.total_data_length = total_data_length;
    temp->data_addr = (uint32)data_addr;
    temp->data_length = data_size;
    memcpy(temp->data, (void*)temp->data_addr, temp->data_length);
    return temp;
}

static void peerSigStaticData_CopyReceivedDataToBuffer(const peer_sig_static_data_t* rx)
{
    if (peer_sig_static_data_received == NULL)
    {
        peer_sig_static_data_received = (peer_sig_static_received_t*)PanicUnlessMalloc(sizeof(peer_sig_static_received_t)+rx->header.total_data_length);
        peer_sig_static_data_received->received.header.total_data_length = rx->header.total_data_length;
        peer_sig_static_data_received->received.data_addr = rx->data_addr;
        peer_sig_static_data_received->received.data_length = 0; /* not used */
        peer_sig_static_data_received->num_bytes_consumed = 0;
        DEBUG_LOG("PeerSigStaticData_Receive: Create receive buffer length %d", rx->header.total_data_length);
    }

    DEBUG_LOG("PeerSigStaticData_Receive: Add to receive buffer at offset %d", peer_sig_static_data_received->num_bytes_consumed);
    memcpy(&peer_sig_static_data_received->received.data[peer_sig_static_data_received->num_bytes_consumed] , rx->data, rx->data_length);
    peer_sig_static_data_received->num_bytes_consumed += rx->data_length;
    DEBUG_LOG("PeerSigStaticData_Receive: New buffered data, consumed %d Total length %d", peer_sig_static_data_received->num_bytes_consumed, peer_sig_static_data_received->received.header.total_data_length);
}


void PeerSigStaticData_Receive(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch(ind->type)
    {
        case MARSHAL_TYPE_peer_sig_static_data:
        {
            peer_sig_static_data_t* temp = ((peer_sig_static_data_t*)ind->msg);

            DEBUG_LOG("PeerSigStaticData_Receive: PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND data_length %d total_length %d data %p ", temp->data_length, temp->header.total_data_length, temp->data);
            if (temp->header.total_data_length > temp->data_length)
            {
                peerSigStaticData_CopyReceivedDataToBuffer(temp);

                if (peer_sig_static_data_received->num_bytes_consumed >= peer_sig_static_data_received->received.header.total_data_length)
                {
                    DEBUG_LOG("PeerSigStaticData_Receive: Copy receive buffer length %d to data_addr %p", peer_sig_static_data_received->received.header.total_data_length, peer_sig_static_data_received->received.data_addr);
                    memcpy((void*)peer_sig_static_data_received->received.data_addr, peer_sig_static_data_received->received.data, peer_sig_static_data_received->received.header.total_data_length);
                    free(peer_sig_static_data_received);
                    peer_sig_static_data_received = NULL;
                }
            }
            else
            {
                DEBUG_LOG("PeerSigStaticData_Receive: PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND Copy length %d to data_addr %p", temp->data_length, temp->data_addr);
                memcpy((void*)temp->data_addr, temp->data, temp->data_length);
            }
        }
        break;

        default:
            Panic();
        break;
    }

    /* Free unmarshalled message after use. */
    free(ind->msg);
}


void PeerSigStaticData_Transmit(Task task, peerSigMsgChannel channel, void* data_addr, uint16 data_size)
{
    if(appPeerSigIsConnected())
    {
        uint16 size_data_transmitted = 0;
        uint16 total_data_length = data_size;

        while (data_size != 0)
        {
            uint16 size_data_to_transmit = (data_size > UCHAR_MAX) ? UCHAR_MAX : data_size;
            uint8 *data_to_transmit = data_addr;

            data_to_transmit += size_data_transmitted;

            DEBUG_LOG("PeerSigStaticData_Transmit: send peer data_to_transmit %p size %d channel %d", data_to_transmit, size_data_to_transmit, channel);
            appPeerSigMarshalledMsgChannelTx(task,
                                            channel,
                                            peerSigStaticData_CreateMessage(total_data_length, data_to_transmit, size_data_to_transmit),
                                            MARSHAL_TYPE_peer_sig_static_data);
            data_size -= size_data_to_transmit;
            size_data_transmitted += size_data_to_transmit;
        }
    }
}

void PeerSigStaticData_Register(Task task, peerSigMsgChannel channel)
{
    DEBUG_LOG("PeerSigStaticData_Register channel=enum:peerSigMsgChannel:%d", channel);
    appPeerSigClientRegister(task);
    appPeerSigMarshalledMsgChannelTaskRegister(task,
                                               channel,
                                               descriptor_peer_sig_static_data,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
}

void PeerSigStaticData_PeerConnectionStatusChange(PEER_SIG_CONNECTION_IND_T* ind)
{
    if (ind->status == peerSigStatusLinkLoss || ind->status == peerSigStatusDisconnected)
    {
        if (peer_sig_static_data_received != NULL)
        {
            free(peer_sig_static_data_received);
            peer_sig_static_data_received = NULL;
        }
    }
}
