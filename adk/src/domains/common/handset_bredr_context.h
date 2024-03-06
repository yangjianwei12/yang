/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Data type for describing the current BREDR context of a handset device.
*/

#ifndef HANDSET_BREDR_CONTEXT_H
#define HANDSET_BREDR_CONTEXT_H

/*! \brief Describes the current BREDR context of a handset device. */
typedef enum
{
    /*! Undefined or no context for the handset. */
    handset_bredr_context_none = 0,

    /*! Handset was disconnected, either by the user, the application, or the handset. */
    handset_bredr_context_disconnected,

    /*! Handset was disconnected due to link loss. */
    handset_bredr_context_link_loss,

    /*! Handset was link lost and is currently reconnecting, This means we are currently
        paging, or preparing to page, this handset. */
    handset_bredr_context_link_loss_reconnecting,

    /*! Handset was link lost and is not currently available. This means when we last
        tried to reconnect the link lost handset, paging timed out. */
    handset_bredr_context_link_loss_not_available,

    /*! Handset is currently connecting the ACL, we are paging or preparing to page. */
    handset_bredr_context_connecting,

    /*! Handset has ACL connected and is connecting profiles. */
    handset_bredr_context_profiles_connecting,

    /*! Handset has some profiles connected, but not all the requested profiles. This
        is likely to be due to a 'stop connect' request being made during profile
        connection. */
    handset_bredr_context_profiles_partially_connected,

    /*! Handset has the requested profiles fully connected. */
    handset_bredr_context_profiles_connected,

    /*! Handset has profiles disconnecting. */
    handset_bredr_context_profiles_disconnecting,

    /*! Last regular handset connection attempt failed. Note: this doesn't include link loss
        reconnection attempts. */
    handset_bredr_context_not_available,

    /*! Handset has initiated a barge-in connection. */
    handset_bredr_context_barge_in

} handset_bredr_context_t;

#endif // HANDSET_BREDR_CONTEXT_H
