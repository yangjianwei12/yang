/*
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_sig_static_data_descriptor.h
\brief      Peer signalling static data descriptor
*/

#ifndef PEER_SIG_STATIC_DATA_DESCRIPTOR_H
#define PEER_SIG_STATIC_DATA_DESCRIPTOR_H

#include <marshal_common.h>
#include <marshal_common_desc.h>
#include <marshal.h>

/*! Header for the peer signalled static data. */
typedef struct
{
    /*! The total length of static data transmitted. Data larger than 255 bytes will be sent as multiple chunks. */
    uint16 total_data_length;
} peer_sig_static_header_t;

/*! The peer signalled static data. */
typedef struct
{
    /*! Header information. */
    peer_sig_static_header_t header;
    /*! The address of the static data that is being peer signalled. */
    uint32 data_addr;
    /*! The length of the chunk of data being peer signalled (max 255 bytes) */
    uint8  data_length;
    /*! The data being peer signalled */
    uint8  data[1];
} peer_sig_static_data_t;

/* Create base list of marshal types the key sync will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(peer_sig_static_data)

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /* now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

#endif // PEER_SIG_STATIC_DATA_DESCRIPTOR_H
