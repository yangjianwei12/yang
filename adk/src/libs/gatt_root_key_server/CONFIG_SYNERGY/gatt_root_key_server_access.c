/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_access.h"
#include "gatt_root_key_server_db.h"
#include "gatt_root_key_server_challenge.h"

#include <string.h>

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_OCTET_SIZE sizeof(uint8) * 2


static void sendRootKeyReadAccessErrorRsp(const GATT_ROOT_KEY_SERVER *instance, 
                                           const CsrBtGattDbAccessReadInd *access_ind, 
                                           uint16 error)
{
    GattDbReadAccessResSend(instance->gattId,
                            access_ind->btConnId,
                            access_ind->attrHandle,
                            error,
                            0,
                            NULL);
}

static void sendInternalChallengeWriteMessage(GATT_ROOT_KEY_SERVER *instance, 
                                              uint32 cid,
                                              GattRootKeyServiceMirrorChallengeControlOpCode opcode,
                                              const uint8 *key)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE);

    message->instance = instance;
    message->cid = cid;
    message->opcode = opcode;
    memcpy(message->value.key, key, sizeof(message->value.key));

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_CHALLENGE_WRITE, message);
}


static void sendInternalKeysWriteMessage(GATT_ROOT_KEY_SERVER *instance, 
                                         uint32 cid,
                                         GattRootKeyServiceKeysControlOpCode opcode,
                                         const uint8 *key)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE);

    message->instance = instance;
    message->cid = cid;
    message->opcode = opcode;
    memcpy(message->value.key, key, sizeof(message->value.key));

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_KEYS_WRITE, message);
}


static void sendInternalKeysCommitMessage(GATT_ROOT_KEY_SERVER *instance, uint32 cid)
{
    MAKE_ROOT_KEY_MESSAGE(ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT);

    message->instance = instance;
    message->cid = cid;

    MessageSend(&instance->lib_task, ROOT_KEY_SERVER_INTERNAL_KEYS_COMMIT, message);
}

static void rootKeyMirrorControlPointWriteAccess(GATT_ROOT_KEY_SERVER *instance, 
                                            const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;

    if (access_ind->writeUnit[0].valueLength == GRKS_SIZE_CONTROL_WITH_KEY_OCTETS)
    {
        uint8 opcode = access_ind->writeUnit[0].value[GRKS_OFFSET_OPCODE];
        response = CSR_BT_GATT_ACCESS_RES_INVALID_PDU;

        switch (opcode)
        {
            case GattRootKeyScOpcodeIncomingRequest:
            case GattRootKeyScOpcodeIncomingResponse:
                 sendInternalChallengeWriteMessage(instance,
                                                   access_ind->btConnId,
                                                   opcode,
                                                   &access_ind->writeUnit[0].value[1]);
                return;
                
            default:
                break;
        }
    }

    /*! \todo Probably need to only send this here for some limited error cases
        and have the app/profile send the remainder */
    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);

}

static void rootKeyKeysControlPointWriteAccess(GATT_ROOT_KEY_SERVER *instance, 
                                          const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;

    if (access_ind->writeUnit[0].valueLength >= GRKS_SIZE_JUST_OPCODE_OCTETS)
    {
        uint8 opcode = access_ind->writeUnit[0].value[GRKS_OFFSET_OPCODE];
        response = CSR_BT_GATT_ACCESS_RES_INVALID_PDU;

        switch (opcode)
        {
            case GattRootKeyKeysOpcodePrepareIr:
            case GattRootKeyKeysOpcodePrepareEr:
                if (access_ind->writeUnit[0].valueLength == GRKS_SIZE_CONTROL_WITH_KEY_OCTETS)
                {
                    sendInternalKeysWriteMessage(instance, access_ind->btConnId,
                                                 opcode, &access_ind->writeUnit[0].value[1]);
                    return;
                }
                //response = gatt_status_key_missing; TODO
                break;

            case GattRootKeyKeysOpcodeExecuteRootUpdate:
                if (access_ind->writeUnit[0].valueLength == GRKS_SIZE_JUST_OPCODE_OCTETS)
                {
                    sendInternalKeysCommitMessage(instance, access_ind->btConnId);
                    return;
                }
                break;

            default:
                break;
        }
    }

   /*! \todo Probably need to only send this here for some limited error cases
            and have the app/profile send the remainder */
    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);
}


/*!
    Handle access to the client config option

    Read and write of the values are handled by the server.

    \todo MAY need to add notification to the application to allow
    the setting to be persisted.
*/
static void rootKeyClientConfigWriteAccess(GATT_ROOT_KEY_SERVER *instance,
                                      const CsrBtGattDbAccessWriteInd *access_ind)
{
    uint16 response = CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH;
    if (access_ind->writeUnit[0].valueLength == GATT_CLIENT_CONFIG_OCTET_SIZE)
    {
        uint16 config_value = (access_ind->writeUnit[0].value[0] & 0xFF) 
                              + ((access_ind->writeUnit[0].value[1] << 8) & 0xFF00);

        instance->mirror_client_config = config_value;

        response = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    GattDbWriteAccessResSend(instance->gattId,
                             access_ind->btConnId,
                             access_ind->attrHandle,
                             response);
}

/***************************************************************************/
void handleRootKeyReadAccess(GATT_ROOT_KEY_SERVER *instance,
                             const CsrBtGattDbAccessReadInd *access_ind)
{
    GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyReadAccess: handle %d",access_ind->attrHandle);
    uint16 handle = access_ind->attrHandle;
    uint16 size_value = 0;
    uint8  *value = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_SUCCESS;

    switch (handle)
    {
        case HANDLE_ROOT_TRANSFER_SERVICE:
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_FEATURES:
        {
            value = (uint8*) CsrPmemAlloc(GRKS_SIZE_FEATURES_OCTETS);
            value[0] = instance->features;
            value[1] = (instance->features >> 8);
            size_value = GRKS_SIZE_FEATURES_OCTETS;
        }
        break;

        case HANDLE_ROOT_TRANSFER_SERVICE_STATUS:
        {
            value = (uint8*) CsrPmemAlloc(GRKS_SIZE_STATUS_OCTETS);
            value[0] = instance->status;
            size_value = GRKS_SIZE_STATUS_OCTETS;
        }
        break;

        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT_CLIENT_CONFIG:
        {
            value = (uint8*) CsrPmemAlloc(GATT_CLIENT_CONFIG_OCTET_SIZE);
            value[0] = (instance->mirror_client_config) & 0xFF;
            value[1] = ((instance->mirror_client_config) >> 8) & 0xFF;
            size_value = GATT_CLIENT_CONFIG_OCTET_SIZE;
        }
        break;
        
        default:
            sendRootKeyReadAccessErrorRsp(instance, access_ind, CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }

    GattDbReadAccessResSend(instance->gattId, access_ind->btConnId, 
                            access_ind->attrHandle,
                            status, size_value,
                            value);
}

/***************************************************************************/
void handleRootKeyWriteAccess(GATT_ROOT_KEY_SERVER *instance,
                              const CsrBtGattDbAccessWriteInd *access_ind)
{
    GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyWriteAccess: handle %d",access_ind->attrHandle);
    uint16 handle = access_ind->attrHandle;

    switch (handle)
    {
        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT:
            rootKeyMirrorControlPointWriteAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_KEYS_CONTROL:
            rootKeyKeysControlPointWriteAccess(instance, access_ind);
            break;

        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT_CLIENT_CONFIG:
            rootKeyClientConfigWriteAccess(instance, access_ind);
            break;
        
        default:
            GattDbWriteAccessResSend(instance->gattId,
                                     access_ind->btConnId,
                                     access_ind->attrHandle,
                                     CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        break;
    }
}

