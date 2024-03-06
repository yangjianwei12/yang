/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup le_unicast_manager
    \brief      Data for a GATT connection being tracked by the le unicast manager
    @{
*/
#ifndef LE_UNICAST_MANAGER_INSTANCE_H
#define LE_UNICAST_MANAGER_INSTANCE_H

#include "le_unicast_manager_private.h"

void LeUnicastManager_InstanceInit(void);

/*! \brief Reset Unicast manager instance. */
void LeUnicastManager_InstanceReset(le_um_instance_t *inst);

/*! \brief Get the default unicast manager instance.

    Get the 'default' unicast instance that code before the LE Audio multipoint
    changes expected.

    \note This is here for backwards compatibility only. Eventually all the
          le_unicast_manager code will be refactored to get the instance by
          an identifier (cid, voice_source, etc.). When that happens this
          function will be removed.

    \return The default unicast manager instance.
*/
le_um_instance_t *LeUnicastManager_GetInstance(void);

/*! \brief Get the unicast manager instance based on the GATT connection Id.

    \param cid GATT connection id to get the unicast manager insrance for.

    \return Pointer to the a unicast manager instance or NULL if there was no match.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByCid(gatt_cid_t cid);

/*! \brief Get the unicast manager instance based on the voice source.

    \param source Voice source to get the unicast manager instance for.

    \return Pointer to the a unicast manager instance or NULL if there was no match.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByVoiceSource(voice_source_t source);

/*! \brief Get the unicast manager instance based on the audio source.

    \param source Audio source to get the unicast manager instance for.

    \return Pointer to the a unicast manager instance or NULL if there was no match.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByAudioSource(audio_source_t source);

/*! \brief Get the unicast manager instance based on the CIS handle.

    \param cis_handle CIS handle to get the unicast manager instance for.

    \return Pointer to the a unicast manager instance or NULL if there was no match.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByCisHandle(uint16 cis_handle);

/*! \brief Get the unicast manager instance based on the CIS id in the ASE QoS config data.

    Note: this is used when matching a newly created CIS with an ASE and instance.

    \param cis_id CIS id to get the unicast manager instance for.

    \return Pointer to the a unicast manager instance or NULL if there was no match.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByAseCisId(uint8 cis_id);

/*! \brief Get or create a new instance for a GATT connection.

    This function will first try to find an existing instance based on the GATT
    connection Id. If none is found then it will attempt to create a new
    instance.

    \param cid GATT connection Id for the new instance.

    \return A pointer to the new instance; NULL if unable to create one.
*/
le_um_instance_t *LeUnicastManager_InstanceGetByCidOrCreate(gatt_cid_t cid);

/*! Get Cis Link loss message for the instance 

    \param inst instance for which the Cis Linkloss Message is requested

    \return Cis link loss Message for instance
*/
le_um_internal_msg_t LeUnicastManager_GetCisLinklossMessageForInst(le_um_instance_t *inst);

/*! Checks if a instance is valid. Currently that means it has a valid GATT connection_id. */
#define LeUnicastManager_InstanceIsValid(inst)      (INVALID_CID != (inst)->cid)

/*! Returns the current active unicast audio instance */
#define LeUnicastManager_InstanceGetAudioContext(inst)    ((inst)->audio_context)

/*! Returns the Left Sink ASE */
#define LeUnicastManager_InstanceGetLeftSinkAse(inst)     (&(inst)->ase[le_um_audio_location_left_sink])

/*! Returns the Right Sink ASE */
#define LeUnicastManager_InstanceGetRightSinkAse(inst)    (&(inst)->ase[le_um_audio_location_right_sink])

/*! Returns the Left Source ASE */
#define LeUnicastManager_InstanceGetLeftSourceAse(inst)   (&(inst)->ase[le_um_audio_location_left_source])

/*! Returns the Right Source ASE */
#define LeUnicastManager_InstanceGetRightSourceAse(inst)  (&(inst)->ase[le_um_audio_location_right_source])

#endif // LE_UNICAST_MANAGER_INSTANCE_H
/*! @} */