/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.

            Copyright (c) 2012 Christopher H. Casebeer. All rights reserved.

            Redistribution and use in source and binary forms, with or without
            modification, are permitted provided that the following conditions are met:

               1. Redistributions of source code must retain the above copyright notice,
                  this list of conditions and the following disclaimer.

               2. Redistributions in binary form must reproduce the above copyright notice,
                  this list of conditions and the following disclaimer in the documentation
                  and/or other materials provided with the distribution.

            THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
            ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
            WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
            DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
            FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
            DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
            SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
            CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
            OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
            OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

\version    
\file
\brief      Implementation of HKDF using SHA256 as the hashing function.
*/

#include <string.h>
#include <stdlib.h>

#include <cryptoalgo.h>
#include <panic.h>

#include "fast_pair_hkdf.h"



static bool FastPair_HkdfExtractSha256(fastpair_hkdf_t * params, uint8 *prk, uint16 prk_len);

static bool FastPair_HkdfExpandSha256(const uint8 *prk, uint16 prk_len, fastpair_hkdf_t * params);

bool FastPair_HkdfSha256(fastpair_hkdf_t * params)
{
    bool ret = FALSE;
    uint8 *prk = PanicUnlessMalloc(SHA256_DIGEST_SIZE);

    ret = FastPair_HkdfExtractSha256(params, prk, SHA256_DIGEST_SIZE);

    if (ret)
    {
        ret = FastPair_HkdfExpandSha256(prk, SHA256_DIGEST_SIZE, params);
    }

    free(prk);

    return ret;
}

static bool FastPair_HkdfExtractSha256(fastpair_hkdf_t * params, uint8 *prk, uint16 prk_len)
{
    bool ret = FALSE;
    uint8 *nullSalt = NULL;

    if (prk_len != SHA256_DIGEST_SIZE)
    {
        return FALSE;
    }

    if (params->salt == 0)
    {
        params->salt_len = prk_len;

        nullSalt = PanicUnlessMalloc(params->salt_len);
        memset(nullSalt, 0, params->salt_len);

        params->salt = nullSalt;
    }
    else if (params->salt_len < 0)
    {
        return FALSE;
    }

    ret = hmac_sha256((uint8 *)params->ikm, params->ikm_len, prk, (uint8 *)params->salt, params->salt_len);

    if (nullSalt)
    {
        free(nullSalt);
    }

    return ret;
}

static bool FastPair_HkdfExpandSha256(const uint8 *prk, uint16 prk_len, fastpair_hkdf_t * params)
{
    uint16 hash_len = SHA256_DIGEST_SIZE;
    uint16 N = 0;

    if (params->info == 0)
    {
        params->info = (const unsigned char *)"";
        params->info_len = 0;
    }
    else if (params->info_len < 0)
    {
        return FALSE;
    }

    if (params->okm_len <= 0) return FALSE;
    if (!params->okm) return FALSE;

    if (prk_len < hash_len)
    {
        return FALSE;
    }

    N = params->okm_len / hash_len;
    if ((params->okm_len % hash_len) != 0)
    {
        N++;
    }

    if (N > 255) return FALSE;

    uint8 T[SHA256_DIGEST_SIZE] = {0};
    uint16 Tlen = 0;
    uint16 where = 0;
    uint16 buffer_len = (SHA256_DIGEST_SIZE + params->info_len + sizeof(uint8));
    uint16 buffer_pos = 0;
    uint8 *buffer = (uint8 *)PanicUnlessMalloc(buffer_len);

    for (uint16 i = 1; i <= N; i++)
    {
        uint8 c = i;

        buffer_pos = 0;

        if (Tlen)
        {
            memmove(buffer + buffer_pos, T, Tlen);
            buffer_pos += Tlen;
        }

        memmove(buffer + buffer_pos, params->info, params->info_len);
        buffer_pos += params->info_len;

        memmove(buffer + buffer_pos, &c, sizeof(c));
        buffer_pos += sizeof(uint8);

        if (!hmac_sha256(buffer, buffer_pos, T, (uint8*)prk, prk_len))
        {
            return FALSE;
        }

        memmove(params->okm + where, T,
               (i != N) ? hash_len : (params->okm_len - where));
        where += hash_len;
        Tlen = hash_len;
    }

    free(buffer);

    return TRUE;
}
