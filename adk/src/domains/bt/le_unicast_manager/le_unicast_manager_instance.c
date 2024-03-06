/*!
    \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    le_unicast_manager
    \brief      Data for a GATT connection bein tracked by the le unicast manager
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST)

#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_private.h"

#include <gatt.h>


le_um_task_data_t le_unicast_taskdata;


void LeUnicastManager_InstanceInit(void)
{
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        LeUnicastManager_InstanceReset(inst);
    }
}

void LeUnicastManager_InstanceReset(le_um_instance_t *inst)
{
    le_um_cis_t *cis;

    memset(inst, 0, sizeof(*inst));
    inst->cid = INVALID_CID;

    ARRAY_FOREACH(cis, inst->cis)
    {
        cis->cis_handle = LE_INVALID_CIS_HANDLE;
        cis->cis_id = LE_INVALID_CIS_ID;
    }
}

le_um_instance_t *LeUnicastManager_GetInstance(void)
{
    return &LeUnicastManager_GetTaskData()->le_unicast_instances[0];
}

le_um_instance_t *LeUnicastManager_InstanceGetByCid(gatt_cid_t cid)
{
    le_um_instance_t *match = NULL;
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        if (inst->cid == cid)
        {
            match = inst;
            break;
        }
    }

    return match;
}

le_um_instance_t *LeUnicastManager_InstanceGetByVoiceSource(voice_source_t source)
{
    /* TBD: add a way to be able to map the multiple connections to a voice
            source. e.g. have two different le_voice sources, similar to hfp. */
    if (source == voice_source_le_audio_unicast_1)
    {
        return LeUnicastManager_GetInstance();
    }

    return NULL;
}

le_um_instance_t *LeUnicastManager_InstanceGetByAudioSource(audio_source_t source)
{
    /* TBD: add a way to be able to map the multiple connections to an audio
            source. e.g. have two different le_voice sources, similar to a2dp. */
    if (source == audio_source_le_audio_unicast_1)
    {
        return LeUnicastManager_GetInstance();
    }

    return NULL;
}

le_um_instance_t *LeUnicastManager_InstanceGetByCidOrCreate(gatt_cid_t cid)
{
    le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);

    if (!inst)
    {
        inst = LeUnicastManager_InstanceGetByCid(INVALID_CID);
        if (inst)
        {
            inst->cid = cid;
        }
    }

    DEBUG_LOG("LeUnicastManager_InstanceCreate instance %p", inst);

    return inst;
}

le_um_instance_t *LeUnicastManager_InstanceGetByCisHandle(uint16 cis_handle)
{
    le_um_instance_t *match = NULL;
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        /* Note: there is no check that the instance is valid because at the
                 moment it is possible for the CIS disconnect to arrive after
                 the ASE(s) have all been disabled and the instance has been
                 marked as invalid. */

        le_um_cis_t *cis = NULL;

        ARRAY_FOREACH(cis, inst->cis)
        {
            if (   (cis->state != le_um_cis_state_free)
                && (cis->cis_handle == cis_handle))
            {
                match = inst;
                break;
            }
        }
    }

    return match;
}

le_um_instance_t *LeUnicastManager_InstanceGetByAseCisId(uint8 cis_id)
{
    le_um_instance_t *match = NULL;
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        if (LeUnicastManager_InstanceIsValid(inst))
        {
            le_um_ase_t *ase = NULL;

            ARRAY_FOREACH(ase, inst->ase)
            {
                if (   LeUnicastManager_IsAseActive(ase)
                    && ase->qos_info->cisId == cis_id)
                {
                    match = inst;
                    break;
                }
            }
        }
    }

    return match;
}

le_um_internal_msg_t LeUnicastManager_GetCisLinklossMessageForInst(le_um_instance_t *inst)
{
    le_um_instance_t *le_um_inst = LeUnicastManager_GetTaskData()->le_unicast_instances;
    le_um_internal_msg_t msg =LE_UM_INTERNAL_INVALID_MSG;

    if (inst == &le_um_inst[0])
    {
        msg = LE_UM_INTERNAL_CIS_LINKLOSS_CONFIRMATION_INST1;
    }
    else if (inst == &le_um_inst[1])
    {
        msg = LE_UM_INTERNAL_CIS_LINKLOSS_CONFIRMATION_INST2;
    }

    return msg;
}

#endif /* INCLUDE_LE_AUDIO_UNICAST */
