/****************************************************************************
Copyright (c) 2004 - 2021 Qualcomm Technologies International, Ltd.


FILE NAME
    avrcp.h
    
DESCRIPTION 

    Header file for the Audio/Video Remote Control Profile library.  This
    profile library implements the AVRCP using the services of the AVCTP
    library which is hidden from the Client application by this library
    
    The library exposes a functional downstream API and an upstream message 
    based API.
    
     CLIENT APPLICATION
      |         |
      |    AVRCP Library
      |         |
      |      |
     CONNECTION Library
             |
         BLUESTACK
    
*/
/*!

\defgroup avrcp avrcp
\ingroup vm_libs

\brief    Interface to the Audio Video Remote Control Profile library.

\section avrcp_intro INTRODUCTION
        This profile library implements the AVRCP.  This library permits one
        device known as the controller (CT) to send dedicated user actions to
        another device known as the target (TG).
        
        Note: This library does not handle audio streaming, this is implemented
        in the GAVDP library.
    
        The library exposes a functional downstream API and an upstream message
        based API.
*/

/** @{ */

#ifndef AVRCP_H_
#define AVRCP_H_


#include <bdaddr_.h>
#include <library.h>
#include <message.h>

#include "handover_if.h"


struct __AVRCP;
/*!
    @brief The Audio Video Remote Control Profile structure.
*/
typedef struct __AVRCP AVRCP;


/*!
    @brief The page data length.
*/
#define PAGE_DATA_LENGTH    (4)

/*!  AVRCP Supported Features Flag Defines

    These flags can be or'd together and used as the supported_features field
    of an avrcp_init_params structure. 
*/

/*!
    @brief Setting this flag when the library is initialised indicates that 
    this device implements category 1 commands.
*/
#define AVRCP_CATEGORY_1                    0x01
/*!
    @brief Setting this flag when the library is initialised indicates that
     this device implements category 2 commands.
*/
#define AVRCP_CATEGORY_2                    0x02
/*!
    @brief Setting this flag when the library is initialised indicates that
     this device implements category 3 commands.
*/
#define AVRCP_CATEGORY_3                    0x04
/*!
    @brief Setting this flag when the library is initialised indicates that
     this device implements category 4 commands.
*/
#define AVRCP_CATEGORY_4                    0x08
/*!
    @brief Setting this flag when the library is initialised indicates that
     this device implements player application settings. Player application
     settings are valid only for Category 1 Target devices. 
     AVRCP_VERSION_1_3 flag shall be enabled in the profile_extensions
     to turn ON this flag. 
*/
#define AVRCP_PLAYER_APPLICATION_SETTINGS    (0x10 |  AVRCP_CATEGORY_1)
/*!
    @brief Setting this flag when the library is initialised indicates that 
    this device implements Group Navigation. Group Navigation is valid only
    for  Category 1 Target devices.
*/
#define AVRCP_GROUP_NAVIGATION                (0x20 | AVRCP_CATEGORY_1)
/*!
     @brief Setting this flag when the library is initialised indicates that
     this device implements Virtual File system browsing. Library ignores this
     bit if AVRCP_BROWSING_SUPPORTED bit is not set in the profile_extensions 
     field of the avrcp_init_params structure while initialising the library.
*/
#define AVRCP_VIRTUAL_FILE_SYSTEM_BROWSING     0x40
/*!
    @brief Setting this flag when the library is initialised indicates that
     this Target device implements Multiple Media Player applications.
     It is valid  only for Category 1 Target devices. Library ignores this
     bit if AVRCP_BROWSING_SUPPORTED bit is not set in the profile_extensions.
*/
#define AVRCP_MULTIPLE_MEDIA_PLAYERS           0x80



/*! This flag enables the Deprecated part of APIs and Code */
/* #define AVRCP_ENABLE_DEPRECATED 1 */

/*!  AVRCP Extensions Flag Defines

    These flags can be or'd together and used as the profile_extensions field
    of an avrcp_init_params structure.
*/

/*!
    @brief Setting this flag when the library is initialised indicates that
    this device implements the AVRCP version 1.3 . If this bit is not set
    in profile_extensions field for the target device, AVRCP Lib will act as 
    v1.0 Library.
*/
#define AVRCP_VERSION_1_3                   0x01

#ifdef AVRCP_ENABLE_DEPRECATED
/*! This flag is deprecated */
#define AVRCP_EXTENSION_METADATA            AVRCP_VERSION_1_3
#endif /* AVRCP_ENABLE_DEPRECATED */


/*!
    @brief This bit shall be set if the library supports version 1.4 AVRCP
*/
#define AVRCP_VERSION_1_4                  (0x02 | AVRCP_VERSION_1_3)

/*!
   @brief Setting this flag when the library is initialised indicates that 
   this device implements the AVRCP 1.4 browsing channel support. If this bit
   is not set in profile_extensions field for the target device, AVRCP library
   will be initialised as version 1.3 for Cat 1 or 3 devices.     
*/
#define AVRCP_BROWSING_SUPPORTED          (0x04  | AVRCP_VERSION_1_4 )
                                              

/*!
   @brief Setting this flag in profile_extensions field when the library is 
   initialised indicates that this device implements the AVRCP 1.4 
   Search feature. 
*/
#define AVRCP_SEARCH_SUPPORTED            (0x08 | AVRCP_BROWSING_SUPPORTED)

/*!
   @brief Setting this flag in profile_extensions field when the library is 
   initialised indicates that this Target device supports Database Aware 
   Media Players.
*/
#define AVRCP_DATABASE_AWARE_PLAYER_SUPPORTED (0x10 | AVRCP_BROWSING_SUPPORTED)

/*!
    @brief This bit shall be set if the library supports version 1.5 AVRCP
*/
#define AVRCP_VERSION_1_5                  (0x20 | AVRCP_VERSION_1_4)

/*!
    @brief This bit shall be set if the library supports version 1.6 AVRCP
*/
#define AVRCP_VERSION_1_6                  (0x40 | AVRCP_VERSION_1_5)



/*!
   @brief Setting this flag in profile_extensions field while initialising 
   the library will turn on all AVRCP 1.4 features.
*/
#define AVRCP_14_ALL_FEATURES_SUPPORTED     0x1F




/*!  List of Media Attributes definitions.
    Application must use defined values for Media Attributes while framing the 
    attributes Data for AvrcpGetElementAttributesRequest() and 
    AvrcpGetElementAttributesResponse() APIs.
*/

/*!    @brief Title of the media.*/
#define AVRCP_MEDIA_ATTRIBUTE_TITLE     0x01

/*!    @brief Name of the artist.*/
#define AVRCP_MEDIA_ATTRIBUTE_ARTIST    0x02

/*!    @brief Name of the album*/
#define AVRCP_MEDIA_ATTRIBUTE_ALBUM     0x03

/*! @brief Number of the media(e.g. Track number of the CD) */
#define AVRCP_MEDIA_ATTRIBUTE_NUMBER    0x04

/*! @brief Total number of the media (e.g.Total track number of the CD)*/
#define AVRCP_MEDIA_ATTRIBUTE_TOTAL_NUMBER  0x05

/*! @brief Genre*/
#define AVRCP_MEDIA_ATTRIBUTE_GENRE     0x06

/*! @brief Playing Time in milliseconds*/
#define AVRCP_MEDIA_ATTRIBUTE_PLAYING_TIME 0x07


/*!   List of defined Player Application Settings Attributes and Values. 
    Application should use defined values for Player Application Settings
    Attributes and values while framing the attributes and values for the APIs.

    AvrcpListAppAttributeResponse(),
    AvrcpListAppValueRequest(),
    AvrcpListAppValueResponse(),
    AvrcpGetCurrentAppValueRequest(),
    AvrcpGetCurrentAppValueResponse(),
    AvrcpSetAppValueRequest(),
    AvrcpGetAppAttributeTextRequest(),
    AvrcpGetAppAttributeTextResponse() and
    AvrcpGetAppValueTextRequest().
*/


/*!  @brief Equalizer ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_EQUALIZER      0x01

/*!  @brief Repeat Mode status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_REPEAT_MODE    0x02

/*!  @brief Shuffle ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_SHUFFLE        0x03

/*!  @brief Scan ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_SCAN           0x04

/*!  @brief OFF status value all Equalizer ON/OFF Attribute ID.*/
#define AVRCP_PLAYER_VALUE_EQUALIZER_OFF      0x01

/*!  @brief ON status value all Equalizer ON/OFF Attribute ID.*/
#define AVRCP_PLAYER_VALUE_EQUALIZER_ON       0x02

/*!  @brief OFF value for Repeat Mode status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_REPEAT_MODE_OFF    0x01

/*!  @brief Single Track Repeat value for Repeat Mode status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_REPEAT_MODE_SINGLE  0x02

/*!  @brief All Track Repeat value for Repeat Mode status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_REPEAT_MODE_ALL     0x03

/*!  @brief Group Track Repeat value for Repeat Mode status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_REPEAT_MODE_GROUP   0x04

/*!  @brief OFF value for Shuffle ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_SHUFFLE_OFF         0x01   

/*!  @brief All Track Shuffle value for Shuffle ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_SHUFFLE_ALL         0x02   

/*!  @brief Group Track Shuffle value for Shuffle ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_VALUE_SHUFFLE_GROUP       0x03   

/*!  @brief OFF value for Scan ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_SCAN_OFF        0x01

/*!  @brief All Track value for Scan ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_SCAN_ALL        0x02

/*!  @brief Group Track value for Scan ON/OFF status Attribute ID.*/
#define AVRCP_PLAYER_ATTRIBUTE_SCAN_GROUP      0x03



/*!  List of AVRCP PDU IDs defined in AVRCP 1.4 specification.
    Application should use these values for requesting or aborting 
    the Continuation response if the last metadata response from 
    the Target device was a fragmented response.
*/  



/*!  @brief PDU ID of GetCapabilities Command */
#define AVRCP_GET_CAPS_PDU_ID                       0x10

/*!  @brief ListPlayerApplicationSettingAttributes command PDU ID*/
#define AVRCP_LIST_APP_ATTRIBUTES_PDU_ID            0x11

/*!  @brief ListPlayerApplicationSettingValues command PDU ID */
#define AVRCP_LIST_APP_VALUE_PDU_ID                 0x12

/*!  @brief GetCurrentPlayerApplicationSettingValue command PDU ID*/
#define AVRCP_GET_APP_VALUE_PDU_ID                  0x13

/*!  @brief SetPlayerApplicationSettingValue command PDU ID*/
#define AVRCP_SET_APP_VALUE_PDU_ID                  0x14

/*!  @brief GetPlayerApplicationSettingAttributeText command PDU ID*/
#define AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID         0x15

/*!  @brief GetPlayerApplicationSettingValueText command PDU ID*/
#define AVRCP_GET_APP_VALUE_TEXT_PDU_ID             0x16

/*!  @brief InformDisplayableCharacterSet command PDU ID*/
#define AVRCP_INFORM_CHARACTER_SET_PDU_ID           0x17

/*!  @brief GetElementAttributes command PDU ID*/
#define AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID         0x20



/*!   AVRCP Response and Status values.
    Target application must use the defined avrcp_response_type values while 
    using the Response APIs. AVRCP library uses the avrcp_status_code in the
    confirmation message to the application on completion of requested
    operation.
*/


/*! 
    @brief AVRCP responses. Values are defined in AV/C Digital Interface spec.
    Additional library defined responses have been added.
*/
typedef enum
{
    /*! The specified profile is not acceptable. */
    avctp_response_bad_profile = 0x01,
    /*! The request is not implemented. */
    avctp_response_not_implemented = 0x08,    
    /*! The request has been accepted. This response should be used when
        accepting commands    with AV/C command type of CONTROL. */
    avctp_response_accepted = 0x09,            
    /*! The request has been rejected. */
    avctp_response_rejected = 0x0A,            
    /*! The target is in a state of transition. */
    avctp_response_in_transition = 0x0B,    
    /*! A stable response.This response should be used when accepting commands
    with AV/C command type of STATUS. */
    avctp_response_stable = 0x0C,            
    /*! The target devices state has changed. This response should be used 
        when accepting commands    with AV/C command type of NOTIFY. */
    avctp_response_changed = 0x0D,            
    /*! The response is an interim response. This response should be used when
        accepting commands    with AV/C command type of NOTIFY. */
    avctp_response_interim = 0x0F, 
           
    /*! More specific error status responses for rejecting the Meta Data AVC
        commands and Browsing commands are as follows.Error status codes
        defined in the AVRCP1.4 specification can be retrieved by masking msb
        (0x80) of the defined response codes here. Ensure to keep the same
        values while inserting or modifying following enum values.  
    */

       /*! The request has been rejected if TG received a PDU that it did not 
        understand. This is valid for all Command Responses*/
    avrcp_response_rejected_invalid_pdu = 0x80,

    /*! The request has been rejected with reason - invalid parameter.
        If the TG received a PDU with a parameter ID that it did not 
        understand.
        Send if there is only one parameter ID in the PDU. Valid for all
        Commands.
    */
    avrcp_response_rejected_invalid_param, /* 0x80 + 0x01 */

    /*! The request has been rejected with reason - invalid content.
       Send if the parameter ID is understood, but content is wrong or 
       corrupted.
       Valid for all commands.*/
    avrcp_response_rejected_invalid_content, /* 0x80 + 0x2 */

    /*! The request has been rejected with reason - internal error.
        Valid for all commands*/
    avrcp_response_rejected_internal_error, /* 0x80 + 0x03*/

    /*! The request has been successful. This response shall be used
        with Browsing commands only */
    avrcp_response_browsing_success, /* 0x80 + 0x04 */

    /*! The request has been rejected with reason - UID Changed. 
        The UIDs on the device have changed */
    avrcp_response_rejected_uid_changed = 0x85, /* 0x80 + 0x05 */

    /*!  The request has been rejected with reason -  Invalid Direction. 
         The Direction parameter is invalid. Response code is valid only for 
         ChangePath command */
    avrcp_response_rejected_invalid_direction = 0x87, /* 0x80 + 0x07 */

    /*! The request has been rejected with reason - Not a Directory. 
        The UID provided does not refer to a folder item. Response code is
        valid only for ChangePath command */
    avrcp_response_rejected_not_directory, /*0x80 + 0x08 */

    /*! The request has been rejected with reason - Does not exist.
        The UID provided does not refer to any currently valid Item. 
        This response code is valid for commands - Change Path, PlayItem, 
        AddToNowPlaying, GetItemAttributes */
    avrcp_response_rejected_uid_not_exist, /*0x80 + 0x09 */

    /*! The request has been rejected with reason - Invalid Scope.
        The scope parameter is invalid. This response code is valid for 
        commands
        - GetFolderItems, PlayItem, AddToNowPlayer, GetItemAttributes.
    */
    avrcp_response_rejected_invalid_scope, /*0x80 + 0x0A */

    /*! The request has been rejected with reason - Range Out of Bounds 
        The start of range provided is not valid. This response is valid for 
        GetFolderItems command.*/
    avrcp_response_rejected_out_of_bound, /* 0x80 + 0x0B */

    /*! The request has been rejected with reason - UID is a Directory.
        The UID provided refers to a directory, which cannot be handled by
        this media player. This response is valid for commands - PlayItem and
        AddToNowPlaying
    */
    avrcp_response_rejected_uid_directory, /*0x80 + 0x0C */

    /*! The request has been rejected with reason - Media in Use.
        The media is not able to be used for this operation at this time.
        This response is valid for commands - PlayItem and AddToNowPlaying.
     */
    avrcp_response_rejected_media_in_use, /*0x80 + 0x0D */

    /*! The request has been rejected with reason - Now Playing List Full.
        No more items can be added to the Now Playing List.
        This response is valid for command - AddToNowPlaying
    */
    avrcp_response_rejected_play_list_full, /*0x80 + 0x0E */

    /*! This request has been rejected with reason - Search Not Supported.
        The Browsed Media Player does not support search.
        This response is valid for command - Search.
    */
    avrcp_response_rejected_search_not_supported, /*0x80 + 0x0F*/

    /*! This request has been rejected with reason - Search in Progress
        A search operation is already in progress.
        This response is valid for command - Search
    */
    avrcp_response_rejected_search_in_progress, /*0x80 + 0x10*/

    /*! This request has been rejected with reason - Invalid Player Id
        The specified Player Id does not refer to a valid player.
        This response is valid for commands - SetAddressedPlayer and
        SetBrowsedPlayer
    */
    avrcp_response_rejected_invalid_player_id, /*0x80 + 0x11*/

    /*! This request has been rejected with reason - Player Not Browsable
        The Player Id supplied refers to a Media Player which does not 
        support browsing. This response is valid for commands - 
        SetBrowsedPlayer
    */
    avrcp_response_rejected_player_not_browsable, /*0x80 + 0x12*/

    /*! This request has been rejected with reason - Player Not Addressed.
        The Player Id supplied refers to a player which is not currently
        addressed, and the command is not able to be performed if the player
        is not set as addressed. This response is valid for commands - 
        Search and SetBrowsedPlayer.
    */
    avrcp_response_rejected_player_not_addressed, /*0x80 + 0x13*/

    /*! This request has been rejected with reason - No valid Search Results.
        The Search result list does not contain valid entries, e.g. after 
        being invalidated due to change of browsed player - This response is
        valid for GetFolderItems
    */
    avrcp_response_rejected_no_valid_search_results, /*0x80 + 0x14*/

    /*! This request has been rejected with reason - No available players */
    avrcp_response_rejected_no_available_players, /*0x80 + 0x15*/

    /*! This request has been rejected with reason - Addressed Player Changed.
        This is valid for command - Register Notifications*/
    avrcp_response_rejected_addressed_player_changed, /*0x80 + 0x16 */ 

    /* Dummy Place Holder */
    avrcp_response_guard_reserved = 0xFF

} avrcp_response_type;

/*!
    @brief AVRCP status codes 
*/
typedef enum
{
    /*! Operation was successful. */
    avrcp_success = (0),            
    /*! Operation failed. */
    avrcp_fail,                        
    /*! Not enough resources. */
    avrcp_no_resource,                
    /*! Request is not supported in the current state. */
    avrcp_bad_state,                
    /*! Operation timed out before completion. */
    avrcp_timeout,                    
    /*! Device specified is not connected. */
    avrcp_device_not_connected,        
    /*! Operation is already in progress. */
    avrcp_busy,                        
    /*! Requested operation is not supported. */
    avrcp_unsupported,                
    /*! Sink supplied was invalid. */
    avrcp_invalid_sink,
    /*! Link loss occurred. */
    avrcp_link_loss,
    /*! The operation was rejected. */
    avrcp_rejected=0x0A,
    /*! General failure during AVRCP Browsing channel initialization*/
    avrcp_browsing_fail,
    /*! Browsing channel is not connected */
    avrcp_browsing_channel_not_connected,
    /*! Remote device does not support Browsing */
    avrcp_remote_browsing_not_supported,
    /*! Timeout on browsing channel */
    avrcp_browsing_timedout,
    /*! Link has been transferred to TWS Peer */
    avrcp_link_transferred,
    /*! Operation was successful, but has only received an
      interim response.*/
    avrcp_interim_success=0x10,

    /* Below status codes depends on the error status code received from the 
       remote device. Retain the same values while inserting new values or
       modifying this enum */

    /*! The operation was rejected with reason - invalid PDU. */
    avrcp_rejected_invalid_pdu = 0x80,
    /*! The operation was rejected with reason - invalid parameter. */
    avrcp_rejected_invalid_param,
    /*! The operation was rejected with reason - invalid content. */
    avrcp_rejected_invalid_content,
    /*! The operation was rejected with reason - internal error. */
    avrcp_rejected_internal_error,

    /*! The operation was rejected with reason - UID Changed. */
    avrcp_rejected_uid_changed = 0x85, 
    /*! The command has been rejected with reason -Invalid Direction.*/
    avrcp_rejected_invalid_direction = 0x87, 
    /*! The command has been rejected with reason -Not a Directory.*/
    avrcp_rejected_not_directory, 
    /*! The command has been rejected with reason -Does not exist.*/
    avrcp_rejected_uid_not_exist, 
    /*! The command has been rejected with reason -Invalid Scope.*/
    avrcp_rejected_invalid_scope, 
    /*! The command has been rejected with reason - Range Out of Bounds.*/
    avrcp_rejected_out_of_bound, 
    /*! The command has been rejected with reason - UID is a Directory.*/
    avrcp_rejected_uid_directory, 
    /*! The command has been rejected with reason - Media in Use.*/
    avrcp_rejected_media_in_use, 
    /*! The command has been rejected with reason - Now Playing List Full.*/
    avrcp_rejected_play_list_full, 
    /*! The command has been rejected with reason - Search Not Supported.*/
    avrcp_rejected_search_not_supported, 
    /*! The command has been rejected with reason - Search in Progress.*/
    avrcp_rejected_search_in_progress, 
    /*! This command has been rejected with reason - Invalid Player ID.*/
    avrcp_rejected_invalid_player_id, 
    /*! This command has been rejected with reason - Player Not Browsable.*/
    avrcp_rejected_player_not_browsable,
    /*! This command has been rejected with reason - Player Not Addressed.*/
    avrcp_rejected_player_not_addressed, 
    /*! This command has been rejected with reason - No valid Search Results.*/
    avrcp_rejected_no_valid_search_results, 
    /*! This command has been rejected with reason - No available players.*/
    avrcp_rejected_no_available_players, 
    /*! This command has been rejected with reason -Addressed Player Changed.*/
    avrcp_rejected_addressed_player_changed,

    /* Dummy Place Holder */
    avrcp_status_guard_reserverd = 0xFF
} avrcp_status_code;



/*! 
    @brief Operation ID, used to identify operation. See table 9.21 AV/C Panel
    Subunit spec. 1.1 #
*/
typedef enum
{
    opid_select                = (0x0),
    opid_up,
    opid_down,
    opid_left,
    opid_right,
    opid_right_up,
    opid_right_down,
    opid_left_up,
    opid_left_down,
    opid_root_menu,
    opid_setup_menu,
    opid_contents_menu,
    opid_favourite_menu,
    opid_exit,
    /* 0x0E to 0x1F Reserved */
    opid_0                    = (0x20),
    opid_1,
    opid_2,
    opid_3,
    opid_4,
    opid_5,
    opid_6,
    opid_7,
    opid_8,
    opid_9,
    opid_dot,
    opid_enter,
    opid_clear,
    /* 0x2D - 0x2F Reserved */
    opid_channel_up            = (0x30),
    opid_channel_down,
    opid_sound_select,
    opid_input_select,
    opid_display_information,
    opid_help,
    opid_page_up,
    opid_page_down,
    /* 0x39 - 0x3F Reserved */
    opid_power                = (0x40),
    opid_volume_up,
    opid_volume_down,
    opid_mute,
    opid_play,
    opid_stop,
    opid_pause,
    opid_record,
    opid_rewind,
    opid_fast_forward,
    opid_eject,
    opid_forward,
    opid_backward,
    /* 0x4D - 0x4F Reserved */
    opid_angle                = (0x50),
    opid_subpicture,
    /* 0x52 - 0x70 Reserved */
    opid_f1                    = (0x71),
    opid_f2,
    opid_f3,
    opid_f4,
    opid_f5,
    opid_vendor_unique        = (0x7E)
    /* Ox7F Reserved */
} avc_operation_id; 


/*!
    @brief Subunit types 
*/
typedef enum
{
    subunit_monitor            = (0x0),
    subunit_audio,
    subunit_printer,
    subunit_disc,
    subunit_tape_recorder_player,
    subunit_tuner,
    subunit_CA,
    subunit_camera,
    subunit_reserved,
    subunit_panel,
    subunit_bulletin_board,
    subunit_camera_storage,
    /* 0x0C - 0x1B Reserved */
    subunit_vendor_unique    = (0x1C),
    subunit_reserved_for_all,
    subunit_extended,
    subunit_unit
} avc_subunit_type;

/*!
    @brief AVRCP device type

    The AVRCP library can be configured to be either a target or a controller
    device.
*/
typedef enum
{
    avrcp_target = 0x01,
    avrcp_controller,
    avrcp_target_and_controller
} avrcp_device_type;


/*!
    @brief AVRCP Metadata transfer capability ID.

    The capability ID type for capabilities supported by the target. 
*/
typedef enum
{
    avrcp_capability_company_id = 0x02,
    avrcp_capability_event_supported = 0x03
} avrcp_capability_id;


/*!
    @brief AVRCP Metadata transfer event IDs.

    The specification mandates the TG to support a number of event IDs.
    Optionally it may also support a number of other events not defined 
    by the specification. This type covers all events defined by the 
    Metadata transfer specification.
*/
typedef enum 
{
    avrcp_event_playback_status_changed = 0x01,
    avrcp_event_track_changed,                      /* 0x02 */
    avrcp_event_track_reached_end,                  /* 0x03 */
    avrcp_event_track_reached_start,                /* 0x04 */
    avrcp_event_playback_pos_changed,               /* 0x05 */
    avrcp_event_batt_status_changed,                /* 0x06 */
    avrcp_event_system_status_changed,              /* 0x07 */
    avrcp_event_player_app_setting_changed,         /* 0x08 */
    avrcp_event_now_playing_content_changed,        /* 0x09 */
    avrcp_event_available_players_changed,          /* 0x0A */
    avrcp_event_addressed_player_changed,           /* 0x0B */
    avrcp_event_uids_changed,                       /* 0x0C */
    avrcp_event_volume_changed                      /* 0x0D */
} avrcp_supported_events;


/*!
    @brief AVRCP play status events.

    Possible values of play status.
*/
typedef enum 
{
    avrcp_play_status_stopped = 0x00,
    avrcp_play_status_playing = 0x01,
    avrcp_play_status_paused = 0x02,
    avrcp_play_status_fwd_seek = 0x03,
    avrcp_play_status_rev_seek = 0x04,
    avrcp_play_status_error = 0xFF
} avrcp_play_status;


/*!
    @brief AVRCP battery status events.

    Possible values of battery status.
*/
typedef enum 
{
    avrcp_battery_status_normal = 0x00,
    avrcp_battery_status_warning,
    avrcp_battery_status_critical,
    avrcp_battery_status_external,
    avrcp_battery_status_full_charge
} avrcp_battery_status;


/*!
    @brief AVRCP system status events.

    Possible values of system status.
*/
typedef enum 
{
    avrcp_system_status_power_on = 0x00,
    avrcp_system_status_power_off,
    avrcp_system_status_unplugged
} avrcp_system_status;


/*!
    @brief AVRCP character sets.

    character set values defined in IANA character set document available at 
    http://www.iana.org/assignments/character-sets
*/
typedef enum
{
    avrcp_char_set_ascii = 3,
    avrcp_char_set_iso_8859_1 = 4,
    avrcp_char_set_jis_x0201 = 15,
    avrcp_char_set_shift_jis = 17,
    avrcp_char_set_ks_c_5601_1987 = 36,
    avrcp_char_set_utf_8 = 106,
    avrcp_char_set_ucs2 = 1000,
    avrcp_char_set_utf_16be = 1013,
    avrcp_char_set_gb2312 = 2025,
    avrcp_char_set_big5 = 2026
} avrcp_char_set;

/*!
    @brief AVRCP initialisation parameters

    The initialisation parameters allow the profile instance to be configured
    either as a controller or target. 
*/

typedef struct
{
    /*! Specifies if this device is controller (CT), target (TG), or both. 
        This filed will be ineffective if the library is enabled with 
        only Controller or Target role support.*/ 
    avrcp_device_type device_type;

    /*! The supported controller features must be filled in if the device 
        supports the controller (CT) role or the library will default to a 
        possibly undesired default setting. The features for the CT must state
        which categories are supported (1 - 4). See the AVRCP Supported Feature
        Flag Defines at the top of avrcp.h. This value reflects the Supported
        features attribute value in the SDP service record of controller. 
        This field will be ignored if the library is built with only the 
        Target support.*/
    uint8 supported_controller_features;

    /*! The supported target features must be filled in if the device supports
        the target(TG) role or the library will default to a possibly undesired
        default setting.The features of the TG must state which categories are 
        supported (1 - 4),and can also indicate support for the Player 
        Application settings and Group Navigation. 
        See the AVRCP Supported Features Flag Defines at the top of avrcp.h. 
        This field will be ignored if the library is built with only the 
        Controller support.*/
    uint8 supported_target_features;

    /*! Set to zero if no extensions are supported in the Target application. 
        If this bit is not set, library acts as v1.0 . If
        supported_target_features sets the PlayerApplicationSettings or Group 
        Navigation bits, this value must be set.
        If extensions are supported (eg. AVRCP Metadata extensions), 
        use the AVRCP Extensions Flag Defines from the top of avrcp.h. 
     */
    uint8 profile_extensions;

} avrcp_init_params;

/* Browsing Data Structures */
/*!
    @brief Direction for browsing navigation.
*/
typedef enum 
{
    avrcp_direction_up   = 0x00,
    avrcp_direction_down = 0x01

}avrcp_browse_direction;

/*!
    @brief The four scopes in which media content navigation may take place.
*/
typedef enum{
    avrcp_media_player_scope = 0,
    avrcp_virtual_filesystem_scope,
    avrcp_search_scope,
    avrcp_now_playing_scope 
}avrcp_browse_scope;

/*!
    @brief The 8 octets UID to identify the media elements.
*/
typedef struct{
    uint32      msb;
    uint32      lsb;
}avrcp_browse_uid;

/*!
    @brief AV/C protocol - Used to form the targets address
*/
typedef uint16 avc_subunit_id;

#endif /* AVRCP_H_ */

/** @} */
