/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       le_unicast_manager_handover.c
    \ingroup    le_unicast_manager
    \brief      Unicast manager Handover functions are defined
*/

#if defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING)


#include <panic.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include "marshal.h"
#include "handover_if.h"
#include "logging.h"
#include "le_unicast_manager_instance.h"
#include "le_unicast_manager_marshal_desc.h"
#include "le_unicast_manager_task.h"
#include "gatt.h"
#include "gatt_connect.h"
#include "mirror_profile.h"
#include "le_unicast_manager_private.h"


/*! Use this flag to clean unmarshalled data, if any, during handover abort phase */
static bool unmarshalled = FALSE;

/*! \brief Update cis info for the ASE after a handover */
static le_um_cis_t * leUnicastManager_HandoverUpdateCisInfoForAse(
        le_um_instance_t * inst,
        uint8 cis_id,
        hci_connection_handle_t cis_handle,
        bool cis_for_peer)
{
    le_um_cis_t *cis_data;

    UNICAST_MANAGER_LOG("leUnicastManager_HandoverUpdateCisInfoForAse cis_id=0x%x", cis_id);

    /* Find a free slot for storing CIS data in unicast manager instance */
    ARRAY_FOREACH(cis_data, inst->cis)
    {
        if (cis_data->state == le_um_cis_state_free)
        {
            cis_data->cis_id = cis_id;
            cis_data->cis_handle = cis_handle;
            cis_data->state = le_um_cis_state_data_path_ready;
            cis_data->is_cis_delegated = cis_for_peer;

            return cis_data;
        }
        else if (cis_data->cis_id == cis_id)
        {
            return cis_data;
        }
    }

    return NULL;
}

/*! \brief If Unicast manager is preparing for streaming music, veto */
static bool leUnicastManager_Veto(void)
{
    le_um_instance_t *inst = NULL;

    ARRAY_FOREACH(inst, LeUnicastManager_GetTaskData()->le_unicast_instances)
    {
        le_um_state_t unicast_state = LeUnicastManager_GetState(inst);

        if (unicast_state == le_um_state_preparing || unicast_state == le_um_state_disabling)
        {
            /* Veto the handover */
            DEBUG_LOG("leUnicastManager_Veto: Unicast manager is preparing/disabling a audio session");
            return TRUE;
        }
    }

    /* Check message queue status */
    if (MessagesPendingForTask(LeUnicastManager_GetTask(), NULL) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*! \brief Iterate through ASE's in streaming state and marshal data */
static bool leUnicastManager_Marshal(const tp_bdaddr *tp_bd_addr,
                                            uint8 *buf,
                                            uint16 length,
                                            uint16 *written)
{
    le_um_ase_t *ase;
    DEBUG_LOG("leUnicastManager_Marshal");
    uint8 ase_index = 0;
    gatt_cid_t cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (cid != INVALID_CID)
    {
        le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);

        if (inst)
        {
            if (LeUnicastManager_GetState(inst) == le_um_state_streaming)
            {
                bool marshalled;
                unicast_manager_marshal_data_t obj;

                /* Iterate through the ase list and get the Ase Ids in streaming state */
                ARRAY_FOREACH(ase, inst->ase)
                {
                    obj.ase_id[ase_index] = ase->ase_id;
                    ase_index++;
                }

                obj.audio_context = inst->audio_context;

                obj.cid = inst->cid;

                DEBUG_LOG("leUnicastManager_Marshal: Marashalling for addr[0x%06x]", tp_bd_addr->taddr.addr.lap);

                marshaller_t marshaller = MarshalInit(mtdesc_unicast_mgr, UNICAST_MANAGER_MARSHAL_OBJ_TYPE_COUNT);
                MarshalSetBuffer(marshaller, (void*)buf, length);
                marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(unicast_manager_marshal_data_t));
                *written = marshalled? MarshalProduced(marshaller): 0;
                MarshalDestroy(marshaller, FALSE);
                return marshalled;
            }
        }
    }

    *written = 0;
    return TRUE;
}

/*! \brief Unmarshal and fill the ASE data */
static bool leUnicastManager_Unmarshal(const tp_bdaddr *tp_bd_addr,
                                               const uint8 *buf,
                                               uint16 length,
                                               uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    marshal_type_t unmarshalled_type;
    unicast_manager_marshal_data_t *data = NULL;
    le_um_ase_t *ase;
    uint8 ase_index = 0;

    DEBUG_LOG("leUnicastManager_Unmarshal");

    unmarshaller_t unmarshaller = UnmarshalInit(mtdesc_unicast_mgr, UNICAST_MANAGER_MARSHAL_OBJ_TYPE_COUNT);
    UnmarshalSetBuffer(unmarshaller, (void *)buf, length);

    if (Unmarshal(unmarshaller, (void **)&data, &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(unicast_manager_marshal_data_t));
        PanicNull(data);

        le_um_instance_t *inst = LeUnicastManager_InstanceGetByCidOrCreate(data->cid);
        PanicNull(inst);

        ARRAY_FOREACH(ase, inst->ase)
        {
            ase->ase_id = data->ase_id[ase_index];
            ase_index++;
        }

        inst->audio_context = data->audio_context;

        unmarshalled = TRUE;
        *consumed = UnmarshalConsumed(unmarshaller);
        UnmarshalDestroy(unmarshaller, TRUE);
        return TRUE;
    }
    else
    {
        *consumed = 0;
        DEBUG_LOG("leUnicastManager_Unmarshal: failed unmarshal");
        UnmarshalDestroy(unmarshaller, TRUE);
        return FALSE;
    }
}

/*! \brief Populate the Unicast manager instance on new primary */
static void leUnicastManager_HandleCommitForPrimary(le_um_instance_t *inst)
{
    le_um_ase_t *ase;
    le_um_cis_t *cis_data;
    le_um_ase_t *sink_ase, *source_ase;
    hci_connection_handle_t own_handle, peer_handle, cis_handle;

    DEBUG_LOG("leUnicastManager_HandoverCommit For Primary");

    if (Multidevice_GetSide() == multidevice_side_left)
    {
        sink_ase = LeUnicastManager_InstanceGetLeftSinkAse(inst);
        source_ase = LeUnicastManager_InstanceGetLeftSourceAse(inst);
    }
    else
    {
        sink_ase = LeUnicastManager_InstanceGetRightSinkAse(inst);
        source_ase = LeUnicastManager_InstanceGetRightSourceAse(inst);
    }

    MirrorProfile_GetCisHandle(&own_handle, &peer_handle);

    ARRAY_FOREACH(ase, inst->ase)
    {
        if (ase->ase_id != LE_INVALID_ASE_ID)
        {
            /* Get the direction for the ASE */
            ase->direction = LeBapUnicastServer_GetAseDirection(inst->cid, ase->ase_id);

            /* Set the state of the ASE to Streaming */
            ase->state = le_um_ase_state_routed;

            /* Set the claimed audio context */
            ase->audio_context = inst->audio_context;

            /* Get the configured codec information */
            ase->codec_info = LeBapUnicastServer_GetCodecParameters(inst->cid, ase->ase_id);

            /* Get the configured qos information */
            ase->qos_info = LeBapUnicastServer_GetQoSParameters(inst->cid, ase->ase_id);

            ase->codec_version = (ase->codec_info == NULL) ? 0 : leUnicastManger_DetermineCodecVersion(ase->codec_info);

            PanicFalse(ase->codec_info != NULL && ase->qos_info != NULL);
            cis_handle = ((ase == sink_ase || ase == source_ase) ? own_handle : peer_handle);
            cis_data = leUnicastManager_HandoverUpdateCisInfoForAse(inst,
                                                                    ase->qos_info->cisId,
                                                                    cis_handle,
                                                                    cis_handle == peer_handle ? TRUE : FALSE);
            ase->cis_data = cis_data;
        }
    }

    (void) leUnicastManager_UpdateMirrorType(inst);
}

/*! \brief Reset the Unicast manager context on new secondary */
static void leUnicastManager_HandleCommitForSecondary(le_um_instance_t *inst)
{
    le_um_ase_t *ase;

    /* Restore the audio context back to PACS service */
    ARRAY_FOREACH(ase, inst->ase)
    {
        if (LeUnicastManager_IsAseActive(ase))
        {
            LeUnicastManager_RestoreAudioContext(ase->direction, ase->audio_context);
        }
    }

    LeUnicastManager_InstanceReset(inst);
}

/*! \brief Handle commit for Unicast manager */
static void leUnicastManager_HandoverCommit(const tp_bdaddr *tp_bd_addr, const bool is_primary)
{
    gatt_cid_t cid = GattConnect_GetConnectionIdFromTpaddr(tp_bd_addr);

    if (cid != INVALID_CID)
    {
        le_um_instance_t *inst = LeUnicastManager_InstanceGetByCid(cid);

        if (inst)
        {
            if (is_primary)
            {
                leUnicastManager_HandleCommitForPrimary(inst);
            }
            else
            {
                leUnicastManager_HandleCommitForSecondary(inst);
            }
        }
    }
}

/*! \brief Handle handover complete for Unicast manager */
static void leUnicastManager_HandoverComplete(const bool is_primary )
{
    UNUSED(is_primary);
    /* mark complete of unmarshalled data */
    unmarshalled = FALSE;
}

/*! \brief On abort, reset the unicast manager task data in the secondary */
static void leUnicastManager_HandoverAbort(void)
{
    DEBUG_LOG("leUnicastManager_HandoverAbort");

    if (unmarshalled)
    {
        LeUnicastManager_TaskReset();
        unmarshalled = FALSE;
    }
}


/*! \brief On abort, Unicast manager handover interfaces */
const handover_interface unicast_manager_handover_if =
        MAKE_BLE_HANDOVER_IF(&leUnicastManager_Veto,
                             &leUnicastManager_Marshal,
                             &leUnicastManager_Unmarshal,
                             &leUnicastManager_HandoverCommit,
                             &leUnicastManager_HandoverComplete,
                             &leUnicastManager_HandoverAbort);

#endif /* defined(INCLUDE_LE_AUDIO_UNICAST) && defined(INCLUDE_MIRRORING) */
