/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_msg_stream_dev_action.h
\brief      Header file for fast pair device action.
*/

#ifndef FASTPAIR_MSG_STREAM_DEV_ACTION_H
#define FASTPAIR_MSG_STREAM_DEV_ACTION_H

#include <marshal_common.h>
#include <marshal.h>
#include <message.h>
#include <task_list.h>

#include "kymera.h"

#define FP_STOP_RING_CURRENT    0
#define FP_STOP_RING_BOTH       1

#define FASTPAIR_DEVICEACTION_RING_RSP_ADD_DATA_LEN      (1)
#define FASTPAIR_DEVICEACTION_STOP_RING                  (0x00)
#define FASTPAIR_DEVICEACTION_RING_RIGHT_MUTE_LEFT       (0x01)
#define FASTPAIR_DEVICEACTION_RING_LEFT_MUTE_RIGHT       (0x02)
#define FASTPAIR_DEVICEACTION_RING_RIGHT_LEFT            (0x03)
#define FASTPAIR_DEVICEACTION_RING_CASE                  (0x04)
#define FASTPAIR_DEVICEACTION_RING_ALL                   (0xFF)
#define FASTPAIR_SEEKER_INVALID                          (0xFF)
#define FASTPAIR_DEVICEACTION_RING_VOL_DEFAULT           (0x00)

/* Message Code for Device action message group */
typedef enum
{
    FASTPAIR_MESSAGESTREAM_DEVACTION_RING_EVENT = 0x01
} FASTPAIR_MESSAGESTREAM_DEVACTION_MESSAGE_CODE;

/* Device Action data structure*/
typedef struct{
    uint8 ring_component;
    uint16 ring_timeout;
    uint8 seeker_number;
}fast_pair_msg_stream_dev_action;

/* Ringtone volume level to ramp up volume for ringing device */
typedef enum
{
    ring_vol32,
    ring_vol64,
    ring_vol128,
    ring_volmax
} ringtone_volume;

typedef struct
{
    TaskData task;
    /* Indicates if device is currently ringing or not. */
    bool is_device_ring;
    /* Volume level indicator for the current ringtone being played. */
    ringtone_volume vol_level;
    /* Play a ringtone with a volume level for certain no of time
       before it is played to a max level. */
    uint16 ringtimes;
} fp_ring_device_task_data_t;

typedef enum
{
    fast_pair_ring_event = 0,
    fast_pair_ring_stop_event
} fast_pair_ring_device_event_id;

extern fp_ring_device_task_data_t ring_device;
extern fast_pair_msg_stream_dev_action dev_action_data;

#define fpRingDevice_GetTaskData() (&ring_device)
#define fpRingDevice_GetTask() (&ring_device.task)

typedef struct fast_pair_ring_device_req
{
    bool  ring_start_stop;
    uint8 ring_volume;
    uint16 ring_time;
} fast_pair_ring_device_req_t;

typedef struct fast_pair_ring_Device_cfm
{
    bool synced;
} fast_pair_ring_device_cfm_t;

/* Fast Pair Find My Device ringing states -equivalent states to google finder ring functionality */
typedef enum
{
    fast_pair_ringing_started = 0x00,
    fast_pair_ringing_failed_to_start_or_stop = 0x01,
    fast_pair_ringing_stopped_timeout = 0x02,
    fast_pair_ringing_stopped_button_press = 0x03,
    fast_pair_ringing_stopped_gatt_request = 0x4,
    fast_pair_ringing_state_invalid = 0xFF
}fast_pair_ring_device_ringing_state;

/* Callback function to notify the registered client (as of now only Google Finder) about the ring/mute operation */
typedef void (*fastPair_RingDeviceCallBack)(uint8 ring_component, fast_pair_ring_device_ringing_state ring_state, uint16 ring_timeout);

/*! \brief Public API to register ring device callback. Used by Google Finder feaure.
 */
void FastPair_RegisterRingDeviceCallback(fastPair_RingDeviceCallBack ring_callback);

/*! \brief Public API to perform ring and mute device operations.

    \param fp_seeker_number - Fast Pair Seeker Index (In case of Google Finder, 0x0F is getting used).
    \param ring_component - Requested ringing components
    \param ring_volume - Ring volume to start the ringing from, then it will gradually increase till the volume max level.
    \param ring_timeout - Ring Duration

    \return TRUE if the operation is successful, FALSE otherwise
*/
bool FastPair_RingMuteDevice(uint8 fp_seeker_number, uint8 ring_component, uint8 ring_volume, uint16 ring_timeout);

/*! Create base list of marshal types ring device will use. */
#define MARSHAL_TYPES_TABLE_RING_DEVICE(ENTRY) \
    ENTRY(fast_pair_ring_device_req_t) \
    ENTRY(fast_pair_ring_device_cfm_t)

/*! X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),

enum MARSHAL_TYPES_RING_DEVICE_SYNC
{
    /*! common types must be placed at the start of the enum */
    DUMMY_DEVICE_ACTION_SYNC = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /*! now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE_RING_DEVICE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_DEVICE_ACTION_SYNC_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/*! Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const fp_ring_device_marshal_type_descriptors[];

void fastPair_MsgStreamDevAction_Init(void);

#endif /* FASTPAIR_MSG_STREAM_DEV_ACTION_H */
