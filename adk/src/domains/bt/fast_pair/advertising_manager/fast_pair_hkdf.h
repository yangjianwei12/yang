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

# Overview

The HKDF function is a re-implementiation in C of the python algortihm used in
https://github.com/casebeer/python-hkdf .
*/

#ifndef FAST_PAIR_HKDF_H
#define FAST_PAIR_HKDF_H


#include <stdlib.h>

/*! \brief Structure containing parameters used by FastPair_HkdfSha256  

    \param[in] salt The optional salt value (a non-secret random value);
                    if not provided (salt == NULL), it is set internally
                    to a string of SHA256_DIGEST_SIZE zeros.
    \param[in] salt_len The length of the salt value.  (Ignored if salt == NULL.)
    \param[in] ikm Input keying material.
    \param[in] ikm_len The length of the input keying material.
    \param[in] info The optional context and application specific information.
                    If info == NULL or a zero-length string, it is ignored.
    \param[in] info_len The length of the optional context and application specific
                        information.  (Ignored if info == NULL.)
    \param[out] okm Where the HKDF is to be stored.
    \param[in] okm_len The length of the buffer to hold okm.
                       okm_len must be <= 255 * SHA256_DIGEST_SIZE
*/
typedef struct fast_pair_hkdf
{
   const uint8 * salt; 
   const uint8 * ikm;
   const uint8 * info;
   uint8 * okm; 
   uint16 salt_len;
   uint16 ikm_len;
   uint16 info_len;
   uint16 okm_len;
} fastpair_hkdf_t;


/*! \brief This function will generate keying material using HKDF and SHA256.

    \param[in] params fastpair_hkdf_t structure containing parameters required by HKDF
*/
bool FastPair_HkdfSha256(fastpair_hkdf_t * params);

#endif // FAST_PAIR_HKDF_H
