/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    mirror_profile
    \brief      Functions to send & process MDM prims to / from the firmware.
*/


#ifdef INCLUDE_MIRRORING

#include <bdaddr.h>
#include <sink.h>
#include <stream.h>

#include "bt_device.h"
#ifdef USE_SYNERGY
#include <hf_lib.h>
#endif /* USE_SYNERGY */
#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "mirror_profile_audio_source.h"
#include "handover_profile.h"
#include "kymera.h"
#include "volume_utils.h"
#include "qualcomm_connection_manager.h"
#include "system_clock.h"
#include "power_manager.h"
#include "hfp_profile_config.h"

/*! Construct a MDM prim of the given type */
#define MAKE_MDM_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;

/*! Defines for configure data path request vendor specific Types */
#define CONFIG_DATA_PATH_VS_LENGTH      7
#define CONNECTION_HANDLE_TYPE          0x06
#define CONNECTION_HANDLE_TYPE_LENGTH   3

#define RELAY_INFO_TYPE                 0x0c
#define RELAY_INFO_TYPE_LENGTH          2

#define RELAY_INFO_FLAG_FOR_RELAY       0x01
#define RELAY_INFO_FLAG_FOR_HANDOVER    0x02

#ifdef CDA2
#define mirrorProfile_IsOwnDataPathSetupAllowed(lea_unicast)    ((lea_unicast)->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_1 || \
                                                                 (lea_unicast)->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_2)
#else
#define mirrorProfile_IsOwnDataPathSetupAllowed(lea_unicast)    (TRUE)
#endif

/*
    Functions to send MDM prims to firmware.
*/

void MirrorProfile_MirrorRegisterReq(void)
{
    MAKE_MDM_PRIM_T(MDM_REGISTER_REQ);
    prim->phandle = 0;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorConnectReq(link_type_t type)
{
    bdaddr bd_addr;

    MAKE_MDM_PRIM_T(MDM_LINK_CREATE_REQ);
    prim->phandle = 0;

    MIRROR_LOG("MirrorProfile_MirrorConnectReq type 0x%x", type);

    if (type == LINK_TYPE_ACL)
    {
        bd_addr = DeviceProperties_GetBdAddr(MirrorProfile_GetTargetDevice());
    }
    else
    {
        /* For eSCO connection, use currently mirrored bdaddr */
        bd_addr = *MirrorProfile_GetMirroredDeviceAddress();
    }
    MIRROR_LOG("MirrorProfile_MirrorConnectReq handset addr {%04x %02x %06lx}",
                bd_addr.nap, bd_addr.uap, bd_addr.lap);
    BdaddrConvertVmToBluestack(&prim->mirror_bd_addr.addrt.addr, &bd_addr);
    prim->mirror_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->mirror_bd_addr.tp_type = BREDR_ACL;

    appDeviceGetSecondaryBdAddr(&bd_addr);
    MIRROR_LOG("MirrorProfile_MirrorConnectReq peer addr {%04x %02x %06lx}",
                bd_addr.nap, bd_addr.uap, bd_addr.lap);
    BdaddrConvertVmToBluestack(&prim->secondary_bd_addr.addrt.addr, &bd_addr);
    prim->secondary_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->secondary_bd_addr.tp_type = BREDR_ACL;

    prim->link_type = type;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorDisconnectReq(hci_connection_handle_t conn_handle,
                                              hci_reason_t reason)
{
    MAKE_MDM_PRIM_T(MDM_LINK_DISCONNECT_REQ);

    MIRROR_LOG("mirrorProfile_MirrorDisconnectReq handle 0x%x reason 0x%x", conn_handle, reason);

    prim->phandle = 0;
    prim->conn_handle = conn_handle;
    prim->reason = reason;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capConnectReq(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_CREATE_REQ);

    MIRROR_LOG("MirrorProfile_MirrorL2capConnectReq handle 0x%x cid 0x%x", conn_handle, cid);

    MirrorPioSet();
    prim->phandle = 0;
    prim->flags = 0;
    prim->connection_handle = conn_handle;
    prim->cid = cid;
    VmSendMdmPrim(prim);
    MirrorPioClr();
}

void MirrorProfile_MirrorL2capConnectRsp(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid)
{
    mirror_profile_a2dp_t *a2dp = MirrorProfile_GetA2dpState();
    mirror_profile_cached_context_t* context = MirrorProfile_GetCachedContext(a2dp->audio_source);

    /* Conftab for when QHS connection is active / when the MTU is greater than 875 */
    static const uint16 l2cap_conftab_qhs[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* MTU for QHS enabled 1015 */
        L2CAP_AUTOPT_MTU_IN, 0x03F7,
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Local MTU exact value (incoming). */
        L2CAP_AUTOPT_MTU_IN, 0x037F,
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    MAKE_MDM_PRIM_T(MDM_L2CAP_CREATE_RSP);

    MIRROR_LOG("MirrorProfile_MirrorL2capConnectRsp handle 0x%x cid 0x%x mtu %d", conn_handle, cid, context->mtu);

    MirrorPioSet();
    prim->phandle = 0;
    prim->connection_handle = conn_handle;
    prim->cid = cid;
    prim->conftab_length = CONFTAB_LEN(l2cap_conftab);
    prim->conftab = (context->mtu > 0x037f) ? (uint16*)l2cap_conftab_qhs : (uint16*)l2cap_conftab;
    prim->response = L2CA_CONNECT_SUCCESS;
    VmSendMdmPrim(prim);
    MirrorPioClr();
}

void MirrorProfile_MirrorL2capDisconnectReq(l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_DISCONNECT_REQ);

    MIRROR_LOG("MirrorProfile_MirrorL2capDisconnectReq cid 0x%x", cid);

    prim->phandle = 0;
    prim->cid = cid;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capDisconnectRsp(l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_DISCONNECT_RSP);

    MIRROR_LOG("MirrorProfile_MirrorL2capDisconnectRsp cid 0x%x", cid);

    prim->cid = cid;
    VmSendMdmPrim(prim);
}

#ifdef ENABLE_LEA_CIS_DELEGATION

void MirrorProfile_MirrorCisCreateReq(hci_connection_handle_t cis_handle, cis_ownership_t cis_ownership)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    bdaddr bd_addr;

    MAKE_MDM_PRIM_T(MDM_LE_CIS_CREATE_REQ);

    MIRROR_LOG("MirrorProfile_MirrorCisCreateReq handle: 0x%x, ownership: %d, time: %d", cis_handle, cis_ownership, SystemClockGetTimerTime());

    PanicFalse(cis_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID);

    if (lea_unicast->peer_cis_handle == cis_handle)
    {
        MirrorProfile_SetPeerCisState(MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING);
    }
    else if (lea_unicast->own_cis_handle == cis_handle)
    {
        MirrorProfile_SetOwnCisState(MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING);
    }

    MirrorPioSet();
    appDeviceGetSecondaryBdAddr(&bd_addr);
    BdaddrConvertVmToBluestack(&prim->secondary_bd_addr.addrt.addr, &bd_addr);
    prim->secondary_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->secondary_bd_addr.tp_type = BREDR_ACL;
    prim->cis_handle = cis_handle;
    prim->cis_ownership = cis_ownership;
    VmSendMdmPrim(prim);
    MirrorPioClr();
}

void MirrorProfile_MirrorCisDisconnectReq(hci_connection_handle_t conn_handle, hci_reason_t reason)
{
    MAKE_MDM_PRIM_T(MDM_LE_CIS_DISCONNECT_REQ);

    MIRROR_LOG("MirrorProfile_MirrorCisDisconnectReq handle: 0x%x, reason: %d", conn_handle, reason);

    PanicFalse(conn_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID);

    MirrorPioSet();
    prim->conn_handle = conn_handle;
    prim->reason = reason;
    VmSendMdmPrim(prim);
    MirrorPioClr();
}

#ifdef INCLUDE_CIS_MIRRORING
static void mirrorProfile_ConfigureDataPath(Task mp_task, uint16 cis_handle, uint8 data_path_dir, uint8 data_path_id)
{
    uint8 vendor_specific_length = CONFIG_DATA_PATH_VS_LENGTH;
    uint8 *data[CM_DM_CONFIGURE_DATA_PATH_MAX_INDEX] = {0};
    uint8* vendor_specific_data = (uint8 *) PanicUnlessMalloc(vendor_specific_length * sizeof(uint8));

    vendor_specific_data[0] = CONNECTION_HANDLE_TYPE_LENGTH;
    vendor_specific_data[1] = CONNECTION_HANDLE_TYPE;
    vendor_specific_data[2] = (uint8)cis_handle;
    vendor_specific_data[3] = (uint8)(cis_handle >> 8);
    vendor_specific_data[4] = RELAY_INFO_TYPE_LENGTH;
    vendor_specific_data[5] = RELAY_INFO_TYPE;
    vendor_specific_data[6] = (RELAY_INFO_FLAG_FOR_RELAY | RELAY_INFO_FLAG_FOR_HANDOVER);

    MIRROR_LOG("mirrorProfile_ConfigureDataPath handle: 0x%x, dir: 0x%x, data_path_id: 0x%02x",
               cis_handle, data_path_dir, data_path_id);

    data[0] = vendor_specific_data;
    CmDmConfigureDataPathReqSend(TrapToOxygenTask(mp_task),
                                data_path_dir,
                                data_path_id,
                                vendor_specific_length,
                                &data[0]);
}
#endif

/*! Send request to setup isochronous data path for mirrored CIS */
static uint8 MirrorProfile_SetupIsocDataPath(hci_connection_handle_t handle, uint8 dir, uint8 ul_data_path_id, uint8 dl_data_path_id)
{
    uint8 data_path_count = 0;
    Task mp_task = MirrorProfile_GetTask();

    MIRROR_LOG("MirrorProfile_SetupIsocDataPath handle: 0x%x, dir: 0x%x, ul_data_path_id: 0x%02x, dl_data_path_id: 0x%02x",
               handle, dir, ul_data_path_id, dl_data_path_id);

    if (dir & LE_AUDIO_ISO_DIRECTION_UL)
    {
        data_path_count++;
#ifdef INCLUDE_CIS_MIRRORING
        mirrorProfile_ConfigureDataPath(mp_task, handle, ISOC_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER, ul_data_path_id);
#endif
        ConnectionIsocSetupIsochronousDataPathRequest(mp_task, handle, ISOC_DATA_PATH_DIRECTION_HOST_TO_CONTROLLER, ul_data_path_id);
    }

    if (dir & LE_AUDIO_ISO_DIRECTION_DL)
    {
        data_path_count++;
#ifdef INCLUDE_CIS_MIRRORING
        mirrorProfile_ConfigureDataPath(mp_task, handle, ISOC_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST, dl_data_path_id);
#endif
        ConnectionIsocSetupIsochronousDataPathRequest(mp_task, handle, ISOC_DATA_PATH_DIRECTION_CONTROLLER_TO_HOST, dl_data_path_id);
    }

    return data_path_count;
}

/*! Determine ISO data path type/id based on mirror (ASE) configuration */
static uint16 mirrorProfile_GetDataPathType(mirror_profile_lea_unicast_t *lea_unicast, bool own_cis, bool is_dl_dir)
{
    uint16 data_path_type = own_cis ? ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY
                                    : ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL;
    multidevice_side_t side = Multidevice_GetSide();

    if (own_cis)
    {
        /* Use shadow data path mode if peer CIS used for shared scenario */
        switch (lea_unicast->audio_config.mirror_type)
        {
            case le_um_cis_mirror_type_delegate_with_left_src_shared:
                if (!is_dl_dir && side != multidevice_side_left)
                {
                    data_path_type = ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_right_src_shared:
                if (!is_dl_dir && side != multidevice_side_right)
                {
                    data_path_type = ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_left_snk_shared:
                if (is_dl_dir && side != multidevice_side_left)
                {
                    data_path_type = ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_right_snk_shared:
                if (is_dl_dir && side != multidevice_side_right)
                {
                    data_path_type = ISOC_DATA_PATH_ID_ISO_PATH_SHADOWING_PERIPHERAL;
                }
                break;

            default:
                break;
        }
    }
    else
    {
        /* Use raw data path mode if peer CIS used for shared scenario or mirroring scenario */
        switch (lea_unicast->audio_config.mirror_type)
        {
            case le_um_cis_mirror_type_mirror:
                data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                break;

            case le_um_cis_mirror_type_delegate_with_left_src_shared:
                if (!is_dl_dir && side != multidevice_side_left)
                {
                    data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_right_src_shared:
                if (!is_dl_dir && side != multidevice_side_right)
                {
                    data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_left_snk_shared:
                if (is_dl_dir && side != multidevice_side_left)
                {
                    data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                }
                break;

            case le_um_cis_mirror_type_delegate_with_right_snk_shared:
                if (is_dl_dir && side != multidevice_side_right)
                {
                    data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                }
                break;

            case le_um_cis_mirror_type_delegate:
                /* In case of two CISes established with one of the CIS being a Bi-directional then datapath needs to be enabled
                   for the other direction even if ASEes are enabled only for one of the direction */
                if (is_dl_dir)
                {
                    if (lea_unicast->peer_cis_dir & LE_AUDIO_ISO_DIRECTION_DL && lea_unicast->own_cis_dir == LE_AUDIO_ISO_DIRECTION_UL)
                    {
                        data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                    }
                }
                else
                {
                    if (lea_unicast->peer_cis_dir & LE_AUDIO_ISO_DIRECTION_UL && lea_unicast->own_cis_dir == LE_AUDIO_ISO_DIRECTION_DL)
                    {
                        data_path_type = ISOC_DATA_PATH_ID_RAW_STREAM_ENDPOINTS_ONLY;
                    }
                }
                break;

            default:
                break;
        }
    }

    return data_path_type;
}

void MirrorProfile_CheckAndEstablishDataPath(mirror_profile_lea_unicast_t *lea_unicast)
{
    uint8 data_path_count = 0;

    if (!MirrorProfile_GetLeUnicastConfigRcvd())
    {
        MIRROR_LOG("MirrorProfile_CheckAndEstablishDataPath waiting for config");
        return;
    }

    if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING)
    {
       data_path_count =
          MirrorProfile_SetupIsocDataPath(lea_unicast->peer_cis_handle, lea_unicast->peer_cis_dir,
                                          mirrorProfile_GetDataPathType(lea_unicast, FALSE, FALSE),
                                          mirrorProfile_GetDataPathType(lea_unicast, FALSE, TRUE));
       PanicZero(data_path_count);
       MirrorProfile_SetPeerCisState(data_path_count == 1 ? MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_1
                                                        : MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_2);
    }

    if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING)
    {
        /* Now send request for owned CIS data path establish */
        data_path_count =
           MirrorProfile_SetupIsocDataPath(lea_unicast->own_cis_handle, lea_unicast->own_cis_dir,
                                           mirrorProfile_GetDataPathType(lea_unicast, TRUE, FALSE),
                                           mirrorProfile_GetDataPathType(lea_unicast, TRUE, TRUE));

        PanicZero(data_path_count);
        MirrorProfile_SetOwnCisState(data_path_count == 1 ? MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_1
                                                          : MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_2);
    }
}

#endif /* ENABLE_LEA_CIS_DELEGATION */

/*
    Functions to handle MDM prims sent by firmware.
*/

/*! \brief Handle MDM_REGISTER_CFM

    This is common to both Primary & Secondary
*/
static void mirrorProfile_HandleMdmRegisterCfm(const MDM_REGISTER_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmRegisterCfm result 0x%x", cfm->result);

    if (cfm->result == MDM_RESULT_SUCCESS)
    {
        mirror_profile_task_data_t *sp = MirrorProfile_Get();

        /* Send init confirmation to init module */
        MessageSend(sp->init_task, MIRROR_PROFILE_INIT_CFM, NULL);
    }
    else
    {
        Panic();
    }
}

/*! \brief Handle MDM_ACL_LINK_CREATE_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror ACL connection.
*/
static void mirrorProfile_HandleMdmAclLinkCreateCfm(const MDM_ACL_LINK_CREATE_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateCfm status 0x%x handle 0x%x role 0x%x",
                    cfm->status, cfm->connection_handle, cfm->role);

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateCfm mirror_addr {%04x %02x %06lx}",
                cfm->mirror_bd_addr.addrt.addr.nap,
                cfm->mirror_bd_addr.addrt.addr.uap,
                cfm->mirror_bd_addr.addrt.addr.lap);

    assert(MirrorProfile_IsPrimary());

    if (MirrorProfile_GetState() == MIRROR_PROFILE_STATE_ACL_CONNECTING ||
        MirrorProfile_GetSwitchState() == MIRROR_PROFILE_STATE_ACL_CONNECTING)
    {
        if (cfm->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_PRIMARY == cfm->role);

            sp->acl.conn_handle = cfm->connection_handle;
            BdaddrConvertBluestackToVm(&sp->acl.bd_addr, &cfm->mirror_bd_addr.addrt.addr);

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);

            /* Switching complete, reset the state */
            MirrorProfile_SetSwitchState(MIRROR_PROFILE_STATE_DISCONNECTED);
            MirrorProfile_ClearTransitionLockBitAclSwitching();
        }
        else
        {
            if (MirrorProfile_GetSwitchState() == MIRROR_PROFILE_STATE_ACL_CONNECTING)
            {
                /* Switch ACL connection failed, reset associated state and kick
                   the main profile to try again (should it still be necessary) */
                MirrorProfile_SetSwitchState(MIRROR_PROFILE_STATE_DISCONNECTED);
                MirrorProfile_ClearTransitionLockBitAclSwitching();
                MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_KICK_TARGET_STATE, NULL);
            }
            else
            {
                /* Re-connect the mirror ACL after entering DISCONNECTED */
                MirrorProfile_SetDelayKick();
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
            }
        }
    }
    else
    {
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_CFM, NULL);
    }
}

/*! \brief Handle MDM_ACL_LINK_CREATE_IND

    Only Secondary should receive this, because the Primary always
    initiates the mirror ACL connection.
*/
static void mirrorProfile_HandleMdmAclLinkCreateInd(const MDM_ACL_LINK_CREATE_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateInd status 0x%x handle 0x%x",
                    ind->status, ind->connection_handle);

    assert(!MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_DISCONNECTED:
        if (ind->status == HCI_SUCCESS)
        {
            sp->acl.conn_handle = ind->connection_handle;
            BdaddrConvertBluestackToVm(&sp->acl.bd_addr, &ind->mirror_bd_addr.addrt.addr);

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_LINK_CREATE_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror eSCO connection.
*/
static void mirrorProfile_HandleMdmEscoLinkCreateCfm(const MDM_ESCO_LINK_CREATE_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateCfm status 0x%x handle 0x%x",
                    cfm->status, cfm->connection_handle);

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ESCO_CONNECTING:
        if (cfm->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_PRIMARY == cfm->role);

            sp->esco.conn_handle = cfm->connection_handle;

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ESCO_CONNECTED);
        }
        else
        {
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

    case MIRROR_PROFILE_STATE_DISCONNECTED:
        /* This can occur on mirror ACL disconnect crossover with attempt
           to connect mirror eSCO. The MDM_LINK_DISCONNECT_IND will be received
           first then the MDM_ESCO_LINK_CREATE_CFM(failure) */
        break;

    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
        /* This occurs rarely if a mirror eSCO connection attempt is pending
           when the mirror eSCO/ACL disconnect and the mirror profile attempts to
           reconnect the mirror ACL. In this case, the MDM_ESCO_LINK_CREATE_CFM_T
           from the original eSCO connect request is received when the state
           has transitioned to MIRROR_PROFILE_STATE_ACL_CONNECTING. The status
           is not expected to be success in this case. */
           assert(cfm->status != HCI_SUCCESS);
        break;

    default:
        MirrorProfile_StateError(MDM_ESCO_LINK_CREATE_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_LINK_CREATE_IND

    Only Secondary should receive this, because the Primary always
    initiates the mirror eSCO connection.
*/
static void mirrorProfile_HandleMdmEscoLinkCreateInd(const MDM_ESCO_LINK_CREATE_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd status 0x%x handle 0x%x role %d type %d",
                    ind->status, ind->connection_handle, ind->role, ind->link_type);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd buddy addr {%04x %02x %06lx}",
                    ind->buddy_bd_addr.addrt.addr.nap,
                    ind->buddy_bd_addr.addrt.addr.uap,
                    ind->buddy_bd_addr.addrt.addr.lap);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd mirror addr {%04x %02x %06lx}",
                    ind->mirror_bd_addr.addrt.addr.nap,
                    ind->mirror_bd_addr.addrt.addr.uap,
                    ind->mirror_bd_addr.addrt.addr.lap);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd tx_interval %d wesco %d rx_len 0x%x tx_len 0x%x air_mode 0x%x",
                    ind->tx_interval, ind->wesco, ind->rx_packet_length, ind->tx_packet_length, ind->air_mode);

    assert(!MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        if (ind->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_SECONDARY == ind->role);

            sp->esco.conn_handle = ind->connection_handle;
            sp->esco.wesco = ind->wesco;
            MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd handle=0x%x", sp->esco.conn_handle);

            appPowerPerformanceProfileRequestDuration(appConfigAudioConnectedCpuBoostDuration());

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ESCO_CONNECTED);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_RENEGOTIATED_IND

    The eSCO parameters have changed but the app is not required to do
    anything in reply as it has been handled by the lower layers.

    Handle it here to keep things tidy.
*/
static void mirrorProfile_HandleMdmEscoRenegotiatedInd(const MDM_ESCO_RENEGOTIATED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmEscoRenegotiatedInd tx_interval %d wesco %d rx_len 0x%x tx_len 0x%x",
                    ind->tx_interval, ind->wesco, ind->rx_packet_length, ind->tx_packet_length);
}

/*! \brief Handle MDM_LINK_DISCONNECT_CFM

    Only Primary should receive this, in response to Primary sending
    a MDM_LINK_DISCONNECT_REQ.
*/
static void mirrorProfile_HandleMdmLinkDisconnectCfm(const MDM_LINK_DISCONNECT_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm handle 0x%x status 0x%x type 0x%x role 0x%x",
                cfm->conn_handle, cfm->status, cfm->link_type, cfm->role);

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
        if (cfm->link_type == LINK_TYPE_ACL && cfm->conn_handle == sp->acl.conn_handle)
        {
            switch (cfm->status)
            {
            case HCI_ERROR_CONN_TERM_LOCAL_HOST:
            case HCI_ERROR_OETC_USER:
            case HCI_ERROR_LMP_RESPONSE_TIMEOUT:
            case HCI_ERROR_UNSPECIFIED:
            case HCI_ERROR_CONN_TIMEOUT:
            {
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
                /* Reset state after setting state, so state change handling
                can use this state. */
                sp->acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
                BdaddrSetZero(&sp->acl.bd_addr);
            }
            break;
            default:
                /* Failure to disconnect typically means the ACL is already
                disconnected and a disconnection indication is in-flight.
                Do nothing and wait for the indication. */
            break;
            }
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_ESCO_DISCONNECTING:
        if (cfm->link_type == LINK_TYPE_ESCO && cfm->conn_handle == sp->esco.conn_handle)
        {
            switch (cfm->status)
            {
            case HCI_ERROR_CONN_TERM_LOCAL_HOST:
            case HCI_ERROR_OETC_USER:
            case HCI_ERROR_LMP_RESPONSE_TIMEOUT:
            case HCI_ERROR_UNSPECIFIED:
            case HCI_ERROR_CONN_TIMEOUT:
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
                MirrorProfile_ResetEscoConnectionState();
            break;
            default:
                /* Failure to disconnect normally means the eSCO is already
                disconnected and a disconnection indication is in-flight.
                Do nothing and wait for the indication. */
            break;
            }
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_DISCONNECTED:
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
    case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
    /* Ignore old focused device MDM ACL disconnection while establishing MDM ACL connection  to new focused device */
    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
        MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm invalid state 0x%x", MirrorProfile_GetState());
        break;

    default:
        MirrorProfile_StateError(MDM_LINK_DISCONNECT_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_LINK_DISCONNECT_IND

    Both Primary & Secondary can get this at any time.
    For example, if there is a link-loss between Primary & Secondary.
*/
static void mirrorProfile_HandleMdmLinkDisconnectInd(const MDM_LINK_DISCONNECT_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd handle 0x%x reason 0x%x type 0x%x role 0x%x",
                ind->conn_handle, ind->reason, ind->link_type, ind->role);

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
    case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
    case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
    case MIRROR_PROFILE_STATE_ESCO_CONNECTING:
    case MIRROR_PROFILE_STATE_DISCONNECTED:
        if (ind->link_type == LINK_TYPE_ACL
            && ind->conn_handle == sp->acl.conn_handle)
        {
            /* If we are the Primary, we should retry creating the mirror ACL connection
               after a short delay */
            if (MirrorProfile_IsPrimary())
            {
                /* Re-connect the mirror ACL after entering DISCONNECTED */
                MirrorProfile_SetDelayKick();
            }
            sp->acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
            BdaddrSetZero(&sp->acl.bd_addr);
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_ESCO_CONNECTED:
    case MIRROR_PROFILE_STATE_ESCO_DISCONNECTING:
        if (ind->link_type == LINK_TYPE_ESCO
            && ind->conn_handle == sp->esco.conn_handle)
        {
            MirrorProfile_ResetEscoConnectionState();
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd Unrecognised mirror link; ignoring");
        }
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
    case MIRROR_PROFILE_STATE_CIS_CONNECTING:
        if (ind->link_type == LINK_TYPE_ESCO && ind->conn_handle == sp->esco.conn_handle)
        {
            MirrorProfile_ResetEscoConnectionState();
        }
        else if (ind->link_type == LINK_TYPE_ACL && ind->conn_handle == sp->acl.conn_handle)
        {
            sp->acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
            BdaddrSetZero(&sp->acl.bd_addr);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd Unrecognised mirror link; ignoring");
        }
        break;
#endif

    default:
        MirrorProfile_StateError(MDM_LINK_DISCONNECT_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_CREATE_IND.
*/
static void mirrorProfile_HandleMdmMirrorL2capCreateInd(const MDM_L2CAP_CREATE_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateInd cid 0x%x handle 0x%x",
                ind->cid, ind->connection_handle);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        {
            mirror_profile_a2dp_t *a2dp_state = MirrorProfile_GetA2dpState();
            a2dp_state->cid = ind->cid;
            a2dp_state->audio_source = MirrorProfile_GetCachedAudioSourceForCid(ind->cid);
            PanicFalse(a2dp_state->audio_source != audio_source_none);

            PanicFalse(ind->connection_handle == MirrorProfile_GetAclState()->conn_handle);
            MirrorProfile_MirrorL2capConnectRsp(
                    MirrorProfile_GetAclState()->conn_handle,
                    MirrorProfile_GetA2dpState()->cid);
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_A2DP_CONNECTING);
        }
        break;
        default:
            MirrorProfile_StateError(MDM_L2CAP_CREATE_IND, NULL);
        break;
    }
}


/*! \brief Handle MDM_L2CAP_CREATE_CFM.
*/
static void mirrorProfile_HandleMdmMirrorL2capCreateCfm(const MDM_L2CAP_CREATE_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateCfm cid 0x%x handle 0x%x result 0x%x",
                cfm->cid, cfm->connection_handle, cfm->result);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        {
            if (cfm->result == L2CA_CONNECT_SUCCESS)
            {
                MirrorPioSet();
                PanicFalse(cfm->cid == MirrorProfile_GetA2dpState()->cid);
                PanicFalse(cfm->connection_handle == MirrorProfile_GetAclState()->conn_handle);

                if(MirrorProfile_IsPrimary())
                {
                    /* Move to connected and wait for SYNC activate to route audio */
                    MirrorProfile_SetState(MIRROR_PROFILE_STATE_A2DP_CONNECTED);
                }
                else
                {
                    /* Move straight to routed on secondary */
                    MirrorProfile_SetState(MIRROR_PROFILE_STATE_A2DP_ROUTED);
                }
                MirrorPioClr();

            }
            else
            {
                MirrorProfile_SetDelayKick();
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);

            }
        }
        break;
        case MIRROR_PROFILE_STATE_DISCONNECTED:
        case MIRROR_PROFILE_STATE_ACL_CONNECTING:
            MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateCfm invalid state 0x%x", MirrorProfile_GetState());
        break;
        default:
            MirrorProfile_StateError(MDM_L2CAP_CREATE_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_DISCONNECT_IND.
*/
static void mirrorProfile_HandleMdmMirrorL2capDisconnectInd(const MDM_L2CAP_DISCONNECT_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectInd cid 0x%x", ind->cid);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
        case MIRROR_PROFILE_STATE_DISCONNECTED:
        /* Ignore old focused device MDM L2CAP disconnection while establishing MDM ACL connection  to new focused device */
        case MIRROR_PROFILE_STATE_ACL_CONNECTING:
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
        break;

        case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
            /* Handle disconnect crossover. Accept the disconnection, but don't
               change state, just wait for the MDM_L2CAP_DISCONNECT_CFM_T.  */
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
        break;

        case MIRROR_PROFILE_STATE_A2DP_ROUTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        {
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
        case MIRROR_PROFILE_STATE_CIS_CONNECTING:
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
            break;
#endif

        default:
            MirrorProfile_StateError(MDM_L2CAP_DISCONNECT_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_DISCONNECT_CFM.
*/
static void mirrorProfile_HandleMdmMirrorL2capDisconnectCfm(const MDM_L2CAP_DISCONNECT_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectCfm cid 0x%x, status 0x%x",
                cfm->cid, cfm->status);

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_DISCONNECTED:
        {
            /* When the L2CAP connection between peers disconnects the state
               transitions to MIRROR_PROFILE_STATE_DISCONNECTED. Shortly afterwards
               the mirror connection disconnects resulting in this message which
               is ignored. */
        }
        break;

        case MIRROR_PROFILE_STATE_A2DP_ROUTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
        {
            PanicFalse(cfm->cid == MirrorProfile_GetA2dpState()->cid);
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

        case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
        /* Ignore old focused device MDM L2CAP disconnection while establishing MDM ACL connection  to new focused device */
        case MIRROR_PROFILE_STATE_ACL_CONNECTING:
            MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectCfm invalid state 0x%x", MirrorProfile_GetState());
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
        case MIRROR_PROFILE_STATE_CIS_CONNECTING:
            break;
#endif

        default:
            MirrorProfile_StateError(MDM_L2CAP_DISCONNECT_CFM, NULL);
        break;
    }
}

/* Convert BT clock (in 1/2 slots) to microseconds */
static rtime_t btclock_to_rtime(uint32 btclock)
{
    rtime_t btclock_us = (btclock / 2) * US_PER_SLOT;
    return (btclock & 1) ? (btclock_us + HALF_SLOT_US) : btclock_us;
}

static void mirrorProfile_HandleMdmMirrorL2capDataSyncInd(const MDM_L2CAP_DATA_SYNC_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDataSyncInd cid 0x%x", ind->cid);
    if (ind->cid == MirrorProfile_GetA2dpState()->cid)
    {
        wallclock_state_t wc_state;
        if (RtimeWallClockGetStateForSink(&wc_state, StreamL2capSink(ind->cid)))
        {
            rtime_t clock_rtime;
            (void)RtimeWallClockToLocal(&wc_state, btclock_to_rtime(ind->clock), &clock_rtime);
            appKymeraA2dpSetSyncStartTime(clock_rtime);
        }
    }
}

#ifdef ENABLE_LEA_CIS_DELEGATION

/*! \brief Determines CIS next state on primary based on current and CIS delegate state

    \param state Current state
    \param peer_side_informed Is information transferred to other side

    \return Next CIS state
*/
static mirror_profile_cis_sub_state_t mirrorProfile_GetCisNextStateOnPrimary(mirror_profile_cis_sub_state_t state,
                                                                             bool peer_side_informed)
{
    mirror_profile_cis_sub_state_t next_state = state;

    switch (state)
    {
        case MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING:
            next_state = peer_side_informed ? MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED :
                                              MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT;
            break;

        case MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED:
            /* Has to be due to disconnect */
            assert(!peer_side_informed);
            next_state = MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT;
            break;

        case MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECTING:
        case MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECT_ON_CONNECTING:
            next_state = peer_side_informed ? MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECTING :
                                              MIRROR_PROFILE_CIS_SUB_STATE_IDLE;
            break;

        default:
            MIRROR_LOG("mirrorProfile_GetCisNextStateOnPrimary bad CIS state enum:mirror_profile_cis_sub_state_t:%d", state);
            assert(0);
            break;
    }

    return next_state;
}

/*! \brief Determines if CIS handle is for peer or own. Needs to be called only on primary.

    \param cis_handle Recieved cis_handle

    \return TRUE if handle is associated to peer CIS
*/
static bool mirrorProfile_IsPeerCisHandleOnPrimary(hci_connection_handle_t cis_handle)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    bool is_peer_cis;

    if (cis_handle == MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)
    {
        /* Unfortunaetly sometimes MDM doesn't carry valid handle on failure
           i.e. race between disconnection request from UM and MDM-CONNECT-REQ */
        if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING ||
            lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECT_ON_CONNECTING)
        {
            is_peer_cis = TRUE;
        }
        else if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING ||
                 lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DISCONNECT_ON_CONNECTING)
        {
            is_peer_cis = FALSE;
        }
        else
        {
            is_peer_cis = FALSE;
            MIRROR_LOG("mirrorProfile_IsPeerCisHandleOnPrimary invalid cis state (own: enum:mirror_profile_cis_sub_state_t:%d, peer: enum:mirror_profile_cis_sub_state_t:%d)",
                       lea_unicast->own_cis_state, lea_unicast->peer_cis_state);
            assert(0);
        }
    }
    else if (lea_unicast->peer_cis_handle == cis_handle)
    {
        is_peer_cis = TRUE;
    }
    else if (lea_unicast->own_cis_handle == cis_handle)
    {
        is_peer_cis = FALSE;
    }
    else
    {
        is_peer_cis = FALSE;
        MIRROR_LOG("mirrorProfile_IsPeerCisHandleOnPrimary invalid cis handle (own: 0x%x, peer: 0x%x)",
                   lea_unicast->own_cis_handle, lea_unicast->peer_cis_handle);
        assert(0);
    }

    return is_peer_cis;
}

#ifdef INCLUDE_CIS_MIRRORING
/*! \brief Enable/Disable CIS relay request

    Only Primary should be calling this, because the Primary always
    initiates the mirror LE CIS connection and relay enabling.

    \param cis_handle cis_handle for which relay enable/disable is requested
    \param relay_direction direction for relay Owner to mirror or mirror to owner LE_RELAY_DIR_OWNER_TO_MIRROR.
    \param enable_relay Enable/Disable relay

*/
static void MirrorProfile_MirrorCisSetRelayEnable(hci_connection_handle_t cis_handle, uint8 relay_direction, bool enable_relay)
{
    bdaddr bd_addr;

    MAKE_MDM_PRIM_T(MDM_LE_SET_RELAY_ENABLE_REQ);

    MIRROR_LOG("MirrorProfile_MirrorCisSetRelayEnable handle: 0x%x, relay_direction: %d, enable_relay: %d", cis_handle, relay_direction, enable_relay);

    PanicFalse(cis_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID);

    appDeviceGetSecondaryBdAddr(&bd_addr);
    BdaddrConvertVmToBluestack(&prim->buddy_bd_addr.addrt.addr, &bd_addr);
    prim->buddy_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->buddy_bd_addr.tp_type = BREDR_ACL;
    prim->cis_bis_handle = cis_handle;
    prim->relay_dir = relay_direction;
    prim->enable = enable_relay;
    prim->relay_nse = 0x1f;
    VmSendMdmPrim(prim);
    MirrorPioClr();
}

/*! \brief Handle MDM_LE_SET_RELAY_ENABLE_CFM

    Only Primary should receive this, because the Primary always
    initiates the Relay Enable request

    \param cfm Message body
*/
static void mirrorProfile_HandleMdmLeSetRelayEnableCfm(const MDM_LE_SET_RELAY_ENABLE_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmLeSetRelayEnableCfm handle: 0x%x, status: 0x%x, Relay Enable: %d", cfm->cis_bis_handle, cfm->status, cfm->enable);
}

/*! \brief Handle MDM_LE_RELAY_ENABLE_IND

    Only Secondary should receive this, because only the Primary always
    initiates the Relay Enable request

    \param cfm Message body
*/
static void mirrorProfile_HandleMdmLeRelayEnableInd(const MDM_LE_RELAY_ENABLE_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmLeRelayEnableInd handle: 0x%x, status: 0x%x, Relay Enable: %d", ind->cis_bis_handle, ind->status, ind->enable);
}
#endif /* INCLUDE_CIS_MIRRORING */

/*! \brief Handle MDM_LE_CIS_CREATE_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror LE CIS connection.

    \param cfm Message body
*/
static void mirrorProfile_HandleMdmLeCisCreateCfm(const MDM_LE_CIS_CREATE_CFM_T *cfm)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    MIRROR_LOG("mirrorProfile_HandleMdmLeCisCreateCfm handle: 0x%x, status: 0x%x, time: %d", cfm->cis_handle, cfm->status, SystemClockGetTimerTime());

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_CIS_CONNECTING:
        {
            bool is_success = cfm->status == HCI_SUCCESS;
            bool retry_later = cfm->status == HCI_ERROR_NO_CONNECTION; /* @TODO Same should be done for OETC? */
            /* Ignore handle if it is a failure */
            hci_connection_handle_t cis_handle = is_success ? cfm->cis_handle : MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;

            if (is_success)
            {
                BdaddrConvertBluestackToVm(&lea_unicast->bd_addr, &cfm->mirror_bd_addr.addrt.addr);
            }

            if (mirrorProfile_IsPeerCisHandleOnPrimary(cis_handle))
            {
                if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING)
                {
                    LeUnicastManager_CisMirrorStatus(lea_unicast->peer_cis_handle, is_success);
                }

                MirrorProfile_SetPeerCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->peer_cis_state, is_success));
            }
            else
            {
                MirrorProfile_SetOwnCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->own_cis_state, is_success));
#ifdef INCLUDE_CIS_MIRRORING
                if (MirrorProfile_IsStereoCisMirroring(lea_unicast) && is_success)
                {
                    MirrorProfile_MirrorCisSetRelayEnable(cis_handle, 0x01 /*LE_RELAY_DIR_OWNER_TO_MIRROR */, LE_RELAY_ENABLE);
                }
#endif /* INCLUDE_CIS_MIRRORING */
            }

            if (MirrorProfile_IsReqdCisInConnectedState(lea_unicast))
            {
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_CIS_CONNECTED);
            }
            else if (MirrorProfile_IsReqdCisInReadyToConnectState(lea_unicast))
            {
                if (MirrorProfile_Get()->acl.conn_handle == MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)
                {
                    /* Retry - try again later */
                    MirrorProfile_SetDelayKick();
                    MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
                }
                else
                {
                    /* This is race between CIS disconnection request and CIS delegation request wherein ACL
                       mirroring disconnection was not completed. So directly go ACL connected state and also
                       kick start the target re-evaluate
                     */
                    MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
                    MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_KICK_TARGET_STATE, NULL);
                }
            }
            else if (!retry_later)
            {
                /* retry_later TRUE means it is highly likely that second CIS will also go down very soon */
                if (lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT)
                {
                    MirrorProfile_MirrorCisCreateReq(lea_unicast->peer_cis_handle,
                        lea_unicast->own_cis_handle != lea_unicast->peer_cis_handle ? CIS_DELEGATE_SECONDARY
                                                                                    : CIS_MIRRORING_SECONDARY);
                }
                else if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_READY_TO_CONNECT)
                {
                    MirrorProfile_MirrorCisCreateReq(lea_unicast->own_cis_handle, CIS_NO_DELEGATE_NO_MIRRORING_SECONDARY);
                }
                else
                {
                    /* Disconnection in progress */
                    assert(MirrorProfile_IsAnyCisInDisconnectingState(lea_unicast));
                }
            }
        }
        break;

    case MIRROR_PROFILE_STATE_CIS_DISCONNECTING:
    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
        if (cfm->status != HCI_SUCCESS)
        {
            if (mirrorProfile_IsPeerCisHandleOnPrimary(cfm->cis_handle))
            {
                MirrorProfile_SetPeerCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->peer_cis_state, FALSE));
            }
            else
            {
                MirrorProfile_SetOwnCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->own_cis_state, FALSE));
            }

            if (MirrorProfile_IsReqdCisInReadyToConnectState(lea_unicast))
            {
                if (MirrorProfile_GetState() != MIRROR_PROFILE_STATE_ACL_CONNECTING)
                {
                    MirrorProfile_SetDelayKick();
                    MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
                }
            }
        }
        break;

    default:
        /* Race condition */
        MIRROR_LOG("mirrorProfile_HandleMdmLeCisCreateCfm invalid state: %d", MirrorProfile_GetState());
        break;
    }
}

/*! \brief Handle MDM_LE_CIS_CREATE_IND

    Only Secondary should receive this, because the Primary always
    initiates the mirror LE CIS connection.

    \param ind Message body
*/
static void mirrorProfile_HandleMdmLeCisCreateInd(const MDM_LE_CIS_CREATE_IND_T *ind)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    hci_connection_handle_t cis_handle = ind->cis_handle;

    MIRROR_LOG("mirrorProfile_HandleMdmLeCisCreateInd handle: 0x%x, status: 0x%x", ind->cis_handle, ind->status);

    assert(!MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_DISCONNECTED:
    case MIRROR_PROFILE_STATE_CIS_CONNECTED:
        if (ind->status == HCI_SUCCESS)
        {
            if (MirrorProfile_GetState() != MIRROR_PROFILE_STATE_CIS_CONNECTED)
            {
                BdaddrConvertBluestackToVm(&lea_unicast->bd_addr, &ind->mirror_bd_addr.addrt.addr);
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_CIS_CONNECTED);
            }

            if (ind->cis_ownership == CIS_NO_DELEGATE_NO_MIRRORING_SECONDARY)
            {
                lea_unicast->peer_cis_handle = cis_handle;
                lea_unicast->peer_cis_dir = ind->cis_params.bn_c_to_p != 0 ? LE_AUDIO_ISO_DIRECTION_DL : 0;
                lea_unicast->peer_cis_dir |= ind->cis_params.bn_p_to_c != 0 ? LE_AUDIO_ISO_DIRECTION_UL : 0;

                if (lea_unicast->audio_config.mirror_type == le_um_cis_mirror_type_mirror)
                {
#ifdef INCLUDE_CIS_MIRRORING
                    /* TBD Do we need this type?? */
                    lea_unicast->peer_cis_type = Multidevice_IsLeft() ? mirror_profile_cis_type_both_render_left : mirror_profile_cis_type_both_render_right;
#endif
                }

                MirrorProfile_SetPeerCisState(MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING);
            }
            else
            {
                /* Power on Audio SS in case it is in OFF state */
                appKymeraProspectiveDspPowerOn(KYMERA_POWER_ACTIVATION_MODE_ASYNC);

                lea_unicast->own_cis_handle = cis_handle;
                lea_unicast->own_cis_dir = ind->cis_params.bn_c_to_p != 0 ? LE_AUDIO_ISO_DIRECTION_DL : 0;
                lea_unicast->own_cis_dir |= ind->cis_params.bn_p_to_c != 0 ? LE_AUDIO_ISO_DIRECTION_UL : 0;
                MirrorProfile_SetOwnCisState(MIRROR_PROFILE_CIS_SUB_STATE_CONNECTING);
            }

            MirrorProfile_CheckAndEstablishDataPath(lea_unicast);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_LE_CIS_CREATE_IND, NULL);
        break;
    }
}


/*! \brief Handle MDM_LE_CIS_DISCONNECT_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror LE CIS disconnection.

    \param cfm Message body
*/
static void mirrorProfile_HandleMdmLeCisDisconnectCfm(const MDM_LE_CIS_DISCONNECT_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectCfm handle: 0x%x, status: 0x%x", cfm->conn_handle, cfm->status);

    Panic();
}

/*! \brief Handle MDM_LE_CIS_DISCONNECT_IND on Primary.

    \param ind Message body
*/
static void mirrorProfile_HandleMdmLeCisDisconnectIndOnPrimary(const MDM_LE_CIS_DISCONNECT_IND_T *ind)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnPrimary handle: 0x%x, reason: 0x%x", ind->conn_handle, ind->reason);

    if (mirrorProfile_IsPeerCisHandleOnPrimary(ind->conn_handle))
    {
        MirrorProfile_SetPeerCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->peer_cis_state, FALSE));
    }
    else
    {
        MirrorProfile_SetOwnCisState(mirrorProfile_GetCisNextStateOnPrimary(lea_unicast->own_cis_state, FALSE));
    }

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_CIS_DISCONNECTING:
    case MIRROR_PROFILE_STATE_CIS_CONNECTING:
    case MIRROR_PROFILE_STATE_CIS_CONNECTED:
        if (MirrorProfile_IsReqdCisInReadyToConnectState(lea_unicast))
        {
            /* Retry - try again later */
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
        }
        else if (!MirrorProfile_IsAnyCisInDisconnectingState(lea_unicast) &&
                 MirrorProfile_GetState() == MIRROR_PROFILE_STATE_CIS_CONNECTED)
        {
            MessageSend(MirrorProfile_GetTask(), MIRROR_INTERNAL_KICK_TARGET_STATE, NULL);
        }
        break;

    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
    case MIRROR_PROFILE_STATE_DISCONNECTED:
        /* We are in the process of ACL connecting (either in progress/failed), so ignore the CIS disconnection */
        break;

    default:
        MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnPrimary invalid state: %d", MirrorProfile_GetState());
        break;
    }
}

/*! \brief Handle MDM_LE_CIS_DISCONNECT_IND on secondary

    \param ind Message body
*/

static void mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary(const MDM_LE_CIS_DISCONNECT_IND_T *ind)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();

    MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary handle: 0x%x, reason: 0x%x", ind->conn_handle, ind->reason);

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_CIS_CONNECTED:
        if (ind->conn_handle == lea_unicast->own_cis_handle)
        {
            MirrorProfile_SetOwnCisState(MIRROR_PROFILE_CIS_SUB_STATE_IDLE);

            MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary stop voice: %d, audio: %d",
                      lea_unicast->audio_config.voice_source, lea_unicast->audio_config.audio_source);

            MirrorProfile_StopLeUnicastAudio();
        }
        else if (ind->conn_handle == lea_unicast->peer_cis_handle)
        {
            if (lea_unicast->audio_config.mirror_type == le_um_cis_mirror_type_mirror)
            {
                MirrorProfile_StopLeUnicastAudio();
            }

            MirrorProfile_SetPeerCisState(MIRROR_PROFILE_CIS_SUB_STATE_IDLE);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary invalid cis handle");
            assert(0);
            return;
        }

        if (MirrorProfile_IsBothCisInIdleState(lea_unicast))
        {
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
        }
        break;

    default:
        MIRROR_LOG("mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary invalid state: %d", MirrorProfile_GetState());
        break;
    }
}

void MirrorProfile_HandleSetupIsoDataPathCfm(const CmIsocSetupIsoDataPathCfm *cfm)
{
    mirror_profile_lea_unicast_t *lea_unicast = MirrorProfile_GetLeaUnicastState();
    bool start_le_audio_graph = FALSE;

    MIRROR_LOG("MirrorProfile_HandleSetupIsoDataPathCfm handle: 0x%x, reason: 0x%x, is_own_handle: %d, mirror type enum:le_um_cis_mirror_type_t:%d",
               cfm->handle, cfm->resultCode, cfm->handle == lea_unicast->own_cis_handle, lea_unicast->audio_config.mirror_type);

    if (cfm->handle == lea_unicast->own_cis_handle)
    {
        MirrorProfile_SetOwnCisState(lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_2 ?
                                     MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_1 : MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED);
    }
    else if (cfm->handle == lea_unicast->peer_cis_handle)
    {
        MirrorProfile_SetPeerCisState(lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_2 ?
                                     MIRROR_PROFILE_CIS_SUB_STATE_DATA_PATH_EST_PENDING_1 : MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED);
    }
    else
    {
        /* CIS got disconnected earlier, so ignore it */
        return;
    }

    switch (lea_unicast->audio_config.mirror_type)
    {
        case le_um_cis_mirror_type_mirror:
            /*  Single CIS mirroring in which case either of the CIS has to be connected to start the audio graph */
            if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED ||
                lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED)
            {
                start_le_audio_graph = TRUE;
            }
            break;

        case le_um_cis_mirror_type_delegate:
            /* In case of delegate type ensure that graph is started only when own CIS is connected */
            if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED && cfm->handle == lea_unicast->own_cis_handle)
            {
                start_le_audio_graph = TRUE;
            }
            break;

        case le_um_cis_mirror_type_delegate_with_left_src_shared:
        case le_um_cis_mirror_type_delegate_with_right_src_shared:
        case le_um_cis_mirror_type_delegate_with_left_snk_shared:
        case le_um_cis_mirror_type_delegate_with_right_snk_shared:
            /* Check if both the CISes are connected in case of delegate with source cis shared */
            if (lea_unicast->own_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED &&
                lea_unicast->peer_cis_state == MIRROR_PROFILE_CIS_SUB_STATE_CONNECTED)
            {
                start_le_audio_graph = TRUE;
            }
            break;

        default :
            /* Waiting for all CIS data path establishement */
            break;
    }

    if (start_le_audio_graph)
    {
        MirrorProfile_StartLeUnicastAudio();
    }
}

void MirrorProfile_HandleRemoveIsoDataPathCfm(const CmIsocRemoveIsoDataPathCfm *cfm)
{
    MIRROR_LOG("MirrorProfile_HandleRemoveIsoDataPathCfm handle: 0x%x, reason: 0x%x", cfm->handle, cfm->resultCode);
}

#endif /* ENABLE_LEA_CIS_DELEGATION */

/*! \brief Handle MESSAGE_BLUESTACK_MDM_PRIM payloads. */
void MirrorProfile_HandleMessageBluestackMdmPrim(const MDM_UPRIM_T *uprim)
{
    switch (uprim->type)
    {
    case MDM_SET_BREDR_SLAVE_ADDRESS_IND:
        break;

    case MDM_REGISTER_CFM:
        mirrorProfile_HandleMdmRegisterCfm((const MDM_REGISTER_CFM_T *)uprim);
        break;

    case MDM_ACL_LINK_CREATE_CFM:
        mirrorProfile_HandleMdmAclLinkCreateCfm((const MDM_ACL_LINK_CREATE_CFM_T *)uprim);
        break;

    case MDM_ACL_LINK_CREATE_IND:
        mirrorProfile_HandleMdmAclLinkCreateInd((const MDM_ACL_LINK_CREATE_IND_T *)uprim);
        break;

    case MDM_ESCO_LINK_CREATE_CFM:
        mirrorProfile_HandleMdmEscoLinkCreateCfm((const MDM_ESCO_LINK_CREATE_CFM_T *)uprim);
        break;

    case MDM_ESCO_LINK_CREATE_IND:
        mirrorProfile_HandleMdmEscoLinkCreateInd((const MDM_ESCO_LINK_CREATE_IND_T *)uprim);
        break;

    case MDM_LINK_DISCONNECT_CFM:
        mirrorProfile_HandleMdmLinkDisconnectCfm((const MDM_LINK_DISCONNECT_CFM_T *)uprim);
        break;

    case MDM_LINK_DISCONNECT_IND:
        mirrorProfile_HandleMdmLinkDisconnectInd((const MDM_LINK_DISCONNECT_IND_T *)uprim);
        break;

    case MDM_ESCO_RENEGOTIATED_IND:
        mirrorProfile_HandleMdmEscoRenegotiatedInd((const MDM_ESCO_RENEGOTIATED_IND_T *)uprim);
        break;

    case MDM_L2CAP_CREATE_IND:
        mirrorProfile_HandleMdmMirrorL2capCreateInd((const MDM_L2CAP_CREATE_IND_T *)uprim);
        break;

    case MDM_L2CAP_CREATE_CFM:
        mirrorProfile_HandleMdmMirrorL2capCreateCfm((const MDM_L2CAP_CREATE_CFM_T *)uprim);
        break;

    case MDM_L2CAP_DISCONNECT_IND:
        mirrorProfile_HandleMdmMirrorL2capDisconnectInd((const MDM_L2CAP_DISCONNECT_IND_T *)uprim);
        break;

    case MDM_L2CAP_DISCONNECT_CFM:
        mirrorProfile_HandleMdmMirrorL2capDisconnectCfm((const MDM_L2CAP_DISCONNECT_CFM_T *)uprim);
        break;

    case MDM_L2CAP_DATA_SYNC_IND:
        mirrorProfile_HandleMdmMirrorL2capDataSyncInd((const MDM_L2CAP_DATA_SYNC_IND_T *)uprim);
        break;

#ifdef ENABLE_LEA_CIS_DELEGATION
    case MDM_LE_CIS_CREATE_CFM:
        mirrorProfile_HandleMdmLeCisCreateCfm((const MDM_LE_CIS_CREATE_CFM_T *)uprim);
        break;

    case MDM_LE_CIS_CREATE_IND:
        mirrorProfile_HandleMdmLeCisCreateInd((const MDM_LE_CIS_CREATE_IND_T *)uprim);
        break;

    case MDM_LE_CIS_DISCONNECT_CFM:
        mirrorProfile_HandleMdmLeCisDisconnectCfm((const MDM_LE_CIS_DISCONNECT_CFM_T *)uprim);
        break;

#ifdef INCLUDE_CIS_MIRRORING
    case MDM_LE_SET_RELAY_ENABLE_CFM:
        mirrorProfile_HandleMdmLeSetRelayEnableCfm((const MDM_LE_SET_RELAY_ENABLE_CFM_T *)uprim);
        break;

    case MDM_LE_RELAY_ENABLE_IND:
        mirrorProfile_HandleMdmLeRelayEnableInd((const MDM_LE_RELAY_ENABLE_IND_T *)uprim);
        break;
#endif /* INCLUDE_CIS_MIRRORING */

    case MDM_LE_CIS_DISCONNECT_IND:
        if (MirrorProfile_IsPrimary())
        {
            mirrorProfile_HandleMdmLeCisDisconnectIndOnPrimary((const MDM_LE_CIS_DISCONNECT_IND_T *) uprim);
        }
        else
        {
            mirrorProfile_HandleMdmLeCisDisconnectIndOnSecondary((const MDM_LE_CIS_DISCONNECT_IND_T *) uprim);
        }
        break;
#endif /* ENABLE_LEA_CIS_DELEGATION */

    default:
        MIRROR_LOG("MirrorProfile_HandleMessageBluestackMdmPrim: Unhandled MDM prim 0x%x", uprim->type);
        break;
    }
}

#endif /* INCLUDE_MIRRORING */
