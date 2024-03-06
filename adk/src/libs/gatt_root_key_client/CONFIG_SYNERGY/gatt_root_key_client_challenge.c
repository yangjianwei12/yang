/* Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <util.h>

#include <cryptovm.h>

#include "gatt_root_key_client_challenge.h"


void gattRootKeyGenerateHashB(GATT_ROOT_KEY_CLIENT *instance)
{
    uint32 *r32 = (uint32*)&instance->remote_random;
    uint32 *l32 = (uint32*)&instance->local_random;

    DEBUG_LOG("gattRootKeyClientGenerateHashB");
    DEBUG_LOG("RRandom %08x %08x %08x %08x LRandom %08x %08x %08x %08x",r32[0],r32[1],r32[2],r32[3],l32[0],l32[1],l32[2],l32[3]);
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->remote_random, &instance->local_random,
                                   &instance->remote_address, &instance->local_address,
                                   &instance->hashB_out);
}


static void gattRootKeyGenerateHashA(GATT_ROOT_KEY_CLIENT *instance, GRKS_KEY_T *hash)
{
    uint32 *r32 = (uint32*)&instance->remote_random;
    uint32 *l32 = (uint32*)&instance->local_random;

    DEBUG_LOG("gattRootKeyClientGenerateHashA");
    DEBUG_LOG("RRandom %08x %08x %08x %08x LRandom %08x %08x %08x %08x",r32[0],r32[1],r32[2],r32[3],l32[0],l32[1],l32[2],l32[3]);
    gattRootKeyServiceGenerateHash(&instance->secret,
                                   &instance->local_random,  &instance->remote_random,
                                   &instance->local_address, &instance->remote_address,
                                   hash);
}


bool gattRootKeyCheckHash(GATT_ROOT_KEY_CLIENT *instance)
{
    GRKS_KEY_T hash;
    gattRootKeyGenerateHashA(instance, &hash);

    return 0 == memcmp(&instance->hashA_in, &hash, sizeof(hash));
}

