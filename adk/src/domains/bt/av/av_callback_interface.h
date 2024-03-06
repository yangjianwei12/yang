/*!
\copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\addtogroup av_state_machines
@{
\brief      AV callback interface

    This callback interface should be used to define different behaviours
    for different types of av instances.
*/

#ifndef AV_CALLBACK_INTERFACE_H
#define AV_CALLBACK_INTERFACE_H
#include <a2dp.h>
#include <message.h>
#include "audio_sources_list.h"
#include <avrcp.h>
#ifdef USE_SYNERGY
#include <avrcp_lib.h>
#endif /* USE_SYNERGY */

typedef struct
{
    /*! \brief Initialise the AV for instance type. Any specific required initialisation steps
        should be put here.

    */
    void (*Initialise) (void);

    /*! \brief Start the A2dp library calling A2dpInit using the appropriate list of
        seids and A2dp role required for the specific instance.

        \param clientTask The task passed to the A2dp library that will receive
                          A2DP library messages
    */
    void (*InitialiseA2dp) (Task clientTask);

    /*! \brief Retrieve a list of seids that should be used for the A2dpMediaOpen call
     * \return The length of the retrieved seid list
     * \param seid_list_out pointer to the seid list array that will returned.
    */
    uint16 (*GetMediaChannelSeids) (const uint8** seid_list_out);

    /*! \brief Called on receipt of a AVRCP passthrough message with play opid
     * \return True if call successful
     * \param pressed indicates if the button is pressed or released
    */
    bool (*OnAvrcpPlay) (void *av_instance, bool pressed);

    /*! \brief Called on receipt of a AVRCP passthrough message with pause opid
     * \return True if call successful
     * \param pressed indicates if the button is pressed or released
    */
    bool (*OnAvrcpPause) (void *av_instance, bool pressed);

    /*! \brief Called on receipt of a AVRCP passthrough message with Forward opid
     * \return True if call successful
     * \param pressed indicates if the button is pressed or released
    */
    bool (*OnAvrcpForward) (void *av_instance, bool pressed);

    /*! \brief Called on receipt of a AVRCP passthrough message with Backward opid
     * \return True if call successful
     * \param pressed indicates if the button is pressed or released
    */
    bool (*OnAvrcpBackward) (void *av_instance, bool pressed);

    /*! \brief Returns the AVRCP events to be registered
     * \return uint16 with each bit representing an event
    */
    uint16 (*GetAvrcpEvents) (void);

    /*! \brief Returns the AVRCP config parameters
     * \return avrcp_init_params a pointer to the AVRCP init configuration
    */
    const avrcp_init_params * (*GetAvrcpConfig) (void);
#ifdef USE_SYNERGY
    /*! \brief Initialises A2DP data block
     * \return none
     * \param av_inst pointer to av instance pointer to be initialised
    */
    void (*InitialiseA2dpDataBlock) (void *av_inst);

    /*! \brief Retrieve A2DP local role
     * \return CSR_BT_AV_SINK / CSR_BT_AV_SOURCE
     * \param none
    */
    uint8 (*GetA2dpLocalRole) (void);
    /*! \brief Configures the AVRCP Role
     * \return none
     * \param ctFeatures Pointer to the CT Features to configure
     * \param tgFeatures Pointer to the TG Features to configure
    */
    void (*AvrcpConfigureRole) (CsrBtAvrcpRoleDetails* ctFeatures, CsrBtAvrcpRoleDetails* tgFeatures);
    /*! \brief Register Media Player
     * \return none
     * \param  none
    */
    void (*AvrcpRegisterMediaPlayer) (void);

    
#endif /* USE_SYNERGY */

} av_callback_interface_t;

#endif /* AV_CALLBACK_INTERFACE_H */
/**! @} !*/