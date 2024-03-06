/* Copyright (c) 2019 - 2022 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_challenge.h"
#include "gatt_root_key_server_db.h"
#include "gatt_root_key_server_state.h"
#include "gatt_root_key_server_indicate.h"
#include "gatt_root_key_server_access.h"

#include <util.h>
#include <bdaddr.h>
#include <cryptovm.h>
#include <gatt_root_key_service.h>


static void gattRootKeyGenerateHashA(GATT_ROOT_KEY_SERVER *instance)
{
    DEBUG_LOG("gattRootKeyServerGenerateHashA");
    //uint32 *s32 = (uint32*)&instance->secret;
    uint32 *r32 = (uint32*)&instance->remote_random;
    uint32 *l32 = (uint32*)&instance->local_random;
    DEBUG_LOG("RRandom %08x %08x %08x %08x LRandom %08x %08x %08x %08x",r32[0],r32[1],r32[2],r32[3],l32[0],l32[1],l32[2],l32[3]);
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->remote_random, &instance->local_random,
                                   &instance->remote_address, &instance->local_address,
                                   &instance->hashA_out);
}


static void gattRootKeyGenerateHashB(GATT_ROOT_KEY_SERVER *instance,GRKS_KEY_T *hash)
{
    DEBUG_LOG("gattRootKeyServerGenerateHashB");
    uint32 *r32 = (uint32*)&instance->remote_random;
    uint32 *l32 = (uint32*)&instance->local_random;
    DEBUG_LOG("RRandom %08x %08x %08x %08x LRandom %08x %08x %08x %08x",r32[0],r32[1],r32[2],r32[3],l32[0],l32[1],l32[2],l32[3]);
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->local_random,  &instance->remote_random,
                                   &instance->local_address, &instance->remote_address,
                                   hash);
}

static bool gattRootKeyCheckHash(GATT_ROOT_KEY_SERVER *instance)
{
    GRKS_KEY_T hash;
    gattRootKeyGenerateHashB(instance, &hash);

    uint32 *src32 = (uint32*)&instance->hashB_in;
    DEBUG_LOG("HASH COMPARE (S)");
    DEBUG_LOG("     REMOTE: %08x %08x %08x %08x",src32[0],src32[1],src32[2],src32[3]);
    src32 = (uint32*)&hash;
    DEBUG_LOG("GENERATED-R: %08x %08x %08x %08x",src32[0],src32[1],src32[2],src32[3]);

    bool match = (0 == memcmp(&instance->hashB_in, &hash, sizeof(hash)));

//    gattRootKeyGenerateLocalHash(instance, &hash);
    src32 = (uint32*)&instance->hashA_out;
    DEBUG_LOG("GENERATED-L: %08x %08x %08x %08x",src32[0],src32[1],src32[2],src32[3]);

    return match;
}


void gattRootKeySendChallengeInd(GATT_ROOT_KEY_SERVER *instance, 
                                 gatt_root_key_challenge_status_t status_code)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_SERVER_CHALLENGE_IND);

    message->instance = instance;
    message->status = status_code;

    MessageSend(instance->app_task, GATT_ROOT_KEY_SERVER_CHALLENGE_IND, message);

    instance->challenge_response_needed = FALSE;
}


void handleInternalChallengeWrite(const ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE_T *write)
{
    GATT_ROOT_KEY_SERVER *instance = write->instance;

    switch (write->opcode)
    {
        case GattRootKeyScOpcodeIncomingRequest:
            if (root_key_server_initialised == gattRootKeyServerGetState(instance))
            {
                instance->remote_random = write->value;
                
                GattDbWriteAccessResSend(instance->gattId,
                                         write->cid,
                                         HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT,
                                         CSR_BT_GATT_ACCESS_RES_SUCCESS);
                                         
                gattRootKeyServerSendChallengeIndication(instance, write->cid,
                                                         GattRootKeyScOpcodeOutgoingRequest, 
                                                         &instance->local_random);
                gattRootKeyServerSetState(instance, root_key_server_responded_random);
                return;
            }
            break;

        case GattRootKeyScOpcodeIncomingResponse:
            if (root_key_server_awaiting_hash == gattRootKeyServerGetState(instance))
            {
                instance->hashB_in = write->value;

                gattRootKeyGenerateHashA(instance);

                /*! \todo What do we do with a mismatched hash */
                GattDbWriteAccessResSend(instance->gattId,
                                         write->cid,
                                         HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT,
                                         CSR_BT_GATT_ACCESS_RES_SUCCESS);
                                         
                gattRootKeyServerSendChallengeIndication(instance, write->cid,
                                                         GattRootKeyScOpcodeOutgoingResponse,
                                                         &instance->hashA_out);
                if (gattRootKeyCheckHash(instance))
                {
                    gattRootKeyServerSetState(instance, root_key_server_responded_hash);
                }
                else
                {
                    gattRootKeyServerSetState(instance, root_key_server_error);
                }
                return;
            }
            break;

        default:
            GATT_ROOT_KEY_SERVER_DEBUG("handleInternalChallengeWrite. Unexpected opcode:%d",write->opcode);
            break;
    }

    GattDbWriteAccessResSend(instance->gattId,
                             write->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT,
                             CSR_BT_GATT_ACCESS_RES_INVALID_PDU);
}


void handleInternalKeysWrite(const ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE_T *write)
{
    GATT_ROOT_KEY_SERVER *instance = write->instance;

    switch (write->opcode)
    {
        case GattRootKeyKeysOpcodePrepareIr:
            if (root_key_server_authenticated == gattRootKeyServerGetState(instance))
            {
                instance->ir_key = write->value;

                GattDbWriteAccessResSend(instance->gattId,
                                         write->cid,
                                         HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL,
                                         CSR_BT_GATT_ACCESS_RES_SUCCESS);
                                         
                gattRootKeyServerSetState(instance, root_key_server_ir_received);
                return;
            }
            break;

        case GattRootKeyKeysOpcodePrepareEr:
            if (root_key_server_ir_received == gattRootKeyServerGetState(instance))
            {
                instance->er_key = write->value;

                GattDbWriteAccessResSend(instance->gattId,
                                         write->cid,
                                         HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL,
                                         CSR_BT_GATT_ACCESS_RES_SUCCESS);
                                         
                gattRootKeyServerSetState(instance, root_key_server_er_received);
                return;
            }
            break;

        default:
            GATT_ROOT_KEY_SERVER_DEBUG("handleInternalKeysWrite. Unexpected opcode:%d",write->opcode);
            break;
    }

    GattDbWriteAccessResSend(instance->gattId,
                             write->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL,
                             CSR_BT_GATT_ACCESS_RES_INVALID_PDU);
}


void handleInternalKeysCommit(const ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT_T *commit)
{
    GATT_ROOT_KEY_SERVER *instance = commit->instance;

    if (root_key_server_er_received == gattRootKeyServerGetState(instance))
    {
        MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND);

        instance->commit_cid = commit->cid;
        message->instance = instance;
        message->ir = instance->ir_key;
        message->er = instance->er_key;

        MessageSend(instance->app_task, GATT_ROOT_KEY_SERVER_KEY_UPDATE_IND,
                    message);

        gattRootKeyServerSetState(instance, root_key_server_keys_exchanged);
        return;
    }

    GattDbWriteAccessResSend(instance->gattId,
                             commit->cid,
                             HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL,
                             CSR_BT_GATT_ACCESS_RES_INVALID_PDU);

}


bool GattRootKeyServerReadyForChallenge(GATT_ROOT_KEY_SERVER *instance,
                                        const GRKS_KEY_T *secret,
                                        const bdaddr   *local_address,
                                        const bdaddr   *remote_address)
{
    if (!instance || !secret || !local_address || !remote_address)
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerReadyForChallenge Null pointer passed");
        return FALSE;
    }

    if (BdaddrIsZero(local_address) || BdaddrIsZero(remote_address))
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerReadyForChallenge zero bdaddr passed");
        return FALSE;
    }

    if (gattRootKeyServerGetState(instance) > root_key_server_initialised)
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerReadyForChallenge Not in correct state. State:%d",
                        gattRootKeyServerGetState(instance));
        return FALSE;
    }

    if (gattRootKeyServerGetState(instance) != root_key_server_idle)
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerReadyForChallenge Not idle. State:%d",
                        gattRootKeyServerGetState(instance));
        return FALSE;
    }

    instance->secret = *secret;
    instance->local_address = *local_address;
    instance->remote_address = *remote_address;
    
    gattRootKeyServiceGenerateRandom(&instance->local_random);

    DEBUG_LOG("GattRootKeyServerReadyForChallenge: LAddress%04x %02x %06x RAddress%04x %02x %06x",
               local_address->nap, local_address->uap, local_address->lap,remote_address->nap, remote_address->uap, remote_address->lap);

    gattRootKeyServerSetState(instance, root_key_server_initialised);
    instance->challenge_response_needed = TRUE;

    return TRUE;
}


bool GattRootKeyServerKeyUpdateResponse(const GATT_ROOT_KEY_SERVER *instance)
{
    GATT_ROOT_KEY_SERVER *server_inst = NULL;
    
    if (gattRootKeyServerGetState(instance) == root_key_server_keys_exchanged)
    {
        if (instance->commit_cid != INVALID_COMMIT_CID)
        {
            GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerKeyUpdateResponse Sending access rsp for cid:0x%x",
                            instance->commit_cid);

            GattDbWriteAccessResSend(instance->gattId,
                                     instance->commit_cid,
                                     HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL,
                                     CSR_BT_GATT_ACCESS_RES_SUCCESS);
                                     
            server_inst = (GATT_ROOT_KEY_SERVER *)instance;
            server_inst->commit_cid = INVALID_COMMIT_CID;

            return TRUE;
        }
    }
    
    GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerKeyUpdateResponse Not in correct state. State:%d",
                        gattRootKeyServerGetState(instance));
    return FALSE;
}
