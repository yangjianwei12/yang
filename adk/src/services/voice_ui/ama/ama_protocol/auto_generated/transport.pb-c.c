/*****************************************************************************
Copyright (c) 2018 - 2021 Qualcomm Technologies International, Ltd.
******************************************************************************/

/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: transport.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "transport.pb-c.h"
void   connection_details__init
                     (ConnectionDetails         *message)
{
  static const ConnectionDetails init_value = CONNECTION_DETAILS__INIT;
  *message = init_value;
}
size_t connection_details__get_packed_size
                     (const ConnectionDetails *message)
{
  assert(message->base.descriptor == &connection_details__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t connection_details__pack
                     (const ConnectionDetails *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &connection_details__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t connection_details__pack_to_buffer
                     (const ConnectionDetails *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &connection_details__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
ConnectionDetails *
       connection_details__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (ConnectionDetails *)
     protobuf_c_message_unpack (&connection_details__descriptor,
                                allocator, len, data);
}
void   connection_details__free_unpacked
                     (ConnectionDetails *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &connection_details__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   upgrade_transport__init
                     (UpgradeTransport         *message)
{
  static const UpgradeTransport init_value = UPGRADE_TRANSPORT__INIT;
  *message = init_value;
}
size_t upgrade_transport__get_packed_size
                     (const UpgradeTransport *message)
{
  assert(message->base.descriptor == &upgrade_transport__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t upgrade_transport__pack
                     (const UpgradeTransport *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &upgrade_transport__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t upgrade_transport__pack_to_buffer
                     (const UpgradeTransport *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &upgrade_transport__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
UpgradeTransport *
       upgrade_transport__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (UpgradeTransport *)
     protobuf_c_message_unpack (&upgrade_transport__descriptor,
                                allocator, len, data);
}
void   upgrade_transport__free_unpacked
                     (UpgradeTransport *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &upgrade_transport__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   switch_transport__init
                     (SwitchTransport         *message)
{
  static const SwitchTransport init_value = SWITCH_TRANSPORT__INIT;
  *message = init_value;
}
size_t switch_transport__get_packed_size
                     (const SwitchTransport *message)
{
  assert(message->base.descriptor == &switch_transport__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t switch_transport__pack
                     (const SwitchTransport *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &switch_transport__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t switch_transport__pack_to_buffer
                     (const SwitchTransport *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &switch_transport__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
SwitchTransport *
       switch_transport__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (SwitchTransport *)
     protobuf_c_message_unpack (&switch_transport__descriptor,
                                allocator, len, data);
}
void   switch_transport__free_unpacked
                     (SwitchTransport *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &switch_transport__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor connection_details__field_descriptors[1] =
{
  {
    "identifier",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(ConnectionDetails, identifier),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned connection_details__field_indices_by_name[] = {
  0,   /* field[0] = identifier */
};
static const ProtobufCIntRange connection_details__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor connection_details__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "ConnectionDetails",
  "ConnectionDetails",
  "ConnectionDetails",
  "",
  sizeof(ConnectionDetails),
  1,
  connection_details__field_descriptors,
  connection_details__field_indices_by_name,
  1,  connection_details__number_ranges,
  (ProtobufCMessageInit) connection_details__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor upgrade_transport__field_descriptors[1] =
{
  {
    "transport",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(UpgradeTransport, transport),
    &transport__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned upgrade_transport__field_indices_by_name[] = {
  0,   /* field[0] = transport */
};
static const ProtobufCIntRange upgrade_transport__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor upgrade_transport__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "UpgradeTransport",
  "UpgradeTransport",
  "UpgradeTransport",
  "",
  sizeof(UpgradeTransport),
  1,
  upgrade_transport__field_descriptors,
  upgrade_transport__field_indices_by_name,
  1,  upgrade_transport__number_ranges,
  (ProtobufCMessageInit) upgrade_transport__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor switch_transport__field_descriptors[1] =
{
  {
    "new_transport",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(SwitchTransport, new_transport),
    &transport__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned switch_transport__field_indices_by_name[] = {
  0,   /* field[0] = new_transport */
};
static const ProtobufCIntRange switch_transport__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor switch_transport__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "SwitchTransport",
  "SwitchTransport",
  "SwitchTransport",
  "",
  sizeof(SwitchTransport),
  1,
  switch_transport__field_descriptors,
  switch_transport__field_indices_by_name,
  1,  switch_transport__number_ranges,
  (ProtobufCMessageInit) switch_transport__init,
  NULL,NULL,NULL    /* reserved[123] */
};
