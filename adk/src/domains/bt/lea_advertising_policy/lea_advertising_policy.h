/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       lea_advertising_policy.h
\defgroup   lea_advertising_policy Lea Advertising Policy (LE)
\ingroup    lea_advertising_policy
\brief      Header file for the LEA Advertising Policy. Interfaces for setting advertising modes.
*/

#ifndef LEA_ADVERTISING_POLICY_H_
#define LEA_ADVERTISING_POLICY_H_

#include "bdaddr.h"
#include "bt_types.h"
#include "device.h"
#include "bt_device.h"

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT
#include "ltv_utilities.h"
#endif

/*\{*/

/*! LE Audio Announcement type */
typedef enum
{
    /*! LEA Announcement type is none, used in reset */
    lea_adv_policy_announcement_type_none = -1,

    /*! LEA Announcement type is General(Connectable but not requesting a connection) */
    lea_adv_policy_announcement_type_general,

    /*! LEA Announcement type is Targeted(Connectable and requesting a connection) */
    lea_adv_policy_announcement_type_targeted,
} lea_adv_policy_announcement_type_t;

/*! LE Audio Advertising Modes */
typedef enum
{
    /* do only undirected advertising */
    lea_adv_policy_mode_undirected = 1 << 0,

    /* do only directed advertising */
    lea_adv_policy_mode_directed = 1 << 1,

    /* do both directed & undirected advertising */
    lea_adv_policy_mode_directed_and_undirected = lea_adv_policy_mode_undirected | lea_adv_policy_mode_directed
} lea_adv_policy_mode_t;

/*! LE Audio Advertising parameters */
typedef struct
{
    /* Announcement type(General/Targeted) to be used with directed & undirected adverts */
    lea_adv_policy_announcement_type_t type;
    /* The sink and source available audio context */
    uint32 audio_context;
} lea_adv_policy_adv_param_t;

/*! Callback structure used when an LEA GATT Server registers with LEA Advertising Policy.

    Note: The GetAdvertisingDataSize and GetAdvertisingData callback functions MUST be supplied when a client registers.
 */
typedef struct
{
    /*! Used to get the advertising data size from the registered client */
    uint8 (*GetAdvertisingDataSize)(void);

    /*! Used to get the advertising data from the registered client 

        \param params                Pointer to LEA advertising params to be used with directed and undirected adverts.
        \param advert_buffer         Registered clients should copy their advertisement data into this buffer.
        \param advert_buffer_length  Length of the advertising buffer.

        \return size_written         Registered clients returns the amount of advert data written into advert_buffer.

        \note The LEA Advertising Policy only stores a pointer to the registered client, so the callback object needs
              to have a lifetime as long as the system.
     */
    uint8 (*GetAdvertisingData)(const lea_adv_policy_adv_param_t *params, uint8 *advert_buffer, uint8 advert_buffer_length);
} lea_adv_policy_clients_callback_t;

/*! @brief Register an LEA Server as a client with LEA Advertising Policy.

    \param callback  Callback funtions to register

    \note The LEA Advertising Policy only stores a pointer to the registered client, so the callback object needs
          to have a lifetime as long as the system.
*/
void LeaAdvertisingPolicy_RegisterClient(const lea_adv_policy_clients_callback_t * const callback);

/*! \brief Sets the advertising interval to either directed or undirected or both.​
           Call this API to update the desired advertising interval to be used for 
           directed or undirected advertisement.

    \param advert_mode   Advertising Mode i.e., directed OR undirected OR both to which this this parameter applies to​
    \param min_interval  Minimum advertising interval for directed advertisements. (N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms))​
    \param max_interval  Maximum advertising interval for directed advertisements. (N = 0x20 to 0xFFFFFF  (Time = N * 0.625 ms))​

    \note  This API will not internally kick the LE Advertising Manager. Higher layer should call LeaAdvertisingPolicy_SetAdvertisingMode 
           API to start LE Advertisement with updated interval.​
*/
void LeaAdvertisingPolicy_SetAdvertisingInterval (lea_adv_policy_mode_t mode, uint32 min_interval, uint32 max_interval);

/*! \brief Sets the advertising mode. A higher layer module calls this interface to set the desired
           advertising mode and configures​ the announcement type, as well as available audio context.
           The LE Advertising policy will preserve the supplied configuration ​and internally kicks the 
           LE Advertising Manager to setup/update the LE Advertising.​

    \param advert_mode          Advertising Mode i.e., directed OR undirected OR both​
    \param dir_announce_type    Announcement type(General/Targeted) to be used for directed adverts​
    \param undir_announce_type  Announcement type(General/Targeted) to be used for undirected adverts​
    \param peer_addr            Peer address used for directed advertisements (Null in case if directed mode is not requested).​

    \return True if parameters are correct and the advertising mode is set properly; FALSE otherwise​

    \note: Pass NULL for peer_addr when doing only undirected adverts. This function does not starts LE
           Advertising instead it​ triggers the LE Advertising Manager to recreate the set with updated
           advertising params and data if any.​
*/
bool LeaAdvertisingPolicy_SetAdvertisingMode(lea_adv_policy_mode_t adv_mode,
                                             lea_adv_policy_announcement_type_t dir_announce_type,
                                             lea_adv_policy_announcement_type_t undir_announce_type,
                                             typed_bdaddr *peer_addr);

#ifdef ENABLE_LEA_TARGETED_ANNOUNCEMENT

/*! \brief Set and Stores the Sink and Source audio context for advertising data. If Sink / Source audio context is 
    passed audio_context_unknown, the default pacs available audio context will be advertised

    \param mode                 Advertising Mode i.e., directed OR undirected OR both to which this this parameter applies to​
    \param sink_audio_context   The Sink available audio context to be advertised
    \param source_audio_context The Source available audio context to be advertised
*/
void LeaAdvertisingPolicy_SetAudioContext(lea_adv_policy_mode_t mode, audio_context_t sink_audio_context, audio_context_t source_audio_context);

#endif

/*! \brief Get the Sink and source available audio context mask which is set to be advertised

    \param mode Can be either directed / undirected.

    \return Sink and Source available audio context mask for the mode. If mode is not valid return audio_context_type_unknown
 */
uint32 LeaAdvertisingPolicy_GetAudioContext(lea_adv_policy_mode_t mode);

/*! \brief Trigger an update of the LE advertising data for registered clients.

    \returns TRUE if the update was successful, else FALSE
 */
bool LeaAdvertisingPolicy_UpdateAdvertisingItems(void);

/*! \brief Application should call this API to initialize LE Advertising Policy during startup( Init table)
    LE Advertising policy will register with LE Advertising Manager.
    Sets up the default Advert Intervals & announcement types.

    \param apptask  Unused Null should be passed.

    \return TRUE if initialization succeeds; FALSE otherwise.
*/
bool LeaAdvertisingPolicy_Init(Task init_task);

/*\}*/

/*! \defgroup   lea_advertising_policy Lea Advertising Policy (LE)

    LE Audio GATT Services such as BAP, BASS, VCS, CAS, TMAS registers themselves
    with LE Advertising Manager. LE Advertising Manger queries each of these
    components for Advertisement Parameters & Data and constructs Groups and
    then accordingly registers them in the form of Sets and enables them
    Whenever there is a change in Advertising required, 
    the LE Components must inform LE Advertising Manager to re-query their data,
    rebuild sets and register them. 

    Rather than registering each profile with LEAM, this module named LEA Advert Policy
    will register with LE Advertising Manager.

    The LEA Advertising policy will hide all the LEA profiles from LE Advertising Manager and
    helps to collect advert data, consolidate them and provide it to LE Advert Manager.

    The LEA Advertising policy is initialized by a call to LeaAdvertisingPolicy_Init() at the time
    of startup of the application. Post this the gatt servers such BAP, BASS, VCS, CAS, TMAS shall
    then registers with the LEA advertising policy through a call to LeaAdvertisingPolicy_RegisterClient()
    with their set of call backs as lea_adv_policy_clients_callback_t.

    The application or Handset service then can set the modes at appropriate time as lea_adv_policy_mode_t
    and announcement as lea_adv_policy_announcement_type_t through a call to LeaAdvertisingPolicy_SetAdvertisingMode().

    The LEA advertising policy then informs LE Advertising manager to update the advertising
    with the mode which was set earlier through LeaAdvertisingPolicy_SetAdvertisingMode().

*/

#endif /* LEA_ADVERTISING_POLICY_H_ */
